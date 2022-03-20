#include "bn.h"
#define ITH 1000
#define ITER_TIMES 10000

__attribute__((always_inline)) static inline void escape(void *p)
{
    __asm__ volatile("" : : "g"(p) : "memory");
}


int main(int argc, char const *argv[])
{
    bn *test = bn_alloc(1);
    for (int i = 0; i < ITER_TIMES; i++) {
        bn_fdoubling_v0(test, ITH);
        escape(test->number);
    }
    bn_free(test);
    return 0;
}