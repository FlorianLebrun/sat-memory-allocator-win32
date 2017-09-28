
#include "../../base.h"

#ifndef _SAT_profiler_h_
#define _SAT_profiler_h_

struct SATSampler;
struct SATProfile;
struct SATStream;

#include "./stream.h"

struct SATThreadCallStackStream : SATStream {
  typedef struct Sample : SATStreamRecord {
    uint64_t stackstamp;
  } *tpSample;
  SATThreadCallStackStream(SAT::IThread* thread);
  void computeHistogram(StackHistogram* histogram, uint64_t start, uint64_t end);
  virtual void emit() override;
};

struct SATProfile : public SATBasicRealeasable<SAT::IProfile> {
  SATProfile* samplerNext;
  SATSampler* sampler;
  SATStream* streams;
  SpinLock lock;

  void addStream(SATStream* stream);
  void removeStream(SATStream* stream);

  virtual void addThreadCallStack(SAT::IThread* thread) override {
    this->addStream(new SATThreadCallStackStream(thread));
  }
  virtual void addThreadCpu(SAT::IThread* thread) override {
  }
  virtual void addHeapSize(SAT::IHeap* heap) override {
  }
  virtual void addProcessCpu() override {
  }
  virtual void removeThread(SAT::IThread* thread) override {
  }
  virtual void removeHeap(SAT::IHeap* heap) override {
  }
  virtual void removeProcess() override {
  }
  virtual SAT::IStackHistogram* computeCallStackHistogram(SAT::IThread* thread, double startTime, double endTime) override;

  virtual void startProfiling() override;
  virtual void stopProfiling() override;
  
  virtual void destroy() override;
};

class SATSampler {
public:
  SATSampler();
  void addProfile(SATProfile* profile);
  void removeProfile(SATProfile* profile);

private:
  SystemThreading::tThread thread;
  SATProfile* profiles;
  SpinLock lock;
  bool enabled;

  static void __stdcall _SamplerThreadEntry(void* arg);
  void resume();
  void pause();
};

#endif

