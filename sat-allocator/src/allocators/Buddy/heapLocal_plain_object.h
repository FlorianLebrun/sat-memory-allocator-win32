
namespace ScaledBuddySubAllocator {

  template <int sizeID>
  struct LocalPlainObjectCache : LocalManifold
  {
    typedef LocalPlainObjectCache<(sizeID - 1) & 3>* tpUpperCache;

    static const int sizeL2 = supportSizeL2 - sizeID;
    static const uintptr_t size = 1 << sizeL2;
    static const uintptr_t objectPtrMask = uint64_t(-1) << sizeL2;
    static const uintptr_t objectOffsetMask = ~objectPtrMask;

    GlobalHeap* global;
    LocalPageObjectHeap* local;

    void init(LocalPageObjectHeap* local) {
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
      if (global->tryPopPlainZones(this->objects, sizeID)) {
        return this->objects.pop();
      }

      // Case 3: Alloc list from upper zone
      Object obj_0;
      if (sizeID > 0) obj_0 = tpUpperCache(&this[1])->acquireObject();
      else obj_0 = global->acquirePage();
      Object obj_1 = Object(uintptr_t(obj_0) ^ size);

      // --- Update object
      obj_0->status = STATUS_FREE(sizeID);
      obj_0->zoneID = sizeL2;
      obj_0->buddy = obj_1;
      obj_0->next = 0;

      // --- Release buddy object
      obj_1->status = STATUS_FREE(sizeID);
      obj_1->buddy = obj_0;
      obj_1->zoneID = sizeL2;
      obj_1->next = 0;
      this->objects.first = obj_1;
      this->objects.last = obj_1;
      this->objects.count = 1;

      return obj_0;
    }
    Object allocObject()
    {
      // Acquire free object
      const Object obj = this->acquireObject();

      // Get object sat entry & index
      const SATEntry entry = g_SAT.get<tSATEntry>(uintptr_t(obj) >> sat::SegmentSizeL2);

      // Tag the entry for this object
      const int index = WriteTags<sizeID, sizeID | cTAG_IS_OBJECT_BIT>::apply(entry, uintptr_t(obj) >> baseSizeL2);

      assert(obj->status == STATUS_FREE(sizeID));
      obj->status &= STATUS_SIZEL2_MASK;
      return obj;
    }
    void freeObjectList(tObjectList list) {
      this->objects.append(list);
      if (this->objects.count > localOverflowThreshold) {
        return this->scavengeOverflow(localUnderflowThreshold);
      }
    }
    void scavengeOverflow(int remain = 0) {
      if (tObjectList list = this->objects.pop_overflow(sizeID, remain)) {
        if (list = global->releasePlainZones(sizeID, list)) {
          if (sizeID < 3) {
            return tpUpperCache(&this[1])->freeObjectList(list);
          }
          else {
            return local->freePageList(list);
          }
        }
      }
    }
    void freeObject(SATEntry entry, int index, uintptr_t ptr) 
    {
      // Replace object address
      const Object obj = Object(ptr&objectPtrMask);
      assert(obj->status == STATUS_ALLOCATE(sizeID));
      obj->status |= STATUS_FREE_ID;

      // Retag the sat entries for this object
      WriteTags<sizeID, 0>::apply(entry, ptr >> baseSizeL2);

      // Release object
      if (this->objects.push(obj) > localOverflowThreshold) {
        return this->scavengeOverflow(localUnderflowThreshold);
      }
    }
  };
}

