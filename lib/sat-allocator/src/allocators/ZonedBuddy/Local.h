#pragma once

namespace ZonedBuddyAllocator {
   namespace Local {
      static const int localOverflowThreshold = 16;
      static const int localUnderflowThreshold = localOverflowThreshold / 2;

      struct PageObjectCache
      {
         Global::Cache* global;
         uint32_t pageSize;
         SATEntry* page_cache;
      };
   }
}

#include "Local_objects.h"
#include "Local_sub_objects.h"
#include "Local_zoned_objects.h"

namespace ZonedBuddyAllocator {
   namespace Local {
      // Local heap
      // > high concurrency performance
      // > increase distributed fragmentation
      struct Cache : PageObjectCache
      {
         ZoneCache<0, ObjectCache<0>> base_cache_0;
         ZoneCache<1, ObjectCache<1>> base_cache_1;
         ZoneCache<2, ObjectCache<2>> base_cache_2;
         ZoneCache<3, ObjectCache<3>> base_cache_3;

         ZoneCache<4, SubObjectCache<4>> base_cache_4;
         ZoneCache<5, SubObjectCache<5>> base_cache_5;
         ZoneCache<6, SubObjectCache<6>> base_cache_6;
         ZoneCache<7, SubObjectCache<7>> base_cache_7;

         void init(Global::Cache* global);
         sat::IObjectAllocator* getAllocator(int id);
         int getCachedSize();
         void flushCache();

         int freePtr(::sat::memory::tEntry* ptrEntry, uintptr_t ptr)
         {
            // Get object sat entry & index
            const SATEntry entry = SATEntry(ptrEntry);
            int index = (ptr >> baseSizeL2) & 0xf;

            // Release in cache
            const uint8_t tag = entry->tags[index];
            if (tag & cTAG_ALLOCATED_BIT) {
               switch (tag & cTAG_SIZEID_MASK) {
               case 0:return this->base_cache_0.freeObject(entry, index, ptr);
               case 1:return this->base_cache_1.freeObject(entry, index, ptr);
               case 2:return this->base_cache_2.freeObject(entry, index, ptr);
               case 3:return this->base_cache_3.freeObject(entry, index, ptr);
               case 4:return this->base_cache_4.freeObject(entry, index, ptr);
               case 5:return this->base_cache_5.freeObject(entry, index, ptr);
               case 6:return this->base_cache_6.freeObject(entry, index, ptr);
               case 7:return this->base_cache_7.freeObject(entry, index, ptr);
               }
            }
            throw std::exception("Cannot free a not allocated object");
         }
         sat::IObjectAllocator* getSizeAllocator(size_t size) {
            int sizeId = SizeMapping::getSizeID(size);
            return this->getAllocator(sizeId);
         }
      };
   }
}
