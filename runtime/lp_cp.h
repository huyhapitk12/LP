/*
 * LP Competitive Programming Module
 *
 * High-performance utilities for competitive programming:
 * - Fast I/O (scanf/printf style)
 * - Number Theory (sieve, mod_pow, gcd, etc.)
 * - Basic Data Structures (Stack, Queue, Deque, Heap, DSU)
 *
 * All functions are optimized for speed and minimal overhead.
 */

#ifndef LP_CP_H
#define LP_CP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

/* ================================================================
 * FAST I/O MODULE - Critical for CP
 * Uses buffered I/O for maximum speed
 * ================================================================ */

/* Fast input buffer */
static char _lp_input_buf[1 << 20];  /* 1MB buffer */
static char *_lp_input_ptr = _lp_input_buf;
static char *_lp_input_end = _lp_input_buf;

static inline char _lp_read_char(void) {
    if (_lp_input_ptr == _lp_input_end) {
        _lp_input_end = _lp_input_buf + fread(_lp_input_buf, 1, sizeof(_lp_input_buf), stdin);
        _lp_input_ptr = _lp_input_buf;
    }
    return *(_lp_input_ptr++);
}

static inline void _lp_skip_whitespace(void) {
    char c;
    while ((c = _lp_read_char()) <= ' ' && c != 0);
    _lp_input_ptr--;  /* Put back non-whitespace */
}

/* Fast read integer */
static inline int64_t lp_io_read_int(void) {
    _lp_skip_whitespace();
    int64_t sign = 1;
    int64_t result = 0;
    char c = _lp_read_char();
    
    if (c == '-') {
        sign = -1;
        c = _lp_read_char();
    }
    
    while (c >= '0' && c <= '9') {
        result = result * 10 + (c - '0');
        c = _lp_read_char();
    }
    
    return sign * result;
}

/* Fast read float */
static inline double lp_io_read_float(void) {
    _lp_skip_whitespace();
    char buf[64];
    int i = 0;
    char c = _lp_read_char();
    
    while (i < 63 && ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+')) {
        buf[i++] = c;
        c = _lp_read_char();
    }
    buf[i] = '\0';
    _lp_input_ptr--;  /* Put back */
    
    return strtod(buf, NULL);
}

/* Fast read string (word) */
static inline char* lp_io_read_str(void) {
    _lp_skip_whitespace();
    char buf[1 << 16];
    int i = 0;
    char c = _lp_read_char();
    
    while (i < (int)(sizeof(buf) - 1) && c > ' ') {
        buf[i++] = c;
        c = _lp_read_char();
    }
    buf[i] = '\0';
    _lp_input_ptr--;  /* Put back */
    
    char* result = (char*)malloc(i + 1);
    strcpy(result, buf);
    return result;
}

/* Fast read line */
static inline char* lp_io_read_line(void) {
    char buf[1 << 16];
    int i = 0;
    char c;
    
    while ((c = _lp_read_char()) != '\n' && c != '\0' && i < (int)(sizeof(buf) - 1)) {
        if (c != '\r') buf[i++] = c;
    }
    buf[i] = '\0';
    
    char* result = (char*)malloc(i + 1);
    strcpy(result, buf);
    return result;
}

/* Fast output buffer */
static char _lp_output_buf[1 << 20];  /* 1MB buffer */
static int _lp_output_pos = 0;

static inline void lp_io_flush(void) {
    fwrite(_lp_output_buf, 1, _lp_output_pos, stdout);
    _lp_output_pos = 0;
}

static inline void lp_io_write_char(char c) {
    if (_lp_output_pos >= (int)(sizeof(_lp_output_buf) - 1)) {
        lp_io_flush();
    }
    _lp_output_buf[_lp_output_pos++] = c;
}

static inline void lp_io_write_int(int64_t x) {
    if (x < 0) {
        lp_io_write_char('-');
        x = -x;
    }
    
    char buf[24];
    int i = 0;
    do {
        buf[i++] = '0' + (x % 10);
        x /= 10;
    } while (x > 0);
    
    while (i > 0) {
        lp_io_write_char(buf[--i]);
    }
}

