#include "./index.h"

using namespace ZonedBuddyAllocator;

bool SizeMapping::init_small_objects_sizemap = false;
uint8_t SizeMapping::small_objects_sizemap[2048 >> 3];

static int getSizeOf(int baseID, int dividor) {
  return (((supportSize >> baseID) - sizeof(tZone)) / dividor)&(-8);
}

static int getSizeIDOf(int baseID, int dividor) {
  return (baseID << 4) | dividor;
}
static int getNextSize(int& baseID, int& dividor) {
  int prevSize = getSizeOf(baseID, dividor);
  if (dividor > 8) {
    dividor--;
  }
  else if (baseID > 0) {
    baseID--;
    dividor = 15;
  }
  else {
    throw "not zonable";
  }
  int nextSize = getSizeOf(baseID, dividor);
  assert(prevSize <= nextSize);
  return nextSize;
}

void SizeMapping::init() {
  if (!init_small_objects_sizemap) {
    int size = 8;
    int c_baseID = 6, c_dividor = 15;
    int c_size = getSizeOf(c_baseID, c_dividor);

    // Map zoned object size
    while (c_baseID >= 1 && size <= 2048) {
      int index = (size - 1) >> 3;
      //printf("%d -> %d\n", size, c_size);
      SizeMapping::small_objects_sizemap[index] = getSizeIDOf(c_baseID, c_dividor);
      size += 8;
      while (size > c_size) c_size = getNextSize(c_baseID, c_dividor);
    }

    // Map medium objects size
    while (size <= 2048) {
      int index = (size - 1) >> 3;
      SizeMapping::small_objects_sizemap[index] = getMediumSizeID(size);
      size += 8;
    }
    init_small_objects_sizemap = true;
  }
}

namespace ZonedBuddyAllocator {
  bool get_address_infos(uintptr_t ptr, SAT::tpObjectInfos infos) {
    tSATEntry* entry = g_SATable->get<tSATEntry>(ptr >> SAT::cSegmentSizeL2);
    uintptr_t offset = ptr&SAT::cSegmentOffsetMask;
    uintptr_t tagId = offset >> ZonedBuddyAllocator::baseSizeL2;
    int tag = entry->tags[tagId];
    if (tag & cTAG_ALLOCATED_BIT) {

      // Find object
      uintptr_t sizeID = tag&cTAG_SIZEID_MASK;
      uintptr_t sizeL2 = supportSizeL2 - sizeID;
      uint64_t meta = 0;
      Object obj = Object(ptr & -(1 << sizeL2));

      // Get infos
      if (obj->zoneID) {
        Zone zone = Zone(obj);
        int subObjectSize = (((supportSize >> sizeID) - sizeof(tZone)) / zone->zoneID)&(-8);

        uintptr_t offset = ptr - uintptr_t(&zone[1]);
        uintptr_t index = offset / subObjectSize;
        uintptr_t base = uintptr_t((index*subObjectSize) + uintptr_t(&zone[1]));
        uint8_t tag = zone->tags[index];
        if (tag & cZONETAG_ALLOCATED_BIT) {
          if (tag&cZONETAG_METADATA_BIT) {
            meta = ((uint64_t*)base)[0];
            base += sizeof(uint64_t);
          }
          if (infos) infos->set(entry->heapID, base, subObjectSize, meta);
          return true;
        }
      }
      else {
        uintptr_t base = uintptr_t(obj) + tObject::headerSize;
        if (obj->tag&cTAG_METADATA_BIT) {
          meta = ((uint64_t*)base)[0];
          base += sizeof(uint64_t);
        }
        if (infos) infos->set(entry->heapID, base, 1 << sizeL2, meta);
        return true;
      }
    }
  }
}

namespace ZonedBuddyAllocator {
  void traverseZonedObjects(tSATEntry* entry, Zone zone, int zoneSize, bool& visitMore, SAT::IObjectVisitor* visitor) {
    uintptr_t ptr = uintptr_t(&zone[1]);
    int dividor = zone->zoneID;
    int size = ((zoneSize - sizeof(tZone)) / dividor)&(-8);
    for (int i = 0; i < dividor && visitMore; i++) {
      uint8_t tag = zone->tags[i];
      if (tag & cZONETAG_ALLOCATED_BIT) {
        uint64_t meta = 0;
        uint64_t* base = ((uint64_t*)ptr);
        if (tag&cZONETAG_METADATA_BIT) meta = (base++)[0];
        visitMore = visitor->visit(&SAT::tObjectInfos().set(entry->heapID, uintptr_t(base), size, meta));
      }
      ptr += size;
    }
  }
  void traverseSubObjects(tSATEntry* entry, uintptr_t ptr, int count, int size, bool& visitMore, SAT::IObjectVisitor* visitor) {
    for (int i = 0; i < count && visitMore; i++) {
      Object obj = Object(ptr);
      if (!(obj->status&cSTATUS_FREE_ID)) {
        if (obj->zoneID & 0xf) {
          traverseZonedObjects(entry, Zone(obj), size, visitMore, visitor);
        }
        else {
          uint64_t meta = 0;
          int offset = tObject::headerSize;
          if (obj->tag&cTAG_METADATA_BIT) {
            meta = obj->content()[0];
            offset += sizeof(uint64_t);
          }
          visitMore = visitor->visit(&SAT::tObjectInfos().set(entry->heapID, uintptr_t(obj) + offset, size - offset, meta));
        }
      }
      ptr += size;
    }
  }
  uintptr_t traversePageObjects(uintptr_t index, bool& visitMore, SAT::IObjectVisitor* visitor) {
    tSATEntry* entry = (tSATEntry*)&g_SATable[index];
    uintptr_t base = index << SAT::cSegmentSizeL2;
    for (int i = 0; i < 16 && visitMore;) {
      int tag = entry->tags[i];
      if (tag & cTAG_ALLOCATED_BIT) {
        int sizeID = tag&cTAG_SIZEID_MASK;
        int size = supportSize >> sizeID;
        uintptr_t ptr = base + (i << baseSizeL2);
        switch (sizeID) {
        case 0:
          traverseSubObjects(entry, ptr, 1, size, visitMore, visitor);
          i += 8;
          break;
        case 1:
          traverseSubObjects(entry, ptr, 1, size, visitMore, visitor);
          i += 4;
          break;
        case 2:
          traverseSubObjects(entry, ptr, 1, size, visitMore, visitor);
          i += 2;
          break;
        case 3:
          traverseSubObjects(entry, ptr, 1, size, visitMore, visitor);
          i++;
          break;
        case 4:
          traverseSubObjects(entry, ptr, 2, size, visitMore, visitor);
          i++;
          break;
        case 5:
          traverseSubObjects(entry, ptr, 4, size, visitMore, visitor);
          i++;
          break;
        case 6:
          traverseSubObjects(entry, ptr, 8, size, visitMore, visitor);
          i++;
          break;
        case 7:
          traverseSubObjects(entry, ptr, 16, size, visitMore, visitor);
          i++;
          break;
        }
      }
      else i++;
    }
    return 1;
  }
}
