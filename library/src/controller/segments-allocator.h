
#ifndef SAT_SegmentsAllocator_h_
#define SAT_SegmentsAllocator_h_

#include "../utils/btree.h"

namespace SAT {
  struct FreeSegmentsTree : public Btree<uint64_t, 4>
  {
    BtNode _init_reserve[3];
    BtNode* reserve;
    FreeSegmentsTree();
    virtual BtNode* allocNode() override;
    virtual void freeNode(BtNode* node) override;
  };

  struct SegmentsAllocator
  {
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

    void traverse() {
      /*if(!spans.size()) printf("(empty)\n");
      for(int i=0;i<spans.size();i++) {
      uintptr_t key= spans[i];
      printf("> address %d - %d %s\n", M_SATFreeKey_GetIndex(key), M_SATFreeKey_GetIndex(key)+M_SATFreeKey_GetSize(key), M_SATFreeKey_GetNotReserved(key)?"(not reserved)":"");
      }*/
    }
  };
}

#endif
