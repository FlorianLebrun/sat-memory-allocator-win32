#include <sat-memory-allocator/stack_analysis.h>
#include <sat-memory-allocator/allocator.h>
#include "./stack-stamp.h"
#include "./thread-stack-tracker.h"

__forceinline sat::ThreadStackTracker* getCurrentStackTracker() {
   sat::Thread* thread = sat::Thread::current();
   auto tracker = thread->getObject<sat::ThreadStackTracker>();
   if (!tracker) {
      tracker = new sat::ThreadStackTracker(thread);
      thread->setObject<sat::ThreadStackTracker>(tracker);
   }
   return tracker;
}

void sat_begin_stack_beacon(sat::StackBeacon* beacon) {
   auto tracker = getCurrentStackTracker();
   if (tracker->stackBeacons >= tracker->stackBeaconsBase) {
      beacon->beaconId = ++tracker->stackLastId;
      (--tracker->stackBeacons)[0] = beacon;
   }
   else {
      printf("Error in sat stack beacon: too many beacon.\n");
   }
}

void sat_end_stack_beacon(sat::StackBeacon* beacon) {
   auto tracker = getCurrentStackTracker();
   while (tracker->stackBeacons[0] && uintptr_t(tracker->stackBeacons[0]) < uintptr_t(beacon)) {
      tracker->stackBeacons++;
      printf("Error in sat stack beacon: a previous beacon has been not closed.\n");
   }
   if (tracker->stackBeacons[0] && tracker->stackBeacons[0] == beacon) {
      tracker->stackBeacons++;
   }
   else if (tracker->stackBeacons != tracker->stackBeaconsBase) {
      printf("Error in sat stack beacon: a beacon is not retrieved and cannot be properly closed.\n");
   }
}

void sat_print_stackstamp(uint64_t stackstamp) {
   struct Visitor : public sat::IStackVisitor {
      virtual void visit(sat::StackMarker& marker) override {
         char symbol[1024];
         marker.getSymbol(symbol, sizeof(symbol));
         printf("* %s\n", symbol);
      }
   } visitor;
   sat::getStackStampDatabase()->traverseStack(stackstamp, &visitor);
}

namespace sat {
   namespace analysis {
      uint64_t getCurrentStackStamp() {
         auto tracker = getCurrentStackTracker();
         return tracker->getStackStamp();
      }
   }
}