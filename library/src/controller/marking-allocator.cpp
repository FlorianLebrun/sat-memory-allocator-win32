#include "../base.h"
#include "./marking-allocator.h"

using namespace SAT;

MarkingDatabase SAT::marking_database;

void MarkingDatabase::init() {
  uint64_t stepTime = g_SAT.timeFrequency / maxSecondDivider;
  for (int i = 0; i < maxLevel; i++) levels[i].init(i, stepTime);
  this->freePages = 0;
}
tpFreePage MarkingDatabase::allocPage() {
  tpFreePage page;
  this->page_guard.lock();
  page = this->freePages;
  if (page) {
    this->freePages = page->nextPage;
  }
  else {
    uintptr_t index = g_SAT.allocSegmentSpan(1);
    tpMarkingEntry entry = tpMarkingEntry(&g_SATable[index]);
    entry->id = tEntryID::MARKING_DATABASE;
    page = tpFreePage(index << SAT::cSegmentSizeL2);
    for (uintptr_t i = 1; i < pagePerSegment; i++) {
      tpFreePage other = tpFreePage(&((char*)page)[i*pageSize]);
      other->nextPage = this->freePages;
      this->freePages = other;
    }
  }
  this->page_guard.unlock();
  return page;
}
bool MarkingDatabase::emitMark(tpMark mark) {
  uint64_t timeElapsed = g_SAT.getCurrentTimestamp() - mark->timeStart;
  for (int i = 0; i < maxLevel; i++) {
    if (timeElapsed < levels[i].timeLimit) {
      levels[i].guard.lock();
      mark->timeEnd = g_SAT.getCurrentTimestamp();
      timeElapsed = mark->timeEnd - mark->timeStart;
      if (timeElapsed < levels[i].timeLimit) {
        levels[i].emitMark(mark);
        levels[i].guard.unlock();
        return true;
      }
      else {
        levels[i].guard.unlock();
      }
    }
  }
  return false;
}
tpPendingMark MarkingDatabase::openPendingMark(SAT::IData* data) {
  this->mark_guard.lock();

  // Fill the free pending marks
  if (!this->freePendingMarks) {
    tPendingMark* marks = tpPendingMark(this->allocPage());
    for (uintptr_t i = 0; i < pendingPerPage; i++) {
      tpFreePendingMark free_mark = tpFreePendingMark(&marks[i]);
      free_mark->nextMark = this->freePendingMarks;
      this->freePendingMarks = free_mark;
    }
  }

  // Pop a free pending mark
  tpFreePendingMark free_mark = this->freePendingMarks;
  this->freePendingMarks = free_mark->nextMark;

  // Create and register the pending mark
  tpPendingMark mark = new(free_mark) tPendingMark(data);
  if (mark->nextMark = this->pendingMarks)
    this->pendingMarks->backMark = mark;
  mark->backMark = 0;
  this->pendingMarks = mark;

  this->mark_guard.unlock();
  return mark;
}
void MarkingDatabase::closePendingMark(tpPendingMark mark) {
  this->emitMark(mark);
  this->mark_guard.lock();

  // Unregister the pending mark
  if (mark->nextMark) mark->nextMark->backMark = mark->backMark;
  if (mark->backMark) mark->backMark->nextMark = 0;
  else this->pendingMarks = mark->nextMark;

  // Push a free pending mark
  tpFreePendingMark free_mark = tpFreePendingMark(mark);
  free_mark->nextMark = this->freePendingMarks;
  this->freePendingMarks = free_mark;

  this->mark_guard.unlock();
}
void MarkingDatabase::traverseAllMarks(IMarkVisitor* visitor) {
  for (int i = 0; i < maxLevel; i++) {
    this->levels[i].traverseAllMarks(visitor);
  }
  char buffer[1024];
  this->mark_guard.lock();
  for (tpPendingMark mark = this->pendingMarks; mark; mark = mark->nextMark) {
    visitor->visit(g_SAT.getTime(mark->timeStart), g_SAT.getTime(mark->timeEnd), mark->data, true);
  }
  this->mark_guard.unlock();
}

