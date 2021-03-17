#include "../base.h"
#include "./index.h"

#include <vector>
#include "../chrono.h"

extern"C" void* tc_malloc(size_t size);
extern"C" void tc_free(void* ptr);
extern"C" void InitSystemAllocators(void);
extern "C" void _tcmalloc();

struct tTest {
  ScaledBuddySubAllocator::GlobalHeap globalHeap;
  ScaledBuddySubAllocator::LocalHeap localHeap;

  tTest()
    : globalHeap()
    , localHeap(&globalHeap)
  {
  }
  void freePtr(void* obj)
  {
    intptr_t ptr = intptr_t(obj);
    tSATEntry* entry = g_SAT.get(ptr >> sat::SegmentSizeL2);
    if (entry->id == tSATEntryID::PAGE_SCALED_BUDDY_1) localHeap.freePtr(entry, ptr);
  }

  void test_ScaledBuddySubAllocator()
  {
    using namespace ScaledBuddySubAllocator;
    int count = 1000000;
    Chrono c;
    std::vector<Object> objects(count);
    c.Start();
    for (int i = 0; i < count; i++) {
      //printf("%d\n", i);
      objects[i] = localHeap.allocObject(7);
    }
    //g_SAT.printSegments();
    for (int i = 0; i < count; i++) {
      //printf("%d\n", i);
      freePtr(objects[i]);
    }
    localHeap.flushCache();
    globalHeap.flushCache();
    printf("time %g ns\n", c.GetDiffFloat(Chrono::NS) / float(count));
    g_SAT.printSegments();
  }
  void test_tcmalloc() {
    using namespace ScaledBuddySubAllocator;
    int count = 1000000;
    Chrono c;
    std::vector<void*> objects(count);
    ::_tcmalloc();
    c.Start();
    for (int i = 0; i < count; i++) {
      objects[i] = ::tc_malloc(256);
    }
    for (int i = 0; i < count; i++) {
      //::tc_free(objects[i]);
    }
    printf("time %g ns\n", c.GetDiffFloat(Chrono::NS) / float(count));
  }
};

void main()
{
  tTest test;
  //test.test_tcmalloc();
  test.test_ScaledBuddySubAllocator();
  getchar();
}