#include <stdio.h>
#include <stdlib.h>
#include "lib/math.h"

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
    printf("Math Library Tests\n");
    printf("==================\n");

    ASSERT("my_multiply stub returns 0", my_multiply(2, 3) == 0);

    TEST_SUMMARY();
}
