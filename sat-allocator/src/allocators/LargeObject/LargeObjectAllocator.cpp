#include "./index.h"

void LargeObjectAllocator::Global::init(sat::IHeap* heap) {
   this->heapID = heap->getID();
}

void* LargeObjectAllocator::Global::allocate(size_t size) {
   return this->LargeObjectAllocator::Global::allocateWithMeta(size, 0);
}

void* LargeObjectAllocator::Global::allocateWithMeta(size_t size, uint64_t meta) {

   // Allocate memory space
   uint32_t length = alignX<int32_t>(size, sat::memory::cSegmentSize) >> sat::memory::cSegmentSizeL2;
   uintptr_t index = sat::memory::table->allocSegmentSpan(length);

   // Mark sat entries
   for (uint32_t i = 0; i < length; i++) {
      auto entry = sat::memory::table->get<tHeapLargeObjectEntry>(index + i);
      entry->set(this->heapID, i, length, meta);
   }

   return (void*)(index << sat::memory::cSegmentSizeL2);
}

size_t LargeObjectAllocator::Global::getMaxAllocatedSize() {
   return sat::memory::table->descriptor.limit << sat::memory::cSegmentSizeL2;
}

size_t LargeObjectAllocator::Global::getMinAllocatedSize() {
   return sat::memory::cSegmentSize;
}

size_t LargeObjectAllocator::Global::getAllocatedSize(size_t size) {
   return alignX<int32_t>(size, sat::memory::cSegmentSize);
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

bool LargeObjectAllocator::get_address_infos(uintptr_t ptr, sat::tpObjectInfos infos) {
   auto entry = sat::memory::table->get<tHeapLargeObjectEntry>(ptr >> sat::memory::cSegmentSizeL2);
   if (infos) {
      infos->set(
         entry->heapID,
         uintptr_t(ptr) & sat::memory::cSegmentPtrMask,
         size_t(entry->length) << sat::memory::cSegmentSizeL2,
         &entry->meta);
   }
   return true;
}

size_t LargeObjectAllocator::Global::freePtr(uintptr_t index) {
   auto entry = sat::memory::table->get<tHeapLargeObjectEntry>(index);
   assert(entry->index == 0);
   size_t length = entry->length;
   sat::memory::table->freeSegmentSpan(index, length);
   return length << sat::memory::cSegmentSizeL2;
}