void MarkingLevel::init(int32_t level, uint64_t stepTime) {
  this->level = level;
  this->factor = 1;
  for (int i = 0; i < level; i++) this->factor *= 8;
  this->timeLimit = stepTime*this->factor;
  this->pageCount = 0;
  this->pageMaxCount = 128;
}
tpMarkPage MarkingLevel::getPage() {
  tpMarkPage page = this->pageEnd;
  if (page && page->length < MarkingDatabase::markPerPage) {
    return page;
  }
  else if (!page) {
    page = tpMarkPage(marking_database.allocPage());
    this->pageStart = page;
    this->pageEnd = page;
  }
  else {
    if (this->pageCount < this->pageMaxCount) {
      page = tpMarkPage(marking_database.allocPage());
    }
    else {
      page = this->pageStart;
      page->clean();
      this->pageStart = page->nextPage;
    }
    this->pageEnd->nextPage = page;
    this->pageEnd = page;
  }
  page->nextPage = 0;
  page->length = 0;
  page->level = this->level;
  return page;
}
void MarkingLevel::emitMark(tpMark mark) {
  tpMarkPage page = this->getPage();
  tpMark new_mark = &page->marks[page->length];
  new_mark->timeStart = mark->timeStart;
  new_mark->timeEnd = mark->timeEnd;
  new_mark->data = mark->data;
  page->length++;
}
void MarkingLevel::traverseAllMarks(IMarkVisitor* visitor) {
  char buffer[1024];
  for (tpMarkPage page = this->pageStart; page; page = page->nextPage) {
    for (int i = 0; i < page->length; i++) {
      tpMark mark = &page->marks[i];
      visitor->visit(g_SAT.getTime(mark->timeStart), g_SAT.getTime(mark->timeEnd), mark->data, false);
    }
  }
}


tPendingMark::tPendingMark(SAT::IData* data) {
  this->timeStart = g_SAT.getCurrentTimestamp();
  this->timeEnd = this->timeEnd;
  this->data = data;
}
double tPendingMark::getStartTime() {
  return g_SAT.getTime(this->timeStart);
}
double tPendingMark::getElapsedTime() {
  this->timeEnd = g_SAT.getCurrentTimestamp();
  return g_SAT.getTime(this->timeEnd - this->timeStart);
}
SAT::IData* tPendingMark::get() {
  return this->data;
}
void tPendingMark::set(SAT::IData* data) {
  if (this->data) this->data->release();
  this->data = data;
}
void tPendingMark::complete() {
  marking_database.closePendingMark(this);
}


void init() {
  static bool is_init = false;
  if (!is_init) {
    is_init = true;
    SAT::marking_database.init();
  }
}

#include <stdarg.h>

struct MessageData : SATBasicRealeasable<SAT::IData> {
  char message[1];

  static MessageData* New(const char* tag, const char* format, va_list& args) {
    char message[1024];
    int messageLen = vsprintf_s(message, sizeof(message) - 1, format, args) + 1;
    int tagLen = strlen(tag) + 1;
    MessageData* data = (MessageData*)new char[sizeof(MessageData) + messageLen + tagLen];
    data->MessageData::MessageData();
    memcpy(data->message, message, messageLen);
    memcpy(data->message + messageLen, tag, tagLen);
    return data;
  }
  virtual const char* getTag() override {
    return &this->message[strlen(this->message) + 1];
  }
  virtual void toString(char* buffer, int size) override {
    strcpy_s(buffer, size, this->message);
  }
};

void sat_event_message(const char* tag, const char* format, ...) {
  init();

  va_list args;
  va_start(args, format);
  SAT::IData* data = MessageData::New(tag, format, args);
  va_end(args);

  tMark mark;
  mark.data = data;
  mark.timeEnd = 0;
  mark.timeStart = g_SAT.getCurrentTimestamp();
  SAT::marking_database.emitMark(&mark);
}

SAT::IMark* sat_mark_message(const char* tag, const char* format, ...) {
  init();

  va_list args;
  va_start(args, format);
  SAT::IData* data = MessageData::New(tag, format, args);
  va_end(args);

  return SAT::marking_database.openPendingMark(data);
}

SAT::IMark* sat_mark(SAT::IData* data) {
  init();

  return SAT::marking_database.openPendingMark(data);
}

void sat_marking_print() {
  struct MarkVisitor: SAT::IMarkVisitor {
    virtual void visit(double timeStart, double timeEnd, SAT::IData* data, bool pending) override {
      if(data) {
        char buffer[1024];
        data->toString(buffer, 1023);
        if(pending) printf("...... %s (%lg s)......\n", buffer, timeEnd-timeStart);
        else printf("------ %s (%lg s)------\n", buffer, timeEnd-timeStart);
      }
    }
  };
  SAT::marking_database.traverseAllMarks(&MarkVisitor());
}
