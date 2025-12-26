#ifndef _MEMORY_TESTS_H_
#define _MEMORY_TESTS_H_

#include "main.h"

void memory_tests_register(void);

BOOLEAN test_page_allocator(void);
BOOLEAN test_heap_allocator(void);
BOOLEAN test_frame_allocator(void);

#endif
