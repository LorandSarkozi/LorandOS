#ifndef _FRAME_ALLOCATOR_H_
#define _FRAME_ALLOCATOR_H_

#include "main.h"
#include "string.h"

#define PAGE_SIZE           4096
#define FRAME_SIZE          PAGE_SIZE
#define FRAMES_PER_BITMAP   64

typedef struct _FRAME_ALLOCATOR
{
    PVOID base_address;
    QWORD total_frames;
    QWORD used_frames;
    QWORD* bitmap;
    QWORD bitmap_size;
} FRAME_ALLOCATOR, *PFRAME_ALLOCATOR;

BOOLEAN frame_init(PFRAME_ALLOCATOR allocator, PVOID base_addr, QWORD total_memory);

QWORD frame_alloc(PFRAME_ALLOCATOR allocator);

BOOLEAN frame_free(PFRAME_ALLOCATOR allocator, QWORD frame_number);

BOOLEAN frame_is_allocated(PFRAME_ALLOCATOR allocator, QWORD frame_number);

QWORD frame_get_used_count(PFRAME_ALLOCATOR allocator);

QWORD frame_get_free_count(PFRAME_ALLOCATOR allocator);

PVOID frame_to_address(PFRAME_ALLOCATOR allocator, QWORD frame_number);

QWORD address_to_frame(PFRAME_ALLOCATOR allocator, PVOID address);

#endif
