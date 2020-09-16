
#include "../base.h"

#ifndef _SAT_profiler_h_
#define _SAT_profiler_h_

namespace SAT {

   struct StackProfiling : public SATBasicRealeasable<IStackProfiling> {
      StackTree<tData> stacktree;
      virtual IStackProfiling::Node getRoot() override;
      virtual void print() override;
      virtual void destroy() override;
      static StackProfiling* create();
   private:
      StackProfiling();
   };

   struct ThreadStackProfiler : public SATScheduler::TickWorker, public SATBasicRealeasable<IStackProfiler> {

      Thread* target;
      ThreadStackTracker tracker;
      StackProfiling* profiling;
      uint64_t lastSampleTime;

      ThreadStackProfiler(Thread* thread);

      virtual void execute() override;

      virtual IStackProfiling* flushProfiling() override;
      virtual void startProfiling() override;
      virtual void stopProfiling() override;

      virtual void destroy() override;
   };
}

#endif

