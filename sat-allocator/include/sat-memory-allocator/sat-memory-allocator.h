#pragma once
#include <sat-memory/memory.hpp>
#include <sat-threads/thread.hpp>
#include "./_base.h"
#include "./_types.h"
#include "./_objects.h"

namespace sat {

   enum class tHeapType {
      COMPACT_HEAP_TYPE,
   };

   struct IHeap : shared::ISharedObject, IObjectAllocator {

      // Basic infos
      virtual int getID() = 0;
      virtual const char* getName() = 0;

      // Allocation
      virtual uintptr_t acquirePages(size_t size) = 0;
      virtual void releasePages(uintptr_t index, size_t size) = 0;
   };

   struct IController {

      // Heap management
      virtual IHeap* createHeap(tHeapType type, const char* name = 0) = 0;
      virtual IHeap* getHeap(int id) = 0;

      // Analysis
      virtual void traverseObjects(IObjectVisitor* visitor, uintptr_t start_address = 0) = 0;
      virtual bool checkObjectsOverflow() = 0;
   };

}

extern"C" void sat_init_process(bool patchlibs);
extern"C" void sat_attach_current_thread();
extern"C" void sat_dettach_current_thread();
extern"C" void sat_terminate_process();

// Allocation API
extern"C" void* sat_malloc(size_t size);
extern"C" void* sat_malloc_ex(size_t size, uint64_t meta);
extern"C" void* sat_calloc(size_t count, size_t size);
extern"C" void* sat_realloc(void* ptr, size_t size, sat::tp_realloc default_realloc = 0);
extern"C" size_t sat_msize(void* ptr, sat::tp_msize default_msize = 0);
extern"C" void sat_free(void* ptr, sat::tp_free default_free = 0);
extern"C" void sat_flush_cache();

// Reflexion API
extern"C" sat::IController* sat_get_contoller();
extern"C" sat::memory::MemoryTable* sat_get_table();
extern"C" sat::Thread* sat_current_thread();

extern"C" bool sat_has_address(void* ptr);
extern"C" bool sat_get_address_infos(void* ptr, sat::tpObjectInfos infos = 0);
