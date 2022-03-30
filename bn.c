#include "bn.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#ifndef SWAP
#define SWAP(x, y)           \
    do {                     \
        typeof(x) __tmp = x; \
        x = y;               \
        y = __tmp;           \
    } while (0)
#endif

#ifndef DIV_ROUNDUP
#define DIV_ROUNDUP(x, len) (((x) + (len) -1) / (len))
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

/* count leading zeros of src*/
static int bn_clz(const bn *src)
{
    int cnt = 0;
    for (int i = src->size - 1; i >= 0; i--) {
        if (src->number[i]) {
            // prevent undefined behavior when src = 0
            cnt += __builtin_clz(src->number[i]);
            return cnt;
        } else {
            cnt += BN_WSIZE;
        }
    }
    return cnt;
}

/* count the digits of most significant bit */
static int bn_msb(const bn *src)
{
    return src->size * BN_WSIZE - bn_clz(src);
}

/*
 * output bn to decimal string
 * Note: the returned string should be freed with the free()
 */
char *bn_to_string(const bn *src)
{
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    size_t len = (BN_WSIZE * src->size) / 3 + 2 + src->sign;
    char *s = (char *) malloc(len);
    char *p = s;

    memset(s, '0', len - 1);
    s[len - 1] = '\0';

    /* src.number[0] contains least significant bits */
    for (int i = src->size - 1; i >= 0; i--) {
        /* walk through every bit of bn */
        for (bn_data d = (bn_data) 1 << (BN_WSIZE - 1); d; d >>= 1) {
            /* binary -> decimal string */
            int carry = !!(d & src->number[i]);
            for (int j = len - 2; j >= 0; j--) {
                s[j] += s[j] - '0' + carry;
                carry = (s[j] > '9');
                if (carry)
                    s[j] -= 10;
            }
        }
    }
    // skip leading zero
    while (p[0] == '0' && p[1] != '\0') {
        p++;
    }
    if (src->sign)
        *(--p) = '-';
    memmove(s, p, strlen(p) + 1);
    return s;
}

/*
 * alloc a bn structure with the given size
 * the value is initialized to +0
 */
bn *bn_alloc(size_t size)
{
    bn *new = (bn *) malloc(sizeof(bn));
    if (!new)
        return NULL;
    new->number = (bn_data *) malloc(sizeof(bn_data) * size);
    if (!new->number) {
        free(new);
        return NULL;
    }
    for (int i = 0; i < size; i++)
        new->number[i] = 0;
    new->size = size;
    new->sign = 0;
    return new;
}

/*
 * free entire bn data structure
 * return 0 on success, -1 on error
 */
int bn_free(bn *src)
{
    if (src == NULL)
        return -1;
    free(src->number);
    free(src);
    return 0;
}

/*
 * resize bn
 * return 0 on success, -1 on error
 * data lose IS neglected when shinking the size
 */
static int bn_resize(bn *src, size_t size)
{
    if (!src)
        return -1;
    if (size == src->size)
        return 0;
    if (size == 0)  // prevent realloc(0) = free, which will cause problem
        return 1;
    src->number = realloc(src->number, sizeof(bn_data) * size);
    if (!src->number) {  // realloc fails
        return -1;
    }
    if (size > src->size) {
        for (int i = src->size; i < size; i++)
            src->number[i] = 0;
    }
    src->size = size;
    return 0;
}

/*
 * copy the value from src to dest
 * return 0 on success, -1 on error
 */
int bn_cpy(bn *dest, bn *src)
{
    if (bn_resize(dest, src->size) < 0)
        return -1;
    dest->sign = src->sign;
    memcpy(dest->number, src->number, src->size * sizeof(bn_data));
    return 0;
}

/* swap bn ptr */
void bn_swap(bn *a, bn *b)
{
    bn tmp = *a;
    *a = *b;
    *b = tmp;
}

/* left bit shift on bn (maximun shift 31) */
void bn_lshift(bn *src, size_t shift, bn *dest)
{
    size_t z = bn_clz(src);
    shift %= BN_WSIZE;  // only handle shift within BN_WSIZE bits atm
    if (!shift)
        return;

    if (shift > z) {
        bn_resize(dest, src->size + 1);
        dest->number[src->size] =
            src->number[src->size - 1] >> (BN_WSIZE - shift);
    } else {
        bn_resize(dest, src->size);
    }

    for (int i = src->size - 1; i > 0; i--)
        dest->number[i] =
            src->number[i] << shift | src->number[i - 1] >> (BN_WSIZE - shift);
    dest->number[0] = src->number[0] << shift;
}

/*
 * compare length
 * return 1 if |a| > |b|
 * return -1 if |a| < |b|
 * return 0 if |a| = |b|
 */
int bn_cmp(const bn *a, const bn *b)
{
    if (a->size > b->size) {
        return 1;
    } else if (a->size < b->size) {
        return -1;
    } else {
        for (int i = a->size - 1; i >= 0; i--) {
            if (a->number[i] > b->number[i])
                return 1;
            if (a->number[i] < b->number[i])
                return -1;
        }
        return 0;
    }
}

