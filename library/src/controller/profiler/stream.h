
#ifndef _SAT_stream_h_
#define _SAT_stream_h_

enum class SATAttributID {
  NONE = 0,
  THREAD_CPU,
  THREAD_CALLSTACK,
  HEAP_SIZE,
  PROCESS_CPU,
};

typedef struct SATStreamRecord {
  uint64_t timestamp;
} *tpSATStreamRecord;

typedef struct tSATStreamEntry {
  uint8_t id;
  uint32_t size;
  uint64_t timestamp;
  uintptr_t nextIndex;
} *tpSATStreamEntry;

struct SATStream {

  template<class SampleT>
  struct iterator;
  template<class SampleT>
  struct range_iterator;

  SATStream* profileNext;
  SATProfile* profile;

  SATAttributID attributId;
  SAT::IThread* thread;
  SAT::IHeap* heap;

  SpinLock lock;
  uint32_t baseSize;
  uint32_t recordSize;

  SATStream(uint32_t recordSize);
  static void* operator new(size_t nSize);
  static void operator delete(void* ptr, size_t nSize);
  ~SATStream();

  uintptr_t getEntryIndex();
  tpSATStreamEntry getEntryFrame();

  template<class SampleT>
  SampleT* appendSample() {return (SampleT*)this->createSample();}

  void getTimeRange(uint64_t& timeStart, uint64_t& timeEnd);

  virtual void emit() = 0;

private:
  tpSATStreamEntry lastEntry;
  uintptr_t lastIndex;

  tpSATStreamRecord createSample();
};

template<class SampleT>
struct SATStream::iterator {
  iterator(SATStream* stream) {
    this->stream = stream;
    this->nextIndex = stream->getEntryFrame()->nextIndex;
    this->current = 0;
    this->last = 0;
  }
  SampleT* begin() {
    this->gotoFrame(stream->getEntryIndex());
    this->current += this->stream->baseSize;
    return (SampleT*)(this->current);
  }
  SampleT* next() {
    this->current += this->stream->recordSize;
    if (this->current >= this->last) {
      this->gotoFrame(this->nextIndex);
    }
    return (SampleT*)(this->current);
  }
  SampleT* gotoTime(uint64_t time) {
    while (this->nextIndex && g_SATable->get<tSATStreamEntry>(this->nextIndex)->timestamp < time) {
      this->gotoFrame(this->nextIndex);
    }
    while (this->current && tpSATStreamRecord(this->current)->timestamp < time) {
      this->next();
    }
    return (SampleT*)(this->current);
  }
private:
  SATStream* stream;
  uintptr_t nextIndex;
  uintptr_t current;
  uintptr_t last;
  tpSATStreamEntry gotoFrame(uintptr_t index) {
    if (index) {
      tpSATStreamEntry entry = tpSATStreamEntry(&g_SATable[index]);
      this->current = index << SAT::cSegmentSizeL2;
      this->last = this->current + entry->size;
      this->nextIndex = entry->nextIndex;
      return entry;
    }
    else {
      this->nextIndex = 0;
      this->current = 0;
      this->last = 0;
      return 0;
    }
  }
};

template<class SampleT>
struct SATStream::range_iterator : SATStream::iterator<SampleT> {
  range_iterator(SATStream* stream, uint64_t start, uint64_t end) : iterator(stream) {
    this->start = start;
    this->end = end;
  }
  SampleT* begin() {
    this->gotoFrame(stream->getEntryIndex());
    return this->gotoTime(this->start);
  }
  SampleT* next() {
    if (SampleT* record = this->iterator::next()) {
      if (record->timestamp <= this->end) {
        return record;
      }
      this->nextIndex = 0;
      this->current = 0;
      this->last = 0;
    }
    return 0;
  }
private:
  uint64_t start, end;
};

#endif
