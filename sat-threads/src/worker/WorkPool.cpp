#include "./index.hpp"

using namespace sat;
using namespace sat::impl;
using namespace sat::worker;

WorkThreadPool::WorkThreadPool(int nominal_thread_count, int maximum_thread_count, int stack_size)
   : BasicThreadPool(stack_size), pipeline_guard(boost::asio::make_work_guard(pipeline))
{
   this->nominal_thread_count = nominal_thread_count;
   this->maximum_thread_count = maximum_thread_count;
   for (int i = 0; i < this->maximum_thread_count; i++) this->create(nullptr);
}

Thread* WorkThreadPool::create(const std::function<void()>&& entrypoint) {
   if (!entrypoint) {
      SpinLockHolder guard(this->lock);
      if (this->threads.size() < maximum_thread_count) {
         auto thread = new BasicThread(this, std::bind(&WorkThreadPool::run, this));
         this->threads.push_back(thread);
         return thread;
      }
   }
   throw std::exception("cannot create thread with custom entrypoint");
}

void WorkThreadPool::schedule(Workload* workload) {
   boost::asio::post(this->pipeline, boost::bind(&Workload::execute, workload));
}

void WorkThreadPool::run() {
   while (!this->pipeline.stopped()) {

      // Do one work
      this->pipeline.run_one();

      // Flush windows message after work
      MSG msg;
      while (PeekMessage((LPMSG)&msg, NULL, 0, 0, true)) {
         TranslateMessage((LPMSG)&msg);
         DispatchMessage((LPMSG)&msg);
      }
   }
}

WorkPool* sat::worker::createWorkPool(int nominal_thread_count, int maximum_thread_count, int stack_size) {
  return new WorkPoolImpl(new WorkThreadPool(nominal_thread_count, maximum_thread_count, stack_size));
}