static inline void lp_io_write_str(const char* s) {
    while (*s) {
        lp_io_write_char(*s++);
    }
}

static inline void lp_io_writeln(void) {
    lp_io_write_char('\n');
}

static inline void lp_io_write_int_ln(int64_t x) {
    lp_io_write_int(x);
    lp_io_write_char('\n');
}

static inline void lp_io_write_str_ln(const char* s) {
    lp_io_write_str(s);
    lp_io_write_char('\n');
}

/* ================================================================
 * NUMBER THEORY MODULE
 * ================================================================ */

/* Prime sieve - returns array of booleans */
static inline bool* lp_nt_sieve(int64_t n) {
    bool* is_prime = (bool*)malloc((n + 1) * sizeof(bool));
    memset(is_prime, true, (n + 1) * sizeof(bool));
    
    if (n >= 0) is_prime[0] = false;
    if (n >= 1) is_prime[1] = false;
    
    for (int64_t i = 2; i * i <= n; i++) {
        if (is_prime[i]) {
            for (int64_t j = i * i; j <= n; j += i) {
                is_prime[j] = false;
            }
        }
    }
    
    return is_prime;
}

/* Check if prime */
static inline bool lp_nt_is_prime(int64_t n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    for (int64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

/* Modular exponentiation: (base^exp) % mod */
static inline int64_t lp_nt_mod_pow(int64_t base, int64_t exp, int64_t mod) {
    int64_t result = 1;
    base %= mod;
    if (base < 0) base += mod;
    
    while (exp > 0) {
        if (exp & 1) {
            result = (__int128)result * base % mod;
        }
        base = (__int128)base * base % mod;
        exp >>= 1;
    }
    
    return result;
}

/* Modular inverse using Fermat's little theorem (mod must be prime) */
static inline int64_t lp_nt_mod_inverse(int64_t a, int64_t mod) {
    return lp_nt_mod_pow(a, mod - 2, mod);
}

/* Extended GCD: returns (gcd, x, y) where ax + by = gcd */
typedef struct {
    int64_t gcd;
    int64_t x;
    int64_t y;
} LpExtGcdResult;

static inline LpExtGcdResult lp_nt_extended_gcd(int64_t a, int64_t b) {
    LpExtGcdResult result;
    
    if (b == 0) {
        result.gcd = a;
        result.x = 1;
        result.y = 0;
        return result;
    }
    
    LpExtGcdResult prev = lp_nt_extended_gcd(b, a % b);
    result.gcd = prev.gcd;
    result.x = prev.y;
    result.y = prev.x - (a / b) * prev.y;
    
    return result;
}

/* Prime factorization - returns array of (prime, exponent) pairs */
typedef struct {
    int64_t prime;
    int64_t exp;
} LpPrimeFactor;

typedef struct {
    LpPrimeFactor* factors;
    int64_t count;
    int64_t cap;
} LpPrimeFactors;

static inline LpPrimeFactors lp_nt_prime_factors(int64_t n) {
    LpPrimeFactors result;
    result.cap = 32;
    result.factors = (LpPrimeFactor*)malloc(result.cap * sizeof(LpPrimeFactor));
    result.count = 0;
    
    if (n <= 1) return result;
    
    /* Factor out 2s */
    if (n % 2 == 0) {
        result.factors[result.count].prime = 2;
        result.factors[result.count].exp = 0;
        while (n % 2 == 0) {
            result.factors[result.count].exp++;
            n /= 2;
        }
        result.count++;
    }
    
    /* Factor out odd primes */
    for (int64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            if (result.count >= result.cap) {
                result.cap *= 2;
                result.factors = (LpPrimeFactor*)realloc(result.factors, result.cap * sizeof(LpPrimeFactor));
            }
            result.factors[result.count].prime = i;
            result.factors[result.count].exp = 0;
            while (n % i == 0) {
                result.factors[result.count].exp++;
                n /= i;
            }
            result.count++;
        }
    }
    
    /* If n is still > 1, it's a prime */
    if (n > 1) {
        result.factors[result.count].prime = n;
        result.factors[result.count].exp = 1;
        result.count++;
    }
    
    return result;
}

/* Euler's totient function */
static inline int64_t lp_nt_euler_phi(int64_t n) {
    int64_t result = n;
    
    if (n % 2 == 0) {
        result -= result / 2;
        while (n % 2 == 0) n /= 2;
    }
    
    for (int64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            result -= result / i;
            while (n % i == 0) n /= i;
        }
    }
    
    if (n > 1) {
        result -= result / n;
    }
    
    return result;
}

