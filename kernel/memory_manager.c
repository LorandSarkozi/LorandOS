#include "memory_manager.h"
#include "logging.h"
#include "memory_tests.h"
#include "test_framework.h"

FRAME_ALLOCATOR gFrameAllocator;
PAGE_ALLOCATOR gPageAllocator;

static BYTE gMemoryPool[8 * 1024];

void Memory_Init(void)
{
    Log("Memory Init START");
    
    Log("Frame allocator START");
    if (!frame_init(&gFrameAllocator, (PVOID)gMemoryPool, sizeof(gMemoryPool)))
    {
        Log("Frame allocator FAILED");
        return;
    }
    Log("Frame allocator OK");
    
    Log("Test framework START");
    test_framework_init();
    Log("Test framework OK");
    
    Log("Memory Init COMPLETE");
}

PFRAME_ALLOCATOR Memory_GetFrameAllocator(void)
{
    return &gFrameAllocator;
}

PPAGE_ALLOCATOR Memory_GetPageAllocator(void)
{
    return &gPageAllocator;
}
