#include "./scheduler.h"
#include "../win32/system.h"

#include <algorithm>

SATScheduler::TickWorker::TickWorker() {
  this->scheduledNext = 0;
  this->scheduler = 0;
}

SATScheduler::SATScheduler() {
  this->enabled = false;
  this->tickWorkers = 0;
}

void SATScheduler::addTickWorker(TickWorker* worker) {
  if(worker && worker->scheduler == 0) {
    this->lock.lock();
    worker->scheduler = this;
    worker->scheduledNext = this->tickWorkers;
    this->tickWorkers = worker;
    this->resume();
    this->lock.unlock();
  }
}

void SATScheduler::removeTickWorker(TickWorker* worker) {
  if(worker && worker->scheduler == this) {
    this->lock.lock();
    for(TickWorker** pworker =&this->tickWorkers;*pworker; pworker =&(*pworker)->scheduledNext) {
      if(*pworker == worker) {
        *pworker = worker->scheduledNext;
        break;
      }
    }
    worker->scheduledNext = 0;
    worker->scheduler = 0;
    if(!this->tickWorkers) this->pause();
    this->lock.unlock();
  }
}

void SATScheduler::resume() {
  if (!this->enabled) {
    this->enabled = true;
    if (!this->thread.id) {
      this->thread = SystemThreading::CreateThread(&SATScheduler::runEventLoop, this);
    }
    else {
      SystemThreading::ResumeThread(&this->thread);
    }
  }
}

void SATScheduler::pause() {
  if (this->enabled) {
    this->enabled = false;
    SystemThreading::SuspendThread(&this->thread);
  }
}

void __stdcall SATScheduler::runEventLoop(void* arg) {
  SATScheduler* scheduler = (SATScheduler*)arg;
  printf("[SAT sampler] start\n");
  uint64_t period = g_SAT.timeFrequency / 10000;
  while (1) {
    SystemThreading::SleepThreadCycle(period);
    scheduler->lock.lock();
    for(auto worker=scheduler->tickWorkers; worker; worker = worker->scheduledNext) {
      worker->execute();
    }
    scheduler->lock.unlock();
  }
  scheduler->thread.tThread::tThread();
  printf("[SAT sampler] stop\n");
}