/* Count divisors */
static inline int64_t lp_nt_count_divisors(int64_t n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    
    int64_t count = 1;
    LpPrimeFactors pf = lp_nt_prime_factors(n);
    
    for (int64_t i = 0; i < pf.count; i++) {
        count *= (pf.factors[i].exp + 1);
    }
    
    free(pf.factors);
    return count;
}

/* Sum of divisors */
static inline int64_t lp_nt_sum_divisors(int64_t n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    
    int64_t sum = 1;
    LpPrimeFactors pf = lp_nt_prime_factors(n);
    
    for (int64_t i = 0; i < pf.count; i++) {
        int64_t p = pf.factors[i].prime;
        int64_t e = pf.factors[i].exp;
        /* Sum = (p^(e+1) - 1) / (p - 1) */
        int64_t partial = (lp_nt_mod_pow(p, e + 1, INT64_MAX) - 1) / (p - 1);
        sum *= partial;
    }
    
    free(pf.factors);
    return sum;
}

/* ================================================================
 * DISJOINT SET UNION (DSU / UNION-FIND)
 * ================================================================ */

typedef struct {
    int64_t* parent;
    int64_t* rank;
    int64_t* size;  /* Size of each component */
    int64_t n;
} LpDSU;

static inline LpDSU lp_dsu_new(int64_t n) {
    LpDSU dsu;
    dsu.n = n;
    dsu.parent = (int64_t*)malloc(n * sizeof(int64_t));
    dsu.rank = (int64_t*)calloc(n, sizeof(int64_t));
    dsu.size = (int64_t*)malloc(n * sizeof(int64_t));
    
    for (int64_t i = 0; i < n; i++) {
        dsu.parent[i] = i;
        dsu.size[i] = 1;
    }
    
    return dsu;
}

static inline int64_t lp_dsu_find(LpDSU* dsu, int64_t x) {
    if (dsu->parent[x] != x) {
        dsu->parent[x] = lp_dsu_find(dsu, dsu->parent[x]);  /* Path compression */
    }
    return dsu->parent[x];
}

static inline bool lp_dsu_union(LpDSU* dsu, int64_t x, int64_t y) {
    int64_t px = lp_dsu_find(dsu, x);
    int64_t py = lp_dsu_find(dsu, y);
    
    if (px == py) return false;  /* Already in same set */
    
    /* Union by rank */
    if (dsu->rank[px] < dsu->rank[py]) {
        int64_t tmp = px;
        px = py;
        py = tmp;
    }
    
    dsu->parent[py] = px;
    dsu->size[px] += dsu->size[py];
    
    if (dsu->rank[px] == dsu->rank[py]) {
        dsu->rank[px]++;
    }
    
    return true;
}

static inline bool lp_dsu_same(LpDSU* dsu, int64_t x, int64_t y) {
    return lp_dsu_find(dsu, x) == lp_dsu_find(dsu, y);
}

static inline int64_t lp_dsu_size(LpDSU* dsu, int64_t x) {
    return dsu->size[lp_dsu_find(dsu, x)];
}

static inline void lp_dsu_free(LpDSU* dsu) {
    free(dsu->parent);
    free(dsu->rank);
    free(dsu->size);
}

/* ================================================================
 * MIN-HEAP / PRIORITY QUEUE
 * ================================================================ */

typedef struct {
    int64_t* data;
    int64_t* priority;  /* For priority queue */
    int64_t size;
    int64_t cap;
} LpHeap;

