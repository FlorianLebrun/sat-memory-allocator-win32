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
    static const uint64_t cTypeID_Mask = 0xffffffff;
    union {
      struct {
        uint32_t typeID;
        unsigned short isReferer : 1; // true, when the object has reference to other data
        unsigned short isStamped : 1; // true, when the object has been stamped by a tracker
        unsigned short __bit_reserve_0 : 14;
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

    tObjectInfos& set(int heapID, uintptr_t base, size_t size, uint64_t meta) {
      this->heapID = heapID;
      this->stamp = tObjectStamp(0, 0);
      if (this->meta.bits = meta) {
        if (this->meta.isStamped) {
          this->stamp = *tpObjectStamp(base);
          base += sizeof(SAT::tObjectStamp);
          size -= sizeof(uint64_t);
        }
      }
      this->base = base;
      this->size = size;
      return *this;
    }
  } *tpObjectInfos;

  struct IObjectVisitor {
    virtual bool visit(tpObjectInfos obj) = 0;
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

  struct IHeap : IReleasable {

    // Basic infos
    virtual int getID() = 0;
    virtual const char* getName() = 0;

    // Allocation
    virtual void* allocBuffer(size_t size, SAT::tObjectMetaData meta) = 0;
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

    // Timing
    virtual double getCurrentTime() = 0;
    virtual uint64_t getCurrentTimestamp() = 0;
    virtual double getTime(uint64_t timestamp) = 0;
    virtual uint64_t getTimestamp(double time) = 0;
  };

}
