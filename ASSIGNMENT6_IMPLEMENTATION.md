# Assignment 6: Memory Management Implementation

## Overview
Complete implementation of physical memory, virtual memory, and heap memory allocators with comprehensive testing framework.

## Files Created

### Memory Allocators
1. **frame_allocator.h/c** - Physical memory frame allocator
   - `frame_init()` - Initialize physical memory bitmap
   - `frame_alloc()` - Allocate a physical frame
   - `frame_free()` - Free a physical frame
   - `frame_is_allocated()` - Check if frame is allocated
   - `frame_get_used_count()` / `frame_get_free_count()` - Statistics

2. **page_allocator.h/c** - Virtual memory page allocator
   - `page_init()` - Initialize virtual memory manager
   - `page_alloc()` - Allocate virtual page (with optional physical frame hint)
   - `page_free()` - Free virtual page (with option to keep/free physical frame)
   - `page_get_physical_frame()` - Get backing physical frame for virtual page
   - `page_is_allocated()` - Check if virtual page is allocated

3. **heap_allocator.c** - Heap memory allocator
   - `heap_create()` - Create heap at specified address
   - `heap_alloc()` - Allocate memory from heap
   - `heap_free()` - Free memory in heap with coalescing
   - `heap_destroy()` - Destroy entire heap
   - `heap_get_used_size()` / `heap_get_free_size()` - Statistics

### Test Framework
4. **test_framework.h/c** - Generic test infrastructure
   - `test_framework_init()` - Initialize test system
   - `test_framework_register()` - Register new test
   - `test_framework_run()` - Run specific test by name
   - `test_framework_run_all()` - Run all registered tests
   - `test_framework_list()` - List all available tests

5. **memory_tests.h/c** - Memory allocator tests
   - `test_page_allocator()` - Page allocator test (as per assignment spec)
   - `test_heap_allocator()` - Heap allocator test (as per assignment spec)
   - `test_frame_allocator()` - Frame allocator test (bonus)

6. **memory_manager.h/c** - Memory subsystem initialization
   - Initializes all memory allocators
   - Registers memory tests
   - Provides global accessor functions

## CLI Commands Added

### test_list
Lists all registered tests.
```
> test_list
Available tests (3):
  - page
  - heap
  - frame
```

### test_run <name>
Runs a specific test by name.
```
> test_run page
Running test: page
  Allocating first page...
  Writing test data to first page...
  Getting physical frame from first page...
  Allocating second page with same frame...
  Verifying data in both pages...
  Unmapping first page without freeing frame...
  Verifying data still accessible via second page...
  Freeing second page...
  Page allocator test passed!
[PASS]
```

### test_run_all
Runs all registered tests sequentially.
```
> test_run_all
Running 3 tests...
Running: page
[PASS]
Running: heap
[PASS]
Running: frame
[PASS]
Tests: 3 passed, 0 failed
```

## Test Specifications

### Page Allocator Test (test_run page)
As per assignment requirements:
1. Allocate a memory page using `page_alloc()`
2. Write data to the buffer (0xDEADBEEF + counter pattern)
3. Allocate a new memory page using `page_alloc()` specifying the same frame
4. Check that data in both pages is identical
5. Unmap the first page without freeing the backing physical frame
6. Verify that data can still be accessed using the second allocation

### Heap Allocator Test (test_run heap)
As per assignment requirements:
1. Create a heap using `heap_create()`
2. Allocate multiple heap entries (10 entries) using `heap_alloc()`
3. Write distinct data in each entry (counter pattern: i*1000 + j)
4. Validate the data in entries and check if values correspond
5. Free the entries using `heap_free()`
6. Delete the heap using `heap_destroy()`

### Frame Allocator Test (test_run frame)
Bonus test:
1. Allocate multiple physical frames
2. Verify frames are distinct
3. Free frames
4. Verify reallocation works correctly

## Memory Configuration

- **Physical Memory Base**: 0x100000 (1 MB)
- **Physical Memory Size**: 16 MB
- **Virtual Memory Base**: 0x80000000 (2 GB)
- **Virtual Memory Size**: 256 MB address space
- **Page/Frame Size**: 4096 bytes (4 KB)

## Integration

Modified files:
- `main.c` - Added Memory_Init() call during boot
- `cli.c` - Added test_run, test_list, test_run_all commands
- `cli.h` - Added command handler declarations

## Build Instructions

Add the following source files to your kernel project:
- frame_allocator.c
- page_allocator.c
- heap_allocator.c
- test_framework.c
- memory_tests.c
- memory_manager.c

## Usage Example

1. Boot MiniOS
2. Type `test_list` to see available tests
3. Type `test_run page` to test page allocator
4. Type `test_run heap` to test heap allocator
5. Type `test_run_all` to run all memory tests

## Key Features

- **Physical Memory Management**: Bitmap-based frame allocator with O(n) allocation
- **Virtual Memory Management**: Page table simulation with frame hints for shared memory
- **Heap Memory Management**: First-fit allocator with automatic coalescing
- **Test Framework**: Extensible test infrastructure for future tests
- **CLI Integration**: Interactive testing via command-line interface

## Implementation Notes

1. Frame allocator reserves frames for its own bitmap storage
2. Page allocator can share physical frames between multiple virtual pages
3. Heap uses linked list of blocks with immediate coalescing on free
4. All allocators include error checking and return appropriate status codes
5. Tests validate both functionality and data integrity

## Assignment Requirements ✓

- ✓ Physical memory frame allocator (frame_*)
- ✓ Virtual memory page allocator (page_*)
- ✓ Heap memory allocator (heap_*)
- ✓ Tests for each function implemented
- ✓ Console commands: test_run <name>
- ✓ Console command: test_list
- ✓ Console command: test_run_all
- ✓ Page allocator test as specified
- ✓ Heap allocator test as specified

## Optional Features (Not Implemented)

- Lazy mapping support (OPTIONAL requirement)
- E820 memory map detection (OPTIONAL requirement)

These can be added in future iterations if needed.
