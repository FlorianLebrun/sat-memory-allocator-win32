#include <sat/allocator.h>
#include "./utils.h"

void test_meta_alloc() {
  printf("------------------ Test meta alloc ------------------\n");
  void* ptr;

  ptr = sat_malloc_ex(100, 0);
  sat_free(ptr);
}

void test_buffer_alloc() {
  printf("------------------ Test buffer alloc ------------------\n");
  void* ptr;
  
  ptr = sat_malloc(0);
  sat_free(ptr);

  ptr = sat_malloc(10);
  ((char*)ptr)[10] = 1;
  sat_get_contoller()->checkObjectsOverflow();
  sat_free(ptr);
}
