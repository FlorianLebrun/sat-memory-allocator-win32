
#include "../../base.h"
#include "./stream.h"

void* SATStream::operator new(size_t nSize) {
  uintptr_t index = g_SAT.allocSegmentSpan(1);
  SATStream* stream = (SATStream*)(index << SAT::cSegmentSizeL2);
  stream->baseSize = align8(nSize);
  return stream;
}

void SATStream::operator delete(void* ptr, size_t nSize) {
  uintptr_t index = uintptr_t(ptr) >> SAT::cSegmentSizeL2;
  g_SAT.freeSegmentSpan(index, 1);
}

SATStream::SATStream(uint32_t recordSize) {
  assert((recordSize & 0x7) == 0);
  assert(recordSize > sizeof(SATStreamRecord));
  assert(recordSize < 0x100);
  this->recordSize = recordSize;

  this->attributId = SATAttributID::NONE;
  this->thread = 0;
  this->heap = 0;
  this->profileNext = 0;
  this->profile = 0;

  // Initialize first samples page
  uintptr_t entry_index = this->getEntryIndex();
  tpSATStreamEntry entry = tpSATStreamEntry(&g_SATable[entry_index]);
  entry->id = SAT::tEntryID::STREAM_SEGMENT;
  entry->timestamp = g_SAT.getCurrentTimestamp();
  entry->size = this->baseSize;
  entry->nextIndex = 0;
  this->lastEntry = entry;
  this->lastIndex = entry_index;
}

SATStream::~SATStream() {
  uintptr_t index = this->getEntryFrame()->nextIndex;
  while(index) {
    uintptr_t nextIndex = tpSATStreamEntry(&g_SATable[index])->nextIndex;
    g_SAT.freeSegmentSpan(index, 1);
    index = nextIndex;
  }
}

uintptr_t SATStream::getEntryIndex() {
  return uintptr_t(this) >> SAT::cSegmentSizeL2;
}

tpSATStreamEntry SATStream::getEntryFrame() {
  return tpSATStreamEntry(&g_SATable[this->getEntryIndex()]);
}

tpSATStreamRecord SATStream::createSample() {

  this->lock.lock();

  // Alloc new frame when current overflow
  if (this->lastEntry->size + this->recordSize > SAT::cSegmentSize) {
    uintptr_t entry_index = g_SAT.allocSegmentSpan(1);
    tpSATStreamEntry entry = tpSATStreamEntry(&g_SATable[entry_index]);
    entry->id = SAT::tEntryID::STREAM_SEGMENT;
    entry->timestamp = g_SAT.getCurrentTimestamp();
    entry->size = 0;
    entry->nextIndex = 0;

    this->lastEntry->nextIndex = entry_index;
    this->lastEntry = entry;
    this->lastIndex = entry_index;
  }

  // Append new record in current frame
  tpSATStreamRecord record = tpSATStreamRecord((this->lastIndex << SAT::cSegmentSizeL2)+this->lastEntry->size);
  record->timestamp = g_SAT.getCurrentTimestamp();
  this->lastEntry->size += this->recordSize;

  this->lock.unlock();
  return record;
}

void SATStream::getTimeRange(uint64_t& timeStart, uint64_t& timeEnd) {
  uintptr_t index = this->getEntryIndex();
  while(index) {
    uintptr_t nextIndex = tpSATStreamEntry(&g_SATable[index])->nextIndex;
    g_SAT.freeSegmentSpan(index, 1);
    index = nextIndex;
  }

  timeStart = this->getEntryFrame()->timestamp;
  if (lastEntry->size > 0) {
    tpSATStreamRecord lastRecord = &tpSATStreamRecord((lastIndex << SAT::cSegmentSizeL2) + lastEntry->size)[-1];
    timeEnd = lastRecord->timestamp;
  }
  else timeEnd = lastEntry->timestamp;
}
