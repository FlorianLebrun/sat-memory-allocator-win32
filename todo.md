
sat::IThread* sat::Controller::configureCurrentThread(const char* name, int heapId) {
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

sat::IThread* sat::Controller::getCurrentThread() {
  uint64_t threadId = SystemThreading::GetCurrentThreadId();
  if (sat::IThread* thread = this->getThreadById(threadId)) {
    return thread;
  }
  else {
    return this->configureCurrentThread(0, 0);
  }
}

sat::IThread* sat::Controller::getThreadById(uint64_t threadId) {
  Thread* thread = 0;
  this->threads_lock.lock();
  for (thread = this->threads_list; thread; thread = thread->nextThread) {
    if (thread->id == threadId) break;
  }
  this->threads_lock.unlock();
  return thread;
}

uint64_t sat::Thread::CaptureThreadCpuTime() {
  uint64_t timestamp;
  QueryThreadCycleTime(this->handle.handle, &timestamp);
  return timestamp;
}
