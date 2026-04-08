#include "math.h"

int my_multiply(int a, int b) {
    int negative = 0;
    unsigned int ua, ub;
    unsigned int result = 0;

    if (a < 0) { negative = !negative; ua = (unsigned int)(-a); }
    else { ua = (unsigned int)a; }

    if (b < 0) { negative = !negative; ub = (unsigned int)(-b); }
    else { ub = (unsigned int)b; }

    while (ub > 0) {
        if (ub & 1) {
            result += ua;
        }
        ua <<= 1;
        ub >>= 1;
    }

    return negative ? -(int)result : (int)result;
}

int my_divide(int a, int b) {
    int negative = 0;
    unsigned int ua, ub;
    unsigned int quotient = 0;
    unsigned int remainder = 0;
    int i;

    if (b == 0) return 0;

    if (a < 0) { negative = !negative; ua = (unsigned int)(-a); }
    else { ua = (unsigned int)a; }

    if (b < 0) { negative = !negative; ub = (unsigned int)(-b); }
    else { ub = (unsigned int)b; }

    for (i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (ua >> i) & 1;
        if (remainder >= ub) {
            remainder -= ub;
            quotient |= (1u << i);
        }
    }

    return negative ? -(int)quotient : (int)quotient;
}

int my_mod(int a, int b) {
    (void)a; (void)b;
    return 0;
}

int my_abs(int a) {
    (void)a;
    return 0;
}

int my_min(int a, int b) {
    (void)a; (void)b;
    return 0;
}

int my_max(int a, int b) {
    (void)a; (void)b;
    return 0;
}

int my_clamp(int val, int min, int max) {
    (void)val; (void)min; (void)max;
    return 0;
}

int pseudo_random(int *seed) {
    (void)seed;
    return 0;
}
