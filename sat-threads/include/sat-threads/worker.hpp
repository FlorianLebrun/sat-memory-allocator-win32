#pragma once
#include "./thread.hpp"
#include <functional>

namespace sat {
   namespace worker {

      struct IWork;
      struct IContext;
      struct IContextFactory;

      struct ObjectID { uint64_t id; ObjectID() :id(0) {} operator bool() { return this->id != 0; } };
      struct WorkID : ObjectID { WorkID() {} WorkID(ObjectID* x) { this->id = x ? x->id : 0; } };
      struct ContextID : ObjectID { ContextID() {} ContextID(ObjectID* x) { this->id = x ? x->id : 0; } };
      struct ContextPoolID : ObjectID { ContextPoolID() {} ContextPoolID(ObjectID* x) { this->id = x ? x->id : 0; } };

      struct IWork {
         virtual void executeWork(IContext* context) = 0;
         virtual void abortWork() = 0;
      };

      struct IContext {
         virtual void execute(IWork* work) = 0;
         virtual void release() = 0;
      };

      struct IContextFactory {
         virtual IContext* createContext() = 0;
      };

      class WorkPool : public shared::SharedObject<> {
      public:
         virtual ContextPoolID createContextPool(IContextFactory* factory, int nominal_context_count, int maximum_context_count) = 0;
         virtual void deleteContextPool(ContextPoolID id) = 0;

         virtual ContextID createContext(IContext* context) = 0;
         virtual void deleteContext(ContextID id) = 0;
         virtual bool isWorkingContext(ContextID) = 0;

         virtual WorkID scheduleWork(IWork* work) = 0;
         virtual WorkID scheduleWork(IWork* work, ContextID containerID) = 0;
         virtual WorkID scheduleWork(IWork* work, ContextPoolID containerID) = 0;
         virtual void abortWork(WorkID id) = 0;

         virtual void foreachContext(ContextPoolID contextsPool, std::function<void(IContext*)>&& callback) = 0;
         virtual void foreachThread(std::function<void(Thread*)>&& callback) = 0;
      };

      extern WorkPool* createWorkPool(int nominal_thread_count, int maximum_thread_count, int stack_size = 0);
   };
}

