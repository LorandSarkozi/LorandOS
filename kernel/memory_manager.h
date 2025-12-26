#ifndef _MEMORY_MANAGER_H_
#define _MEMORY_MANAGER_H_

#include "main.h"
#include "frame_allocator.h"
#include "page_allocator.h"

#define MEMORY_BASE_ADDRESS     0x100000
#define MEMORY_SIZE             (16 * 1024 * 1024)

void Memory_Init(void);

PFRAME_ALLOCATOR Memory_GetFrameAllocator(void);
PPAGE_ALLOCATOR Memory_GetPageAllocator(void);

#endif
