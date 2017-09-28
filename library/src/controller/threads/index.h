
#include "../../base.h"

#ifndef _SAT_threads_index_h_
#define _SAT_threads_index_h_

namespace SAT {

  const int cThreadMaxStackBeacons = 1024;

  struct Thread : public SATBasicRealeasable<IThread> {
  private:
    char name[256];

  public:
    Thread* backThread,* nextThread;
    SystemThreading::tThread handle;
    uint64_t id;

    int heapId;
    LocalHeap* local_heap;
    GlobalHeap* global_heap;

    bool isSampled;
    IStackStampAnalyzer* stack_analyzer;
    StackBeacon* beacons[cThreadMaxStackBeacons];
    int beaconsCount;

    Thread(const char* name, uint64_t threadId, int heapId);

    virtual uint64_t getID() override;
    virtual const char* getName() override;
    virtual bool setName(const char*) override;

    virtual uint64_t getStackStamp() override;
    virtual void setStackAnalyzer(IStackStampAnalyzer* stack_analyzer) override;

    virtual void destroy() override;

    uint64_t CaptureThreadCpuTime();
    bool CaptureThreadStackStamp(SAT::IStackStampBuilder& stack_builder, SAT::IStackStampAnalyzer* stack_analyzer);
  };
}

#endif
