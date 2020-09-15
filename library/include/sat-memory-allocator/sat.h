#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

#include <atomic>

namespace SAT {

  enum class tHeapType {
    COMPACT_HEAP_TYPE,
  };

  typedef struct tObjectMetaData {
    static const uint64_t cIsReferer_Bit = uint64_t(1) << 32;
    static const uint64_t cIsStamped_Bit = uint64_t(1) << 33;
    static const uint64_t cIsOverflowProtected_Bit = uint64_t(1) << 34;
    static const uint64_t cTypeID_Mask = 0xffffffff;
    union {
      struct {
        uint32_t typeID;
        unsigned short isReferer : 1; // true, when the object has reference to other data
        unsigned short isStamped : 1; // true, when the object has been stamped by a tracker
        unsigned short isOverflowProtected : 1; // true, when the object has canary words at buffer end
        unsigned short __bit_reserve_0 : 13;
        uint16_t numRef; // ref counter for cached object (avoid releasing of cached object)
      };
      uint64_t bits;
    };
    tObjectMetaData(uint64_t bits = 0) {
      this->bits = bits;
    }
    void lock() {
      ((std::atomic_int16_t*)&this->numRef)->fetch_add(1);
    }
    void unlock() {
      ((std::atomic_int16_t*)&this->numRef)->fetch_sub(1);
    }
  } *tpObjectMetaData;
  static_assert(sizeof(tObjectMetaData) == sizeof(uint64_t), "tObjectMetaData bad size");
  
  typedef struct tObjectInfos {
    uint8_t heapID;
    size_t size;
    uintptr_t base;
    tpObjectMetaData meta;

    tObjectInfos& set(int heapID, uintptr_t base, size_t size, uint64_t* meta = 0) {
      this->heapID = heapID;
      this->meta = tpObjectMetaData(meta);
      this->base = base;
      this->size = size;
      return *this;
    }
    tpObjectStamp getStamp() {
       if (!this->meta->isStamped) return 0;
       else return tpObjectStamp(this->base + this->size - sizeof(tObjectStamp));
    }
    void* detectOverflow() {
      if(this->meta && this->meta->isOverflowProtected) {
        uint32_t paddingEnd = this->size-sizeof(uint32_t);
        if (this->meta->isStamped) paddingEnd -= sizeof(tObjectStamp);

        // Read and check padding size
        uint32_t* pBufferSize = (uint32_t*)(this->base+paddingEnd);
        uint32_t bufferSize = (*pBufferSize) ^ 0xabababab;
        if(bufferSize > this->size) {
          return pBufferSize;
        }

        // Read and check padding bytes
        uint8_t* bytes = (uint8_t*)this->base;
        for(int i=bufferSize;i<paddingEnd;i++) {
          if(bytes[i] != 0xab) {
            return &bytes[i];
          }
        }
      }
      return 0;
    }
  } *tpObjectInfos;

  struct IObjectVisitor {
    virtual bool visit(tpObjectInfos obj) = 0;
  };

  struct IObjectAllocator {

    virtual size_t getMaxAllocatedSize() = 0;
    virtual size_t getMinAllocatedSize() = 0;
    virtual size_t getAllocatedSize(size_t size) = 0;
    virtual void* allocate(size_t size) = 0;

    virtual size_t getMaxAllocatedSizeWithMeta() = 0;
    virtual size_t getMinAllocatedSizeWithMeta() = 0;
    virtual size_t getAllocatedSizeWithMeta(size_t size) = 0;
    virtual void* allocateWithMeta(size_t size, uint64_t meta) = 0;
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
