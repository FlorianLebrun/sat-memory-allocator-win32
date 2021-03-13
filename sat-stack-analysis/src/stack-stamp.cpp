#include "./stack-stamp.h"

namespace sat {

   StackStampDatabase* StackStampDatabase::create() {
      StackStampDatabase* tracer;
      size_t size;
      tracer = (StackStampDatabase*)LargeAllocator::allocator.allocateSegment(size);
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
