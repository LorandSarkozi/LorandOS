#ifndef _HEAP_ALLOCATOR_H_
#define _HEAP_ALLOCATOR_H_

#include "main.h"
#include "string.h"

typedef struct _HEAP_BLOCK
{
    QWORD size;
    QWORD is_free;
    struct _HEAP_BLOCK* next;
    struct _HEAP_BLOCK* prev;
} HEAP_BLOCK, * PHEAP_BLOCK;

#define HEAP_BLOCK_HEADER_SIZE sizeof(HEAP_BLOCK)

typedef struct _HEAP
{
    PVOID base_address;
    QWORD total_size;
    QWORD used_size;
    PHEAP_BLOCK first_block;
} HEAP, * PHEAP;

PHEAP heap_create(PVOID base_addr, QWORD size);

PVOID heap_alloc(PHEAP heap, QWORD size);

BOOLEAN heap_free(PHEAP heap, PVOID ptr);

BOOLEAN heap_destroy(PHEAP heap);

QWORD heap_get_used_size(PHEAP heap);
QWORD heap_get_free_size(PHEAP heap);

#endif
