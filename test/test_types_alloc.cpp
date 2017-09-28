#include <sat-memory-allocator/sat-memory-allocator.h>
#include "./utils.h"

struct myClass2 {
  static SAT::TypeDefID typeID;
  uint32_t x;
};
SAT::TypeDefID myClass2::typeID = 0;

struct myClass1 {
  static SAT::TypeDefID typeID;
  myClass1* x;
  myClass2* y;
};
SAT::TypeDefID myClass1::typeID = 0;

void test_types_alloc() {
  /*{
  SAT::ITypesController* types = sat_get_types_controller();
  SAT::TypeDef def_uint32_t = types->createBufferType("uint32_t", 0, SAT::ENC_INT, sizeof(uint32_t));
  SAT::TypeDef def_myClass1 = types->createClassType("myClass1", 0, sizeof(myClass1), 2, 0);
  SAT::TypeDef def_myClass2 = types->createClassType("myClass2", 0, sizeof(myClass2), 0, 1);

  myClass1::typeID = types->getTypeID(def_myClass1);
  myClass2::typeID = types->getTypeID(def_myClass2);

  def_myClass1->setRef(0, &SAT::tTypeDef::zero<myClass1>().x, myClass2::typeID);
  def_myClass1->setRef(1, &SAT::tTypeDef::zero<myClass1>().y, myClass2::typeID);

  def_myClass2->setBuffer(0, &SAT::tTypeDef::zero<myClass2>().x, types->getTypeID(def_uint32_t));
  }*/

  Chrono c;
  static const int count = 50000;
  std::vector<myClass1*> objects(count);
  c.Start();

  for (int i = 0; i < count; i++) {
    objects[i] = new (sat_malloc_ex(sizeof(myClass1), myClass1::typeID)) myClass1();
  }
  printf("[SAT-malloc] alloc object time = %g ns\n", c.GetDiffFloat(Chrono::NS) / float(count));

  c.Start();
  for (int i = 0; i < count; i++) {
    int k = fastrand() % objects.size();
    void* obj = objects[k];
    objects[k] = objects.back();
    objects.pop_back();
    sat_free(obj);
  }
  printf("[SAT-malloc] free object time = %g ns\n", c.GetDiffFloat(Chrono::NS) / float(count));

  struct Visitor : SAT::IObjectVisitor {
    virtual bool visit(SAT::tpObjectInfos obj) override {
      printf("%.8X\n", obj->base);
      return true;
    }
  };
  sat_get_contoller()->traverseObjects(&Visitor());
}
