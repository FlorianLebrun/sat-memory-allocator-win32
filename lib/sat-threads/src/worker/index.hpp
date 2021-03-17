#pragma once
#include <sat/worker.hpp>
#include <sat/spinlock.hpp>
#include "../threads.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace sat {
   namespace worker {

      struct Workload;
      struct WorkThread;
      struct WorkThreadPool;
      struct WorkContext;
      struct WorkContextPool;

      struct WorkObject : worker::ObjectID {
         WorkObject() { this->id = uint64_t(this); }
      };

      /*******************************************************
      /*
      /* Work stream
      /*
      /*******************************************************/

      struct Workload : WorkObject {
         uint8_t overlapped[32] = { 0 };
         volatile Workload* nextWorkload = 0;
         WorkContext* context = 0;
         worker::IWork* work = 0;
         SpinLock lock;

         Workload(worker::IWork* work);
         void abort();
         void execute();
      };

      struct WorkloadQueue {
         volatile Workload* firstWorkload;
         volatile Workload* lastWorkload;

         WorkloadQueue();
         Workload* current();
         void push(Workload* workload);
         Workload* pop();
         operator bool();
      };

      /*******************************************************
      /*
      /* Work Context
      /*
      /*******************************************************/

      struct WorkContext : WorkObject {
         volatile WorkContext* nextContext;
         SpinLock lock;
         WorkThreadPool* workers;
         WorkContextPool* pool;
         WorkloadQueue workloads;
         worker::IContext* handler;
         bool is_available;

         WorkContext(WorkThreadPool* workers, WorkContextPool* pool, worker::IContext* handler);
         void schedule(Workload* workload);
         Workload* scheduleNextWork();
         void destroy();
      };

      struct WorkContextStack {
         volatile WorkContext* lastContext;

         WorkContextStack();
         void push(WorkContext* context);
         WorkContext* pop();
         operator bool();
      };

      struct WorkContextPool : WorkObject {
         SpinLock lock;
         WorkThreadPool* workers;
         WorkloadQueue workloads;
         std::vector<WorkContext*> contexts;
         WorkContextStack available_contexts;

         worker::IContextFactory* factory;
         int nominal_context_count;
         int maximum_context_count;

         WorkContextPool(WorkThreadPool* workers, worker::IContextFactory* factory, int nominal_context_count, int maximum_context_count);
         WorkContext* acquireContext();
         void releaseContext(WorkContext* context);
         void schedule(Workload* workload);
         void destroy();
      };

      /*******************************************************
      /*
      /* Work threads
      /*
      /*******************************************************/

      struct WorkThreadPool : public impl::BasicThreadPool {
         boost::asio::io_context pipeline;
         boost::asio::executor_work_guard<boost::asio::io_context::executor_type> pipeline_guard;

         int nominal_thread_count;
         int maximum_thread_count;

         WorkThreadPool(int nominal_thread_count, int maximum_thread_count, int stack_size);
         virtual Thread* create(const std::function<void()>&& entrypoint) final;
         void schedule(Workload* workload);
         void run();
      };

      /*******************************************************
      /*
      /* Work pool
      /*
      /*******************************************************/

      struct WorkPoolImpl : worker::WorkPool {
         WorkThreadPool* workers;

         WorkPoolImpl(WorkThreadPool* workers)
            : workers(workers) {
         }
         ~WorkPoolImpl() {
            delete workers;
         }
         virtual worker::ContextPoolID createContextPool(worker::IContextFactory* factory, int nominal_context_count, int maximum_context_count) override {
            WorkContextPool* contextsPool = new WorkContextPool(this->workers, factory, nominal_context_count, maximum_context_count);
            return contextsPool;
         }
         virtual void deleteContextPool(worker::ContextPoolID contextsPoolID) override {
            WorkContextPool* contextsPool = (WorkContextPool*)contextsPoolID.id;
            contextsPool->destroy();
         }
         virtual worker::ContextID createContext(worker::IContext* handler) override {
            WorkContext* context = new WorkContext(this->workers, 0, handler);
            return context;
         }
         virtual void deleteContext(worker::ContextID contextID) override {
            WorkContext* context = (WorkContext*)contextID.id;
            context->destroy();
         }
         virtual bool isWorkingContext(worker::ContextID contextID) override {
            WorkContext* context = (WorkContext*)contextID.id;
            return context->workloads.current() != 0;
         }
         virtual worker::WorkID scheduleWork(worker::IWork* work) override {
            Workload* workload = new Workload(work);
            this->workers->schedule(workload);
            return workload;
         }
         virtual worker::WorkID scheduleWork(worker::IWork* work, worker::ContextID contextID) override {
            WorkContext* context = (WorkContext*)contextID.id;
            Workload* workload = new Workload(work);
            context->schedule(workload);
            return workload;
         }
         virtual worker::WorkID scheduleWork(worker::IWork* work, worker::ContextPoolID contextsPoolID) override {
            WorkContextPool* contextsPool = (WorkContextPool*)contextsPoolID.id;
            Workload* workload = new Workload(work);
            contextsPool->schedule(workload);
            return workload;
         }
         virtual void abortWork(worker::WorkID id) override {

         }
         virtual void foreachContext(worker::ContextPoolID contextsPoolID, std::function<void(worker::IContext*)>&& callback) override {
            WorkContextPool* contextsPool = (WorkContextPool*)contextsPoolID.id;
            SpinLockHolder guard(contextsPool->lock);
            for (WorkContext* context : contextsPool->contexts) {
               callback(context->handler);
            }
         }
         virtual void foreachThread(std::function<void(Thread*)>&& callback) override {
            this->workers->foreach(std::forward<decltype(callback)>(callback));
         }
      };
   }
}
