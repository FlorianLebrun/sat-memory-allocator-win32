#include <sat-memory-allocator/sat-memory-allocator.h>
#include <windows.h>
#include "./utils.h"

struct AllocTest {
  float meanTime;
  int sizeMin;
  int sizeMax;
  int testFreeFrequency;
  int num_cycle;

  virtual AllocTest* Duplicate() = 0;
  virtual intptr_t AllocBuffer(int size) = 0;
  virtual void FreeBuffer(intptr_t buf) = 0;
  virtual void StartInfos() = 0;
  virtual void EndInfos() {
    printf("> time = %g ns\n", this->meanTime);
  }

  AllocTest() {
    this->sizeMin = 1;
    this->sizeMax = 65536;
    this->testFreeFrequency = 3;
    this->num_cycle = 4;
  }
  void Run(int sizeMin = 1, int sizeMax = 65536) {
    this->sizeMin = sizeMin;
    this->sizeMax = sizeMax;
    this->StartInfos();
    this->RunLoop();
    this->EndInfos();
  }
  __declspec(noinline) void RunLoop() {
    intptr_t count = 500000;
    intptr_t* buffers = new intptr_t[count];
    intptr_t buffersCount = 0;

    Chrono c;
    c.Start();

    for (int cycle = 0; cycle < num_cycle; cycle++) {
      //printf("cycle %d: alloc\n", cycle);
      int sizeDelta = sizeMax - sizeMin;
      if (sizeDelta <= 0)sizeDelta = 1;
      for (intptr_t i = 0; i < count; i++) {
        int size = sizeMin + fastrand() % sizeDelta;

        intptr_t& buf = buffers[buffersCount];
        buf = this->AllocBuffer(size);
        memset((void*)buf, 0, size);
        if (buf) buffersCount++;
        else printf("not enough memory\n");

        if (testFreeFrequency) {
          if ((fastrand() % testFreeFrequency) == 0) {
            if (buffersCount) {
              int i = fastrand() % buffersCount;
              intptr_t buf = buffers[i];
              buffers[i] = buffers[--buffersCount];
              this->FreeBuffer(buf);
            }
          }
        }
      }

      //printf("cycle %d: purge\n", cycle);
      for (int i = 0; i < buffersCount; i++) {
        this->FreeBuffer(buffers[i]);
      }
      buffersCount = 0;
    }

    this->meanTime = c.GetDiffFloat(Chrono::NS) / float(count*num_cycle);
  }
};

template <int numThread>
struct MultiThreadAllocTest {
  void Run(AllocTest* test, int sizeMin = 1, int sizeMax = 65536) {
    test->StartInfos();

    AllocTest* threadTests[numThread];
    for (int i = 0; i < numThread; i++) {
      threadTests[i] = test->Duplicate();
      threadTests[i]->sizeMin = sizeMin;
      threadTests[i]->sizeMax = sizeMax;
      //threadTests[i]->StartInfos();
    }

    HANDLE hThreads[numThread];
    for (int i = 0; i < numThread; i++) {
      DWORD ThreadID;
      hThreads[i] = CreateThread(NULL, 0, MultiThreadAllocTest::StaticThreadStart, (void*)threadTests[i], 0, &ThreadID);
    }
    WaitForMultipleObjects(numThread, hThreads, true, -1);

    float mean_times = 0;
    for (int i = 0; i < numThread; i++) {
      mean_times += threadTests[i]->meanTime;
      //threadTests[i]->EndInfos();
      delete threadTests[i];
    }
    mean_times /= numThread;
    printf("> MultiThread time = %g ns\n", mean_times);
  }
  static DWORD WINAPI StaticThreadStart(void* Param) {
    ((AllocTest*)Param)->RunLoop();
    return 0;
  }
};

#ifdef USE_TCMALLOC
extern"C" void* tc_malloc(size_t size);
extern"C" void tc_free(void* ptr);
extern"C" void InitSystemAllocators(void);
extern "C" void _tcmalloc();

struct TCmallocAllocTest : AllocTest {
  TCmallocAllocTest() {
  }
  virtual AllocTest* Duplicate() override {
    return new TCmallocAllocTest();
  }
  virtual intptr_t AllocBuffer(int size) override {
    return (intptr_t)::tc_malloc(size);
  }
  virtual void FreeBuffer(intptr_t buf) override {
    ::tc_free((void*)buf);
  }
  virtual void StartInfos() override {
    printf("-------- TCmalloc alloc --------\n");
  }
  virtual void EndInfos() override {
    this->AllocTest::EndInfos();
  }
};
#endif

struct DefaultAllocTest : AllocTest {
  DefaultAllocTest() {
  }
  virtual AllocTest* Duplicate() override {
    return new DefaultAllocTest();
  }
  virtual intptr_t AllocBuffer(int size) override {
    return (intptr_t)::malloc(size);
  }
  virtual void FreeBuffer(intptr_t buf) override {
    ::free((void*)buf);
  }
  virtual void StartInfos() {
    printf("-------- Default alloc --------\n");
  }
  virtual void EndInfos() override {
    this->AllocTest::EndInfos();
  }
};

struct SATmallocAllocTest : AllocTest {
  SATmallocAllocTest() {
  }
  SATmallocAllocTest(SATmallocAllocTest* other) {

  }
  virtual AllocTest* Duplicate() override {
    return new SATmallocAllocTest(this);
  }
  virtual intptr_t AllocBuffer(int size) override {
    return (intptr_t)sat_malloc(size);
  }
  virtual void FreeBuffer(intptr_t ptr) override {
    sat_free((void*)ptr);
  }
  virtual void StartInfos() override {
    printf("-------- sat-malloc alloc --------\n");
  }
  virtual void EndInfos() override {
    this->AllocTest::EndInfos();
    //g_SAT.printSegments();
  }
};
