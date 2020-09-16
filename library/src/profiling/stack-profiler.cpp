#include "./stack-profiler.h"

namespace SAT {

   StackProfiling* StackProfiling::create() {
      uintptr_t index = g_SAT.allocSegmentSpan(1);
      StackProfiling* profiling = (StackProfiling*)(index << SAT::cSegmentSizeL2);
      profiling->StackProfiling::StackProfiling();
      return profiling;
   }
   StackProfiling::StackProfiling()
      : stacktree(uintptr_t(&this[1]))
   {
   }
   IStackProfiling::Node StackProfiling::getRoot() {
      return &this->stacktree.root;
   }
   void StackProfiling::print() {

   }
   void StackProfiling::destroy() {
   }

   ThreadStackProfiler::ThreadStackProfiler(Thread* thread) : tracker(thread) {
      this->profiling = 0;
      this->lastSampleTime = 0;
   }

   void ThreadStackProfiler::execute() {
      this->tracker.captureFrames([this](StackFrame* topOfStack) {
         typedef StackProfiling::tData tData;
         StackTree<tData>& stacktree = this->profiling->stacktree;
         StackNode<tData>* node = &stacktree.root;

         // Stack inversion
         StackFrame* stack[1024];
         int stackSize = 0;
         for (StackFrame* frame = topOfStack; frame; frame = frame->next) {
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

         node->data.hits++;
         this->lastSampleTime = g_SAT.getCurrentTimestamp();
      });
   }

   IStackProfiling* ThreadStackProfiler::flushProfiling() {
      IStackProfiling* profiling = this->profiling;
      this->profiling = 0;
      return profiling;
   }

   void ThreadStackProfiler::startProfiling() {
      if (!this->profiling) {
         this->profiling = StackProfiling::create();
         g_SAT.scheduler.addTickWorker(this);
      }
      else {
         printf("Cannot start profiling.\n");
      }
   }

   void ThreadStackProfiler::stopProfiling() {
      g_SAT.scheduler.removeTickWorker(this);
   }

   void ThreadStackProfiler::destroy() {
      g_SAT.scheduler.removeTickWorker(this);
      g_SAT.freeBuffer(this, sizeof(*this));
   }

}
