#pragma once
#include <sat-memory/memory.hpp>
#include <sat-threads/thread.hpp>
#include "./objects_types.h"
#include "./objects_infos.h"

namespace sat {

   enum class tHeapType {
      COMPACT_HEAP_TYPE,
   };

   struct IObjectVisitor {
      virtual bool visit(tpObjectInfos obj) = 0;
   };

   struct IObjectAllocator {

      virtual size_t getMaxAllocatedSize() = 0;
      virtual size_t getMinAllocatedSize() = 0;
      virtual size_t getAllocatedSize(size_t size) = 0;
      virtual void* allocate(size_t size) = 0;

      virtual size_t getMaxAllocatedSizeWithMeta() = 0;
      virtual size_t getMinAllocatedSizeWithMeta() = 0;
      virtual size_t getAllocatedSizeWithMeta(size_t size) = 0;
      virtual void* allocateWithMeta(size_t size, uint64_t meta) = 0;
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

   typedef void* (*tp_malloc)(size_t size);
   typedef void* (*tp_realloc)(void* ptr, size_t size);
   typedef size_t(*tp_msize)(void* ptr);
   typedef void (*tp_free)(void*);

}

extern"C" SAT_API void sat_init_process();
extern"C" SAT_API void sat_patch_default_allocator();
extern"C" SAT_API void sat_attach_current_thread();
extern"C" SAT_API void sat_dettach_current_thread();
extern"C" SAT_API void sat_terminate_process();

// Allocation API
extern"C" SAT_API void* sat_malloc(size_t size);
extern"C" SAT_API void* sat_malloc_ex(size_t size, uint64_t meta);
extern"C" SAT_API void* sat_calloc(size_t count, size_t size);
extern"C" SAT_API void* sat_realloc(void* ptr, size_t size, sat::tp_realloc default_realloc = 0);
extern"C" SAT_API size_t sat_msize(void* ptr, sat::tp_msize default_msize = 0);
extern"C" SAT_API void sat_free(void* ptr, sat::tp_free default_free = 0);
extern"C" SAT_API void sat_flush_cache();

// Reflexion API
extern"C" SAT_API sat::IController* sat_get_contoller();
extern"C" SAT_API sat::memory::MemoryTable* sat_get_table();
extern"C" SAT_API sat::Thread* sat_current_thread();

extern"C" SAT_API bool sat_has_address(void* ptr);
extern"C" SAT_API bool sat_get_address_infos(void* ptr, sat::tpObjectInfos infos = 0);
