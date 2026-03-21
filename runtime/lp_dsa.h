/*
 * LP Data Structures & Algorithms (DSA) Module
 *
 * High-performance utilities for competitive programming:
 * - Fast I/O (scanf/printf style)
 * - Number Theory (sieve, mod_pow, gcd, etc.)
 * - Basic Data Structures (Stack, Queue, Deque, Heap, DSU)
 * - Advanced Data Structures (Segment Tree, Fenwick Tree)
 * - Graph Algorithms (BFS, DFS, Dijkstra, Floyd-Warshall)
 * - String Algorithms (KMP, Z-algorithm)
 * - Geometry (Point, Line, Polygon, Convex Hull)
 *
 * All functions are optimized for speed and minimal overhead.
 */

#ifndef LP_DSA_H
#define LP_DSA_H

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

static inline void lp_io_write_float(double x) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.4f", x);
    lp_io_write_str(buf);
}

static inline void lp_io_write_float_ln(double x) {
    lp_io_write_float(x);
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

/* ================================================================
 * SEGMENT TREE with Lazy Propagation
 * ================================================================ */

typedef struct {
    int64_t* tree;      /* Segment tree */
    int64_t* lazy;      /* Lazy propagation values */
    int64_t* arr;       /* Original array */
    int64_t n;          /* Size */
    int64_t op;         /* 0=sum, 1=min, 2=max */
    int64_t identity;   /* Identity element for operation */
} LpSegTree;

/* Create segment tree */
static inline LpSegTree lp_segtree_new(int64_t* arr, int64_t n, const char* op_type) {
    LpSegTree st;
    st.n = n;
    st.arr = arr;
    st.tree = (int64_t*)calloc(4 * n, sizeof(int64_t));
    st.lazy = (int64_t*)calloc(4 * n, sizeof(int64_t));
    
    /* Set operation type */
    if (strcmp(op_type, "min") == 0) {
        st.op = 1;
        st.identity = INT64_MAX;
    } else if (strcmp(op_type, "max") == 0) {
        st.op = 2;
        st.identity = INT64_MIN;
    } else {
        st.op = 0;  /* sum */
        st.identity = 0;
    }
    
    return st;
}

/* Build segment tree */
static inline void _lp_segtree_build(LpSegTree* st, int64_t node, int64_t start, int64_t end) {
    if (start == end) {
        st->tree[node] = st->arr[start];
    } else {
        int64_t mid = (start + end) / 2;
        _lp_segtree_build(st, 2 * node, start, mid);
        _lp_segtree_build(st, 2 * node + 1, mid + 1, end);
        
        if (st->op == 0) st->tree[node] = st->tree[2 * node] + st->tree[2 * node + 1];
        else if (st->op == 1) st->tree[node] = (st->tree[2 * node] < st->tree[2 * node + 1]) ? st->tree[2 * node] : st->tree[2 * node + 1];
        else st->tree[node] = (st->tree[2 * node] > st->tree[2 * node + 1]) ? st->tree[2 * node] : st->tree[2 * node + 1];
    }
}

static inline void lp_segtree_build(LpSegTree* st) {
    _lp_segtree_build(st, 1, 0, st->n - 1);
}

/* Push lazy value */
static inline void _lp_segtree_push(LpSegTree* st, int64_t node, int64_t start, int64_t end) {
    if (st->lazy[node] != 0) {
        if (st->op == 0) {
            st->tree[node] += st->lazy[node] * (end - start + 1);
        } else {
            st->tree[node] += st->lazy[node];
        }
        
        if (start != end) {
            st->lazy[2 * node] += st->lazy[node];
            st->lazy[2 * node + 1] += st->lazy[node];
        }
        st->lazy[node] = 0;
    }
}

/* Range update */
static inline void _lp_segtree_update_range(LpSegTree* st, int64_t node, int64_t start, int64_t end, int64_t l, int64_t r, int64_t val) {
    _lp_segtree_push(st, node, start, end);
    
    if (start > r || end < l) return;
    
    if (l <= start && end <= r) {
        st->lazy[node] += val;
        _lp_segtree_push(st, node, start, end);
        return;
    }
    
    int64_t mid = (start + end) / 2;
    _lp_segtree_update_range(st, 2 * node, start, mid, l, r, val);
    _lp_segtree_update_range(st, 2 * node + 1, mid + 1, end, l, r, val);
    
    if (st->op == 0) st->tree[node] = st->tree[2 * node] + st->tree[2 * node + 1];
    else if (st->op == 1) st->tree[node] = (st->tree[2 * node] < st->tree[2 * node + 1]) ? st->tree[2 * node] : st->tree[2 * node + 1];
    else st->tree[node] = (st->tree[2 * node] > st->tree[2 * node + 1]) ? st->tree[2 * node] : st->tree[2 * node + 1];
}

static inline void lp_segtree_update(LpSegTree* st, int64_t l, int64_t r, int64_t val) {
    _lp_segtree_update_range(st, 1, 0, st->n - 1, l, r, val);
}

/* Range query */
static inline int64_t _lp_segtree_query(LpSegTree* st, int64_t node, int64_t start, int64_t end, int64_t l, int64_t r) {
    _lp_segtree_push(st, node, start, end);
    
    if (start > r || end < l) return st->identity;
    
    if (l <= start && end <= r) return st->tree[node];
    
    int64_t mid = (start + end) / 2;
    int64_t left = _lp_segtree_query(st, 2 * node, start, mid, l, r);
    int64_t right = _lp_segtree_query(st, 2 * node + 1, mid + 1, end, l, r);
    
    if (st->op == 0) return left + right;
    else if (st->op == 1) return (left < right) ? left : right;
    else return (left > right) ? left : right;
}

static inline int64_t lp_segtree_query(LpSegTree* st, int64_t l, int64_t r) {
    return _lp_segtree_query(st, 1, 0, st->n - 1, l, r);
}

static inline void lp_segtree_free(LpSegTree* st) {
    free(st->tree);
    free(st->lazy);
}

/* ================================================================
 * STACK
 * ================================================================ */

typedef struct {
    int64_t* data;
    int64_t size;
    int64_t cap;
} LpStack;

static inline LpStack lp_stack_new(int64_t cap) {
    LpStack s;
    s.data = (int64_t*)malloc(cap * sizeof(int64_t));
    s.size = 0;
    s.cap = cap;
    return s;
}

static inline void lp_stack_push(LpStack* s, int64_t val) {
    if (s->size >= s->cap) {
        s->cap *= 2;
        s->data = (int64_t*)realloc(s->data, s->cap * sizeof(int64_t));
    }
    s->data[s->size++] = val;
}

static inline int64_t lp_stack_pop(LpStack* s) {
    return s->data[--s->size];
}

static inline int64_t lp_stack_top(LpStack* s) {
    return s->data[s->size - 1];
}

static inline bool lp_stack_is_empty(LpStack* s) {
    return s->size == 0;
}

static inline void lp_stack_free(LpStack* s) {
    free(s->data);
}

/* ================================================================
 * QUEUE
 * ================================================================ */

typedef struct {
    int64_t* data;
    int64_t front;
    int64_t rear;
    int64_t size;
    int64_t cap;
} LpQueue;

static inline LpQueue lp_queue_new(int64_t cap) {
    LpQueue q;
    q.data = (int64_t*)malloc(cap * sizeof(int64_t));
    q.front = 0;
    q.rear = 0;
    q.size = 0;
    q.cap = cap;
    return q;
}

static inline void lp_queue_push(LpQueue* q, int64_t val) {
    if (q->size >= q->cap) {
        int64_t new_cap = q->cap * 2;
        int64_t* new_data = (int64_t*)malloc(new_cap * sizeof(int64_t));
        for (int64_t i = 0; i < q->size; i++) {
            new_data[i] = q->data[(q->front + i) % q->cap];
        }
        free(q->data);
        q->data = new_data;
        q->front = 0;
        q->rear = q->size;
        q->cap = new_cap;
    }
    q->data[q->rear] = val;
    q->rear = (q->rear + 1) % q->cap;
    q->size++;
}

static inline int64_t lp_queue_pop(LpQueue* q) {
    int64_t val = q->data[q->front];
    q->front = (q->front + 1) % q->cap;
    q->size--;
    return val;
}

static inline int64_t lp_queue_front(LpQueue* q) {
    return q->data[q->front];
}

static inline bool lp_queue_is_empty(LpQueue* q) {
    return q->size == 0;
}

static inline void lp_queue_free(LpQueue* q) {
    free(q->data);
}

/* ================================================================
 * DEQUE (Double-ended Queue)
 * ================================================================ */

typedef struct {
    int64_t* data;
    int64_t front;
    int64_t rear;
    int64_t size;
    int64_t cap;
} LpDeque;

static inline LpDeque lp_deque_new(int64_t cap) {
    LpDeque d;
    d.data = (int64_t*)malloc(cap * sizeof(int64_t));
    d.front = cap / 2;
    d.rear = cap / 2;
    d.size = 0;
    d.cap = cap;
    return d;
}

static inline void _lp_deque_expand(LpDeque* d) {
    int64_t new_cap = d->cap * 2;
    int64_t* new_data = (int64_t*)malloc(new_cap * sizeof(int64_t));
    int64_t offset = (new_cap - d->size) / 2;
    
    for (int64_t i = 0; i < d->size; i++) {
        new_data[offset + i] = d->data[(d->front + i) % d->cap];
    }
    
    free(d->data);
    d->data = new_data;
    d->front = offset;
    d->rear = offset + d->size;
    d->cap = new_cap;
}

static inline void lp_deque_push_front(LpDeque* d, int64_t val) {
    if (d->front == 0) _lp_deque_expand(d);
    d->data[--d->front] = val;
    d->size++;
}

static inline void lp_deque_push_back(LpDeque* d, int64_t val) {
    if (d->rear >= d->cap) _lp_deque_expand(d);
    d->data[d->rear++] = val;
    d->size++;
}

static inline int64_t lp_deque_pop_front(LpDeque* d) {
    d->size--;
    return d->data[d->front++];
}

static inline int64_t lp_deque_pop_back(LpDeque* d) {
    d->size--;
    return d->data[--d->rear];
}

static inline int64_t lp_deque_front(LpDeque* d) {
    return d->data[d->front];
}

static inline int64_t lp_deque_back(LpDeque* d) {
    return d->data[d->rear - 1];
}

static inline bool lp_deque_is_empty(LpDeque* d) {
    return d->size == 0;
}

static inline void lp_deque_free(LpDeque* d) {
    free(d->data);
}

/* ================================================================
 * GRAPH ALGORITHMS
 * ================================================================ */

/* Graph using adjacency list */
typedef struct {
    int64_t to;
    int64_t weight;
} LpEdge;

typedef struct {
    LpEdge** adj;      /* Adjacency list */
    int64_t* adj_size;
    int64_t* adj_cap;
    int64_t n;          /* Number of vertices */
    bool directed;
} LpGraph;

static inline LpGraph lp_graph_new(int64_t n, bool directed) {
    LpGraph g;
    g.n = n;
    g.directed = directed;
    g.adj = (LpEdge**)malloc(n * sizeof(LpEdge*));
    g.adj_size = (int64_t*)calloc(n, sizeof(int64_t));
    g.adj_cap = (int64_t*)malloc(n * sizeof(int64_t));
    
    for (int64_t i = 0; i < n; i++) {
        g.adj_cap[i] = 16;
        g.adj[i] = (LpEdge*)malloc(16 * sizeof(LpEdge));
    }
    
    return g;
}

static inline void lp_graph_add_edge(LpGraph* g, int64_t u, int64_t v, int64_t weight) {
    if (g->adj_size[u] >= g->adj_cap[u]) {
        g->adj_cap[u] *= 2;
        g->adj[u] = (LpEdge*)realloc(g->adj[u], g->adj_cap[u] * sizeof(LpEdge));
    }
    g->adj[u][g->adj_size[u]].to = v;
    g->adj[u][g->adj_size[u]].weight = weight;
    g->adj_size[u]++;
    
    if (!g->directed) {
        if (g->adj_size[v] >= g->adj_cap[v]) {
            g->adj_cap[v] *= 2;
            g->adj[v] = (LpEdge*)realloc(g->adj[v], g->adj_cap[v] * sizeof(LpEdge));
        }
        g->adj[v][g->adj_size[v]].to = u;
        g->adj[v][g->adj_size[v]].weight = weight;
        g->adj_size[v]++;
    }
}

/* BFS - returns distances array */
static inline int64_t* lp_graph_bfs(LpGraph* g, int64_t start) {
    int64_t* dist = (int64_t*)malloc(g->n * sizeof(int64_t));
    for (int64_t i = 0; i < g->n; i++) dist[i] = -1;
    
    LpQueue q = lp_queue_new(g->n);
    dist[start] = 0;
    lp_queue_push(&q, start);
    
    while (!lp_queue_is_empty(&q)) {
        int64_t u = lp_queue_pop(&q);
        for (int64_t i = 0; i < g->adj_size[u]; i++) {
            int64_t v = g->adj[u][i].to;
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                lp_queue_push(&q, v);
            }
        }
    }
    
    lp_queue_free(&q);
    return dist;
}