/* |c| = |a| + |b| */
static void bn_do_add(const bn *a, const bn *b, bn *c)
{
    // max digits = max(sizeof(a) + sizeof(b)) + 1
    // int d = MAX(bn_msb(a), bn_msb(b)) + 1;
    // d = DIV_ROUNDUP(d, BN_WSIZE) + !d;
    // bn_resize(c, d);  // round up, min size = 1

    // bn_data_tmp_u carry = 0;
    // for (int i = 0; i < c->size; i++) {
    //     bn_data tmp1 = (i < a->size) ? a->number[i] : 0;
    //     bn_data tmp2 = (i < b->size) ? b->number[i] : 0;
    //     carry += (bn_data_tmp_u) tmp1 + tmp2;
    //     c->number[i] = carry;
    //     carry >>= BN_WSIZE;
    // }

    // if (!c->number[c->size - 1] && c->size > 1)
    //     bn_resize(c, c->size - 1);

    if (a->size < b->size)
        SWAP(a, b);

    bn_resize(c, a->size);
    bn_data carry = 0;
    for (int i = 0; i < b->size; i++) {
        bn_data tmp1 = a->number[i];
        bn_data tmp2 = b->number[i];
        carry = (tmp1 += carry) < carry;
        carry += (c->number[i] = tmp1 + tmp2) < tmp2;
    }
    // remaining part if a->size > b->size
    for (int i = b->size; i < a->size; i++) {
        bn_data tmp1 = a->number[i];
        carry = (tmp1 += carry) < carry;
        c->number[i] = tmp1;
    }
    // remaining carry which need new number space
    if (carry) {
        bn_resize(c, a->size + 1);
        c->number[c->size - 1] = carry;
    }
}

/*
 * |c| = |a| - |b|
 * Note: |a| > |b| must be true
 */
static void bn_do_sub(const bn *a, const bn *b, bn *c)
{
    // max digits = max(sizeof(a) + sizeof(b))
    bn_data d = a->size;
    bn_resize(c, d);

    bn_data_tmp_s carry = 0;
    for (int i = 0; i < c->size; i++) {
        bn_data tmp1 = (i < a->size) ? a->number[i] : 0;
        bn_data tmp2 = (i < b->size) ? b->number[i] : 0;

        carry = (bn_data_tmp_s) tmp1 - tmp2 - carry;
        if (carry < 0) {
            c->number[i] = carry + ((bn_data_tmp_u) 1 << BN_WSIZE);
            carry = 1;
        } else {
            c->number[i] = carry;
            carry = 0;
        }
    }

    d = bn_clz(c) / BN_WSIZE;
    if (d == c->size)
        --d;
    bn_resize(c, c->size - d);
}

/*
 * c = a + b
 * Note: work for c == a or c == b
 */
void bn_add(const bn *a, const bn *b, bn *c)
{
    if (a->sign == b->sign) {  // both positive or negative
        bn_do_add(a, b, c);
        c->sign = a->sign;
    } else {          // different sign
        if (a->sign)  // let a > 0, b < 0
            SWAP(a, b);
        int cmp = bn_cmp(a, b);
        if (cmp > 0) {
            /* |a| > |b| and b < 0, hence c = a - |b| */
            bn_do_sub(a, b, c);
            c->sign = 0;
        } else if (cmp < 0) {
            /* |a| < |b| and b < 0, hence c = -(|b| - |a|) */
            bn_do_sub(b, a, c);
            c->sign = 1;
        } else {
            /* |a| == |b| */
            bn_resize(c, 1);
            c->number[0] = 0;
            c->sign = 0;
        }
    }
}

/*
 * c = a - b
 * Note: work for c == a or c == b
 */
void bn_sub(const bn *a, const bn *b, bn *c)
{
    /* xor the sign bit of b and let bn_add handle it */
    bn tmp = *b;
    tmp.sign ^= 1;  // a - b = a + (-b)
    bn_add(a, &tmp, c);
}

/* c[size] += a[size] * k, and return the carry */
static bn_data _mult_partial(const bn_data *a,
                             bn_data asize,
                             const bn_data k,
                             bn_data *c)
{
    if (k == 0)
        return 0;

    bn_data carry = 0;
    for (int i = 0; i < asize; i++) {
        bn_data high, low;
        __asm__("mulq %3" : "=a"(low), "=d"(high) : "%0"(a[i]), "rm"(k));
        carry = high + ((low += carry) < carry);
        carry += ((c[i] += low) < low);
    }
    return carry;
}
/*
 * c = a x b
 * Note: work for c == a or c == b
 * using the simple quadratic-time algorithm (long multiplication)
 */
void bn_mult(const bn *a, const bn *b, bn *c)
{
    // max digits = sizeof(a) + sizeof(b))
    int d = bn_msb(a) + bn_msb(b);
    d = DIV_ROUNDUP(d, BN_WSIZE) + !d;  // round up, min size = 1
    bn *tmp;
    /* make it work properly when c == a or c == b */
    if (c == a || c == b) {
        tmp = c;  // save c
        c = bn_alloc(d);
    } else {
        tmp = NULL;
        bn_resize(c, d);
    }

    for (int j = 0; j < b->size; j++) {
        c->number[a->size + j] =
            _mult_partial(a->number, a->size, b->number[j], c->number + j);
    }

    c->sign = a->sign ^ b->sign;

    if (tmp) {
        bn_cpy(tmp, c);  // restore c
        bn_free(c);
    }
}

