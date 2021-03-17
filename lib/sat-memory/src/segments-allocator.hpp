#pragma once
#include <sat/memory.hpp>
#include <sat/spinlock.hpp>
#include "./btree.h"

namespace sat {

  struct FreeSegmentsTree : public Btree<uint64_t, 4> {
    BtNode _init_reserve[3];
    BtNode* reserve;
    FreeSegmentsTree();
    virtual BtNode* allocNode() override;
    virtual void freeNode(BtNode* node) override;
  };

  struct SegmentsAllocator {
  private:
    FreeSegmentsTree freespans;
    SpinLock freelist_lock;

    uintptr_t allocated_segments;

    uintptr_t analyzeNotReservedSpan(uintptr_t index, uintptr_t length); // return new total free length when span has been split

  public:
    SegmentsAllocator();
    uintptr_t allocSegments(uintptr_t size, uintptr_t alignL2 = 1);
    void freeSegments(uintptr_t index, uintptr_t size);
    void appendSegments(uintptr_t index, uintptr_t size);
  };
}

