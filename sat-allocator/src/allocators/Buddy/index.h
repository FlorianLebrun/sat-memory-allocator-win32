
namespace ScaledBuddySubAllocator {

  union tObjectTag {
    struct {
      unsigned object : 1; // = 1
      unsigned meta : 4; // Metadata
      unsigned sizeID : 3; // Size of object (0-3)
    } object;
    struct {
      unsigned object : 1; // = 1
      unsigned __unused : 4; // = 0
      unsigned sizeID : 3; // Size of object (0-3)
    } object_partial;
    struct {
      unsigned object : 1; // = 1
      unsigned subID : 4; // Sub objects id
      unsigned sizeID : 3; // = 0x7
    } sub_objects;
    struct {
      unsigned object : 1; // = 0
      unsigned status : 2; // = (STATUS_FREE, STATUS_SCAVENGED, STATUS_MERGED)
      unsigned sizeID : 5; // Size of object (0-7)
    } released;
  };

  const uint8_t STATUS_ALLOCATE_ID = 0x00;
  const uint8_t STATUS_FREE_ID = 0x80;
  const uint8_t STATUS_SCAVENGED_ID = 0xC0;
  const uint8_t STATUS_MERGED_ID = 0x40;
  const uint8_t STATUS_ID_MASK = 0xC0;
  const uint8_t STATUS_SIZEL2_MASK = 0x3F;
  inline uint8_t STATUS_ALLOCATE(int sizeID) { return STATUS_ALLOCATE_ID | sizeID; }
  inline uint8_t STATUS_FREE(int sizeID) { return STATUS_FREE_ID | sizeID; }
  inline uint8_t STATUS_SCAVENGED(int sizeID) { return STATUS_SCAVENGED_ID | sizeID; }
  inline uint8_t STATUS_MERGED(int sizeID) { return STATUS_MERGED_ID | sizeID; }

  const int cTAG_IS_OBJECT_BIT = 0x80;
  const int cTAG_IS_NOT_INDEXED_MASK = 0xc0;
  const int cTAG_STATUS_MASK = 0x3 << 5;

  // Base object size (smallest plain object size)
  const int baseCountL2 = 4;
  const int baseCount = 1 << baseCountL2;
  const int baseSizeL2 = sat::SegmentSizeL2 - baseCountL2;
  const int baseSize = 1 << baseSizeL2;
  const uintptr_t baseOffsetMask = (1 << baseSizeL2) - 1;
  const uintptr_t basePtrMask = ~baseOffsetMask;

  // Support object size (largest plain object size)
  const int supportLengthL2 = baseCountL2 - 1;
  const int supportLength = 1 << supportLengthL2;
  const int supportSizeL2 = sat::SegmentSizeL2 - 1;
  const int supportSize = 1 << supportSizeL2;

  typedef struct tSATEntry {
    uint8_t id;
    uint8_t index;
    uint8_t __unused__[6];
    tSATEntry* next;
    uint8_t tags[baseCount];

  } *SATEntry;

  static_assert(sizeof(tSATEntry) == sat::EntrySize, "Bad tSATEntry size");

  typedef struct tObject {
    tObject* next;
    tObject* buddy;
    uint8_t status;
    uint8_t zoneID;
  } *Object;

  typedef struct tObjectEx :tObject {
    tObject* back;

    void swap(Object obj) {
      if (Object next = obj->next = this->next) {
        ObjectEx(next)->back = obj;
      }
      if (Object back = ObjectEx(obj)->back = this->back) {
        ObjectEx(back)->next = obj;
      }
    }
  } *ObjectEx;

  struct tObjectList {
    Object first;
    Object last;
    int count;

    operator int() {
      return this->count;
    }
    void append(tObjectList list) {
      assert(list.count);
      list.last->next = this->first;
      this->first = list.first;
      this->count += list.count;
    }
    int push(Object obj) {
      if (this->count++) {
        obj->next = this->first;
        this->first = obj;
      }
      else {
        obj->next = 0;
        this->first = obj;
        this->last = obj;
      }
      return this->count;
    }
    Object pop() {
      if (Object obj = this->first) {
        this->first = obj->next;
        this->count--;
        return obj;
      }
      return 0;
    }
    tObjectList pop_list(int n)
    {
      tObjectList list;
      assert(n > 0);
      list.count = (this->count < n) ? this->count : n;
      Object last = this->first;
      switch (list.count) {
      default:
        for (int i = 7; i < list.count; i++)
          last = last->next;
      case 7: last = last->next;
      case 6: last = last->next;
      case 5: last = last->next;
      case 4: last = last->next;
      case 3: last = last->next;
      case 2: last = last->next;
      case 1: {
        list.last = last;
        list.first = this->first;
        this->first = last->next;
        this->count -= list.count;
      }
      case 0: break;
      }
      return list;
    }
    tObjectList pop_overflow(int sizeID, int remain)
    {
      if (remain) {
        tObjectList list;
        list.count = this->count - remain;

        Object obj = this->first;
        this->count = remain;
        while (--remain) {
          obj = obj->next;
        }
        list.last = this->last;
        list.first = obj->next;
        this->last = obj;
        obj->next = 0;
        return list;
      }
      else {
        tObjectList list = *this;
        this->first = 0;
        this->count = 0;
        return list;
      }
    }
    tObjectList flush() {
      tObjectList list = *this;
      this->first = 0;
      this->count = 0;
      return list;
    }
    static tObjectList empty() {
      tObjectList list;
      list.first = 0;
      list.count = 0;
      return list;
    }
  };

  int getScaleFactorPageID(int scaleFactor)
  {
    switch (scaleFactor) {
    case 1: return tSATEntryID::PAGE_SCALED_BUDDY_1;
    case 3: return tSATEntryID::PAGE_SCALED_BUDDY_3;
    case 5: return tSATEntryID::PAGE_SCALED_BUDDY_5;
    case 7: return tSATEntryID::PAGE_SCALED_BUDDY_7;
    case 9: return tSATEntryID::PAGE_SCALED_BUDDY_9;
    case 11: return tSATEntryID::PAGE_SCALED_BUDDY_11;
    case 13: return tSATEntryID::PAGE_SCALED_BUDDY_13;
    case 15: return tSATEntryID::PAGE_SCALED_BUDDY_15;
    }
    return 0;
  }

  template<int sizeID, int value>
  struct WriteTags {
  };
  template<int value>
  struct WriteTags<0, value> {
    static int apply(SATEntry entry, int index) { (*(uint8_t*)&entry->tags[index &= 0xf]) = value; return index; }
  };
  template<int value>
  struct WriteTags<1, value> {
    static int apply(SATEntry entry, int index) { (*(uint16_t*)&entry->tags[index &= 0xe]) = value | (value << 8); return index; }
  };
  template<int value>
  struct WriteTags<2, value> {
    static int apply(SATEntry entry, int index) { (*(uint32_t*)&entry->tags[index &= 0xc]) = value | (value << 8) | (value << 16) | (value << 24); return index; }
  };
  template<int value>
  struct WriteTags<3, value> {
    static int apply(SATEntry entry, int index) { (*(uint64_t*)&entry->tags[index &= 0x8]) = value | (value << 8) | (value << 16) | (value << 24) | (value << 32) | (value << 40) | (value << 48) | (value << 56); return index; }
  };

}

#include "heapGlobal.h"
#include "heapLocal.h"
