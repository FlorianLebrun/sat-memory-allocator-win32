#include "./index.h"

void LargeObjectAllocator::Global::init(SAT::IHeap* heap) {
  this->heapID = heap->getID();
}

void* LargeObjectAllocator::Global::allocate(size_t size) {
  return this->LargeObjectAllocator::Global::allocateWithMeta(size, 0);
}

void* LargeObjectAllocator::Global::allocateWithMeta(size_t size, uint64_t meta) {

  // Allocate memory space
  uint32_t length = alignX<int32_t>(size, SAT::cSegmentSize) >> SAT::cSegmentSizeL2;
  uintptr_t index = g_SAT.allocSegmentSpan(length);

  // Mark SAT entries
  for (uint32_t i = 0; i < length; i++) {
    SAT::tpHeapLargeObjectEntry entry = SAT::tpHeapLargeObjectEntry(&g_SATable[index + i]);
    entry->set(this->heapID, i, length, meta);
  }

  return (void*)(index << SAT::cSegmentSizeL2);
}

size_t LargeObjectAllocator::Global::getMaxAllocatedSize() {
  return g_SATable->SATDescriptor.limit << SAT::cSegmentSizeL2;
}

size_t LargeObjectAllocator::Global::getMinAllocatedSize() {
  return SAT::cSegmentSize;
}

size_t LargeObjectAllocator::Global::getAllocatedSize(size_t size) {
  return alignX<int32_t>(size, SAT::cSegmentSize);
}

size_t LargeObjectAllocator::Global::getMaxAllocatedSizeWithMeta() {
  return this->LargeObjectAllocator::Global::getMaxAllocatedSize();
}

size_t LargeObjectAllocator::Global::getMinAllocatedSizeWithMeta() {
  return this->LargeObjectAllocator::Global::getMinAllocatedSize();
}

size_t LargeObjectAllocator::Global::getAllocatedSizeWithMeta(size_t size) {
  return this->LargeObjectAllocator::Global::getAllocatedSize(size);
}

bool LargeObjectAllocator::get_address_infos(uintptr_t ptr, SAT::tpObjectInfos infos) {
  SAT::tpHeapLargeObjectEntry entry = g_SATable->get<SAT::tHeapLargeObjectEntry>(ptr >> SAT::cSegmentSizeL2);
  if (infos) {
    infos->set(
      entry->heapID,
      uintptr_t(ptr)&SAT::cSegmentPtrMask,
      entry->length << SAT::cSegmentSizeL2,
      &entry->meta);
  }
  return true;
}

size_t LargeObjectAllocator::Global::freePtr(uintptr_t index) {
  SAT::tpHeapLargeObjectEntry entry = SAT::tpHeapLargeObjectEntry(&g_SATable[index]);
  assert(entry->index == 0);
  size_t length = entry->length;
  g_SAT.freeSegmentSpan(index, length);
  return length << SAT::cSegmentSizeL2;
}
