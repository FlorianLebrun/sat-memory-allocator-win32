
namespace ScaledBuddySubAllocator {

  static const int localOverflowThreshold = 16;
  static const int localUnderflowThreshold = 8;

  struct LocalManifold
  {
    tObjectList objects;
    LocalManifold()
    {
      this->objects = tObjectList::empty();
    }
  };
}

namespace ScaledBuddySubAllocator {
  struct LocalPageObjectHeap
  {
    GlobalHeap* global;
    uint32_t pageSize;
    SATEntry* page_cache;

    LocalPageObjectHeap(GlobalHeap* global)
    {
      this->global = global;
      this->pageSize = global->pageSize;
    }
    void freePageList(tObjectList list) {
      //TODO
      Object page = list.first;
      while (page) {
        Object page_next = page->next;
        g_SAT.freeSegmentSpan(uintptr_t(page) >> sat::SegmentSizeL2, 1);
        page = page_next;
      }
    }
  };
}

#include "heapLocal_plain_object.h"


namespace ScaledBuddySubAllocator {
  struct LocalPlainObjectHeap : LocalPageObjectHeap
  {
    LocalPlainObjectCache<3> plain_cache_3;
    LocalPlainObjectCache<2> plain_cache_2;
    LocalPlainObjectCache<1> plain_cache_1;
    LocalPlainObjectCache<0> plain_cache_0;

    LocalPlainObjectHeap(GlobalHeap* global)
      : LocalPageObjectHeap(global)
    {
      this->plain_cache_0.init(this);
      this->plain_cache_1.init(this);
      this->plain_cache_2.init(this);
      this->plain_cache_3.init(this);
    }
  };
}

#include "heapLocal_sub_object.h"


namespace ScaledBuddySubAllocator {

  // Local heap
  // > high concurrency performance
  // > increase distributed fragmentation
  struct LocalHeap : LocalPlainObjectHeap {
    LocalSubObjectCache<0> sub_caches_0;
    LocalSubObjectCache<1> sub_caches_1;
    LocalSubObjectCache<2> sub_caches_2;
    LocalSubObjectCache<3> sub_caches_3;

    LocalHeap(GlobalHeap* global) :LocalPlainObjectHeap(global) {
      sub_caches_0.init(this);
      sub_caches_1.init(this);
      sub_caches_2.init(this);
      sub_caches_3.init(this);
    }
    void freePtr(::tSATEntry* ptrEntry, uintptr_t ptr)
    {
      // Get object sat entry & index
      const SATEntry entry = SATEntry(ptrEntry);
      int index = (ptr >> baseSizeL2) & 0xf;

      // Check tag
      const uint8_t tag = entry->tags[index];
      if (!(tag&cTAG_IS_OBJECT_BIT)) {
        throw std::exception("free object cannot be free");
      }

      // Release in cache
      const int sizeID = tag & 0xf;
      switch (sizeID) {
      case 0:return this->plain_cache_0.freeObject(entry, index, ptr);
      case 1:return this->plain_cache_1.freeObject(entry, index, ptr);
      case 2:return this->plain_cache_2.freeObject(entry, index, ptr);
      case 3:return this->plain_cache_3.freeObject(entry, index, ptr);
      case 4:return this->sub_caches_0.freeObject(entry, index, ptr);
      case 5:return this->sub_caches_1.freeObject(entry, index, ptr);
      case 6:return this->sub_caches_2.freeObject(entry, index, ptr);
      case 7:return this->sub_caches_3.freeObject(entry, index, ptr);
      }
    }
    Object allocObject(int sizeID)
    {
      switch (sizeID) {
      case 0:return this->plain_cache_0.allocObject(); // 32k
      case 1:return this->plain_cache_1.allocObject(); // 16k
      case 2:return this->plain_cache_2.allocObject(); // 8k
      case 3:return this->plain_cache_3.allocObject(); // 4k
      case 4:return this->sub_caches_0.allocObject(); // 2k
      case 5:return this->sub_caches_1.allocObject(); // 1k
      case 6:return this->sub_caches_2.allocObject(); // 512
      case 7:return this->sub_caches_3.allocObject(); // 256
      }
    }
    int getCachedSize() {
      int size = 0;
      size += this->sub_caches_3.getCachedSize();
      size += this->sub_caches_2.getCachedSize();
      size += this->sub_caches_1.getCachedSize();
      size += this->sub_caches_0.getCachedSize();
      size += this->plain_cache_3.getCachedSize();
      size += this->plain_cache_2.getCachedSize();
      size += this->plain_cache_1.getCachedSize();
      size += this->plain_cache_0.getCachedSize();
      return size;
    }
    void flushCache()
    {
      this->sub_caches_3.scavengeOverflow(0); // 256
      this->sub_caches_2.scavengeOverflow(0); // 512
      this->sub_caches_1.scavengeOverflow(0); // 1k
      this->sub_caches_0.scavengeOverflow(0); // 2k
      this->plain_cache_3.scavengeOverflow(0); // 4k
      this->plain_cache_2.scavengeOverflow(0); // 8k
      this->plain_cache_1.scavengeOverflow(0); // 16k
      this->plain_cache_0.scavengeOverflow(0); // 32k
    }
  };
}
