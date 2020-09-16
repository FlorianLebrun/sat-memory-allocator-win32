#include "./stack-stamp.h"

namespace SAT {

   StackStampDatabase* StackStampDatabase::create() {
      uintptr_t index = g_SAT.allocSegmentSpan(1);
      StackStampDatabase* tracer = (StackStampDatabase*)(index << SAT::cSegmentSizeL2);
      tracer->StackStampDatabase::StackStampDatabase(uintptr_t(&tracer[1]));
      return tracer;
   }

   StackStampDatabase::StackStampDatabase(uintptr_t firstSegmentBuffer)
      : stacktree(firstSegmentBuffer)
   {
   }

   void StackStampDatabase::traverseStack(uint64_t stackstamp, SAT::IStackVisitor* visitor) {
      typedef StackNode<tData>* Node;
      for (auto node = Node(stackstamp); node; node = node->parent) {
         visitor->visit(node->marker);
      }
   }
}
