#include "./controller.h"
#include "../win32/system.h"
#include <algorithm>

using namespace SAT;

SAT::Controller g_SAT;
SAT::tEntry* g_SATable = 0;

namespace ZonedBuddyAllocator {
  uintptr_t traversePageObjects(uintptr_t index, bool &visitMore, SAT::IObjectVisitor* visitor);
}

namespace SAT {
  extern SAT::GlobalHeap* createHeapCompact(const char* name);
}

void SAT::Controller::traverseStack(uint64_t stackstamp, IStackVisitor* visitor) {
  if (this->stackStampDatabase) {
    this->stackStampDatabase->traverseStack(stackstamp, visitor);
  }
}

void SAT::Controller::traverseObjects(SAT::IObjectVisitor* visitor, uintptr_t start_address) {
  int i = start_address >> SAT::cSegmentSizeL2;
  bool visitMore = true;
  while (i < g_SATable->SATDescriptor.length && visitMore) {
    if (g_SATable[i].getHeapID() >= 0) {
      switch (g_SATable[i].id) {
      case SAT::tEntryID::PAGE_ZONED_BUDDY:
        i += ZonedBuddyAllocator::traversePageObjects(i, visitMore, visitor);
        break;
      default: i++;
      }
    }
    else i++;
  }
}

IHeap* SAT::Controller::getHeap(int id) {
  if (id >= 0 && id < 256) return this->heaps_table[id];
  return 0;
}

IHeap* SAT::Controller::createHeap(tHeapType type, const char* name) {
  g_SAT.heaps_lock.lock();

  // Get heap id
  int heapId = -1;
  for (int i = 0; i < 256; i++) {
    if (!g_SAT.heaps_table[i]) {
      heapId = i;
      break;
    }
  }

  // Create heap
  GlobalHeap* heap = 0;
  if (heapId >= 0) {
    switch (type) {
    case tHeapType::COMPACT_HEAP_TYPE:
      heap = SAT::createHeapCompact(name);
      break;
    }
    heap->id = heapId;
    g_SAT.heaps_table[heapId] = heap;
  }

  g_SAT.heaps_lock.unlock();
  return heap;
}

void createSegmentAllocationTable() {
  // Compute memory limit
  uintptr_t memoryLimit = SystemMemory::GetMemorySize();
  if (sizeof(uintptr_t) > 4) {
    memoryLimit = std::min<uintptr_t>(uintptr_t(10000000000), memoryLimit);
  }

  uintptr_t size = alignX<intptr_t>(sizeof(SAT::tEntry)*(memoryLimit >> SAT::cSegmentSizeL2), SAT::cSegmentSize);
  uintptr_t limit = size / sizeof(SAT::tEntry);

  // Check memory page size
  if (SAT::cSegmentSize < SystemMemory::GetPageSize()) {
    throw std::exception("SAT allocator cannot work with the current system memory page size");
  }

  // Create the table
  g_SATable = (SAT::tEntry*)SystemMemory::AllocBuffer(SAT::cSegmentMinAddress, memoryLimit, size, SAT::cSegmentSize);
  memset(g_SATable, 0xff, size);

  // Initialize the segment table
  g_SATable->SATDescriptor.id = SAT::tEntryID::FORBIDDEN;
  g_SATable->SATDescriptor.bytesPerSegmentL2 = SAT::cSegmentSizeL2;
  g_SATable->SATDescriptor.bytesPerAddress = sizeof(uintptr_t);
  g_SATable->SATDescriptor.limit = limit;
  g_SATable->SATDescriptor.length = limit;

  // Mark the Forbidden zone
  for (uintptr_t i = 1; i < SAT::cSegmentMinIndex; i++) {
    g_SATable[i].forbidden.set();
  }

  // Mark the SAT with itself
  uintptr_t SATIndex = uintptr_t(g_SATable) >> SAT::cSegmentSizeL2;
  uintptr_t SATSize = size >> SAT::cSegmentSizeL2;
  for (uintptr_t i = 0; i < SATSize; i++) {
    g_SATable[SATIndex + i].SATSegment.set();
  }

  // Mark free pages
  g_SAT.segments_allocator.appendSegments(SAT::cSegmentMinIndex, SATIndex - SAT::cSegmentMinIndex);
  g_SAT.segments_allocator.appendSegments(SATIndex + SATSize, limit - SATIndex - SATSize);
}

