#include "string.h"

int my_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void my_strcpy(char *dest, const char *src) {
    while (*src) { *dest = *src; dest++; src++; }
    *dest = '\0';
}

int my_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

void my_strcat(char *dest, const char *src) {
    while (*dest) dest++;
    my_strcpy(dest, src);
}

void int_to_str(int n, char *buf) {
    int i = 0, neg = 0, start, end;
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    if (n < 0) { neg = 1; n = -n; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n = n / 10; }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';
    start = 0; end = i - 1;
    while (start < end) {
        char tmp = buf[start]; buf[start] = buf[end]; buf[end] = tmp;
        start++; end--;
    }
}

int str_to_int(const char *s) {
    int result = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { result = result * 10 + (*s - '0'); s++; }
    return neg ? -result : result;
}
