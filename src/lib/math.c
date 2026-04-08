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
    if (b == 0) return 0;
    return a - my_multiply(my_divide(a, b), b);
}

int my_abs(int a) {
    return a < 0 ? -a : a;
}

int my_min(int a, int b) {
    return a < b ? a : b;
}

int my_max(int a, int b) {
    return a > b ? a : b;
}

int my_clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

int pseudo_random(int *seed) {
    unsigned int s = (unsigned int)(*seed);
    /* MINSTD: a=16807, m=2^31-1 */
    /* Use Schrage's method to avoid overflow: */
    /* m/a = 127773, m%a = 2836 */
    int hi = (int)(s / 127773u);
    int lo = (int)(s % 127773u);
    int t = my_multiply(16807, lo) - my_multiply(2836, hi);
    if (t <= 0) t += 2147483647;
    *seed = t;
    return t;
}
