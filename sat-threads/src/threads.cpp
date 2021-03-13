#include "./threads.hpp"

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
   : pool(&root.defaultPools), threadId(boost::this_thread::get_id()) {
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
      this->threads.push_back(thread);
      return  ::current_thread = thread;
   }
   throw std::exception("cannot create current thread when already exists");
}

BasicThread::BasicThread(BasicThreadPool* pool, std::function<void()>&& entrypoint)
   : pool(pool), thread(pool->attrs, entrypoint) {
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
