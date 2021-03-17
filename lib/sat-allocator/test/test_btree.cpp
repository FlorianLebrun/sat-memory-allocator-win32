#include "./utils.h"
#include "./btree.h"

struct TestBTree : public Btree<uint64_t, 4>
{
  virtual BtNode* allocNode() { return new BtNode(); }
  virtual void freeNode(BtNode* node) { delete node; }
};

void test_btree() {
  TestBTree tree;
  /*
  uint64_t found = 0;
  this->insert(0);
  this->insert(5);
  this->insert(10);
  this->insert(15);
  this->insert(20);
  if (this->removeUpper(21, found)) {
  printf("found %d\n", found);
  this->display();
  }*/

  int count = 100000;
  for (int i = 0; i < count; ) {
    uint64_t value = uint64_t(rand())*uint64_t(rand());
    if (tree.insert(value)) {
      i++;
    }
  }
  for (int i = 0; i < count; ) {
    uint64_t value = uint64_t(rand())*uint64_t(rand());
    uint64_t found = 0;
    if (tree.removeUpper(value, found)) {
      assert(value <= found);
      i++;
    }
  }
}
