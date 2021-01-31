#include "../includes/malloc.h"
 
void print_heap(t_heap *heap) {
    t_heap *block = heap;
    int heap_size = 0;
    int metadata_size = 0;
    int i = 0;
    
    if (!heap)
        return;
    printf("\n> Dumping heap\n");
    while (block->next) {
        printf("\nBlock #%d: data: %s\n\t  free: %d\n\t  size: %zu \n\t  data_size: %lu \n",
            i++, block->data, block->free, block->size, strlen(block->data));
        heap_size += block->size;
        metadata_size++;
        printf("\t  heap_type: %d\n", block->heap_type);
        printf("\t  size_from_origin: %zu\n", block->size_from_origin);
        printf("\t  addr: %p\n", block);
        printf("\t  prev: %p\n", block->prev);
        printf("\t  next: %p\n", block->next);
        printf("\t  distance_next_block: %ld \n", (block->next - block)*sizeof(*block));
        block = block->next;
    }
 
    printf("\nHeap size: %d Bytes || Metadata Size: %ld Bytes\n",
            heap_size, metadata_size*sizeof(t_heap));
}

void defrag(t_heap *block) {
    int defrag;
 
    while (block->next) {
        defrag = 0;
        if (block->next && (block->free && block->next->free)) {
            if (block->next->next) {
                defrag = 1;
                block->next = block->next->next;
                block->size += block->next->size + sizeof(t_heap);
                block->next->prev = block;
            }
        }
        block = defrag == 1 ? block : block->next;
    }
}

void ft_free(void *ptr) {
    t_heap *block;
    if (!ptr)
        return;
    block = (t_heap *)(ptr - sizeof(t_heap));
    
    if (block->heap_type == LARGE) {
        if (block->prev) {
            block->prev->next = block->next;
            if (block->next != block+1)
                block->next->prev = block->prev;
            else
                block->prev->next = block->prev+1;
            munmap(block, block->size/PAGESIZE+1);
        } else {
            if (block->next != block+1)
                g_arena.large = block->next;
            else
                g_arena.large = NULL;
            munmap(block, block->size/PAGESIZE+1);
        }
    } else {
        block->free = TRUE;
        for (int i = 0; *(char *)(ptr + i); i++) {
            *((char *)(ptr + i)) = '\0';
        }
    }
}
 
void *find_block(size_t size, t_heap *block, size_t page_size, int heap_type) {
    if (!block)
        return (NULL);
    defrag(block);

    while (block->next) { /* Try to find a free block big enough for the data */
        if (block->free && block->size >= size) {
            block->free = FALSE;
            block->data = (char *)block + sizeof(t_heap);
            return block->data;
        }
        block = block->next;
    }

    if ((block->prev->size_from_origin + size + sizeof(t_heap)) >= page_size) // Heap Overflow
        return(NULL);
    // Else, populate the last block (which is empty)
    block->free = FALSE;
    block->size = size;
    block->size_from_origin = size + sizeof(t_heap) + block->prev->size_from_origin;
    block->heap_type = heap_type;
    block->data = (char*)block + sizeof(t_heap);

    block->next = (t_heap*)((char *)(block + 1) + size);
    block->next->free = TRUE;
    block->next->prev = block;
    block->next->next = NULL;
    return block->data;
}

void *init_heap(size_t size, t_heap **block, size_t page_size, int heap_type) {
    (*block) = (t_heap*)mmap(NULL, page_size, PROT, MAP, -1, 0);
    if (*block == MAP_FAILED)
        return (NULL);
    (*block)->size = size;
    (*block)->size_from_origin = size + sizeof(t_heap);
    (*block)->heap_type = heap_type;
    (*block)->free = FALSE;
    (*block)->data = ((char *)(*block) + sizeof(t_heap));
    (*block)->prev = NULL;
    
    if (heap_type != LARGE)
        (*block)->next = (t_heap*)((char*)(*block + 1) + size);
    else
        (*block)->next = *block+1;
    (*block)->next->prev = *block;
    (*block)->next->free = TRUE;
    (*block)->next->next = NULL;
    return (*block)->data;
}

void *allocate_large(size_t size, t_heap *block, size_t page_size) {
    t_heap *tmp;
    if (!block)
        return (NULL);
    while (block->next) {
        tmp = block;
        block = block->next;
    }
    block = (t_heap*)mmap(NULL, page_size, PROT, MAP, -1, 0);
    block->prev = tmp;
    block->prev->next = block;
    // block->next->prev = block;

    block->free = FALSE;
    block->size = size;
    block->data = (char*)block + sizeof(t_heap);
    block->heap_type = LARGE;
    block->next = block+1;
    block->next->free = TRUE;
    block->next->next = NULL;
    return block->data;
}

void *allocate(size_t size, int heap) {
    if (heap == TINY) {
        if (!g_arena.tiny) // If no arena currently exist, initialize one.
            return init_heap(size, &g_arena.tiny, TINY_PAGE, TINY);
        else
            return find_block(size, g_arena.tiny, TINY_PAGE, TINY);
    }
    else if (heap == SMALL) {
        if (!g_arena.small)
            return init_heap(size, &g_arena.small, SMALL_PAGE, SMALL);
        else
            return find_block(size, g_arena.small, SMALL_PAGE, SMALL);
    }
    else if (heap == LARGE) {
        if (!g_arena.large)
            return init_heap(size, &g_arena.large, size/PAGESIZE+1, LARGE);
        else 
            return allocate_large(size, g_arena.large, size/PAGESIZE+1);
        ;
    } //TODO
 
    return NULL;
}
 
void *ft_malloc(size_t size) {
    if (size <= 0)
        return (NULL);
    else if (ALIGN(size) <= TINY_LIMIT)
        return (allocate(ALIGN(size), TINY));
    else if (ALIGN(size <= SMALL_LIMIT))
        return (allocate(ALIGN(size), SMALL));
    else
        return allocate(ALIGN(size), LARGE); // TODO LARGE
    return NULL;
}