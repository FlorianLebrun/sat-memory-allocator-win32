#pragma once
#include <sat-memory-allocator/sat-memory-allocator.h>
#include <sat-threads/spinlock.hpp>
#include "./AllocatorSizeMapping.h"

#ifdef _DEBUG
#define SAT_DEBUG_CHECK(x)// x
#endif

namespace sat {
   struct BaseHeap;
   struct LocalHeap;
   struct GlobalHeap;

   struct BaseHeap : shared::SharedObject<IHeap> {
      AllocatorSizeMapping<false> sizeMapping;
      AllocatorSizeMapping<true> sizeMappingWithMeta;
      int heapID;

      virtual ~BaseHeap() = 0;
      virtual int getID() override final;

      virtual size_t getMaxAllocatedSize() override;
      virtual size_t getMinAllocatedSize() override;
      virtual size_t getAllocatedSize(size_t size) override;
      virtual void* allocate(size_t size) override final;

      virtual size_t getMaxAllocatedSizeWithMeta() override;
      virtual size_t getMinAllocatedSizeWithMeta() override;
      virtual size_t getAllocatedSizeWithMeta(size_t size) override;
      virtual void* allocateWithMeta(size_t size, uint64_t meta) override final;

      virtual size_t free(void* ptr) = 0;
      virtual void flushCache() = 0;
   };

   struct GlobalHeap : BaseHeap {
      static const Thread::ThreadObjectID ThreadObjectID = Thread::GLOBAL_HEAP;
      GlobalHeap* backGlobal, * nextGlobal;
      char name[256];

      SpinLock locals_lock;
      LocalHeap* locals_list;

      GlobalHeap(const char* name, int heapID);
      virtual const char* getName() override final;
      virtual ~GlobalHeap() override;

      virtual uintptr_t acquirePages(size_t size) override;
      virtual void releasePages(uintptr_t index, size_t size) override;

      virtual LocalHeap* createLocal() = 0;
   };

   struct LocalHeap : BaseHeap {
      static const Thread::ThreadObjectID ThreadObjectID = Thread::LOCAL_HEAP;
      LocalHeap* backLocal, * nextLocal;
      GlobalHeap* global;

      LocalHeap(GlobalHeap* global);
      virtual const char* getName() override final;
      virtual ~LocalHeap() override;

      virtual uintptr_t acquirePages(size_t size) override final;
      virtual void releasePages(uintptr_t index, size_t size) override final;
   };
}

