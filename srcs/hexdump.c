#include <stdio.h>

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
            printf("\033[0;31m%05lx\033[0;90m | \033[0m", i);
        if (((unsigned char*)data)[i] != '\0')
            printf("\033[0;32m%02x \033[0m", ((unsigned char*)data)[i]);
        else
            printf("\033[0;90m%02x \033[0m", ((unsigned char*)data)[i]);
       
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
            ascii[i % 16] = ((unsigned char*)data)[i];
        else
            ascii[i % 16] = '.';
        if ((i+1) % 8 == 0 || i+1 == size) {
            // printf("");
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