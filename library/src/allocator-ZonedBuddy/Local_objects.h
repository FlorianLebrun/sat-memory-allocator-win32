
namespace ZonedBuddyAllocator {
  namespace Local {

    template<int sizeID>
    struct ObjectCache : public IObjectAllocator {
      typedef Global::ObjectCache<sizeID> tGlobalCache;
      typedef ObjectCache<(sizeID - 1) & 3> tUpperCache;

      static const int sizeL2 = supportSizeL2 - sizeID;
      static const uintptr_t objectSize = 1 << sizeL2;
      static const uintptr_t objectPtrMask = uint64_t(-1) << sizeL2;
      static const uintptr_t objectOffsetMask = ~objectPtrMask;

      tObjectList<Object> objects;
      PageObjectCache* heap;
      tUpperCache* upper;
      tGlobalCache* global;

      void init(PageObjectCache* heap, tUpperCache* upper, tGlobalCache* global)
      {
        this->heap = heap;
        this->upper = upper;
        this->global = global;
        this->objects = tObjectList<Object>::empty();
      }
      Object acquireObject()
      {
        // Case 1: Alloc an object from local cache
        if (Object obj = this->objects.pop()) {
          assert(obj->status == STATUS_FREE(sizeID));
          return obj;
        }

        // Case 2: Alloc list from global cache
        if (global->tryPopObjects(this->objects)) {
          return this->objects.pop();
        }

        // Case 3: Alloc list from upper zone
        Object obj_0;
        if (sizeID > 0) obj_0 = this->upper->acquireObject();
        else obj_0 = Object(global->heap->acquirePage());
        Object obj_1 = Object(uintptr_t(obj_0) ^ objectSize);

        // --- Update object
        obj_0->status = STATUS_FREE(sizeID);
        obj_0->zoneID = 0;
        obj_0->buddy = obj_1;
        obj_0->next = 0;

        // --- Release buddy object
        obj_1->status = STATUS_FREE(sizeID);
        obj_1->buddy = obj_0;
        obj_1->zoneID = 0;
        obj_1->next = 0;
        this->objects.first = obj_1;
        this->objects.last = obj_1;
        this->objects.count = 1;

        return obj_0;
      }
      virtual size_t allocatedSize() override {
        return objectSize - tObject::headerSize;
      }
      virtual void* allocObject(size_t, uint64_t meta)
      {
        // Acquire free object
        const Object obj = this->acquireObject();
        assert(obj->status == STATUS_FREE(sizeID));

        // Prepare object content
        uint64_t* ptr = obj->content();
        if(meta) {
          (ptr++)[0] = meta;// Prepare metadata before declare object as allocated with type
          obj->tag = cTAG_ALLOCATED_BIT|cTAG_METADATA_BIT;
        }
        else obj->tag = cTAG_ALLOCATED_BIT;
        obj->status = STATUS_ALLOCATE(sizeID);

        // Tag the entry for this object
        const SATEntry entry = g_SATable->get<tSATEntry>(uintptr_t(obj) >> SAT::cSegmentSizeL2);
        WriteTags<sizeID, sizeID | cTAG_ALLOCATED_BIT>::apply(entry, uintptr_t(obj) >> baseSizeL2);

        return ptr;
      }
      void freeObjectList(tObjectList<Object> list) {
        this->objects.append(list);
        if (this->objects.count > localOverflowThreshold) {
          return this->scavengeOverflow(localUnderflowThreshold);
        }
      }
      void scavengeOverflow(int remain = 0) {
        SAT_TRACE(printf("scavengeOverflow\n"));
        if (tObjectList<Object> list = this->objects.pop_overflow(remain)) {
          if (list = global->releaseObjects(list)) {
            if (sizeID > 0) {
              return this->upper->freeObjectList(list);
            }
            else {
              printf("memory leak on scavenged page\n");
            }
          }
        }
      }
      size_t releaseObject(SATEntry entry, Object obj, int index, uintptr_t ptr) {

        // Retag as FREE
        obj->status = STATUS_FREE(sizeID);
        WriteTags<sizeID, 0>::apply(entry, index);

        // Release object
        if (this->objects.push(obj) > localOverflowThreshold) {
          this->scavengeOverflow(localUnderflowThreshold);
        }
        return objectSize;
      }
    };
  }
}