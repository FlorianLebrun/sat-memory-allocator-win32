
#include "../base.h"

#ifndef _SAT_profiler_h_
#define _SAT_profiler_h_

struct SATStackProfiling : public SATBasicRealeasable<SAT::IStackProfiling> {
  typedef SATStackNodeManifold<tData> StackManifold;
  typedef SAT::StackNode<tData> StackNode;
  StackManifold manifold;

  SATStackProfiling();
  virtual SAT::IStackProfiling::Node getRoot() override;
  virtual void print() override;
  virtual void destroy() override;
};

struct SATProfile : public SATScheduler::TickWorker, public SATBasicRealeasable<SAT::IStackProfiler> {

  SAT::Thread* target;
  SATStackProfiling* profiling;
  uint64_t lastSampleTime;

  SATProfile();

  virtual void execute() override;

  virtual SAT::IStackProfiling* flushProfiling() override;
  virtual void startProfiling(SAT::IThread* thread) override;
  virtual void stopProfiling() override;
  
  virtual void destroy() override;
};

#endif