/* DFS - returns discovery times and finish times */
typedef struct {
    int64_t* disc;      /* Discovery time */
    int64_t* finish;    /* Finish time */
    int64_t* parent;
} LpDFSResult;

static inline void _lp_graph_dfs_visit(LpGraph* g, int64_t u, int64_t* time, int64_t* disc, int64_t* finish, int64_t* parent) {
    disc[u] = ++(*time);
    
    for (int64_t i = 0; i < g->adj_size[u]; i++) {
        int64_t v = g->adj[u][i].to;
        if (disc[v] == -1) {
            parent[v] = u;
            _lp_graph_dfs_visit(g, v, time, disc, finish, parent);
        }
    }
    
    finish[u] = ++(*time);
}

static inline LpDFSResult lp_graph_dfs(LpGraph* g, int64_t start) {
    LpDFSResult result;
    result.disc = (int64_t*)malloc(g->n * sizeof(int64_t));
    result.finish = (int64_t*)malloc(g->n * sizeof(int64_t));
    result.parent = (int64_t*)malloc(g->n * sizeof(int64_t));
    
    for (int64_t i = 0; i < g->n; i++) {
        result.disc[i] = -1;
        result.finish[i] = -1;
        result.parent[i] = -1;
    }
    
    int64_t time = 0;
    if (start >= 0) {
        _lp_graph_dfs_visit(g, start, &time, result.disc, result.finish, result.parent);
    } else {
        /* DFS from all unvisited vertices */
        for (int64_t i = 0; i < g->n; i++) {
            if (result.disc[i] == -1) {
                _lp_graph_dfs_visit(g, i, &time, result.disc, result.finish, result.parent);
            }
        }
    }
    
    return result;
}

