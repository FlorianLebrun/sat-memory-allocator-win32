
#include "./index.h"

using namespace SAT;

BaseHeap::~BaseHeap() {
}

int BaseHeap::getID() {
  return this->id;
}

void* BaseHeap::alloc(size_t size, uint64_t meta)
{
#ifdef _DEBUG
  size_t cfsize = size;
#endif
  void* ptr;
  if (g_SAT.enableObjectTracing) {
    ptr = this->allocTraced(size, meta);
  }
  else {
    IObjectAllocator* allocator;
    if (meta) size += sizeof(uint64_t);
    allocator = sizeMapping.getAllocator(size);
    assert(size <= allocator->allocatedSize());
    ptr = allocator->allocObject(size, meta);
  }

#ifdef _DEBUG
  SAT_DEBUG_CHECK(SAT::tObjectInfos infos);
  SAT_DEBUG_CHECK(bool hasInfos = sat_get_address_infos(ptr, &infos));
  SAT_DEBUG_CHECK(assert(hasInfos));
  SAT_DEBUG_CHECK(assert(infos.base == uintptr_t(ptr)));
  SAT_DEBUG_CHECK(assert(infos.size >= cfsize));
  SAT_DEBUG_CHECK(assert(infos.meta.bits == meta));
#endif
  return ptr;
}

void* BaseHeap::allocTraced(size_t size, uint64_t meta) {
  IObjectAllocator* allocator;
  void* ptr;

  // Trace object allocation
  if (g_SAT.enableObjectStackTracing) {
    size += sizeof(uint64_t) + sizeof(SAT::tObjectStamp);
    meta |= SAT::tObjectMetaData::cIsStamped_Bit;

    allocator = sizeMapping.getAllocator(size);
    assert(size <= allocator->allocatedSize());

    SAT::tpObjectStamp stamp = SAT::tpObjectStamp(allocator->allocObject(size, meta));
    stamp[0] = tObjectStamp(g_SAT.getCurrentTimestamp(), SAT::thread_instance->getStackStamp());
    ptr = &stamp[1];
  }
  else {
    if (meta) size += sizeof(uint64_t);
    allocator = sizeMapping.getAllocator(size);
    assert(size <= allocator->allocatedSize());
    ptr = allocator->allocObject(size, meta);
  }

  return ptr;
}

GlobalHeap::GlobalHeap(const char* name) {
  this->id = id;
  this->locals_list = 0;
  strcpy_s(this->name, 256, name ? name : "");
}

void GlobalHeap::destroy() {
  g_SAT.heaps_lock.lock();
  if (this->numRefs == 0) {
    assert(g_SAT.heaps_table[this->id] == this);
    g_SAT.heaps_table[this->id] = 0;
    if (this->nextGlobal) this->nextGlobal->backGlobal = this->backGlobal;
    if (this->backGlobal) this->backGlobal->nextGlobal = this->nextGlobal;
    else g_SAT.heaps_list = this->nextGlobal;
    this->terminate();
  }
  g_SAT.heaps_lock.unlock();
}

const char* GlobalHeap::getName() {
  return this->name;
}

void* GlobalHeap::allocBuffer(size_t size, SAT::tObjectMetaData meta) {
  return this->alloc(size, meta.bits);
}

uintptr_t GlobalHeap::acquirePages(size_t size) {
  return g_SAT.allocSegmentSpan(size);
}

void GlobalHeap::releasePages(uintptr_t index, size_t size) {
  return g_SAT.freeSegmentSpan(index, size);
}

LocalHeap::LocalHeap(GlobalHeap* global) {
  global->retain();
  global->locals_lock.lock();
  this->id = global->id;
  this->global = global;
  if (this->nextLocal = global->locals_list) global->locals_list->backLocal = this;
  this->backLocal = 0;
  global->locals_list = this;
  global->locals_lock.unlock();
}

void LocalHeap::destroy() {
  this->global->locals_lock.lock();
  if (this->numRefs == 0) {
    if (this->nextLocal) this->nextLocal->backLocal = this->backLocal;
    if (this->backLocal) this->backLocal->nextLocal = this->nextLocal;
    else this->global->locals_list = this->nextLocal;
    this->terminate();
  }
  this->global->locals_lock.unlock();
}

const char* LocalHeap::getName() {
  return this->global->name;
}

uintptr_t LocalHeap::acquirePages(size_t size) {
  return g_SAT.allocSegmentSpan(size);
}

void LocalHeap::releasePages(uintptr_t index, size_t size) {
  return g_SAT.freeSegmentSpan(index, size);
}

void* LocalHeap::allocBuffer(size_t size, SAT::tObjectMetaData meta) {
  uint64_t threadId = SystemThreading::GetCurrentThreadId();
  if (SAT::thread_instance && SAT::thread_instance->id == threadId) {
    return this->alloc(size, meta.bits);
  }
  else {
    return this->global->allocBuffer(size, meta);
  }
}
