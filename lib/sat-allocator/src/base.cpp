#include "./allocators/ZonedBuddy/index.h"
#include "./allocators/LargeObject/index.h"
#include "./controller.h"
#include <sat/thread.hpp>
#include <iostream>

static sat::tp_malloc _sat_default_malloc = 0;
static sat::tp_realloc _sat_default_realloc = 0;
static sat::tp_msize _sat_default_msize = 0;
static sat::tp_free _sat_default_free = 0;
static bool is_initialized = false;
static bool is_patched = false;

struct CurrentLocalHeap {
private:
   static __declspec(thread) sat::LocalHeap* __current_local_heap;
   static sat::LocalHeap* _fetch() {
      _ASSERT(is_initialized);
      if (!__current_local_heap) {
         auto thread = sat::Thread::current();
         __current_local_heap = thread->getObject<sat::LocalHeap>();
      }
      return __current_local_heap;
   }
   static sat::LocalHeap* _install() {
      _ASSERT(is_initialized);
      if (!__current_local_heap) {
         if (auto thread = sat::Thread::current()) {
            __current_local_heap = thread->getObject<sat::LocalHeap>();
            if (!__current_local_heap) {
               auto global_heap = thread->getObject<sat::GlobalHeap>();
               if (!global_heap) {
                  global_heap = g_SAT.heaps_table[0];
                  global_heap->retain();
                  thread->setObject<sat::GlobalHeap>(global_heap);
               }
               __current_local_heap = global_heap->createLocal();
            }
         }
      }
      return __current_local_heap;
   }
public:
   __forceinline static sat::LocalHeap* check() {
      if (!__current_local_heap) return _fetch();
      return __current_local_heap;
   }
   __forceinline static sat::LocalHeap* get() {
      if (!__current_local_heap) return _install();
      else return __current_local_heap;
   }
};

__declspec(thread) sat::LocalHeap* CurrentLocalHeap::__current_local_heap = nullptr;

void _sat_set_default_allocator(
   sat::tp_malloc malloc,
   sat::tp_realloc realloc,
   sat::tp_msize msize,
   sat::tp_free free)
{
   _sat_default_malloc = malloc;
   _sat_default_realloc = realloc;
   _sat_default_msize = msize;
   _sat_default_free = free;
}

void sat_init_process() {
   if (!is_initialized) {
      is_initialized = true;
      sat::memory::MemoryTable::initialize();
      g_SAT.initialize();
   }
   else printf("sat library is initalized more than once.");
}

void sat_patch_default_allocator() {
   extern void PatchWindowsFunctions();
   if (!is_initialized) {
      printf("Critical: Cannot patch before sat_init_process()");
      exit(1);
   }
   else if (!is_patched) {
      is_patched = true;
      PatchWindowsFunctions();
   }
   else printf("default allocator is patched more than once.");
}

void sat_terminate_process() {
   // COULD BE USED
}

void sat_attach_current_thread() {
}

void sat_dettach_current_thread() {
   if (sat::Thread* thread = sat::Thread::current()) {
      std::cout << "thread:" << std::this_thread::get_id() << " is dettached.\n";
      //sat::current_thread = 0;
      thread->release();
   }
}

void sat_flush_cache() {
   if (auto thread = sat::Thread::current()) {
      if (auto local_heap = thread->getObject<sat::LocalHeap>()) {
         local_heap->flushCache();
      }
      if (auto global_heap = thread->getObject<sat::GlobalHeap>()) {
         global_heap->flushCache();
      }
   }
}

void* sat_malloc_ex(size_t size, uint64_t meta) {
   if (auto local_heap = CurrentLocalHeap::get()) {
      void* ptr = local_heap->allocateWithMeta(size, meta);
#ifdef _DEBUG
      memset(ptr, 0xdd, size);
#endif
      return ptr;
   }
   return 0;
}

void* sat_malloc(size_t size) {
   if (auto local_heap = CurrentLocalHeap::get()) {
      void* ptr = local_heap->allocate(size);
#ifdef _DEBUG
      memset(ptr, 0xdd, size);
#endif
      return ptr;
   }
   return 0;
}

void* sat_calloc(size_t count, size_t size) {
   return malloc(count * size);
}

