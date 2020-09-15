#include "./base.h"
#include "./allocators/ZonedBuddy/index.h"
#include "./allocators/LargeObject/index.h"

__declspec(thread) SAT::Thread* SAT::current_thread = 0;

static SAT::tp_malloc _sat_default_malloc = 0;
static SAT::tp_realloc _sat_default_realloc = 0;
static SAT::tp_msize _sat_default_msize = 0;
static SAT::tp_free _sat_default_free = 0;

void _sat_set_default_allocator(
  SAT::tp_malloc malloc, 
  SAT::tp_realloc realloc, 
  SAT::tp_msize msize, 
  SAT::tp_free free) 
{
  _sat_default_malloc = malloc;
  _sat_default_realloc = realloc;
  _sat_default_msize = msize;
  _sat_default_free = free;
}

void _satmalloc_init() {
  g_SAT.initialize();
}

void sat_flush_cache() {
  if (SAT::current_thread) {
    SAT::current_thread->local_heap->flushCache();
    SAT::current_thread->global_heap->flushCache();
  }
}

void* sat_malloc_ex(size_t size, uint64_t meta) {
  SAT::Thread* thread = SAT::current_thread;
  if (!thread) thread = (SAT::Thread*)g_SAT.getCurrentThread();
  void* ptr = thread->local_heap->allocateWithMeta(size, meta);
#ifdef _DEBUG
  memset(ptr, 0xdd, size);
#endif
  return ptr;
}

void* sat_malloc(size_t size) {
  SAT::Thread* thread = SAT::current_thread;
  if (!thread) thread = (SAT::Thread*)g_SAT.getCurrentThread();
  void* ptr = thread->local_heap->allocate(size);
#ifdef _DEBUG
  memset(ptr, 0xdd, size);
#endif
  return ptr;
}

void* sat_calloc(size_t count, size_t size) {
  return malloc(count*size);
}

bool sat_get_metadata(void* ptr, SAT::tObjectMetaData& meta) {
  return false;
}

size_t sat_msize(void* ptr, SAT::tp_msize default_msize) {
  SAT::tObjectInfos infos;
  if(sat_get_address_infos(ptr, &infos)) {
    return infos.size;
  }
  else {
    if(default_msize) return default_msize(ptr);
    else if(_sat_default_msize) return _sat_default_msize(ptr);
    else {
      printf("SAT cannot find size of unkown buffer\n");
    }
  }
}

