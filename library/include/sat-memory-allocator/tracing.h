#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

namespace SAT {
  
  /**************************************************
  ** Stack tracing
  **************************************************/

  struct IStackMarker {
    virtual void getSymbol(char* buffer, int size) = 0;
  };

  struct StackMarker : IStackMarker {
    uint64_t word64[3];
    template<typename MarkerT>
    MarkerT* as() {
      _ASSERT(sizeof(typename MarkerT) <= sizeof(StackMarker));
      this->word64[0] = 0;
      this->word64[1] = 0;
      this->word64[2] = 0;
      return new(this) typename MarkerT();
    }
    intptr_t compare(StackMarker& other) {
      intptr_t c;
      if(c = other.word64[0] - this->word64[0]) return c;
      if(c = other.word64[-1] - this->word64[-1]) return c;
      if(c = other.word64[1] - this->word64[1]) return c;
      if(c = other.word64[2] - this->word64[2]) return c;
      return 0;
    }
    void copy(StackMarker& other) {
      this->word64[-1] = other.word64[-1];
      this->word64[0] = other.word64[0];
      this->word64[1] = other.word64[1];
      this->word64[2] = other.word64[2];
    }
    virtual void getSymbol(char* buffer, int size) override final {
      buffer[0] = 0;
    }
  };
  static_assert(sizeof(StackMarker) == 4*sizeof(uint64_t), "StackMarker size invalid");

  struct StackBeacon {
    StackBeacon *parentBeacon;
    virtual void createrMarker(StackMarker& buffer) = 0;
  };

  template<class StackBeaconT>
  struct StackBeaconHolder {
    bool isDeployed;
    StackBeaconT beacon;
    StackBeaconHolder() {
      this->isDeployed = false;
    }
    ~StackBeaconHolder() {
      this->remove();
    }
    void deploy() {
      if(!this->isDeployed) {
        sat_begin_stack_beacon(&this->beacon);
        this->isDeployed = true;
      }
    }
    void remove() {
      if(this->isDeployed) {
        sat_end_stack_beacon(&this->beacon);
        this->isDeployed = false;
      }
    }
  };

  struct IStackStampBuilder {
    virtual void push(SAT::StackMarker& marker) = 0;
  };

  struct IStackStampAnalyzer {
    virtual bool analyze(IStackStampBuilder& builder, StackBeacon* stack, int stackLen) = 0;
  };

  struct IStackVisitor {
    virtual void visit(StackMarker& marker) = 0;
  };

  /**************************************************
  ** Object tracing
  **************************************************/

  typedef struct tObjectStamp {
    uint64_t stackstamp;
    uint64_t timestamp;
    tObjectStamp(uint64_t timestamp = 0, uint64_t stackstamp = 0) {
      this->stackstamp = stackstamp;
      this->timestamp = timestamp;
    }
  } *tpObjectStamp;
  
}
