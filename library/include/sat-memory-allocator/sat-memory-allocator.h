
#ifndef _sat_memory_allocator_win32_h_
#define _sat_memory_allocator_win32_h_

#include "./_base.h"
#include "./_types.h"
#include "./_stack_analysis.h"
#include "./_objects.h"
#include "./_system.h"

extern"C" void _satmalloc_init();

// Allocation API
extern"C" void* sat_malloc(size_t size);
extern"C" void* sat_malloc_ex(size_t size, uint64_t meta);
extern"C" void* sat_calloc(size_t count, size_t size);
extern"C" void* sat_realloc(void* ptr, size_t size, SAT::tp_realloc default_realloc = 0);
extern"C" size_t sat_msize(void* ptr, SAT::tp_msize default_msize = 0);
extern"C" void sat_free(void* ptr, SAT::tp_free default_free = 0);
extern"C" void sat_flush_cache();

// Stack marking API
extern"C" void sat_begin_stack_beacon(SAT::StackBeacon* beacon);
extern"C" void sat_end_stack_beacon(SAT::StackBeacon* beacon);
extern"C" void sat_print_stackstamp(uint64_t stackstamp);

// Reflexion API
extern"C" SAT::IController* sat_get_contoller();
extern"C" SAT::tEntry* sat_get_table();
extern"C" SAT::IThread* sat_current_thread();

extern"C" bool sat_has_address(void* ptr);
extern"C" bool sat_get_address_infos(void* ptr, SAT::tpObjectInfos infos = 0);

#endif
