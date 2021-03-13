#include "./index.hpp"

using namespace sat::worker;

WorkContext::WorkContext(WorkThreadPool* workers, WorkContextPool* pool, worker::IContext* handler) {
  this->nextContext = 0;
  this->workers = workers;
  this->handler = handler;
  this->pool = pool;
  this->is_available = false;
}

void WorkContext::destroy() {
  this->lock.lock();
  while(Workload* workload = this->workloads.pop()) {
    workload->abort();
  }
  if(this->handler) {
    this->handler->release();
    this->handler = 0;
  }
  this->lock.unlock();
}

void WorkContext::schedule(Workload* workload) {
  workload->context = this;
  this->lock.lock();
  if (!this->workloads) {
    this->workloads.push(workload);
    this->lock.unlock();
    this->workers->schedule(workload);
  }
  else {
    this->workloads.push(workload);
    this->lock.unlock();
  }
}

Workload* WorkContext::scheduleNextWork() {
  Workload* previous_workload;
  this->lock.lock();
  previous_workload = this->workloads.pop();
  if (Workload* next_workload = this->workloads.current()) {
    this->lock.unlock();
    _ASSERT(next_workload->context == this);
    this->workers->schedule((Workload*)next_workload);
  }
  else {
    this->lock.unlock();
    if (this->pool) {
      this->pool->releaseContext(this);
    }
  }
  return (Workload*)previous_workload;
}

WorkContextStack::WorkContextStack() {
  this->lastContext = 0;
}

void WorkContextStack::push(WorkContext* context) {
  context->nextContext = this->lastContext;
  this->lastContext = context;
}

WorkContext* WorkContextStack::pop() {
  WorkContext* context = (WorkContext*)this->lastContext;
  if (context) this->lastContext = context->nextContext;
  return (WorkContext*)context;
}

WorkContextStack::operator bool() {
  return this->lastContext != 0;
}

WorkContextPool::WorkContextPool(WorkThreadPool* workers, worker::IContextFactory* factory, int nominal_context_count, int maximum_context_count) {
  this->factory = factory;
  this->workers = workers;
  this->nominal_context_count = nominal_context_count;
  this->maximum_context_count = maximum_context_count;
}

void WorkContextPool::releaseContext(WorkContext* context) {
  this->lock.lock();
  _ASSERT(context->is_available == false);

  // When reusable for a pending workload
  if (Workload* workload = this->workloads.pop()) {
    this->lock.unlock();

    // Schedule in thread pool
    _ASSERT(!context->workloads);
    workload->context = context;
    context->workloads.push((Workload*)workload);
    this->workers->schedule((Workload*)workload);
  }
  // Otherwise, keep for future workload
  else {
    context->is_available = true;
    this->available_contexts.push(context);
    this->lock.unlock();
  }
}

WorkContext* WorkContextPool::acquireContext() {
  WorkContext* context = this->available_contexts.pop();
  if (context) {
    _ASSERT(context->is_available == true);
    context->is_available = false;
  }
  else {
    // Create context
    if (this->contexts.size() < this->maximum_context_count) {
      if (factory) {
        if (worker::IContext* handler = this->factory->createContext()) {
          context = new WorkContext(this->workers, this, handler);
        }
      }
      else context = new WorkContext(this->workers, this, 0);
    }

    // Register context when success
    if (context) {
      this->contexts.push_back(context);
    }
  }
  return context;
}

void WorkContextPool::schedule(Workload* workload) {
  this->lock.lock();

  // Try to acquire context
  WorkContext* context = 0;
  if (!this->workloads) {
    context = this->acquireContext();
  }

  // When context, then schedule directly on thread pool
  if (context) {
    this->lock.unlock();

    // Schedule in thread pool
    _ASSERT(!context->workloads);
    workload->context = context;
    context->workloads.push(workload);
    this->workers->schedule(workload);
  }
  // Otherwise, push in pendings workload
  else {
    this->workloads.push(workload);
    this->lock.unlock();
  }
}

void WorkContextPool::destroy() {
  this->lock.lock();
  while(WorkContext* context = this->available_contexts.pop()) {
    context->destroy();
  }
  while(Workload* workload = this->workloads.pop()) {
    workload->abort();
  }
  this->lock.unlock();
}
