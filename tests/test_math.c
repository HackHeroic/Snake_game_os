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

    /* multiply tests */
    ASSERT("multiply 2*3=6", my_multiply(2, 3) == 6);
    ASSERT("multiply 0*5=0", my_multiply(0, 5) == 0);
    ASSERT("multiply 7*1=7", my_multiply(7, 1) == 7);
    ASSERT("multiply -3*4=-12", my_multiply(-3, 4) == -12);
    ASSERT("multiply -2*-5=10", my_multiply(-2, -5) == 10);
    ASSERT("multiply 100*200=20000", my_multiply(100, 200) == 20000);
    ASSERT("multiply 16807*12345 (PRNG-sized)", my_multiply(16807, 12345) == 207482415);

    /* divide tests */
    ASSERT("divide 6/3=2", my_divide(6, 3) == 2);
    ASSERT("divide 7/2=3", my_divide(7, 2) == 3);
    ASSERT("divide 0/5=0", my_divide(0, 5) == 0);
    ASSERT("divide 100/10=10", my_divide(100, 10) == 10);
    ASSERT("divide -10/3=-3", my_divide(-10, 3) == -3);
    ASSERT("divide 10/-3=-3", my_divide(10, -3) == -3);
    ASSERT("divide 2147483647/16807=127773 (PRNG-sized)", my_divide(2147483647, 16807) == 127773);
    ASSERT("divide by zero returns 0", my_divide(5, 0) == 0);

    TEST_SUMMARY();
}
