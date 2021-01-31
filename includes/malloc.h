#ifndef MALLOC_H
#define MALLOC_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <bits/mman-linux.h>
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

void hexdump(const void* data, size_t size);
void print_heap(t_heap *heap);
void *ft_malloc(size_t size);
void ft_free(void *ptr);

#endif