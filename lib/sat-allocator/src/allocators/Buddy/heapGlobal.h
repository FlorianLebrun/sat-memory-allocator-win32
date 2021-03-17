
namespace ScaledBuddySubAllocator {
  // Global heap
  // > provide memory coalescing
  // > provide page allocation & release
  // > provide cache transfert between local heap
  struct GlobalHeap {
    static const int objectOverflowThreshold = 16;
    static const int objectUnderflowThreshold = 8;

    struct ScavengeManifold {
      SpinLock lock;
      Object objects;
      int count;

      ScavengeManifold()
      {
        this->objects = 0;
        this->count = 0;
      }
      void push(Object obj)
      {
        ObjectEx(obj)->back = Object(&this->objects);
        if (Object next = obj->next = this->objects) {
          ObjectEx(next)->back = obj;
        }
        this->count++;
      }
      void pop(Object obj) {
        ObjectEx(obj)->back->next = obj->next;
        if (obj->next) ObjectEx(obj->next)->back = ObjectEx(obj)->back;
        this->count--;
      }
      Object pop()
      {
        if (Object obj = this->objects) {
          if (Object next = this->objects = obj->next) {
            ObjectEx(next)->back = Object(&this->objects);
          }
          this->count--;
          return obj;
        }
        return 0;
      }
    };

    struct ObjectManifold {
      SpinLock lock;
      tObjectList objects;

      ObjectManifold()
      {
        this->objects = tObjectList::empty();
      }
    };

    GlobalContext context;
    ScavengeManifold scavenge_manifolds[24];
    ObjectManifold manifolds[24];
    uint32_t pageID;
    uint32_t pageSize;

    GlobalHeap()
    {
      this->pageID = getScaleFactorPageID(1);
      this->pageSize = 1 << sat::SegmentSizeL2;
    }
    int getCachedSize() {
      int size = 0;
      for (int i = 0; i < 24; i++) {
        size += scavenge_manifolds[i].count*(supportSize >> i);
      }
      for (int i = 0; i < 24; i++) {
        size += manifolds[i].objects.count*(supportSize >> i);
      }
      return size;
    }
    void initializeSATEntry(SATEntry entry)
    {
      entry[0].id = this->pageID;
      entry[0].index = 0;
      ((uint64_t*)entry[0].tags)[0] = 0;
      ((uint64_t*)entry[0].tags)[1] = 0;
    }
    int tryPopSubZones(tObjectList& list, int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];
      if (manifold->objects.count) {
        int n = (4 >> (supportLengthL2 - sizeID)) || 1;
        list = manifold->objects.pop_list(n);
        return list.count;
      }
      return 0;
    }
    int tryPopPlainZones(tObjectList& list, int sizeID)
    {
      ObjectManifold* manifold = &this->manifolds[sizeID];
      if (manifold->objects.count) {
        int n = (4 >> (supportLengthL2 - sizeID)) || 1;
        list = manifold->objects.pop_list(n);
        return list.count;
      }
      return 0;
    }
    Object acquirePage()
    {
      uintptr_t index = g_SAT.allocSegmentSpan(1);
      this->initializeSATEntry(g_SAT.get<tSATEntry>(index));
      return Object(index << sat::SegmentSizeL2);
    }
    tObjectList acquirePlainZones(int sizeID)
    {
      tObjectList list;

      // Case 1: Obtain objects from cache
      do {
        ObjectManifold* manifold = &this->manifolds[sizeID];
        if (manifold->objects.count) {
          int n = (4 >> (supportLengthL2 - sizeID)) || 1;
          list = manifold->objects.pop_list(n);
          if (list.count) return list;
        }
        sizeID--;
      } while (sizeID >= 0);

      // Case 2: Obtain objects from page
      Object obj_0 = this->acquirePage();
      Object obj_1 = Object(uintptr_t(obj_0) + supportSize);
      list.first = obj_0;
      list.last = obj_1;
      list.count = 2;
      // --- Init object 0
      obj_0->status = STATUS_FREE(0);
      obj_0->zoneID = 0;
      obj_0->buddy = obj_1;
      obj_0->next = obj_1;
      // --- v object 1
      obj_1->status = STATUS_FREE(0);
      obj_1->zoneID = 0;
      obj_1->buddy = obj_0;
      obj_1->next = 0;
      return list;
    }
    void scavengeSubZones(tObjectList list)
    {
      // TODO: scavenge the list
    }
    void releaseSubZones(int sizeID, tObjectList list)
    {
      this->releasePlainZones(sizeID, list);
    }
    tObjectList scavengePlainZones(int sizeID, tObjectList list)
    {
      ScavengeManifold* scavenge = &this->scavenge_manifolds[sizeID];
      tObjectList merged = tObjectList::empty();
      Object obj = list.first;
      while (obj) {
        Object buddy = obj->buddy;
        Object obj_next = obj->next;
        //assert((obj->status&STATUS_ID_MASK) == STATUS_FREE_ID);
        obj->status |= STATUS_SCAVENGED_ID;
        assert((obj->status&STATUS_ID_MASK) == STATUS_SCAVENGED_ID);

        // When the buddy is scavenged too
        int status = obj->status;
        if (buddy->status == status) {
          if (uintptr_t(buddy) < uintptr_t(obj)) obj = buddy;
          scavenge->pop(buddy);
          if (status == STATUS_SCAVENGED(0)) {
            //printf("page free\n");
            g_SAT.freeSegmentSpan(uintptr_t(obj) >> sat::SegmentSizeL2, 1);
          }
          else {
            status = status - 1;
            uintptr_t objectSize = (supportSize >> (status&STATUS_SIZEL2_MASK));
            buddy = Object(uintptr_t(obj) ^ objectSize);
            obj->status = status;
            obj->buddy = buddy;
            obj->next = obj_next;
            obj_next = obj;
            //printf("%.8X : %X\n", obj, obj->status);
          }
        }
        // When the buddy is used
        else {
          scavenge->push(obj);
        }
        obj = obj_next;
      }

      // Release scavenging
      scavenge->lock.unlock();
      return merged;
    }
    void flushCache() {
      tObjectList list;
      for (int sizeID = 0; sizeID < 24; sizeID++) {
        ObjectManifold* cache = &this->manifolds[sizeID];
        cache->lock.lock();
        list = cache->objects.flush();
        cache->lock.unlock();

        if (list) {
          ScavengeManifold* scavenge = &this->scavenge_manifolds[sizeID];
          scavenge->lock.lock();
          if (list = scavengePlainZones(sizeID, list)) {
            printf("lost %d bytes\n", list.count*(supportSize >> sizeID));
          }
        }
      }
    }
    tObjectList releasePlainZones(int sizeID, tObjectList list)
    {
      ObjectManifold* cache = &this->manifolds[sizeID];
      cache->lock.lock();
      int count = cache->objects.count + list.count;

      // Check if scavenging shall be done
      if (count > objectOverflowThreshold) {
        ScavengeManifold* scavenge = &this->scavenge_manifolds[sizeID];
        if (scavenge->lock.tryLock()) {

          // Pop the global cache overflow
          int n = cache->objects.count - objectUnderflowThreshold;
          if (n > 0) {
            list.append(cache->objects.pop_list(n));
          }
          cache->lock.unlock();

          // Scavenge the overflow
          return this->scavengePlainZones(sizeID, list);
        }
      }

      // Fill the global cache
      cache->objects.count = count;
      list.last->next = cache->objects.first;
      cache->objects.first = list.first;
      cache->lock.unlock();
      return tObjectList::empty();
    }
  };
}