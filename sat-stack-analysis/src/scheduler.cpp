#include "./scheduler.h"
#include "./win32/system.h"

#include <algorithm>

using namespace sat;

static struct {
   SystemThreading::tThread thread;
   TickWorker* tickWorkers = 0;
   bool enabled = false;
   SpinLock lock;

   void resume() {
      if (!this->enabled) {
         this->enabled = true;
         if (!this->thread.id) {
            this->thread = SystemThreading::CreateThread(&runEventLoop, this);
         }
         else {
            SystemThreading::ResumeThread(&this->thread);
         }
      }
   }

   void pause() {
      if (this->enabled) {
         this->enabled = false;
         SystemThreading::SuspendThread(&this->thread);
      }
   }

   static void __stdcall runEventLoop(void*) {
      printf("[sat sampler] start\n");
      uint64_t period = SystemThreading::GetTimeFrequency() / 10000;
      while (1) {
         SystemThreading::SleepThreadCycle(period);
         scheduler.lock.lock();
         for (auto worker = scheduler.tickWorkers; worker; worker = worker->scheduledNext) {
            worker->execute();
         }
         scheduler.lock.unlock();
      }
      scheduler.thread.tThread::tThread();
      printf("[sat sampler] stop\n");
   }

} scheduler;

sat::TickWorker::TickWorker() {
   this->scheduledNext = 0;
}

void sat::addTickWorker(TickWorker* worker) {
   if (worker) {
      scheduler.lock.lock();
      worker->scheduledNext = scheduler.tickWorkers;
      scheduler.tickWorkers = worker;
      scheduler.resume();
      scheduler.lock.unlock();
   }
}

void sat::removeTickWorker(TickWorker* worker) {
   if (worker) {
      scheduler.lock.lock();
      for (TickWorker** pworker = &scheduler.tickWorkers; *pworker; pworker = &(*pworker)->scheduledNext) {
         if (*pworker == worker) {
            *pworker = worker->scheduledNext;
            break;
         }
      }
      worker->scheduledNext = 0;
      if (!scheduler.tickWorkers) scheduler.pause();
      scheduler.lock.unlock();
   }
}
