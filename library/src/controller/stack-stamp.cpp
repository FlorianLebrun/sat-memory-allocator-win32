#include "./stack-stamp.h"

using namespace SAT;

struct RootStackMarker : SAT::IStackMarker {
  virtual void getSymbol(char* buffer, int size) override {
    sprintf_s(buffer, size, "<root>");
  }
};

StackStampDatabase::StackStampDatabase() {
  memset(&this->root, 0, sizeof(this->root));
  this->root.marker.as<RootStackMarker>();
  this->root.lock.SpinLock::SpinLock();
  this->buffer_lock.SpinLock::SpinLock();
  this->buffer_base = (char*)alignX(8, (uintptr_t)&this[1]);
  this->buffer_size = SAT::cSegmentSize - sizeof(this[0]);
}

void* StackStampDatabase::allocBuffer(int size) {
  char* ptr;
  this->buffer_lock.lock();
  if (this->buffer_size < size) {
    uintptr_t index = g_SAT.allocSegmentSpan(1);
    tStackStampDatabaseEntry& entry = (tStackStampDatabaseEntry&)g_SATable[index];
    entry.id = SAT::tEntryID::STACKSTAMP_DATABASE;
    entry.tracer = uintptr_t(this) >> SAT::cSegmentSizeL2;
    ptr = (char*)(index << SAT::cSegmentSizeL2);
    this->buffer_base = ptr + size;
    this->buffer_size = SAT::cSegmentSize - size;
  }
  else {
    ptr = this->buffer_base;
    this->buffer_base = ptr + size;
    this->buffer_size -= size;
  }
  this->buffer_lock.unlock();
  return ptr;
}

StackNode StackStampDatabase::map(SAT::StackMarker& marker, StackNode parent) {
  for(;;) {
    StackNode lastNode = parent->children;
    for(StackNode node = lastNode; node; node=lastNode->next) {
      lastNode = node;
      if(node->marker.compare(marker) == 0) {
        return node;
      }
    }

    if(lastNode) {
      lastNode->lock.lock();
      if(!lastNode->next) {
        StackNode node = (StackNode)this->allocBuffer(sizeof(tStackNode));
        node->lock.SpinLock::SpinLock();
        node->next = 0;
        node->children = 0;
        node->parent = parent;
        node->marker.copy(marker);
        lastNode->next = node;
        lastNode->lock.unlock();
        return node;
      }
      else lastNode->lock.unlock();
    }
    else {
      parent->lock.lock();
      if(!parent->children) {
        StackNode node = (StackNode)this->allocBuffer(sizeof(tStackNode));
        node->lock.SpinLock::SpinLock();
        node->next = 0;
        node->children = 0;
        node->parent = parent;
        node->marker.copy(marker);
        parent->children = node;
        parent->lock.unlock();
        return node;
      }
      else parent->lock.unlock();
    }
  }
}

struct StackStampBuilder : IStackStampBuilder {
  StackStampDatabase* dbase;
  StackNode tos;
  StackStampBuilder(StackStampDatabase* dbase) {
    this->dbase = dbase;
    this->tos = &dbase->root;
  }
  virtual void push(SAT::StackMarker& marker) override {
    this->tos = this->dbase->map(marker, this->tos);
  }
  uint64_t flush() {
    return uint64_t(this->tos);
  }
};

uint64_t StackStampDatabase::getStackStamp(SAT::Thread* thread, IStackStampAnalyzer* stack_analyzer) {
  StackStampBuilder stack_builder(this);
  if(thread->CaptureThreadStackStamp(stack_builder, stack_analyzer)) {
    return stack_builder.flush();
  }
  else {
    return 0;
  }
}

void StackStampDatabase::traverseStack(uint64_t stackstamp, IStackVisitor* visitor) {
  for (StackNode node = StackNode(stackstamp); node; node=node->parent) {
    visitor->visit(node->marker);
  }
}

StackStampDatabase* createStackStampDatabase() {
  uintptr_t index = g_SAT.allocSegmentSpan(1);
  StackStampDatabase* tracer = (StackStampDatabase*)(index << SAT::cSegmentSizeL2);
  tracer->StackStampDatabase::StackStampDatabase();

  tStackStampDatabaseEntry& entry = (tStackStampDatabaseEntry&)g_SATable[index];
  entry.id = SAT::tEntryID::STACKSTAMP_DATABASE;
  entry.tracer = index;

  return tracer;
}

void StackHistogram::insertCallsite(uint64_t stackstamp) {

  root.hits++;
  if(!stackstamp) return;

  SAT::StackMarker* frames_buffer[1024];
  SAT::StackMarker** frames = &frames_buffer[1024];
  int numFrames = 0;
  for (StackNode node = StackNode(stackstamp); node; node=node->parent) {
    frames--;
    frames[0] = &node->marker;
    numFrames++;
  }

  tpNode cnode = &root;
  for (int i = 0; i < numFrames; i++) {
    tpNode fnode = cnode->subcalls;
    while (fnode && fnode->marker->compare(*frames[i]) != 0) fnode = fnode->next;
    if (!fnode) {
      fnode = new tNode(frames[i]);
      fnode->next = cnode->subcalls;
      cnode->subcalls = fnode;
    }
    fnode->hits++;
    cnode = fnode;
  }
}

static int printStackHistogramNode(StackHistogram::tNode* node, int level, float hitsScale) {
  char buffer[512];
  node->marker->getSymbol(buffer, sizeof(buffer));
  for (int i = 0; i < level; i++) printf("  ");
  printf("%s %3.2f %%\n", buffer, float(node->hits) *hitsScale);

  if (node->subcalls) {
    int nleaf = 0;
    level++;
    for (StackHistogram::tNode* sub = node->subcalls; sub; sub = sub->next) {
      nleaf += printStackHistogramNode(sub, level, hitsScale);
    }
    return nleaf;
  }
  else return 1;
}

void StackHistogram::print() {
  int nleaf = printStackHistogramNode(&root, 0, 100.0f / float(root.hits));
  printf("Num. hits = %d\n", root.hits);
  printf("Num. callgraph path = %d\n", nleaf);
}
