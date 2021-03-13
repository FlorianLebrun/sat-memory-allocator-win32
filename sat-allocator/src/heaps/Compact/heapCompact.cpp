#include "../../allocators/ZonedBuddy/index.h"
#include "../../allocators/LargeObject/index.h"


struct Global : public sat::GlobalHeap {
  LargeObjectAllocator::Global largeObject;
  ZonedBuddyAllocator::Global::Cache zonedBuddy;

  Global(const char* name, int heapID) : sat::GlobalHeap(name, heapID) {
    
    // Init allocators
    this->zonedBuddy.init(this);
    this->largeObject.init(this);

    // Register zonedBuddy allocators
    for (int baseID = 0; baseID <= 6; baseID++) {
      sat::IObjectAllocator* base = this->zonedBuddy.getAllocator(baseID << 4);
      this->sizeMapping.registerAllocator(base);
      this->sizeMappingWithMeta.registerAllocator(base);
      for (int dividor = 8; dividor <= 16; dividor++) {
        sat::IObjectAllocator* zoned = this->zonedBuddy.getAllocator((baseID << 4) | dividor);
        this->sizeMapping.registerAllocator(zoned);
        this->sizeMappingWithMeta.registerAllocator(zoned);
      }
    }

    // Register largeObject allocator
    this->sizeMapping.registerAllocator(&this->largeObject);
    this->sizeMappingWithMeta.registerAllocator(&this->largeObject);

    // Compute size mappings
    this->sizeMapping.finalize();
    this->sizeMappingWithMeta.finalize();
  }
  virtual ~Global() override {
     this->zonedBuddy.flushCache();
  }
  virtual size_t free(void* obj) override {
    int size = 0;
    uintptr_t ptr = uintptr_t(obj);
    uintptr_t index = ptr >> sat::memory::cSegmentSizeL2;
    auto entry = &sat::memory::table->entries[index];
    switch (entry->id) {
    case sat::tHeapEntryID::PAGE_ZONED_BUDDY:
      size = this->zonedBuddy.freePtr(entry, ptr);
      break;
    case sat::tHeapEntryID::LARGE_OBJECT:
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
  virtual sat::LocalHeap* createLocal();
};

struct Local : sat::LocalHeap {
  LargeObjectAllocator::Global& largeObject;
  ZonedBuddyAllocator::Local::Cache zonedBuddy;

  Local(Global* global)
    : sat::LocalHeap(global), largeObject(global->largeObject) {
      
    // Init allocators
    this->zonedBuddy.init(&global->zonedBuddy);

    // Register zonedBuddy allocators
    for (int baseID = 0; baseID <= 6; baseID++) {
      sat::IObjectAllocator* base = this->zonedBuddy.getAllocator(baseID << 4);
      this->sizeMapping.registerAllocator(base);
      this->sizeMappingWithMeta.registerAllocator(base);
      for (int dividor = 8; dividor <= 16; dividor++) {
        sat::IObjectAllocator* zoned = this->zonedBuddy.getAllocator((baseID << 4) | dividor);
        this->sizeMapping.registerAllocator(zoned);
        this->sizeMappingWithMeta.registerAllocator(zoned);
      }
    }

    // Register largeObject allocator
    this->sizeMapping.registerAllocator(&this->largeObject);
    this->sizeMappingWithMeta.registerAllocator(&this->largeObject);

    // Compute size mappings
    this->sizeMapping.finalize();
    this->sizeMappingWithMeta.finalize();
  }
  virtual ~Local() override {
  }
  virtual size_t free(void* obj) override
  {
    int size = 0;
    uintptr_t ptr = uintptr_t(obj);
    uintptr_t index = ptr >> sat::memory::cSegmentSizeL2;
    auto entry = &sat::memory::table->entries[index];
    switch (entry->id) {
    case sat::tHeapEntryID::PAGE_ZONED_BUDDY:
      size = this->zonedBuddy.freePtr(entry, ptr);
      break;
    case sat::tHeapEntryID::LARGE_OBJECT:
      size = this->largeObject.freePtr(index);
      break;
    }
    return size;
  }
  virtual void flushCache() override
  {
    this->zonedBuddy.flushCache();
  }
};

sat::LocalHeap* Global::createLocal() {
  Local* heap = new(sat::memory::table->allocBuffer(sizeof(Local))) Local(this);
  return heap;
}

namespace sat {
  sat::GlobalHeap* createHeapCompact(const char* name) {
    return new(sat::memory::table->allocBuffer(sizeof(Global))) Global(name, 0);
  }
}
