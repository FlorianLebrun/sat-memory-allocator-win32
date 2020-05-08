#include "./index.h"

void ZonedBuddyAllocator::Local::Cache::init(Global::Cache* global)
{
  this->Cache::Cache();
  this->global = global;
  this->pageSize = global->pageSize;

  this->base_cache_0.init(this, 0, &global->base_cache_0);
  this->base_cache_0.init_zoning();

  this->base_cache_1.init(this, &this->base_cache_0, &global->base_cache_1);
  this->base_cache_1.init_zoning();

  this->base_cache_2.init(this, &this->base_cache_1, &global->base_cache_2);
  this->base_cache_2.init_zoning();

  this->base_cache_3.init(this, &this->base_cache_2, &global->base_cache_3);
  this->base_cache_3.init_zoning();

  this->base_cache_4.init(this, &this->base_cache_3, &global->base_cache_4);
  this->base_cache_4.init_zoning();

  this->base_cache_5.init(this, &this->base_cache_3, &global->base_cache_5);
  this->base_cache_5.init_zoning();

  this->base_cache_6.init(this, &this->base_cache_3, &global->base_cache_6);
  this->base_cache_6.init_zoning();

  this->base_cache_7.init(this, &this->base_cache_3, &global->base_cache_7);
  this->base_cache_7.init_zoning();
}

int ZonedBuddyAllocator::Local::Cache::getCachedSize() {
  int size = 0;
  size += this->base_cache_7.getCachedSize();
  size += this->base_cache_6.getCachedSize();
  size += this->base_cache_5.getCachedSize();
  size += this->base_cache_4.getCachedSize();
  size += this->base_cache_3.getCachedSize();
  size += this->base_cache_2.getCachedSize();
  size += this->base_cache_1.getCachedSize();
  size += this->base_cache_0.getCachedSize();
  return size;
}

void ZonedBuddyAllocator::Local::Cache::flushCache()
{
  this->base_cache_7.flushCache(); // 256
  this->base_cache_6.flushCache(); // 512
  this->base_cache_5.flushCache(); // 1k
  this->base_cache_4.flushCache(); // 2k
  this->base_cache_3.flushCache(); // 4k
  this->base_cache_2.flushCache(); // 8k
  this->base_cache_1.flushCache(); // 16k
  this->base_cache_0.flushCache(); // 32k
}