void SAT::Controller::initialize() {
  static bool is_initialized = false;
  if (is_initialized) {
    //printf("sat allocator is initalized more than once.");
    return;
  }
  is_initialized = true;

  // Init os features
  InitSystemFeatures();

  // Allocate the SAT
  createSegmentAllocationTable();

  // Init analysis features
  this->enableObjectTracing = false;
  this->enableObjectStackTracing = false;
  this->enableObjectTimeTracing = false;

  // Init timing features
  this->timeOrigin = SystemThreading::GetTimeStamp();
  this->timeFrequency = SystemThreading::GetTimeFrequency();

  // Init heap table
  this->heaps_list = 0;
  memset(this->heaps_table, sizeof(this->heaps_table), 0);

  // Allocate buffer 64
  uintptr_t buffer64_size = 1024 * 4;
  uintptr_t buffer64_index = g_SAT.allocSegmentSpan(buffer64_size);
  if (!buffer64_index) throw std::exception("map on reserved segment has failed");
  this->buffers64 = tpBuffer64(buffer64_index << SAT::cSegmentSizeL2);
  this->buffers64Limit = tpBuffer64((buffer64_index + buffer64_size) << SAT::cSegmentSizeL2);
  memset(this->buffers64Levels, 0, sizeof(this->buffers64Levels));

  // Mark buffer 64 pool
  for (uintptr_t i = 0; i < buffer64_size; i++) {
    g_SATable[buffer64_index + i].id = SAT::tEntryID::PAGE_DESCRIPTOR;
  }

  // Create stackstamp database
  this->stackStampDatabase = createStackStampDatabase();

  // Create types database
  SAT::TypesDataBase_init();

  // Create heap 0
  g_SAT.createHeap(SAT::tHeapType::COMPACT_HEAP_TYPE, "main");
}

uintptr_t SAT::Controller::allocSegmentSpan(uintptr_t size) {
  uintptr_t index = this->segments_allocator.allocSegments(size, 1);
  this->commitMemory(index, size);
  return index;
}

void SAT::Controller::freeSegmentSpan(uintptr_t index, uintptr_t size) {
  _ASSERT(size > 0);
  g_SAT.decommitMemory(index, size);
  this->segments_allocator.freeSegments(index, size);
}

void SAT::Controller::commitMemory(uintptr_t index, uintptr_t size) {
  uintptr_t ptr = index << SAT::cSegmentSizeL2;

  // Commit the segment memory
  if (!SystemMemory::CommitMemory(ptr, size << SAT::cSegmentSizeL2)) {
    g_SAT.printSegments();
    throw std::exception("commit on reserved segment has failed");
  }

}

void SAT::Controller::decommitMemory(uintptr_t index, uintptr_t size) {
  uintptr_t ptr = index << SAT::cSegmentSizeL2;

  // Commit the segment memory
  if (!SystemMemory::DecommitMemory(ptr, size << SAT::cSegmentSizeL2)) {
    throw std::exception("decommit on reserved segment has failed");
  }
}

uintptr_t SAT::Controller::reserveMemory(uintptr_t size, uintptr_t alignL2) {
  return this->segments_allocator.allocSegments(size, alignL2);
}

void* SAT::Controller::allocBuffer64(uint32_t level) {
  tpBuffer64 ptr;
  if(this->buffers64Levels[level]) {
    this->buffers64Lock.lock();
    ptr = this->buffers64Levels[level];
    this->buffers64Lock.unlock();
    if(ptr) return ptr;
  }
  ptr = this->buffers64.fetch_add(uint64_t(1) << level);
  _ASSERT(ptr < this->buffers64Limit);
  return ptr;
}

void SAT::Controller::freeBuffer64(void* ptr, uint32_t level) {
  this->buffers64Lock.lock();
  tpBuffer64(ptr)->next = this->buffers64Levels[level];
  this->buffers64Levels[level] = tpBuffer64(ptr);
  this->buffers64Lock.unlock();
}

