#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define FIB_DEV "/dev/fibonacci"
#define offset 100 /* TODO: try test something bigger than the limit */
#define mode 1
int main()
{
    uint64_t sz;

    char buf[1];
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = write(fd, buf, mode);
        printf("Writing from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lu.\n",
               i, sz);
    }
    printf("\n");
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, mode);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lu.\n",
               i, sz);
    }
    printf("\n");
    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, mode);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lu.\n",
               i, sz);
    }

    close(fd);
    return 0;
}
