// origin code: KYG-yaya573142/fibdrv/client_statistic.c

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define sample_size 1000
#define offset 10000
#define mode 1

int main()
{
    FILE *fp = fopen("./plot_bn_fd_v3_bn_mult", "w");
    char write_buf[] = "testing writing";

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    /* for each F(i), measure sample_size times of data and
     * remove outliers based on the 95% confidence level
     */
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        double time[mode][sample_size] = {0};
        double mean[mode] = {0.0}, sd[mode] = {0.0}, result[mode] = {0.0};
        int count[mode] = {0};

        for (int m = 0; m < mode; ++m) {
            for (int n = 0; n < sample_size; n++) { /* sampling */
                /* get the runtime in kernel space here */
                time[m][n] = write(fd, write_buf, m);
                mean[m] += time[m][n]; /* sum */
            }
            mean[m] /= sample_size; /* mean */

            for (int n = 0; n < sample_size; n++) {
                sd[m] += (time[m][n] - mean[m]) * (time[m][n] - mean[m]);
            }
            sd[m] = sqrt(sd[m] / (sample_size - 1)); /* standard deviation */

            for (int n = 0; n < sample_size; n++) { /* remove outliers */
                if (time[m][n] <= (mean[m] + 2 * sd[m]) &&
                    time[m][n] >= (mean[m] - 2 * sd[m])) {
                    result[m] += time[m][n];
                    count[m]++;
                }
            }
            result[m] /= count[m];
        }

        // print result to file
        fprintf(fp, "%d ", i);
        for (int m = 0; m < mode; ++m) {
            fprintf(fp, "%.5lf ", result[m]);
        }
        fprintf(fp, "samples: ");
        for (int m = 0; m < mode; ++m) {
            fprintf(fp, "%d ", count[m]);
        }
        fprintf(fp, "\n");
    }
    close(fd);
    fclose(fp);
    return 0;
}
