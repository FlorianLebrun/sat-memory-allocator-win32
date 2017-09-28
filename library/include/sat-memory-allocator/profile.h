#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif


namespace SAT {

  struct IStackHistogram : public IReleasable {
    typedef struct tNode {
      tNode* next;
      tNode* subcalls;
      StackMarker* marker;
      int hits;
      tNode(StackMarker* _marker = 0): next(0), hits(0), subcalls(0), marker(_marker) {
      }
    } *tpNode;
    tNode root;
    virtual void print() = 0;
  };

  struct IProfile : IReleasable {
    virtual void addThreadCallStack(IThread* thread) = 0;
    virtual void addThreadCpu(IThread* thread) = 0;
    virtual void addHeapSize(IHeap* heap) = 0;
    virtual void addProcessCpu() = 0;

    virtual void removeThread(IThread* thread) = 0;
    virtual void removeHeap(IHeap* heap) = 0;
    virtual void removeProcess() = 0;

    virtual void startProfiling() = 0;
    virtual void stopProfiling() = 0;

    virtual IStackHistogram* computeCallStackHistogram(IThread* thread = 0, double startTime = -1.0, double endTime = -1.0) = 0;
  };

}
