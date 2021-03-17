#include <sat/worker.hpp>
#include <atomic>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

std::atomic<int> PayloadCounter = 0;

using namespace sat;
using namespace std::chrono;

worker::WorkPool* workPool = sat::worker::createWorkPool(1, 8);

struct MyContext : worker::IContext {
   virtual void execute(worker::IWork* work) override {
      work->executeWork(this);
   }
   virtual void release() override {
   }
};

struct MyContextFactory : worker::IContextFactory {
   virtual worker::IContext* createContext() override {
      return new MyContext();
   }
};

MyContextFactory contextsFactory;

struct MyWork : worker::IWork {
   int64_t id;
   int64_t time;
   worker::ContextPoolID contextsPool;
   worker::ContextID context;
   MyWork(int time) {
      this->id = PayloadCounter++;
      this->time = time;
   }
   virtual void executeWork(worker::IContext* context) override {
      auto start = high_resolution_clock::now();
      if (this->time) {
         for (int64_t i = this->time * 100000; i > 0; i--);
         std::this_thread::sleep_for(milliseconds(this->time));
      }
      auto end = high_resolution_clock::now();
      printf("payload %ld on %d -- %lg s\n", this->id, std::this_thread::get_id(), duration<double>(end - start).count());
   }
   virtual void abortWork() override {
   }
   void send() {
      if (this->context) workPool->scheduleWork(this, this->context);
      else if (this->contextsPool) workPool->scheduleWork(this, this->contextsPool);
      else workPool->scheduleWork(this);
   }
};


void main() {
   if (1) {
      worker::ContextPoolID contextsPool = workPool->createContextPool(&contextsFactory, 1, 4);
      for (int i = 0; i < 100; i++) {
         MyWork* work = new MyWork(rand() % 100);
         work->contextsPool = contextsPool;
         workPool->scheduleWork(work, contextsPool);
      }
   }
   if (1) {
      worker::ContextID context = workPool->createContext(contextsFactory.createContext());
      for (int i = 0; i < 100; i++) {
         MyWork* work = new MyWork(rand() % 100);
         work->context = context;
         workPool->scheduleWork(work, context);
      }
   }
   std::cout << "\n------ all payload emitted -------\n";
   while (1) std::this_thread::sleep_for(milliseconds(100));
   workPool->release();
}