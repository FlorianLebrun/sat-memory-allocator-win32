
#include "../../base.h"

#ifndef _SAT_heaps_AllocatorSizeMapping_h_
#define _SAT_heaps_AllocatorSizeMapping_h_

namespace SAT {

  struct AllocatorSizeMapping {
    static const uintptr_t sizeMax = -1;

    static const int smallSizeCount = 512;
    static const int smallSizeMax = smallSizeCount << 3;

    static const uintptr_t mediumSubL2 = 6;
    static const uintptr_t mediumSub = 1 << mediumSubL2;
    static const uintptr_t mediumBaseL2 = 12;

    uint32_t numAllocators;

    uint8_t small_objects_sizemap[smallSizeCount/*index*/]; // size map for 12bit ( size = (index<<3) + 7 )
    uint8_t medium_objects_sizemap[24/*exponent*/][64/*mantisse*/]; // size map for 12bit to 36bit ( size = mantisse<<(exponent+12) + 1<< )

    IObjectAllocator* allocators[256];

    AllocatorSizeMapping() {
      this->numAllocators = 0;
      memset(this->allocators, 0, sizeof(this->allocators));
    }
    void registerAllocator(IObjectAllocator* allocator) {
      this->allocators[this->numAllocators++] = allocator;
    }
    IObjectAllocator* getAllocator(int size) {
      if (size <= 4096) {
        int index = (size - 1) >> 3;
        int id = this->small_objects_sizemap[index];
        return this->allocators[id];
      }
      else {
        uintptr_t sizeL2 = msb_32(size);
        uintptr_t shift = sizeL2 - mediumSubL2;
        uintptr_t index0 = sizeL2 - mediumBaseL2;
        uintptr_t index1 = (size >> shift) & (mediumSub - 1);
        int id = this->medium_objects_sizemap[index0][index1];
        return this->allocators[id];
      }
    }
    static int _compare_allocator(const void* a, const void* b) {
      return (*(IObjectAllocator**)a)->allocatedSize() - (*(IObjectAllocator**)b)->allocatedSize();
    }
    void check() {
      uintptr_t size, allocSize;
      uintptr_t sizeLimit = this->allocators[this->numAllocators - 1]->allocatedSize();
      for (size = 0; size <= sizeLimit; size++) {
        if (IObjectAllocator* alloc = getAllocator(size)) {
          allocSize = alloc->allocatedSize();
          if (size <= allocSize) continue;
        }
        printf("allocator mapping invalid");
        getchar();
      }
    }
    void finalize() {
      qsort(this->allocators, this->numAllocators, sizeof(IObjectAllocator*), &this->_compare_allocator);

      uintptr_t curAllocId = 0;
      uintptr_t curAllocSize = this->allocators[0]->allocatedSize();

      for (int size = 1; size <= 4096; size++) {
        // size -> index
        int index = (size - 1) >> 3;

        // index -> size
        uintptr_t min_size = (index << 3) + 1;
        uintptr_t max_size = min_size + 7;

        if (!(size >= min_size && size <= max_size)) {
          printf("allocator mapping invalid");
          getchar();
        }
      }
      for (int size = 4097; size < (4096 << 8); size++) {
        // size -> index
        uintptr_t csize = size;
        uintptr_t sizeL2 = msb_32(csize);
        uintptr_t shift = sizeL2 - mediumSubL2;
        uintptr_t index0 = sizeL2 - mediumBaseL2;
        uintptr_t index1 = (csize >> shift) & (mediumSub - 1);

        // index -> size
        uintptr_t _shift = index0 + mediumSubL2;
        uintptr_t min_size = (index1 | mediumSub) << _shift;
        uintptr_t max_size = min_size | ((1 << _shift) - 1);

        assert(_shift == shift);
        if (!(size >= min_size && size <= max_size)) {
          printf("allocator mapping invalid");
          getchar();
        }
      }

      // Fill the small mapping
      for (int index = 0; index < smallSizeCount; index++) {
        // Compute size limit for the current mapping slot
        uintptr_t min_size = (index << 3) + 1;
        uintptr_t max_size = min_size + 7;

        // Find the first matching allocator
        while (max_size > curAllocSize && curAllocId < this->numAllocators) {
          if (IObjectAllocator* alloc = this->allocators[++curAllocId]) curAllocSize = alloc->allocatedSize();
          else curAllocSize = 0;
        }

        // Set the map
        this->small_objects_sizemap[index] = curAllocId;
      }

      // Fill the medium sizes
      for (int index0 = 0; index0 < 24; index0++) {
        for (int index1 = 0; index1 < 64; index1++) {
          // Compute size limit for the current mapping slot
          uintptr_t _shift = index0 + mediumSubL2;
          uintptr_t min_size = (index1 | mediumSub) << _shift;
          uintptr_t max_size = min_size | ((1 << _shift) - 1);

          // Find the first matching allocator
          while (max_size > curAllocSize && curAllocId < this->numAllocators) {
            if (IObjectAllocator* alloc = this->allocators[++curAllocId]) curAllocSize = alloc->allocatedSize();
            else curAllocSize = 0;
          }

          // Set the map
          this->medium_objects_sizemap[index0][index1] = curAllocId;
        }
      }

      //this->check();
    }
  };
}

#endif