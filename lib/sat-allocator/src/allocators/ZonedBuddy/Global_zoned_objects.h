#pragma once

namespace ZonedBuddyAllocator {
   namespace Global {

      template<class TBaseCache, int sizeID, int dividor>
      struct ZonedObjectCache : public sat::IObjectAllocator {

         static const int subSizeID = sizeID | (dividor << 2);
         static const int subObjectSize = (((supportSize >> sizeID) - sizeof(tZone)) / dividor) & (-8);
         static const int subObjectPerZone = dividor;
         static const int subObjectPerTryCache = (4 >> (supportLengthL2 - sizeID)) || 1;
         static const int subObjectPerTryScavenge = (4 >> (supportLengthL2 - sizeID)) || 1;
         static const int allocatedSize = subObjectSize;

         sat::SpinLock lock;
         tObjectList<ZoneObject> objects;
         TBaseCache* heap;

         ScavengeManifold<Zone> scavenge;

         virtual size_t getMaxAllocatedSize() override { return allocatedSize; }
         virtual size_t getMinAllocatedSize() override { return allocatedSize; }
         virtual size_t getAllocatedSize(size_t size) override { return allocatedSize; }
         virtual size_t getMaxAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getMinAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getAllocatedSizeWithMeta(size_t size) override { return allocatedSize - sizeof(sat::tObjectMetaData); }

         virtual void* allocate(size_t) override { return 0; }
         virtual void* allocateWithMeta(size_t, uint64_t meta) override { return 0; }

         void init(TBaseCache* heap)
         {
            this->heap = heap;
            this->objects = tObjectList<ZoneObject>::empty();
         }
         int tryPopObjects(tObjectList<ZoneObject>& list)
         {
            // Try in cache
            if (this->objects.count) {
               this->lock.lock();
               list = this->objects.pop_list(subObjectPerTryCache);
               this->lock.unlock();
               return list.count;
            }

            // Try in scavenge
            if (this->scavenge.count) {
               if (this->scavenge.lock.tryLock()) {
                  list = this->unscavengeObjects();
                  return list.count;
               }
            }

            return 0;
         }
         tObjectList<ZoneObject> unscavengeObjects() {
            int zones_count = 0;
            int object_count = 0;

            // Pop all needed zones
            // --- Find the last zone to remove
            Zone last_zone = this->scavenge.objects;
            while (last_zone && object_count < subObjectPerTryScavenge) {
               object_count += last_zone->freeCount;
               last_zone = Zone(last_zone->next);
               zones_count++;
            }
            // --- Extract the zones before the last zone
            Zone zones = this->scavenge.objects;
            if (last_zone) {
               last_zone->back->next = 0;
               last_zone->back = Zone(&this->scavenge.objects);
               this->scavenge.objects = last_zone;
               this->scavenge.count -= zones_count;
               this->scavenge.lock.unlock();
            }
            else {
               this->scavenge.objects = 0;
               this->scavenge.count = 0;
               this->scavenge.lock.unlock();
               return tObjectList<ZoneObject>::empty();
            }
            OBJECT_LIST_CHECK(this->scavenge._check());

            // Fill the list with acquired zones
            Object first = 0, last;
            tObjectList<ZoneObject> list;
            ZoneObject last_obj = 0;
            while (zones) {
               uintptr_t ptr = uintptr_t(zones) + sizeof(tZone);
               for (int offset = zones->freeOffset; offset; offset = tpBytes(zones)[offset]) {
                  int index = offset - (sizeof(tZone) - 16);
                  ZoneObject obj = ZoneObject(ptr + (index * subObjectSize));
                  assert(obj->tag == &zones->tags[index]);
                  if (last_obj) last_obj->next = obj;
                  else list.first = obj;
                  last_obj = obj;
               }
               zones = Zone(zones->next);
            }
            last_obj->next = 0;
            list.last = last_obj;
            list.count = object_count;
            OBJECT_LIST_CHECK(list._check());
            return list;
         }
         tObjectList<Object> scavengeObjects(tObjectList<ZoneObject> list)
         {
            SAT_TRACE(printf("scavengeObjects\n"));
            tObjectList<Object> merged = tObjectList<Object>::empty();
            ZoneObject obj = list.first;
            while (obj) {
               ZoneObject _next = obj->next;
               Zone zone = Zone(uintptr_t(obj->tag) & (-64));

               // Chain the free objects
               int freeOffset = uintptr_t(obj->tag) - uintptr_t(zone);
               obj->tag[0] = zone->freeOffset;
               zone->freeOffset = freeOffset;
               zone->freeCount++;

               // When zone is emptry in scavenging
               if (zone->zoneStatus == cSTATUS_SCAVENGED_ID) {
                  if (zone->freeCount == subObjectPerZone) {
                     zone->status = STATUS_FREE(sizeID);
                     zone->zoneID = 0;
                     this->scavenge.pop(zone);
                     merged.push(zone);
                  }
               }
               // Zone is scavenged for the first time
               else if (zone->freeCount == 1) {
                  zone->zoneStatus = cSTATUS_SCAVENGED_ID;
                  this->scavenge.push(zone);
               }

               obj = _next;
            }
            this->scavenge.lock.unlock();
            return merged;
         }
         tObjectList<Object> releaseObjects(tObjectList<ZoneObject> list) {
            SAT_TRACE(printf("releaseObjects\n"));
            this->lock.lock();
            int count = this->objects.count + list.count;

            // Check if scavenging shall be done
            if (count > globalOverflowThreshold) {
               if (this->scavenge.lock.tryLock()) {

                  // Pop the global cache overflow
                  int n = this->objects.count - globalUnderflowThreshold;
                  if (n > 0) {
                     list.append(this->objects.pop_list(n));
                  }
                  this->lock.unlock();

                  // Scavenge the overflow
                  return this->scavengeObjects(list);
               }
            }

            // Fill the global cache
            this->objects.append(list);
            this->lock.unlock();
            return tObjectList<Object>::empty();
         }
         int freeObject(SATEntry entry, Zone zone, uintptr_t ptr) {
            // Find object
            uintptr_t offset = ptr & TBaseCache::objectOffsetMask;
            uintptr_t index = (offset - sizeof(tZone)) / subObjectSize;
            ZoneObject obj = ZoneObject((index * subObjectSize) + uintptr_t(zone) + sizeof(tZone));
            assert(zone->tags[index] & cZONETAG_ALLOCATED_BIT);
            obj->tag = &zone->tags[index];
            obj->tag[0] = cZONETAG_FREE;

            // Release object
            this->releaseObjects(tObjectList<ZoneObject>(obj));
            return this->subObjectSize;
         }
         void markObject(SATEntry entry, Zone zone, uintptr_t ptr) {
            // Find object
            uintptr_t offset = ptr & TBaseCache::objectOffsetMask;
            uintptr_t index = (offset - sizeof(tZone)) / subObjectSize;
            ZoneObject obj = ZoneObject((index * subObjectSize) + uintptr_t(zone) + sizeof(tZone));
            assert(zone->tags[index] & cZONETAG_ALLOCATED_BIT);
            obj->tag = &zone->tags[index];
            obj->tag[0] |= cZONETAG_MARK_BIT;
         }
      };

