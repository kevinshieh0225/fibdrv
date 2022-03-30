#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bn.h"
#define FIB_DEV "/dev/fibonacci"
#define offset 2000
#define mode 0


int main()
{
    void (*fib_algorithm[3])(bn *, unsigned int) = {bn_fib_v1, bn_fdoubling_v0,
                                                    bn_fdoubling_v1};

    for (int i = 0; i <= offset; i++) {
        bn *fib = bn_alloc(1);
        fib_algorithm[mode](fib, i);
        char *buf = bn_to_string(fib);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
        bn_free(fib);
        free(buf);
    }

    return 0;
}