/* Dijkstra's algorithm */
static inline int64_t* lp_graph_dijkstra(LpGraph* g, int64_t start) {
    int64_t* dist = (int64_t*)malloc(g->n * sizeof(int64_t));
    bool* visited = (bool*)calloc(g->n, sizeof(bool));
    
    for (int64_t i = 0; i < g->n; i++) dist[i] = INT64_MAX;
    dist[start] = 0;
    
    LpHeap pq = lp_heap_new(g->n);
    lp_heap_push(&pq, start, 0);
    
    while (!lp_heap_is_empty(&pq)) {
        int64_t u = lp_heap_top(&pq);
        int64_t d = lp_heap_top_priority(&pq);
        lp_heap_pop(&pq);
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (int64_t i = 0; i < g->adj_size[u]; i++) {
            int64_t v = g->adj[u][i].to;
            int64_t w = g->adj[u][i].weight;
            
            if (!visited[v] && dist[u] != INT64_MAX && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                lp_heap_push(&pq, v, dist[v]);
            }
        }
    }
    
    lp_heap_free(&pq);
    free(visited);
    return dist;
}

/* Floyd-Warshall */
static inline int64_t** lp_graph_floyd_warshall(LpGraph* g) {
    int64_t** dist = (int64_t**)malloc(g->n * sizeof(int64_t*));
    for (int64_t i = 0; i < g->n; i++) {
        dist[i] = (int64_t*)malloc(g->n * sizeof(int64_t));
        for (int64_t j = 0; j < g->n; j++) {
            dist[i][j] = (i == j) ? 0 : INT64_MAX;
        }
    }
    
    /* Initialize with edge weights */
    for (int64_t u = 0; u < g->n; u++) {
        for (int64_t i = 0; i < g->adj_size[u]; i++) {
            int64_t v = g->adj[u][i].to;
            int64_t w = g->adj[u][i].weight;
            if (w < dist[u][v]) dist[u][v] = w;
        }
    }
    
    /* Floyd-Warshall */
    for (int64_t k = 0; k < g->n; k++) {
        for (int64_t i = 0; i < g->n; i++) {
            for (int64_t j = 0; j < g->n; j++) {
                if (dist[i][k] != INT64_MAX && dist[k][j] != INT64_MAX) {
                    if (dist[i][k] + dist[k][j] < dist[i][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                    }
                }
            }
        }
    }
    
    return dist;
}

static inline void lp_graph_free(LpGraph* g) {
    for (int64_t i = 0; i < g->n; i++) {
        free(g->adj[i]);
    }
    free(g->adj);
    free(g->adj_size);
    free(g->adj_cap);
}

/* ================================================================
 * STRING ALGORITHMS
 * ================================================================ */

/* KMP - compute LPS (Longest Proper Prefix which is also Suffix) */
static inline int64_t* lp_kmp_lps(const char* pattern) {
    int64_t m = strlen(pattern);
    int64_t* lps = (int64_t*)calloc(m, sizeof(int64_t));
    
    int64_t len = 0;
    int64_t i = 1;
    
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i++] = 0;
            }
        }
    }
    
    return lps;
}

