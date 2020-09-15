
#include "./index.h"

using namespace SAT;

BaseHeap::~BaseHeap() {
}

int BaseHeap::getID() {
  return this->id;
}

size_t BaseHeap::getMaxAllocatedSize() {
  return this->sizeMapping.getMaxAllocator()->getMaxAllocatedSize();
}
size_t BaseHeap::getMinAllocatedSize() {
  return 0;
}
size_t BaseHeap::getAllocatedSize(size_t size) {
  return this->sizeMapping.getAllocator(size)->getAllocatedSize(size);
}
size_t BaseHeap::getMaxAllocatedSizeWithMeta() {
  return this->sizeMappingWithMeta.getMaxAllocator()->getMaxAllocatedSize();
}
size_t BaseHeap::getMinAllocatedSizeWithMeta() {
  return 0;
}
size_t BaseHeap::getAllocatedSizeWithMeta(size_t size) {
  return this->sizeMappingWithMeta.getAllocator(size)->getAllocatedSizeWithMeta(size);
}

#define UseOverflowProtection 1

void* BaseHeap::allocate(size_t size)
{  
#if UseOverflowProtection
  void* ptr = this->allocateWithMeta(size, 0);
#else
  SAT::IObjectAllocator* allocator = this->sizeMapping.getAllocator(size);
  void* ptr = allocator->allocate(size);
#endif

#ifdef _DEBUG
  SAT_DEBUG_CHECK(SAT::tObjectInfos infos);
  SAT_DEBUG_CHECK(bool hasInfos = sat_get_address_infos(ptr, &infos));
  SAT_DEBUG_CHECK(assert(hasInfos));
  SAT_DEBUG_CHECK(assert(infos.base == uintptr_t(ptr)));
  SAT_DEBUG_CHECK(assert(infos.size >= size));
#endif
  return ptr;
}

void* BaseHeap::allocateWithMeta(size_t size, uint64_t meta)
{
#if UseOverflowProtection
  static const size_t paddingMinSize = 8;
  size_t allocatedSize = size + paddingMinSize + sizeof(tObjectStamp);
  meta |= tObjectMetaData::cIsOverflowProtected_Bit | tObjectMetaData::cIsStamped_Bit;

  // Allocate buffer
  SAT::IObjectAllocator* allocator = this->sizeMappingWithMeta.getAllocator(allocatedSize);
  void* ptr = allocator->allocateWithMeta(allocatedSize, meta);

  // Prepare buffer tail
  {
     allocatedSize = allocator->getAllocatedSizeWithMeta(allocatedSize);

     // Write stamp
     tpObjectStamp stamp = tpObjectStamp(((uint8_t*)ptr) + allocatedSize - sizeof(tObjectStamp));
     stamp->stackstamp = 0;
     stamp->timestamp = g_SAT.getCurrentTimestamp();

     // Write overflow detection bytes
     uint32_t& bufferSize = *(uint32_t*)(((uint8_t*)ptr) + allocatedSize - sizeof(uint32_t) - sizeof(tObjectStamp));
     bufferSize = size ^ 0xabababab;
     uint8_t* paddingBytes = ((uint8_t*)ptr) + size;
     memset(paddingBytes, 0xab, allocatedSize - size - sizeof(uint32_t) - sizeof(tObjectStamp));
  }
#else
  SAT::IObjectAllocator* allocator = this->sizeMappingWithMeta.getAllocator(size);
  void* ptr = allocator->allocateWithMeta(size, meta);
#endif

#ifdef _DEBUG
  SAT_DEBUG_CHECK(SAT::tObjectInfos infos);
  SAT_DEBUG_CHECK(bool hasInfos = sat_get_address_infos(ptr, &infos));
  SAT_DEBUG_CHECK(assert(hasInfos));
  SAT_DEBUG_CHECK(assert(infos.base == uintptr_t(ptr)));
  SAT_DEBUG_CHECK(assert(infos.size >= size));
  SAT_DEBUG_CHECK(assert(infos.meta.bits == meta));
#endif
  return ptr;
}

/*void* BaseHeap::allocTraced(size_t size, uint64_t meta) {
SAT::IObjectAllocator* allocator;
void* ptr;

// Trace object allocation
if (g_SAT.enableObjectStackTracing) {
size += sizeof(uint64_t) + sizeof(SAT::tObjectStamp);
meta |= SAT::tObjectMetaData::cIsStamped_Bit;

allocator = sizeMapping.getAllocator(size);
assert(size <= allocator->getMaxAllocatedSize());

SAT::tpObjectStamp stamp = SAT::tpObjectStamp(allocator->allocObject(size, meta));
stamp[0] = tObjectStamp(g_SAT.getCurrentTimestamp(), SAT::current_thread->getStackStamp());
ptr = &stamp[1];
}
else {
if (meta) size += sizeof(uint64_t);
allocator = sizeMapping.getAllocator(size);
assert(size <= allocator->getMaxAllocatedSize());
ptr = allocator->allocObject(size, meta);
}

return ptr;
}*/

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

