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
    PrintToScreen("Page allocator not implemented");
    return FALSE;
}

BOOLEAN test_heap_allocator(void)
{
    PrintToScreen("Allocating frame...");
    QWORD heap_frame = frame_alloc(&gFrameAllocator);
    if (heap_frame == (QWORD)-1)
    {
        PrintToScreen("ERROR: alloc failed");
        return FALSE;
    }
    PrintToScreen("Frame allocated OK");
    
    PrintToScreen("Freeing frame...");
    if (!frame_free(&gFrameAllocator, heap_frame))
    {
        PrintToScreen("ERROR: free failed");
        return FALSE;
    }
    
    PrintToScreen("Heap test PASSED");
    return TRUE;
}

BOOLEAN test_frame_allocator(void)
{
    PrintToScreen("Allocating frame...");
    QWORD frame1 = frame_alloc(&gFrameAllocator);
    if (frame1 == (QWORD)-1)
    {
        PrintToScreen("ERROR: alloc failed");
        return FALSE;
    }
    PrintToScreen("Frame allocated OK");

    PrintToScreen("Freeing frame...");
    if (!frame_free(&gFrameAllocator, frame1))
    {
        PrintToScreen("ERROR: free failed");
        return FALSE;
    }

    PrintToScreen("Frame test PASSED");
    return TRUE;
}

void memory_tests_register(void)
{
    PrintToScreen("Registering frame test");
    test_framework_register("frame", test_frame_allocator);
    Log("Registering heap test");
    test_framework_register("heap", test_heap_allocator);
    Log("Registering page test");
    test_framework_register("page", test_page_allocator);
    Log("All tests registered");
}