/* KMP search - returns array of positions (first element is count) */
static inline int64_t* lp_kmp_search(const char* text, const char* pattern) {
    int64_t n = strlen(text);
    int64_t m = strlen(pattern);
    
    int64_t* lps = lp_kmp_lps(pattern);
    int64_t* result = (int64_t*)malloc((n + 1) * sizeof(int64_t));
    int64_t count = 0;
    
    int64_t i = 0, j = 0;
    while (i < n) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
        }
        
        if (j == m) {
            result[++count] = i - j;
            j = lps[j - 1];
        } else if (i < n && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    
    result[0] = count;
    free(lps);
    return result;
}

/* Z-algorithm */
static inline int64_t* lp_z_algorithm(const char* s) {
    int64_t n = strlen(s);
    int64_t* z = (int64_t*)calloc(n, sizeof(int64_t));
    
    int64_t l = 0, r = 0;
    for (int64_t i = 1; i < n; i++) {
        if (i < r) {
            z[i] = (z[i - l] < r - i) ? z[i - l] : r - i;
        }
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) {
            z[i]++;
        }
        if (i + z[i] > r) {
            l = i;
            r = i + z[i];
        }
    }
    z[0] = n;
    
    return z;
}

/* Rolling hash for string matching */
typedef struct {
    uint64_t* hash;
    uint64_t* power;
    uint64_t mod;
    int64_t n;
} LpRollingHash;

