#pragma once
#include "../../common/bitwise.h"

namespace ZonedBuddyAllocator {

#pragma pack(push)  
#pragma pack(1)
   typedef __declspec(align(1)) struct tObject {
#if UINTPTR_MAX == UINT64_MAX
      static const int headerSize = 24;
#else
      static const int headerSize = 16;
#endif
      tObject* next;
      tObject* buddy;
      uint8_t status;
      uint8_t tag;
      uint8_t zoneID; // >0 when object is zoned (zoneID is zone dividor)

      uint64_t* content() {
         return (uint64_t*)&((char*)this)[headerSize];
      }
   } *Object;
   static_assert(sizeof(tObject) < tObject::headerSize, "Bad tZone size");
#pragma pack(pop)  

#pragma pack(push)  
#pragma pack(1)  
   typedef __declspec(align(1)) struct tZone : tObject {
#if UINTPTR_MAX == UINT64_MAX
      static const int headerSize = 48;
#else
      static const int headerSize = 36;
#endif
      uint8_t freeCount;
      uint8_t freeOffset;
      uint8_t zoneStatus;
      uint8_t __[2];
      tZone* back;
      uint8_t tags[16];
   } *Zone;
   static_assert(sizeof(tZone) == tZone::headerSize, "Bad tZone size");
#pragma pack(pop)  

   typedef struct tZoneObject {
      union {
         tZoneObject* next;
         uint64_t meta;
      };
      uint8_t* tag;
      Zone getZone() { return Zone(uintptr_t(tag) & (-64)); }
   } *ZoneObject;

   typedef struct tObjectEx :tObject {
      tObject* back;
   } *ObjectEx;


   template <class TObject>
   struct tObjectList {
      TObject first;
      TObject last;
      int count;

      tObjectList() {
      }
      tObjectList(TObject obj) {
         obj->next = 0;
         this->first = obj;
         this->last = obj;
         this->count = 1;
      }
      operator int() {
         return this->count;
      }
      void _check() {
         TObject obj;
         int ccount = 0;
         for (obj = first; obj; obj = obj->next) {
            if (!obj->next) {
               assert(obj == this->last);
            }
            ccount++;
         }
         assert(this->count == ccount);
      }
      void append(tObjectList<TObject> list) {
         OBJECT_LIST_CHECK(this->_check());
         OBJECT_LIST_CHECK(list._check());
         assert(list.count);
         if (this->count) {
            list.last->next = this->first;
            this->first = list.first;
            this->count += list.count;
         }
         else {
            this->first = list.first;
            this->last = list.last;
            this->count = list.count;
         }
         OBJECT_LIST_CHECK(this->_check());
      }
      int push(TObject obj) {
         OBJECT_LIST_CHECK(this->_check());
         if (this->count++) {
            obj->next = this->first;
            this->first = obj;
         }
         else {
            obj->next = 0;
            this->first = obj;
            this->last = obj;
         }
         OBJECT_LIST_CHECK(this->_check());
         return this->count;
      }
      TObject pop() {
         OBJECT_LIST_CHECK(this->_check());
         if (TObject obj = this->first) {
            this->first = obj->next;
            this->count--;
            return obj;
         }
         OBJECT_LIST_CHECK(this->_check());
         return 0;
      }
      tObjectList<TObject> pop_list(int n)
      {
         OBJECT_LIST_CHECK(this->_check());
         if (this->count < n) {
            tObjectList<TObject> list;
            list.first = this->first;
            list.last = this->last;
            list.count = this->count;
            this->first = 0;
            this->count = 0;
            return list;
         }
         else {
            assert(n > 0);
            tObjectList<TObject> list;
            TObject cur = this->first;
            switch (n) {
            default:
               for (int i = 7; i < n; i++)
                  cur = cur->next;
            case 7: cur = cur->next;
            case 6: cur = cur->next;
            case 5: cur = cur->next;
            case 4: cur = cur->next;
            case 3: cur = cur->next;
            case 2: cur = cur->next;
            case 1: {
               // Fill the destination
               list.first = this->first;
               list.last = cur;
               list.count = n;

               // Reduce the source
               this->count -= n;
               this->first = cur->next;
               cur->next = 0;

               OBJECT_LIST_CHECK(this->_check());
               OBJECT_LIST_CHECK(list._check());
               return list;
            }
            }
         }
      }
      tObjectList<TObject> pop_overflow(int remain)
      {
         OBJECT_LIST_CHECK(this->_check());
         if (remain) {
            tObjectList<TObject> list;
            list.count = this->count - remain;

            TObject obj = this->first;
            this->count = remain;
            while (--remain) {
               obj = obj->next;
            }
            list.last = this->last;
            list.first = obj->next;
            this->last = obj;
            obj->next = 0;
            OBJECT_LIST_CHECK(this->_check());
            OBJECT_LIST_CHECK(list._check());
            return list;
         }
         else {
            tObjectList<TObject> list = *this;
            this->first = 0;
            this->count = 0;
            return list;
         }
      }
      tObjectList<TObject> flush() {
         tObjectList<TObject> list = *this;
         this->first = 0;
         this->count = 0;
         return list;
      }
      static tObjectList<TObject> empty() {
         tObjectList<TObject> list;
         list.first = 0;
         list.count = 0;
         return list;
      }
   };

