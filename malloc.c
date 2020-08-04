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
#define TINY_PAGE       (PAGESIZE * 2)
#define TINY_LIMIT      128

#define SMALL           2
#define SMALL_PAGE      (PAGESIZE * 16)
#define SMALL_LIMIT     1024

#define LARGE           3

#define PROT            PROT_READ | PROT_WRITE
#define MAP             MAP_ANON | MAP_PRIVATE

typedef struct          s_zone
{
    size_t              size;
    int                 free;
    struct s_zone       *next;
}                       t_zone;

typedef struct          s_arena
{
    t_zone              *tiny;
    t_zone              *small;
    t_zone              *large;
}                       t_arena;

static t_arena          g_arena;

void        print_heap(t_zone *heap, int heap_type)
{
    int i = 1;

    printf("\n> Printing blocks in heap.\n");
    while (heap->next)
    {
        printf("Block #%d: str=%s :: free=%d :: size=%d\n", 
                i++, heap + sizeof(t_zone), heap->free, heap->size);
        heap = heap->next;
    }
    printf("\n");
}

void        ft_free(void *ptr)
{
    t_zone *block;

    printf("> Block freed.\n");
    block = (t_zone *)ptr - sizeof(t_zone);
    block->free = TRUE;
}

void        *find_block(size_t size, t_zone *zone)
{
    int     i = 1;

    while (zone->next)
    {
        if (zone->free == TRUE && (zone->size == 0 || zone->size >= 0))
        {
            printf("I'm here\n");
            zone->free = FALSE;
            zone->size = size;
            return (zone + sizeof(t_zone));
        }
        zone = zone->next;
    }

    zone->free = FALSE;
    zone->size = size;
    zone->next = (t_zone *)((char *)(zone + 1) + size);
    zone->next->free = TRUE;
    zone->next->next = NULL;

    printf("> Block of %d bytes handed.\n", (int)size);
    return (zone + sizeof(t_zone));
}

void        *init_heap(size_t size, t_zone **zone, size_t page_size)
{
    *zone = (t_zone*)mmap(NULL, page_size, PROT, MAP, -1, 0);
    (*zone)->size = size;
    (*zone)->free = FALSE;
    (*zone)->next = (t_zone *)((char *)(zone + 1) + size + sizeof(t_zone));
    (*zone)->next->free = TRUE;
    (*zone)->next->next = NULL;
    
    printf("> Block of %d bytes handed.\n", (int)size);
    return (*zone + sizeof(t_zone));
}

void        *allocate(size_t size, int heap)
{
    if (heap == TINY)
    {
        if (g_arena.tiny == FALSE) // If no arena currently exist, initialize one.
            return (init_heap(size, &g_arena.tiny, TINY_PAGE));
        else
            return (find_block(size, g_arena.tiny));
    }
    else if (heap == SMALL)
    {
        if (g_arena.small == FALSE)
            return (init_heap(size, &g_arena.small, TINY_PAGE));
        else
            return (find_block(size, g_arena.small));
    }
    else if (heap == LARGE)
    {} //TODO
    return (NULL);
}

void        *ft_malloc(size_t size)
{
    if (size <= 0)
        return (NULL);
    else if (ALIGN(size) <= TINY_LIMIT)
        return (allocate(ALIGN(size), TINY));
    else if (ALIGN(size <= SMALL_LIMIT))
        return (allocate(ALIGN(size), SMALL));
    else
        return (NULL); // TODO LARGE
    return (NULL);
}

int main(void)
{
    char *ptr1 = ft_malloc(123);
    strcpy(ptr1, "[tiny malloc 1 12345678901234567890]");
    printf("Content of ptr1: %s\n", ptr1);

    char *ptr2 = ft_malloc(123);
    strcpy(ptr2, "[tiny malloc 2 12345678901234567890]");
    printf("Content of ptr2: %s\n", ptr2);

    char *ptr3 = ft_malloc(123);
    strcpy(ptr3, "[tiny malloc 3 12345678901234567890]");
    printf("Content of ptr3: %s\n", ptr3);

    print_heap((t_zone *)ptr1 - sizeof(t_zone), TINY);

    // ft_free(ptr2);
    // char *ptrptr = ft_malloc(123);
    // printf("Content of ptr2: %s\n", ptr2);
    // strcpy(ptrptr, "[renewed pointer 1234567890]");
    // printf("%s\n", ptr2);

    char *ptr4 = ft_malloc(123);
    strcpy(ptr4, "[tiny malloc 4 12345678901234567890]");
    printf("Content of ptr4: %s\n", ptr4);

    char *ptr5 = ft_malloc(123);
    strcpy(ptr5, "[tiny malloc 5 12345678901234567890]");
    printf("Content of ptr5: %s\n", ptr5);

    char *ptr6 = ft_malloc(123);
    strcpy(ptr6, "[tiny malloc 6 12345678901234567890]");
    printf("Content of ptr6: %s\n", ptr6);

    print_heap((t_zone *)ptr1 - sizeof(t_zone), TINY);
    // printf("Let's break the heap\n");
    // for (int i = 0 ; i<1000 ; i++)
    //     ptrptr = ft_malloc(240);

    return (0);
}