static inline LpRollingHash lp_rolling_hash_new(const char* s, uint64_t base, uint64_t mod) {
    LpRollingHash rh;
    rh.n = strlen(s);
    rh.mod = mod;
    rh.hash = (uint64_t*)malloc((rh.n + 1) * sizeof(uint64_t));
    rh.power = (uint64_t*)malloc((rh.n + 1) * sizeof(uint64_t));
    
    rh.hash[0] = 0;
    rh.power[0] = 1;
    
    for (int64_t i = 0; i < rh.n; i++) {
        rh.hash[i + 1] = (rh.hash[i] * base + (uint64_t)s[i]) % mod;
        rh.power[i + 1] = (rh.power[i] * base) % mod;
    }
    
    return rh;
}

static inline uint64_t lp_rolling_hash_get(LpRollingHash* rh, int64_t l, int64_t r) {
    return (rh->hash[r] - rh->hash[l] * rh->power[r - l] % rh->mod + rh->mod) % rh->mod;
}

static inline void lp_rolling_hash_free(LpRollingHash* rh) {
    free(rh->hash);
    free(rh->power);
}

/* ================================================================
 * GEOMETRY
 * ================================================================ */

typedef struct {
    double x;
    double y;
} LpPoint;

typedef struct {
    LpPoint a;
    LpPoint b;
} LpLine;

