#ifndef SAT_LargeObjectAllocator_h_
#define SAT_LargeObjectAllocator_h_

#include "../base.h"

namespace LargeObjectAllocator {
  struct Global : public IObjectAllocator {
    int heapID;

    void init(SAT::IHeap* pageHeap);
    virtual void* allocObject(size_t size, uint64_t meta) override;
    virtual size_t allocatedSize() override;
    size_t freePtr(uintptr_t index);
  };

  bool get_address_infos(uintptr_t ptr, SAT::tpObjectInfos infos);
}

#endif
