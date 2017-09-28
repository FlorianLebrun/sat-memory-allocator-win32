
namespace ScaledBuddySubAllocator {

  template <int subSizeID>
  struct LocalSubObjectCache : LocalManifold
  {
    static const int sizeID = subSizeID + baseCountL2;
    static const int sizeL2 = supportSizeL2 - sizeID;
    static const uintptr_t objectPtrMask = uintptr_t(-1) << sizeL2;
    static const uintptr_t objectOffsetMask = ~objectPtrMask;
    static const int objectSize = 1 << sizeL2;
    static const int objectPerZone = baseSize >> sizeL2;

    GlobalHeap* global;
    LocalPlainObjectHeap* local;

    void init(LocalPlainObjectHeap* local) {
      this->global = local->global;
      this->local = local;
    }
    int getCachedSize() {
      return this->objects.count << sizeL2;
    }
    Object acquireObject()
    {
      // Case 1: Alloc an object from local cache
      if (Object obj = this->objects.pop()) {
        return obj;
      }

      // Case 2: Alloc list from global cache
      if (global->tryPopSubZones(this->objects, sizeID)) {
        return this->objects.pop();
      }

      // Case 3: Alloc a base zone to split in sub size object
      // --- Get a free zone
      uintptr_t ptr = (uintptr_t)local->plain_cache_3.acquireObject();
      SATEntry entry = g_SAT.get<tSATEntry>(ptr >> SAT::SegmentSizeL2);
      entry->tags[(ptr >> baseSizeL2) & 0xf] = sizeID | cTAG_IS_OBJECT_BIT;

      // --- Cache the remain objects
      Object obj_0, obj_1;
      this->objects.count = objectPerZone;
      this->objects.first = Object(ptr);
      for (int i = 0; i < objectPerZone; i += 2) {
        obj_0 = Object(ptr); ptr += objectSize;
        obj_1 = Object(ptr); ptr += objectSize;
        obj_0->status = STATUS_FREE(sizeID);
        obj_0->zoneID = sizeL2;
        obj_0->buddy = obj_1;
        obj_0->next = obj_1;
        obj_1->status = STATUS_FREE(sizeID);
        obj_1->zoneID = sizeL2;
        obj_1->buddy = obj_0;
        obj_1->next = Object(ptr);
      }
      obj_1->next = 0;
      this->objects.last = obj_1;

      return this->objects.pop();
    }
    Object allocObject()
    {
      Object obj = this->acquireObject();
      assert(obj->status == STATUS_FREE(sizeID));
      obj->status &= STATUS_SIZEL2_MASK;
      return obj;
    }
    void scavengeOverflow(int remain) {
      if (tObjectList list = this->objects.pop_overflow(sizeID, remain)) {
        if (list = global->releasePlainZones(sizeID, list)) {
          return local->plain_cache_3.freeObjectList(list);
        }
      }
    }
    void freeObject(SATEntry entry, int index, uintptr_t ptr) {

      // Find object
      Object obj = Object(ptr&objectPtrMask);
      assert(obj->status == STATUS_ALLOCATE(sizeID));
      obj->status |= STATUS_FREE_ID;

      // Release object
      if (this->objects.push(obj) > localOverflowThreshold) {
        return this->scavengeOverflow(localUnderflowThreshold);
      }
    }
  };
}