/* Cross product: (b - a) x (c - a) */
static inline double lp_cross(LpPoint a, LpPoint b, LpPoint c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/* Distance squared */
static inline double lp_dist_sq(LpPoint a, LpPoint b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

/* Distance */
static inline double lp_dist(LpPoint a, LpPoint b) {
    return sqrt(lp_dist_sq(a, b));
}

/* Check if points are collinear */
static inline bool lp_collinear(LpPoint a, LpPoint b, LpPoint c) {
    return fabs(lp_cross(a, b, c)) < 1e-9;
}

/* Check if c is on segment ab */
static inline bool lp_on_segment(LpPoint a, LpPoint b, LpPoint c) {
    return lp_collinear(a, b, c) &&
           c.x >= fmin(a.x, b.x) && c.x <= fmax(a.x, b.x) &&
           c.y >= fmin(a.y, b.y) && c.y <= fmax(a.y, b.y);
}

/* Check if two segments intersect */
static inline bool lp_segments_intersect(LpPoint p1, LpPoint p2, LpPoint p3, LpPoint p4) {
    double d1 = lp_cross(p3, p4, p1);
    double d2 = lp_cross(p3, p4, p2);
    double d3 = lp_cross(p1, p2, p3);
    double d4 = lp_cross(p1, p2, p4);
    
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }
    
    if (fabs(d1) < 1e-9 && lp_on_segment(p3, p4, p1)) return true;
    if (fabs(d2) < 1e-9 && lp_on_segment(p3, p4, p2)) return true;
    if (fabs(d3) < 1e-9 && lp_on_segment(p1, p2, p3)) return true;
    if (fabs(d4) < 1e-9 && lp_on_segment(p1, p2, p4)) return true;
    
    return false;
}

/* Convex Hull (Andrew's monotone chain) */
typedef struct {
    LpPoint* points;
    int64_t count;
} LpPolygon;

static inline int _lp_point_cmp(const void* a, const void* b) {
    LpPoint* pa = (LpPoint*)a;
    LpPoint* pb = (LpPoint*)b;
    if (pa->x != pb->x) return (pa->x < pb->x) ? -1 : 1;
    return (pa->y < pb->y) ? -1 : 1;
}

static inline LpPolygon lp_convex_hull(LpPoint* points, int64_t n) {
    LpPolygon hull;
    hull.points = (LpPoint*)malloc(2 * n * sizeof(LpPoint));
    hull.count = 0;
    
    if (n < 3) {
        for (int64_t i = 0; i < n; i++) {
            hull.points[i] = points[i];
        }
        hull.count = n;
        return hull;
    }
    
    /* Sort points */
    qsort(points, n, sizeof(LpPoint), _lp_point_cmp);
    
    /* Build lower hull */
    int64_t k = 0;
    for (int64_t i = 0; i < n; i++) {
        while (k >= 2 && lp_cross(hull.points[k-2], hull.points[k-1], points[i]) <= 0) {
            k--;
        }
        hull.points[k++] = points[i];
    }
    
    /* Build upper hull */
    int64_t t = k + 1;
    for (int64_t i = n - 2; i >= 0; i--) {
        while (k >= t && lp_cross(hull.points[k-2], hull.points[k-1], points[i]) <= 0) {
            k--;
        }
        hull.points[k++] = points[i];
    }
    
    hull.count = k - 1;
    return hull;
}

/* Polygon area (shoelace formula) */
static inline double lp_polygon_area(LpPoint* points, int64_t n) {
    double area = 0;
    for (int64_t i = 0; i < n; i++) {
        int64_t j = (i + 1) % n;
        area += points[i].x * points[j].y;
        area -= points[j].x * points[i].y;
    }
    return fabs(area) / 2.0;
}

/* Point in polygon (ray casting) */
static inline bool lp_point_in_polygon(LpPoint* points, int64_t n, LpPoint p) {
    bool inside = false;
    for (int64_t i = 0, j = n - 1; i < n; j = i++) {
        if (((points[i].y > p.y) != (points[j].y > p.y)) &&
            (p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}

#endif /* LP_DSA_H */
