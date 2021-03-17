#pragma once

namespace ZonedBuddyAllocator {
   namespace Local {

      template<int sizeID>
      struct SubObjectCache : sat::IObjectAllocator {
         typedef Global::SubObjectCache<sizeID> tGlobalCache;
         typedef ObjectCache<3> tUpperCache;

         static const int sizeL2 = supportSizeL2 - sizeID;
         static const uintptr_t objectSize = 1 << sizeL2;
         static const uintptr_t objectPtrMask = uint64_t(-1) << sizeL2;
         static const uintptr_t objectOffsetMask = ~objectPtrMask;
         static const int objectPerUpper = baseSize >> sizeL2;
         static const size_t allocatedSize = objectSize - tObject::headerSize;

         tGlobalCache* global;
         PageObjectCache* heap;
         tUpperCache* upper;
         tObjectList<Object> objects;

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
               return obj;
            }

            // Case 2: Alloc list from global cache
            if (global->tryPopObjects(this->objects)) {
               return this->objects.pop();
            }

            // Case 3: Alloc a base zone to split in sub size object
            // --- Get a free zone
            uintptr_t ptr = (uintptr_t)upper->acquireObject();
            SATEntry entry = sat::memory::table->get<tSATEntry>(ptr >> sat::memory::cSegmentSizeL2);
            entry->tags[(ptr >> baseSizeL2) & 0xf] = sizeID | cTAG_ALLOCATED_BIT;

            // --- Cache the remain objects
            Object obj_0, obj_1;
            this->objects.count = objectPerUpper;
            this->objects.first = Object(ptr);
            for (int i = 0; i < objectPerUpper; i += 2) {
               obj_0 = Object(ptr); ptr += objectSize;
               obj_1 = Object(ptr); ptr += objectSize;
               obj_0->status = STATUS_FREE(sizeID);
               obj_0->zoneID = 0;
               obj_0->buddy = obj_1;
               obj_0->next = obj_1;
               obj_1->status = STATUS_FREE(sizeID);
               obj_1->zoneID = 0;
               obj_1->buddy = obj_0;
               obj_1->next = Object(ptr);
            }
            obj_1->next = 0;
            this->objects.last = obj_1;

            return this->objects.pop();
         }

         virtual size_t getMaxAllocatedSize() override { return allocatedSize; }
         virtual size_t getMinAllocatedSize() override { return allocatedSize; }
         virtual size_t getAllocatedSize(size_t size) override { return allocatedSize; }
         virtual size_t getMaxAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getMinAllocatedSizeWithMeta() override { return allocatedSize - sizeof(sat::tObjectMetaData); }
         virtual size_t getAllocatedSizeWithMeta(size_t size) override { return allocatedSize - sizeof(sat::tObjectMetaData); }

         virtual void* allocate(size_t size) override
         {
            // Acquire free object
            Object obj = this->acquireObject();
            assert(obj->status == STATUS_FREE(sizeID));
            assert(size <= this->getMaxAllocatedSize());

            // Prepare object content
            uint64_t* ptr = obj->content();
            obj->tag = cTAG_ALLOCATED_BIT;
            obj->status = STATUS_ALLOCATE(sizeID);

            return ptr;
         }
         virtual void* allocateWithMeta(size_t size, uint64_t meta) override
         {
            // Acquire free object
            Object obj = this->acquireObject();
            assert(obj->status == STATUS_FREE(sizeID));
            assert(size <= this->getMaxAllocatedSizeWithMeta());

            // Prepare object content
            uint64_t* ptr = obj->content();
            (ptr++)[0] = meta;
            obj->tag = cTAG_ALLOCATED_BIT | cTAG_METADATA_BIT;
            obj->status = STATUS_ALLOCATE(sizeID);

            return ptr;
         }
         void freeObjectList(tObjectList<Object> list) {
            this->objects.append(list);
            if (this->objects.count > localOverflowThreshold) {
               return this->scavengeOverflow(localUnderflowThreshold);
            }
         }
         void scavengeOverflow(int remain) {
            if (tObjectList<Object> list = this->objects.pop_overflow(remain)) {
               if (list = global->releaseObjects(list)) {
                  return upper->freeObjectList(list);
               }
            }
         }
         int releaseObject(SATEntry entry, Object obj, int index, uintptr_t ptr) {

            // Find object
            obj->status = STATUS_FREE(sizeID);

            // Release object
            if (this->objects.push(obj) > localOverflowThreshold) {
               this->scavengeOverflow(localUnderflowThreshold);
            }
            return objectSize;
         }
      };
   }
}