
namespace ZonedBuddyAllocator {
  namespace Local {

    template<class TBaseCache, int sizeID, int dividor>
    struct ZonedObjectCache : SAT::IObjectAllocator
    {
      typedef Global::ZonedObjectCache<typename TBaseCache::tGlobalCache, sizeID, dividor>* GlobalCache;

      static const int subSizeID = sizeID | (dividor << 2);
      static const int subObjectSize = (((supportSize >> sizeID) - sizeof(tZone)) / dividor)&(-8);
      static const size_t allocatedSize = subObjectSize;

      GlobalCache global;
      TBaseCache* heap;
      tObjectList<ZoneObject> objects;

      void init(TBaseCache* heap, GlobalCache global)
      {
        this->heap = heap;
        this->global = global;
        this->objects = tObjectList<ZoneObject>::empty();
      }
      ZoneObject acquireObject()
      {
        // Case 1: Alloc an object from local cache
        if (ZoneObject obj = this->objects.pop()) {
          return obj;
        }

        // Case 2: Alloc list from global cache
        if (global->tryPopObjects(this->objects)) {
          SAT_TRACE_ALLOC(printf("acquireObject(case 2) try pop objects from global\n"));
          return this->objects.pop();
        }

        // Case 3: Alloc a base zone to split in sub size object
        SAT_TRACE_ALLOC(printf("acquireObject(case 3) zone filling\n"));
        // --- Get a free zone
        Zone zone = (Zone)this->heap->acquireObject();
        assert(zone->status == STATUS_FREE(sizeID));
        zone->zoneID = dividor;
        zone->status = STATUS_ALLOCATE(sizeID);
        ((uint64_t*)zone->tags)[0] = 0;
        ((uint64_t*)zone->tags)[1] = 0;
        zone->freeOffset = 0;
        zone->freeCount = 0;
        zone->zoneStatus = cSTATUS_ALLOCATE_ID;

        // --- Retag the SAT entries for this object
        if(sizeID < baseCountL2) {
          SATEntry entry = g_SATable->get<tSATEntry>(uintptr_t(zone) >> SAT::cSegmentSizeL2);
          WriteTags<sizeID, sizeID | cTAG_ALLOCATED_BIT>::apply(entry, uintptr_t(zone) >> baseSizeL2);
        }

        // --- Cache the remain objects
        ZoneObject obj;
        uintptr_t ptr = uintptr_t(zone) + sizeof(tZone);
        this->objects.count = dividor;
        this->objects.first = ZoneObject(ptr);
        for (int i = 0; i < dividor; i++) {
          obj = ZoneObject(ptr); ptr += subObjectSize;
          obj->next = ZoneObject(ptr);
          obj->tag = &zone->tags[i];
          obj->tag[0] = cZONETAG_FREE;
        }
        obj->next = 0;
        this->objects.last = obj;

        return this->objects.pop();
      }

      virtual size_t getMaxAllocatedSize() override {return allocatedSize;}
      virtual size_t getMinAllocatedSize() override {return allocatedSize;}
      virtual size_t getAllocatedSize(size_t size) override {return allocatedSize;}
      virtual size_t getMaxAllocatedSizeWithMeta() override {return allocatedSize-sizeof(SAT::tObjectMetaData);}
      virtual size_t getMinAllocatedSizeWithMeta() override {return allocatedSize-sizeof(SAT::tObjectMetaData);}
      virtual size_t getAllocatedSizeWithMeta(size_t size) override {return allocatedSize-sizeof(SAT::tObjectMetaData);}

      virtual void* allocate(size_t size) override
      {
        ZoneObject obj = this->acquireObject();
        assert(obj->tag[0] == cZONETAG_FREE);
        assert(size <= this->getMaxAllocatedSize());
        obj->tag[0] = cZONETAG_ALLOCATED_BIT;
        return obj;
      }
      virtual void* allocateWithMeta(size_t size, uint64_t meta) override
      {
        ZoneObject obj = this->acquireObject();
        assert(obj->tag[0] == cZONETAG_FREE);
        assert(size <= this->getMaxAllocatedSizeWithMeta());
        obj->meta = meta;
        obj->tag[0] = cZONETAG_ALLOCATED_BIT|cZONETAG_METADATA_BIT;
        return &((uint64_t*)obj)[1];
      }
      void scavengeOverflow(int remain) {
        SAT_TRACE(printf("scavengeOverflow\n"));
        if (tObjectList<ZoneObject> list = this->objects.pop_overflow(remain)) {
          if (tObjectList<Object> mergeds = global->releaseObjects(list)) {
            return heap->freeObjectList(mergeds);
          }
        }
      }
      size_t freeObject(SATEntry entry, Zone zone, uintptr_t ptr) {

        // Find object
        uintptr_t offset = ptr&TBaseCache::objectOffsetMask;
        assert(offset >= sizeof(tZone));
        uintptr_t index = (offset - sizeof(tZone)) / subObjectSize;
        ZoneObject obj = ZoneObject((index*subObjectSize) + uintptr_t(zone) + sizeof(tZone));
        assert(zone->tags[index] & cZONETAG_ALLOCATED_BIT);
        obj->tag = &zone->tags[index];
        obj->tag[0] = cZONETAG_FREE;

        // Release object
        if (this->objects.push(obj) > localOverflowThreshold) {
          this->scavengeOverflow(localUnderflowThreshold);
        }
        return subObjectSize;
      }
    };