      template <int sizeID, class TBaseCache>
      struct ZoneCache : public TBaseCache
      {
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

         void init_zoning()
         {
            this->zone_cache_2.init(this);
            this->zone_cache_3.init(this);
            this->zone_cache_4.init(this);
            this->zone_cache_5.init(this);
            this->zone_cache_6.init(this);
            this->zone_cache_7.init(this);
            this->zone_cache_8.init(this);
            this->zone_cache_9.init(this);
            this->zone_cache_10.init(this);
            this->zone_cache_11.init(this);
            this->zone_cache_12.init(this);
            this->zone_cache_13.init(this);
            this->zone_cache_14.init(this);
            this->zone_cache_15.init(this);
         }
         int freeObject(SATEntry entry, int index, uintptr_t ptr)
         {
            // Find object address
            const Object obj = Object(ptr & objectPtrMask);

            // Switch on the right procedure
            if (obj->status == STATUS_ALLOCATE(sizeID)) {
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
         void markObject(SATEntry entry, int index, uintptr_t ptr)
         {
            // Find object address
            const Object obj = Object(ptr & objectPtrMask);
            assert(obj->status == STATUS_ALLOCATE(sizeID));

            // Switch on the right procedure
            switch (obj->zoneID) {
            case 0:
            case 1:return;
            case 2:return this->zone_cache_2.markObject(entry, Zone(obj), ptr);
            case 3:return this->zone_cache_3.markObject(entry, Zone(obj), ptr);
            case 4:return this->zone_cache_4.markObject(entry, Zone(obj), ptr);
            case 5:return this->zone_cache_5.markObject(entry, Zone(obj), ptr);
            case 6:return this->zone_cache_6.markObject(entry, Zone(obj), ptr);
            case 7:return this->zone_cache_7.markObject(entry, Zone(obj), ptr);
            case 8:return this->zone_cache_8.markObject(entry, Zone(obj), ptr);
            case 9:return this->zone_cache_9.markObject(entry, Zone(obj), ptr);
            case 10:return this->zone_cache_10.markObject(entry, Zone(obj), ptr);
            case 11:return this->zone_cache_11.markObject(entry, Zone(obj), ptr);
            case 12:return this->zone_cache_12.markObject(entry, Zone(obj), ptr);
            case 13:return this->zone_cache_13.markObject(entry, Zone(obj), ptr);
            case 14:return this->zone_cache_14.markObject(entry, Zone(obj), ptr);
            case 15:return this->zone_cache_15.markObject(entry, Zone(obj), ptr);
            }
         }
      };

   }
}