   template <class TObject>
   struct tObjectListEx {
      TObject objects;
      int count;

      tObjectListEx()
      {
         this->objects = 0;
         this->count = 0;
      }
      void _check() {
         TObject obj;
         int ccount = 0;
         for (obj = objects; obj; obj = TObject(obj->next)) {
            if (!obj->next) {
               //assert(obj == this->last);
            }
            else {
               assert(TObject(obj->next)->back == obj);
            }
            ccount++;
         }
         assert(this->count == ccount);
      }
      void swap(TObject obj1, TObject obj2) {
         if (TObject next = obj2->next = obj1->next) {
            TObject(next)->back = obj2;
         }
         if (TObject back = TObject(obj2)->back = obj1->back) {
            TObject(back)->next = obj2;
         }
      }
      void push(TObject obj)
      {
         TObject(obj)->back = TObject(&this->objects);
         if (TObject next = TObject(obj->next = this->objects)) {
            TObject(next)->back = obj;
         }
         this->count++;
      }
      void pop(TObject obj) {
         TObject(obj)->back->next = obj->next;
         if (obj->next) TObject(obj->next)->back = TObject(obj)->back;
         this->count--;
      }
      TObject pop()
      {
         if (TObject obj = this->objects) {
            if (TObject next = this->objects = TObject(obj->next)) {
               TObject(next)->back = TObject(&this->objects);
            }
            this->count--;
            return obj;
         }
         return 0;
      }
   };

   template<int sizeID, uint64_t value>
   struct WriteTags {
      static int apply(SATEntry entry, int index) { (*(uint8_t*)&entry->tags[index &= 0xf]) = value; return index; }
   };
   template<uint64_t value>
   struct WriteTags<2, value> {
      static int apply(SATEntry entry, int index) { (*(uint16_t*)&entry->tags[index &= 0xe]) = value | (value << 8); return index; }
   };
   template<uint64_t value>
   struct WriteTags<1, value> {
      static int apply(SATEntry entry, int index) { (*(uint32_t*)&entry->tags[index &= 0xc]) = value | (value << 8) | (value << 16) | (value << 24); return index; }
   };
   template<uint64_t value>
   struct WriteTags<0, value> {
      static int apply(SATEntry entry, int index) { (*(uint64_t*)&entry->tags[index &= 0x8]) = value | (value << 8) | (value << 16) | (value << 24) | (value << 32) | (value << 40) | (value << 48) | (value << 56); return index; }
   };

   struct SizeMapping {
      static uint8_t small_objects_sizemap[2048 >> 3];
      static bool init_small_objects_sizemap;
      static void init();

      static uint8_t getMediumSizeID(int size) {
         int sizeL2 = msb_32(size + tObject::headerSize - 1) + 1;
         int sizeID = supportSizeL2 - sizeL2;
         assert(sizeID >= 0 && sizeID < 8);
         assert(size <= (supportSize >> sizeID));
         return sizeID << 4;
      }
      static uint8_t getSizeID(int size) {
         if (size <= 2048) {
            assert(init_small_objects_sizemap);
            int index = (size - 1) >> 3;
            return small_objects_sizemap[index];
         }
         else {
            return getMediumSizeID(size);
         }
      }
   };
}