    template <int sizeID, class TBaseCache>
    struct ZoneCache : public TBaseCache
    {
      typedef typename TBaseCache tBaseCache;
      typedef Global::ZoneCache<sizeID, typename TBaseCache::tGlobalCache> tGlobalZonedCache;

      ZonedObjectCache<TBaseCache, sizeID, 2> zone_cache_2;
      ZonedObjectCache<TBaseCache, sizeID, 3> zone_cache_3;
      ZonedObjectCache<TBaseCache, sizeID, 4> zone_cache_4;
      ZonedObjectCache<TBaseCache, sizeID, 5> zone_cache_5;
      ZonedObjectCache<TBaseCache, sizeID, 6> zone_cache_6;
      ZonedObjectCache<TBaseCache, sizeID, 7> zone_cache_7;
      ZonedObjectCache<TBaseCache, sizeID, 8> zone_cache_8;
      ZonedObjectCache<TBaseCache, sizeID, 9> zone_cache_9;
      ZonedObjectCache<TBaseCache, sizeID, 10> zone_cache_10;
      ZonedObjectCache<TBaseCache, sizeID, 11> zone_cache_11;
      ZonedObjectCache<TBaseCache, sizeID, 12> zone_cache_12;
      ZonedObjectCache<TBaseCache, sizeID, 13> zone_cache_13;
      ZonedObjectCache<TBaseCache, sizeID, 14> zone_cache_14;
      ZonedObjectCache<TBaseCache, sizeID, 15> zone_cache_15;

      void init_zoning() {
        tGlobalZonedCache* global = (tGlobalZonedCache*)this->global;
        this->zone_cache_2.init(this, &global->zone_cache_2);
        this->zone_cache_3.init(this, &global->zone_cache_3);
        this->zone_cache_4.init(this, &global->zone_cache_4);
        this->zone_cache_5.init(this, &global->zone_cache_5);
        this->zone_cache_6.init(this, &global->zone_cache_6);
        this->zone_cache_7.init(this, &global->zone_cache_7);
        this->zone_cache_8.init(this, &global->zone_cache_8);
        this->zone_cache_9.init(this, &global->zone_cache_9);
        this->zone_cache_10.init(this, &global->zone_cache_10);
        this->zone_cache_11.init(this, &global->zone_cache_11);
        this->zone_cache_12.init(this, &global->zone_cache_12);
        this->zone_cache_13.init(this, &global->zone_cache_13);
        this->zone_cache_14.init(this, &global->zone_cache_14);
        this->zone_cache_15.init(this, &global->zone_cache_15);
      }
      int getCachedSize() {
        return this->objects.count << sizeL2;
      }
      int freeObject(SATEntry entry, int index, uintptr_t ptr)
      {
        // Find object address
        const Object obj = Object(ptr&objectPtrMask);

        // Switch on the right procedure
        if(obj->status == STATUS_ALLOCATE(sizeID)) {
          switch (obj->zoneID) {
          case 0:
          case 1:return this->releaseObject(entry, obj, index, ptr);
          case 2:return this->zone_cache_2.freeObject(entry, Zone(obj), ptr);
          case 3:return this->zone_cache_3.freeObject(entry, Zone(obj), ptr);
          case 4:return this->zone_cache_4.freeObject(entry, Zone(obj), ptr);
          case 5:return this->zone_cache_5.freeObject(entry, Zone(obj), ptr);
          case 6:return this->zone_cache_6.freeObject(entry, Zone(obj), ptr);
          case 7:return this->zone_cache_7.freeObject(entry, Zone(obj), ptr);
          case 8:return this->zone_cache_8.freeObject(entry, Zone(obj), ptr);
          case 9:return this->zone_cache_9.freeObject(entry, Zone(obj), ptr);
          case 10:return this->zone_cache_10.freeObject(entry, Zone(obj), ptr);
          case 11:return this->zone_cache_11.freeObject(entry, Zone(obj), ptr);
          case 12:return this->zone_cache_12.freeObject(entry, Zone(obj), ptr);
          case 13:return this->zone_cache_13.freeObject(entry, Zone(obj), ptr);
          case 14:return this->zone_cache_14.freeObject(entry, Zone(obj), ptr);
          case 15:return this->zone_cache_15.freeObject(entry, Zone(obj), ptr);
          }
        }
        throw std::exception("Cannot free a not allocated object");
      }
      void flushCache() {
        this->zone_cache_2.scavengeOverflow(0);
        this->zone_cache_3.scavengeOverflow(0);
        this->zone_cache_4.scavengeOverflow(0);
        this->zone_cache_5.scavengeOverflow(0);
        this->zone_cache_6.scavengeOverflow(0);
        this->zone_cache_7.scavengeOverflow(0);
        this->zone_cache_8.scavengeOverflow(0);
        this->zone_cache_9.scavengeOverflow(0);
        this->zone_cache_10.scavengeOverflow(0);
        this->zone_cache_11.scavengeOverflow(0);
        this->zone_cache_12.scavengeOverflow(0);
        this->zone_cache_13.scavengeOverflow(0);
        this->zone_cache_14.scavengeOverflow(0);
        this->zone_cache_15.scavengeOverflow(0);
        this->scavengeOverflow(0);
      }
    };
  }
}
