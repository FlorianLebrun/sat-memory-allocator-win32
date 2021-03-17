#pragma once
#include <sat/thread.hpp>
#include <sat/spinlock.hpp>
#include <boost/thread.hpp>

#include <sat/memory.hpp>

namespace sat {
   namespace impl {

      class BasicThread;
      class BasicThreadPool;
      class DefaultThread;
      class DefaultThreadPool;

      class BasicThread : public shared::SharedObject<sat::Thread> {
      public:
         BasicThreadPool* pool;
         boost::thread thread;
         BasicThread(BasicThreadPool* pool, std::function<void()>&& entrypoint);
         virtual uint64_t getID() final;
         virtual uintptr_t getNativeHandle() final;
      };

      class BasicThreadPool : public shared::SharedObject<sat::ThreadPool> {
         friend BasicThread;
      protected:
         SpinLock lock;
         std::vector<BasicThread*> threads;
         boost::thread::attributes attrs;
      public:
         BasicThreadPool(int stacksize = 0);
         virtual ~BasicThreadPool();
         virtual void foreach(std::function<void(Thread*)>&& callback) override final;
         virtual Thread* create(std::function<void()>&& entrypoint) override;
      };

      class DefaultThread : public shared::SharedObject<sat::Thread> {
      public:
         DefaultThreadPool* pool;
         void* threadHandle;
         uint64_t threadId;
         DefaultThread();
         ~DefaultThread();
         void* operator new(size_t size) {
            return sat::memory::table->allocBuffer(sizeof(DefaultThread));
         }
         virtual uint64_t getID() final;
         virtual uintptr_t getNativeHandle() final;
      };

      class DefaultThreadPool : public shared::SharedObject<sat::ThreadPool> {
         friend DefaultThread;
      protected:
         SpinLock lock;
         std::vector<DefaultThread*> threads;
      public:
         virtual void foreach(std::function<void(Thread*)>&& callback) override final;
         virtual Thread* create(std::function<void()>&& entrypoint) override;
      };
   }
}

