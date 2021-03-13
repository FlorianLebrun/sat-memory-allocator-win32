#include <sat-memory/memory.hpp>
#include <sat-threads/spinlock.hpp>
#include "./memory-buffers64.hpp"
#include "./utils/bitwise.h"
#include <exception>

void sat::PooledBuffers64::initialize(sat::memory::MemoryTable* table) {

   // Allocate buffer 64
   uintptr_t buffer64_size = 1024 * 4;
   uintptr_t buffer64_index = table->allocSegmentSpan(buffer64_size);
   if (!buffer64_index) throw std::exception("map on reserved segment has failed");
   this->Cursor = tpBuffer64(buffer64_index << sat::memory::cSegmentSizeL2);
   this->Limit = tpBuffer64((buffer64_index + buffer64_size) << sat::memory::cSegmentSizeL2);
   memset(this->Levels, 0, sizeof(this->Levels));

   // Mark buffer 64 pool
   for (uintptr_t i = 0; i < buffer64_size; i++) {
      table->entries[buffer64_index + i].id = sat::memory::tEntryID::PAGE_DESCRIPTOR;
   }
}

void* sat::PooledBuffers64::allocBufferSpanL2(uint32_t level) {
   tpBuffer64 ptr;
   if (this->Levels[level]) {
      this->Lock.lock();
      ptr = this->Levels[level];
      this->Lock.unlock();
      if (ptr) return ptr;
   }
   ptr = this->Cursor.fetch_add(uint64_t(1) << level);
   _ASSERT(ptr < this->Limit);
   return ptr;
}

void sat::PooledBuffers64::freeBufferSpanL2(void* ptr, uint32_t level) {
   this->Lock.lock();
   tpBuffer64(ptr)->next = this->Levels[level];
   this->Levels[level] = tpBuffer64(ptr);
   this->Lock.unlock();
}