SAT::IObjectAllocator* ZonedBuddyAllocator::Local::Cache::getAllocator(int id) {
  switch (id) {
  case 0:
  case 1: return &this->base_cache_0;
  case 2: return &this->base_cache_0.zone_cache_2;
  case 3: return &this->base_cache_0.zone_cache_3;
  case 4: return &this->base_cache_0.zone_cache_4;
  case 5: return &this->base_cache_0.zone_cache_5;
  case 6: return &this->base_cache_0.zone_cache_6;
  case 7: return &this->base_cache_0.zone_cache_7;
  case 8: return &this->base_cache_0.zone_cache_8;
  case 9: return &this->base_cache_0.zone_cache_9;
  case 10: return &this->base_cache_0.zone_cache_10;
  case 11: return &this->base_cache_0.zone_cache_11;
  case 12: return &this->base_cache_0.zone_cache_12;
  case 13: return &this->base_cache_0.zone_cache_13;
  case 14: return &this->base_cache_0.zone_cache_14;
  case 15: return &this->base_cache_0.zone_cache_15;
  case 16:
  case 17: return &this->base_cache_1;
  case 18: return &this->base_cache_1.zone_cache_2;
  case 19: return &this->base_cache_1.zone_cache_3;
  case 20: return &this->base_cache_1.zone_cache_4;
  case 21: return &this->base_cache_1.zone_cache_5;
  case 22: return &this->base_cache_1.zone_cache_6;
  case 23: return &this->base_cache_1.zone_cache_7;
  case 24: return &this->base_cache_1.zone_cache_8;
  case 25: return &this->base_cache_1.zone_cache_9;
  case 26: return &this->base_cache_1.zone_cache_10;
  case 27: return &this->base_cache_1.zone_cache_11;
  case 28: return &this->base_cache_1.zone_cache_12;
  case 29: return &this->base_cache_1.zone_cache_13;
  case 30: return &this->base_cache_1.zone_cache_14;
  case 31: return &this->base_cache_1.zone_cache_15;
  case 32:
  case 33: return &this->base_cache_2;
  case 34: return &this->base_cache_2.zone_cache_2;
  case 35: return &this->base_cache_2.zone_cache_3;
  case 36: return &this->base_cache_2.zone_cache_4;
  case 37: return &this->base_cache_2.zone_cache_5;
  case 38: return &this->base_cache_2.zone_cache_6;
  case 39: return &this->base_cache_2.zone_cache_7;
  case 40: return &this->base_cache_2.zone_cache_8;
  case 41: return &this->base_cache_2.zone_cache_9;
  case 42: return &this->base_cache_2.zone_cache_10;
  case 43: return &this->base_cache_2.zone_cache_11;
  case 44: return &this->base_cache_2.zone_cache_12;
  case 45: return &this->base_cache_2.zone_cache_13;
  case 46: return &this->base_cache_2.zone_cache_14;
  case 47: return &this->base_cache_2.zone_cache_15;
  case 48:
  case 49: return &this->base_cache_3;
  case 50: return &this->base_cache_3.zone_cache_2;
  case 51: return &this->base_cache_3.zone_cache_3;
  case 52: return &this->base_cache_3.zone_cache_4;
  case 53: return &this->base_cache_3.zone_cache_5;
  case 54: return &this->base_cache_3.zone_cache_6;
  case 55: return &this->base_cache_3.zone_cache_7;
  case 56: return &this->base_cache_3.zone_cache_8;
  case 57: return &this->base_cache_3.zone_cache_9;
  case 58: return &this->base_cache_3.zone_cache_10;
  case 59: return &this->base_cache_3.zone_cache_11;
  case 60: return &this->base_cache_3.zone_cache_12;
  case 61: return &this->base_cache_3.zone_cache_13;
  case 62: return &this->base_cache_3.zone_cache_14;
  case 63: return &this->base_cache_3.zone_cache_15;
  case 64:
  case 65: return &this->base_cache_4;
  case 66: return &this->base_cache_4.zone_cache_2;
  case 67: return &this->base_cache_4.zone_cache_3;
  case 68: return &this->base_cache_4.zone_cache_4;
  case 69: return &this->base_cache_4.zone_cache_5;
  case 70: return &this->base_cache_4.zone_cache_6;
  case 71: return &this->base_cache_4.zone_cache_7;
  case 72: return &this->base_cache_4.zone_cache_8;
  case 73: return &this->base_cache_4.zone_cache_9;
  case 74: return &this->base_cache_4.zone_cache_10;
  case 75: return &this->base_cache_4.zone_cache_11;
  case 76: return &this->base_cache_4.zone_cache_12;
  case 77: return &this->base_cache_4.zone_cache_13;
  case 78: return &this->base_cache_4.zone_cache_14;
  case 79: return &this->base_cache_4.zone_cache_15;
  case 80:
  case 81: return &this->base_cache_5;
  case 82: return &this->base_cache_5.zone_cache_2;
  case 83: return &this->base_cache_5.zone_cache_3;
  case 84: return &this->base_cache_5.zone_cache_4;
  case 85: return &this->base_cache_5.zone_cache_5;
  case 86: return &this->base_cache_5.zone_cache_6;
  case 87: return &this->base_cache_5.zone_cache_7;
  case 88: return &this->base_cache_5.zone_cache_8;
  case 89: return &this->base_cache_5.zone_cache_9;
  case 90: return &this->base_cache_5.zone_cache_10;
  case 91: return &this->base_cache_5.zone_cache_11;
  case 92: return &this->base_cache_5.zone_cache_12;
  case 93: return &this->base_cache_5.zone_cache_13;
  case 94: return &this->base_cache_5.zone_cache_14;
  case 95: return &this->base_cache_5.zone_cache_15;
  case 96:
  case 97: return &this->base_cache_6;
  case 98: return &this->base_cache_6.zone_cache_2;
  case 99: return &this->base_cache_6.zone_cache_3;
  case 100: return &this->base_cache_6.zone_cache_4;
  case 101: return &this->base_cache_6.zone_cache_5;
  case 102: return &this->base_cache_6.zone_cache_6;
  case 103: return &this->base_cache_6.zone_cache_7;
  case 104: return &this->base_cache_6.zone_cache_8;
  case 105: return &this->base_cache_6.zone_cache_9;
  case 106: return &this->base_cache_6.zone_cache_10;
  case 107: return &this->base_cache_6.zone_cache_11;
  case 108: return &this->base_cache_6.zone_cache_12;
  case 109: return &this->base_cache_6.zone_cache_13;
  case 110: return &this->base_cache_6.zone_cache_14;
  case 111: return &this->base_cache_6.zone_cache_15;
  case 112:
  case 113: return &this->base_cache_7;
  case 114: return &this->base_cache_7.zone_cache_2;
  case 115: return &this->base_cache_7.zone_cache_3;
  case 116: return &this->base_cache_7.zone_cache_4;
  case 117: return &this->base_cache_7.zone_cache_5;
  case 118: return &this->base_cache_7.zone_cache_6;
  case 119: return &this->base_cache_7.zone_cache_7;
  case 120: return &this->base_cache_7.zone_cache_8;
  case 121: return &this->base_cache_7.zone_cache_9;
  case 122: return &this->base_cache_7.zone_cache_10;
  case 123: return &this->base_cache_7.zone_cache_11;
  case 124: return &this->base_cache_7.zone_cache_12;
  case 125: return &this->base_cache_7.zone_cache_13;
  case 126: return &this->base_cache_7.zone_cache_14;
  case 127: return &this->base_cache_7.zone_cache_15;
  default: return 0;
  }
}