
#ifndef _SAT_base_h_
#define _SAT_base_h_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include <sat-memory-allocator/sat-memory-allocator.h>
#include "./utils/index.h"
#include "./win32/system.h"
#include "./SATable.h"

#ifdef _DEBUG
#define SAT_DEBUG_CHECK(x)// x
#endif

extern"C" SAT::tEntry* g_SATable;

template <class Interface>
class SATBasicRealeasable : public Interface {
public:
  std::atomic_uint32_t numRefs;
  SATBasicRealeasable() {
    this->numRefs = 1;
  }
  virtual void retain() override final {
    assert(this->numRefs >= 0);
    this->numRefs++;
  }
  virtual void release() override final {
    assert(this->numRefs >= 0);
    if (!(--this->numRefs)) this->destroy();
  }
  virtual void destroy() = 0;
};

struct IObjectAllocator {
  typedef void* (IObjectAllocator::*f_malloc)(uint64_t meta);

  virtual void* allocObject(size_t size, uint64_t meta) = 0;
  virtual size_t allocatedSize() = 0;
};

#include "./controller/index.h"

namespace SAT {
  void TypesDataBase_init();
  extern __declspec(thread) Thread* thread_instance;
  extern __declspec(thread) LocalHeap* thread_local_heap;
  extern __declspec(thread) GlobalHeap* thread_global_heap;
}

#endif
