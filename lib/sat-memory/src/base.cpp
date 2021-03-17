#include "./segments-allocator.hpp"
#include "./memory-buffers64.hpp"
#include "./win32/system.h"
#include "../../common/alignment.h"
#include "../../common/bitwise.h"
#include <algorithm>
#include <string>

sat::memory::MemoryTable* sat::memory::table = 0;
static sat::SegmentsAllocator segments_allocator;
static sat::PooledBuffers64 buffers64_allocator;

sat::memory::MemoryTable::MemoryTable(uintptr_t size, uintptr_t limit) {
   memset(this, 0xff, size);

   // Initialize the segment table
   this->descriptor.id = sat::memory::tEntryID::FORBIDDEN;
   this->descriptor.bytesPerSegmentL2 = sat::memory::cSegmentSizeL2;
   this->descriptor.bytesPerAddress = sizeof(uintptr_t);
   this->descriptor.limit = limit;
   this->descriptor.length = limit;

   // Mark the Forbidden zone
   for (uintptr_t i = 1; i < sat::memory::cSegmentMinIndex; i++) {
      this->entries[i].forbidden.set();
   }

   // Mark the sat with itself
   uintptr_t SATIndex = uintptr_t(this) >> sat::memory::cSegmentSizeL2;
   uintptr_t SATSize = size >> sat::memory::cSegmentSizeL2;
   for (uintptr_t i = 0; i < SATSize; i++) {
      this->entries[SATIndex + i].SATSegment.set();
   }

   // Mark free pages
   segments_allocator.appendSegments(sat::memory::cSegmentMinIndex, SATIndex - sat::memory::cSegmentMinIndex);
   segments_allocator.appendSegments(SATIndex + SATSize, limit - SATIndex - SATSize);

   // Allocate buffer 64
   buffers64_allocator.initialize(this);
}

void sat::memory::MemoryTable::initialize() {
   if (sat::memory::table) {
      printf("sat allocator is initalized more than once.");
      return;
   }

   // Init os features
   InitSystemFeatures();

   // Compute memory limit
   uintptr_t memoryLimit = SystemMemory::GetMemorySize();
   if (sizeof(uintptr_t) > 4) {
      memoryLimit = std::min<uintptr_t>(uintptr_t(10000000000), memoryLimit);
   }

   uintptr_t size = alignX<intptr_t>(sizeof(sat::memory::tEntry) * (memoryLimit >> sat::memory::cSegmentSizeL2), sat::memory::cSegmentSize);
   uintptr_t limit = size / sizeof(sat::memory::tEntry);

   // Check memory page size
   if (sat::memory::cSegmentSize < SystemMemory::GetPageSize()) {
      throw std::exception("sat allocator cannot work with the current system memory page size");
   }

   // Create the table
   sat::memory::table = (MemoryTable*)SystemMemory::AllocBuffer(sat::memory::cSegmentMinAddress, memoryLimit, size, sat::memory::cSegmentSize);
   sat::memory::table->MemoryTable::MemoryTable(size, limit);
}

uintptr_t sat::memory::MemoryTable::allocSegmentSpan(uintptr_t size) {
   uintptr_t index = segments_allocator.allocSegments(size, 1);
   this->commitMemory(index, size);
   return index;
}

void sat::memory::MemoryTable::freeSegmentSpan(uintptr_t index, uintptr_t size) {
   _ASSERT(size > 0);
   this->decommitMemory(index, size);
   segments_allocator.freeSegments(index, size);
}

void sat::memory::MemoryTable::commitMemory(uintptr_t index, uintptr_t size) {
   uintptr_t ptr = index << sat::memory::cSegmentSizeL2;

   // Commit the segment memory
   if (!SystemMemory::CommitMemory(ptr, size << sat::memory::cSegmentSizeL2)) {
      this->printSegments();
      throw std::exception("commit on reserved segment has failed");
   }

}

void sat::memory::MemoryTable::decommitMemory(uintptr_t index, uintptr_t size) {
   uintptr_t ptr = index << sat::memory::cSegmentSizeL2;

   // Commit the segment memory
   if (!SystemMemory::DecommitMemory(ptr, size << sat::memory::cSegmentSizeL2)) {
      throw std::exception("decommit on reserved segment has failed");
   }
}

uintptr_t sat::memory::MemoryTable::reserveMemory(uintptr_t size, uintptr_t alignL2) {
   return segments_allocator.allocSegments(size, alignL2);
}

void* sat::memory::MemoryTable::allocBuffer64(uint32_t level) {
   return buffers64_allocator.allocBufferSpanL2(level);
}

void sat::memory::MemoryTable::freeBuffer64(void* ptr, uint32_t level) {
   return buffers64_allocator.freeBufferSpanL2(ptr, level);
}

void* sat::memory::MemoryTable::allocBuffer(uint32_t size) {
   return buffers64_allocator.allocBufferSpanL2(msb_32(size) + 1);
}

void sat::memory::MemoryTable::freeBuffer(void* ptr, uint32_t size) {
   return buffers64_allocator.freeBufferSpanL2(ptr, msb_32(size) + 1);
}

static uintptr_t SATEntryToString(uintptr_t index, char* out) {
   using namespace sat::memory;
   auto& entry = table->entries[index];
   std::string name;
   uintptr_t len;
   for (len = 0; (index + len) < sat::memory::table->entries->SATDescriptor.length && sat::memory::table->entries[index + len].id == entry.id; len++);
   switch (entry.id) {
   case tEntryID::SAT_SEGMENT: name = "SAT_SEGMENT"; break;
   case tEntryID::FREE: name = "FREE"; break;
   case tEntryID::FORBIDDEN: name = "FORBIDDEN"; break;
   case tEntryID::PAGE_DESCRIPTOR: name = "PAGE_DESCRIPTOR"; break;
   default: name = "<user.specific>"; break;
   }
   sprintf_s(out, 512, "[ %.5d to %.5d ] %s", uint32_t(index), uint32_t(index + len - 1), name.c_str());
   return len;
}

void sat::memory::MemoryTable::printSegments() {
   char bout[512];
   uintptr_t i = 0;
   std::string name;
   printf("\nSegment Allocation Table:\n");
   while (i < sat::memory::table->descriptor.length) {
      i += SATEntryToString(i, bout);
      printf("%s\n", bout);
   }
}
