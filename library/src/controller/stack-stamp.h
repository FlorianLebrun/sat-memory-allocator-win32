
#include "../base.h"

#ifndef _SAT_StackStamp_h_
#define _SAT_StackStamp_h_

typedef struct tStackNode* StackNode;

struct tStackNode {
  SpinLock lock;
  StackNode next;
  StackNode parent;
  StackNode children;
  SAT::StackMarker marker;
};

typedef struct tStackStampDatabaseEntry {
  uint8_t id; // = STACKSTAMP_DATABASE
  uint64_t tracer; // Page index of the tracer (see IStackTracer)
} *tpStackStampDatabaseEntry;

struct StackStampDatabase
{
  tStackNode root;

  char* buffer_base;
  int buffer_size;
  SpinLock buffer_lock;

  StackStampDatabase();

  void* allocBuffer(int size);
  StackNode map(SAT::StackMarker& marker, StackNode parent);

  uint64_t getStackStamp(SAT::Thread* thread, SAT::IStackStampAnalyzer* stack_analyzer);
  void traverseStack(uint64_t stackstamp, SAT::IStackVisitor* visitor);
};

struct StackHistogram : public SATBasicRealeasable<SAT::IStackHistogram> 
{
  void insertCallsite(uint64_t stackstamp);
  virtual void print() override;
  virtual void destroy() override {delete this;}
};

StackStampDatabase* createStackStampDatabase();

#endif