/* calc n-th Fibonacci number and save into dest */
void bn_fib_v0(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *a = bn_alloc(1);
    bn *b = bn_alloc(1);
    dest->number[0] = 1;

    for (unsigned int i = 1; i < n; i++) {
        bn_swap(b, dest);
        bn_add(a, b, dest);
        bn_swap(a, b);
    }  // dest = result
    bn_free(a);
    bn_free(b);
}

/* calc n-th Fibonacci number and save into dest */
void bn_fib_v1(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }
    bn *state[2];
    state[0] = bn_alloc(1);
    state[1] = bn_alloc(1);
    state[0]->number[0] = 0;
    state[1]->number[0] = 1;

    for (unsigned int i = 2; i <= n; ++i)
        bn_add(state[(i & 1)], state[((i - 1) & 1)], state[(i & 1)]);

    bn_cpy(dest, state[(n & 1)]);
    bn_free(state[0]);
    bn_free(state[1]);
}

/*
 * calc n-th Fibonacci number and save into dest
 * using fast doubling algorithm
 */
void bn_fdoubling_v0(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *f1 = dest;        /* F(k) */
    bn *f2 = bn_alloc(1); /* F(k+1) */
    f1->number[0] = 0;
    f2->number[0] = 1;
    bn *k1 = bn_alloc(1);
    bn *k2 = bn_alloc(1);

    /* walk through the digit of n */
    for (unsigned int i = 1U << (31 - __builtin_clz(n)); i; i >>= 1) {
        /* F(2k) = F(k) * [ 2 * F(k+1) – F(k) ] */
        // bn_cpy(k1, f2);     // k1 = F(k+1)
        bn_lshift(f2, 1, k1);  // k1 = 2* F(k+1)
        bn_sub(k1, f1, k1);    // k1 = 2 * F(k+1) – F(k)
        bn_mult(k1, f1, k1);   // k1 = k1 * f1 = F(2k)
        /* state: k1 = F(2k) ; k2 = X; f1 = F(k); f2 = F(k+1) */

        /* F(2k+1) = F(k)^2 + F(k+1)^2 */
        bn_mult(f1, f1, f1);  // f1 = F(k)^2
        bn_mult(f2, f2, f2);  // f2 = F(k+1)^2
        bn_cpy(k2, f1);       // k2 = F(k)^2
        bn_add(k2, f2, k2);   // k2 = F(k)^2 + F(k+1)^2  = F(2k+1)
        /* state: k1 = F(2k) ; k2 = F(2k+1); f1 = X; f2 = X */
        if (n & i) {
            bn_cpy(f1, k2);
            bn_cpy(f2, k1);
            bn_add(f2, k2, f2);
            /* state: f1 = F(2k+1); f2 = F(2k+2) */
        } else {
            bn_cpy(f1, k1);
            bn_cpy(f2, k2);
            /* state: f1 = F(2k); f2 = F(2k+1) */
        }
    }
    // return f1
    bn_free(f2);
    bn_free(k1);
    bn_free(k2);
}

void bn_fdoubling_v1(bn *dest, unsigned int n)
{
    bn_resize(dest, 1);
    if (n < 2) {  // Fib(0) = 0, Fib(1) = 1
        dest->number[0] = n;
        return;
    }

    bn *f1 = dest;        /* F(k) */
    bn *f2 = bn_alloc(1); /* F(k+1) */
    f1->number[0] = 0;
    f2->number[0] = 1;
    bn *k = bn_alloc(1);

    /* walk through the digit of n */
    for (unsigned int i = 1U << (31 - __builtin_clz(n)); i; i >>= 1) {
        /* F(2k) = F(k) * [ 2 * F(k+1) – F(k) ] */
        bn_lshift(f2, 1, k);  // k = 2* F(k+1)
        bn_sub(k, f1, k);     // k = 2 * F(k+1) – F(k)
        bn_mult(k, f1, k);    // k = k1 * f1 = F(2k)
        /* state: k = F(2k); f1 = F(k); f2 = F(k+1) */

        /* F(2k+1) = F(k)^2 + F(k+1)^2 */
        bn_mult(f1, f1, f1);  // f1 = F(k)^2
        bn_mult(f2, f2, f2);  // f2 = F(k+1)^2
        bn_add(f1, f2, f2);   // f2 = f1^2 + f2^2 = F(2k+1) now
        bn_swap(f1, k);       // f1 <-> k, f1 = F(2k) now
        /* state: k = X; f1 = F(2k); f2 = F(2k+1) */

        if (n & i) {
            bn_swap(f1, f2);     // f1 = F(2k+1)
            bn_add(f1, f2, f2);  // f2 = F(2k+2)
        }
    }
    // return f1
    bn_free(f2);
    bn_free(k);
}