uintptr_t SATEntryToString(uintptr_t index, char* out) {
  SAT::tEntry& entry = g_SATable[index];
  string name;
  uintptr_t len;
  for (len = 0; (index + len) < g_SATable->SATDescriptor.length && g_SATable[index + len].id == entry.id; len++);
  switch (entry.id) {
  case SAT::tEntryID::SAT_SEGMENT: name = "SAT_SEGMENT"; break;
  case SAT::tEntryID::FREE: name = "FREE"; break;
  case SAT::tEntryID::FORBIDDEN: name = "FORBIDDEN"; break;
  case SAT::tEntryID::STREAM_SEGMENT: name = "STREAM_SEGMENT"; break;
  case SAT::tEntryID::STACKSTAMP_DATABASE: name = "STACKSTAMP_DATABASE"; break;
  case SAT::tEntryID::TYPES_DATABASE: name = "TYPES_DATABASE"; break;
  case SAT::tEntryID::PAGE_DESCRIPTOR: name = "PAGE_DESCRIPTOR"; break;

  case SAT::tEntryID::LARGE_OBJECT: name = "LARGE_OBJECT"; len = tpHeapLargeObjectEntry(&entry)->length; break;
  case SAT::tEntryID::PAGE_ZONED_BUDDY: name = "PAGE_ZONED_BUDDY"; len = 1; break;
  default: name = "Unkown"; break;
  }
  sprintf_s(out, 512, "[ %.5d to %.5d ] %s", uint32_t(index), uint32_t(index + len - 1), name.c_str());
  return len;
}

void SAT::Controller::printSegments() {
  char bout[512];
  uintptr_t i = 0;
  std::string name;
  printf("\nSegment Allocation Table:\n");
  while (i < g_SATable->SATDescriptor.length) {
    i += SATEntryToString(i, bout);
    printf("%s\n", bout);
  }

  //printf("\nFreesegment map:\n");
  //g_SAT.segments_allocator.traverse();
}

IProfile* SAT::Controller::createProfile() {
  return new(this->allocBuffer(sizeof(SATProfile))) SATProfile();
}

double SAT::Controller::getCurrentTime() {
  uint64_t timestamp = this->Controller::getCurrentTimestamp();
  return double(timestamp) / double(this->timeFrequency);
}

double SAT::Controller::getTime(uint64_t timestamp) {
  return double(timestamp) / double(this->timeFrequency);
}

uint64_t SAT::Controller::getCurrentTimestamp() {
  return SystemThreading::GetTimeStamp() - this->timeOrigin;
}

uint64_t SAT::Controller::getTimestamp(double time) {
  return uint64_t(time*double(this->timeFrequency));
}

SAT::IThread* SAT::Controller::configureCurrentThread(const char* name, int heapId) {
  uint64_t threadId = SystemThreading::GetCurrentThreadId();
  if (this->getThreadById(threadId)) {
    throw std::exception("Thread already registered");
  }
  else if (!g_SAT.heaps_table[heapId]) {
    throw std::exception("Heap required for this thread doesn't exist");
  }
  printf("thread:%lld %s is attached.\n", threadId, name);
  return new(this->allocBuffer(sizeof(Thread))) Thread(name, threadId, heapId);
}

SAT::IThread* SAT::Controller::getCurrentThread() {
  uint64_t threadId = SystemThreading::GetCurrentThreadId();
  if (SAT::IThread* thread = this->getThreadById(threadId)) {
    return thread;
  }
  else {
    return this->configureCurrentThread(0, 0);
  }
}

SAT::IThread* SAT::Controller::getThreadById(uint64_t threadId) {
  Thread* thread = 0;
  this->threads_lock.lock();
  for (thread = this->threads_list; thread; thread = thread->nextThread) {
    if (thread->id == threadId) break;
  }
  this->threads_lock.unlock();
  return thread;
}

void sat_init_process() {
  g_SAT.initialize();
}

void sat_terminate_process() {
  // COULD BE USED
}

void sat_attach_current_thread() {
}

void sat_dettach_current_thread() {
  if (SAT::Thread* thread = SAT::current_thread) {
    printf("thread:%lld is dettached.\n", thread->id);
    SAT::current_thread = 0;
    thread->release();
  }
}

