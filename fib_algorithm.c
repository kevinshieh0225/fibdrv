#include "fib_algorithm.h"

uint64_t fib_sequence(long long k)
{
    uint64_t state[] = {0, 1};

    for (int i = 2; i <= k; i++) {
        state[(i & 1)] += state[((i - 1) & 1)];
    }

    return state[(k & 1)];
}

uint64_t fib_fast_doubling(long long k)
{
    if (k < 2)
        return k;
    uint64_t f1 = 0;  // F(0) = 0
    uint64_t f2 = 1;  // F(1) = 1

    for (unsigned int mask = 1U << (31 - __builtin_clz(k)); mask; mask >>= 1) {
        uint64_t k1 = f1 * ((f2 << 1) - f1);  // F(2k) = F(k)*[2*F(k+1) – F(k)]
        uint64_t k2 = f1 * f1 + f2 * f2;      // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & k) {    // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            f1 = k2;       //   F(n_j) = F(2k + 1)
            f2 = k1 + k2;  //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {           // n_j is even: k = n_j/2 => n_j = 2k
            f1 = k1;       //   F(n_j) = F(2k)
            f2 = k2;       //   F(n_j + 1) = F(2k + 1)
        }
    }
    return f1;
}