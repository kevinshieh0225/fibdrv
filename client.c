#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define FIB_DEV "/dev/fibonacci"
#define offset 10000 /* TODO: try test something bigger than the limit */
#define mode 0


int main()
{
    char buf[100000];
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        int sz = read(fd, buf, mode);
        if (sz)
            return 0;
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
    }

    close(fd);
    return 0;
}
