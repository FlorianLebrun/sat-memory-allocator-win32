#include "./utils.h"

extern void test_meta_alloc();
extern void test_perf_alloc();
extern void test_types_alloc();
extern void test_buffer_alloc();
extern void test_btree();
extern void test_stack_analysis();

int main() {

   //test_buffer_alloc();
   //test_meta_alloc();
   //test_perf_alloc();
   //test_types_alloc();
   //test_btree();
   test_stack_analysis();

   fflush(stdout);
   return 0;
}
