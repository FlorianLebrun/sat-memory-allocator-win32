#include "../base.h"

#ifndef SAT_MarkingAllocator_h_
#define SAT_MarkingAllocator_h_

namespace SAT {
  
  typedef struct tMarkingEntry {
    uint8_t id; // = MARKING_DATABASE
  } *tpMarkingEntry;

  typedef struct tMark {
    uint64_t timeStart;
    uint64_t timeEnd;
    SAT::IData* data;
  } *tpMark;

  typedef struct tMarkPage {
    tMarkPage* nextPage;
    uint32_t length;
    uint32_t level;
    tMark marks[0];
    void clean() {
      int length = this->length;
      this->length = 0;
      for (int i = 0; i < length; i++) {
        tpMark mark = &this->marks[i];
        if (SAT::IData* data = mark->data) {
          mark->data = 0;
          data->release();
        }
      }
    }
  } *tpMarkPage;

  typedef struct tPendingMark : SAT::IMark, tMark {
    tPendingMark *nextMark, *backMark;

    tPendingMark(SAT::IData* data = 0);
    virtual double getStartTime() override;
    virtual SAT::IData* get() override;
    virtual double getElapsedTime() override;
    virtual void set(SAT::IData* data) override;
    virtual void complete() override;
  } *tpPendingMark;

  typedef struct tFreePendingMark {
    tFreePendingMark* nextMark;
  } *tpFreePendingMark;

  typedef struct tFreePage {
    tFreePage* nextPage;
  } *tpFreePage;

  struct MarkingLevel {
    SpinLock guard;

    // Level timing
    int32_t level;
    int32_t factor;
    uint64_t timeLimit;

    // Level Stream pages
    tMarkPage *pageStart, *pageEnd;
    int pageCount, pageMaxCount;

    void init(int32_t level, uint64_t stepTime);
    void emitMark(tpMark mark);
    tpMarkPage getPage();
    void traverseAllMarks(IMarkVisitor* visitor);
  };

  struct MarkingDatabase {
    static const uint32_t pageSize = 4096;
    static const uint32_t pagePerSegment = SAT::cSegmentSize / pageSize;
    static const uint32_t markPerPage = pageSize / sizeof(tMark);
    static const uint32_t pendingPerPage = pageSize / sizeof(tPendingMark);
    static const uint64_t maxSecondDivider = 8 * 8 * 8 * 8;
    static const uint32_t maxLevel = 9;
    SpinLock page_guard;
    SpinLock mark_guard;

    MarkingLevel levels[maxLevel];
    tpPendingMark pendingMarks;
    tpFreePendingMark freePendingMarks;
    tpFreePage freePages;

    void init();
    tpFreePage allocPage();
    bool emitMark(tpMark mark);
    tpPendingMark openPendingMark(SAT::IData* data);
    void closePendingMark(tpPendingMark mark);
    void traverseAllMarks(IMarkVisitor* visitor);
  };
  
  extern MarkingDatabase marking_database;
}

#endif