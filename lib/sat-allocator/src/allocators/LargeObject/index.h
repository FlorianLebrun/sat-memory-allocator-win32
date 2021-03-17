#pragma once
#include "../../heaps/heaps.h"

namespace LargeObjectAllocator {

   struct tHeapLargeObjectEntry {
      uint8_t id; // = LARGE_OBJECT
      uint8_t heapID;
      uint32_t index;
      uint32_t length;
      uint64_t meta;
      void set(uint8_t heapID, uint32_t index, uint32_t length, uint64_t meta) {
         this->id = sat::tHeapEntryID::LARGE_OBJECT;
         this->heapID = heapID;
         this->index = index;
         this->length = length;
         this->meta = meta;
      }
   };

   struct Global : public sat::IObjectAllocator {
      int heapID;

      void init(sat::IHeap* pageHeap);

      virtual size_t getMaxAllocatedSize() override;
      virtual size_t getMinAllocatedSize() override;
      virtual size_t getAllocatedSize(size_t size) override;
      virtual void* allocate(size_t size) override;

      virtual size_t getMaxAllocatedSizeWithMeta() override;
      virtual size_t getMinAllocatedSizeWithMeta() override;
      virtual size_t getAllocatedSizeWithMeta(size_t size) override;
      virtual void* allocateWithMeta(size_t size, uint64_t meta) override;

      size_t freePtr(uintptr_t index);
   };

   bool get_address_infos(uintptr_t ptr, sat::tpObjectInfos infos);
}
