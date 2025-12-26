#include "frame_allocator.h"

BOOLEAN frame_init(PFRAME_ALLOCATOR allocator, PVOID base_addr, QWORD total_memory)
{
    if (!allocator || !base_addr || total_memory == 0)
    {
        return FALSE;
    }

    allocator->base_address = base_addr;
    allocator->total_frames = total_memory / FRAME_SIZE;
    allocator->used_frames = 0;
    
    allocator->bitmap_size = (allocator->total_frames + 63) / 64;
    
    if (allocator->bitmap_size > 1024)
    {
        return FALSE;
    }
    
    allocator->bitmap = (QWORD*)base_addr;
    
    for (QWORD i = 0; i < allocator->bitmap_size; i++)
    {
        allocator->bitmap[i] = 0;
    }
    
    QWORD bitmap_frames = (allocator->bitmap_size * sizeof(QWORD) + FRAME_SIZE - 1) / FRAME_SIZE;
    for (QWORD i = 0; i < bitmap_frames; i++)
    {
        QWORD qword_idx = i / 64;
        QWORD bit_idx = i % 64;
        allocator->bitmap[qword_idx] |= (1ULL << bit_idx);
        allocator->used_frames++;
    }
    
    return TRUE;
}

QWORD frame_alloc(PFRAME_ALLOCATOR allocator)
{
    if (!allocator)
    {
        return (QWORD)-1;
    }
    
    for (QWORD i = 0; i < allocator->bitmap_size; i++)
    {
        if (allocator->bitmap[i] != 0xFFFFFFFFFFFFFFFFULL)
        {
            for (QWORD bit = 0; bit < 64; bit++)
            {
                QWORD mask = 1ULL << bit;
                if (!(allocator->bitmap[i] & mask))
                {
                    QWORD frame_number = i * 64 + bit;
                    if (frame_number >= allocator->total_frames)
                    {
                        return (QWORD)-1;
                    }
                    
                    allocator->bitmap[i] |= mask;
                    allocator->used_frames++;
                    return frame_number;
                }
            }
        }
    }
    
    return (QWORD)-1;
}

BOOLEAN frame_free(PFRAME_ALLOCATOR allocator, QWORD frame_number)
{
    if (!allocator || frame_number >= allocator->total_frames)
    {
        return FALSE;
    }
    
    QWORD qword_idx = frame_number / 64;
    QWORD bit_idx = frame_number % 64;
    QWORD mask = 1ULL << bit_idx;
    
    if (!(allocator->bitmap[qword_idx] & mask))
    {
        return FALSE;
    }
    
    allocator->bitmap[qword_idx] &= ~mask;
    allocator->used_frames--;
    
    return TRUE;
}

BOOLEAN frame_is_allocated(PFRAME_ALLOCATOR allocator, QWORD frame_number)
{
    if (!allocator || frame_number >= allocator->total_frames)
    {
        return FALSE;
    }
    
    QWORD qword_idx = frame_number / 64;
    QWORD bit_idx = frame_number % 64;
    QWORD mask = 1ULL << bit_idx;
    
    return (allocator->bitmap[qword_idx] & mask) != 0;
}

QWORD frame_get_used_count(PFRAME_ALLOCATOR allocator)
{
    return allocator ? allocator->used_frames : 0;
}

QWORD frame_get_free_count(PFRAME_ALLOCATOR allocator)
{
    return allocator ? (allocator->total_frames - allocator->used_frames) : 0;
}

PVOID frame_to_address(PFRAME_ALLOCATOR allocator, QWORD frame_number)
{
    if (!allocator || frame_number >= allocator->total_frames)
    {
        return NULL;
    }
    
    return (PVOID)((QWORD)allocator->base_address + (frame_number * FRAME_SIZE));
}

QWORD address_to_frame(PFRAME_ALLOCATOR allocator, PVOID address)
{
    if (!allocator || address < allocator->base_address)
    {
        return (QWORD)-1;
    }
    
    QWORD offset = (QWORD)address - (QWORD)allocator->base_address;
    return offset / FRAME_SIZE;
}
