
namespace ZonedBuddyAllocator {
  namespace Global {
    static const int globalOverflowThreshold = 16;
    static const int globalUnderflowThreshold = globalOverflowThreshold / 2;

    template<class TObject>
    struct ScavengeManifold : tObjectListEx<TObject> {
      SpinLock lock;
    };

    struct PageObjectCache {
      uint32_t pageID;
      uint32_t pageSize;
      uint32_t pageHeapID;
      SAT::IHeap* pageHeap;

      void initializeSATEntry(uintptr_t index)
      {
        SATEntry entry = g_SATable->get<tSATEntry>(index);
        entry[0].heapID = this->pageHeapID;
        entry[0].id = this->pageID;
        entry[0].index = 0;
        ((uint64_t*)entry[0].tags)[0] = 0;
        ((uint64_t*)entry[0].tags)[1] = 0;
      }
      uintptr_t acquirePage() {
        uintptr_t index = this->pageHeap->acquirePages(1);
        this->initializeSATEntry(index);
        return uintptr_t(index << SAT::cSegmentSizeL2);
      }
      void releasePage(uintptr_t ptr) {
        this->pageHeap->releasePages(uintptr_t(ptr) >> SAT::cSegmentSizeL2, 1);
      }
    };
  }
}

#include "Global_objects.h"
#include "Global_sub_objects.h"
#include "Global_zoned_objects.h"

namespace ZonedBuddyAllocator {
  namespace Global {
    // Global heap
    // > provide memory coalescing
    // > provide page allocation & release
    // > provide cache transfert between local heap
    struct Cache : PageObjectCache {

      ZoneCache<0, ObjectCache<0> > base_cache_0;
      ZoneCache<1, ObjectCache<1> > base_cache_1;
      ZoneCache<2, ObjectCache<2> > base_cache_2;
      ZoneCache<3, ObjectCache<3> > base_cache_3;

      ZoneCache<4, SubObjectCache<4> > base_cache_4;
      ZoneCache<5, SubObjectCache<5> > base_cache_5;
      ZoneCache<6, SubObjectCache<6> > base_cache_6;
      ZoneCache<7, SubObjectCache<7> > base_cache_7;

      void init(SAT::IHeap* pageHeap, int pageID = SAT::tEntryID::PAGE_ZONED_BUDDY);
      SAT::IObjectAllocator* getAllocator(int id);
      int getCachedSize();
      void flushCache();

      int freePtr(::SAT::tEntry* ptrEntry, uintptr_t ptr)
      {
        // Get object SAT entry & index
        const SATEntry entry = SATEntry(ptrEntry);
        int index = (ptr >> baseSizeL2) & 0xf;

        // Release in cache
        const uint8_t tag = entry->tags[index];
        if (tag&cTAG_ALLOCATED_BIT) {
          switch (tag&cTAG_SIZEID_MASK) {
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
      void markPtr(SAT::tEntry* ptrEntry, uintptr_t ptr)
      {
        // Get object SAT entry & index
        const SATEntry entry = SATEntry(ptrEntry);
        int index = (ptr >> baseSizeL2) & 0xf;

        // Release in cache
        const uint8_t tag = entry->tags[index];
        if (tag&cTAG_ALLOCATED_BIT) {
          switch (tag&cTAG_SIZEID_MASK) {
          case 0:return this->base_cache_0.markObject(entry, index, ptr);
          case 1:return this->base_cache_1.markObject(entry, index, ptr);
          case 2:return this->base_cache_2.markObject(entry, index, ptr);
          case 3:return this->base_cache_3.markObject(entry, index, ptr);
          case 4:return this->base_cache_4.markObject(entry, index, ptr);
          case 5:return this->base_cache_5.markObject(entry, index, ptr);
          case 6:return this->base_cache_6.markObject(entry, index, ptr);
          case 7:return this->base_cache_7.markObject(entry, index, ptr);
          }
        }
        else {
          throw std::exception("free object cannot be free");
        }
      }
    };
  }
}