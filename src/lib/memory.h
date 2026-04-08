#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#define VIRTUAL_RAM_SIZE 65536  /* 64KB */
#define HEADER_SIZE 8           /* 4 bytes size + 1 byte in_use + 3 padding */
#define MIN_ALLOC 8             /* minimum user data size */
#define MIN_SPLIT (HEADER_SIZE + MIN_ALLOC) /* 16 bytes: minimum to split a block */

void memory_init(void);
void *alloc(int size);
void dealloc(void *ptr);
int memory_used(void);

#endif
