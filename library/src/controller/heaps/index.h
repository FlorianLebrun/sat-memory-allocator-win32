
#include "../../base.h"

#ifndef _SAT_heaps_index_h_
#define _SAT_heaps_index_h_

#include "./AllocatorSizeMapping.h"

namespace SAT {
  struct BaseHeap;
  struct LocalHeap;
  struct GlobalHeap;

  struct BaseHeap : SATBasicRealeasable<IHeap> {
    AllocatorSizeMapping sizeMapping;
    int id;

    virtual ~BaseHeap() = 0;
    virtual int getID() override final;

    void* allocTraced(size_t size, uint64_t meta);

    void* alloc(size_t size, uint64_t meta);
    virtual size_t free(void* ptr) = 0;
    virtual void flushCache() = 0;
    virtual void terminate() = 0;
  };

  struct GlobalHeap : BaseHeap {
    GlobalHeap* backGlobal, *nextGlobal;
    char name[256];

    SpinLock locals_lock;
    LocalHeap* locals_list;
    
    GlobalHeap(const char* name);
    virtual const char* getName() override final;
    virtual void destroy() override final;

    virtual void* allocBuffer(size_t size, SAT::tObjectMetaData meta) override final;
    virtual uintptr_t acquirePages(size_t size) override;
    virtual void releasePages(uintptr_t index, size_t size) override;

    virtual LocalHeap* createLocal() = 0;
  };

  struct LocalHeap : BaseHeap {
    LocalHeap* backLocal, *nextLocal;
    GlobalHeap* global;

    LocalHeap(GlobalHeap* global);
    virtual const char* getName() override final;
    virtual void destroy() override final;

    virtual void* allocBuffer(size_t size, SAT::tObjectMetaData meta) override final;
    virtual uintptr_t acquirePages(size_t size) override final;
    virtual void releasePages(uintptr_t index, size_t size) override final;
  };
}

#endif
