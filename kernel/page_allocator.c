#include "page_allocator.h"

BOOLEAN page_init(PPAGE_ALLOCATOR allocator, PVOID virtual_base, PFRAME_ALLOCATOR frame_allocator)
{
    if (!allocator || !virtual_base || !frame_allocator)
    {
        return FALSE;
    }

    allocator->virtual_base = virtual_base;
    allocator->frame_allocator = frame_allocator;
    allocator->max_entries = MAX_PAGE_ENTRIES;
    allocator->allocated_count = 0;

    QWORD entries_frame = frame_alloc(frame_allocator);
    if (entries_frame == (QWORD)-1)
    {
        return FALSE;
    }

    allocator->entries = (PAGE_ENTRY*)frame_to_address(frame_allocator, entries_frame);
    
    for (QWORD i = 0; i < allocator->max_entries; i++)
    {
        allocator->entries[i].virtual_address = (PVOID)((QWORD)virtual_base + (i * PAGE_SIZE));
        allocator->entries[i].physical_frame = (QWORD)-1;
        allocator->entries[i].is_allocated = FALSE;
        allocator->entries[i].is_mapped = FALSE;
    }

    return TRUE;
}

PVOID page_alloc(PPAGE_ALLOCATOR allocator, QWORD physical_frame_hint)
{
    if (!allocator)
    {
        return NULL;
    }

    QWORD physical_frame;
    
    if (physical_frame_hint != (QWORD)-1 && frame_is_allocated(allocator->frame_allocator, physical_frame_hint))
    {
        physical_frame = physical_frame_hint;
    }
    else
    {
        physical_frame = frame_alloc(allocator->frame_allocator);
        if (physical_frame == (QWORD)-1)
        {
            return NULL;
        }
    }

    for (QWORD i = 0; i < allocator->max_entries; i++)
    {
        if (!allocator->entries[i].is_allocated)
        {
            allocator->entries[i].physical_frame = physical_frame;
            allocator->entries[i].is_allocated = TRUE;
            allocator->entries[i].is_mapped = TRUE;
            allocator->allocated_count++;
            
            return allocator->entries[i].virtual_address;
        }
    }

    if (physical_frame_hint == (QWORD)-1)
    {
        frame_free(allocator->frame_allocator, physical_frame);
    }

    return NULL;
}

BOOLEAN page_free(PPAGE_ALLOCATOR allocator, PVOID virtual_address, BOOLEAN free_physical_frame)
{
    if (!allocator || !virtual_address)
    {
        return FALSE;
    }

    QWORD offset = (QWORD)virtual_address - (QWORD)allocator->virtual_base;
    QWORD page_index = offset / PAGE_SIZE;

    if (page_index >= allocator->max_entries)
    {
        return FALSE;
    }

    if (!allocator->entries[page_index].is_allocated)
    {
        return FALSE;
    }

    if (free_physical_frame && allocator->entries[page_index].physical_frame != (QWORD)-1)
    {
        frame_free(allocator->frame_allocator, allocator->entries[page_index].physical_frame);
    }

    allocator->entries[page_index].is_allocated = FALSE;
    allocator->entries[page_index].is_mapped = FALSE;
    allocator->entries[page_index].physical_frame = (QWORD)-1;
    allocator->allocated_count--;

    return TRUE;
}

QWORD page_get_physical_frame(PPAGE_ALLOCATOR allocator, PVOID virtual_address)
{
    if (!allocator || !virtual_address)
    {
        return (QWORD)-1;
    }

    QWORD offset = (QWORD)virtual_address - (QWORD)allocator->virtual_base;
    QWORD page_index = offset / PAGE_SIZE;

    if (page_index >= allocator->max_entries)
    {
        return (QWORD)-1;
    }

    if (!allocator->entries[page_index].is_allocated)
    {
        return (QWORD)-1;
    }

    return allocator->entries[page_index].physical_frame;
}

BOOLEAN page_is_allocated(PPAGE_ALLOCATOR allocator, PVOID virtual_address)
{
    if (!allocator || !virtual_address)
    {
        return FALSE;
    }

    QWORD offset = (QWORD)virtual_address - (QWORD)allocator->virtual_base;
    QWORD page_index = offset / PAGE_SIZE;

    if (page_index >= allocator->max_entries)
    {
        return FALSE;
    }

    return allocator->entries[page_index].is_allocated;
}
