#include "./base.h"

namespace ScaledBuddySubAllocator {

  union tObjectTag {
    struct {
      unsigned object : 1; // = 1
      unsigned meta : 4; // Metadata
      unsigned sizeID : 3; // Size of object (0-3)
    } object;
    struct {
      unsigned object : 1; // = 1
      unsigned __unused : 4; // = 0
      unsigned sizeID : 3; // Size of object (0-3)
    } object_partial;
    struct {
      unsigned object : 1; // = 1
      unsigned subID : 4;  // Sub objects id
      unsigned sizeID : 3; // = 0x7
    } sub_objects;
    struct {
      unsigned object : 1; // = 0
      unsigned status : 2; // = (STATUS_FREE, STATUS_SCAVENGED, STATUS_MERGED)
      unsigned sizeID : 5; // Size of object (0-7)
    } released;
  };

  const int STATUS_FREE = 1;
  const int STATUS_SCAVENGED = 2;
  const int STATUS_MERGED = 3;

  const int cTAG_IS_OBJECT_BIT = 0x80;
  const int cTAG_IS_NOT_INDEXED_MASK = 0xc0;

  const uintptr_t baseSizeID = 4;
  const int baseCount = 1 << baseSizeID;
  const int baseSizeL2 = SAT::SegmentSizeL2 - baseSizeID;

  typedef struct tSATEntry {
    uint8_t id;
    uint8_t index;
    uint8_t tags[baseCount];
    uint8_t __unused__[14];
  } *SATEntry;
  static_assert(sizeof(tSATEntry) == SAT::EntrySize, "Bad tSATEntry size");

  typedef struct tObject {
    tObject* next;
    uint8_t zoneID;
    uint8_t status;
  } *Object;

  typedef struct tZone : tObject {} *Zone;

  struct tZoneList {
    Object first;
    Object last;
    int sizeID;
    int count;
  };

  int getScaleFactorPageID(int scaleFactor) {
    switch (scaleFactor) {
    case 1:return tSATEntryID::PAGE_SCALED_BUDDY_1;
    case 3:return tSATEntryID::PAGE_SCALED_BUDDY_3;
    case 5:return tSATEntryID::PAGE_SCALED_BUDDY_5;
    case 7:return tSATEntryID::PAGE_SCALED_BUDDY_7;
    case 9:return tSATEntryID::PAGE_SCALED_BUDDY_9;
    case 11:return tSATEntryID::PAGE_SCALED_BUDDY_11;
    case 13:return tSATEntryID::PAGE_SCALED_BUDDY_13;
    case 15:return tSATEntryID::PAGE_SCALED_BUDDY_15;
    }
    return 0;
  }
  // Global heap
  // > provide memory coalescing
  // > provide page allocation & release
  // > provide cache transfert between local heap
  struct GlobalHeap {
    static const int objectOverflowThreshold = 16;
    static const int objectUnderflowThreshold = 8;

    struct ObjectManifold {
      SpinLock lock_pop_push;
      SpinLock lock_scavenge;
      Object objects;
      int count;

      ObjectManifold()
      {
        this->objects = 0;
        this->count = 0;
      }
      void push_list(tZoneList list)
      {
        this->lock_pop_push.lock();
        list.last->next = this->objects;
        this->objects = list.first;
        this->count += list.count;
        this->lock_pop_push.unlock();
      }
      tZoneList pop_list(int sizeID, int n)
      {
        tZoneList list;
        if (this->count) {
          this->lock_pop_push.lock();
          list = this->pop_list_unsafe(sizeID, n);
          this->lock_pop_push.unlock();
        }
        else {
          list.count = 0;
        }
        return list;
      }

      tZoneList pop_list_unsafe(int sizeID, int n)
      {
        tZoneList list;
        assert(n > 0);
        list.sizeID = sizeID;
        list.count = (this->count < n) ? this->count : n;
        Object last = this->objects;
        switch (list.count) {
        default:
          for (int i = 7; i < list.count; i++)
            last = last->next;
        case 7: last = last->next;
        case 6: last = last->next;
        case 5: last = last->next;
        case 4: last = last->next;
        case 3: last = last->next;
        case 2: last = last->next;
        case 1: {
          list.last = last;
          list.first = this->objects;
          this->objects = last->next;
          this->count -= list.count;
        }
        case 0: break;
        }
        return list;
      }
    };

