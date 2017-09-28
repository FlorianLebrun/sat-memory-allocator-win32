#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

namespace SAT {
  struct tEntryID {
    enum _ {

      /// Heap SEGMENTS
      ///---------------

      PAGE_ZONED_BUDDY,
      PAGE_SCALED_BUDDY_3,
      PAGE_SCALED_BUDDY_5,
      PAGE_SCALED_BUDDY_7,
      PAGE_SCALED_BUDDY_9,
      PAGE_SCALED_BUDDY_11,
      PAGE_SCALED_BUDDY_13,
      PAGE_SCALED_BUDDY_15,
      LARGE_OBJECT,

      /// System SEGMENTS
      ///-----------------

      SAT_SEGMENT = 0x40, // SAT table segment
      FREE, // Available segment
      FORBIDDEN, // Forbidden segment (not reservable by allocator)

      // Tracing page
      STREAM_SEGMENT, // Segment for sampling stream
      STACKSTAMP_DATABASE, // Segment for stackstamp tracing
      MARKING_DATABASE, // Segment for marking

      // Other page
      TYPES_DATABASE,
      PAGE_DESCRIPTOR,

      NOT_COMMITED = 0x80,
    };
    static bool isHeapSegment(int id) {
      return id < SAT_SEGMENT;
    }
  };

  const uintptr_t cMemorySAT_MaxSpanClass = 128;

  typedef struct tDescriptorEntry {
    uint8_t id; // = SAT_DESCRIPTOR
    uint8_t bytesPerAddress; // Bytes per address (give the sizeof intptr_t)
    uint8_t bytesPerSegmentL2; // Bytes per segment (in log2)
    uintptr_t length; // Length of table
    uintptr_t limit; // Limit of table length
  } *tpDescriptorEntry;

  typedef struct tSegmentEntry {
    uint8_t id; // = SAT_SEGMENT
    void set() {
      this->id = tEntryID::SAT_SEGMENT;
    }
  } *tpSegmentEntry;

  typedef struct tFreeEntry {
    uint8_t id; // = FREE
    uint8_t isReserved;
    void set(uint8_t isReserved) {
      this->id = tEntryID::FREE;
      this->isReserved = isReserved;
    }
  } *tpFreeEntry;

  typedef struct tForbiddenEntry {
    uint8_t id; // = FORBIDDEN
    void set() {
      this->id = tEntryID::FORBIDDEN;
    }
  } *tpForbiddenEntry;

  typedef struct tHeapSegmentEntry {
    uint8_t id;
    uint8_t heapID;
  } *tpHeapSegmentEntry;

  typedef struct tHeapLargeObjectEntry {
    uint8_t id; // = LARGE_OBJECT
    uint8_t heapID;
    uint32_t index;
    uint32_t length;
    uint64_t meta;
    void set(uint8_t heapID, uint32_t index, uint32_t length, uint64_t meta) {
      this->id = tEntryID::LARGE_OBJECT;
      this->heapID = heapID;
      this->index = index;
      this->length = length;
      this->meta = meta;
    }
  } *tpHeapLargeObjectEntry;

  typedef struct tEntry {
    union {
      tDescriptorEntry SATDescriptor;
      tSegmentEntry SATSegment;
      tHeapSegmentEntry heapSegment;
      tFreeEntry free;
      tForbiddenEntry forbidden;

      struct {
        uint8_t id;
        uint8_t bytes[31];
      };
    };

    template <typename T>
    T* get(uintptr_t index) { return (T*)&this[index]; }
    int getHeapID() { return (this->id & 0x40) ? -1 : this->heapSegment.heapID; }

  } *tpEntry;

  static_assert(sizeof(tEntry) == SAT::cEntrySize, "Bad tEntry size");
}
