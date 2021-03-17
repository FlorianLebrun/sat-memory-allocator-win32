#include <sat-memory-allocator/allocator.h>
#include <sat-memory-allocator/stack_analysis.h>
#include <sat-threads/timing.hpp>

struct TestStackMarker : sat::IStackMarker {
   int level;
   bool spanned;
   virtual void getSymbol(char* buffer, int size) override {
      sprintf_s(buffer, size, "beacon.%s.%d", spanned ? "span" : "tag", level);
   }
};

struct TestStackBeacon : sat::StackBeacon {
   int level;
   bool spanned;
   TestStackBeacon() {
      spanned = true;
   }
   virtual bool isSpan() override {
      return spanned;
   }
   virtual void createrMarker(sat::StackMarker& buffer) override {
      auto marker = buffer.as<TestStackMarker>();
      marker->level = this->level;
      marker->spanned = this->spanned;
   }
};

void test_stack_print() {
   printf("\n\n---------------- test_stack_print ----------------\n\n");
   struct _internal {
      bool spanned;
      _internal() {
         this->spanned = 1;
      }
      void call(int level) {
         sat::StackBeaconHolder<TestStackBeacon> beacon;
         if (level % 4 == 2) {
            beacon.beacon.level = level;
            beacon.beacon.spanned = this->spanned;
            this->spanned = !this->spanned;
            beacon.deploy();
         }
         if (level > 0) {
            call(level - 1);
            //call(level - 2);
         }
         else {
            auto stackstamp = sat::analysis::getCurrentStackStamp();
            sat_print_stackstamp(stackstamp);
         }
      }
   };
   printf("\nstack:\n");
   _internal().call(16);
   printf("\nstack:\n");
   _internal().call(20);
}

void test_deep_basic_call(int count) {
   printf("\n\n---------------- test_deep_basic_call ----------------\n\n");
   struct _internal {
      static void calltest(int level) {
         if (level > 0) {
            calltest(level - 1);
         }
         else {
            sat::analysis::getCurrentStackStamp();
         }
      }
   };
   sat::timing::Chrono c;
   for (int i = 0; i < count; i++) {
      _internal::calltest(25 + (i & 3));
   }
   printf("[sat-stack-analysis] stack trace time = %lg us\n", c.GetDiffDouble(sat::timing::Chrono::US));
}

void test_stack_analysis() {
   test_stack_print();
   test_deep_basic_call(10);
}
