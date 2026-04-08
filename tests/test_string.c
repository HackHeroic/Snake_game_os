#include <stdio.h>
#include <stdlib.h>
#include "lib/string.h"

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

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

int main(void) {
    printf("String Library Tests\n");
    printf("====================\n");

    /* strlen */
    ASSERT("strlen empty", my_strlen("") == 0);
    ASSERT("strlen hello", my_strlen("hello") == 5);
    ASSERT("strlen single", my_strlen("x") == 1);

    /* strcpy */
    {
        char buf[32];
        my_strcpy(buf, "hello");
        ASSERT("strcpy hello", str_eq(buf, "hello"));
        my_strcpy(buf, "");
        ASSERT("strcpy empty", str_eq(buf, ""));
    }

    /* strcmp */
    ASSERT("strcmp equal", my_strcmp("abc", "abc") == 0);
    ASSERT("strcmp less", my_strcmp("abc", "abd") < 0);
    ASSERT("strcmp greater", my_strcmp("abd", "abc") > 0);
    ASSERT("strcmp prefix", my_strcmp("ab", "abc") < 0);

    /* strcat */
    {
        char buf[32];
        my_strcpy(buf, "hel");
        my_strcat(buf, "lo");
        ASSERT("strcat hello", str_eq(buf, "hello"));
    }

    /* int_to_str */
    {
        char buf[32];
        int_to_str(0, buf);
        ASSERT("int_to_str 0", str_eq(buf, "0"));
        int_to_str(42, buf);
        ASSERT("int_to_str 42", str_eq(buf, "42"));
        int_to_str(-7, buf);
        ASSERT("int_to_str -7", str_eq(buf, "-7"));
        int_to_str(12345, buf);
        ASSERT("int_to_str 12345", str_eq(buf, "12345"));
    }

    /* str_to_int */
    ASSERT("str_to_int 0", str_to_int("0") == 0);
    ASSERT("str_to_int 42", str_to_int("42") == 42);
    ASSERT("str_to_int -7", str_to_int("-7") == -7);
    ASSERT("str_to_int 12345", str_to_int("12345") == 12345);

    TEST_SUMMARY();
}
