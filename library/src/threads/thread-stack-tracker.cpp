#include "./index.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>

#include <windows.h>
#include <psapi.h>
#include <Dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

struct NativeStackMarker : SAT::IStackMarker {
   uintptr_t base;
   uintptr_t callsite;
   virtual void getSymbol(char* buffer, int size) override {
      SystemThreading::tSymbol symbol(buffer, size);
      if (!symbol.resolve(this->base)) {
         sprintf_s(buffer, size, "0x%.8lX", uint64_t(this->base));
      }
   }
};

void StackFrame::print() {
   SystemThreading::tSymbolEx<512> symbol;
   int frameCount = 0;
   for (StackFrame* frame = this; frame; frame = frame->next) {
      symbol.resolve(frame->BaseAddress);
      if (!frameCount) printf("\n-------------------------------\n");
      printf("> %s\n", symbol.name);
      frameCount++;
   }
   printf(">>> %d frames <<<\n", frameCount);
}

StackFrame* ThreadStackTracker::updateFrames(void* pcontext) {
   CONTEXT& context = *LPCONTEXT(pcontext);

   // Fix stack register for some terminal function
   if (context.Rip == 0 && context.Rsp != 0) {
      context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
      context.Rsp += 8;
   }

   // Unroll frames
   StackFrame* stack = this->allocFrameBatch();
   StackFrame* prevStack = this->lastStack;
   StackFrame* current = stack;
   while (context.Rip) {// Check when end of stack reached

     // Reconcil stack position between current & prev
      current->StackAddress = context.Rsp;
      while (current->StackAddress < current->StackAddress) {
         prevStack = prevStack->next;
      }

      // Resolve current frame
      DWORD64 ImageBase;
      PRUNTIME_FUNCTION pFunctionEntry = ::RtlLookupFunctionEntry(context.Rip, &ImageBase, NULL);
      if (pFunctionEntry) {
         PVOID HandlerData;
         DWORD64 EstablisherFrame;
         current->InsAddress = context.Rip;
         current->BaseAddress = ImageBase + pFunctionEntry->BeginAddress;
         ::RtlVirtualUnwind(UNW_FLAG_NHANDLER,
            ImageBase,
            current->InsAddress,
            pFunctionEntry,
            &context,
            &HandlerData,
            &EstablisherFrame,
            NULL);
      }
      else {
         current->InsAddress = context.Rip;
         current->BaseAddress = context.Rip;
         context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
         context.Rsp += 8;
      }

      if (current->StackAddress == prevStack->StackAddress) {
         if (current->BaseAddress == prevStack->BaseAddress && current->InsAddress == prevStack->InsAddress) {

            // Switch tail of current and prev stack
            StackFrame* tmp = current->next;
            current->next = prevStack->next;
            prevStack->next = tmp;

            // Release tail of current & prevStack
            this->_reserve = this->lastStack;
            this->lastStack = stack;
            return stack;
         }
      }

      if (current->next) current = current->next;
      else current = current->next = this->allocFrameBatch();
   }
   this->_reserve = current->enclose();
   this->lastStack = stack;
   return stack;
}

uint64_t SAT::Thread::getStackStamp() {
   bool isCurrentThread = (sat_current_thread() == this);
   if (g_SAT.stackStampDatabase && isCurrentThread) {
#if 0

      // Update stack tracker with current stack
      CONTEXT context;
      context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
      context.Rsp = (DWORD64)_AddressOfReturnAddress();
      context.Rip = 0;
      this->stackTracker.updateFrames(&context);

      // Make a stackstamp
      auto& manifold = g_SAT.stackStampDatabase->manifold;
      auto node = &manifold.root;
      for (auto frame = this->stackTracker.firstFrame; frame; frame = frame->next) {
         NativeStackMarker marker;
         marker.base = frame->BaseAddress; // Function entrypoint
         marker.callsite = 0;
         node = manifold.addMarkerInto((SAT::StackMarker&)marker, node);
      }

      return uint64_t(node);
#else
      return uint64_t(g_SAT.stackStampDatabase->manifold.getStackNode(this));
#endif
   }
   return 0;
}