bool sat_get_metadata(void* ptr, sat::tObjectMetaData& meta) {
   return false;
}

size_t sat_msize(void* ptr, sat::tp_msize default_msize) {
   sat::tObjectInfos infos;
   if (sat_get_address_infos(ptr, &infos)) {
      return infos.size;
   }
   else {
      if (default_msize) return default_msize(ptr);
      else if (_sat_default_msize) return _sat_default_msize(ptr);
      else printf("sat cannot find size of unkown buffer\n");
   }
   return 0;
}

void* sat_realloc(void* ptr, size_t size, sat::tp_realloc default_realloc) {
   sat::tObjectInfos infos;
   if (sat_get_address_infos(ptr, &infos)) {
      if (size == 0) {
         sat_free(ptr);
         return 0;
      }
      else if (size > infos.size) {
         void* new_ptr = sat_malloc(size);
         memcpy(new_ptr, ptr, infos.size);
         sat_free(ptr);
         return new_ptr;
      }
      else {
         return ptr;
      }
   }
   else {
      if (default_realloc) return default_realloc(ptr, size);
      else if (_sat_default_realloc) return _sat_default_realloc(ptr, size);
      else {
         void* new_ptr = sat_malloc(size);
         __try { memcpy(new_ptr, ptr, size); }
         __except (1) {}
         printf("sat cannot realloc unkown buffer\n");
         return new_ptr;
      }
   }
}

void sat_free(void* ptr, sat::tp_free default_free) {
   uintptr_t index = uintptr_t(ptr) >> sat::memory::cSegmentSizeL2;
   int size;
   if (index < sat::memory::table->descriptor.length) {
      auto entry = sat::memory::table->get<sat::memory::tHeapSegmentEntry>(index);
      if (entry->isHeapSegment()) {
         auto local_heap = CurrentLocalHeap::check();
         if (local_heap && entry->heapID == local_heap->heapID) {
            size = local_heap->free(ptr);
         }
         else {
            size = g_SAT.heaps_table[entry->heapID]->free(ptr);
         }
         assert(size > 0);
         return;
      }
      else if (entry->id == sat::memory::tEntryID::FORBIDDEN || entry->id == sat::memory::tEntryID::FREE) {
         if (default_free) default_free(ptr);
         else if (_sat_default_free) _sat_default_free(ptr);
         else printf("sat cannot free unkown buffer\n");
      }
      else printf("sat_free: out of range pointer %.12llX\n", int64_t(ptr));
   }
   else printf("sat_free: out of range pointer %.12llX\n", int64_t(ptr));
}

bool sat_has_address(void* ptr) {
   uintptr_t index = uintptr_t(ptr) >> sat::memory::cSegmentSizeL2;
   if (index < sat::memory::table->descriptor.length) {
      auto entry = &sat::memory::table->entries[index];
      return (entry->id != sat::memory::tEntryID::FORBIDDEN && entry->id != sat::memory::tEntryID::FREE);
   }
   return false;
}

bool sat_get_address_infos(void* ptr, sat::tpObjectInfos infos) {
   uintptr_t index = uintptr_t(ptr) >> sat::memory::cSegmentSizeL2;
   if (index < sat::memory::table->descriptor.length) {
      auto entry = &sat::memory::table->entries[index];
      switch (entry->id) {
      case sat::tHeapEntryID::PAGE_ZONED_BUDDY:
         return ZonedBuddyAllocator::get_address_infos(uintptr_t(ptr), infos);
      case sat::tHeapEntryID::LARGE_OBJECT:
         return LargeObjectAllocator::get_address_infos(uintptr_t(ptr), infos);
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_3:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_5:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_7:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_9:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_11:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_13:
      case sat::tHeapEntryID::PAGE_SCALED_BUDDY_15:
         if (infos) infos->set(entry->getHeapID(), uintptr_t(ptr), 0);
         return true;
      }
   }
   return false;
}

sat::memory::MemoryTable* sat_get_table() {
   return sat::memory::table;
}

sat::IController* sat_get_contoller() {
   return &g_SAT;
}

sat::Thread* sat_current_thread() {
   return sat::Thread::current();
}
