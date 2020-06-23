#include "./index.h"

using namespace SAT;

Thread::Thread(const char* name, uint64_t threadId, int heapId) {
  assert(!SAT::current_thread);

  // Get global heap
  GlobalHeap* heap = g_SAT.heaps_table[heapId];
  heap->retain();

  // Create thread instance
  if(name) strcpy_s(this->name, sizeof(this->name), name);
  else this->name[0] = 0;
  this->id = threadId;
  this->handle = SystemThreading::GetCurrentThread();
  this->global_heap = heap;
  this->local_heap = heap->createLocal();
  
  this->stack_analyzer = 0;
  this->beaconsCount = 0;

  // Map to TLS
  SAT::current_thread = this;

  // Attach thread
  g_SAT.threads_lock.lock();
  if (this->nextThread = g_SAT.threads_list)
    g_SAT.threads_list->backThread = this;
  this->backThread = 0;
  g_SAT.threads_list = this;
  g_SAT.threads_lock.unlock();
}

void Thread::destroy() {
  g_SAT.threads_lock.lock();
  if (this->numRefs == 0) {
    if (this->nextThread) this->nextThread->backThread = this->backThread;
    if (this->backThread) this->backThread->nextThread = this->nextThread;
    else g_SAT.threads_list = this->nextThread;
    this->local_heap->release();
    this->global_heap->release();
    this->nextThread = 0;
    this->backThread = 0;
    this->local_heap = 0;
    this->global_heap = 0;
    this->id = 0;
    this->stack_analyzer = 0;
  }
  g_SAT.threads_lock.unlock();
}

uint64_t Thread::getID() {
  return this->id;
}

bool Thread::setName(const char* name) {
  strcpy_s(this->name, sizeof(this->name), name);
  return true;
}

const char* Thread::getName() {
  if(!this->name[0]) SystemThreading::GetThreadName(&this->handle, this->name, sizeof(this->name));
  return this->name;
}

uint64_t Thread::getStackStamp() {
  if (g_SAT.stackStampDatabase) {
    if (SAT::current_thread == this) return g_SAT.stackStampDatabase->getStackStamp(0, this->stack_analyzer);
    else return g_SAT.stackStampDatabase->getStackStamp(this, this->stack_analyzer);
  }
  return 0;
}

void Thread::setStackAnalyzer(IStackStampAnalyzer* stack_analyzer) {
  this->stack_analyzer = stack_analyzer;
}
