#pragma once
#include <stdint.h>

namespace sat {

   struct tHeapEntryID {
      enum _ {
         PAGE_ZONED_BUDDY = memory::tEntryID::__FIRST_HEAP_SEGMENT,
         PAGE_SCALED_BUDDY_3,
         PAGE_SCALED_BUDDY_5,
         PAGE_SCALED_BUDDY_7,
         PAGE_SCALED_BUDDY_9,
         PAGE_SCALED_BUDDY_11,
         PAGE_SCALED_BUDDY_13,
         PAGE_SCALED_BUDDY_15,
         LARGE_OBJECT,
         PROFILING_SEGMENT,
      };
   };

   struct tTechEntryID {
      enum _ {
         PROFILING_SEGMENT = memory::tEntryID::__FIRST_TECHNICAL_SEGMENT,
         TYPES_DATABASE,
      };
   };

   typedef struct tProfilingSegmentEntry {
      uint8_t id; // = PROFILING_SEGMENT
      uint64_t profilingID; // Number representing the profiling
      void set(uint64_t profilingID = 0) {
         this->id = tTechEntryID::PROFILING_SEGMENT;
         this->profilingID = profilingID;
      }
   } *tpProfilingSegmentEntry;

   typedef struct tHeapSegmentEntry {
      uint8_t id;
      uint8_t heapID;
      bool isHeapSegment() { return this->id < sat::memory::tEntryID::SAT_SEGMENT; }
   } *tpHeapSegmentEntry;

   typedef struct tHeapLargeObjectEntry {
      uint8_t id; // = LARGE_OBJECT
      uint8_t heapID;
      uint32_t index;
      uint32_t length;
      uint64_t meta;
      void set(uint8_t heapID, uint32_t index, uint32_t length, uint64_t meta) {
         this->id = tHeapEntryID::LARGE_OBJECT;
         this->heapID = heapID;
         this->index = index;
         this->length = length;
         this->meta = meta;
      }
   } *tpHeapLargeObjectEntry;

   typedef void* (*tp_malloc)(size_t size);
   typedef void* (*tp_realloc)(void* ptr, size_t size);
   typedef size_t(*tp_msize)(void* ptr);
   typedef void (*tp_free)(void*);

}
