#include <sat-memory-allocator/sat-memory-allocator.h>
#include "./utils.h"

struct TestStackMarker : SAT::IStackMarker {
   int level;
   virtual void getSymbol(char* buffer, int size) override {
      sprintf_s(buffer, size, "beacon.%d", level);
   }
};

struct TestStackBeacon : SAT::StackBeacon {
   int level;
   virtual void createrMarker(SAT::StackMarker& buffer) override {
      auto marker = buffer.as<TestStackMarker>();
      marker->level = this->level;
   }
};

void test_stack_print() {
   struct _internal {
      static void beacon_calltest(int level) {
         SAT::StackBeaconHolder<TestStackBeacon> beacon;
         beacon.beacon.level = level;
         beacon.deploy();
         if (level > 10) {
            beacon_calltest(level - 1);
         }
         else if (level > 0) {
            basic_calltest(level - 1);
         }
         else {
            auto stackstamp = sat_current_thread()->getStackStamp();
            sat_print_stackstamp(stackstamp);
         }
      }

      static void basic_calltest(int level) {
         if (level > 0) {
            basic_calltest(level - 1);
         }
         else {
            auto stackstamp = sat_current_thread()->getStackStamp();
            sat_print_stackstamp(stackstamp);
         }
      }
   };
   _internal::beacon_calltest(25);
}

void test_deep_basic_call() {
   struct _internal {
      static void calltest(int level) {
         if (level > 0) {
            calltest(level - 1);
         }
         else {
            sat_current_thread()->getStackStamp();
         }
      }
   };
   int count = 1000;
   Chrono c;
   c.Start();
   for (int i = 0; i < count; i++) {
      _internal::calltest(25);
   }
   printf("[SAT-stack-analysis] stack trace time = %g us\n", c.GetDiffFloat(Chrono::US) / float(count));
}

void test_stack_analysis() {

   test_stack_print();
}
