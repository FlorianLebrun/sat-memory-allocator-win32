#pragma once

namespace ZonedBuddyAllocator {
   namespace Global {

      template<int sizeID>
      struct ObjectCache;

      template<int sizeID, int upperSizeID>
      struct BaseCache : public sat::IObjectAllocator
      {
         typedef ObjectCache<upperSizeID> tUpperCache;

         static const int sizeL2 = supportSizeL2 - sizeID;
         static const uintptr_t objectSize = 1 << sizeL2;
         static const uintptr_t objectPtrMask = uint64_t(-1) << sizeL2;
         static const uintptr_t objectOffsetMask = ~objectPtrMask;
         static const int objectPerTryCache = (4 >> (supportLengthL2 - sizeID)) || 1;
         static const int objectPerTryScavenge = (4 >> (supportLengthL2 - sizeID)) || 1;
         static const size_t allocatedSize = objectSize;

         sat::SpinLock lock;
         PageObjectCache* heap;
         tUpperCache* upper;
         tObjectList<Object> objects;
         ScavengeManifold<ObjectEx> scavenge;

         virtual size_t getMaxAllocatedSize() override { return allocatedSize; }
         virtual size_t getMinAllocatedSize() override { return allocatedSize; }
         virtual size_t getAllocatedSize(size_t size) override { return allocatedSize; }
         virtual size_t getMaxAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getMinAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getAllocatedSizeWithMeta(size_t size) override { return allocatedSize - sizeof(sat::tObjectMetaData); }

         virtual void* allocate(size_t) override { return 0; }
         virtual void* allocateWithMeta(size_t, uint64_t meta) override { return 0; }
      };

      template<int sizeID>
      struct ObjectCache : BaseCache<sizeID, (sizeID - 1) & 3>
      {
         void init(PageObjectCache* heap, tUpperCache* upper)
         {
            this->heap = heap;
            this->upper = upper;
            this->objects = tObjectList<Object>::empty();
         }
         int tryPopObjects(tObjectList<Object>& list)
         {
            // Try in cache
            if (this->objects.count) {
               this->lock.lock();
               list = this->objects.pop_list(objectPerTryCache);
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
         void flushCache() {
            tObjectList<Object> list;

            this->lock.lock();
            list = this->objects.flush();
            this->lock.unlock();

            if (list) {
               this->scavenge.lock.lock();
               if (list = this->scavengeObjects(list)) {
                  this->upper->releaseObjects(list);
               }
            }
         }
         tObjectList<Object> unscavengeObjects() {

            // Pop all needed zones
            tObjectList<Object> list;
            // --- Find the last zone to remove
            list.count = 0;
            list.first = list.last = this->scavenge.objects;
            while (list.last && list.count < objectPerTryScavenge) {
               list.count++;
               list.last->status ^= 0x40;
               list.last = list.last->next;
            }
            // --- Extract the zones before the last zone
            list.first = this->scavenge.objects;
            if (list.last) {
               ObjectEx(list.last)->back->next = 0;
               ObjectEx(list.last)->back = ObjectEx(&this->scavenge.objects);
               this->scavenge.objects = ObjectEx(list.last);
               this->scavenge.count -= list.count;
            }
            else {
               this->scavenge.objects = 0;
               this->scavenge.count = 0;
            }
            this->scavenge.lock.unlock();
            OBJECT_LIST_CHECK(this->scavenge._check());
            OBJECT_LIST_CHECK(list);
            return list;
         }
         tObjectList<Object> scavengeObjects(tObjectList<Object> list)
         {
            SAT_TRACE(printf("scavengeObjects\n"));
            tObjectList<Object> merged = tObjectList<Object>::empty();
            Object obj = list.first;
            while (obj) {
               Object _next = obj->next;
               assert(obj->status == STATUS_FREE(sizeID));
               Object buddy = obj->buddy;

               // When the buddy is scavenged too
               if (buddy->status == STATUS_SCAVENGED(sizeID)) {

                  // Extract the upper object
                  if (uintptr_t(buddy) < uintptr_t(obj)) obj = buddy;
                  this->scavenge.pop(ObjectEx(buddy));

                  // Release a page when the upper is a page
                  if (sizeID == 0) {
                     this->heap->releasePage(uintptr_t(obj));
                  }
                  // Release the object as merged upper object
                  else {
                     obj->status = STATUS_FREE(sizeID - 1);
                     obj->buddy = Object(uintptr_t(obj) ^ (objectSize << 1));
                     merged.push(obj);
                  }
               }
               // Otherwise push object in scavenge list
               else {
                  obj->status = STATUS_SCAVENGED(sizeID);
                  this->scavenge.push(ObjectEx(obj));
               }
               obj = _next;
            }

            // Release scavenging
            this->scavenge.lock.unlock();
            return merged;
         }
         tObjectList<Object> releaseObjects(tObjectList<Object> list)
         {
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
         size_t releaseObject(SATEntry entry, Object obj, int index, uintptr_t ptr) {

            // Retag as FREE
            obj->status = STATUS_FREE(sizeID);
            WriteTags<sizeID, 0>::apply(entry, index);

            // Release object
            tObjectList<Object> list(obj);
            if (list = this->releaseObjects(list)) {
               if (sizeID > 0) {
                  return this->upper->releaseObjects(list);
               }
               else {
                  printf("memory leak on scavenged page\n");
               }
            }
            return objectSize;
         }
         int getCachedSize() {
            return 0;
         }
      };
   }
}
