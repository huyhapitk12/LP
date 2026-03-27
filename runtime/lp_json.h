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
                lp_jb_printf(b, "\"%s\": ", lp_entry_key(&v.as.d->entries[i]));
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

/* ========================================
 * ADDITIONAL JSON FUNCTIONS
 * ======================================== */

/* Pretty-print JSON with indentation */
static inline void lp_json_dump_val_pretty(LpVal v, LpJsonBuffer *b, int indent, int level) {
    char pad[256];
    int pad_len = (level * indent < 255) ? level * indent : 255;
    memset(pad, ' ', pad_len);
    pad[pad_len] = '\0';
    
    char pad_inner[256];
    int inner_len = ((level + 1) * indent < 255) ? (level + 1) * indent : 255;
    memset(pad_inner, ' ', inner_len);
    pad_inner[inner_len] = '\0';

    if (v.type == LP_VAL_NULL) lp_jb_writes(b, "null");
    else if (v.type == LP_VAL_BOOL) lp_jb_writes(b, v.as.b ? "true" : "false");
    else if (v.type == LP_VAL_INT) lp_jb_printf(b, "%lld", (long long)v.as.i);
    else if (v.type == LP_VAL_FLOAT) lp_jb_printf(b, "%g", v.as.f);
    else if (v.type == LP_VAL_STRING) lp_jb_printf(b, "\"%s\"", v.as.s);
    else if (v.type == LP_VAL_LIST) {
        if (!v.as.l || v.as.l->len == 0) { lp_jb_writes(b, "[]"); return; }
        lp_jb_writes(b, "[\n");
        for (int i = 0; i < v.as.l->len; i++) {
            if (i > 0) lp_jb_writes(b, ",\n");
            lp_jb_writes(b, pad_inner);
            lp_json_dump_val_pretty(v.as.l->items[i], b, indent, level + 1);
        }
        lp_jb_writes(b, "\n");
        lp_jb_writes(b, pad);
        lp_jb_writes(b, "]");
    } else if (v.type == LP_VAL_DICT) {
        if (!v.as.d || v.as.d->count == 0) { lp_jb_writes(b, "{}"); return; }
        lp_jb_writes(b, "{\n");
        int first = 1;
        for (int i = 0; i < v.as.d->capacity; i++) {
            if (v.as.d->entries[i].is_occupied) {
                if (!first) lp_jb_writes(b, ",\n");
                lp_jb_writes(b, pad_inner);
                lp_jb_printf(b, "\"%s\": ", lp_entry_key(&v.as.d->entries[i]));
                lp_json_dump_val_pretty(v.as.d->entries[i].value, b, indent, level + 1);
                first = 0;
            }
        }
        lp_jb_writes(b, "\n");
        lp_jb_writes(b, pad);
        lp_jb_writes(b, "}");
    }
}

/* json.dumps(val, indent=2) — pretty-printed JSON string */
static inline const char* lp_json_dumps_pretty(LpVal v, int indent) {
    LpJsonBuffer b;
    b.len = 0; b.cap = 256;
    b.data = (char*)malloc(b.cap);
    b.data[0] = '\0';
    lp_json_dump_val_pretty(v, &b, indent, 0);
    return b.data;
}

/* json.load(filename) — read JSON from file */
static inline LpVal lp_json_load_file(const char *filename) {
    if (!filename) return lp_val_null();
    FILE *f = fopen(filename, "rb");
    if (!f) return lp_val_null();
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return lp_val_null(); }
    char *buf = (char*)malloc(len + 1);
    if (!buf) { fclose(f); return lp_val_null(); }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    LpVal result = lp_json_loads(buf);
    free(buf);
    return result;
}

/* json.dump(filename, val) — write JSON to file */
static inline int lp_json_save_file(const char *filename, LpVal val, int pretty) {
    if (!filename) return 0;
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;
    const char *str = pretty ? lp_json_dumps_pretty(val, 2) : lp_json_dumps(val);
    if (str) {
        fwrite(str, 1, strlen(str), f);
        free((void*)str);
    }
    fclose(f);
    return 1;
}

/* json.get_path(val, "a.b.c") — access nested JSON by dot-path */
static inline LpVal lp_json_get_path(LpVal val, const char *path) {
    if (!path || !*path) return val;
    LpVal current = val;
    char key[256];
    const char *p = path;
    while (*p) {
        int k = 0;
        while (*p && *p != '.' && k < 255) {
            key[k++] = *p++;
        }
        key[k] = '\0';
        if (*p == '.') p++;
        
        if (current.type == LP_VAL_DICT && current.as.d) {
            current = lp_dict_get(current.as.d, key);
        } else if (current.type == LP_VAL_LIST && current.as.l) {
            int64_t idx = atoll(key);
            if (idx >= 0 && idx < current.as.l->len) {
                current = current.as.l->items[idx];
            } else {
                return lp_val_null();
            }
        } else {
            return lp_val_null();
        }
    }
    return current;
}

#endif /* LP_JSON_H */
