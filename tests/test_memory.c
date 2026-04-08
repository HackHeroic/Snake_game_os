#include <stdio.h>
#include <stdlib.h>
#include "lib/memory.h"

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(msg, expr) do { \
    tests_run++; \
    if (expr) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { printf("  FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

#define TEST_SUMMARY() do { \
    printf("\n%d/%d tests passed\n", tests_passed, tests_run); \
    return tests_passed == tests_run ? 0 : 1; \
} while(0)

int main(void) {
    printf("Memory Library Tests\n");
    printf("====================\n");

    memory_init();

    /* basic alloc */
    {
        void *p = alloc(16);
        ASSERT("alloc returns non-null", p != NULL);
        ASSERT("memory_used > 0 after alloc", memory_used() > 0);
        dealloc(p);
        ASSERT("memory_used == 0 after dealloc", memory_used() == 0);
    }

    /* multiple allocs */
    {
        void *a = alloc(32);
        void *b = alloc(32);
        ASSERT("two allocs return different pointers", a != b);
        ASSERT("memory_used reflects both allocs", memory_used() >= 64);
        dealloc(a);
        dealloc(b);
        ASSERT("memory_used == 0 after freeing both", memory_used() == 0);
    }

    /* alloc respects minimum size */
    {
        void *p = alloc(1);
        ASSERT("alloc(1) returns non-null", p != NULL);
        ASSERT("alloc(1) uses at least MIN_ALLOC bytes", memory_used() >= MIN_ALLOC);
        dealloc(p);
    }

    /* coalescing */
    {
        void *a = alloc(100);
        void *b = alloc(100);
        void *c = alloc(100);
        dealloc(b);
        dealloc(c);
        /* b and c should coalesce into one free block */
        void *d = alloc(200);
        ASSERT("coalesced block fits larger alloc", d != NULL);
        dealloc(a);
        dealloc(d);
        ASSERT("all freed, memory_used == 0", memory_used() == 0);
    }

    /* stress test */
    {
        void *ptrs[100];
        int i;
        int all_ok = 1;
        for (i = 0; i < 100; i++) {
            ptrs[i] = alloc(16);
            if (ptrs[i] == NULL) { all_ok = 0; break; }
        }
        ASSERT("100 small allocs succeed", all_ok);
        for (i = 0; i < 100; i++) {
            dealloc(ptrs[i]);
        }
        ASSERT("all 100 freed, memory_used == 0", memory_used() == 0);
    }

    /* alloc too large */
    {
        void *p = alloc(VIRTUAL_RAM_SIZE);
        ASSERT("alloc(too large) returns NULL", p == NULL);
    }

    TEST_SUMMARY();
}