void* sat_realloc(void* ptr, size_t size, SAT::tp_realloc default_realloc) {
  SAT::tObjectInfos infos;
  if(sat_get_address_infos(ptr, &infos)) {
    if(size == 0) {
      sat_free(ptr);
      return 0;
    }
    else if(size > infos.size) {
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
    if(default_realloc) return default_realloc(ptr, size);
    else if(_sat_default_realloc) return _sat_default_realloc(ptr, size);
    else {
      void* new_ptr = sat_malloc(size);
      __try {memcpy(new_ptr, ptr, size);}
      __except(1) {}
      printf("SAT cannot realloc unkown buffer\n");
      return new_ptr;
    }
  }
}

void sat_free(void* ptr, SAT::tp_free default_free) {
  uintptr_t index = uintptr_t(ptr) >> SAT::cSegmentSizeL2;
  int size;
  if (index < g_SATable->SATDescriptor.length) {
    SAT::tpHeapSegmentEntry entry = SAT::tpHeapSegmentEntry(&g_SATable[index]);
    if(SAT::tEntryID::isHeapSegment(entry->id)) {
      if (SAT::current_thread && entry->heapID == SAT::current_thread->heapId) {
        size = SAT::current_thread->local_heap->free(ptr);
      }
      else {
        size = g_SAT.heaps_table[entry->heapID]->free(ptr);
      }
      assert(size>0);
      return;
    }
    else if(entry->id == SAT::tEntryID::FORBIDDEN || entry->id == SAT::tEntryID::FREE) {
      if(default_free) default_free(ptr);
      else if(_sat_default_free) _sat_default_free(ptr);
      else {
        printf("SAT cannot free unkown buffer\n");
      }
    }
    else printf("sat_free: out of range pointer %.8X\n", uintptr_t(ptr));
  }
  else printf("sat_free: out of range pointer %.8X\n", uintptr_t(ptr));
}

bool sat_has_address(void* ptr) {
  uintptr_t index = uintptr_t(ptr) >> SAT::cSegmentSizeL2;
  if (index < g_SATable->SATDescriptor.length) {
    SAT::tEntry* entry = &g_SATable[index];
    return (entry->id != SAT::tEntryID::FORBIDDEN && entry->id != SAT::tEntryID::FREE);
  }
  return false;
}

bool sat_get_address_infos(void* ptr, SAT::tpObjectInfos infos) {
  uintptr_t index = uintptr_t(ptr) >> SAT::cSegmentSizeL2;
  if (index < g_SATable->SATDescriptor.length) {
    SAT::tEntry* entry = &g_SATable[index];
    switch (entry->id) {
    case SAT::tEntryID::PAGE_ZONED_BUDDY:
      return ZonedBuddyAllocator::get_address_infos(uintptr_t(ptr), infos);
    case SAT::tEntryID::LARGE_OBJECT:
      return LargeObjectAllocator::get_address_infos(uintptr_t(ptr), infos);
    case SAT::tEntryID::PAGE_SCALED_BUDDY_3:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_5:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_7:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_9:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_11:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_13:
    case SAT::tEntryID::PAGE_SCALED_BUDDY_15:
      if (infos) {
        infos->set(entry->getHeapID(), uintptr_t(ptr), 0);
      }
      return true;
    }
  }
  return false;
}

SAT::tEntry* sat_get_table() {
  return g_SATable;
}

SAT::IController* sat_get_contoller() {
  return &g_SAT;
}

SAT::IThread* sat_current_thread() {
  return g_SAT.getCurrentThread();
}

void sat_begin_stack_beacon(SAT::StackBeacon* beacon) {
  SAT::Thread* thread = SAT::current_thread;
  if (!thread) thread = (SAT::Thread*)g_SAT.getCurrentThread();
  if(thread->stackBeaconsCount < SAT::cThreadMaxStackBeacons) {
    if(thread->stackBeaconsCount == 0) {
      beacon->parentBeacon = 0;
      thread->stackBeacons[thread->stackBeaconsCount++] = beacon;
    }
    else {
      beacon->parentBeacon = thread->stackBeacons[thread->stackBeaconsCount-1];
      thread->stackBeacons[thread->stackBeaconsCount++] = beacon;
    }
  }
}

void sat_end_stack_beacon(SAT::StackBeacon* beacon) {
  SAT::Thread* thread = SAT::current_thread;
  if (!thread) thread = (SAT::Thread*)g_SAT.getCurrentThread();
  while(thread->stackBeaconsCount && uintptr_t(thread->stackBeacons[thread->stackBeaconsCount-1])<uintptr_t(beacon)) {
    thread->stackBeaconsCount--;
    printf("Error in SAT stack beaon: a previous beacon has been not closed.\n");
  }
  if(thread->stackBeaconsCount && thread->stackBeacons[thread->stackBeaconsCount-1] == beacon) {
    thread->stackBeaconsCount--;
    beacon->parentBeacon = 0;
  }
  else if(thread->stackBeaconsCount != SAT::cThreadMaxStackBeacons) {
    printf("Error in SAT stack beaon: a beacon is not retrieved and cannot be properly closed.\n");
  }
}

void sat_print_stackstamp(uint64_t stackstamp) {
   struct Visitor: public SAT::IStackVisitor {
      virtual void visit(SAT::StackMarker& marker) override {
         char symbol[1024];
         marker.getSymbol(symbol, sizeof(symbol));
         printf("* %s\n", symbol);
      }
   } visitor;
   g_SAT.traverseStack(stackstamp, &visitor);
}