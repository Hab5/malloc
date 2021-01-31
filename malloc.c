#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <bits/mman-linux.h> // LINUX FUCKERY, REMOVE IN MACOS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define FALSE           0
#define TRUE            1
 
#define ALIGNEMENT      4
#define ALIGN(size)     (((size) + (ALIGNEMENT-1)) & ~(ALIGNEMENT-1))
 
#define PAGESIZE        (size_t)getpagesize()
 
#define TINY            1
#define TINY_PAGE       (PAGESIZE * 48) // 1117 tiny allocations (196kb) 
#define TINY_LIMIT      128
 
#define SMALL           2
#define SMALL_PAGE      (PAGESIZE * 256) // 978 small allocations (1mb)
#define SMALL_LIMIT     1024
 
#define LARGE           3
 
#define PROT            PROT_READ | PROT_WRITE
#define MAP             MAP_ANON | MAP_PRIVATE
 
typedef struct          s_heap {
    size_t              size;
    size_t              size_from_origin;
    char                *data;
    int                 heap_type;
    short int           free;
    struct s_heap       *next;
    struct s_heap       *prev;
}                       t_heap;

typedef struct          s_arena {
    t_heap              *tiny;
    t_heap              *small;
    t_heap              *large;
}                       t_arena;

static t_arena          g_arena;
 
void hexdump_print(char ascii[]) {
    int i = 0;
 
    printf("\033[0;90m| \033[0m");
    for (i = 0; ascii[i]; i++)
        if (ascii[i] != '.')
            printf("\033[0;32m%c\033[0m", ascii[i]);
        else
            printf("\033[0;90m%c\033[0m", ascii[i]);
    printf("\033[0;90m\n\033[0m");
}

void hexdump(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    
    printf("\n\033[0;90m      | \033[0;31m");
    printf(" 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F ");
    printf("\033[0;90m|\033[0;31m 0123456789ABCDEF\033[0;33m\n");
    for (i = 0; i < size; ++i) {
        if (i % 16 == 0)
            printf("\033[0;31m%05x\033[0;90m | \033[0m", i);
        if (((unsigned char*)data)[i] != '\0')
            printf("\033[0;32m%02x \033[0m", ((unsigned char*)data)[i]);
        else
            printf("\033[0;90m%02x \033[0m", ((unsigned char*)data)[i]);
       
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
            ascii[i % 16] = ((unsigned char*)data)[i];
        else
            ascii[i % 16] = '.';
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf("");
            if ((i+1) % 16 == 0)
                hexdump_print(ascii);
            else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8)
                    printf(" ");
                for (j = (i+1) % 16; j < 16; ++j)
                    printf("   ");
                hexdump_print(ascii);
            }
        }
    }
}

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
        printf("\t  distance_next_block: %d \n", (block->next - block)*sizeof(*block));
        block = block->next;
    }
 
    printf("\nHeap size: %d Bytes || Metadata Size: %d Bytes\n",
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
 
int main(void) {
    
    // char *ptr1 = ft_malloc(16);
    // strcpy(ptr1, "[tiny malloc 1]");

    // char *ptr2 = ft_malloc(16);
    // strcpy(ptr2, "[tiny malloc 2]");

    // char *ptr3 = ft_malloc(16);
    // strcpy(ptr3, "[tiny malloc 3]");

    // char *ptr4 = ft_malloc(16);
    // strcpy(ptr4, "[tiny malloc 4]");

    // char *ptr5 = ft_malloc(16);
    // strcpy(ptr5, "[tiny malloc 5]");

    // ft_free(ptr3);
    // ft_free(ptr4);

    // char *ptr6 = ft_malloc(16);
    // strcpy(ptr6, "[tiny malloc 6]");

    // char *tiny_ptr = ft_malloc(128);
    // strcpy(tiny_ptr, "begin");

    // char *tiny_ptr1 = NULL;
    // for (int i = 0; i < 1117; i++) {
    //     if (tiny_ptr1 = ft_malloc(128))
    //         strcpy(tiny_ptr1, "0123456789ABCDEF");
    //     else
    //         printf("Malloc returned NULL.\n");
    // }

    // char *small_ptr = ft_malloc(1024);
    // strcpy(small_ptr, "begin");

    char *small_ptr1 = NULL;
    for (int i = 0; i < 3; i++) {
        if (small_ptr1 = ft_malloc(1024))
            strcpy(small_ptr1, "SMALL AF BOI");
        else
            printf("Malloc returned NULL.\n");
    }

    char *ptr1 = ft_malloc(2000);
    strcpy(ptr1, "[large malloc 1]");
    ft_free(ptr1);
    char *ptr2 = ft_malloc(2200000);
    strcpy(ptr2, "[large malloc 2]");

    char *ptr3 = ft_malloc(2200000);
    strcpy(ptr3, "[large malloc 3]");

    ft_free(ptr3);
    char *ptr4 = ft_malloc(2200000);
    strcpy(ptr4, "[large malloc 4]");
    // ft_free(ptr4);
    ft_free(ptr2);

    print_heap(g_arena.small);
    print_heap(g_arena.large);
    
    // hexdump(
    //     (t_heap *)(tiny_ptr - sizeof(t_heap)), 
    //     ((t_heap *)(tiny_ptr1 - sizeof(t_heap)))->size_from_origin
    // ); 

    // hexdump(
    //     (t_heap *)(small_ptr - sizeof(t_heap)), 
    //     ((t_heap *)(small_ptr1 - sizeof(t_heap)))->size_from_origin
    // ); 

}