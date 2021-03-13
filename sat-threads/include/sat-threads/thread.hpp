#pragma once
#include "./shared.hpp"
#include <string>
#include <thread>
#include <functional>

namespace sat {

   class Thread : public shared::ISharedObject {
   public:

      enum ThreadObjectID {
         GLOBAL_HEAP = 0,     // sat::mem::GlobalHeap
         LOCAL_HEAP = 1,      // sat::mem::LocalHeap
         STACK_TRACKER = 2,   // sat::stack_analysis::ThreadStackTracker
         WORKPOOL = 3,        // sat::worker::IWorkPool
         MaxObjectCount = 16,
      };
      int heapId;

      virtual std::thread::id getID() = 0;

      static Thread* current();
      
      template <typename T>
      T* getObject() { return (T*)this->objects[T::ThreadObjectID]; }
      template <typename T>
      void setObject(T* object) { this->objects[T::ThreadObjectID] = object; }

   protected:
      shared::ISharedObject* objects[MaxObjectCount] = { 0 };
   };

   class ThreadPool : public shared::ISharedObject {
   public:
      virtual Thread* create(std::function<void()>&& entrypoint = nullptr) = 0;
      virtual void foreach(std::function<void(Thread*)>&& callback) = 0;
   };

   extern ThreadPool* createThreadPool(std::function<void()>&& entrypoint = nullptr, int stacksize = 0);
}
