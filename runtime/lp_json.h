#ifndef LP_JSON_H
#define LP_JSON_H

#include "lp_dict.h"
#include <ctype.h>
#include <stdio.h>

static inline void lp_json_skip_whitespace(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static inline LpVal lp_json_parse_value(const char **p);

static inline LpVal lp_json_parse_string(const char **p) {
    (*p)++; /* Skip '"' */
    const char *start = *p;
    while (**p && **p != '"') {
        if (**p == '\\') (*p)++; /* Skip escaped char */
        (*p)++;
    }
    int len = *p - start;
    char *s = (char*)malloc(len + 1);
    memcpy(s, start, len);
    s[len] = '\0';
    if (**p == '"') (*p)++;
    
    LpVal v = lp_val_str(s);
    free(s);
    return v;
}

static inline LpVal lp_json_parse_number(const char **p) {
    char *endptr;
    double d = strtod(*p, &endptr);
    int is_float = 0;
    for (const char *c = *p; c < endptr; c++) {
        if (*c == '.' || *c == 'e' || *c == 'E') { is_float = 1; break; }
    }
    *p = endptr;
    if (is_float) return lp_val_float(d);
    return lp_val_int((int64_t)d);
}

static inline LpVal lp_json_parse_array(const char **p) {
    (*p)++; /* '[' */
    LpList *l = lp_list_new();
    lp_json_skip_whitespace(p);
    while (**p && **p != ']') {
        LpVal val = lp_json_parse_value(p);
        lp_list_append(l, val);
        lp_json_skip_whitespace(p);
        if (**p == ',') {
            (*p)++;
            lp_json_skip_whitespace(p);
        }
    }
    if (**p == ']') (*p)++;
    return lp_val_list(l);
}

static inline LpVal lp_json_parse_object(const char **p) {
    (*p)++; /* '{' */
    LpDict *d = lp_dict_new();
    lp_json_skip_whitespace(p);
    while (**p && **p != '}') {
        LpVal key_val = lp_json_parse_string(p);
        lp_json_skip_whitespace(p);
        if (**p == ':') (*p)++;
        LpVal val = lp_json_parse_value(p);
        lp_dict_set(d, key_val.as.s, val);
        free(key_val.as.s);
        lp_json_skip_whitespace(p);
        if (**p == ',') {
            (*p)++;
            lp_json_skip_whitespace(p);
        }
    }
    if (**p == '}') (*p)++;
    return lp_val_dict(d);
}

static inline LpVal lp_json_parse_value(const char **p) {
    lp_json_skip_whitespace(p);
    if (**p == '"') return lp_json_parse_string(p);
    if (**p == '{') return lp_json_parse_object(p);
    if (**p == '[') return lp_json_parse_array(p);
    if (strncmp(*p, "true", 4) == 0) { *p += 4; return lp_val_bool(1); }
    if (strncmp(*p, "false", 5) == 0) { *p += 5; return lp_val_bool(0); }
    if (strncmp(*p, "null", 4) == 0) { *p += 4; return lp_val_null(); }
    if (**p == '-' || isdigit((unsigned char)**p)) return lp_json_parse_number(p);
    return lp_val_null();
}

static inline LpVal lp_json_loads(const char *s) {
    const char *p = s;
    return lp_json_parse_value(&p);
}

typedef struct { char *data; int len; int cap; } LpJsonBuffer;

static inline void lp_jb_write(LpJsonBuffer *b, const char *s, int sn) {
    if (b->len + sn >= b->cap) {
        b->cap = (b->len + sn + 1) * 2;
        b->data = (char*)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, sn);
    b->len += sn;
    b->data[b->len] = '\0';
}

static inline void lp_jb_writes(LpJsonBuffer *b, const char *s) {
    lp_jb_write(b, s, (int)strlen(s));
}

static inline void lp_jb_printf(LpJsonBuffer *b, const char *fmt, ...) {
    char tmp[512];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    lp_jb_write(b, tmp, n);
}

static inline void lp_json_dump_val(LpVal v, LpJsonBuffer *b) {
    if (v.type == LP_VAL_NULL) lp_jb_writes(b, "null");
    else if (v.type == LP_VAL_BOOL) lp_jb_writes(b, v.as.b ? "true" : "false");
    else if (v.type == LP_VAL_INT) lp_jb_printf(b, "%lld", (long long)v.as.i);
    else if (v.type == LP_VAL_FLOAT) lp_jb_printf(b, "%f", v.as.f);
    else if (v.type == LP_VAL_STRING) lp_jb_printf(b, "\"%s\"", v.as.s);
    else if (v.type == LP_VAL_LIST) {
        lp_jb_writes(b, "[");
        for (int i=0; i<v.as.l->len; i++) {
            if (i > 0) lp_jb_writes(b, ", ");
            lp_json_dump_val(v.as.l->items[i], b);
        }
        lp_jb_writes(b, "]");
    } else if (v.type == LP_VAL_DICT) {
        lp_jb_writes(b, "{");
        int first = 1;
        for (int i=0; i<v.as.d->capacity; i++) {
            if (v.as.d->entries[i].is_occupied) {
                if (!first) lp_jb_writes(b, ", ");
                lp_jb_printf(b, "\"%s\": ", v.as.d->entries[i].key);
                lp_json_dump_val(v.as.d->entries[i].value, b);
                first = 0;
            }
        }
        lp_jb_writes(b, "}");
    }
}

static inline const char* lp_json_dumps(LpVal v) {
    LpJsonBuffer b;
    b.len = 0; b.cap = 64;
    b.data = (char*)malloc(b.cap);
    b.data[0] = '\0';
    lp_json_dump_val(v, &b);
    return b.data;
}

#endif /* LP_JSON_H */
