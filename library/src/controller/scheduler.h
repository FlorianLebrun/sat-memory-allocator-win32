
#include "../base.h"

#ifndef _SAT_Scheduler_h_
#define _SAT_Scheduler_h_

class SATScheduler {
public:

  class TickWorker {
    friend class SATScheduler;
    TickWorker* scheduledNext;
    SATScheduler* scheduler;
  public:
    TickWorker();
    virtual void execute() = 0;
  };

  SATScheduler();

  void addTickWorker(TickWorker* worker);
  void removeTickWorker(TickWorker* worker);

private:
  SystemThreading::tThread thread;
  TickWorker* tickWorkers;
  SpinLock lock;
  bool enabled;

  static void __stdcall runEventLoop(void* arg);
  void resume();
  void pause();
};

#endif

