#include "test_framework.h"
#include "frame_allocator.h"
#include "page_allocator.h"
#include "heap_allocator.h"
#include "logging.h"
#include "screen.h"
#include "cli.h"

extern FRAME_ALLOCATOR gFrameAllocator;
extern PAGE_ALLOCATOR gPageAllocator;

static void PrintToScreen(const char* msg)
{
    extern PSCREEN gVideo;
    extern CLI_STATE gCliState;
    
    DWORD pos = gCliState.cursorPosition;
    
    while (*msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *msg;
        gVideo[pos].color = 0x0A;
        pos++;
        msg++;
    }
    
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    gCliState.cursorPosition = pos;
    CursorPosition(pos);
}

BOOLEAN test_page_allocator(void)
{
    PrintToScreen("Test 1: Alloc 2 pages");
    PVOID page1 = page_alloc(&gPageAllocator, (QWORD)-1);
    PVOID page2 = page_alloc(&gPageAllocator, (QWORD)-1);
    
    if (!page1 || !page2)
    {
        PrintToScreen("ERROR: page alloc failed");
        return FALSE;
    }
    
    if (page1 == page2)
    {
        PrintToScreen("ERROR: same page twice");
        return FALSE;
    }
    PrintToScreen("OK: Got different pages");

    PrintToScreen("Test 2: Check frames");
    QWORD frame1 = page_get_physical_frame(&gPageAllocator, page1);
    QWORD frame2 = page_get_physical_frame(&gPageAllocator, page2);
    
    if (frame1 == (QWORD)-1 || frame2 == (QWORD)-1)
    {
        PrintToScreen("ERROR: no physical frame");
        return FALSE;
    }
    
    if (frame1 == frame2)
    {
        PrintToScreen("ERROR: same phys frame");
        return FALSE;
    }
    PrintToScreen("OK: Different phys frames");

    PrintToScreen("Test 3: Free pages");
    page_free(&gPageAllocator, page1, TRUE);
    page_free(&gPageAllocator, page2, TRUE);

    PrintToScreen("Page test PASSED");
    return TRUE;
}

BOOLEAN test_heap_allocator(void)
{
    PrintToScreen("Test 1: Create heap");
    QWORD heap_frame = frame_alloc(&gFrameAllocator);
    if (heap_frame == (QWORD)-1)
    {
        PrintToScreen("ERROR: no frame for heap");
        return FALSE;
    }
    
    PVOID heap_base = frame_to_address(&gFrameAllocator, heap_frame);
    PHEAP heap = heap_create(heap_base, FRAME_SIZE);
    if (!heap)
    {
        PrintToScreen("ERROR: heap_create failed");
        return FALSE;
    }
    PrintToScreen("OK: Heap created");
    
    PrintToScreen("Test 2: Alloc & write data");
    DWORD* ptr1 = (DWORD*)heap_alloc(heap, sizeof(DWORD) * 4);
    DWORD* ptr2 = (DWORD*)heap_alloc(heap, sizeof(DWORD) * 4);
    
    if (!ptr1 || !ptr2)
    {
        PrintToScreen("ERROR: heap_alloc failed");
        return FALSE;
    }
    
    ptr1[0] = 0xDEADBEEF;
    ptr2[0] = 0xCAFEBABE;
    
    if (ptr1[0] != 0xDEADBEEF || ptr2[0] != 0xCAFEBABE)
    {
        PrintToScreen("ERROR: data corrupted");
        return FALSE;
    }
    PrintToScreen("OK: Data integrity");
    
    PrintToScreen("Test 3: Free & cleanup");
    heap_free(heap, ptr1);
    heap_free(heap, ptr2);
    heap_destroy(heap);
    frame_free(&gFrameAllocator, heap_frame);

    PrintToScreen("Heap test PASSED");
    return TRUE;
}

BOOLEAN test_frame_allocator(void)
{
    PrintToScreen("Test 1: Alloc 2 frames");
    QWORD frame1 = frame_alloc(&gFrameAllocator);
    QWORD frame2 = frame_alloc(&gFrameAllocator);
    
    if (frame1 == (QWORD)-1 || frame2 == (QWORD)-1)
    {
        PrintToScreen("ERROR: alloc failed");
        return FALSE;
    }
    
    if (frame1 == frame2)
    {
        PrintToScreen("ERROR: same frame twice");
        return FALSE;
    }
    PrintToScreen("OK: Got different frames");

    PrintToScreen("Test 2: Free and realloc");
    if (!frame_free(&gFrameAllocator, frame1))
    {
        PrintToScreen("ERROR: free failed");
        return FALSE;
    }
    
    QWORD frame3 = frame_alloc(&gFrameAllocator);
    if (frame3 != frame1)
    {
        PrintToScreen("ERROR: didn't reuse freed");
        return FALSE;
    }
    PrintToScreen("OK: Reused freed frame");
    
    frame_free(&gFrameAllocator, frame2);
    frame_free(&gFrameAllocator, frame3);

    PrintToScreen("Frame test PASSED");
    return TRUE;
}

void memory_tests_register(void)
{
    test_framework_register("frame", test_frame_allocator);
    test_framework_register("heap", test_heap_allocator);
    test_framework_register("page", test_page_allocator);
}
