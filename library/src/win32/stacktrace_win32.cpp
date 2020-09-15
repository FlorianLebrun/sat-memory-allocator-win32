#include <windows.h>
#include <psapi.h>
#include <malloc.h>
#include <stdio.h>
#include "../base.h"
#include "../utils/chrono.h"

#include <Dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

extern std::atomic<intptr_t> _windows_loader_working;

struct NativeStackMarker : SAT::IStackMarker {
  uintptr_t base;
  uintptr_t callsite;
  virtual void getSymbol(char* buffer, int size) override {
    SystemThreading::tSymbol symbol(buffer, size);
    if(!symbol.resolve(this->base)) {
      sprintf_s(buffer, size, "0x%.8lX", uint64_t(this->base));
    }
  }
};

bool ReadThreadContext(HANDLE hThread, CONTEXT& context) {

  // Prevent loader deadlock
  if(_windows_loader_working.load()) {
    //printf("'Windows Loader Working' thread analysis is unsafe\n");
    return false;
  }

  // Read thread context
  if (::GetThreadContext(hThread, &context) == 0) {
    printf("'GetThreadContext' has failed\n");
    return false;
  }
  return true;
}


struct tTimes {
  double suspend_time;
  double context_time;
  double unwind_lookup_time;
  double unwind_move_time;
  double unwind_time;
  double stackstamp_time;
  int64_t stackstamp_frames;
  double total_time;
  int64_t count;
  tTimes(){reset();}
  void add(tTimes& t) {
    suspend_time += t.suspend_time;
    context_time += t.context_time;
    unwind_lookup_time += t.unwind_lookup_time;
    unwind_move_time += t.unwind_move_time;
    unwind_time += t.unwind_time;
    stackstamp_time += t.stackstamp_time;
    stackstamp_frames += t.stackstamp_frames;
    total_time += t.total_time;
    count++;
  }
  void print() {
    printf("------------- Times (us) ----------\n");
    printf("suspend_time: %lg\n", suspend_time/double(count));
    printf("context_time: %lg\n", context_time/double(count));
    printf("\n");
    printf("unwind_lookup_time: %lg\n", unwind_lookup_time/double(count));
    printf("unwind_move_time: %lg\n", unwind_move_time/double(count));
    printf("unwind_time: %lg\n", unwind_time/double(count));
    printf("\n");
    printf("stackstamp_time: %lg\n", stackstamp_time/double(count));
    printf("stackstamp_frames: %ld\n", stackstamp_frames/count);
    printf("\n");
    printf("total_time: %lg\n", total_time/double(count));
    printf("-----------------------------------\n");
  }
  void reset() {
    memset(this,0,sizeof(*this));
  }
};

__declspec(noinline) bool SAT::Thread::CaptureThreadStackStamp(SAT::IStackStampBuilder& stack_builder) {
  const int maxStackFrames = 1024;
  char _frames_buffer[sizeof(SAT::StackMarker)*maxStackFrames];
  bool useCompactMarker = true;
  bool captureOtherThread = this->handle.id != GetCurrentThreadId();

  static tTimes times;
  Chrono c, ct, cs;
  tTimes delta;
  
  // Get thread cpu context
  CONTEXT context;
  memset(&context, 0, sizeof(context));
  context.ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER;
  if (captureOtherThread) {

    // Suspend thread
    if (::SuspendThread(this->handle.handle) < 0) {
      return 0;
    }

    delta.suspend_time = c.GetDiffDouble(Chrono::US);
    c.Start();

    // Read thread context
    if (!ReadThreadContext(this->handle.handle, context)) {
      ::ResumeThread(this->handle.handle);
      return 0;
    }

    delta.context_time = c.GetDiffDouble(Chrono::US);
    c.Start();
  }
  else {
    RtlCaptureContext(&context);
  }

  // Initiate markers stack
  int markerCount = 0;
  SAT::StackBeacon* currentBeacon = this->stackBeaconsCount?this->stackBeacons[stackBeaconsCount-1]:0;
  SAT::StackMarker* markers = (SAT::StackMarker*)&_frames_buffer[sizeof(SAT::StackMarker)*maxStackFrames];

  // Unwinds stackframes and stack beacons
  if (context.Rip == 0 && context.Rsp != 0) {
    context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
    context.Rsp += 8;
  }
  while (context.Rip && uintptr_t(markers) > uintptr_t(_frames_buffer))
  {
    // Add a marker for stack beacon, when detect entry into a beacon
    while(currentBeacon && uintptr_t(currentBeacon) < uintptr_t(context.Rsp)) {
      markers--;
      currentBeacon->createrMarker(*markers);
      markerCount++;
      currentBeacon = currentBeacon->parentBeacon;
    }
    
    // Goto next stackframe
    cs.Start();
    DWORD64 ImageBase, BaseAddress, InsAddress;
    PRUNTIME_FUNCTION pFunctionEntry = ::RtlLookupFunctionEntry(context.Rip, &ImageBase, NULL);
    delta.unwind_lookup_time += cs.GetDiffDouble(Chrono::US);
    cs.Start();
    if(pFunctionEntry) {
      PVOID HandlerData;
      DWORD64 EstablisherFrame;
      InsAddress = context.Rip;
      BaseAddress = ImageBase + pFunctionEntry->BeginAddress;
      ::RtlVirtualUnwind(UNW_FLAG_NHANDLER,
        ImageBase,
        InsAddress,
        pFunctionEntry,
        &context,
        &HandlerData,
        &EstablisherFrame,
        NULL);
    }
    else {
      InsAddress = context.Rip;
      BaseAddress = context.Rip;
      context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
      context.Rsp += 8;
    }
    delta.unwind_move_time += cs.GetDiffDouble(Chrono::US);

    // Add marker for stackframe, when not under a stack beacon
    if(!currentBeacon) {
      markers--;
      NativeStackMarker* nativeMarker = markers->as<NativeStackMarker>();
      nativeMarker->base = BaseAddress; // Function entrypoint
      if(!useCompactMarker) {
        nativeMarker->callsite = InsAddress; // Callsite -> Function entrypoint
      }
      markerCount++;
    }
  }
  delta.stackstamp_frames = markerCount;

  // Release thread
  if(captureOtherThread) {
    ::ResumeThread(this->handle.handle);
  }

  delta.unwind_time = c.GetDiffDouble(Chrono::US);
  c.Start();

  // Process stack locations
  for (int i = 0; i < markerCount; i++) {
    stack_builder.push(markers[i]);
  }

  delta.stackstamp_time = c.GetDiffDouble(Chrono::US);
  delta.total_time = ct.GetDiffDouble(Chrono::US);

  times.add(delta);
  if(times.count > 1000) {
    times.print();
    times.reset();
  }

  return true;
}