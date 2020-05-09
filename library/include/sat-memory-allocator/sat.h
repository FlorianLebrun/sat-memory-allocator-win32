#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

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
    tObjectMetaData(uint64_t bits = 0) { this->bits = bits; }
  } *tpObjectMetaData;
  static_assert(sizeof(tObjectMetaData) == sizeof(uint64_t), "tObjectMetaData bad size");

  typedef struct tObjectInfos {
    uint8_t heapID;
    size_t size;
    uintptr_t base;
    tObjectMetaData meta;
    tObjectStamp stamp;

    tObjectInfos& set(int heapID, uintptr_t base, size_t size, uint64_t meta = 0) {
      this->heapID = heapID;
      this->stamp = tObjectStamp(0, 0);
      this->meta.bits = meta;
      this->base = base;
      this->size = size;
      return *this;
    }
    bool checkOverflow() {
      if(this->meta.isOverflowProtected) {

        // Read and check padding size
        uint32_t bufferSize = *(uint32_t*)(this->base+this->size-sizeof(uint32_t));
        bufferSize ^= 0xabababab;
        if(bufferSize > this->size) return false;

        // Read and check padding bytes
        uint8_t* bytes = (uint8_t*)this->base;
        for(int i=this->size-sizeof(uint32_t)-1;i>=bufferSize;i--) {
          if(bytes[i] != 0xab) {
            return false;
          }
        }
      }
      return true;
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
    virtual IProfile* createProfile() = 0;
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
