#include "../allocator-ZonedBuddy/index.h"
#include "../allocator-LargeObject/index.h"


struct Global : SAT::GlobalHeap {
  LargeObjectAllocator::Global largeObject;
  ZonedBuddyAllocator::Global::Cache zonedBuddy;

  Global(const char* name) : SAT::GlobalHeap(name) {
    this->largeObject.init(this);
    this->zonedBuddy.init(this);
    for (int baseID = 0; baseID <= 6; baseID++) {
      IObjectAllocator* base = this->zonedBuddy.getAllocator(baseID << 4);
      this->sizeMapping.registerAllocator(base);
      for (int dividor = 8; dividor <= 16; dividor++) {
        IObjectAllocator* zoned = this->zonedBuddy.getAllocator((baseID << 4) | dividor);
        this->sizeMapping.registerAllocator(zoned);
      }
    }
    this->sizeMapping.registerAllocator(&this->largeObject);
    this->sizeMapping.finalize();
  }
  virtual size_t free(void* obj) override {
    int size = 0;
    uintptr_t ptr = uintptr_t(obj);
    uintptr_t index = ptr >> SAT::cSegmentSizeL2;
    SAT::tEntry* entry = g_SATable->get<SAT::tEntry>(index);
    switch (entry->id) {
    case SAT::tEntryID::PAGE_ZONED_BUDDY:
      size = this->zonedBuddy.freePtr(entry, ptr);
      break;
    case SAT::tEntryID::LARGE_OBJECT:
      size = this->largeObject.freePtr(index);
      break;
    default:
      throw std::exception("NOT SUPPORTED");
    }
    return size;
  }
  virtual void flushCache() override
  {
    this->zonedBuddy.flushCache();
  }
  virtual void terminate() override
  {
    this->zonedBuddy.flushCache();
  }
  virtual SAT::LocalHeap* createLocal();
};

struct Local : SAT::LocalHeap {
  LargeObjectAllocator::Global& largeObject;
  ZonedBuddyAllocator::Local::Cache zonedBuddy;

  Local(Global* global)
    : SAT::LocalHeap(global), largeObject(global->largeObject) {
    this->zonedBuddy.init(&global->zonedBuddy);
    for (int baseID = 0; baseID <= 6; baseID++) {
      IObjectAllocator* base = this->zonedBuddy.getAllocator(baseID << 4);
      this->sizeMapping.registerAllocator(base);
      for (int dividor = 8; dividor <= 16; dividor++) {
        IObjectAllocator* zoned = this->zonedBuddy.getAllocator((baseID << 4) | dividor);
        this->sizeMapping.registerAllocator(zoned);
      }
    }
    this->sizeMapping.registerAllocator(&this->largeObject);
    this->sizeMapping.finalize();
  }
  virtual size_t free(void* obj) override
  {
    int size = 0;
    uintptr_t ptr = uintptr_t(obj);
    uintptr_t index = ptr >> SAT::cSegmentSizeL2;
    SAT::tEntry* entry = g_SATable->get<SAT::tEntry>(index);
    switch (entry->id) {
    case SAT::tEntryID::PAGE_ZONED_BUDDY:
      size = this->zonedBuddy.freePtr(entry, ptr);
      break;
    case SAT::tEntryID::LARGE_OBJECT:
      size = this->largeObject.freePtr(index);
      break;
    }
    return size;
  }
  virtual void flushCache() override
  {
    this->zonedBuddy.flushCache();
  }
  virtual void terminate() override {
  }
};

SAT::LocalHeap* Global::createLocal() {
  Local* heap = new(g_SAT.allocBuffer(sizeof(Local))) Local(this);
  return heap;
}

namespace SAT {
  SAT::GlobalHeap* createHeapCompact(const char* name) {
    return new(g_SAT.allocBuffer(sizeof(Global))) Global(name);
  }
}
