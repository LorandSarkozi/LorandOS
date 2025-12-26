#ifndef _PAGE_ALLOCATOR_H_
#define _PAGE_ALLOCATOR_H_

#include "main.h"
#include "string.h"
#include "frame_allocator.h"

#define VIRTUAL_MEMORY_SIZE     (1 * 1024 * 1024)
#define MAX_PAGE_ENTRIES        (VIRTUAL_MEMORY_SIZE / PAGE_SIZE)

typedef struct _PAGE_ENTRY
{
    PVOID virtual_address;
    QWORD physical_frame;
    BOOLEAN is_allocated;
    BOOLEAN is_mapped;
} PAGE_ENTRY, *PPAGE_ENTRY;

typedef struct _PAGE_ALLOCATOR
{
    PVOID virtual_base;
    PFRAME_ALLOCATOR frame_allocator;
    PAGE_ENTRY* entries;
    QWORD max_entries;
    QWORD allocated_count;
} PAGE_ALLOCATOR, *PPAGE_ALLOCATOR;

BOOLEAN page_init(PPAGE_ALLOCATOR allocator, PVOID virtual_base, PFRAME_ALLOCATOR frame_allocator);

PVOID page_alloc(PPAGE_ALLOCATOR allocator, QWORD physical_frame_hint);

BOOLEAN page_free(PPAGE_ALLOCATOR allocator, PVOID virtual_address, BOOLEAN free_physical_frame);

QWORD page_get_physical_frame(PPAGE_ALLOCATOR allocator, PVOID virtual_address);

BOOLEAN page_is_allocated(PPAGE_ALLOCATOR allocator, PVOID virtual_address);

#endif
