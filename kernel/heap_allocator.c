#include "heap_allocator.h"

PHEAP heap_create(PVOID base_addr, QWORD size)
{
    if (!base_addr || size < sizeof(HEAP) + sizeof(HEAP_BLOCK) + 16)
    {
        return NULL;
    }

    PHEAP heap = (PHEAP)base_addr;
    heap->base_address = base_addr;
    heap->total_size = size;
    heap->used_size = sizeof(HEAP);

    PHEAP_BLOCK first_block = (PHEAP_BLOCK)((QWORD)base_addr + sizeof(HEAP));
    first_block->size = size - sizeof(HEAP) - HEAP_BLOCK_HEADER_SIZE;
    first_block->is_free = TRUE;
    first_block->next = NULL;
    first_block->prev = NULL;

    heap->first_block = first_block;

    return heap;
}

PVOID heap_alloc(PHEAP heap, QWORD size)
{
    if (!heap || size == 0)
    {
        return NULL;
    }

    QWORD aligned_size = (size + 7) & ~7ULL;

    PHEAP_BLOCK current = heap->first_block;

    while (current)
    {
        if (current->is_free && current->size >= aligned_size)
        {
            if (current->size >= aligned_size + HEAP_BLOCK_HEADER_SIZE + 16)
            {
                PHEAP_BLOCK new_block = (PHEAP_BLOCK)((QWORD)current + HEAP_BLOCK_HEADER_SIZE + aligned_size);
                new_block->size = current->size - aligned_size - HEAP_BLOCK_HEADER_SIZE;
                new_block->is_free = TRUE;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next)
                {
                    current->next->prev = new_block;
                }

                current->next = new_block;
                current->size = aligned_size;
            }

            current->is_free = FALSE;
            heap->used_size += current->size + HEAP_BLOCK_HEADER_SIZE;

            return (PVOID)((QWORD)current + HEAP_BLOCK_HEADER_SIZE);
        }

        current = current->next;
    }

    return NULL;
}

BOOLEAN heap_free(PHEAP heap, PVOID ptr)
{
    if (!heap || !ptr)
    {
        return FALSE;
    }

    PHEAP_BLOCK block = (PHEAP_BLOCK)((QWORD)ptr - HEAP_BLOCK_HEADER_SIZE);

    if (block->is_free)
    {
        return FALSE;
    }

    block->is_free = TRUE;
    heap->used_size -= block->size + HEAP_BLOCK_HEADER_SIZE;

    if (block->next && block->next->is_free)
    {
        block->size += block->next->size + HEAP_BLOCK_HEADER_SIZE;
        block->next = block->next->next;
        if (block->next)
        {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free)
    {
        block->prev->size += block->size + HEAP_BLOCK_HEADER_SIZE;
        block->prev->next = block->next;
        if (block->next)
        {
            block->next->prev = block->prev;
        }
    }

    return TRUE;
}

BOOLEAN heap_destroy(PHEAP heap)
{
    if (!heap)
    {
        return FALSE;
    }

    memset(heap, 0, heap->total_size);
    return TRUE;
}

QWORD heap_get_used_size(PHEAP heap)
{
    return heap ? heap->used_size : 0;
}

QWORD heap_get_free_size(PHEAP heap)
{
    if (!heap)
    {
        return 0;
    }

    QWORD free_size = 0;
    PHEAP_BLOCK current = heap->first_block;

    while (current)
    {
        if (current->is_free)
        {
            free_size += current->size;
        }
        current = current->next;
    }

    return free_size;
}
