#include "./threads.hpp"
#include <Windows.h>

using namespace sat;
using namespace sat::impl;

struct BasicThreadRoot {
   SpinLock lock;
   std::vector<BasicThreadPool*> pools;
   DefaultThreadPool defaultPools;
   void addPool(BasicThreadPool* pool) {
      SpinLockHolder guard(this->lock);
      this->pools.push_back(pool);
   }
   void removePool(BasicThreadPool* pool) {
      SpinLockHolder guard(this->lock);
      for (auto it = this->pools.begin(); it != this->pools.end(); ) {
         if (*it == pool) {
            this->pools.erase(it);
            return;
         }
      }
   }
};

static BasicThreadRoot root;
static __declspec(thread) Thread* current_thread = nullptr;

Thread* Thread::current() {
   if (::current_thread) return ::current_thread;
   else return root.defaultPools.create(nullptr);
}

DefaultThread::DefaultThread()
   : pool(&root.defaultPools)
{
   this->threadId = GetCurrentThreadId();
   this->threadHandle = 0;
}
DefaultThread::~DefaultThread() {
   if (this->threadHandle) CloseHandle(this->threadHandle);
}

uint64_t DefaultThread::getID() {
   return this->threadId;
}

uintptr_t DefaultThread::getNativeHandle() {
   if (!this->threadHandle) this->threadHandle = OpenThread(PROCESS_ALL_ACCESS, TRUE, this->threadId);
   return uintptr_t(this->threadHandle);
}

void DefaultThreadPool::foreach(std::function<void(Thread*)>&& callback) {
   SpinLockHolder guard(this->lock);
   for (auto* thread : this->threads) {
      callback(thread);
   }
}

Thread* DefaultThreadPool::create(std::function<void()>&& entrypoint) {
   if (!::current_thread) {
      SpinLockHolder guard(this->lock);
      DefaultThread* thread = new DefaultThread();
      ::current_thread = thread;
      this->threads.push_back(thread);
      return thread;
   }
   throw std::exception("cannot create current thread when already exists");
}

BasicThread::BasicThread(BasicThreadPool* pool, std::function<void()>&& entrypoint)
   : pool(pool), thread(pool->attrs, entrypoint) {
}

uint64_t BasicThread::getID() {
   return uint64_t(GetThreadId(this->thread.native_handle()));
}

uintptr_t BasicThread::getNativeHandle() {
   return uintptr_t(this->thread.native_handle());
}

BasicThreadPool::BasicThreadPool(int stacksize) {
   this->attrs.set_stack_size(stacksize);
   root.addPool(this);
}

BasicThreadPool::~BasicThreadPool() {
   root.removePool(this);
}

void BasicThreadPool::foreach(std::function<void(Thread*)>&& callback) {
   SpinLockHolder guard(this->lock);
   for (auto* thread : this->threads) {
      callback(thread);
   }
}

Thread* BasicThreadPool::create(std::function<void()>&& entrypoint) {
   if (entrypoint) {
      SpinLockHolder guard(this->lock);
      auto thread = new BasicThread(this, std::forward<decltype(entrypoint)>(entrypoint));
      this->threads.push_back(thread);
      return thread;
   }
   throw std::exception("cannot create thread without entrypoint");
}

ThreadPool* sat::createThreadPool(std::function<void()>&& entrypoint, int stacksize) {
   return new BasicThreadPool(stacksize);
}
