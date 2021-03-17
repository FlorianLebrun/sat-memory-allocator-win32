#include "./index.hpp"

using namespace sat::worker;

Workload::Workload(worker::IWork* work) {
  this->work = work;
}

void Workload::execute() {
   if (WorkContext* context = this->context) {
      _ASSERT(this == context->workloads.current());

      // Execute work and release
      worker::IWork* work = this->work;
      if (work) {
         context->handler->execute(work);
         this->work = 0;
      }

      // Unregister current work and active the next work
      Workload* previous_workload = context->scheduleNextWork();
      _ASSERT(previous_workload == this);
   }
   else {
      // Execute work and release
      if (worker::IWork* work = this->work) {
         work->executeWork(0);
         this->work = 0;
      }
   }
}

void Workload::abort() {
   this->lock.lock();
   if (this->work) {
      this->work->abortWork();
      this->work = 0;
   }
}

WorkloadQueue::WorkloadQueue() {
  this->firstWorkload = 0;
  this->lastWorkload = 0;
}

Workload* WorkloadQueue::current() {
  return (Workload*)this->firstWorkload;
}

void WorkloadQueue::push(Workload* workload) {
  if (this->lastWorkload) this->lastWorkload->nextWorkload = workload;
  else this->firstWorkload = workload;
  this->lastWorkload = workload;
}

Workload* WorkloadQueue::pop() {
  if (Workload* workload = (Workload*)this->firstWorkload) {
    if (!(this->firstWorkload = workload->nextWorkload)) this->lastWorkload = 0;
    workload->nextWorkload = 0;
    return workload;
  }
  return 0;
}

WorkloadQueue::operator bool() {
  return this->firstWorkload != 0;
}
