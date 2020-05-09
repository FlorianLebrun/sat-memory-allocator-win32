
#include "../base.h"

#ifndef SAT_descriptor_h_
#define SAT_descriptor_h_

#include "./stack-stamp.h"
#include "./profiler/index.h"
#include "./segments-allocator.h"

namespace SAT {

  typedef union tBuffer64 {
    tBuffer64* next;
    char bytes[64];
  } *tpBuffer64;
  static_assert(sizeof(tBuffer64) == 64, "Bad SAT::tBuffer64 size");

  struct Controller : SAT::IController {

    // Free page properties
    SegmentsAllocator segments_allocator;
    std::atomic<tpBuffer64> buffers64;
    tpBuffer64 buffers64Levels[16];
    tpBuffer64 buffers64Limit;
    SpinLock buffers64Lock;

    bool enableObjectTracing;
    bool enableObjectStackTracing;
    bool enableObjectTimeTracing;

    StackStampDatabase* stackStampDatabase;
    SATSampler sampler;

    uint64_t timeOrigin;
    uint64_t timeFrequency;

    SpinLock heaps_lock;
    GlobalHeap* heaps_list;
    GlobalHeap* heaps_table[256];

    SpinLock threads_lock;
    SAT::Thread* threads_list;

    void initialize();

    // Segment Table allocation
    uintptr_t allocSegmentSpan(uintptr_t size);
    void freeSegmentSpan(uintptr_t index, uintptr_t size);
    uintptr_t reserveMemory(uintptr_t size, uintptr_t alignL2 = 1);
    void commitMemory(uintptr_t index, uintptr_t size);
    void decommitMemory(uintptr_t index, uintptr_t size);
    void printSegments();

    // Heap management
    virtual SAT::IHeap* createHeap(SAT::tHeapType type, const char* name) override final;
    virtual SAT::IHeap* getHeap(int id) override final;

    // Thread management
    virtual SAT::IThread* getThreadById(uint64_t id) override final;
    virtual SAT::IThread* getCurrentThread() override final;
    virtual SAT::IThread* configureCurrentThread(const char* name, int heapId) override final;

    // Analysis
    virtual IProfile* createProfile() override final;
    virtual void traverseObjects(SAT::IObjectVisitor* visitor, uintptr_t target_address) override final;
    virtual void traverseStack(uint64_t stackstamp, IStackVisitor* visitor) override final;
    virtual bool checkObjectsOverflow() override final;

    // Timing
    virtual double getCurrentTime() override final;
    virtual uint64_t getCurrentTimestamp() override final;
    virtual double getTime(uint64_t timestamp) override final;
    virtual uint64_t getTimestamp(double time) override final;

    // Allocator
    void* allocBuffer64(uint32_t level);
    void freeBuffer64(void* ptr, uint32_t level);
    void* allocBuffer(uint32_t size) { return this->allocBuffer64(msb_32(size) + 1); }
    void freeBuffer(void* ptr, uint32_t size) { return this->freeBuffer64(ptr, msb_32(size) + 1); }
  };
}

extern SAT::Controller g_SAT;

#endif