static inline LpHeap lp_heap_new(int64_t cap) {
    LpHeap h;
    h.data = (int64_t*)malloc(cap * sizeof(int64_t));
    h.priority = (int64_t*)malloc(cap * sizeof(int64_t));
    h.size = 0;
    h.cap = cap;
    return h;
}

static inline void lp_heap_free(LpHeap* h) {
    free(h->data);
    free(h->priority);
}

static inline void _lp_heap_sift_up(LpHeap* h, int64_t idx) {
    while (idx > 0) {
        int64_t parent = (idx - 1) / 2;
        if (h->priority[parent] <= h->priority[idx]) break;
        
        /* Swap */
        int64_t tmp = h->data[idx];
        h->data[idx] = h->data[parent];
        h->data[parent] = tmp;
        
        tmp = h->priority[idx];
        h->priority[idx] = h->priority[parent];
        h->priority[parent] = tmp;
        
        idx = parent;
    }
}

static inline void _lp_heap_sift_down(LpHeap* h, int64_t idx) {
    while (true) {
        int64_t left = 2 * idx + 1;
        int64_t right = 2 * idx + 2;
        int64_t smallest = idx;
        
        if (left < h->size && h->priority[left] < h->priority[smallest]) {
            smallest = left;
        }
        if (right < h->size && h->priority[right] < h->priority[smallest]) {
            smallest = right;
        }
        
        if (smallest == idx) break;
        
        /* Swap */
        int64_t tmp = h->data[idx];
        h->data[idx] = h->data[smallest];
        h->data[smallest] = tmp;
        
        tmp = h->priority[idx];
        h->priority[idx] = h->priority[smallest];
        h->priority[smallest] = tmp;
        
        idx = smallest;
    }
}

static inline void lp_heap_push(LpHeap* h, int64_t value, int64_t priority) {
    if (h->size >= h->cap) {
        h->cap *= 2;
        h->data = (int64_t*)realloc(h->data, h->cap * sizeof(int64_t));
        h->priority = (int64_t*)realloc(h->priority, h->cap * sizeof(int64_t));
    }
    
    h->data[h->size] = value;
    h->priority[h->size] = priority;
    _lp_heap_sift_up(h, h->size);
    h->size++;
}

static inline int64_t lp_heap_top(LpHeap* h) {
    return h->data[0];
}

static inline int64_t lp_heap_top_priority(LpHeap* h) {
    return h->priority[0];
}

static inline void lp_heap_pop(LpHeap* h) {
    if (h->size == 0) return;
    
    h->data[0] = h->data[h->size - 1];
    h->priority[0] = h->priority[h->size - 1];
    h->size--;
    
    if (h->size > 0) {
        _lp_heap_sift_down(h, 0);
    }
}

static inline bool lp_heap_is_empty(LpHeap* h) {
    return h->size == 0;
}

/* ================================================================
 * FENWICK TREE (Binary Indexed Tree)
 * ================================================================ */

typedef struct {
    int64_t* tree;
    int64_t n;
} LpFenwick;

static inline LpFenwick lp_fenwick_new(int64_t n) {
    LpFenwick f;
    f.n = n;
    f.tree = (int64_t*)calloc(n + 1, sizeof(int64_t));
    return f;
}

static inline void lp_fenwick_free(LpFenwick* f) {
    free(f->tree);
}

static inline void lp_fenwick_add(LpFenwick* f, int64_t idx, int64_t delta) {
    idx++;  /* 1-indexed internally */
    while (idx <= f->n) {
        f->tree[idx] += delta;
        idx += idx & (-idx);
    }
}

static inline int64_t lp_fenwick_prefix_sum(LpFenwick* f, int64_t idx) {
    idx++;  /* 1-indexed internally */
    int64_t sum = 0;
    while (idx > 0) {
        sum += f->tree[idx];
        idx -= idx & (-idx);
    }
    return sum;
}

static inline int64_t lp_fenwick_range_sum(LpFenwick* f, int64_t l, int64_t r) {
    if (l == 0) return lp_fenwick_prefix_sum(f, r);
    return lp_fenwick_prefix_sum(f, r) - lp_fenwick_prefix_sum(f, l - 1);
}

#endif /* LP_CP_H */
