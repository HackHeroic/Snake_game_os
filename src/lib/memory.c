#include "memory.h"
#include <stdlib.h>

static char *virtual_ram = NULL;

typedef struct {
    int size;       /* size of user data (not including header) */
    char in_use;    /* 1 = allocated, 0 = free */
    char padding[3];
} BlockHeader;

void memory_init(void) {
    if (virtual_ram != NULL) {
        free(virtual_ram);
    }
    virtual_ram = (char *)malloc(VIRTUAL_RAM_SIZE);
    if (!virtual_ram) return;

    BlockHeader *first = (BlockHeader *)virtual_ram;
    first->size = VIRTUAL_RAM_SIZE - HEADER_SIZE;
    first->in_use = 0;
}

void *alloc(int size) {
    char *ptr;
    BlockHeader *block;

    if (size < MIN_ALLOC) size = MIN_ALLOC;
    if (virtual_ram == NULL) return NULL;

    ptr = virtual_ram;
    while (ptr < virtual_ram + VIRTUAL_RAM_SIZE) {
        block = (BlockHeader *)ptr;
        if (!block->in_use && block->size >= size) {
            if (block->size >= size + MIN_SPLIT) {
                BlockHeader *next = (BlockHeader *)(ptr + HEADER_SIZE + size);
                next->size = block->size - size - HEADER_SIZE;
                next->in_use = 0;
                block->size = size;
            }
            block->in_use = 1;
            return ptr + HEADER_SIZE;
        }
        ptr += HEADER_SIZE + block->size;
    }
    return NULL;
}

void dealloc(void *user_ptr) {
    char *ptr;
    BlockHeader *block;
    BlockHeader *next;

    if (user_ptr == NULL || virtual_ram == NULL) return;

    block = (BlockHeader *)((char *)user_ptr - HEADER_SIZE);
    block->in_use = 0;

    /* forward coalescing */
    ptr = (char *)block + HEADER_SIZE + block->size;
    while (ptr < virtual_ram + VIRTUAL_RAM_SIZE) {
        next = (BlockHeader *)ptr;
        if (next->in_use) break;
        block->size += HEADER_SIZE + next->size;
        ptr += HEADER_SIZE + next->size;
    }
}

int memory_used(void) {
    char *ptr;
    int used = 0;
    if (virtual_ram == NULL) return 0;
    ptr = virtual_ram;
    while (ptr < virtual_ram + VIRTUAL_RAM_SIZE) {
        BlockHeader *block = (BlockHeader *)ptr;
        if (block->in_use) used += block->size;
        ptr += HEADER_SIZE + block->size;
    }
    return used;
}