    GlobalContext context;
    ObjectManifold manifolds[24];
    uint32_t pageID;
    uint32_t pageSize;
    uint32_t scaleFactor;
    uint32_t scaleDividor;

    GlobalHeap(int scaleFactor) {
      this->pageID = getScaleFactorPageID(scaleFactor);
      this->pageSize = scaleFactor << SAT::SegmentSizeL2;
      this->scaleFactor = scaleFactor;
      this->scaleDividor = 65536 / scaleFactor;
    }
    void initializeSATEntry(SATEntry entry)
    {
      for (int i = 0; i < scaleFactor; i++) {
        entry[i].id = this->pageID;
        entry[i].index = i;
        ((uint64_t*)entry[i].tags)[0] = 0;
        ((uint64_t*)entry[i].tags)[1] = 0;
      }
    }
    int tryPopSubZones(tZoneList& list, int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];
      if (manifold->count) {
        int n = 1;//TODO
        list = manifold->pop_list(sizeID, n);
        return list.count;
      }
      return 0;
    }
    int tryPopPlainZones(tZoneList& list, int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];
      if (manifold->count) {
        int n = 1;//TODO
        list = manifold->pop_list(sizeID, n);
        return list.count;
      }
      return 0;
    }
    Object acquirePage()
    {
      uintptr_t index = g_SAT.allocSegmentSpan(this->pageID);
      this->initializeSATEntry(g_SAT.get<tSATEntry>(index));
      return Object(index << SAT::SegmentSizeL2);
    }
    tZoneList acquirePlainZones(int sizeID)
    {
      tZoneList list;

      // Case 1: Obtain objects from cache
      do {
        ObjectManifold* manifold = &this->manifolds[sizeID];
        if (manifold->count) {
          int n = 1;//TODO
          list = manifold->pop_list(sizeID, n);
          if (list.count) return list;
        }
        sizeID--;
      } while (sizeID > 0);

      // Case 2: Obtain objects from page
      list.first = list.last = this->acquirePage();
      list.count = 1;
      list.sizeID = 0;
      return list;
    }
    void scavengeSubZones(tZoneList list)
    {
      // TODO: scavenge the list
    }
    void releaseSubZones(tZoneList list)
    {
      int count;
      ObjectManifold* manifold = &this->manifolds[list.sizeID];
      manifold->lock_pop_push.lock();
      count = manifold->count + list.count;

      // Select the release operation
      if (count > objectOverflowThreshold && manifold->lock_scavenge.tryLock()) {

        // Pop the global cache overflow
        int n = manifold->count - objectUnderflowThreshold;
        if (n > 0) {
          tZoneList others = manifold->pop_list_unsafe(list.sizeID, n);
          list.last->next = others.first;
          list.last = others.last;
          list.count += others.count;
        }
        manifold->lock_pop_push.unlock();

        // Scavenge the overflow
        this->scavengeSubZones(list);
        manifold->lock_scavenge.unlock();

      }
      else {

        // Fill the global cache
        manifold->count = count;
        list.last->next = manifold->objects;
        manifold->objects = list.first;
        manifold->lock_pop_push.unlock();
      }
    }
    void scavengePlainZones(tZoneList list)
    {
      int num_merged = 0;
      for (Object obj = list.first; obj; obj = obj->next) {
        Object buddy = 0; // TODO
        if (buddy->status == STATUS_SCAVENGED) {
          buddy->status = STATUS_MERGED;
          num_merged++;
        }
        else {
          obj->status = STATUS_SCAVENGED;
        }
      }
    }
    void releasePlainZones(tZoneList list)
    {
      int count;
      ObjectManifold* manifold = &this->manifolds[list.sizeID];
      manifold->lock_pop_push.lock();
      count = manifold->count + list.count;

      // Select the release operation
      if (count > objectOverflowThreshold && manifold->lock_scavenge.tryLock()) {

        // Pop the global cache overflow
        int n = manifold->count - objectUnderflowThreshold;
        if (n > 0) {
          tZoneList others = manifold->pop_list_unsafe(list.sizeID, n);
          list.last->next = others.first;
          list.last = others.last;
          list.count += others.count;
        }
        manifold->lock_pop_push.unlock();

        // Scavenge the overflow
        this->scavengePlainZones(list);
        manifold->lock_scavenge.unlock();

      }
      else {

        // Fill the global cache
        manifold->count = count;
        list.last->next = manifold->objects;
        manifold->objects = list.first;
        manifold->lock_pop_push.unlock();
      }
    }
  };

  // Local heap
  // > high concurrency performance
  // > increase distributed fragmentation
  struct LocalHeap {
    static const int objectOverflowThreshold = 16;
    static const int objectUnderflowThreshold = 8;

    struct ObjectManifold {
      Object objects;
      int count;
      ObjectManifold()
      {
        this->objects = 0;
        this->count = 0;
      }
      bool push(Object obj)
      {
        obj->next = this->objects;
        this->objects = obj;
        return (++this->count) > objectOverflowThreshold;
      }
      tZoneList pop_overflow(int sizeID)
      {
        tZoneList list;
        Object cur = this->objects;
        list.sizeID = sizeID;
        list.count = this->count - objectUnderflowThreshold;
        this->count = objectUnderflowThreshold;
        for (int i = list.count; i > 0; i--) {
          cur = cur->next;
        }
        list.last = cur;
        list.first = this->objects;
        this->objects = cur->next;
        return list;
      }
    };

    GlobalHeap* global;
    ObjectManifold manifolds[16];
    uint32_t pageSize;
    uint32_t scaleFactor;
    uint32_t scaleDividor;

    LocalHeap(GlobalHeap* global)
    {
      this->global = global;
      this->pageSize = global->pageSize;
      this->scaleFactor = global->scaleFactor;
      this->scaleDividor = global->scaleDividor;
    }

    void freePtr(::tSATEntry* ptrEntry, uintptr_t ptr)
    {

      // Get object SAT entry & index
      SATEntry entry = g_SAT.get<tSATEntry>(uintptr_t(ptr) >> SAT::SegmentSizeL2);
      uintptr_t offset = uintptr_t(ptr)&SAT::SegmentOffsetMask;
      if (int delta = entry->index) {
        entry -= delta;
        offset += delta << SAT::SegmentSizeL2;
      }
      int index = (offset / this->scaleFactor) >> baseSizeL2;

      // Get object tag
      uint8_t tag = entry->tags[index];
      if (!(tag&cTAG_IS_OBJECT_BIT)) {
        throw std::exception("bad ptr");
      }

      // Release in cache
      const int sizeID = tag & 0x7;
      if (sizeID > baseSizeID) {

        // Find object
        int offset = ptr % (pageSize >> sizeID);
        Object obj = Object(ptr - offset);

        // Release object
        ObjectManifold* manifold = &this->manifolds[sizeID];
        if (manifold->push(obj)) {
          tZoneList list = manifold->pop_overflow(sizeID);
          this->global->releaseSubZones(list);
        }
      }
      else {
        index = (index >> sizeID) << sizeID;
        tag = entry->tags[index];

        // Retag the SAT entries for this object
        int freeTag = (STATUS_FREE << 5) | sizeID;
        if (sizeID < baseSizeID) {
          int numTags = baseCount >> sizeID;
          for (int i = numTags - 1; i > 0; i--) {
            entry->tags[index + i] = 0;
          }
        }
        entry->tags[index] = 0;

        // Release object
        const Object obj = Object(ptr);
        ObjectManifold* manifold = &this->manifolds[tag];
        if (manifold->push(obj)) {
          tZoneList list = manifold->pop_overflow(tag);
          this->global->releasePlainZones(list);
        }
      }
    }
    Object acquirePlainZone(int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];

      // Case 1: Alloc an object from local cache
      if (Object obj = manifold->objects) {
        manifold->objects = obj->next;
        manifold->count--;
        return obj;
      }

      // Case 2: Alloc list from global cache
      if (sizeID) {
        tZoneList list;
        if (global->tryPopPlainZones(list, sizeID)) {
          if (int n = list.count - 1) {
            manifold->count = n;
            manifold->objects = list.first->next;
            list.last->next = 0;
          }
          return list.first;
        }
      }
      else {
        return global->acquirePage();
      }

      // Case 3: Alloc list from upper zone
      Object obj = this->acquirePlainZone(sizeID - 1);
      Object buddy = Object(uintptr_t(obj) + (pageSize >> sizeID));
      buddy->next = 0;
      manifold->objects = buddy;
      manifold->count = 1;
      return obj;
    }
    Object allocPlainObject(int sizeID)
    {
      // Acquire free object
      const Object obj = this->acquirePlainZone(sizeID);

      // Get object SAT entry & index
      SATEntry entry = g_SAT.get<tSATEntry>(uintptr_t(obj) >> SAT::SegmentSizeL2);
      uintptr_t offset = uintptr_t(obj)&SAT::SegmentOffsetMask;
      if (int delta = entry->index) {
        entry -= delta;
        offset += delta << SAT::SegmentSizeL2;
      }
      const int index = (offset / this->scaleFactor) >> baseSizeL2;

      // Retag the SAT entries for this object
      const int tag = sizeID | cTAG_IS_OBJECT_BIT;
      if (sizeID <= baseSizeID) {
        int numTags = 8 >> sizeID;
        for (int i = numTags - 1; i > 0; i--) {
          int idx = index + i;
          entry[idx >> 4].tags[idx & 0xf] = i;
        }
      }
      entry->tags[index] = tag;
      return obj;
    }
    Object allocSubObject(int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];

      // Case 1: Alloc an object from local cache
      if (Object obj = manifold->objects) {
        manifold->objects = obj->next;
        manifold->count--;
        return obj;
      }

      // Case 2: Alloc list from global cache
      tZoneList list;
      if (global->tryPopSubZones(list, sizeID)) {
        if (int n = list.count - 1) {
          manifold->count = n;
          manifold->objects = list.first->next;
          list.last->next = 0;
        }
        return list.first;
      }

      // Case 3: Alloc a base zone to split in sub size object
      // --- Get a free zone
      uintptr_t ptr = (uintptr_t)this->acquirePlainZone(baseSizeID);

      // --- Find object tag index
      SATEntry entry = g_SAT.get<tSATEntry>(ptr >> SAT::SegmentSizeL2);
      uintptr_t offset = uintptr_t(ptr)&SAT::SegmentOffsetMask;
      if (int delta = entry->index) {
        entry -= delta;
        offset += delta << SAT::SegmentSizeL2;
      }
      const int index = (offset / this->scaleFactor) >> baseSizeL2;

      // --- Set object tag
      entry->tags[index] = sizeID | cTAG_IS_OBJECT_BIT;

      // --- Cache the remain objects
      Object obj = Object(ptr);
      int sz = SAT::SegmentSize >> sizeID;
      int n = baseCount >> sizeID;
      manifold->count = n - 1;
      for (int i = 1; i < n; i++) {
        obj->zoneID = sizeID;
        obj->status = STATUS_FREE;
        obj->next = manifold->objects;
        manifold->objects = obj;
        obj += sz;
      }
      obj->zoneID = sizeID;
      return obj;
    }
    Object allocObject(int sizeID)
    {
      if (sizeID > baseSizeID) {
        return this->allocSubObject(sizeID);
      }
      else {
        return this->allocPlainObject(sizeID);
      }
    }
  };

  // Shared heap
  // > low concurrency performance
  // > reduce distributed fragmentation
  struct SharedHeap {
    struct SharableHeap : LocalHeap {
      SpinLock barrier;
    };
    SharableHeap* shared;
    Object allocObject(int sizeID)
    {
      Object obj;
      shared->barrier.lock();
      obj = shared->allocObject(sizeID);
      shared->barrier.unlock();
      return obj;
    }
    void freePtr(::tSATEntry* ptrEntry, uintptr_t ptr)
    {
      Object obj;
      shared->barrier.lock();
      shared->freePtr(ptrEntry, ptr);
      shared->barrier.unlock();
    }
  };

} // namespace ScaledBuddySubAllocator

#include <vector>

struct tTest {
  ScaledBuddySubAllocator::GlobalHeap globalHeap;
  ScaledBuddySubAllocator::LocalHeap localHeap;

  tTest() : globalHeap(3), localHeap(&globalHeap) {
  }
  void freePtr(void* obj) {
    intptr_t ptr = intptr_t(obj);
    tSATEntry* entry = g_SAT.get(ptr >> SAT::SegmentSizeL2);
    if (entry->id == tSATEntryID::PAGE_SCALED_BUDDY_3) localHeap.freePtr(entry, ptr);
  }

  void test_ScaledBuddySubAllocator()
  {
    std::vector<void*> objects;
    for (int i = 0; i < 10; i++) {
      objects.push_back(localHeap.allocObject(4));
    }
    while (objects.size()) {
      freePtr(objects.back());
      objects.pop_back();
    }
  }
};

void main()
{
  tTest test;
  printf("rr");
  test.test_ScaledBuddySubAllocator();
  getchar();
}