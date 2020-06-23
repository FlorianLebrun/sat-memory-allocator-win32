
#ifndef _sat_memory_allocator_win32_h_
#define _sat_memory_allocator_win32_h_

#include <atomic>
#include <string>
#include <vector>
#include <stdint.h>
#include <assert.h>

namespace SAT {
  struct IThread;
  struct IHeap;
  struct IController;

  typedef struct tEntry *tpEntry;
  typedef struct tObjectInfos *tpObjectInfos;
  typedef struct IMark *Mark;

  const uintptr_t cSegmentSizeL2 = 16;
  const uintptr_t cSegmentSize = 1 << cSegmentSizeL2;
  const uintptr_t cSegmentOffsetMask = (1 << cSegmentSizeL2) - 1;
  const uintptr_t cSegmentPtrMask = ~cSegmentOffsetMask;
  const uintptr_t cSegmentMinIndex = 32;
  const uintptr_t cSegmentMinAddress = cSegmentMinIndex << cSegmentSizeL2;

  const uintptr_t cEntrySizeL2 = 5;
  const uintptr_t cEntrySize = 1 << cEntrySizeL2;

  typedef void* (*tp_malloc)(size_t size);
  typedef void* (*tp_realloc)(void* ptr, size_t size);
  typedef size_t (*tp_msize)(void* ptr);
  typedef void (*tp_free)(void*);

  struct IReleasable {
    virtual void retain() = 0;
    virtual void release() = 0;
  };

}

#include "./types.h"
#include "./stack-tracing.h"
#include "./sat.h"

extern"C" void _satmalloc_init();

// Allocation API
extern"C" void* sat_malloc(size_t size);
extern"C" void* sat_malloc_ex(size_t size, uint64_t meta);
extern"C" void* sat_calloc(size_t count, size_t size);
extern"C" void* sat_realloc(void* ptr, size_t size, SAT::tp_realloc default_realloc = 0);
extern"C" size_t sat_msize(void* ptr, SAT::tp_msize default_msize = 0);
extern"C" void sat_free(void* ptr, SAT::tp_free default_free = 0);
extern"C" void sat_flush_cache();

// Stack marking API
extern"C" void sat_begin_stack_beacon(SAT::StackBeacon* beacon);
extern"C" void sat_end_stack_beacon(SAT::StackBeacon* beacon);

// Reflexion API
extern"C" SAT::IController* sat_get_contoller();
extern"C" SAT::tEntry* sat_get_table();
extern"C" SAT::IThread* sat_current_thread();

extern"C" bool sat_has_address(void* ptr);
extern"C" bool sat_get_address_infos(void* ptr, SAT::tpObjectInfos infos = 0);

#endif
