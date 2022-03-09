#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/*
 * prevent compilor for optimize the none return value
 * from fib_write.
 */
__attribute__((always_inline)) static inline void escape(void *p)
{
    __asm__ volatile("" : : "g"(p) : "memory");
}

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 100

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

#define FIB_TIME_PROXY(fib_f, k, result)     \
    ({                                       \
        ktime_t kt = ktime_get();            \
        result = fib_f(k);                   \
        (size_t) ktime_sub(ktime_get(), kt); \
    })

static uint64_t fib_sequence(long long k)
{
    uint64_t state[] = {0, 1};

    for (int i = 2; i <= k; i++) {
        state[(i & 1)] += state[((i - 1) & 1)];
    }

    return state[(k & 1)];
}

static uint64_t fib_fast_doubling(long long k)
{
    if (k < 2)
        return k;
    uint64_t a = 0;  // F(0) = 0
    uint64_t b = 1;  // F(1) = 1

    for (unsigned int mask = 1 << ((sizeof(k) << 3) - __builtin_clz(k) - 1);
         mask; mask >>= 1) {
        uint64_t c = a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k+1) – F(k) ]
        uint64_t d = a * a + b * b;    // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & k) {  // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            a = d;       //   F(n_j) = F(2k + 1)
            b = c + d;   //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {         // n_j is even: k = n_j/2 => n_j = 2k
            a = c;       //   F(n_j) = F(2k)
            b = d;       //   F(n_j + 1) = F(2k + 1)
        }
    }
    return a;
}

static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t mode,
                         loff_t *offset)
{
    ktime_t kt;
    uint64_t result = 0;
    switch (mode) {
    case 0:
        kt = FIB_TIME_PROXY(fib_sequence, *offset, result);
        break;
    case 1:
        kt = FIB_TIME_PROXY(fib_fast_doubling, *offset, result);
        break;
    default:
        return 1;
    }
    escape(&result);
    return (ssize_t) ktime_to_ns(kt);
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t mode,
                        loff_t *offset)
{
    ssize_t fib_result;
    switch (mode) {
    case 0:
        fib_result = fib_sequence(*offset);
        break;
    case 1:
        fib_result = fib_fast_doubling(*offset);
        break;
    }
    return fib_result;
}



static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
