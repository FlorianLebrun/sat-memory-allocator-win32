#pragma once

namespace ZonedBuddyAllocator {
   namespace Global {

      template<int sizeID>
      struct SubObjectCache : BaseCache<sizeID, 3>
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
            tObjectList<Object> list = tObjectList<Object>::empty();
            // --- Find the last zone to remove
            while (list.count < objectPerTryScavenge)
            {
               if (ObjectEx cur = this->scavenge.pop()) {
                  uintptr_t ptr = uintptr_t(cur);
                  int countL2 = sizeID - (cur->status & 0xf);
                  int count = 1 << countL2;
                  for (int i = 0; i < count; i++) {
                     ObjectEx obj = ObjectEx(ptr);
                     obj->status = sizeID;
                     list.push(obj);
                     ptr += objectSize;
                  }
               }
               else break;
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
                  int status = STATUS_SCAVENGED(sizeID);
                  for (int upperSize = objectSize << 1;; upperSize <<= 1) {

                     // Compute the upper object
                     if (uintptr_t(buddy) < uintptr_t(obj)) obj = buddy;
                     this->scavenge.pop(ObjectEx(buddy));

                     // Compute the upper buddy
                     buddy = Object(uintptr_t(obj) ^ upperSize);

                     // Compute the upper status for scavenge
                     if ((--status) == STATUS_SCAVENGED(3)) {
                        obj->status = STATUS_FREE(3);
                        obj->buddy = buddy;
                        merged.push(obj);
                        break;
                     }
                     else if (buddy->status != status) {
                        obj->status = status;
                        obj->buddy = buddy;
                        this->scavenge.push(ObjectEx(obj));
                        break;
                     }
                  }
               }
               // When the buddy is used
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
         int releaseObject(SATEntry entry, Object obj, int index, uintptr_t ptr) {

            // Find object
            obj->status = STATUS_FREE(sizeID);

            // Release object
            tObjectList<Object> list(obj);
            if (list = this->releaseObjects(list)) {
               return upper->releaseObjects(list);
            }
            return objectSize;
         }
      };

   }
}
