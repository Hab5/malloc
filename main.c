#include <string.h>
#include "includes/malloc.h"

int main(void) {

    char *ptr = ft_malloc(24);
    strcpy(ptr, "And so heap begins!");

    char *ptr1 = ft_malloc(64);
    strcpy(ptr1, 
        "in between are the block's metadata represented in memory");

    char *ptr2 = ft_malloc(24);
    strcpy(ptr2, "to-be-freed block");

    char *ptr3 = ft_malloc(16);
    strcpy(ptr3, "goodbye world!");

    ft_free(ptr2);

    ptr2 = ft_malloc(24);
    strcpy(ptr2, "new data in freed block");

    print_heap(g_arena.tiny);
    
    hexdump(
        g_arena.tiny, 
        ((t_heap *)(ptr3 - sizeof(t_heap)))->size_from_origin
    ); 
}