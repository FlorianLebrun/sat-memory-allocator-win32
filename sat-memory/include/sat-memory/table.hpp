#pragma once
#include <stdint.h>

namespace sat {
   namespace memory {

      const uintptr_t cEntrySizeL2 = 5;
      const uintptr_t cEntrySize = 1 << cEntrySizeL2;

      struct tEntryID {
         enum _ {

            /// Heap SEGMENTS
            ///---------------
            // Distributed on 0-64 indexes
            __FIRST_HEAP_SEGMENT,

            /// System SEGMENTS
            ///-----------------

            NOT_COMMITED = 0x80,
            SAT_SEGMENT = 0x40, // sat table segment
            FREE, // Available segment
            FORBIDDEN, // Forbidden segment (not reservable by allocator)
            PAGE_DESCRIPTOR, // Pages for descriptors buffers

            /// Technical SEGMENTS
            ///---------------
            // Distributed on 67-127 indexes
            __FIRST_TECHNICAL_SEGMENT,
         };
      };

      const uintptr_t cMemorySAT_MaxSpanClass = 128;

      typedef struct tDescriptorEntry {
         uint8_t id; // = SAT_DESCRIPTOR
         uint8_t bytesPerAddress; // Bytes per address (give the sizeof intptr_t)
         uint8_t bytesPerSegmentL2; // Bytes per segment (in log2)
         uintptr_t length; // Length of table
         uintptr_t limit; // Limit of table length
      } *tpDescriptorEntry;

      typedef struct tSegmentEntry {
         uint8_t id; // = SAT_SEGMENT
         void set() {
            this->id = tEntryID::SAT_SEGMENT;
         }
      } *tpSegmentEntry;

      typedef struct tFreeEntry {
         uint8_t id; // = FREE
         uint8_t isReserved;
         void set(uint8_t isReserved) {
            this->id = tEntryID::FREE;
            this->isReserved = isReserved;
         }
      } *tpFreeEntry;

      typedef struct tForbiddenEntry {
         uint8_t id; // = FORBIDDEN
         void set() {
            this->id = tEntryID::FORBIDDEN;
         }
      } *tpForbiddenEntry;

      typedef struct tHeapSegmentEntry {
         uint8_t id;
         uint8_t heapID;
      } *tpHeapSegmentEntry;

      typedef struct tEntry {
         union {
            tDescriptorEntry SATDescriptor;
            tSegmentEntry SATSegment;
            tHeapSegmentEntry heapSegment;
            tFreeEntry free;
            tForbiddenEntry forbidden;
            struct {
               uint8_t id;
               uint8_t bytes[31];
            };
         };

         template <typename T>
         T* get(uintptr_t index) { return (T*)&this[index]; }
         int getHeapID() { return (this->id & 0x40) ? -1 : this->heapSegment.heapID; }
         bool isHeapSegment(int id) { return this->id < tEntryID::SAT_SEGMENT; }
      } *tpEntry;

      static_assert(sizeof(tEntry) == cEntrySize, "Bad tEntry size");
   }
}

