#pragma once
#include "../../heaps/heaps.h"
#include "../../utils/binary-arithmetic.h"

namespace LargeObjectAllocator {
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
