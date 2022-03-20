#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#if defined(__LP64__) || defined(__x86_64__) || defined(__amd64__) || \
    defined(__aarch64__)
#define BN_WSIZE 64
#else
#define BN_WSIZE 32
#endif

#if BN_WSIZE == 64
typedef uint64_t bn_data;
typedef unsigned __int128 bn_data_tmp_u;  // gcc support __int128
typedef __int128 bn_data_tmp_s;           // gcc support __int128
#elif BN_WSIZE == 32
typedef uint32_t bn_data;
typedef uint64_t bn_data_tmp;
#else
#error "BN_WSIZE must be 32 or 64"
#endif

/*
 * bignum data structure
 * number[0] contains least significant bits
 * number[size - 1] contains most significant bits
 * sign = 1 for negative number
 */
typedef struct _bn {
    bn_data *number;
    int size;
    int sign;
} bn;

/*
 * output bn to decimal string
 * Note: the returned string should be freed with the kfree()
 */
char *bn_to_string(const bn *src);

/*
 * alloc a bn structure with the given size
 * the value is initialized to +0
 */
bn *bn_alloc(size_t size);

/*
 * free entire bn data structure
 * return 0 on success, -1 on error
 */
int bn_free(bn *src);

/*
 * copy the value from src to dest
 * return 0 on success, -1 on error
 */
int bn_cpy(bn *dest, bn *src);

/*
 * compare length
 * return 1 if |a| > |b|
 * return -1 if |a| < |b|
 * return 0 if |a| = |b|
 */
int bn_cmp(const bn *a, const bn *b);

/* swap bn ptr */
void bn_swap(bn *a, bn *b);

/* dest = src << shift (maximun shift 31) */
void bn_lshift(bn *src, size_t shift, bn *dest);

/* right bit shift on bn (maximun shift 31) */
// void bn_rshift(bn *src, size_t shift);

/* c = a + b */
void bn_add(const bn *a, const bn *b, bn *c);

/* c = a - b */
void bn_sub(const bn *a, const bn *b, bn *c);

/* c = a x b */
void bn_mult(const bn *a, const bn *b, bn *c);

/* calc n-th Fibonacci number and save into dest */
void bn_fib_v0(bn *dest, unsigned int n);
void bn_fib_v1(bn *dest, unsigned int n);

void bn_fdoubling_v0(bn *dest, unsigned int n);
void bn_fdoubling_v1(bn *dest, unsigned int n);