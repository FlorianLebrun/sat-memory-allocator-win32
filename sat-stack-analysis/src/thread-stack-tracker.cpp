#include "./stack-stamp.h"
#include "./thread-stack-tracker.h"
#include "./win32/system.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>

#include <windows.h>
#include <psapi.h>
#include <Dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

#define DEV_TRACE(x)

extern"C" uintptr_t __fastcall getRbpX64Proc();

static bool ReadThreadContext(HANDLE hThread, CONTEXT& context) {
   extern std::atomic<intptr_t> _windows_loader_working;

   // Prevent loader deadlock
   if (_windows_loader_working.load()) {
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


namespace sat {

   struct ThreadStackContext {
      CONTEXT context;

      operator bool() {
         return this->context.Rip != 0;
      }
      uintptr_t getStackAddress() {
         return context.Rsp;
      }
      uintptr_t getInsAddress() {
         return context.Rip;
      }
      uintptr_t unwind() {
         DWORD64 ImageBase;
         PRUNTIME_FUNCTION pFunctionEntry = ::RtlLookupFunctionEntry(context.Rip, &ImageBase, NULL);
         uintptr_t FunctionAddress;
         if (pFunctionEntry) {
            PVOID HandlerData;
            DWORD64 EstablisherFrame;
            FunctionAddress = ImageBase + pFunctionEntry->BeginAddress;
            ::RtlVirtualUnwind(UNW_FLAG_NHANDLER,
               ImageBase,
               this->context.Rip,
               pFunctionEntry,
               &this->context,
               &HandlerData,
               &EstablisherFrame,
               NULL);
         }
         else {
            FunctionAddress = this->context.Rip;
            this->context.Rip = (ULONG64)(*(PULONG64)this->context.Rsp);
            this->context.Rsp += 8;
         }
         return FunctionAddress;
      }
   };

   struct ThreadLocalStackContext : ThreadStackContext {
      ThreadLocalStackContext(void* addressOfReturnAddress, uintptr_t rbp) {

         // Initiate context as terminal function
         this->context.ContextFlags = CONTEXT_CONTROL;
         this->context.Rsp = (DWORD64)addressOfReturnAddress;
         this->context.Rip = 0;
         this->context.Rbp = rbp;

         // Fix stack register for some terminal function
         if (this->context.Rip == 0 && this->context.Rsp != 0) {
            this->context.Rip = (ULONG64)(*(PULONG64)this->context.Rsp);
            this->context.Rsp += 8;
         }
      }
   };

   struct ThreadCapturedStackContext : ThreadStackContext {
      HANDLE hThread;
      ThreadCapturedStackContext()
         :hThread(0)
      {
      }
      bool capture(Thread* thread) {
         HANDLE hThread = thread->handle.handle;

         // Get thread cpu context
         memset(&this->context, 0, sizeof(this->context));
         this->context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

         // Suspend thread
         if (::SuspendThread(hThread) < 0) {
            return false;
         }

         // Read thread context
         if (!ReadThreadContext(hThread, this->context)) {
            ::ResumeThread(hThread);
            return false;
         }

         this->hThread = hThread;
         return true;
      }
      void release() {
         if (this->hThread) {
            ::ResumeThread(this->hThread);
            this->hThread = 0;
         }
      }
   };


   void NativeStackMarker::getSymbol(char* buffer, int size) {
      SystemThreading::tSymbol symbol(buffer, size);
      if (!symbol.resolve(this->base)) {
         sprintf_s(buffer, size, "0x%.8lX", uint64_t(this->base));
      }
   }

   StackFrame* StackFrame::setRoot() {
      StackFrame* overflow = this->next;
      this->Kind = RootFrame;
      this->BeaconId = 0;
      this->StackAddress = 0;
      this->InsAddress = 0;
      this->node = 0;
      this->next = 0;
      return overflow;
   }

   std::string StackFrame::toString() {
      switch (this->Kind)
      {
      case StackFrame::RootFrame: {
         return "<root>";
      }
      case StackFrame::FunctionFrame: {
         SystemThreading::tSymbolEx<512> symbol;
         symbol.resolve(this->Data.FunctionAddress);
         return symbol.name;
      }
      case StackFrame::BeaconTagFrame:
      case StackFrame::BeaconSpanFrame: {
         if (this->BeaconId == this->Data.BeaconAddress->beaconId) {
            char symbol[512];
            StackMarker marker;
            this->Data.BeaconAddress->createrMarker(marker);
            ((IStackMarker*)&marker)->getSymbol(symbol, sizeof(symbol));
            return symbol;
         }
         else return "(dead beacon)";
      }
      default:throw;
      }
   }

   void StackFrame::print() {
      SystemThreading::tSymbolEx<512> symbol;
      int frameCount = 0;
      for (StackFrame* frame = this; frame; frame = frame->next) {
         symbol.resolve(frame->Data.FunctionAddress);
         if (!frameCount) printf("\n-------------------------------\n");
         printf("> %s\n", symbol.name);
         frameCount++;
      }
      printf(">>> %d frames <<<\n", frameCount);
   }

   ThreadStackTracker::ThreadStackTracker(Thread* thread) {
      this->thread = thread;
      this->_reserve = 0;
      this->lastStack = this->allocFrame();
      this->lastStack->setRoot();

      this->stackLastId = 2;
      this->stackBeacons = &this->stackBeaconsBase[cThreadMaxStackBeacons - 1];
      this->stackBeacons[0] = 0;
   }

   bool ThreadStackTracker::updateFrames(ThreadStackContext& context, StackBeacon** beacons) {
      //__try {

      // Unroll first beacon
      StackBeacon* currentBeacon = beacons[0];
      uint64_t currentBeaconId = currentBeacon ? currentBeacon->beaconId : 0;
      if (currentBeacon && currentBeacon->isSpan()) {
         while (uintptr_t(currentBeacon) >= context.getStackAddress()) context.unwind(); // swallow span frames
      }

      // Unroll frames
      StackFrame* stack = this->allocFrameBatch();
      StackFrame* prevStack = this->lastStack;
      StackFrame* current = stack;
      while (context) // Check when end of stack reached
      {
         // Initiate current frame
         current->StackAddress = context.getStackAddress();
         current->InsAddress = context.getInsAddress();
         current->BeaconId = currentBeaconId;
         current->node = 0;

         // Reconcil stack position between current & prev
         while (current->StackAddress > prevStack->StackAddress) {
            if (prevStack->next) prevStack = prevStack->next;
            else break;
         }

         if (prevStack->BeaconId == currentBeaconId && prevStack->Kind == StackFrame::BeaconSpanFrame) {
            break;
         }

         // Fill current frame infos
         // -- case of beacon frame
         if (currentBeacon && uintptr_t(currentBeacon) < current->StackAddress) {

            // Set beacon infos
            current->Kind = currentBeacon->isSpan() ? StackFrame::BeaconSpanFrame : StackFrame::BeaconTagFrame;
            current->Data.BeaconAddress = currentBeacon;

            // Goto next beacon
            currentBeacon = (++beacons)[0];
            currentBeaconId = currentBeacon ? currentBeacon->beaconId : 0;
            if (currentBeacon && currentBeacon->isSpan()) {
               while (uintptr_t(currentBeacon) >= context.getStackAddress()) context.unwind(); // swallow span frames
            }
         }
         // -- case of native frame
         else {
            current->Kind = StackFrame::FunctionFrame;
            current->Data.FunctionAddress = context.unwind();
         }

         DEV_TRACE(std::string name = current->toString());
         DEV_TRACE(printf("> frame: %s %*c prev: %s\n", name.c_str(), 80 - name.size(), ' ', prevStack->toString().c_str()));

         // Check connexion with previous stack
         if (current->StackAddress == prevStack->StackAddress && current->InsAddress == prevStack->InsAddress && current->BeaconId == prevStack->BeaconId) {

            // Check specific infos
            if (current->Kind == prevStack->Kind && current->Data.FunctionAddress == prevStack->Data.FunctionAddress) {
               break;
            }
         }

         // Goto next frame
         if (current->next) current = current->next;
         else current = current->next = this->allocFrameBatch();
      }

      // When stopped due to previous stack matching
      if (context) {

         // Switch tail of current and prev stack
         StackFrame* tmp = current->next;
         current->next = prevStack->next;
         prevStack->next = tmp;

         // Release tail of current & prevStack
         this->_reserve = this->lastStack;
         this->lastStack = stack;

         DEV_TRACE(printf("> stack reconnection\n\n"));
      }
      // When stopped on the root
      else {
         // Release tail of current
         this->_reserve = current->setRoot();
         this->lastStack = stack;
         DEV_TRACE(printf("> stack end\n\n"));
      }
      return true;
      /* }
       __except (1) {
          return false;
       }*/
   }

   bool ThreadStackTracker::captureFrames(std::function<void(StackFrame*)> processing) {
      ThreadCapturedStackContext context;
      if (context.capture(this->thread)) {
         bool success = this->updateFrames(context, this->stackBeacons);
         if (success) processing(this->lastStack);
         context.release();
         return success;
      }
      return false;
   }

   uint64_t ThreadStackTracker::getStackStamp() {
      bool isCurrentThread = (Thread::current() == this->thread);
      if (g_SAT.stackStampDatabase && isCurrentThread) {
         typedef StackStampDatabase::tData tData;
         StackTree<tData>& stacktree = g_SAT.stackStampDatabase->stacktree;
         StackNode<tData>* node = &stacktree.root;

         // Update stack tracker with current stack
         ThreadLocalStackContext context(_AddressOfReturnAddress(), getRbpX64Proc());
         if (!this->updateFrames(context, this->stackBeacons)) {
            return 0;
         }

         // Stack inversion
         StackFrame* stack[1024];
         int stackSize = 0;
         for (StackFrame* frame = this->lastStack; frame; frame = frame->next) {
            if (frame->node) {
               node = (StackNode<tData>*)frame->node;
               break;
            }
            stack[stackSize++] = frame;
         }

         // Make a stackstamp
         StackMarker marker;
         for (int index = stackSize - 1; index >= 0; index--) {
            StackFrame* frame = stack[index];
            switch (frame->Kind)
            {
            case StackFrame::RootFrame: {
               node = &stacktree.root;
               break;
            }
            case StackFrame::FunctionFrame: {
               auto nativeMarker = marker.as<NativeStackMarker>();
               nativeMarker->base = frame->Data.FunctionAddress;
               nativeMarker->callsite = 0;
               node = stacktree.addMarkerInto((StackMarker&)marker, node);
               break;
            }
            case StackFrame::BeaconTagFrame:
            case StackFrame::BeaconSpanFrame: {
               frame->Data.BeaconAddress->createrMarker(marker);
               node = stacktree.addMarkerInto((StackMarker&)marker, node);
               break;
            }
            default:throw;
            }
            frame->node = node;
         }

         return uint64_t(node);
      }
      return 0;
   }

}