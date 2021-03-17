#pragma once
#include <sat/spinlock.hpp>
#include "../../heaps/heaps.h"
#include "../../../../common/alignment.h"

#define OBJECT_LIST_CHECK(x)// x
#define SAT_TRACE_ALLOC(x)// x
#define SAT_TRACE_WARNING(x)// x
#define SAT_TRACE(x)// x

namespace ZonedBuddyAllocator {

   typedef unsigned char tByte;
   typedef unsigned char* tpBytes;

   // TAG: tagging in sat entry
   // when allocated: (allocated:1 = 1, [:4] = 0, sizeID:3)
   // when free: (allocated:1 = 1, [:4] = 0, sizeID:3)
   const uint8_t cTAG_FREE = 0x00;
   const uint8_t cTAG_ALLOCATED_BIT = 0x80;
   const uint8_t cTAG_MARK_BIT = 0x20;
   const uint8_t cTAG_METADATA_BIT = 0x10;
   const uint8_t cTAG_SIZEID_MASK = 0x0f;
   inline const uint8_t TAG_ALLOCATE(const int sizeID) { return cTAG_ALLOCATED_BIT | sizeID; }
   inline const uint8_t TAG_FREE(const int sizeID) { return sizeID; }

   // TAG: tagging in sat entry & in zone
   // when allocated: (allocated:1 = 1, meta:4, sizeID:3)
   // when free: (allocated:1 = 0, nextFree:7)
   const uint8_t cZONETAG_FREE = 0x00;
   const uint8_t cZONETAG_ALLOCATED_BIT = 0x80;
   const uint8_t cZONETAG_MARK_BIT = 0x01;
   const uint8_t cZONETAG_METADATA_BIT = 0x02;

   // STATUS: status in buddy allocated object (see tObject)
   // when allocated: (free:1 = 0, meta:4, sizeID:3)
   // when free: (free:1 = 1, scavenge:1, [:3] = 0, sizeID:3)
   const uint8_t cSTATUS_ALLOCATE_ID = 0x00;
   const uint8_t cSTATUS_FREE_ID = 0x80;
   const uint8_t cSTATUS_SCAVENGED_ID = 0xC0;
   const uint8_t cSTATUS_ID_MASK = 0xC0;
   const uint8_t cSTATUS_SIZEID_MASK = 0x07;
   inline uint8_t STATUS_ALLOCATE(int sizeID) { return cSTATUS_ALLOCATE_ID | sizeID; }
   inline uint8_t STATUS_FREE(int sizeID) { return cSTATUS_FREE_ID | sizeID; }
   inline uint8_t STATUS_SCAVENGED(int sizeID) { return cSTATUS_SCAVENGED_ID | sizeID; }

   // Base object size (smallest plain object size)
   const int baseCountL2 = 4;
   const int baseCount = 1 << baseCountL2;
   const int baseSizeL2 = sat::memory::cSegmentSizeL2 - baseCountL2;
   const int baseSize = 1 << baseSizeL2;
   const uintptr_t baseOffsetMask = (1 << baseSizeL2) - 1;
   const uintptr_t basePtrMask = ~baseOffsetMask;

   // Support object size (largest plain object size)
   const int supportLengthL2 = baseCountL2 - 1;
   const int supportLength = 1 << supportLengthL2;
   const int supportSizeL2 = sat::memory::cSegmentSizeL2 - 1;
   const int supportSize = 1 << supportSizeL2;

   inline constexpr int computeObjectBatchSize(int sizeID) {
      return (4 >> (supportLengthL2 - sizeID)) || 1;
   }

   typedef struct tSATEntry {
      uint8_t id;
      uint8_t heapID;
      uint8_t index;
      uint8_t __unused__[5];
      tSATEntry* next;
#if UINTPTR_MAX == UINT32_MAX
      uint32_t __next_i64;
#endif
      uint8_t tags[baseCount];
   } *SATEntry;

   static_assert(sizeof(tSATEntry) == sat::memory::cEntrySize, "Bad tSATEntry size");

   uintptr_t traversePageObjects(uintptr_t index, bool& visitMore, sat::IObjectVisitor* visitor);
   bool get_address_infos(uintptr_t ptr, sat::tpObjectInfos infos);
}

#include "utils.h"
#include "Global.h"
#include "Local.h"
