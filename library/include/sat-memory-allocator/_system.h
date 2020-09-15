#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

namespace SAT {

   enum class tHeapType {
      COMPACT_HEAP_TYPE,
   };

   struct IThread : IReleasable {

      // Basic infos
      virtual uint64_t getID() = 0;
      virtual const char* getName() = 0;
      virtual bool setName(const char*) = 0;

      // Stack Analysis
      virtual uint64_t getStackStamp() = 0;
      virtual void setStackAnalyzer(IStackStampAnalyzer* stack_analyzer) = 0;
   };

   struct IHeap : IReleasable, IObjectAllocator {

      // Basic infos
      virtual int getID() = 0;
      virtual const char* getName() = 0;

      // Allocation
      virtual uintptr_t acquirePages(size_t size) = 0;
      virtual void releasePages(uintptr_t index, size_t size) = 0;
   };

   struct IController {

      // Thread management
      virtual IThread* getThreadById(uint64_t id) = 0;
      virtual IThread* getCurrentThread() = 0;
      virtual IThread* configureCurrentThread(const char* name, int heapId) = 0;

      // Heap management
      virtual IHeap* createHeap(tHeapType type, const char* name = 0) = 0;
      virtual IHeap* getHeap(int id) = 0;

      // Analysis
      virtual IStackProfiler* createStackProfiler() = 0;
      virtual void traverseObjects(IObjectVisitor* visitor, uintptr_t start_address = 0) = 0;
      virtual void traverseStack(uint64_t stackstamp, IStackVisitor* visitor) = 0;
      virtual bool checkObjectsOverflow() = 0;

      // Timing
      virtual double getCurrentTime() = 0;
      virtual uint64_t getCurrentTimestamp() = 0;
      virtual double getTime(uint64_t timestamp) = 0;
      virtual uint64_t getTimestamp(double time) = 0;
   };

}
