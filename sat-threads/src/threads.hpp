#pragma once
#include <sat-threads/thread.hpp>
#include <sat-threads/spinlock.hpp>
#include <boost/thread.hpp>

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
         virtual std::thread::id getID() final {
            //static_assert(sizeof(std::thread::id) != sizeof(boost::thread::id), "");
            //return (std::thread::id&)this->thread.get_id();
            return std::thread::id();
         }
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
         boost::thread::id threadId;
         DefaultThread();
         virtual std::thread::id getID() final {
            //static_assert(sizeof(std::thread::id) != sizeof(boost::thread::id), "");
            //return (std::thread::id&)this->thread.get_id();
            return std::thread::id();
         }
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

