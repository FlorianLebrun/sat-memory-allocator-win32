#include "./stack-stamp.h"

namespace sat {

   sat::StackStampDatabase* getStackStampDatabase() {
      static sat::StackStampDatabase* stackStampDatabase = nullptr;
      static sat::SpinLock lock;
      if (!stackStampDatabase) {
         sat::SpinLockHolder guard(lock);
         if (!stackStampDatabase) {
            stackStampDatabase = sat::StackStampDatabase::create();
         }
      }
      return stackStampDatabase;
   }

   StackStampDatabase* StackStampDatabase::create() {
      uintptr_t index = memory::table->allocSegmentSpan(1);
      StackStampDatabase* tracer = (StackStampDatabase*)(index << memory::cSegmentSizeL2);
      tracer->StackStampDatabase::StackStampDatabase(uintptr_t(&tracer[1]));
      return tracer;
   }

   StackStampDatabase::StackStampDatabase(uintptr_t firstSegmentBuffer)
      : stacktree(firstSegmentBuffer)
   {
   }

   void StackStampDatabase::traverseStack(uint64_t stackstamp, sat::IStackVisitor* visitor) {
      typedef StackNode<tData>* Node;
      for (auto node = Node(stackstamp); node; node = node->parent) {
         visitor->visit(node->marker);
      }
   }
}
