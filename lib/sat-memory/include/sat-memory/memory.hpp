#pragma once
#include "./table.hpp"
#include <stdint.h>

namespace sat {
   namespace memory {

      const uintptr_t cSegmentSizeL2 = 16;
      const uintptr_t cSegmentSize = 1 << cSegmentSizeL2;
      const uintptr_t cSegmentOffsetMask = (1 << cSegmentSizeL2) - 1;
      const uintptr_t cSegmentPtrMask = ~cSegmentOffsetMask;
      const uintptr_t cSegmentMinIndex = 32;
      const uintptr_t cSegmentMinAddress = cSegmentMinIndex << cSegmentSizeL2;

      struct MemoryTable {
         union {
            tDescriptorEntry descriptor;
            tEntry entries[1];
         };

         static void initialize();

         // Segment Table allocation
         uintptr_t allocSegmentSpan(uintptr_t size);
         void freeSegmentSpan(uintptr_t index, uintptr_t size);
         uintptr_t reserveMemory(uintptr_t size, uintptr_t alignL2 = 1);
         void commitMemory(uintptr_t index, uintptr_t size);
         void decommitMemory(uintptr_t index, uintptr_t size);

         // Allocator
         void* allocBuffer64(uint32_t level);
         void freeBuffer64(void* ptr, uint32_t level);
         void* allocBuffer(uint32_t size);
         void freeBuffer(void* ptr, uint32_t size);

         // Helpers
         template <typename T>
         T* get(uintptr_t index) { return (T*)&this->entries[index]; }
         void printSegments();

      private:
         MemoryTable(uintptr_t size, uintptr_t limit);
      };

      extern MemoryTable* table;
   }
}
