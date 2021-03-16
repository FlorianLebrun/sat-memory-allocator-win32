#pragma once
#include <sat-threads/shared.hpp>
#include "./scheduler.h"
#include "./stack-stamp.h"
#include "./thread-stack-tracker.h"

namespace sat {

   struct StackProfiling : public shared::SharedObject<IStackProfiling> {
      StackTree<tData> stacktree;
      virtual IStackProfiling::Node getRoot() override;
      virtual void print() override;
      static StackProfiling* create();
   private:
      StackProfiling();
   };

   struct ThreadStackProfiler : public sat::TickWorker, public shared::SharedObject<IStackProfiler> {

      Thread* target;
      ThreadStackTracker tracker;
      StackProfiling* profiling;
      uint64_t lastSampleTime;

      ThreadStackProfiler(Thread* thread);
      virtual ~ThreadStackProfiler() override;

      virtual void execute() override;

      virtual IStackProfiling* flushProfiling() override;
      virtual void startProfiling() override;
      virtual void stopProfiling() override;
   };
}
