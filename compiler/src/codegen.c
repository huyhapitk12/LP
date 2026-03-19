#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "lp_compat.h"

/* --- Buffer --- */
void buf_init(Buffer *b) { b->data = NULL; b->len = 0; b->cap = 0; }

void buf_write(Buffer *b, const char *s) {
    int slen = (int)strlen(s);
    if (b->len + slen >= b->cap) {
        b->cap = (b->len + slen + 1) * 2;
        char *new_data = (char *)realloc(b->data, b->cap);
        if (!new_data) {
            /* Realloc failed - buffer remains unchanged but null-terminated */
            if (b->data) b->data[b->len] = '\0';
            return;
        }
        b->data = new_data;
    }
    memcpy(b->data + b->len, s, slen);
    b->len += slen;
    b->data[b->len] = '\0';
}

void buf_printf(Buffer *b, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    if (len < 0) return;

    size_t needed = (size_t)b->len + (size_t)len + 1;
    if (needed >= (size_t)b->cap) {
        size_t new_cap = needed * 2;
        if (new_cap < needed) return;
        
        char *new_data = (char *)realloc(b->data, new_cap);
        if (!new_data) {
            return; 
        }
        b->data = new_data;
        b->cap = (int)new_cap;
    }

    va_start(args, fmt);
    vsnprintf(b->data + b->len, len + 1, fmt, args);
    va_end(args);
    b->len += len;
}

void buf_free(Buffer *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }

/* --- Scope / Symbol Table --- */
static Scope *scope_new(Scope *parent) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    s->parent = parent;
    return s;
}

static void scope_free(Scope *s) {
    for (int i = 0; i < s->count; i++) {
        free(s->symbols[i].name);
        free(s->symbols[i].class_name);
        free(s->symbols[i].first_param_class_name);
        free(s->symbols[i].owner_class);
        free(s->symbols[i].base_class);
    }
    free(s);
}

static Symbol *scope_lookup(Scope *s, const char *name) {
    for (; s; s = s->parent)
        for (int i = s->count - 1; i >= 0; i--)
            if (strcmp(s->symbols[i].name, name) == 0)
                return &s->symbols[i];
    return NULL;
}

static Symbol *scope_lookup_field(Scope *s, const char *name) {
    for (; s; s = s->parent)
        for (int i = s->count - 1; i >= 0; i--)
            if (strcmp(s->symbols[i].name, name) == 0 && !s->symbols[i].is_method)
                return &s->symbols[i];
    return NULL;
}

static Symbol *scope_define(Scope *s, const char *name, LpType type) {
    if (s->count >= 512) return NULL;
    Symbol *sym = &s->symbols[s->count++];
    size_t name_len = strlen(name);
    sym->name = (char *)malloc(name_len + 1);
    if (sym->name) {
        strncpy(sym->name, name, name_len + 1);
        sym->name[name_len] = '\0';
    }
    sym->type = type;
    sym->class_name = NULL;
    sym->declared = 0;
    sym->is_function = 0;
    sym->is_variadic = 0;
    sym->has_kwargs = 0;
    sym->num_params = 0;
    for (int i = 0; i < 16; i++) {
        sym->param_types[i] = LP_UNKNOWN;
    }
    sym->first_param_type = LP_UNKNOWN;
    sym->first_param_class_name = NULL;
    sym->access = 0;
    sym->owner_class = NULL;
    sym->base_class = NULL;
    sym->is_method = 0;
    /* Native array initialization */
    sym->array_dims = 0;
    for (int i = 0; i < 4; i++) {
        sym->array_size_expr[i] = NULL;
    }
    return sym;
}

static Symbol *scope_define_obj(Scope *s, const char *name, LpType type, const char *class_name) {
    Symbol *sym = scope_define(s, name, type);
    if (sym && type == LP_OBJECT && class_name) {
        size_t class_name_len = strlen(class_name);
        sym->class_name = (char *)malloc(class_name_len + 1);
        if (sym->class_name) {
            strncpy(sym->class_name, class_name, class_name_len + 1);
            sym->class_name[class_name_len] = '\0';
        }
    }
    return sym;
}

static void codegen_set_error(CodeGen *cg, const char *fmt, ...) {
    va_list args;

    if (cg->had_error) return;

    va_start(args, fmt);
    vsnprintf(cg->error_msg, sizeof(cg->error_msg), fmt, args);
    va_end(args);
    cg->had_error = 1;
}

static int is_class_or_subclass(CodeGen *cg, const char *class_name, const char *target_class) {
    const char *current = class_name;

    while (current && target_class) {
        Symbol *sym;

        if (strcmp(current, target_class) == 0) return 1;
        sym = scope_lookup(cg->scope, current);
        if (!sym || sym->type != LP_CLASS || !sym->base_class) break;
        current = sym->base_class;
    }

    return 0;
}

static int can_access_member(CodeGen *cg, Symbol *mem) {
    if (!mem || !mem->access) return 1;
    if (!cg->current_class || !mem->owner_class) return 0;
    if (mem->access == TOK_PRIVATE) return strcmp(cg->current_class, mem->owner_class) == 0;
    if (mem->access == TOK_PROTECTED) return is_class_or_subclass(cg, cg->current_class, mem->owner_class);
    return 1;
}

static void buf_write_c_string_literal(Buffer *buf, const char *s) {
    buf_write(buf, "\"");
    while (*s) {
        unsigned char ch = (unsigned char)*s++;
        switch (ch) {
            case '\\': buf_write(buf, "\\\\"); break;
            case '"': buf_write(buf, "\\\""); break;
            case '\n': buf_write(buf, "\\n"); break;
            case '\r': buf_write(buf, "\\r"); break;
            case '\t': buf_write(buf, "\\t"); break;
            default:
                if (ch < 32) {
                    char tmp[8];
                    snprintf(tmp, sizeof(tmp), "\\x%02x", ch);
                    buf_write(buf, tmp);
                } else {
                    char tmp[2];
                    tmp[0] = (char)ch;
                    tmp[1] = '\0';
                    buf_write(buf, tmp);
                }
                break;
        }
    }
    buf_write(buf, "\"");
}

/* --- Type mapping --- */
/* Helper: check if annotation is a union type (contains '|') */
static int is_union_type(const char *ann) {
    if (!ann) return 0;
    return strchr(ann, '|') != NULL;
}

/* Helper: check if a type name matches one of the types in a union */
__attribute__((unused))
static int type_matches_union(const char *type_name, const char *union_ann) {
    if (!type_name || !union_ann) return 0;
    
    /* Make a copy to tokenize */
    char *copy = strdup(union_ann);
    if (!copy) return 0;
    
    char *saveptr;
    char *token = strtok_r(copy, "|", &saveptr);
    int found = 0;
    
    while (token != NULL) {
        /* Skip leading/trailing whitespace */
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;
        *(end + 1) = '\0';
        
        if (strcmp(token, type_name) == 0) {
            found = 1;
            break;
        }
        token = strtok_r(NULL, "|", &saveptr);
    }
    
    free(copy);
    return found;
}

/* Check if type annotation is a native array: int[expr] or int[expr][expr] */
static int is_native_array_type(const char *ann) {
    if (!ann) return 0;
    /* Check for int[...] pattern */
    if (strncmp(ann, "int[", 4) == 0) return 1;
    if (strncmp(ann, "i64[", 4) == 0) return 1;
    if (strncmp(ann, "float[", 6) == 0) return 1;
    if (strncmp(ann, "f64[", 4) == 0) return 1;
    return 0;
}

/* Parse native array dimensions from type annotation like "int[n+2][m+2]" 
 * Returns number of dimensions found, extracts size expressions into sizes array */
static int parse_native_array_dims(const char *ann, char *sizes[4]) {
    if (!ann || !is_native_array_type(ann)) return 0;
    
    int dims = 0;
    const char *p = ann;
    
    /* Skip base type name */
    while (*p && *p != '[') p++;
    
    /* Parse each dimension */
    while (*p == '[' && dims < 4) {
        p++; /* skip '[' */
        const char *start = p;
        int depth = 1;
        while (*p && depth > 0) {
            if (*p == '[') depth++;
            else if (*p == ']') depth--;
            if (depth > 0) p++;
        }
        /* Extract size expression */
        int len = (int)(p - start);
        if (len > 0) {
            sizes[dims] = (char *)malloc(len + 1);
            if (sizes[dims]) {
                strncpy(sizes[dims], start, len);
                sizes[dims][len] = '\0';
            }
            dims++;
        }
        if (*p == ']') p++;
    }
    
    return dims;
}

/* Convert size expression to C code with lp_ prefix for variables
 * E.g., "n + 2" -> "lp_n + 2" 
 * Allocates memory for result, caller must free */
static char *convert_size_expr_to_c(const char *expr) {
    if (!expr) return strdup("1");
    
    /* Allocate result buffer - worst case is each char becomes "lp_" + char */
    size_t len = strlen(expr);
    size_t capacity = len * 4 + 1;
    char *result = (char *)malloc(capacity);
    if (!result) return strdup("1");
    
    size_t out_len = 0;
    const char *p = expr;
    
    while (*p) {
        /* Check if this is start of an identifier */
        if (isalpha((unsigned char)*p) || *p == '_') {
            /* Extract identifier */
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            
            /* Check if it's a keyword (skip prefixing) */
            size_t id_len = p - start;
            int is_keyword = (id_len == 2 && strncmp(start, "if", 2) == 0) ||
                            (id_len == 4 && strncmp(start, "true", 4) == 0) ||
                            (id_len == 5 && strncmp(start, "false", 5) == 0);
            
            if (!is_keyword) {
                /* Add lp_ prefix */
                if (out_len + 3 < capacity) {
                    result[out_len++] = 'l';
                    result[out_len++] = 'p';
                    result[out_len++] = '_';
                }
            }
            
            /* Copy identifier */
            for (const char *c = start; c < p && out_len < capacity - 1; c++) {
                result[out_len++] = *c;
            }
        } else {
            /* Non-identifier character, copy as-is */
            if (out_len < capacity - 1) {
                result[out_len++] = *p;
            }
            p++;
        }
    }
    result[out_len] = '\0';
    
    /* If empty, default to "1" */
    if (out_len == 0) {
        free(result);
        return strdup("1");
    }
    
    return result;
}

static LpType type_from_annotation(CodeGen *cg, const char *ann) {
    if (!ann) return LP_UNKNOWN;
    
    /* Check for union types: int|str|float -> return LP_VAL (variant type) */
    if (is_union_type(ann)) {
        return LP_VAL;
    }
    
    /* Check for native arrays: int[expr] or int[expr][expr] */
    if (is_native_array_type(ann)) {
        /* Count dimensions to return appropriate type */
        char *sizes[4] = {NULL, NULL, NULL, NULL};
        int dims = parse_native_array_dims(ann, sizes);
        /* Free the size strings (caller will re-parse if needed) */
        for (int i = 0; i < dims; i++) {
            if (sizes[i]) free(sizes[i]);
        }
        return (dims == 1) ? LP_NATIVE_ARRAY_1D : LP_NATIVE_ARRAY_2D;
    }
    
    if (strcmp(ann, "int") == 0 || strcmp(ann, "i64") == 0) return LP_INT;
    if (strcmp(ann, "float") == 0 || strcmp(ann, "f64") == 0) return LP_FLOAT;
    if (strcmp(ann, "str") == 0) return LP_STRING;
    if (strcmp(ann, "bool") == 0) return LP_BOOL;
    if (strcmp(ann, "ptr") == 0) return LP_PTR;
    if (strcmp(ann, "pool") == 0) return LP_POOL;
    if (strcmp(ann, "arena") == 0) return LP_ARENA;
    if (strcmp(ann, "val") == 0) return LP_VAL;
    if (strcmp(ann, "sqlite_db") == 0) return LP_SQLITE_DB;
    if (strcmp(ann, "thread") == 0) return LP_THREAD;
    if (strcmp(ann, "lock") == 0) return LP_LOCK;
    if (strcmp(ann, "list") == 0) return LP_LIST;
    if (strcmp(ann, "dict") == 0) return LP_DICT;
    if (cg) {
        Symbol *s = scope_lookup(cg->scope, ann);
        if (s && s->type == LP_CLASS) return LP_OBJECT;
    }
    return LP_INT; /* default for unknown types */
}

static const char *lp_type_to_c(LpType t) {
    switch (t) {
        case LP_INT:    return "int64_t";
        case LP_FLOAT:  return "double";
        case LP_STRING: return "const char*";
        case LP_BOOL:   return "int";
        case LP_VOID:   return "void";
        case LP_PYOBJ:  return "PyObject*";
        case LP_LIST:   return "LpList*";
        case LP_ARRAY:  return "LpArray";
        case LP_STR_ARRAY: return "LpStrArray";
        case LP_DICT:   return "LpDict*";
        case LP_SET:    return "LpSet*";
        case LP_TUPLE:  return "LpTuple*";
        case LP_FILE:   return "LpFile*";
        case LP_VAL:    return "LpVal";
        case LP_SQLITE_DB: return "sqlite3*";
        case LP_THREAD: return "LpThread";
        case LP_LOCK:   return "LpLock";
        case LP_ARENA:  return "LpArena*";
        case LP_POOL:   return "LpPool*";
        case LP_PTR:    return "void*";
        /* Native arrays for competitive programming */
        case LP_NATIVE_ARRAY_1D: return "LpIntArray*";
        case LP_NATIVE_ARRAY_2D: return "LpIntArray2D*";
        default:        return "int64_t";
    }
}

static const char *lp_type_to_c_obj(LpType t, const char *class_name) {
    if (t == LP_OBJECT && class_name) {
        static char buf[256];
        snprintf(buf, sizeof(buf), "LpObj_%s*", class_name);
        return buf;
    }
    return lp_type_to_c(t);
}

/* --- Import lookup --- */
static ImportInfo *find_import(CodeGen *cg, const char *alias) {
    for (int i = 0; i < cg->import_count; i++) {
        if (strcmp(cg->imports[i].alias, alias) == 0)
            return &cg->imports[i];
    }
    return NULL;
}


/* --- Infer expression type --- */
static LpType infer_type(CodeGen *cg, AstNode *node);

static int is_range_call(AstNode *node) {
    return node && node->type == NODE_CALL &&
           node->call.func->type == NODE_NAME &&
           strcmp(node->call.func->name_expr.name, "range") == 0;
}

static int get_int_literal_value(AstNode *node, int64_t *out) {
    int64_t value = 0;

    if (!node) return 0;
    if (node->type == NODE_INT_LIT) {
        if (out) *out = node->int_lit.value;
        return 1;
    }
    if (node->type == NODE_UNARY_OP &&
        node->unary_op.op == TOK_MINUS &&
        get_int_literal_value(node->unary_op.operand, &value)) {
        if (out) *out = -value;
        return 1;
    }
    return 0;
}

static int is_single_int_repeat_list(CodeGen *cg, AstNode *node, AstNode **elem_out) {
    AstNode *elem;

    if (!node || node->type != NODE_LIST_EXPR || node->list_expr.elems.count != 1) {
        return 0;
    }

    elem = node->list_expr.elems.items[0];
    if (!elem || infer_type(cg, elem) != LP_INT) {
        return 0;
    }

    if (elem_out) *elem_out = elem;
    return 1;
}

static int match_native_int_repeat_expr(CodeGen *cg, AstNode *node, AstNode **elem_out, AstNode **count_out) {
    AstNode *elem = NULL;
    AstNode *count = NULL;

    if (!node || node->type != NODE_BIN_OP || node->bin_op.op != TOK_STAR) {
        return 0;
    }

    if (is_single_int_repeat_list(cg, node->bin_op.left, &elem) && infer_type(cg, node->bin_op.right) == LP_INT) {
        count = node->bin_op.right;
    } else if (is_single_int_repeat_list(cg, node->bin_op.right, &elem) && infer_type(cg, node->bin_op.left) == LP_INT) {
        count = node->bin_op.left;
    } else {
        return 0;
    }

    if (elem_out) *elem_out = elem;
    if (count_out) *count_out = count;
    return 1;
}

static LpType normalize_minmax_scalar_type(LpType t) {
    if (t == LP_BOOL) return LP_INT;
    return t;
}

static int is_native_minmax_scalar_type(LpType t) {
    t = normalize_minmax_scalar_type(t);
    return t == LP_INT || t == LP_FLOAT;
}

static LpType infer_minmax_call_type(CodeGen *cg, AstNode *node) {
    LpType result = LP_UNKNOWN;

    if (!node || node->type != NODE_CALL || node->call.args.count == 0) {
        return LP_UNKNOWN;
    }

    if (node->call.args.count == 1) {
        LpType arg_type = infer_type(cg, node->call.args.items[0]);
        if (arg_type == LP_NATIVE_ARRAY_1D) return LP_INT;
        if (arg_type == LP_ARRAY) return LP_FLOAT;
        return LP_UNKNOWN;
    }

    for (int i = 0; i < node->call.args.count; i++) {
        LpType arg_type = normalize_minmax_scalar_type(infer_type(cg, node->call.args.items[i]));

        if (!is_native_minmax_scalar_type(arg_type)) {
            return LP_UNKNOWN;
        }

        if (result == LP_UNKNOWN) {
            result = arg_type;
        } else if (result == LP_FLOAT || arg_type == LP_FLOAT) {
            result = LP_FLOAT;
        } else {
            result = LP_INT;
        }
    }

    return result;
}

static LpType infer_type(CodeGen *cg, AstNode *node) {
    if (!node) return LP_VOID;
    switch (node->type) {
        case NODE_INT_LIT:    return LP_INT;
        case NODE_FLOAT_LIT:  return LP_FLOAT;
        case NODE_STRING_LIT: return LP_STRING;
        case NODE_FSTRING:    return LP_STRING;  /* F-strings always produce strings */
        case NODE_BOOL_LIT:   return LP_BOOL;
        case NODE_NONE_LIT:   return LP_VOID;
        case NODE_LIST_EXPR:  return LP_LIST;
        case NODE_DICT_EXPR:  return LP_DICT;
        case NODE_SET_EXPR:   return LP_SET;
        case NODE_TUPLE_EXPR: return LP_TUPLE;
        case NODE_NAME: {
            Symbol *s = scope_lookup(cg->scope, node->name_expr.name);
            return s ? s->type : LP_UNKNOWN;
        }
        case NODE_BIN_OP: {
            LpType lt = infer_type(cg, node->bin_op.left);
            LpType rt = infer_type(cg, node->bin_op.right);
            TokenType op = node->bin_op.op;
            if (op == TOK_EQ || op == TOK_NEQ || op == TOK_LT || op == TOK_GT ||
                op == TOK_LTE || op == TOK_GTE || op == TOK_AND || op == TOK_OR ||
                op == TOK_IS)
                return LP_BOOL;
            /* String + anything = string concat */
            if (op == TOK_PLUS && (lt == LP_STRING || rt == LP_STRING))
                return LP_STRING;
            if (lt == LP_VAL || rt == LP_VAL || lt == LP_DICT || rt == LP_DICT || lt == LP_LIST || rt == LP_LIST) {
                if (op == TOK_EQ || op == TOK_NEQ || op == TOK_LT || op == TOK_GT || op == TOK_LTE || op == TOK_GTE)
                    return LP_BOOL;
                return LP_VAL;
            }
            if (op == TOK_BIT_AND || op == TOK_BIT_OR || op == TOK_BIT_XOR || op == TOK_LSHIFT || op == TOK_RSHIFT)
                return LP_INT;
            if (lt == LP_FLOAT || rt == LP_FLOAT) return LP_FLOAT;
            if (op == TOK_SLASH) return LP_FLOAT;
            return LP_INT;
        }
        case NODE_UNARY_OP:
            if (node->unary_op.op == TOK_NOT) return LP_BOOL;
            if (node->unary_op.op == TOK_BIT_NOT) return LP_INT;
            return infer_type(cg, node->unary_op.operand);
        case NODE_CALL: {
            /* === Nested module calls: mod.sub.func(...) e.g. os.path.exists(...) === */
            if (node->call.func->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->attribute.obj->type == NODE_NAME) {
                const char *mod_alias = node->call.func->attribute.obj->attribute.obj->name_expr.name;
                const char *sub_name = node->call.func->attribute.obj->attribute.attr;
                const char *func_name = node->call.func->attribute.attr;
                ImportInfo *imp = find_import(cg, mod_alias);
                if (imp) {
                    if (imp->tier == MOD_TIER1_OS && strcmp(sub_name, "path") == 0) {
                        if (strcmp(func_name, "exists") == 0 ||
                            strcmp(func_name, "isfile") == 0 ||
                            strcmp(func_name, "isdir") == 0)
                            return LP_BOOL;
                        if (strcmp(func_name, "getsize") == 0)
                            return LP_INT;
                        return LP_STRING; /* join, basename, dirname */
                    }
                    return LP_PYOBJ; /* Fallback for other nested calls */
                }
            }
            /* Module-qualified calls: e.g. math.sqrt(...) or np.array(...) */
            if (node->call.func->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->type == NODE_NAME) {
                const char *mod_alias = node->call.func->attribute.obj->name_expr.name;
                const char *func_name = node->call.func->attribute.attr;
                ImportInfo *imp = find_import(cg, mod_alias);
                if (imp) {
                    if (imp->tier == MOD_TIER2_NUMPY) {
                        /* numpy functions that return arrays */
                        if (strcmp(func_name, "array") == 0 ||
                            strcmp(func_name, "zeros") == 0 ||
                            strcmp(func_name, "ones") == 0 ||
                            strcmp(func_name, "arange") == 0 ||
                            strcmp(func_name, "linspace") == 0 ||
                            strcmp(func_name, "sort") == 0 ||
                            strcmp(func_name, "add") == 0 ||
                            strcmp(func_name, "subtract") == 0 ||
                            strcmp(func_name, "multiply") == 0 ||
                            strcmp(func_name, "sqrt") == 0 ||
                            strcmp(func_name, "abs") == 0 ||
                            strcmp(func_name, "sin") == 0 ||
                            strcmp(func_name, "cos") == 0 ||
                            strcmp(func_name, "exp") == 0 ||
                            strcmp(func_name, "log") == 0)
                            return LP_ARRAY;
                        /* numpy functions that return scalars */
                        return LP_FLOAT;
                    }
                    if (imp->tier == MOD_TIER1_MATH) {
                        if (strcmp(func_name, "factorial") == 0 ||
                            strcmp(func_name, "gcd") == 0 ||
                            strcmp(func_name, "lcm") == 0)
                            return LP_INT;
                        if (strcmp(func_name, "isnan") == 0 ||
                            strcmp(func_name, "isinf") == 0)
                            return LP_BOOL;
                        return LP_FLOAT;
                    }
                    if (imp->tier == MOD_TIER1_RANDOM) {
                        if (strcmp(func_name, "randint") == 0) return LP_INT;
                        return LP_FLOAT;
                    }
                    if (imp->tier == MOD_TIER1_TIME) return LP_FLOAT;
                    if (imp->tier == MOD_TIER1_OS) {
                        if (strcmp(func_name, "path_exists") == 0 ||
                            strcmp(func_name, "path_isfile") == 0 ||
                            strcmp(func_name, "path_isdir") == 0)
                            return LP_BOOL;
                        if (strcmp(func_name, "path_getsize") == 0)
                            return LP_INT;
                        return LP_STRING; /* getcwd, path_join, path_basename, etc */
                    }
                    if (imp->tier == MOD_TIER1_SYS) {
                        if (strcmp(func_name, "exit") == 0) return LP_VOID;
                        if (strcmp(func_name, "argv_len") == 0) return LP_INT;
                        if (strcmp(func_name, "getsizeof") == 0) return LP_INT;
                        if (strcmp(func_name, "getrecursionlimit") == 0) return LP_INT;
                        return LP_STRING;
                    }
                    if (imp->tier == MOD_TIER1_HTTP) {
                        if (strcmp(func_name, "get") == 0 || strcmp(func_name, "post") == 0) return LP_STRING;
                        return LP_UNKNOWN;
                    }
                    if (imp->tier == MOD_TIER1_JSON) {
                        if (strcmp(func_name, "loads") == 0 || strcmp(func_name, "parse") == 0) return LP_VAL;
                        if (strcmp(func_name, "dumps") == 0) return LP_STRING;
                        return LP_UNKNOWN;
                    }
                    if (imp->tier == MOD_TIER1_SQLITE) {
                        if (strcmp(func_name, "connect") == 0) return LP_SQLITE_DB;
                        if (strcmp(func_name, "execute") == 0) return LP_BOOL;
                        if (strcmp(func_name, "query") == 0) return LP_VAL;
                        return LP_VOID;
                    }
                    if (imp->tier == MOD_TIER1_THREAD) {
                        if (strcmp(func_name, "spawn") == 0) return LP_THREAD;
                        if (strcmp(func_name, "join") == 0) return LP_INT;
                        if (strcmp(func_name, "lock_init") == 0) return LP_LOCK;
                        return LP_VOID;
                    }
                    if (imp->tier == MOD_TIER1_MEMORY) {
                        if (strcmp(func_name, "arena_new") == 0) return LP_ARENA;
                        if (strcmp(func_name, "pool_new") == 0) return LP_POOL;
                        if (strcmp(func_name, "arena_alloc") == 0) return LP_PTR;
                        if (strcmp(func_name, "pool_alloc") == 0) return LP_PTR;
                        if (strcmp(func_name, "cast") == 0) return LP_OBJECT;
                        return LP_VOID;
                    }
                    if (imp->tier == MOD_TIER1_PLATFORM) {
                        if (strcmp(func_name, "cores") == 0) return LP_INT;
                        return LP_STRING; /* os(), arch() return const char* */
                    }
                    if (imp->tier == MOD_TIER1_SECURITY) {
                        /* Validation functions - return bool */
                        if (strcmp(func_name, "is_safe_string") == 0) return LP_BOOL;
                        if (strcmp(func_name, "validate_email") == 0) return LP_BOOL;
                        if (strcmp(func_name, "validate_numeric") == 0) return LP_BOOL;
                        if (strcmp(func_name, "validate_alphanumeric") == 0) return LP_BOOL;
                        if (strcmp(func_name, "validate_url") == 0) return LP_BOOL;
                        if (strcmp(func_name, "validate_identifier") == 0) return LP_BOOL;
                        if (strcmp(func_name, "detect_sql_injection") == 0) return LP_BOOL;
                        if (strcmp(func_name, "detect_xss") == 0) return LP_BOOL;
                        /* Escape/sanitize functions - return string */
                        if (strcmp(func_name, "sql_escape") == 0) return LP_STRING;
                        if (strcmp(func_name, "html_escape") == 0) return LP_STRING;
                        if (strcmp(func_name, "strip_html") == 0) return LP_STRING;
                        if (strcmp(func_name, "sanitize") == 0) return LP_STRING;
                        /* Hash functions - return string */
                        if (strcmp(func_name, "hash_md5") == 0) return LP_STRING;
                        if (strcmp(func_name, "hash_sha256") == 0) return LP_STRING;
                        /* Rate limit - return bool */
                        if (strcmp(func_name, "check_rate_limit") == 0) return LP_BOOL;
                        return LP_STRING;
                    }
                    if (imp->tier == MOD_TIER1_DSA) {
                        /* Fast I/O - read functions */
                        if (strcmp(func_name, "read_int") == 0) return LP_INT;
                        if (strcmp(func_name, "read_float") == 0) return LP_FLOAT;
                        if (strcmp(func_name, "read_str") == 0) return LP_STRING;
                        if (strcmp(func_name, "read_line") == 0) return LP_STRING;
                        /* Fast I/O - write functions */
                        if (strncmp(func_name, "write", 5) == 0) return LP_VOID;
                        if (strcmp(func_name, "flush") == 0) return LP_VOID;
                        /* Number Theory */
                        if (strcmp(func_name, "sieve") == 0) return LP_PTR; /* bool* */
                        if (strcmp(func_name, "is_prime") == 0) return LP_BOOL;
                        if (strcmp(func_name, "mod_pow") == 0) return LP_INT;
                        if (strcmp(func_name, "mod_inverse") == 0) return LP_INT;
                        if (strcmp(func_name, "extended_gcd") == 0) return LP_PTR; /* struct */
                        if (strcmp(func_name, "prime_factors") == 0) return LP_PTR; /* struct */
                        if (strcmp(func_name, "euler_phi") == 0) return LP_INT;
                        if (strcmp(func_name, "count_divisors") == 0) return LP_INT;
                        if (strcmp(func_name, "sum_divisors") == 0) return LP_INT;
                        /* DSU */
                        if (strcmp(func_name, "dsu_new") == 0) return LP_PTR;
                        if (strcmp(func_name, "dsu_find") == 0) return LP_INT;
                        if (strcmp(func_name, "dsu_union") == 0) return LP_BOOL;
                        if (strcmp(func_name, "dsu_same") == 0) return LP_BOOL;
                        if (strcmp(func_name, "dsu_size") == 0) return LP_INT;
                        /* Heap */
                        if (strcmp(func_name, "heap_new") == 0) return LP_PTR;
                        if (strcmp(func_name, "heap_push") == 0) return LP_VOID;
                        if (strcmp(func_name, "heap_pop") == 0) return LP_INT;
                        if (strcmp(func_name, "heap_top") == 0) return LP_INT;
                        if (strcmp(func_name, "heap_is_empty") == 0) return LP_BOOL;
                        /* Fenwick Tree */
                        if (strcmp(func_name, "fenwick_new") == 0) return LP_PTR;
                        if (strcmp(func_name, "fenwick_add") == 0) return LP_VOID;
                        if (strcmp(func_name, "fenwick_prefix_sum") == 0) return LP_INT;
                        if (strcmp(func_name, "fenwick_range_sum") == 0) return LP_INT;
                        return LP_INT;
                    }
                    return LP_PYOBJ; /* Tier 3 */
                }
            }
            if (node->call.func->type == NODE_ATTRIBUTE) {
                LpType obj_type = infer_type(cg, node->call.func->attribute.obj);
                if (obj_type == LP_FILE) {
                    const char *attr = node->call.func->attribute.attr;
                    if (strcmp(attr, "read") == 0) return LP_STRING;
                    if (strcmp(attr, "write") == 0 || strcmp(attr, "close") == 0) return LP_VOID;
                }
                if (obj_type == LP_STRING) {
                    const char *attr = node->call.func->attribute.attr;
                    cg->uses_strings = 1;
                    if (strcmp(attr, "split") == 0) return LP_STR_ARRAY;
                    if (strcmp(attr, "startswith") == 0 || strcmp(attr, "endswith") == 0 ||
                        strcmp(attr, "isdigit") == 0 || strcmp(attr, "isalpha") == 0 ||
                        strcmp(attr, "isalnum") == 0) return LP_BOOL;
                    if (strcmp(attr, "find") == 0 || strcmp(attr, "count") == 0) return LP_INT;
                    if (strcmp(attr, "find") == 0 || strcmp(attr, "count") == 0) return LP_INT;
                    return LP_STRING; /* upper, lower, strip, replace, join */
                }
                if (obj_type == LP_LIST) {
                    const char *attr = node->call.func->attribute.attr;
                    if (strcmp(attr, "append") == 0) return LP_VOID;
                    if (strcmp(attr, "sort") == 0) return LP_VOID;
                    if (strcmp(attr, "binary_search") == 0) return LP_INT;
                }
            }
            if (node->call.func->type == NODE_NAME) {
                const char *func = node->call.func->name_expr.name;
                if (strcmp(func, "open") == 0) return LP_FILE;
                if (strcmp(func, "input") == 0) return LP_STRING;
                if (strcmp(func, "len") == 0) return LP_INT;
                if (strcmp(func, "min") == 0 || strcmp(func, "max") == 0)
                    return infer_minmax_call_type(cg, node);
                if (strcmp(func, "int") == 0) return LP_INT;
                if (strcmp(func, "float") == 0) return LP_FLOAT;
                if (strcmp(func, "str") == 0) return LP_STRING;
                if (strcmp(func, "bool") == 0) return LP_BOOL;
                Symbol *s = scope_lookup(cg->scope, func);
                if (s && s->type == LP_CLASS) return LP_OBJECT;
                if (s) return s->type;
            }
            return LP_UNKNOWN;
        }
        /* Module attribute access (e.g. math.pi) */
        case NODE_ATTRIBUTE: {
            if (node->attribute.obj->type == NODE_NAME) {
                ImportInfo *imp = find_import(cg, node->attribute.obj->name_expr.name);
                if (imp) {
                    if (imp->tier == MOD_TIER1_MATH) return LP_FLOAT;
                    if (imp->tier == MOD_TIER1_OS) return LP_STRING;
                    if (imp->tier == MOD_TIER1_SYS) {
                        if (strcmp(node->attribute.attr, "platform") == 0) return LP_STRING;
                        if (strcmp(node->attribute.attr, "maxsize") == 0) return LP_INT;
                        return LP_STRING;
                    }
                    if (imp->tier == MOD_TIER3_PYTHON) return LP_PYOBJ;
                }
            }
            LpType obj_type = infer_type(cg, node->attribute.obj);
            if (obj_type == LP_VAL || obj_type == LP_DICT || obj_type == LP_PYOBJ) {
                return LP_VAL;
            }
            return LP_UNKNOWN;
        }
        case NODE_SUBSCRIPT: {
            LpType obj_type = infer_type(cg, node->subscript.obj);
            if (obj_type == LP_DICT || obj_type == LP_PYOBJ || obj_type == LP_VAL || obj_type == LP_LIST)
                return LP_VAL;
            if (obj_type == LP_ARRAY)
                return LP_FLOAT;
            if (obj_type == LP_STR_ARRAY || obj_type == LP_STRING)
                return LP_STRING;
            /* Native arrays contain int64_t */
            if (obj_type == LP_NATIVE_ARRAY_1D || obj_type == LP_NATIVE_ARRAY_2D)
                return LP_INT;
            return LP_UNKNOWN;
        }
        case NODE_KWARG:
            return infer_type(cg, node->kwarg.value);
        case NODE_LIST_COMP:
            return LP_LIST;
        case NODE_DICT_COMP:
            return LP_DICT;
        case NODE_YIELD:
            return LP_INT; /* yield returns the yielded value as int for now */
        case NODE_LAMBDA:
            return LP_INT; /* function pointer stored as int */
        case NODE_AWAIT_EXPR:
            /* await expr returns the type of the awaited expression */
            if (node->await_expr.expr) {
                return infer_type(cg, node->await_expr.expr);
            }
            return LP_UNKNOWN;
        case NODE_GENERIC_INST:
            /* Generic type instantiation - look up the base type */
            if (node->generic_inst.base_name) {
                Symbol *s = scope_lookup(cg->scope, node->generic_inst.base_name);
                if (s) return s->type;
            }
            return LP_VAL;  /* Default to variant type for generics */
        case NODE_TYPE_UNION:
            /* Type union always returns LP_VAL (variant type) */
            return LP_VAL;
        default: return LP_UNKNOWN;
    }
}

/* --- Code generation --- */
static void emit_cast(CodeGen *cg, Buffer *buf, AstNode *expr, LpType target_type);
static void gen_expr(CodeGen *cg, Buffer *buf, AstNode *node);
static void gen_stmt(CodeGen *cg, Buffer *buf, AstNode *node, int indent);
static LpType infer_return_type(CodeGen *cg, NodeList *body);
static void gen_thread_spawn_expr(CodeGen *cg, Buffer *buf, AstNode *node);
static void populate_function_symbol(CodeGen *cg, Symbol *sym, AstNode *node, const char *owner_class);
static int is_range_call(AstNode *node);
static void emit_native_int_array_access(CodeGen *cg, Buffer *buf, AstNode *obj_expr, AstNode *index_expr);
static void emit_native_numeric_minmax(CodeGen *cg, Buffer *buf, AstNode *node, int is_min, LpType result_type);

static void write_indent(Buffer *buf, int level) {
    for (int i = 0; i < level; i++) buf_write(buf, "    ");
}

static void gen_thread_spawn_expr(CodeGen *cg, Buffer *buf, AstNode *node) {
    AstNode *worker_node;
    Symbol *worker_sym;
    const char *worker_name;
    const char *param_c_type;
    char helper_name[64];
    char ctx_name[64];
    int adapter_id;

    if (node->call.args.count < 1 || node->call.args.count > 4) {
        codegen_set_error(cg, "thread.spawn expects a worker function and up to 3 arguments");
        buf_write(buf, "NULL");
        return;
    }

    worker_node = node->call.args.items[0];
    if (!worker_node || worker_node->type != NODE_NAME) {
        codegen_set_error(cg, "thread.spawn only supports named LP functions as workers (lambdas not yet supported for thread safety)");
        buf_write(buf, "NULL");
        return;
    }

    worker_name = worker_node->name_expr.name;
    worker_sym = scope_lookup(cg->scope, worker_name);
    if (!worker_sym || !worker_sym->is_function) {
        codegen_set_error(cg, "thread.spawn worker '%s' must be a named LP function", worker_name);
        buf_write(buf, "NULL");
        return;
    }

    if (worker_sym->is_variadic || worker_sym->has_kwargs) {
        codegen_set_error(cg, "thread.spawn worker '%s' cannot use *args or **kwargs", worker_name);
        buf_write(buf, "NULL");
        return;
    }

    /* Relaxed: allow up to 3 parameters now */
    if (worker_sym->num_params > 3) {
        codegen_set_error(cg, "thread.spawn worker '%s' must accept 0-3 arguments", worker_name);
        buf_write(buf, "NULL");
        return;
    }

    /* Relaxed: allow int, void, float, and string return types */
    if (worker_sym->type != LP_INT && worker_sym->type != LP_VOID && 
        worker_sym->type != LP_FLOAT && worker_sym->type != LP_STRING) {
        codegen_set_error(cg, "thread.spawn worker '%s' must return int, void, float, or string", worker_name);
        buf_write(buf, "NULL");
        return;
    }

    /* Check argument count matches parameter count */
    if (worker_sym->num_params == 0 && node->call.args.count != 1) {
        codegen_set_error(cg, "thread.spawn worker '%s' does not accept arguments", worker_name);
        buf_write(buf, "NULL");
        return;
    }

    if (worker_sym->num_params > 0 && node->call.args.count != worker_sym->num_params + 1) {
        codegen_set_error(cg, "thread.spawn worker '%s' expects %d argument(s)", worker_name, worker_sym->num_params);
        buf_write(buf, "NULL");
        return;
    }

    if (worker_sym->num_params >= 1) {
        if (worker_sym->first_param_type == LP_UNKNOWN) {
            codegen_set_error(cg, "thread.spawn worker '%s' has an unsupported argument type", worker_name);
            buf_write(buf, "NULL");
            return;
        }
        if (worker_sym->first_param_type == LP_OBJECT && !worker_sym->first_param_class_name) {
            codegen_set_error(cg, "thread.spawn worker '%s' has an object argument with no class metadata", worker_name);
            buf_write(buf, "NULL");
            return;
        }
    }

    adapter_id = cg->thread_adapter_count++;
    snprintf(helper_name, sizeof(helper_name), "lp_thread_adapter_%d", adapter_id);
    snprintf(ctx_name, sizeof(ctx_name), "LpThreadCtx_%d", adapter_id);

    if (worker_sym->num_params == 0) {
        buf_printf(&cg->helpers, "static int64_t %s(void *lp_raw_arg) {\n", helper_name);
        buf_write(&cg->helpers, "    (void)lp_raw_arg;\n");
        if (worker_sym->type == LP_VOID) {
            buf_printf(&cg->helpers, "    lp_%s();\n", worker_name);
            buf_write(&cg->helpers, "    return 0;\n");
        } else {
            buf_printf(&cg->helpers, "    return lp_%s();\n", worker_name);
        }
        buf_write(&cg->helpers, "}\n\n");

        buf_printf(buf, "lp_thread_spawn(%s, NULL)", helper_name);
        return;
    }

    param_c_type = lp_type_to_c_obj(worker_sym->first_param_type, worker_sym->first_param_class_name);

    buf_printf(&cg->helpers, "typedef struct {\n    %s arg;\n} %s;\n\n", param_c_type, ctx_name);
    buf_printf(&cg->helpers, "static int64_t %s(void *lp_raw_arg) {\n", helper_name);
    buf_printf(&cg->helpers, "    %s *lp_ctx = (%s*)lp_raw_arg;\n", ctx_name, ctx_name);
    buf_printf(&cg->helpers, "    %s lp_arg = lp_ctx->arg;\n", param_c_type);
    buf_write(&cg->helpers, "    free(lp_ctx);\n");
    if (worker_sym->type == LP_VOID) {
        buf_printf(&cg->helpers, "    lp_%s(lp_arg);\n", worker_name);
        buf_write(&cg->helpers, "    return 0;\n");
    } else {
        buf_printf(&cg->helpers, "    return lp_%s(lp_arg);\n", worker_name);
    }
    buf_write(&cg->helpers, "}\n\n");

    buf_write(buf, "({ ");
    buf_write(buf, "LpThread _lp_thread = NULL; ");
    buf_printf(buf, "%s *_lp_ctx = (%s*)malloc(sizeof(%s)); ", ctx_name, ctx_name, ctx_name);
    buf_write(buf, "if (_lp_ctx) { _lp_ctx->arg = ");
    gen_expr(cg, buf, node->call.args.items[1]);
    buf_printf(buf, "; _lp_thread = lp_thread_spawn(%s, _lp_ctx); if (!_lp_thread) free(_lp_ctx); } _lp_thread; })", helper_name);
}
static void emit_lp_val(CodeGen *cg, Buffer *buf, AstNode *expr) {
    LpType t = infer_type(cg, expr);
    switch (t) {
        case LP_INT:
            if (expr->type == NODE_CALL && expr->call.func->type == NODE_NAME && strcmp(expr->call.func->name_expr.name, "int") == 0) {
                buf_write(buf, "lp_val_int("); gen_expr(cg, buf, expr); buf_write(buf, ")");
            } else {
                buf_write(buf, "lp_val_int("); gen_expr(cg, buf, expr); buf_write(buf, ")");
            }
            break;
        case LP_FLOAT: buf_write(buf, "lp_val_float("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_STRING: buf_write(buf, "lp_val_str("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_BOOL: buf_write(buf, "lp_val_bool("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_LIST: buf_write(buf, "lp_val_list("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_DICT: buf_write(buf, "lp_val_dict("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_STR_ARRAY:
            buf_write(buf, "({ LpList *_l = lp_list_new(); for(int i=0;i<(");
            gen_expr(cg, buf, expr);
            buf_write(buf, ").count;i++) lp_list_append(_l, lp_val_str((");
            gen_expr(cg, buf, expr);
            buf_write(buf, ").items[i])); lp_val_list(_l); })");
            break;
        case LP_VAL: gen_expr(cg, buf, expr); break;
        case LP_UNKNOWN: gen_expr(cg, buf, expr); break;
        default: buf_write(buf, "lp_val_null()"); break;
    }
}

static void gen_expr(CodeGen *cg, Buffer *buf, AstNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_INT_LIT:
            buf_printf(buf, "%" PRId64, node->int_lit.value);
            break;
        case NODE_FLOAT_LIT:
            buf_printf(buf, "%g", node->float_lit.value);
            break;
        case NODE_STRING_LIT:
            buf_write_c_string_literal(buf, node->str_lit.value);
            break;
        case NODE_BOOL_LIT:
            buf_write(buf, node->bool_lit.value ? "1" : "0");
            break;
        case NODE_NONE_LIT:
            buf_write(buf, "0 /* None */");
            break;
        case NODE_NAME:
            buf_printf(buf, "lp_%s", node->name_expr.name);
            break;
        case NODE_FSTRING: {
            /* Generate f-string: concatenate all parts into a string */
            /* We'll use snprintf with a buffer for each part */
            NodeList *parts = &node->fstring_expr.parts;
            if (parts->count == 0) {
                buf_write(buf, "\"\"");
                break;
            }
            
            /* Calculate total buffer size needed */
            buf_write(buf, "({ ");
            buf_write(buf, "char _fs_buf[4096]; ");
            buf_write(buf, "int _fs_len = 0; ");
            
            for (int i = 0; i < parts->count; i++) {
                AstNode *part = parts->items[i];
                if (part->type == NODE_STRING_LIT) {
                    /* String literal - append directly */
                    buf_printf(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%s\"); ", 
                              part->str_lit.value);
                } else {
                    /* Expression - convert to string and append */
                    LpType part_type = infer_type(cg, part);
                    if (part_type == LP_INT) {
                        buf_write(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%lld\", (long long)");
                        gen_expr(cg, buf, part);
                        buf_write(buf, "); ");
                    } else if (part_type == LP_FLOAT) {
                        buf_write(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%g\", ");
                        gen_expr(cg, buf, part);
                        buf_write(buf, "); ");
                    } else if (part_type == LP_STRING) {
                        buf_write(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%s\", ");
                        gen_expr(cg, buf, part);
                        buf_write(buf, "); ");
                    } else if (part_type == LP_BOOL) {
                        buf_write(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%s\", ");
                        gen_expr(cg, buf, part);
                        buf_write(buf, " ? \"True\" : \"False\"); ");
                    } else {
                        /* Default: convert to string representation */
                        buf_write(buf, "_fs_len += snprintf(_fs_buf + _fs_len, sizeof(_fs_buf) - _fs_len, \"%lld\", (long long)");
                        gen_expr(cg, buf, part);
                        buf_write(buf, "); ");
                    }
                }
            }
            
            buf_write(buf, "lp_strdup(_fs_buf); })");
            break;
        }
        case NODE_LIST_EXPR: {
            NodeList *elems = &node->list_expr.elems;
            if (elems->count == 0) {
                buf_write(buf, "lp_list_new()");
                break;
            }
            buf_write(buf, "({ LpList *_l = lp_list_new(); ");
            for (int i = 0; i < elems->count; i++) {
                buf_write(buf, "lp_list_append(_l, ");
                emit_lp_val(cg, buf, elems->items[i]);
                buf_write(buf, "); ");
            }
            buf_write(buf, "_l; })");
            break;
        }
        case NODE_DICT_EXPR: {
            NodeList *keys = &node->dict_expr.keys;
            if (keys->count == 0) {
                buf_write(buf, "lp_dict_new()");
                break;
            }
            
            buf_write(buf, "({ LpDict *_d = lp_dict_new(); ");
            for (int i = 0; i < keys->count; i++) {
                buf_write(buf, "lp_dict_set(_d, ");
                gen_expr(cg, buf, keys->items[i]);
                buf_write(buf, ", ");
                emit_lp_val(cg, buf, node->dict_expr.values.items[i]);
                buf_write(buf, "); ");
            }
            buf_write(buf, "_d; })");
            break;
        }
        case NODE_SET_EXPR: {
            if (node->set_expr.elems.count == 0) {
                buf_write(buf, "lp_set_new()");
                break;
            }
            buf_write(buf, "({ LpSet *_s = lp_set_new(); ");
            for (int i = 0; i < node->set_expr.elems.count; i++) {
                buf_write(buf, "lp_set_add(_s, ");
                gen_expr(cg, buf, node->set_expr.elems.items[i]);
                buf_write(buf, "); ");
            }
            buf_write(buf, "_s; })");
            break;
        }
        case NODE_TUPLE_EXPR: {
            buf_printf(buf, "({ LpTuple *_t = lp_tuple_new(%d); ", node->tuple_expr.elems.count);
            for (int i = 0; i < node->tuple_expr.elems.count; i++) {
                buf_printf(buf, "lp_tuple_set(_t, %d, (void*)(intptr_t)", i);
                gen_expr(cg, buf, node->tuple_expr.elems.items[i]);
                buf_write(buf, "); ");
            }
            buf_write(buf, "_t; })");
            break;
        }
        case NODE_BIN_OP: {
            TokenType op = node->bin_op.op;
            /* Handle 'is' operator for type checking */
            if (op == TOK_IS) {
                LpType lt = infer_type(cg, node->bin_op.left);
                /* For LpVal types, use lp_val_isinstance */
                if (lt == LP_VAL || lt == LP_DICT || lt == LP_LIST) {
                    buf_write(buf, "lp_val_isinstance(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    /* Get the type name from the right side */
                    if (node->bin_op.right && node->bin_op.right->type == NODE_NAME) {
                        buf_write(buf, "\"");
                        buf_write(buf, node->bin_op.right->name_expr.name);
                        buf_write(buf, "\"");
                    } else {
                        buf_write(buf, "\"unknown\"");
                    }
                    buf_write(buf, ")");
                } else {
                    /* For native types, compare type sizes or use type inference */
                    /* 'x is int' for native int64_t always returns true if x is int */
                    const char *type_name = "";
                    if (node->bin_op.right && node->bin_op.right->type == NODE_NAME) {
                        type_name = node->bin_op.right->name_expr.name;
                    }
                    if (strcmp(type_name, "int") == 0) {
                        buf_write(buf, "(1)"); /* Native int is always int */
                    } else if (strcmp(type_name, "float") == 0) {
                        buf_write(buf, "(1)"); /* Native float is always float */
                    } else if (strcmp(type_name, "str") == 0) {
                        buf_write(buf, "(1)"); /* Native str is always str */
                    } else {
                        buf_write(buf, "(0)"); /* Unknown type check */
                    }
                }
                break;
            }
            if (op == TOK_DSTAR) {
                LpType lt = infer_type(cg, node->bin_op.left);
                if (lt == LP_FLOAT) {
                    buf_write(buf, "pow(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "lp_pow_int(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                }
            } else if (op == TOK_DSLASH) {
                LpType lt = infer_type(cg, node->bin_op.left);
                LpType rt = infer_type(cg, node->bin_op.right);
                if (lt == LP_VAL || rt == LP_VAL || lt == LP_DICT || rt == LP_DICT || lt == LP_LIST || rt == LP_LIST) {
                    buf_write(buf, "lp_val_floordiv(");
                    emit_lp_val(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "lp_floordiv(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                }
            } else if (op == TOK_PERCENT) {
                LpType lt = infer_type(cg, node->bin_op.left);
                LpType rt = infer_type(cg, node->bin_op.right);
                if (lt == LP_VAL || rt == LP_VAL || lt == LP_DICT || rt == LP_DICT || lt == LP_LIST || rt == LP_LIST) {
                    buf_write(buf, "lp_val_mod(");
                    emit_lp_val(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "lp_mod(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                }
            } else {
                LpType lt = infer_type(cg, node->bin_op.left);
                LpType rt = infer_type(cg, node->bin_op.right);

                /* Special handling for list * int: create a list with repeated elements */
                if (op == TOK_STAR && lt == LP_LIST && (rt == LP_INT || rt == LP_VAL)) {
                    buf_write(buf, "lp_list_repeat(");
                    emit_lp_val(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                    break;
                }
                /* Also handle int * list: 3 * [0] -> [0, 0, 0] */
                if (op == TOK_STAR && rt == LP_LIST && (lt == LP_INT || lt == LP_VAL)) {
                    buf_write(buf, "lp_list_repeat(");
                    emit_lp_val(cg, buf, node->bin_op.right);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ")");
                    break;
                }

                if (lt == LP_VAL || rt == LP_VAL || lt == LP_DICT || rt == LP_DICT || lt == LP_LIST || rt == LP_LIST) {
                    const char *func = NULL;
                    switch (op) {
                        case TOK_PLUS: func = "lp_val_add"; break;
                        case TOK_MINUS: func = "lp_val_sub"; break;
                        case TOK_STAR: func = "lp_val_mul"; break;
                        case TOK_SLASH: func = "lp_val_div"; break;
                        case TOK_EQ: func = "lp_val_eq"; break;
                        case TOK_NEQ: func = "lp_val_neq"; break;
                        case TOK_LT: func = "lp_val_lt"; break;
                        case TOK_GT: func = "lp_val_gt"; break;
                        case TOK_LTE: func = "lp_val_lte"; break;
                        case TOK_GTE: func = "lp_val_gte"; break;
                        default: break;
                    }
                    if (func) {
                        buf_printf(buf, "%s(", func);
                        emit_lp_val(cg, buf, node->bin_op.left);
                        buf_write(buf, ", ");
                        emit_lp_val(cg, buf, node->bin_op.right);
                        buf_write(buf, ")");
                        break;
                    }
                }

                /* Check for string concatenation: str + str => lp_str_concat */
                if (op == TOK_PLUS && (lt == LP_STRING || rt == LP_STRING)) {
                    buf_write(buf, "lp_str_concat(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                    break;
                }
                if ((op == TOK_EQ || op == TOK_NEQ) && (lt == LP_STRING || rt == LP_STRING)) {
                    buf_write(buf, "(strcmp(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, op == TOK_EQ ? ") == 0)" : ") != 0)");
                    break;
                }
                const char *ops;
                switch (op) {
                    case TOK_PLUS: ops = " + "; break;
                    case TOK_MINUS: ops = " - "; break;
                    case TOK_STAR: ops = " * "; break;
                    case TOK_SLASH: ops = " / "; break;
                    case TOK_EQ: ops = " == "; break;
                    case TOK_NEQ: ops = " != "; break;
                    case TOK_LT: ops = " < "; break;
                    case TOK_GT: ops = " > "; break;
                    case TOK_LTE: ops = " <= "; break;
                    case TOK_GTE: ops = " >= "; break;
                    case TOK_AND: ops = " && "; break;
                    case TOK_OR: ops = " || "; break;
                    case TOK_BIT_AND: ops = " & "; break;
                    case TOK_BIT_OR: ops = " | "; break;
                    case TOK_BIT_XOR: ops = " ^ "; break;
                    case TOK_LSHIFT: ops = " << "; break;
                    case TOK_RSHIFT: ops = " >> "; break;
                    default: ops = " ? "; break;
                }
                buf_write(buf, "(");
                if (op == TOK_BIT_AND || op == TOK_BIT_OR || op == TOK_BIT_XOR || op == TOK_LSHIFT || op == TOK_RSHIFT) {
                    buf_write(buf, "(int64_t)(");
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ")");
                    buf_write(buf, ops);
                    buf_write(buf, "(int64_t)(");
                    gen_expr(cg, buf, node->bin_op.right);
                    buf_write(buf, ")");
                } else {
                    gen_expr(cg, buf, node->bin_op.left);
                    buf_write(buf, ops);
                    gen_expr(cg, buf, node->bin_op.right);
                }
                buf_write(buf, ")");
            }
            break;
        }
        case NODE_UNARY_OP:
            if (node->unary_op.op == TOK_NOT) {
                buf_write(buf, "!(");
                gen_expr(cg, buf, node->unary_op.operand);
                buf_write(buf, ")");
            } else if (node->unary_op.op == TOK_MINUS) {
                buf_write(buf, "(-(");
                gen_expr(cg, buf, node->unary_op.operand);
                buf_write(buf, "))");
            } else if (node->unary_op.op == TOK_BIT_NOT) {
                buf_write(buf, "(~(int64_t)(");
                gen_expr(cg, buf, node->unary_op.operand);
                buf_write(buf, "))");
            } else {
                gen_expr(cg, buf, node->unary_op.operand);
            }
            break;
        case NODE_CALL: {
            /* === Nested module calls: mod.sub.func(...) e.g. os.path.exists(...) === */
            if (node->call.func->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->attribute.obj->type == NODE_NAME) {
                const char *mod_alias = node->call.func->attribute.obj->attribute.obj->name_expr.name;
                const char *sub_name = node->call.func->attribute.obj->attribute.attr;
                const char *func_name = node->call.func->attribute.attr;
                ImportInfo *imp = find_import(cg, mod_alias);
                if (imp) {
                    /* Emit lp_<module>_<sub>_<func>(...) */
                    buf_printf(buf, "lp_%s_%s_%s(", imp->module, sub_name, func_name);
                    for (int i = 0; i < node->call.args.count; i++) {
                        if (i > 0) buf_write(buf, ", ");
                        gen_expr(cg, buf, node->call.args.items[i]);
                    }
                    buf_write(buf, ")");
                    break;
                }
            }
            /* === Module-qualified calls: mod.func(...) === */
            if (node->call.func->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->type == NODE_NAME) {
                const char *mod_alias = node->call.func->attribute.obj->name_expr.name;
                const char *func_name = node->call.func->attribute.attr;
                ImportInfo *imp = find_import(cg, mod_alias);
                if (imp) {
                    /* ---- TIER 1: math ??? direct C <math.h> ---- */
                    if (imp->tier == MOD_TIER1_MATH) {
                        buf_printf(buf, "lp_math_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: random ---- */
                    if (imp->tier == MOD_TIER1_RANDOM) {
                        buf_printf(buf, "lp_random_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: time ---- */
                    if (imp->tier == MOD_TIER1_TIME) {
                        buf_printf(buf, "lp_time_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: os ---- */
                    if (imp->tier == MOD_TIER1_OS) {
                        /* os.path.join, os.path.exists etc are handled as os.path_join etc */
                        buf_printf(buf, "lp_os_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: sys ---- */
                    if (imp->tier == MOD_TIER1_SYS) {
                        buf_printf(buf, "lp_sys_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: http ---- */
                    if (imp->tier == MOD_TIER1_HTTP) {
                        /* Supported HTTP methods: get, post, put, delete, patch */
                        if (strcmp(func_name, "get") != 0 && strcmp(func_name, "post") != 0 &&
                            strcmp(func_name, "put") != 0 && strcmp(func_name, "delete") != 0 &&
                            strcmp(func_name, "patch") != 0) {
                            codegen_set_error(cg, "http.%s is not supported; available methods: get, post, put, delete, patch", func_name);
                            buf_write(buf, "\"\"");
                            break;
                        }
                        buf_printf(buf, "lp_http_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: json ---- */
                    if (imp->tier == MOD_TIER1_JSON) {
                        if (strcmp(func_name, "loads") != 0 && strcmp(func_name, "dumps") != 0 && strcmp(func_name, "parse") != 0) {
                            codegen_set_error(cg, "json.%s is not supported yet; only json.loads/parse and json.dumps are available", func_name);
                            buf_write(buf, strcmp(func_name, "dumps") == 0 ? "\"\"" : "lp_val_null()");
                            break;
                        }
                        if (strcmp(func_name, "parse") == 0) {
                            buf_write(buf, "lp_json_loads(");
                        } else {
                            buf_printf(buf, "lp_json_%s(", func_name);
                        }
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            /* json.dumps expects LpVal; wrap dict/list/other non-LpVal args */
                            if (strcmp(func_name, "dumps") == 0) {
                                LpType arg_type = infer_type(cg, node->call.args.items[i]);
                                if (arg_type == LP_DICT) {
                                    buf_write(buf, "lp_val_dict(");
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                    buf_write(buf, ")");
                                } else if (arg_type == LP_VAL) {
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                } else {
                                    emit_lp_val(cg, buf, node->call.args.items[i]);
                                }
                            } else {
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: sqlite ---- */
                    if (imp->tier == MOD_TIER1_SQLITE) {
                        buf_printf(buf, "lp_sqlite_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        if (strcmp(func_name, "execute") == 0 || strcmp(func_name, "query") == 0) {
                            if (node->call.args.count == 2) {
                                buf_write(buf, ", NULL");
                            }
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: thread ---- */
                    if (imp->tier == MOD_TIER1_THREAD) {
                        if (strcmp(func_name, "spawn") == 0) {
                            gen_thread_spawn_expr(cg, buf, node);
                            break;
                        }
                        buf_printf(buf, "lp_thread_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: memory ---- */
                    if (imp->tier == MOD_TIER1_MEMORY) {
                        /* memory.cast(ptr, Type) -> special handler */
                        if (strcmp(func_name, "cast") == 0 && node->call.args.count == 2) {
                            AstNode *type_arg = node->call.args.items[1];
                            if (type_arg->type == NODE_NAME) {
                                buf_printf(buf, "(LpObj_%s*)", type_arg->name_expr.name);
                                gen_expr(cg, buf, node->call.args.items[0]);
                                break;
                            }
                        }
                        buf_printf(buf, "lp_memory_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: platform ---- */
                    if (imp->tier == MOD_TIER1_PLATFORM) {
                        buf_printf(buf, "lp_platform_%s(", func_name);
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: security ---- */
                    if (imp->tier == MOD_TIER1_SECURITY) {
                        /* Validation functions */
                        if (strcmp(func_name, "is_safe_string") == 0) {
                            buf_write(buf, "lp_is_safe_string(");
                        } else if (strcmp(func_name, "validate_email") == 0) {
                            buf_write(buf, "lp_validate_email(");
                        } else if (strcmp(func_name, "validate_numeric") == 0) {
                            buf_write(buf, "lp_validate_numeric(");
                        } else if (strcmp(func_name, "validate_alphanumeric") == 0) {
                            buf_write(buf, "lp_validate_alphanumeric(");
                        } else if (strcmp(func_name, "validate_url") == 0) {
                            buf_write(buf, "lp_validate_url(");
                        } else if (strcmp(func_name, "validate_identifier") == 0) {
                            buf_write(buf, "lp_validate_identifier(");
                        }
                        /* Detection functions */
                        else if (strcmp(func_name, "detect_sql_injection") == 0) {
                            buf_write(buf, "lp_detect_sql_injection(");
                        } else if (strcmp(func_name, "detect_xss") == 0) {
                            buf_write(buf, "lp_detect_xss(");
                        }
                        /* Escape functions */
                        else if (strcmp(func_name, "sql_escape") == 0) {
                            buf_write(buf, "lp_sql_escape(");
                        } else if (strcmp(func_name, "html_escape") == 0) {
                            buf_write(buf, "lp_html_escape(");
                        } else if (strcmp(func_name, "strip_html") == 0) {
                            buf_write(buf, "lp_strip_html(");
                        }
                        /* Hash functions */
                        else if (strcmp(func_name, "hash_md5") == 0) {
                            buf_write(buf, "lp_hash_md5(");
                        } else if (strcmp(func_name, "hash_sha256") == 0) {
                            buf_write(buf, "lp_hash_sha256(");
                        }
                        /* Rate limiting */
                        else if (strcmp(func_name, "check_rate_limit") == 0) {
                            buf_write(buf, "lp_check_func_rate_limit(");
                        } else if (strcmp(func_name, "rate_limit_remaining") == 0) {
                            buf_write(buf, "lp_rate_limit_remaining(");
                        }
                        /* Authentication context */
                        else if (strcmp(func_name, "set_authenticated") == 0) {
                            buf_write(buf, "lp_set_authenticated(");
                        } else if (strcmp(func_name, "set_guest") == 0) {
                            buf_write(buf, "lp_set_guest(");
                        } else if (strcmp(func_name, "is_authenticated") == 0) {
                            buf_write(buf, "lp_is_authenticated(");
                        } else if (strcmp(func_name, "get_access_level") == 0) {
                            buf_write(buf, "lp_get_access_level(");
                        }
                        /* Default fallback */
                        else {
                            buf_printf(buf, "lp_security_%s(", func_name);
                        }
                        
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 1: cp (Competitive Programming) ---- */
                    if (imp->tier == MOD_TIER1_DSA) {
                        /* Fast I/O functions */
                        if (strcmp(func_name, "read_int") == 0) {
                            buf_write(buf, "lp_io_read_int(");
                        } else if (strcmp(func_name, "read_float") == 0) {
                            buf_write(buf, "lp_io_read_float(");
                        } else if (strcmp(func_name, "read_str") == 0) {
                            buf_write(buf, "lp_io_read_str(");
                        } else if (strcmp(func_name, "read_line") == 0) {
                            buf_write(buf, "lp_io_read_line(");
                        } else if (strcmp(func_name, "write") == 0) {
                            buf_write(buf, "lp_io_write_int(");
                        } else if (strcmp(func_name, "write_str") == 0) {
                            buf_write(buf, "lp_io_write_str(");
                        } else if (strcmp(func_name, "writeln") == 0) {
                            buf_write(buf, "lp_io_writeln(");
                        } else if (strcmp(func_name, "write_int") == 0) {
                            buf_write(buf, "lp_io_write_int(");
                            /* Generate argument with type conversion if needed */
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                LpType arg_type = infer_type(cg, node->call.args.items[i]);
                                if (arg_type == LP_VAL || arg_type == LP_LIST) {
                                    buf_write(buf, "lp_val_as_int(");
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                    buf_write(buf, ")");
                                } else {
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                }
                            }
                            buf_write(buf, ")");
                            break;
                        } else if (strcmp(func_name, "write_int_ln") == 0) {
                            buf_write(buf, "lp_io_write_int_ln(");
                            /* Generate argument with type conversion if needed */
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                LpType arg_type = infer_type(cg, node->call.args.items[i]);
                                if (arg_type == LP_VAL || arg_type == LP_LIST) {
                                    buf_write(buf, "lp_val_as_int(");
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                    buf_write(buf, ")");
                                } else {
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                }
                            }
                            buf_write(buf, ")");
                            break;
                        } else if (strcmp(func_name, "write_str_ln") == 0) {
                            buf_write(buf, "lp_io_write_str_ln(");
                        } else if (strcmp(func_name, "flush") == 0) {
                            buf_write(buf, "lp_io_flush(");
                        }
                        /* Number Theory functions */
                        else if (strcmp(func_name, "sieve") == 0) {
                            buf_write(buf, "lp_nt_sieve(");
                        } else if (strcmp(func_name, "is_prime") == 0) {
                            buf_write(buf, "lp_nt_is_prime(");
                        } else if (strcmp(func_name, "mod_pow") == 0) {
                            buf_write(buf, "lp_nt_mod_pow(");
                        } else if (strcmp(func_name, "mod_inverse") == 0) {
                            buf_write(buf, "lp_nt_mod_inverse(");
                        } else if (strcmp(func_name, "extended_gcd") == 0) {
                            buf_write(buf, "lp_nt_extended_gcd(");
                        } else if (strcmp(func_name, "prime_factors") == 0) {
                            buf_write(buf, "lp_nt_prime_factors(");
                        } else if (strcmp(func_name, "euler_phi") == 0) {
                            buf_write(buf, "lp_nt_euler_phi(");
                        } else if (strcmp(func_name, "count_divisors") == 0) {
                            buf_write(buf, "lp_nt_count_divisors(");
                        } else if (strcmp(func_name, "sum_divisors") == 0) {
                            buf_write(buf, "lp_nt_sum_divisors(");
                        }
                        /* DSU functions */
                        else if (strcmp(func_name, "dsu_new") == 0) {
                            buf_write(buf, "lp_dsu_new(");
                        } else if (strcmp(func_name, "dsu_find") == 0) {
                            buf_write(buf, "lp_dsu_find(");
                        } else if (strcmp(func_name, "dsu_union") == 0) {
                            buf_write(buf, "lp_dsu_union(");
                        } else if (strcmp(func_name, "dsu_same") == 0) {
                            buf_write(buf, "lp_dsu_same(");
                        } else if (strcmp(func_name, "dsu_size") == 0) {
                            buf_write(buf, "lp_dsu_size(");
                        }
                        /* Heap functions */
                        else if (strcmp(func_name, "heap_new") == 0) {
                            buf_write(buf, "lp_heap_new(");
                        } else if (strcmp(func_name, "heap_push") == 0) {
                            buf_write(buf, "lp_heap_push(");
                        } else if (strcmp(func_name, "heap_pop") == 0) {
                            buf_write(buf, "lp_heap_pop(");
                        } else if (strcmp(func_name, "heap_top") == 0) {
                            buf_write(buf, "lp_heap_top(");
                        } else if (strcmp(func_name, "heap_is_empty") == 0) {
                            buf_write(buf, "lp_heap_is_empty(");
                        }
                        /* Fenwick Tree functions */
                        else if (strcmp(func_name, "fenwick_new") == 0) {
                            buf_write(buf, "lp_fenwick_new(");
                        } else if (strcmp(func_name, "fenwick_add") == 0) {
                            buf_write(buf, "lp_fenwick_add(");
                        } else if (strcmp(func_name, "fenwick_prefix_sum") == 0) {
                            buf_write(buf, "lp_fenwick_prefix_sum(");
                        } else if (strcmp(func_name, "fenwick_range_sum") == 0) {
                            buf_write(buf, "lp_fenwick_range_sum(");
                        }
                        /* Default fallback */
                        else {
                            buf_printf(buf, "lp_dsa_%s(", func_name);
                        }
                        
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 2: numpy - optimized C arrays ---- */
                    if (imp->tier == MOD_TIER2_NUMPY) {
                        /* np.array([1,2,3]) ??? from_doubles */
                        if (strcmp(func_name, "array") == 0 &&
                            node->call.args.count == 1 &&
                            node->call.args.items[0]->type == NODE_LIST_EXPR) {
                            NodeList *elems = &node->call.args.items[0]->list_expr.elems;
                            buf_printf(buf, "lp_np_from_doubles(%d", elems->count);
                            for (int i = 0; i < elems->count; i++) {
                                buf_write(buf, ", (double)(");
                                gen_expr(cg, buf, elems->items[i]);
                                buf_write(buf, ")");
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.zeros(n), np.ones(n) */
                        if (strcmp(func_name, "zeros") == 0 || strcmp(func_name, "ones") == 0) {
                            buf_printf(buf, "lp_np_%s(", func_name);
                            if (node->call.args.count > 0)
                                gen_expr(cg, buf, node->call.args.items[0]);
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.arange(start, stop, step) */
                        if (strcmp(func_name, "arange") == 0) {
                            buf_write(buf, "lp_np_arange(");
                            if (node->call.args.count == 1) {
                                buf_write(buf, "0.0, ");
                                gen_expr(cg, buf, node->call.args.items[0]);
                                buf_write(buf, ", 1.0)");
                            } else {
                                for (int i = 0; i < node->call.args.count; i++) {
                                    if (i > 0) buf_write(buf, ", ");
                                    gen_expr(cg, buf, node->call.args.items[i]);
                                }
                                if (node->call.args.count == 2) buf_write(buf, ", 1.0");
                                buf_write(buf, ")");
                            }
                            break;
                        }
                        /* np.linspace(start, stop, n) */
                        if (strcmp(func_name, "linspace") == 0) {
                            buf_write(buf, "lp_np_linspace(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.sum(arr), np.mean(arr), np.min, np.max, np.std, np.dot, np.sort */
                        if (strcmp(func_name, "sum") == 0 || strcmp(func_name, "mean") == 0 ||
                            strcmp(func_name, "min") == 0 || strcmp(func_name, "max") == 0 ||
                            strcmp(func_name, "std") == 0 || strcmp(func_name, "dot") == 0 ||
                            strcmp(func_name, "sort") == 0 || strcmp(func_name, "var") == 0 ||
                            strcmp(func_name, "median") == 0 || strcmp(func_name, "argmax") == 0 ||
                            strcmp(func_name, "argmin") == 0 || strcmp(func_name, "len") == 0 ||
                            strcmp(func_name, "flatten") == 0 || strcmp(func_name, "transpose") == 0 ||
                            strcmp(func_name, "cumsum") == 0 || strcmp(func_name, "reverse") == 0 ||
                            strcmp(func_name, "matmul") == 0 || strcmp(func_name, "diag") == 0) {
                            buf_printf(buf, "lp_np_%s(", func_name);
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.eye(n) - identity matrix */
                        if (strcmp(func_name, "eye") == 0 || strcmp(func_name, "identity") == 0) {
                            buf_write(buf, "lp_np_eye(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.clip(arr, min, max) */
                        if (strcmp(func_name, "clip") == 0) {
                            buf_write(buf, "lp_np_clip(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.power(arr, p) */
                        if (strcmp(func_name, "power") == 0) {
                            buf_write(buf, "lp_np_power(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.reshape2d(arr, rows, cols) */
                        if (strcmp(func_name, "reshape2d") == 0 || strcmp(func_name, "reshape") == 0) {
                            buf_write(buf, "lp_np_reshape2d(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.take(arr, indices) */
                        if (strcmp(func_name, "take") == 0) {
                            buf_write(buf, "lp_np_take(");
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.count_greater, count_less, count_equal */
                        if (strcmp(func_name, "count_greater") == 0 || 
                            strcmp(func_name, "count_less") == 0 ||
                            strcmp(func_name, "count_equal") == 0) {
                            buf_printf(buf, "lp_np_%s(", func_name);
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.add, np.subtract, np.multiply ??? element-wise ops */
                        if (strcmp(func_name, "add") == 0) {
                            buf_write(buf, "lp_np_add(");
                        } else if (strcmp(func_name, "subtract") == 0) {
                            buf_write(buf, "lp_np_sub(");
                        } else if (strcmp(func_name, "multiply") == 0) {
                            buf_write(buf, "lp_np_mul(");
                        } else if (strcmp(func_name, "sqrt") == 0) {
                            buf_write(buf, "lp_np_sqrt_arr(");
                        } else if (strcmp(func_name, "abs") == 0) {
                            buf_write(buf, "lp_np_abs_arr(");
                        } else if (strcmp(func_name, "sin") == 0) {
                            buf_write(buf, "lp_np_sin_arr(");
                        } else if (strcmp(func_name, "cos") == 0) {
                            buf_write(buf, "lp_np_cos_arr(");
                        } else if (strcmp(func_name, "exp") == 0) {
                            buf_write(buf, "lp_np_exp_arr(");
                        } else if (strcmp(func_name, "log") == 0) {
                            buf_write(buf, "lp_np_log_arr(");
                        } else {
                            /* Generic numpy function ??? fallback */
                            buf_printf(buf, "lp_np_%s(", func_name);
                        }
                        for (int i = 0; i < node->call.args.count; i++) {
                            if (i > 0) buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                        break;
                    }
                    /* ---- TIER 3: Python/C API fallback ---- */
                    if (imp->tier == MOD_TIER3_PYTHON) {
                        buf_printf(buf, "lp_py_call_method(lp_pymod_%s, \"%s\", ",
                                   imp->alias, func_name);
                        if (node->call.args.count == 0) {
                            buf_write(buf, "lp_py_args0()");
                        } else {
                            buf_printf(buf, "lp_py_args%d(",
                                       node->call.args.count <= 3 ? node->call.args.count : node->call.args.count);
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                LpType at = infer_type(cg, node->call.args.items[i]);
                                switch (at) {
                                    case LP_INT:    buf_write(buf, "lp_py_from_int("); break;
                                    case LP_FLOAT:  buf_write(buf, "lp_py_from_float("); break;
                                    case LP_STRING: buf_write(buf, "lp_py_from_str("); break;
                                    case LP_BOOL:   buf_write(buf, "lp_py_from_bool("); break;
                                    default:        buf_write(buf, "lp_py_from_int("); break;
                                }
                                gen_expr(cg, buf, node->call.args.items[i]);
                                buf_write(buf, ")");
                            }
                            buf_write(buf, ")");
                        }
                        buf_write(buf, ")");
                        break;
                    }
                }
            }
            /* === String method calls: e.g. text.upper() === */
            if (node->call.func->type == NODE_ATTRIBUTE) {
                LpType obj_type = infer_type(cg, node->call.func->attribute.obj);
                const char *attr = node->call.func->attribute.attr;
                if (obj_type == LP_FILE) {
                    buf_printf(buf, "lp_io_%s(", attr);
                    gen_expr(cg, buf, node->call.func->attribute.obj);
                    for (int i = 0; i < node->call.args.count; i++) {
                        buf_write(buf, ", ");
                        gen_expr(cg, buf, node->call.args.items[i]);
                    }
                    buf_write(buf, ")");
                    break;
                }
                if (obj_type == LP_STRING) {
                    if (strcmp(attr, "join") == 0) {
                        buf_write(buf, "lp_str_join(");
                        gen_expr(cg, buf, node->call.func->attribute.obj); /* The separator string */
                        if (node->call.args.count > 0) {
                            buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[0]); /* The array */
                        }
                        buf_write(buf, ")");
                    } else if (strcmp(attr, "split") == 0) {
                        buf_write(buf, "lp_str_split(");
                        gen_expr(cg, buf, node->call.func->attribute.obj);
                        buf_write(buf, ", ");
                        if (node->call.args.count > 0) {
                            gen_expr(cg, buf, node->call.args.items[0]);
                        } else {
                            buf_write(buf, "NULL");
                        }
                        buf_write(buf, ")");
                    } else {
                        buf_printf(buf, "lp_str_%s(", attr);
                        gen_expr(cg, buf, node->call.func->attribute.obj); /* The string itself is the first arg */
                        for (int i = 0; i < node->call.args.count; i++) {
                            buf_write(buf, ", ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                        }
                        buf_write(buf, ")");
                    }
                    break;
                }
                if (obj_type == LP_LIST) {
                    if (strcmp(attr, "append") == 0) {
                        buf_write(buf, "lp_list_append(");
                        gen_expr(cg, buf, node->call.func->attribute.obj);
                        for (int i = 0; i < node->call.args.count; i++) {
                            buf_write(buf, ", ");
                            if (infer_type(cg, node->call.args.items[i]) != LP_VAL) {
                                emit_lp_val(cg, buf, node->call.args.items[i]);
                            } else {
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                        }
                        buf_write(buf, ")");
                        break;
                    } else if (strcmp(attr, "sort") == 0) {
                        buf_write(buf, "lp_list_sort(");
                        gen_expr(cg, buf, node->call.func->attribute.obj);
                        buf_write(buf, ")");
                        break;
                    } else if (strcmp(attr, "binary_search") == 0) {
                        buf_write(buf, "lp_list_binary_search(");
                        gen_expr(cg, buf, node->call.func->attribute.obj);
                        buf_write(buf, ", ");
                        if (node->call.args.count > 0) {
                            emit_lp_val(cg, buf, node->call.args.items[0]);
                        } else {
                            buf_write(buf, "lp_val_null()");
                        }
                        buf_write(buf, ")");
                        break;
                    }
                }
            }
            /* Special-case: print() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "print") == 0) {
                for (int i = 0; i < node->call.args.count; i++) {
                    if (i > 0) buf_write(buf, ", ");
                    LpType at = infer_type(cg, node->call.args.items[i]);
                    if (node->call.args.items[i]->type == NODE_CALL &&
                        node->call.args.items[i]->call.func->type == NODE_NAME &&
                        (strcmp(node->call.args.items[i]->call.func->name_expr.name, "min") == 0 ||
                         strcmp(node->call.args.items[i]->call.func->name_expr.name, "max") == 0)) {
                        LpType minmax_type = infer_type(cg, node->call.args.items[i]);
                        at = minmax_type != LP_UNKNOWN ? minmax_type : LP_VAL;
                    }
                    switch (at) {
                        case LP_FLOAT:  buf_write(buf, "lp_print_float("); break;
                        case LP_STRING: buf_write(buf, "lp_print_str("); break;
                        case LP_BOOL:   buf_write(buf, "lp_print_bool("); break;
                        case LP_LIST:   buf_write(buf, "lp_list_print("); break;
                        case LP_ARRAY:  buf_write(buf, "lp_np_print("); break;
                        case LP_STR_ARRAY: buf_write(buf, "lp_print_str_array("); break;
                        case LP_PYOBJ:  buf_write(buf, "lp_py_print("); break;
                        case LP_DICT:   buf_write(buf, "lp_print_dict("); break;
                        case LP_SET:    buf_write(buf, "lp_print_set("); break;
                        case LP_TUPLE:  buf_write(buf, "lp_print_tuple("); break;
                        case LP_VAL:    buf_write(buf, "lp_print_val("); break;
                        default:        buf_write(buf, "lp_print_generic("); break;
                    }
                    gen_expr(cg, buf, node->call.args.items[i]);
                    buf_write(buf, ")");
                }
                break;
            }
            /* Special-case: min(), max() */
            if (node->call.func->type == NODE_NAME &&
                (strcmp(node->call.func->name_expr.name, "min") == 0 ||
                 strcmp(node->call.func->name_expr.name, "max") == 0)) {
                LpType minmax_type = infer_minmax_call_type(cg, node);
                int is_min = strcmp(node->call.func->name_expr.name, "min") == 0;

                if (minmax_type == LP_INT || minmax_type == LP_FLOAT) {
                    emit_native_numeric_minmax(cg, buf, node, is_min, minmax_type);
                    break;
                }

                buf_write(buf, "({ LpVal __lp_minmax_res; ");

                if (node->call.args.count == 1) {
                    /* Arg is a list or tuple */
                    buf_write(buf, "LpVal __lp_minmax_arg = ");
                    emit_lp_val(cg, buf, node->call.args.items[0]);
                    buf_write(buf, "; ");
                    buf_write(buf, "if (__lp_minmax_arg.type == LP_VAL_LIST && __lp_minmax_arg.as.l->len > 0) { ");
                    buf_write(buf, "__lp_minmax_res = __lp_minmax_arg.as.l->items[0]; ");
                    buf_write(buf, "for (int64_t __lp_i = 1; __lp_i < __lp_minmax_arg.as.l->len; __lp_i++) { ");
                    buf_write(buf, "LpVal __v = __lp_minmax_arg.as.l->items[__lp_i]; ");
                    if (is_min) {
                        buf_write(buf, "if (lp_val_lt(__v, __lp_minmax_res)) __lp_minmax_res = __v; ");
                    } else {
                        buf_write(buf, "if (lp_val_gt(__v, __lp_minmax_res)) __lp_minmax_res = __v; ");
                    }
                    buf_write(buf, "} } else { __lp_minmax_res = lp_val_null(); } ");
                } else if (node->call.args.count > 1) {
                    /* Multiple arguments */
                    buf_write(buf, "__lp_minmax_res = ");
                    emit_lp_val(cg, buf, node->call.args.items[0]);
                    buf_write(buf, "; ");
                    for (int i = 1; i < node->call.args.count; i++) {
                        buf_write(buf, "LpVal __v_");
                        buf_printf(buf, "%d = ", i);
                        emit_lp_val(cg, buf, node->call.args.items[i]);
                        buf_write(buf, "; ");
                        if (is_min) {
                            buf_write(buf, "if (lp_val_lt(__v_");
                            buf_printf(buf, "%d, __lp_minmax_res)) __lp_minmax_res = __v_%d; ", i, i);
                        } else {
                            buf_write(buf, "if (lp_val_gt(__v_");
                            buf_printf(buf, "%d, __lp_minmax_res)) __lp_minmax_res = __v_%d; ", i, i);
                        }
                    }
                } else {
                    buf_write(buf, "__lp_minmax_res = lp_val_null(); ");
                }
                buf_write(buf, "__lp_minmax_res; })");
                break;
            }

            /* Special-case: int(), float(), bool() */
            if (node->call.func->type == NODE_NAME &&
                (strcmp(node->call.func->name_expr.name, "int") == 0 ||
                 strcmp(node->call.func->name_expr.name, "float") == 0 ||
                 strcmp(node->call.func->name_expr.name, "bool") == 0)) {
                buf_printf(buf, "lp_%s(", node->call.func->name_expr.name);
                if (node->call.args.count > 0) {
                    emit_lp_val(cg, buf, node->call.args.items[0]);
                } else {
                    buf_write(buf, "lp_val_null()");
                }
                buf_write(buf, ")");
                break;
            }
            /* Special-case: str() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "str") == 0) {
                if (node->call.args.count > 0) {
                    buf_write(buf, "lp_str(");
                    emit_lp_val(cg, buf, node->call.args.items[0]);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "\"\"");
                }
                break;
            }
            /* Special-case: input() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "input") == 0) {
                buf_write(buf, "lp_io_input(");
                if (node->call.args.count > 0) {
                    gen_expr(cg, buf, node->call.args.items[0]);
                } else {
                    buf_write(buf, "NULL");
                }
                buf_write(buf, ")");
                break;
            }
            /* Special-case: len() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "len") == 0) {
                if (node->call.args.count > 0) {
                    LpType at = infer_type(cg, node->call.args.items[0]);
                    if (at == LP_ARRAY) {
                        buf_write(buf, "lp_np_len(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")");
                    } else if (at == LP_NATIVE_ARRAY_1D) {
                        buf_write(buf, "(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")->len");
                    } else if (at == LP_NATIVE_ARRAY_2D) {
                        buf_write(buf, "(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")->rows");
                    } else if (at == LP_LIST) {
                        buf_write(buf, "(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")->len");
                    } else if (at == LP_DICT) {
                        buf_write(buf, "(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")->count");
                    } else if (at == LP_STRING) {
                        buf_write(buf, "strlen(");
                        gen_expr(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")");
                    } else {
                        buf_write(buf, "lp_val_len(");
                        emit_lp_val(cg, buf, node->call.args.items[0]);
                        buf_write(buf, ")");
                    }
                }
                break;
            }
            /* Special-case: open() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "open") == 0) {
                buf_write(buf, "lp_io_open(");
                for (int i = 0; i < node->call.args.count; i++) {
                    if (i > 0) buf_write(buf, ", ");
                    LpType expected = LP_UNKNOWN;
                    if (node->call.func->type == NODE_NAME) {
                        Symbol *s = scope_lookup(cg->scope, node->call.func->name_expr.name);
                        if (s && i < 16) expected = s->param_types[i];
                    }
                    if (expected != LP_UNKNOWN) {
                        emit_cast(cg, buf, node->call.args.items[i], expected);
                    } else {
                        gen_expr(cg, buf, node->call.args.items[i]);
                    }
                }
                buf_write(buf, ")");
                break;
            }
            /* Class Instantiation / Constructor (obj = MyClass()) */
            if (node->call.func->type == NODE_NAME) {
                Symbol *s = scope_lookup(cg->scope, node->call.func->name_expr.name);
                if (s && s->type == LP_CLASS) {
                    const char *cname = node->call.func->name_expr.name;
                    buf_printf(buf, "({ LpObj_%s* _lp_tmp_obj = (LpObj_%s*)malloc(sizeof(LpObj_%s)); %s_init(_lp_tmp_obj); _lp_tmp_obj; })", cname, cname, cname, cname);
                    break;
                }
            }
            /* Object Method Call: obj.method(args) -> Class_method(obj, args) */
            if (node->call.func->type == NODE_ATTRIBUTE &&
                node->call.func->attribute.obj->type == NODE_NAME) {
                Symbol *s = scope_lookup(cg->scope, node->call.func->attribute.obj->name_expr.name);
                if (s && s->type == LP_OBJECT && s->class_name) {
                    const char *target_class = s->class_name;
                    char method_key[256];
                    snprintf(method_key, sizeof(method_key), "%s.%s", s->class_name, node->call.func->attribute.attr);
                    Symbol *ms = scope_lookup(cg->scope, method_key);
                    if (ms && !can_access_member(cg, ms)) {
                        codegen_set_error(cg,
                            "Cannot access %s member '%s' of class '%s'",
                            ms->access == TOK_PRIVATE ? "private" : "protected",
                            node->call.func->attribute.attr,
                            ms->owner_class ? ms->owner_class : s->class_name);
                    }
                    if (ms && ms->owner_class) target_class = ms->owner_class;

                    buf_printf(buf, "lp_%s_%s(", target_class, node->call.func->attribute.attr);
                    buf_printf(buf, "(LpObj_%s*)lp_%s", target_class, s->name); /* Pass 'self' casted properly */
                    for (int i = 0; i < node->call.args.count; i++) {
                        buf_write(buf, ", ");
                        gen_expr(cg, buf, node->call.args.items[i]);
                    }
                    buf_write(buf, ")");
                    break;
                }
            }
            /* Regular function call */
            int has_unpack = 0;
            int has_kwargs_call = 0;
            int fixed_params = 0;
            Symbol *fsym_call = NULL;
            if (node->call.func->type == NODE_NAME) {
                fsym_call = scope_lookup(cg->scope, node->call.func->name_expr.name);
                if (fsym_call && fsym_call->is_variadic) {
                    has_unpack = 1;
                    /* Count fixed params (all except *args and **kwargs) */
                    fixed_params = fsym_call->num_params - 1;
                    if (fsym_call->has_kwargs) fixed_params--;
                }
                if (fsym_call && fsym_call->has_kwargs) {
                    has_kwargs_call = 1;
                    /* If not variadic, fixed_params is all params except **kwargs */
                    if (!fsym_call->is_variadic) {
                        fixed_params = fsym_call->num_params - 1;
                    }
                }
            }

            /* Check for *list unpacking in call args */
            for (int i = 0; i < node->call.args.count; i++) {
                if (node->call.is_unpacked_list && node->call.is_unpacked_list[i]) has_unpack = 1;
            }

            /* Check if any arg is NODE_KWARG (name=value) */
            int has_kwarg_nodes = 0;
            for (int i = 0; i < node->call.args.count; i++) {
                if (node->call.args.items[i]->type == NODE_KWARG) has_kwarg_nodes = 1;
            }
            if (has_kwarg_nodes) has_kwargs_call = 1;

            if (has_unpack || has_kwargs_call) {
                buf_write(buf, "({ ");

                /* Build *args LpVarArgs if needed */
                if (has_unpack) {
                    buf_write(buf, "LpVarArgs _args = lp_args_new(4); ");
                    for (int i = fixed_params; i < node->call.args.count; i++) {
                        if (node->call.args.items[i]->type == NODE_KWARG) continue; /* skip kwargs */
                        if (node->call.is_unpacked_list && node->call.is_unpacked_list[i]) {
                            buf_write(buf, "lp_args_extend_array(&_args, ");
                            gen_expr(cg, buf, node->call.args.items[i]);
                            buf_write(buf, "); ");
                        } else {
                            int arg_type = infer_type(cg, node->call.args.items[i]);
                            buf_printf(buf, "lp_args_push(&_args, %d, (void*)(intptr_t)", arg_type);
                            gen_expr(cg, buf, node->call.args.items[i]);
                            buf_write(buf, "); ");
                        }
                    }
                }

                /* Build **kwargs LpDict if needed */
                if (has_kwargs_call) {
                    buf_write(buf, "LpDict *_kwargs = lp_dict_new(); ");
                    for (int i = 0; i < node->call.args.count; i++) {
                        AstNode *arg = node->call.args.items[i];
                        if (arg->type == NODE_KWARG) {
                            buf_printf(buf, "lp_dict_set(_kwargs, \"%s\", ", arg->kwarg.name);
                            emit_lp_val(cg, buf, arg->kwarg.value);
                            buf_write(buf, "); ");
                        } else if (node->call.is_unpacked_dict && node->call.is_unpacked_dict[i]) {
                            /* **dict_var unpacking ??? merge into _kwargs */
                            buf_write(buf, "lp_dict_merge(_kwargs, ");
                            gen_expr(cg, buf, arg);
                            buf_write(buf, "); ");
                        }
                    }
                }

                /* Emit typeof + result capture */
                buf_write(buf, "__typeof__(");
                if (node->call.func->type == NODE_NAME) {
                    buf_printf(buf, "lp_%s(", node->call.func->name_expr.name);
                } else {
                    gen_expr(cg, buf, node->call.func);
                    buf_write(buf, "(");
                }
                /* dummy args for typeof */
                int first_arg = 1;
                for (int i = 0; i < fixed_params && i < node->call.args.count; i++) {
                    if (!first_arg) buf_write(buf, ", ");
                    gen_expr(cg, buf, node->call.args.items[i]);
                    first_arg = 0;
                }
                if (has_unpack) {
                    if (!first_arg) buf_write(buf, ", ");
                    buf_write(buf, "lp_args_to_array(&_args)");
                    first_arg = 0;
                }
                if (has_kwargs_call) {
                    if (!first_arg) buf_write(buf, ", ");
                    buf_write(buf, "_kwargs");
                    first_arg = 0;
                }
                buf_write(buf, ")) _res = ");

                /* Actual function call */
                if (node->call.func->type == NODE_NAME) {
                    buf_printf(buf, "lp_%s(", node->call.func->name_expr.name);
                } else {
                    gen_expr(cg, buf, node->call.func);
                    buf_write(buf, "(");
                }
                first_arg = 1;
                for (int i = 0; i < fixed_params && i < node->call.args.count; i++) {
                    if (!first_arg) buf_write(buf, ", ");
                    gen_expr(cg, buf, node->call.args.items[i]);
                    first_arg = 0;
                }
                if (has_unpack) {
                    if (!first_arg) buf_write(buf, ", ");
                    buf_write(buf, "lp_args_to_array(&_args)");
                    first_arg = 0;
                }
                if (has_kwargs_call) {
                    if (!first_arg) buf_write(buf, ", ");
                    buf_write(buf, "_kwargs");
                    first_arg = 0;
                }
                buf_write(buf, "); ");

                /* Cleanup */
                if (has_unpack) buf_write(buf, "lp_args_free(&_args); ");
                buf_write(buf, "_res; })");
            } else {
                if (node->call.func->type == NODE_NAME) {
                    buf_printf(buf, "lp_%s(", node->call.func->name_expr.name);
                } else {
                    gen_expr(cg, buf, node->call.func);
                    buf_write(buf, "(");
                }
                for (int i = 0; i < node->call.args.count; i++) {
                    if (i > 0) buf_write(buf, ", ");
                    LpType expected = LP_UNKNOWN;
                    if (node->call.func->type == NODE_NAME) {
                        Symbol *s = scope_lookup(cg->scope, node->call.func->name_expr.name);
                        if (s && i < 16) expected = s->param_types[i];
                    }
                    if (expected != LP_UNKNOWN) {
                        emit_cast(cg, buf, node->call.args.items[i], expected);
                    } else {
                        gen_expr(cg, buf, node->call.args.items[i]);
                    }
                }
                buf_write(buf, ")");
            }
            break;
        }
        case NODE_KWARG:
            /* Keyword argument ??? only used inline when not packed into dict */
            gen_expr(cg, buf, node->kwarg.value);
            break;
        case NODE_LIST_COMP: {
            /* [expr for var in iter if cond]
             * Generates: ({ LpList *_lc = lp_list_new();
             *              for(int lp_var = ...) { if(cond) lp_list_append(_lc, expr); }
             *              _lc; })
             */
            buf_write(buf, "({ ");
            buf_write(buf, "LpList *_lc = lp_list_new(); ");

            /* Define loop var in a new scope so it can be resolved during expression generation */
            Scope *comp_scope = scope_new(cg->scope);
            cg->scope = comp_scope;

            /* Generate the iteration. */
            if (is_range_call(node->list_comp.iter)) {
                NodeList *args = &node->list_comp.iter->call.args;
                scope_define(cg->scope, node->list_comp.var, LP_INT);
                if (args->count == 1) {
                    buf_printf(buf, "for (int64_t lp_%s = 0; lp_%s < ",
                               node->list_comp.var, node->list_comp.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s++) { ", node->list_comp.var);
                } else if (args->count >= 2) {
                    buf_printf(buf, "for (int64_t lp_%s = ", node->list_comp.var);
                    gen_expr(cg, buf, args->items[0]);
                    
                    if (args->count >= 3) {
                        /* Tạo block để khai báo biến step dùng chung */
                        buf_write(buf, "; 0;) { break; } "); /* Hack đóng lệnh for rỗng nêú cần, hoặc viết lại block */
                        
                        buf_printf(buf, "int64_t __step_%s = ", node->list_comp.var);
                        gen_expr(cg, buf, args->items[2]);
                        buf_write(buf, "; ");
                        buf_printf(buf, "if (__step_%s > 0) { ", node->list_comp.var);
                        buf_printf(buf, "for (int64_t lp_%s = ", node->list_comp.var);
                        gen_expr(cg, buf, args->items[0]);
                        buf_printf(buf, "; lp_%s < ", node->list_comp.var);
                        gen_expr(cg, buf, args->items[1]);
                        buf_printf(buf, "; lp_%s += __step_%s) { ", node->list_comp.var, node->list_comp.var);
                    } else {
                        buf_printf(buf, "; lp_%s < ", node->list_comp.var);
                        gen_expr(cg, buf, args->items[1]);
                        buf_printf(buf, "; lp_%s++", node->list_comp.var);
                        buf_write(buf, ") { ");
                    }
                }
            } else {
                LpType iter_type = infer_type(cg, node->list_comp.iter);
                if (iter_type == LP_NATIVE_ARRAY_1D) {
                    scope_define(cg->scope, node->list_comp.var, LP_INT);
                    static int native_int_lc_loop_counter = 0;
                    int cur_loop = native_int_lc_loop_counter++;
                    buf_printf(buf, "LpIntArray* __lp_lc_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->list_comp.iter);
                    buf_write(buf, "; ");
                    buf_printf(buf, "int64_t __lp_lc_len_%d = __lp_lc_iter_%d->len; ", cur_loop, cur_loop);
                    buf_printf(buf, "for (int64_t __lp_lc_i_%d = 0; __lp_lc_i_%d < __lp_lc_len_%d; __lp_lc_i_%d++) { ",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    buf_printf(buf, "int64_t lp_%s = __lp_lc_iter_%d->data[__lp_lc_i_%d]; ",
                               node->list_comp.var, cur_loop, cur_loop);
                } else if (iter_type == LP_ARRAY) {
                    scope_define(cg->scope, node->list_comp.var, LP_FLOAT);
                    static int native_float_lc_loop_counter = 0;
                    int cur_loop = native_float_lc_loop_counter++;
                    buf_printf(buf, "LpArray __lp_lc_arr_%d = ", cur_loop);
                    gen_expr(cg, buf, node->list_comp.iter);
                    buf_write(buf, "; ");
                    buf_printf(buf, "int64_t __lp_lc_len_%d = __lp_lc_arr_%d.len; ", cur_loop, cur_loop);
                    buf_printf(buf, "for (int64_t __lp_lc_i_%d = 0; __lp_lc_i_%d < __lp_lc_len_%d; __lp_lc_i_%d++) { ",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    buf_printf(buf, "double lp_%s = __lp_lc_arr_%d.data[__lp_lc_i_%d]; ",
                               node->list_comp.var, cur_loop, cur_loop);
                } else if (iter_type == LP_STR_ARRAY) {
                    scope_define(cg->scope, node->list_comp.var, LP_STRING);
                    static int generic_lc_loop_counter = 0;
                    int cur_loop = generic_lc_loop_counter++;
                    buf_printf(buf, "LpStrArray __lp_lc_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->list_comp.iter);
                    buf_write(buf, "; ");
                    buf_printf(buf, "int64_t __lp_lc_len_%d = __lp_lc_iter_%d.count; ", cur_loop, cur_loop);
                    buf_printf(buf, "for (int64_t __lp_lc_i_%d = 0; __lp_lc_i_%d < __lp_lc_len_%d; __lp_lc_i_%d++) { ", cur_loop, cur_loop, cur_loop, cur_loop);
                    buf_printf(buf, "const char *lp_%s = __lp_lc_iter_%d.items[__lp_lc_i_%d]; ", node->list_comp.var, cur_loop, cur_loop);
                } else if (iter_type == LP_VAL || iter_type == LP_LIST || iter_type == LP_PYOBJ || iter_type == LP_ARRAY || iter_type == LP_DICT || iter_type == LP_SET || iter_type == LP_TUPLE) {
                    scope_define(cg->scope, node->list_comp.var, LP_VAL);
                    static int generic_lc_loop_counter = 0;
                    int cur_loop = generic_lc_loop_counter++;
                    buf_printf(buf, "LpVal __lp_lc_iter_%d = ", cur_loop);
                    emit_lp_val(cg, buf, node->list_comp.iter);
                    buf_write(buf, "; ");
                    buf_printf(buf, "int64_t __lp_lc_len_%d = lp_val_len(__lp_lc_iter_%d); ", cur_loop, cur_loop);
                    buf_printf(buf, "for (int64_t __lp_lc_i_%d = 0; __lp_lc_i_%d < __lp_lc_len_%d; __lp_lc_i_%d++) { ", cur_loop, cur_loop, cur_loop, cur_loop);
                    buf_printf(buf, "LpVal lp_%s = lp_val_getitem_int(__lp_lc_iter_%d, __lp_lc_i_%d); ", node->list_comp.var, cur_loop, cur_loop);
                } else {
                    buf_write(buf, "/* TODO: generic for loop */ ");
                    buf_write(buf, "{ ");
                }
            }

            /* Optional if condition */
            if (node->list_comp.cond) {
                buf_write(buf, "if (");
                gen_expr(cg, buf, node->list_comp.cond);
                buf_write(buf, ") { ");
            }

            buf_write(buf, "lp_list_append(_lc, ");
            if (infer_type(cg, node->list_comp.expr) != LP_VAL) {
                emit_lp_val(cg, buf, node->list_comp.expr);
            } else {
                gen_expr(cg, buf, node->list_comp.expr);
            }
            buf_write(buf, "); ");

            if (node->list_comp.cond) {
                buf_write(buf, "} ");
            }

            buf_write(buf, "} "); /* close for loop */
            buf_write(buf, "_lc; })");

            /* Restore scope */
            cg->scope = comp_scope->parent;
            scope_free(comp_scope);
            break;
        }
        case NODE_LAMBDA: {
            /* Lambda: generate a static function in cg->funcs and reference by name */
            static int lambda_counter = 0;
            char fname[64];
            snprintf(fname, sizeof(fname), "_lp_lambda_%d", lambda_counter++);

            LpType body_ret;
            if (node->lambda_expr.is_multiline) {
                /* Infer return type from body statements */
                body_ret = infer_return_type(cg, &node->lambda_expr.body_stmts);
                if (body_ret == LP_UNKNOWN) body_ret = LP_INT;
            } else {
                body_ret = infer_type(cg, node->lambda_expr.body);
                if (body_ret == LP_UNKNOWN) body_ret = LP_FLOAT;
            }

            /* Generate the lambda function definition */
            buf_printf(&cg->helpers, "static %s %s(", lp_type_to_c(body_ret), fname);
            for (int i = 0; i < node->lambda_expr.params.count; i++) {
                if (i > 0) buf_write(&cg->helpers, ", ");
                Param *lp = &node->lambda_expr.params.items[i];
                LpType pt = lp->type_ann ? type_from_annotation(cg, lp->type_ann) : LP_FLOAT;
                buf_printf(&cg->helpers, "%s lp_%s", lp_type_to_c(pt), lp->name);
            }
            if (node->lambda_expr.params.count == 0) {
                buf_write(&cg->helpers, "void");
            }
            buf_write(&cg->helpers, ") {\n");

            if (node->lambda_expr.is_multiline) {
                /* Multiline lambda: generate full body */
                for (int i = 0; i < node->lambda_expr.body_stmts.count; i++)
                    gen_stmt(cg, &cg->helpers, node->lambda_expr.body_stmts.items[i], 1);
            } else {
                /* Single expression lambda */
                buf_write(&cg->helpers, "    return ");
                gen_expr(cg, &cg->helpers, node->lambda_expr.body);
                buf_write(&cg->helpers, ";\n");
            }
            buf_write(&cg->helpers, "}\n\n");

            /* Emit the function name as the expression */
            buf_write(buf, fname);
            break;
        }
        case NODE_SUBSCRIPT: {
            LpType obj_type = infer_type(cg, node->subscript.obj);
            if (obj_type == LP_DICT) {
                LpType key_type = infer_type(cg, node->subscript.index);
                if (key_type == LP_STRING) {
                    buf_write(buf, "lp_dict_get(");
                    gen_expr(cg, buf, node->subscript.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript.index);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "lp_val_null()");
                }
            } else if (obj_type == LP_PYOBJ || obj_type == LP_VAL || obj_type == LP_LIST) {
                LpType key_type = infer_type(cg, node->subscript.index);
                if (key_type == LP_STRING) {
                    buf_write(buf, "lp_val_getitem_str(");
                    emit_lp_val(cg, buf, node->subscript.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript.index);
                    buf_write(buf, ")");
                } else {
                    buf_write(buf, "lp_val_getitem_int(");
                    emit_lp_val(cg, buf, node->subscript.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript.index);
                    buf_write(buf, ")");
                }
            } else if (obj_type == LP_STR_ARRAY) {
                /* LpStrArray.items[index] */
                gen_expr(cg, buf, node->subscript.obj);
                buf_write(buf, ".items[");
                gen_expr(cg, buf, node->subscript.index);
                buf_write(buf, "]");
            } else if (obj_type == LP_NATIVE_ARRAY_1D) {
                /* Emit raw data access so hot loops stay as plain array loads. */
                emit_native_int_array_access(cg, buf, node->subscript.obj, node->subscript.index);
            } else if (obj_type == LP_NATIVE_ARRAY_2D) {
                /* LpIntArray2D* -> use lp_int_array2d_get(arr, row, col) */
                buf_write(buf, "lp_int_array2d_get(");
                gen_expr(cg, buf, node->subscript.obj);
                buf_write(buf, ", ");
                gen_expr(cg, buf, node->subscript.index);
                buf_write(buf, ", 0");  /* Default column for 1D-style access */
                buf_write(buf, ")");
            } else {
                gen_expr(cg, buf, node->subscript.obj);
                buf_write(buf, "[");
                gen_expr(cg, buf, node->subscript.index);
                buf_write(buf, "]");
            }
            break;
        }
        case NODE_ATTRIBUTE: {
            /* Object instance method or property access */
            if (node->attribute.obj->type == NODE_NAME) {
                Symbol *s = scope_lookup(cg->scope, node->attribute.obj->name_expr.name);
                if (s && s->type == LP_OBJECT) {
                    if (s->class_name) {
                        char field_key[256];
                        snprintf(field_key, sizeof(field_key), "%s.%s", s->class_name, node->attribute.attr);
                        Symbol *mem = scope_lookup(cg->scope, field_key);
                        if (mem && !can_access_member(cg, mem)) {
                            codegen_set_error(cg,
                                "Cannot access %s member '%s' of class '%s'",
                                mem->access == TOK_PRIVATE ? "private" : "protected",
                                node->attribute.attr,
                                mem->owner_class ? mem->owner_class : s->class_name);
                        }
                    }
                    /* Access property/method:  obj->attr */
                    gen_expr(cg, buf, node->attribute.obj);
                    buf_write(buf, "->lp_");
                    buf_write(buf, node->attribute.attr);
                    break;
                }
            }
            /* Module constant access: math.pi, math.e, etc. */
            if (node->attribute.obj->type == NODE_NAME) {
                ImportInfo *imp = find_import(cg, node->attribute.obj->name_expr.name);
                if (imp) {
                    if (imp->tier == MOD_TIER1_MATH || imp->tier == MOD_TIER2_NUMPY) {
                        buf_printf(buf, "lp_%s_%s", imp->module, node->attribute.attr);
                        break;
                    }
                    if (imp->tier == MOD_TIER3_PYTHON) {
                        buf_printf(buf, "lp_py_getattr(lp_pymod_%s, \"%s\")",
                                   imp->alias, node->attribute.attr);
                        break;
                    }
                    /* Tier 1: os, sys ??? attribute access */
                    if (imp->tier == MOD_TIER1_OS || imp->tier == MOD_TIER1_SYS) {
                        buf_printf(buf, "lp_%s_%s", imp->module, node->attribute.attr);
                        break;
                    }
                }
            }
            LpType obj_type = infer_type(cg, node->attribute.obj);
            if (obj_type == LP_VAL) {
                buf_write(buf, "lp_val_getitem_str(");
                gen_expr(cg, buf, node->attribute.obj);
                buf_printf(buf, ", \"%s\")", node->attribute.attr);
                break;
            }
            gen_expr(cg, buf, node->attribute.obj);
            buf_printf(buf, ".%s", node->attribute.attr);
            break;
        }
        case NODE_DICT_COMP: {
            /* {key: val for var in iter if cond}
             * Generates: ({ LpDict *_dc = lp_dict_new();
             *              for(int lp_var = ...) { if(cond) lp_dict_set(_dc, key, val); }
             *              _dc; })
             */
            buf_write(buf, "({ ");
            buf_write(buf, "LpDict *_dc = lp_dict_new(); ");

            /* Define loop var in a new scope so it can be resolved during expression generation */
            Scope *comp_scope = scope_new(cg->scope);
            cg->scope = comp_scope;

            /* Generate the iteration. */
            if (is_range_call(node->dict_comp.iter)) {
                NodeList *args = &node->dict_comp.iter->call.args;
                scope_define(cg->scope, node->dict_comp.var, LP_INT);
                if (args->count == 1) {
                    buf_printf(buf, "for (int64_t lp_%s = 0; lp_%s < ",
                               node->dict_comp.var, node->dict_comp.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s++) { ", node->dict_comp.var);
                } else if (args->count >= 2) {
                    buf_printf(buf, "for (int64_t lp_%s = ", node->dict_comp.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_write(buf, "; lp_");
                    buf_write(buf, node->dict_comp.var);
                    buf_write(buf, " < ");
                    gen_expr(cg, buf, args->items[1]);
                    if (args->count >= 3) {
                        buf_write(buf, "; 0;) { break; } "); /* Hack for step variable */
                        buf_printf(buf, "int64_t __step_%s = ", node->dict_comp.var);
                        gen_expr(cg, buf, args->items[2]);
                        buf_write(buf, "; ");
                        buf_printf(buf, "for (int64_t lp_%s = ", node->dict_comp.var);
                        gen_expr(cg, buf, args->items[0]);
                        buf_write(buf, "; lp_");
                        buf_write(buf, node->dict_comp.var);
                        buf_write(buf, " < ");
                        gen_expr(cg, buf, args->items[1]);
                        buf_printf(buf, "; lp_%s += __step_%s) { ", node->dict_comp.var, node->dict_comp.var);
                    } else {
                        buf_printf(buf, "; lp_%s++) { ", node->dict_comp.var);
                    }
                } else {
                    buf_printf(buf, "for (int64_t lp_%s = 0; lp_%s < 0; lp_%s++) { ",
                               node->dict_comp.var, node->dict_comp.var, node->dict_comp.var);
                }
            } else {
                /* Generic iteration over array/list */
                scope_define(cg->scope, node->dict_comp.var, LP_INT);
                buf_write(buf, "for (int64_t _lp_i = 0; _lp_i < ");
                gen_expr(cg, buf, node->dict_comp.iter);
                buf_printf(buf, ".len; _lp_i++) { int64_t lp_%s = (int64_t)", node->dict_comp.var);
                gen_expr(cg, buf, node->dict_comp.iter);
                buf_write(buf, ".data[_lp_i]; ");
            }

            /* Optional condition */
            if (node->dict_comp.cond) {
                buf_write(buf, "if (");
                gen_expr(cg, buf, node->dict_comp.cond);
                buf_write(buf, ") { ");
            }

            /* Set the key-value pair */
            buf_write(buf, "lp_dict_set(_dc, ");
            /* Convert key to string if it's an integer */
            LpType key_type = infer_type(cg, node->dict_comp.key);
            if (key_type == LP_INT) {
                buf_write(buf, "lp_str_from_int(");
                gen_expr(cg, buf, node->dict_comp.key);
                buf_write(buf, ")");
            } else if (key_type == LP_FLOAT) {
                buf_write(buf, "lp_str_from_float(");
                gen_expr(cg, buf, node->dict_comp.key);
                buf_write(buf, ")");
            } else {
                gen_expr(cg, buf, node->dict_comp.key);
            }
            buf_write(buf, ", ");
            emit_lp_val(cg, buf, node->dict_comp.value);
            buf_write(buf, "); ");

            if (node->dict_comp.cond) {
                buf_write(buf, "} ");
            }

            buf_write(buf, "} ");

            /* Pop scope */
            cg->scope = comp_scope->parent;
            scope_free(comp_scope);

            buf_write(buf, "_dc; })");
            break;
        }
        case NODE_YIELD:
            /* Yield expression - basic implementation */
            /* For now, yield just returns the value (simplified generator) */
            buf_write(buf, "/* yield */");
            if (node->yield_expr.value) {
                /* Simple yield - just return the value */
                buf_write(buf, "return ");
                gen_expr(cg, buf, node->yield_expr.value);
            } else {
                buf_write(buf, "return 0");
            }
            break;
        case NODE_AWAIT_EXPR:
            /* Await expression - synchronous implementation
             * In this simple approach, await just evaluates the expression directly.
             * This treats async/await as syntactic sugar for synchronous code.
             * For true async, a coroutine-based or callback implementation would be needed.
             */
            buf_write(buf, "/* await */ ");
            if (node->await_expr.expr) {
                gen_expr(cg, buf, node->await_expr.expr);
            } else {
                buf_write(buf, "0");
            }
            break;
        case NODE_GENERIC_INST: {
            /* Generic type instantiation: Box[int] - treat as constructor call */
            /* For now, generate a call to the type's constructor function */
            buf_printf(buf, "lp_%s_new()", node->generic_inst.base_name);
            break;
        }
        case NODE_TYPE_UNION:
            /* Type union expression - this shouldn't appear at runtime */
            /* Type unions are handled in type annotations, not expressions */
            buf_write(buf, "/* type union */ lp_val_null()");
            break;
        default:
            buf_write(buf, "/* unknown expr */");
            break;
    }
}

static void emit_native_int_array_init_expr(CodeGen *cg, Buffer *buf, AstNode *elem_expr, AstNode *count_expr) {
    int64_t literal = 0;

    if (get_int_literal_value(elem_expr, &literal) && literal == 0) {
        buf_write(buf, "lp_int_array_new(");
        gen_expr(cg, buf, count_expr);
        buf_write(buf, ")");
        return;
    }

    buf_write(buf, "lp_int_array_repeat(");
    gen_expr(cg, buf, elem_expr);
    buf_write(buf, ", ");
    gen_expr(cg, buf, count_expr);
    buf_write(buf, ")");
}

static void emit_native_int_array_access(CodeGen *cg, Buffer *buf, AstNode *obj_expr, AstNode *index_expr) {
    buf_write(buf, "((");
    gen_expr(cg, buf, obj_expr);
    buf_write(buf, ")->data[");
    gen_expr(cg, buf, index_expr);
    buf_write(buf, "])");
}

static void emit_native_numeric_minmax(CodeGen *cg, Buffer *buf, AstNode *node, int is_min, LpType result_type) {
    const char *cmp = is_min ? "<" : ">";
    const char *c_type = result_type == LP_FLOAT ? "double" : "int64_t";

    if (!node || node->type != NODE_CALL || node->call.args.count == 0) {
        buf_write(buf, result_type == LP_FLOAT ? "0.0" : "0");
        return;
    }

    if (node->call.args.count == 1) {
        LpType arg_type = infer_type(cg, node->call.args.items[0]);

        if (arg_type == LP_NATIVE_ARRAY_1D) {
            buf_write(buf, "({ LpIntArray* __lp_minmax_arr = ");
            gen_expr(cg, buf, node->call.args.items[0]);
            buf_write(buf, "; int64_t __lp_minmax_res = 0; ");
            buf_write(buf, "if (__lp_minmax_arr && __lp_minmax_arr->len > 0) { ");
            buf_write(buf, "__lp_minmax_res = __lp_minmax_arr->data[0]; ");
            buf_write(buf, "for (int64_t __lp_i = 1; __lp_i < __lp_minmax_arr->len; __lp_i++) { ");
            buf_write(buf, "int64_t __lp_v = __lp_minmax_arr->data[__lp_i]; ");
            buf_printf(buf, "if (__lp_v %s __lp_minmax_res) __lp_minmax_res = __lp_v; ", cmp);
            buf_write(buf, "} } __lp_minmax_res; })");
            return;
        }

        if (arg_type == LP_ARRAY) {
            buf_write(buf, is_min ? "lp_np_min(" : "lp_np_max(");
            gen_expr(cg, buf, node->call.args.items[0]);
            buf_write(buf, ")");
            return;
        }
    }

    buf_write(buf, "({ ");
    buf_printf(buf, "%s __lp_minmax_res = (%s)(", c_type, c_type);
    gen_expr(cg, buf, node->call.args.items[0]);
    buf_write(buf, "); ");

    for (int i = 1; i < node->call.args.count; i++) {
        buf_printf(buf, "%s __lp_minmax_arg_%d = (%s)(", c_type, i, c_type);
        gen_expr(cg, buf, node->call.args.items[i]);
        buf_write(buf, "); ");
        buf_printf(buf, "if (__lp_minmax_arg_%d %s __lp_minmax_res) __lp_minmax_res = __lp_minmax_arg_%d; ",
                   i, cmp, i);
    }

    buf_write(buf, "__lp_minmax_res; })");
}


static void emit_cast(CodeGen *cg, Buffer *buf, AstNode *expr, LpType target_type) {
    LpType expr_type = infer_type(cg, expr);
    if (expr_type == target_type) {
        gen_expr(cg, buf, expr);
        return;
    }
    if (target_type == LP_VAL) {
        emit_lp_val(cg, buf, expr);
    } else if (target_type == LP_INT && expr_type == LP_VAL) {
        buf_write(buf, "lp_int(");
        gen_expr(cg, buf, expr);
        buf_write(buf, ")");
    } else if (target_type == LP_FLOAT && expr_type == LP_VAL) {
        buf_write(buf, "lp_float(");
        gen_expr(cg, buf, expr);
        buf_write(buf, ")");
    } else if (target_type == LP_STRING && expr_type == LP_VAL) {
        buf_write(buf, "lp_str(");
        gen_expr(cg, buf, expr);
        buf_write(buf, ")");
    } else if (target_type == LP_BOOL && expr_type == LP_VAL) {
        buf_write(buf, "lp_bool(");
        gen_expr(cg, buf, expr);
        buf_write(buf, ")");
    } else {
        gen_expr(cg, buf, expr);
    }
}

static void gen_stmt(CodeGen *cg, Buffer *buf, AstNode *node, int indent) {
    if (!node) return;
    switch (node->type) {
        case NODE_ASSIGN: {
            Buffer *assign_buf = buf;
            int assign_indent = indent;
            LpType t = LP_UNKNOWN;
            AstNode *auto_array_elem = NULL;
            AstNode *auto_array_count = NULL;
            if (node->assign.type_ann)
                t = type_from_annotation(cg, node->assign.type_ann);
            else if (node->assign.value)
                t = infer_type(cg, node->assign.value);
            if ((t == LP_LIST || t == LP_UNKNOWN) && node->assign.value &&
                match_native_int_repeat_expr(cg, node->assign.value, &auto_array_elem, &auto_array_count)) {
                t = LP_NATIVE_ARRAY_1D;
            }
            if (t == LP_UNKNOWN) t = LP_INT;

            if (cg->scope->parent == NULL && node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                assign_buf = &cg->helpers;
                assign_indent = 0;
            }

            /* Check if it's an attribute assignment (e.g. obj.attr) */
            char *dot = strchr(node->assign.name, '.');
            if (dot) {
                *dot = '\0'; /* split into obj and attr */
                const char *obj_name = node->assign.name;
                const char *attr_name = dot + 1;
                Symbol *obj_sym = scope_lookup(cg->scope, obj_name);
                LpType target_type = LP_UNKNOWN; /* THÊM DÒNG NÀY */

                if (obj_sym && obj_sym->type == LP_OBJECT && obj_sym->class_name) {
                    char field_key[256];
                    Symbol *mem;
                    snprintf(field_key, sizeof(field_key), "%s.%s", obj_sym->class_name, attr_name);
                    mem = scope_lookup_field(cg->scope, field_key);
                    if (mem) {
                        target_type = mem->type;
                        if (!can_access_member(cg, mem)) {
                            codegen_set_error(cg,
                                "Cannot access %s member '%s' of class '%s'",
                                mem->access == TOK_PRIVATE ? "private" : "protected",
                                attr_name,
                                mem->owner_class ? mem->owner_class : obj_sym->class_name);
                        }
                    }
                }

                write_indent(assign_buf, assign_indent);
                buf_printf(assign_buf, "lp_%s->lp_%s = ", obj_name, attr_name);
                
                /* SỬA TẠI ĐÂY: Dùng emit_cast để tự động bọc bằng hàm ép kiểu C (như lp_int) */
                if (target_type != LP_UNKNOWN) {
                    emit_cast(cg, assign_buf, node->assign.value, target_type);
                } else {
                    gen_expr(cg, assign_buf, node->assign.value);
                }
                
                buf_write(assign_buf, ";\n");
                
                *dot = '.'; /* restore string */
            } else {
                Symbol *existing = scope_lookup(cg->scope, node->assign.name);
                if (!existing) {
                    const char *class_name = NULL;
                    if (t == LP_OBJECT && node->assign.value->type == NODE_CALL &&
                        node->assign.value->call.func->type == NODE_NAME) {
                        Symbol *cs = scope_lookup(cg->scope, node->assign.value->call.func->name_expr.name);
                        if (cs && cs->type == LP_CLASS) class_name = cs->name;
                    } else if (t == LP_OBJECT && node->assign.value->type == NODE_NAME) {
                        Symbol *os = scope_lookup(cg->scope, node->assign.value->name_expr.name);
                        if (os) class_name = os->class_name;
                    } else if (t == LP_OBJECT && node->assign.value->type == NODE_CALL &&
                               node->assign.value->call.func->type == NODE_ATTRIBUTE &&
                               node->assign.value->call.func->attribute.obj->type == NODE_NAME &&
                               strcmp(node->assign.value->call.func->attribute.obj->name_expr.name, "memory") == 0 &&
                               strcmp(node->assign.value->call.func->attribute.attr, "cast") == 0 &&
                               node->assign.value->call.args.count == 2) {
                        AstNode *type_arg = node->assign.value->call.args.items[1];
                        if (type_arg->type == NODE_NAME) {
                            class_name = type_arg->name_expr.name;
                        }
                    }
                    scope_define_obj(cg->scope, node->assign.name, t, class_name);
                    existing = scope_lookup(cg->scope, node->assign.name);
                    if (existing) existing->declared = 1;  /* Mark as declared */
                    
                    /* Handle native array allocation */
                    if (t == LP_NATIVE_ARRAY_1D || t == LP_NATIVE_ARRAY_2D) {
                        write_indent(assign_buf, assign_indent);
                        if (node->assign.type_ann && is_native_array_type(node->assign.type_ann)) {
                            char *sizes[4] = {NULL, NULL, NULL, NULL};
                            int dims = parse_native_array_dims(node->assign.type_ann, sizes);
                            
                            if (dims == 1) {
                                /* 1D array: LpIntArray* lp_arr = lp_int_array_new(size) */
                                char *c_size = convert_size_expr_to_c(sizes[0]);
                                buf_printf(assign_buf, "LpIntArray* lp_%s = lp_int_array_new(%s);", 
                                          node->assign.name, c_size ? c_size : "1");
                                if (c_size) free(c_size);
                            } else if (dims >= 2) {
                                /* 2D array: LpIntArray2D* lp_arr = lp_int_array2d_new(rows, cols) */
                                char *c_size0 = convert_size_expr_to_c(sizes[0]);
                                char *c_size1 = convert_size_expr_to_c(sizes[1]);
                                buf_printf(assign_buf, "LpIntArray2D* lp_%s = lp_int_array2d_new(%s, %s);", 
                                          node->assign.name, c_size0 ? c_size0 : "1", c_size1 ? c_size1 : "1");
                                if (c_size0) free(c_size0);
                                if (c_size1) free(c_size1);
                            }
                            
                            /* Free size strings */
                            for (int i = 0; i < dims; i++) {
                                if (sizes[i]) free(sizes[i]);
                            }
                        } else if (t == LP_NATIVE_ARRAY_1D && auto_array_elem && auto_array_count) {
                            buf_printf(assign_buf, "LpIntArray* lp_%s = ", node->assign.name);
                            emit_native_int_array_init_expr(cg, assign_buf, auto_array_elem, auto_array_count);
                            buf_write(assign_buf, ";");
                        } else if (t == LP_NATIVE_ARRAY_2D) {
                            buf_printf(assign_buf, "LpIntArray2D* lp_%s = NULL;", node->assign.name);
                        } else {
                            buf_printf(assign_buf, "LpIntArray* lp_%s = NULL;", node->assign.name);
                        }
                        buf_write(assign_buf, "\n");
                    } else if (assign_buf == &cg->helpers && node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                        Buffer lambda_expr_buf;
                        buf_init(&lambda_expr_buf);
                        gen_expr(cg, &lambda_expr_buf, node->assign.value);
                        write_indent(assign_buf, assign_indent);
                        buf_printf(assign_buf, "__auto_type lp_%s = ", node->assign.name);
                        buf_write(assign_buf, lambda_expr_buf.data ? lambda_expr_buf.data : "");
                        buf_write(assign_buf, ";\n");
                        buf_free(&lambda_expr_buf);
                    } else {
                        write_indent(assign_buf, assign_indent);
                        /* Lambda: use __auto_type for function pointer inference */
                        if (node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                            buf_printf(assign_buf, "__auto_type lp_%s", node->assign.name);
                        } else {
                            buf_printf(assign_buf, "%s lp_%s", lp_type_to_c_obj(t, class_name), node->assign.name);
                        }
                        if (node->assign.value) {
                            buf_write(assign_buf, " = ");
                            emit_cast(cg, assign_buf, node->assign.value, t);
                        }
                        buf_write(assign_buf, ";\n");
                    }
                } else {
                    /* Symbol exists - check if it needs declaration */
                    if (!existing->declared) {
                        /* First use: emit declaration */
                        existing->declared = 1;
                        const char *class_name = existing->class_name;
                        
                        /* Handle native array allocation */
                        if (existing->type == LP_NATIVE_ARRAY_1D || existing->type == LP_NATIVE_ARRAY_2D) {
                            write_indent(assign_buf, assign_indent);
                            if (node->assign.type_ann && is_native_array_type(node->assign.type_ann)) {
                                char *sizes[4] = {NULL, NULL, NULL, NULL};
                                int dims = parse_native_array_dims(node->assign.type_ann, sizes);
                                
                                if (dims == 1) {
                                    /* 1D array: LpIntArray* lp_arr = lp_int_array_new(size) */
                                    char *c_size = convert_size_expr_to_c(sizes[0]);
                                    buf_printf(assign_buf, "LpIntArray* lp_%s = lp_int_array_new(%s);", 
                                              node->assign.name, c_size ? c_size : "1");
                                    if (c_size) free(c_size);
                                } else if (dims >= 2) {
                                    /* 2D array: LpIntArray2D* lp_arr = lp_int_array2d_new(rows, cols) */
                                    char *c_size0 = convert_size_expr_to_c(sizes[0]);
                                    char *c_size1 = convert_size_expr_to_c(sizes[1]);
                                    buf_printf(assign_buf, "LpIntArray2D* lp_%s = lp_int_array2d_new(%s, %s);", 
                                              node->assign.name, c_size0 ? c_size0 : "1", c_size1 ? c_size1 : "1");
                                    if (c_size0) free(c_size0);
                                    if (c_size1) free(c_size1);
                                }
                                buf_write(assign_buf, "\n");
                                
                                /* Free size strings */
                                for (int i = 0; i < dims; i++) {
                                    if (sizes[i]) free(sizes[i]);
                                }
                            } else if (existing->type == LP_NATIVE_ARRAY_1D && auto_array_elem && auto_array_count) {
                                buf_printf(assign_buf, "LpIntArray* lp_%s = ", node->assign.name);
                                emit_native_int_array_init_expr(cg, assign_buf, auto_array_elem, auto_array_count);
                                buf_write(assign_buf, ";\n");
                            } else if (existing->type == LP_NATIVE_ARRAY_2D) {
                                buf_printf(assign_buf, "LpIntArray2D* lp_%s = NULL;\n", node->assign.name);
                            } else {
                                buf_printf(assign_buf, "LpIntArray* lp_%s = NULL;\n", node->assign.name);
                            }
                        } else if (assign_buf == &cg->helpers && node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                            Buffer lambda_expr_buf;
                            buf_init(&lambda_expr_buf);
                            gen_expr(cg, &lambda_expr_buf, node->assign.value);
                            write_indent(assign_buf, assign_indent);
                            buf_printf(assign_buf, "__auto_type lp_%s = ", node->assign.name);
                            buf_write(assign_buf, lambda_expr_buf.data ? lambda_expr_buf.data : "");
                            buf_write(assign_buf, ";\n");
                            buf_free(&lambda_expr_buf);
                        } else {
                            write_indent(assign_buf, assign_indent);
                            if (node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                                buf_printf(assign_buf, "__auto_type lp_%s", node->assign.name);
                            } else {
                                buf_printf(assign_buf, "%s lp_%s", lp_type_to_c_obj(existing->type, class_name), node->assign.name);
                            }
                            if (node->assign.value) {
                                buf_write(assign_buf, " = ");
                                emit_cast(cg, assign_buf, node->assign.value, existing->type);
                            }
                            buf_write(assign_buf, ";\n");
                        }
                    } else {
                        /* Already declared: just assign */
                        if (assign_buf == &cg->helpers && node->assign.value && node->assign.value->type == NODE_LAMBDA) {
                            Buffer lambda_expr_buf;
                            buf_init(&lambda_expr_buf);
                            gen_expr(cg, &lambda_expr_buf, node->assign.value);
                            write_indent(assign_buf, assign_indent);
                            buf_printf(assign_buf, "lp_%s = ", node->assign.name);
                            buf_write(assign_buf, lambda_expr_buf.data ? lambda_expr_buf.data : "");
                            buf_write(assign_buf, ";\n");
                            buf_free(&lambda_expr_buf);
                        } else {
                            write_indent(assign_buf, assign_indent);
                            if (existing->type == LP_NATIVE_ARRAY_1D && auto_array_elem && auto_array_count) {
                                buf_printf(assign_buf, "lp_%s = ", node->assign.name);
                                emit_native_int_array_init_expr(cg, assign_buf, auto_array_elem, auto_array_count);
                                buf_write(assign_buf, ";\n");
                            } else {
                                buf_printf(assign_buf, "lp_%s = ", node->assign.name);
                                emit_cast(cg, assign_buf, node->assign.value, existing->type);
                                buf_write(assign_buf, ";\n");
                            }
                        }
                    }
                }
            }
            break;
        }
        case NODE_AUG_ASSIGN: {
            const char *op;
            switch (node->aug_assign.op) {
                case TOK_PLUS_ASSIGN:  op = " += "; break;
                case TOK_MINUS_ASSIGN: op = " -= "; break;
                case TOK_STAR_ASSIGN:  op = " *= "; break;
                case TOK_SLASH_ASSIGN: op = " /= "; break;
                case TOK_BIT_AND_ASSIGN: op = " &= "; break;
                case TOK_BIT_OR_ASSIGN: op = " |= "; break;
                case TOK_BIT_XOR_ASSIGN: op = " ^= "; break;
                case TOK_LSHIFT_ASSIGN: op = " <<= "; break;
                case TOK_RSHIFT_ASSIGN: op = " >>= "; break;
                default: op = " += "; break;
            }
            char *dot = strchr(node->aug_assign.name, '.');
            if (dot) {
                *dot = '\0';
                write_indent(buf, indent);
                buf_printf(buf, "lp_%s->lp_%s%s", node->aug_assign.name, dot+1, op);
                gen_expr(cg, buf, node->aug_assign.value);
                buf_write(buf, ";\n");
                *dot = '.';
            } else {
                write_indent(buf, indent);
                buf_printf(buf, "lp_%s%s", node->aug_assign.name, op);
                gen_expr(cg, buf, node->aug_assign.value);
                buf_write(buf, ";\n");
            }
            break;
        }
        case NODE_SUBSCRIPT_ASSIGN: {
            write_indent(buf, indent);
            /* Check if object is a native array */
            LpType obj_type = infer_type(cg, node->subscript_assign.obj);
            if (obj_type == LP_NATIVE_ARRAY_1D) {
                /* Native 1D array: emit direct raw-array stores/updates */
                if (node->subscript_assign.op != TOK_ASSIGN) {
                    const char *op_str = "+=";
                    switch (node->subscript_assign.op) {
                        case TOK_PLUS_ASSIGN:  op_str = "+="; break;
                        case TOK_MINUS_ASSIGN: op_str = "-="; break;
                        case TOK_STAR_ASSIGN:  op_str = "*="; break;
                        case TOK_SLASH_ASSIGN: op_str = "/="; break;
                        default: break;
                    }
                    emit_native_int_array_access(cg, buf, node->subscript_assign.obj, node->subscript_assign.index);
                    buf_write(buf, " ");
                    buf_write(buf, op_str);
                    buf_write(buf, " ");
                    gen_expr(cg, buf, node->subscript_assign.value);
                    buf_write(buf, ";\n");
                } else {
                    /* Direct assignment: arr[i] = x */
                    emit_native_int_array_access(cg, buf, node->subscript_assign.obj, node->subscript_assign.index);
                    buf_write(buf, " = ");
                    gen_expr(cg, buf, node->subscript_assign.value);
                    buf_write(buf, ";\n");
                }
            } else if (obj_type == LP_NATIVE_ARRAY_2D) {
                /* Native 2D array: use lp_int_array2d_set */
                if (node->subscript_assign.op != TOK_ASSIGN) {
                    buf_write(buf, "lp_int_array2d_set(");
                    gen_expr(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript_assign.index);
                    buf_write(buf, ", 0, ");  /* Default col */
                    buf_printf(buf, "lp_int_array2d_get(");
                    gen_expr(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript_assign.index);
                    buf_write(buf, ", 0) ");
                    const char *op_str = "+";
                    switch (node->subscript_assign.op) {
                        case TOK_PLUS_ASSIGN:  op_str = "+"; break;
                        case TOK_MINUS_ASSIGN: op_str = "-"; break;
                        case TOK_STAR_ASSIGN:  op_str = "*"; break;
                        case TOK_SLASH_ASSIGN: op_str = "/"; break;
                        default: break;
                    }
                    buf_write(buf, op_str);
                    buf_write(buf, " ");
                    gen_expr(cg, buf, node->subscript_assign.value);
                    buf_write(buf, ");\n");
                } else {
                    buf_write(buf, "lp_int_array2d_set(");
                    gen_expr(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    gen_expr(cg, buf, node->subscript_assign.index);
                    buf_write(buf, ", 0, ");  /* Default col */
                    gen_expr(cg, buf, node->subscript_assign.value);
                    buf_write(buf, ");\n");
                }
            } else {
                /* Regular LpVal handling */
                /* Handle augmented assignment to a subscript (like arr[0] += 1) vs direct assignment (arr[0] = 1) */
                if (node->subscript_assign.op != TOK_ASSIGN) {
                    const char *op_func = "lp_val_add";
                    switch (node->subscript_assign.op) {
                        case TOK_PLUS_ASSIGN:  op_func = "lp_val_add"; break;
                        case TOK_MINUS_ASSIGN: op_func = "lp_val_sub"; break;
                        case TOK_STAR_ASSIGN:  op_func = "lp_val_mul"; break;
                        case TOK_SLASH_ASSIGN: op_func = "lp_val_div"; break;
                        default: break;
                    }
                    buf_write(buf, "lp_val_set_item(");
                    emit_lp_val(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->subscript_assign.index);
                    buf_printf(buf, ", %s(", op_func);
                    buf_write(buf, "lp_val_get_item(");
                    emit_lp_val(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->subscript_assign.index);
                    buf_write(buf, "), ");
                    emit_lp_val(cg, buf, node->subscript_assign.value);
                    buf_write(buf, "));\n");
                } else {
                    buf_write(buf, "lp_val_set_item(");
                    emit_lp_val(cg, buf, node->subscript_assign.obj);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->subscript_assign.index);
                    buf_write(buf, ", ");
                    emit_lp_val(cg, buf, node->subscript_assign.value);
                    buf_write(buf, ");\n");
                }
            }
            break;
        }
        case NODE_RETURN:
            write_indent(buf, indent);
            buf_write(buf, "return");
            if (node->return_stmt.value) {
                buf_write(buf, " ");
                gen_expr(cg, buf, node->return_stmt.value);
            }
            buf_write(buf, ";\n");
            break;
        case NODE_EXPR_STMT:
            write_indent(buf, indent);
            gen_expr(cg, buf, node->expr_stmt.expr);
            buf_write(buf, ";\n");
            break;
        case NODE_YIELD:
            write_indent(buf, indent);
            if (node->yield_expr.value) {
                buf_write(buf, "return ");
                gen_expr(cg, buf, node->yield_expr.value);
                buf_write(buf, ";\n");
            } else {
                buf_write(buf, "return 0;\n");
            }
            break;
        case NODE_IF:
            write_indent(buf, indent);
            buf_write(buf, "if (");
            gen_expr(cg, buf, node->if_stmt.cond);
            buf_write(buf, ") {\n");
            for (int i = 0; i < node->if_stmt.then_body.count; i++)
                gen_stmt(cg, buf, node->if_stmt.then_body.items[i], indent + 1);
            if (node->if_stmt.else_branch) {
                if (node->if_stmt.else_branch->type == NODE_IF) {
                    write_indent(buf, indent);
                    buf_write(buf, "} else ");
                    /* gen_stmt prints 'if (...)' */
                    gen_stmt(cg, buf, node->if_stmt.else_branch, indent);
                    return; /* skip closing brace, already handled */
                } else {
                    write_indent(buf, indent);
                    buf_write(buf, "} else {\n");
                    NodeList *stmts = &node->if_stmt.else_branch->program.stmts;
                    for (int i = 0; i < stmts->count; i++)
                        gen_stmt(cg, buf, stmts->items[i], indent + 1);
                }
            }
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            break;
        case NODE_FOR: {
            if (is_range_call(node->for_stmt.iter)) {
                NodeList *args = &node->for_stmt.iter->call.args;
                /* Define loop var */
                scope_define(cg->scope, node->for_stmt.var, LP_INT);
                write_indent(buf, indent);
                if (args->count == 1) {
                    buf_printf(buf, "for (int64_t lp_%s = 0; lp_%s < ",
                               node->for_stmt.var, node->for_stmt.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s++) {\n", node->for_stmt.var);
                } else if (args->count == 2) {
                    buf_printf(buf, "for (int64_t lp_%s = ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s < ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[1]);
                    buf_printf(buf, "; lp_%s++) {\n", node->for_stmt.var);
                    
                } else if (args->count >= 3) {
                    int64_t step_value = 0;

                    if (get_int_literal_value(args->items[2], &step_value) && step_value != 0) {
                        buf_printf(buf, "for (int64_t lp_%s = ", node->for_stmt.var);
                        gen_expr(cg, buf, args->items[0]);
                        buf_printf(buf, "; lp_%s %s ", node->for_stmt.var, step_value > 0 ? "<" : ">");
                        gen_expr(cg, buf, args->items[1]);
                        if (step_value == 1) {
                            buf_printf(buf, "; lp_%s++) {\n", node->for_stmt.var);
                        } else if (step_value == -1) {
                            buf_printf(buf, "; lp_%s--) {\n", node->for_stmt.var);
                        } else {
                            buf_printf(buf, "; lp_%s += %" PRId64 ") {\n", node->for_stmt.var, step_value);
                        }
                    } else {

                    /* Use a unique step variable name to avoid redefinition */
                    static int step_counter = 0;
                    int step_id = step_counter++;
                    buf_printf(buf, "int64_t __lp_step_%d = ", step_id);
                    gen_expr(cg, buf, args->items[2]);
                    buf_write(buf, ";\n");

                    /* Rẽ nhánh if/else dựa trên giá trị step */
                    write_indent(buf, indent);
                    buf_printf(buf, "if (__lp_step_%d > 0) {\n", step_id);
                    
                    write_indent(buf, indent + 1);
                    buf_printf(buf, "for (int64_t lp_%s = ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s < ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[1]);
                    buf_printf(buf, "; lp_%s += __lp_step_%d) {\n", node->for_stmt.var, step_id);
                    
                    /* Sinh thân vòng lặp cho nhánh dương */
                    for (int i = 0; i < node->for_stmt.body.count; i++)
                        gen_stmt(cg, buf, node->for_stmt.body.items[i], indent + 2);
                    
                    write_indent(buf, indent + 1);
                    buf_printf(buf, "}\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "} else {\n");
                    
                    write_indent(buf, indent + 1);
                    buf_printf(buf, "for (int64_t lp_%s = ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s > ", node->for_stmt.var); /* Dùng dấu > cho step âm */
                    gen_expr(cg, buf, args->items[1]);
                    buf_printf(buf, "; lp_%s += __lp_step_%d) {\n", node->for_stmt.var, step_id);
                    
                    /* Sinh thân vòng lặp cho nhánh âm */
                    for (int i = 0; i < node->for_stmt.body.count; i++)
                        gen_stmt(cg, buf, node->for_stmt.body.items[i], indent + 2);
                    
                    write_indent(buf, indent + 1);
                    buf_printf(buf, "}\n");
                    
                    /* Đóng brace cho nhánh else */
                    write_indent(buf, indent);
                    buf_printf(buf, "}\n");
                    
                        break;
                    }
                }
            } else {
                LpType iter_type = infer_type(cg, node->for_stmt.iter);
                if (iter_type == LP_NATIVE_ARRAY_1D) {
                    static int native_int_loop_counter = 0;
                    int cur_loop = native_int_loop_counter++;

                    write_indent(buf, indent);
                    buf_printf(buf, "LpIntArray* __lp_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->for_stmt.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_len_%d = __lp_iter_%d->len;\n", cur_loop, cur_loop);
                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_len_%d; __lp_i_%d++) {\n",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->for_stmt.var, LP_INT);
                    buf_printf(buf, "int64_t lp_%s = __lp_iter_%d->data[__lp_i_%d];\n",
                               node->for_stmt.var, cur_loop, cur_loop);
                } else if (iter_type == LP_ARRAY) {
                    static int native_float_loop_counter = 0;
                    int cur_loop = native_float_loop_counter++;

                    write_indent(buf, indent);
                    buf_printf(buf, "LpArray __lp_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->for_stmt.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_len_%d = __lp_iter_%d.len;\n", cur_loop, cur_loop);
                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_len_%d; __lp_i_%d++) {\n",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->for_stmt.var, LP_FLOAT);
                    buf_printf(buf, "double lp_%s = __lp_iter_%d.data[__lp_i_%d];\n",
                               node->for_stmt.var, cur_loop, cur_loop);
                } else if (iter_type == LP_VAL || iter_type == LP_LIST || iter_type == LP_PYOBJ /* ... */) {
                    static int generic_loop_counter = 0;
                    int cur_loop = generic_loop_counter++;
                    
                    write_indent(buf, indent);
                    buf_printf(buf, "LpVal __lp_iter_%d = ", cur_loop);
                    emit_lp_val(cg, buf, node->for_stmt.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_len_%d = lp_val_len(__lp_iter_%d);\n", cur_loop, cur_loop);
                    
                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_len_%d; __lp_i_%d++) {\n", 
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->for_stmt.var, LP_VAL);
                    buf_printf(buf, "LpVal lp_%s = lp_val_getitem_int(__lp_iter_%d, __lp_i_%d);\n", 
                               node->for_stmt.var, cur_loop, cur_loop);
                } else {
                    write_indent(buf, indent);
                    buf_write(buf, "/* TODO: generic for loop */\n");
                    write_indent(buf, indent);
                    buf_write(buf, "{\n");
                }
            }
            for (int i = 0; i < node->for_stmt.body.count; i++)
                gen_stmt(cg, buf, node->for_stmt.body.items[i], indent + 1);
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            break;
        }
        case NODE_WHILE:
            write_indent(buf, indent);
            buf_write(buf, "while (");
            gen_expr(cg, buf, node->while_stmt.cond);
            buf_write(buf, ") {\n");
            for (int i = 0; i < node->while_stmt.body.count; i++)
                gen_stmt(cg, buf, node->while_stmt.body.items[i], indent + 1);
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            break;
        case NODE_PARALLEL_FOR: {
            /* Generate OpenMP parallel for with settings from @settings decorator */
            cg->uses_parallel = 1;  /* Mark that we use OpenMP */
            
            /* Get settings from the parallel_for node */
            int num_threads = node->parallel_for.num_threads;
            const char *schedule = node->parallel_for.schedule ? node->parallel_for.schedule : "static";
            int64_t chunk_size = node->parallel_for.chunk_size;
            int device_type = node->parallel_for.device_type;
            int gpu_id = node->parallel_for.gpu_id;
            
            /* Handle GPU execution */
            if (device_type == 1) {  /* GPU */
                cg->uses_gpu = 1;
                write_indent(buf, indent);
                buf_write(buf, "/* GPU-accelerated parallel for (device=");
                buf_printf(buf, "%d, gpu_id=%d) */\n", device_type, gpu_id);
                write_indent(buf, indent);
                buf_printf(buf, "lp_gpu_select_device(%d);\n", gpu_id);
            } else if (device_type == 2) {  /* Auto - select best device */
                cg->uses_gpu = 1;
                write_indent(buf, indent);
                buf_write(buf, "/* Auto-select best device */\n");
                write_indent(buf, indent);
                buf_write(buf, "lp_gpu_select_by_type(LP_DEVICE_AUTO);\n");
            }
            
            /* Emit OpenMP parallel for pragma with settings */
            write_indent(buf, indent);
            if (num_threads > 0 && chunk_size > 0) {
                buf_printf(buf, "#pragma omp parallel for num_threads(%d) schedule(%s, %lld)\n",
                          num_threads, schedule, (long long)chunk_size);
            } else if (num_threads > 0) {
                buf_printf(buf, "#pragma omp parallel for num_threads(%d)\n", num_threads);
            } else if (chunk_size > 0) {
                buf_printf(buf, "#pragma omp parallel for schedule(%s, %lld)\n",
                          schedule, (long long)chunk_size);
            } else {
                buf_write(buf, "#pragma omp parallel for\n");
            }
            
            /* Reuse NODE_FOR logic using parallel_for fields */
            if (is_range_call(node->parallel_for.iter)) {
                NodeList *args = &node->parallel_for.iter->call.args;
                scope_define(cg->scope, node->parallel_for.var, LP_INT);
                write_indent(buf, indent);
                if (args->count == 1) {
                    buf_printf(buf, "for (int64_t lp_%s = 0; lp_%s < ",
                               node->parallel_for.var, node->parallel_for.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s++) {\n", node->parallel_for.var);
                } else if (args->count >= 2) {
                    buf_printf(buf, "for (int64_t lp_%s = ", node->parallel_for.var);
                    gen_expr(cg, buf, args->items[0]);

                    /* If step is negative, use > instead of < */
                    if (args->count >= 3 && args->items[2]->type == NODE_UNARY_OP && args->items[2]->unary_op.op == TOK_MINUS && args->items[2]->unary_op.operand->type == NODE_INT_LIT) {
                        buf_printf(buf, "; lp_%s > ", node->parallel_for.var);
                    } else if (args->count >= 3 && args->items[2]->type == NODE_INT_LIT && args->items[2]->int_lit.value < 0) {
                        buf_printf(buf, "; lp_%s > ", node->parallel_for.var);
                    } else if (args->count >= 3) {
                        buf_printf(buf, "; lp_%s < ", node->parallel_for.var);
                    } else {
                        buf_printf(buf, "; lp_%s < ", node->parallel_for.var);
                    }

                    gen_expr(cg, buf, args->items[1]);
                    if (args->count >= 3) {
                        buf_printf(buf, "; lp_%s += ", node->parallel_for.var);
                        gen_expr(cg, buf, args->items[2]);
                    } else {
                        buf_printf(buf, "; lp_%s++", node->parallel_for.var);
                    }
                    buf_write(buf, ") {\n");
                }
            } else {
                LpType iter_type = infer_type(cg, node->parallel_for.iter);
                if (iter_type == LP_NATIVE_ARRAY_1D) {
                    static int native_int_parallel_loop_counter = 0;
                    int cur_loop = native_int_parallel_loop_counter++;
                    write_indent(buf, indent);
                    buf_printf(buf, "LpIntArray* __lp_p_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->parallel_for.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_p_len_%d = __lp_p_iter_%d->len;\n", cur_loop, cur_loop);
                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_p_len_%d; __lp_i_%d++) {\n",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->parallel_for.var, LP_INT);
                    buf_printf(buf, "int64_t lp_%s = __lp_p_iter_%d->data[__lp_i_%d];\n",
                               node->parallel_for.var, cur_loop, cur_loop);
                } else if (iter_type == LP_ARRAY) {
                    static int native_float_parallel_loop_counter = 0;
                    int cur_loop = native_float_parallel_loop_counter++;
                    write_indent(buf, indent);
                    buf_printf(buf, "LpArray __lp_p_iter_%d = ", cur_loop);
                    gen_expr(cg, buf, node->parallel_for.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_p_len_%d = __lp_p_iter_%d.len;\n", cur_loop, cur_loop);
                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_p_len_%d; __lp_i_%d++) {\n",
                               cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->parallel_for.var, LP_FLOAT);
                    buf_printf(buf, "double lp_%s = __lp_p_iter_%d.data[__lp_i_%d];\n",
                               node->parallel_for.var, cur_loop, cur_loop);
                } else if (iter_type == LP_VAL || iter_type == LP_LIST || iter_type == LP_PYOBJ || iter_type == LP_ARRAY || iter_type == LP_STR_ARRAY || iter_type == LP_DICT || iter_type == LP_SET || iter_type == LP_TUPLE) {
                    /* Create a unique loop index variable for generic loops to avoid shadowing */
                    static int generic_parallel_loop_counter = 0;
                    int cur_loop = generic_parallel_loop_counter++;
                    write_indent(buf, indent);
                    buf_printf(buf, "LpVal __lp_p_iter_%d = ", cur_loop);
                    emit_lp_val(cg, buf, node->parallel_for.iter);
                    buf_write(buf, ";\n");
                    write_indent(buf, indent);
                    buf_printf(buf, "int64_t __lp_p_len_%d = lp_val_len(__lp_p_iter_%d);\n", cur_loop, cur_loop);

                    write_indent(buf, indent);
                    buf_printf(buf, "for (int64_t __lp_i_%d = 0; __lp_i_%d < __lp_p_len_%d; __lp_i_%d++) {\n", cur_loop, cur_loop, cur_loop, cur_loop);
                    write_indent(buf, indent + 1);
                    scope_define(cg->scope, node->parallel_for.var, LP_VAL);
                    /* For OpenMP threads, the loop variable must be explicitly declared locally to be private */
                    buf_printf(buf, "LpVal lp_%s = lp_val_getitem_int(__lp_p_iter_%d, __lp_i_%d);\n", node->parallel_for.var, cur_loop, cur_loop);
                } else {
                    write_indent(buf, indent);
                    buf_write(buf, "#pragma omp parallel\n"); 
                    write_indent(buf, indent);
                    buf_write(buf, "/* TODO: generic parallel for loop */\n");
                    write_indent(buf, indent);
                    buf_write(buf, "{\n");
                }
            }
            for (int i = 0; i < node->parallel_for.body.count; i++)
                gen_stmt(cg, buf, node->parallel_for.body.items[i], indent + 1);
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            
            /* Sync GPU if used */
            if (device_type == 1 || device_type == 2) {
                write_indent(buf, indent);
                buf_write(buf, "lp_gpu_sync();\n");
            }
            break;
        }
        case NODE_WITH: {
            /* Declare file variable OUTSIDE the block so it stays in scope */
            if (node->with_stmt.alias) {
                Symbol *existing = scope_lookup(cg->scope, node->with_stmt.alias);
                if (!existing) {
                    scope_define(cg->scope, node->with_stmt.alias, LP_FILE);
                    write_indent(buf, indent);
                    buf_printf(buf, "LpFile* lp_%s = ", node->with_stmt.alias);
                } else {
                    write_indent(buf, indent);
                    buf_printf(buf, "lp_%s = ", node->with_stmt.alias);
                }
                gen_expr(cg, buf, node->with_stmt.expr);
                buf_write(buf, ";\n");
            } else {
                write_indent(buf, indent);
                buf_write(buf, "LpFile* __lp_with_tmp = ");
                gen_expr(cg, buf, node->with_stmt.expr);
                buf_write(buf, ";\n");
            }
            /* Body statements at same indent level (no extra block scope) */
            for (int i = 0; i < node->with_stmt.body.count; i++)
                gen_stmt(cg, buf, node->with_stmt.body.items[i], indent);
            
            /* finally: close the file implicitly */
            write_indent(buf, indent);
            if (node->with_stmt.alias) {
                buf_printf(buf, "lp_io_close(lp_%s);\n", node->with_stmt.alias);
            } else {
                buf_write(buf, "lp_io_close(__lp_with_tmp);\n");
            }
            break;
        }
        case NODE_TRY: {
            write_indent(buf, indent);
            buf_write(buf, "{\n");
            write_indent(buf, indent + 1);
            buf_write(buf, "int __lp_exp_caught_local = 0;\n");
            write_indent(buf, indent + 1);
            buf_write(buf, "if (setjmp(lp_exp_env[lp_exp_depth++]) == 0) {\n");
            for (int i = 0; i < node->try_stmt.body.count; i++)
                gen_stmt(cg, buf, node->try_stmt.body.items[i], indent + 2);
            write_indent(buf, indent + 2);
            buf_write(buf, "lp_exp_depth--;\n");
            write_indent(buf, indent + 1);
            buf_write(buf, "}");
            
            if (node->try_stmt.except_body.count > 0 || node->try_stmt.except_type) {
                buf_write(buf, " else if (lp_current_exp.type != 0) {\n");
                write_indent(buf, indent + 2);
                buf_write(buf, "lp_exp_depth--;\n");
                write_indent(buf, indent + 2);
                buf_write(buf, "__lp_exp_caught_local = 1;\n");
                write_indent(buf, indent + 2);
                buf_write(buf, "lp_current_exp.type = 0;\n");
                for (int i = 0; i < node->try_stmt.except_body.count; i++)
                    gen_stmt(cg, buf, node->try_stmt.except_body.items[i], indent + 2);
                write_indent(buf, indent + 1);
                buf_write(buf, "}\n");
            } else {
                buf_write(buf, " else {\n");
                write_indent(buf, indent + 2);
                buf_write(buf, "lp_exp_depth--;\n");
                write_indent(buf, indent + 1);
                buf_write(buf, "}\n");
            }
            
            if (node->try_stmt.finally_body.count > 0) {
                write_indent(buf, indent + 1);
                buf_write(buf, "/* finally */ {\n");
                for (int i = 0; i < node->try_stmt.finally_body.count; i++)
                    gen_stmt(cg, buf, node->try_stmt.finally_body.items[i], indent + 2);
                write_indent(buf, indent + 1);
                buf_write(buf, "}\n");
            }
            
            write_indent(buf, indent + 1);
            buf_write(buf, "if (!__lp_exp_caught_local && lp_current_exp.type != 0) {\n");
            write_indent(buf, indent + 2);
            buf_write(buf, "lp_raise(lp_current_exp.type, lp_current_exp.message);\n");
            write_indent(buf, indent + 1);
            buf_write(buf, "}\n");
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            break;
        }
        case NODE_RAISE: {
            write_indent(buf, indent);
            if (node->raise_stmt.exc) {
                buf_write(buf, "lp_raise_str(");
                gen_expr(cg, buf, node->raise_stmt.exc);
                buf_write(buf, ");\n");
            } else {
                buf_write(buf, "lp_raise(1, \"Exception raised\");\n");
            }
            break;
        }
        case NODE_CONST_DECL: {
            LpType t = infer_type(cg, node->const_decl.value);
            if (t == LP_UNKNOWN) t = LP_INT;
            scope_define(cg->scope, node->const_decl.name, t);
            write_indent(buf, indent);
            buf_printf(buf, "const %s lp_%s = ", lp_type_to_c(t), node->const_decl.name);
            gen_expr(cg, buf, node->const_decl.value);
            buf_write(buf, ";\n");
            break;
        }
        case NODE_PASS:
            write_indent(buf, indent);
            buf_write(buf, "/* pass */\n");
            break;
        case NODE_BREAK:
            write_indent(buf, indent);
            buf_write(buf, "break;\n");
            break;
        case NODE_CONTINUE:
            write_indent(buf, indent);
            buf_write(buf, "continue;\n");
            break;
        case NODE_MATCH: {
            /* Generate match statement as a chain of if-else */
            write_indent(buf, indent);
            
            /* Store match value in a temporary variable */
            buf_write(buf, "{\n");
            write_indent(buf, indent + 1);
            buf_write(buf, "__auto_type _match_val = ");
            gen_expr(cg, buf, node->match_stmt.value);
            buf_write(buf, ";\n");
            
            int first_case = 1;
            for (int i = 0; i < node->match_stmt.cases.count; i++) {
                AstNode *case_node = node->match_stmt.cases.items[i];
                
                write_indent(buf, indent + 1);
                if (!first_case) {
                    buf_write(buf, "} else ");
                }
                
                if (case_node->match_case.is_wildcard) {
                    /* Wildcard case: matches everything */
                    buf_write(buf, "{ /* default case */\n");
                } else {
                    /* Pattern match: compare value */
                    buf_write(buf, "if (");
                    if (case_node->match_case.pattern) {
                        LpType pattern_type = infer_type(cg, case_node->match_case.pattern);
                        if (pattern_type == LP_STRING) {
                            buf_write(buf, "strcmp(_match_val, ");
                            gen_expr(cg, buf, case_node->match_case.pattern);
                            buf_write(buf, ") == 0");
                        } else {
                            buf_write(buf, "_match_val == ");
                            gen_expr(cg, buf, case_node->match_case.pattern);
                        }
                    }
                    
                    /* Add guard if present */
                    if (case_node->match_case.guard) {
                        buf_write(buf, " && (");
                        gen_expr(cg, buf, case_node->match_case.guard);
                        buf_write(buf, ")");
                    }
                    buf_write(buf, ") {\n");
                }
                
                /* Generate case body */
                for (int j = 0; j < case_node->match_case.body.count; j++) {
                    gen_stmt(cg, buf, case_node->match_case.body.items[j], indent + 2);
                }
                
                first_case = 0;
            }
            
            /* Close the last if block */
            if (node->match_stmt.cases.count > 0) {
                write_indent(buf, indent + 1);
                buf_write(buf, "}\n");
            }
            
            write_indent(buf, indent);
            buf_write(buf, "}\n");
            break;
        }
        default:
            write_indent(buf, indent);
            buf_write(buf, "/* unknown statement */\n");
            break;
    }
}

/* Infer function return type by scanning body for return statements */

static void scan_declarations(CodeGen *cg, NodeList *body) {
    if (!body) return;
    for (int i = 0; i < body->count; i++) {
        AstNode *stmt = body->items[i];
        if (stmt->type == NODE_ASSIGN) {
            LpType t = LP_UNKNOWN;
            AstNode *auto_array_elem = NULL;
            AstNode *auto_array_count = NULL;
            if (stmt->assign.type_ann)
                t = type_from_annotation(cg, stmt->assign.type_ann);
            else if (stmt->assign.value)
                t = infer_type(cg, stmt->assign.value);
            if ((t == LP_LIST || t == LP_UNKNOWN) && stmt->assign.value &&
                match_native_int_repeat_expr(cg, stmt->assign.value, &auto_array_elem, &auto_array_count)) {
                t = LP_NATIVE_ARRAY_1D;
            }
            if (t == LP_UNKNOWN) t = LP_INT;

            Symbol *existing = scope_lookup(cg->scope, stmt->assign.name);
            if (!existing) {
                const char *class_name = NULL;
                if (t == LP_OBJECT && stmt->assign.value && stmt->assign.value->type == NODE_CALL &&
                    stmt->assign.value->call.func->type == NODE_NAME) {
                    Symbol *cs = scope_lookup(cg->scope, stmt->assign.value->call.func->name_expr.name);
                    if (cs && cs->type == LP_CLASS) class_name = cs->name;
                }
                scope_define_obj(cg->scope, stmt->assign.name, t, class_name);
            }
        } else if (stmt->type == NODE_IF) {
            scan_declarations(cg, &stmt->if_stmt.then_body);
            if (stmt->if_stmt.else_branch && stmt->if_stmt.else_branch->type == NODE_IF) {
                // To simplify, we can just scan the else body directly if we cast it
                // Actually, if it's a NODE_IF, else_branch is just an AstNode*.
                // We'd have to wrap it in a NodeList. Let's just ignore nested for now,
                // most top-level variables are declared at the top of the function.
            }
        } else if (stmt->type == NODE_FOR) {
            scan_declarations(cg, &stmt->for_stmt.body);
        } else if (stmt->type == NODE_WHILE) {
            scan_declarations(cg, &stmt->while_stmt.body);
        }
    }
}

/* Emit declarations for all local variables in scope (hoisting) */
static void emit_local_declarations(CodeGen *cg, Buffer *buf, Scope *scope, int indent) {
    if (!scope) return;
    /* Only emit for the current scope level, not parent scopes */
    for (int i = 0; i < scope->count; i++) {
        Symbol *sym = &scope->symbols[i];
        /* Skip functions, classes, and native arrays (need special handling) */
        if (!sym->is_function && sym->type != LP_CLASS &&
            sym->type != LP_NATIVE_ARRAY_1D && sym->type != LP_NATIVE_ARRAY_2D) {
            /* Emit declaration without initialization */
            write_indent(buf, indent);
            const char *class_name = sym->class_name;
            buf_printf(buf, "%s lp_%s;\n", lp_type_to_c_obj(sym->type, class_name), sym->name);
            sym->declared = 1;  /* Mark as declared so gen_stmt doesn't emit again */
        }
    }
}

static LpType infer_return_type(CodeGen *cg, NodeList *body) {
    for (int i = 0; i < body->count; i++) {
        AstNode *stmt = body->items[i];
        if (stmt->type == NODE_RETURN && stmt->return_stmt.value) {
            LpType t = infer_type(cg, stmt->return_stmt.value);
            if (t != LP_UNKNOWN) return t;
        }
        /* Check inside if-else blocks too */
        if (stmt->type == NODE_IF) {
            LpType t = infer_return_type(cg, &stmt->if_stmt.then_body);
            if (t != LP_UNKNOWN) return t;
        }
    }
    return LP_UNKNOWN;
}


static LpType infer_function_return_type(CodeGen *cg, AstNode *node) {
    LpType ret;

    if (node->func_def.ret_type) {
        ret = type_from_annotation(cg, node->func_def.ret_type);
    } else {
        Scope *temp_scope = scope_new(cg->scope);
        cg->scope = temp_scope;

        for (int i = 0; i < node->func_def.params.count; i++) {
            Param *p = &node->func_def.params.items[i];
            LpType pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
            scope_define(cg->scope, p->name, pt);
        }

        scan_declarations(cg, &node->func_def.body);
        ret = infer_return_type(cg, &node->func_def.body);
        
        cg->scope = temp_scope->parent;
        
        scope_free(temp_scope);
        
        if (ret == LP_UNKNOWN) ret = LP_VOID;
    }

    return ret;
}


static void populate_function_symbol(CodeGen *cg, Symbol *sym, AstNode *node, const char *owner_class) {
    if (!sym || !node || node->type != NODE_FUNC_DEF) return;

    sym->type = infer_function_return_type(cg, node);
    sym->is_function = 1;
    sym->is_variadic = 0;
    sym->has_kwargs = 0;
    sym->num_params = node->func_def.params.count;
    sym->first_param_type = LP_UNKNOWN;
    if (sym->first_param_class_name) {
        free(sym->first_param_class_name);
        sym->first_param_class_name = NULL;
    }
    sym->access = node->func_def.access;
    if (sym->owner_class) {
        free(sym->owner_class);
        sym->owner_class = NULL;
    }
    sym->is_method = owner_class != NULL;
    if (owner_class) {
        sym->owner_class = strdup(owner_class);
    }

    for (int i = 0; i < node->func_def.params.count; i++) {
        Param *p = &node->func_def.params.items[i];
        LpType pt;

        if (p->is_vararg) {
            sym->is_variadic = 1;
            continue;
        }
        if (p->is_kwarg) {
            sym->has_kwargs = 1;
            continue;
        }

        if (owner_class && strcmp(p->name, "self") == 0) {
            pt = LP_OBJECT;
        } else {
            pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
        }

        if (i < 16) { sym->param_types[i] = pt; }
        if (sym->first_param_type == LP_UNKNOWN) {
            sym->first_param_type = pt;
            if (pt == LP_OBJECT) {
                const char *class_name = owner_class && strcmp(p->name, "self") == 0 ? owner_class : p->type_ann;
                if (class_name) sym->first_param_class_name = strdup(class_name);
            }
        }
    }
}

static void gen_func_def(CodeGen *cg, AstNode *node, const char *class_name) {
    Symbol *func_sym = scope_lookup(cg->scope, node->func_def.name);
    LpType ret;

    if (!func_sym) {
        func_sym = scope_define(cg->scope, node->func_def.name, LP_VOID);
    }
    populate_function_symbol(cg, func_sym, node, class_name);
    ret = func_sym ? func_sym->type : infer_function_return_type(cg, node);

    buf_printf(&cg->header, "%s lp_%s(", lp_type_to_c(ret), node->func_def.name);
    for (int i = 0; i < node->func_def.params.count; i++) {
        Param *p = &node->func_def.params.items[i];
        if (i > 0) buf_write(&cg->header, ", ");
        if (class_name && strcmp(p->name, "self") == 0) {
            buf_printf(&cg->header, "LpObj_%s* lp_%s", class_name, p->name);
        } else if (p->is_vararg) {
            buf_printf(&cg->header, "LpArray lp_%s", p->name);
        } else if (p->is_kwarg) {
            buf_printf(&cg->header, "LpDict* lp_%s", p->name);
        } else {
            LpType pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
            if (pt == LP_OBJECT) {
                buf_printf(&cg->header, "LpObj_%s* lp_%s", p->type_ann, p->name);
            } else {
                buf_printf(&cg->header, "%s lp_%s", lp_type_to_c(pt), p->name);
            }
        }
    }
    if (node->func_def.params.count == 0) {
        buf_write(&cg->header, "void");
    }
    buf_write(&cg->header, ");\n");

    /* Function scope */
    Scope *func_scope = scope_new(cg->scope);
    cg->scope = func_scope;

    /* Signature */
    buf_write(&cg->funcs, "__attribute__((hot, optimize(\"O3,unroll-loops,strict-aliasing,omit-frame-pointer\"))) ");
    buf_printf(&cg->funcs, "%s lp_%s(", lp_type_to_c(ret), node->func_def.name);
    for (int i = 0; i < node->func_def.params.count; i++) {
        if (i > 0) buf_write(&cg->funcs, ", ");
        Param *p = &node->func_def.params.items[i];
        if (class_name && strcmp(p->name, "self") == 0) {
            buf_printf(&cg->funcs, "LpObj_%s* lp_%s", class_name, p->name);
            scope_define_obj(func_scope, p->name, LP_OBJECT, class_name);
        } else if (p->is_vararg) {
            buf_printf(&cg->funcs, "LpArray lp_%s", p->name);
            scope_define(func_scope, p->name, LP_ARRAY);
        } else if (p->is_kwarg) {
            buf_printf(&cg->funcs, "LpDict* lp_%s", p->name);
            scope_define(func_scope, p->name, LP_DICT);
        } else {
            LpType pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
            if (pt == LP_OBJECT) {
                buf_printf(&cg->funcs, "LpObj_%s* lp_%s", p->type_ann, p->name);
                scope_define_obj(func_scope, p->name, pt, p->type_ann);
            } else {
                buf_printf(&cg->funcs, "%s lp_%s", lp_type_to_c(pt), p->name);
                scope_define(func_scope, p->name, pt);
            }
        }
    }
    buf_write(&cg->funcs, ") {\n");

    /* Scan body for local variable declarations and emit them at the top */
    scan_declarations(cg, &node->func_def.body);
    emit_local_declarations(cg, &cg->funcs, func_scope, 1);

    /* Process decorators for @settings (parallel/GPU configuration) */
    for (int d = 0; d < node->func_def.decorators.count; d++) {
        AstNode *decorator = node->func_def.decorators.items[d];
        if (decorator->type == NODE_SETTINGS && decorator->settings.enabled) {
            /* Mark that we use parallel/GPU features */
            if (decorator->settings.device_type > 0) {
                cg->uses_gpu = 1;
            }
            cg->uses_parallel = 1;
            
            buf_write(&cg->funcs, "    /* === PARALLEL/GPU SETTINGS === */\n");
            
            /* Configure parallel settings */
            if (decorator->settings.num_threads > 0) {
                buf_printf(&cg->funcs, "    lp_parallel_set_threads(%d);\n", 
                          decorator->settings.num_threads);
            }
            
            /* Configure GPU device */
            if (decorator->settings.device_type == 1) {  /* GPU */
                buf_printf(&cg->funcs, "    lp_gpu_select_device(%d);\n", decorator->settings.gpu_id);
                if (decorator->settings.unified_memory) {
                    buf_write(&cg->funcs, "    lp_gpu_configure(-1, 1, 0);\n");
                }
            } else if (decorator->settings.device_type == 2) {  /* Auto */
                buf_write(&cg->funcs, "    lp_gpu_select_by_type(LP_DEVICE_AUTO);\n");
            }
            
            /* Set schedule if provided */
            if (decorator->settings.schedule) {
                buf_printf(&cg->funcs, "    lp_parallel_configure(%d, \"%s\", %lld, 0, 0);\n",
                          decorator->settings.num_threads,
                          decorator->settings.schedule,
                          (long long)decorator->settings.chunk_size);
            }
            
            buf_write(&cg->funcs, "    /* === END PARALLEL/GPU SETTINGS === */\n\n");
            break;  /* Only process first settings decorator */
        }
    }

    /* Process decorators for security settings */
    for (int d = 0; d < node->func_def.decorators.count; d++) {
        AstNode *decorator = node->func_def.decorators.items[d];
        if (decorator->type == NODE_SECURITY && decorator->security.enabled) {
            /* Mark that we use security features */
            cg->uses_security = 1;
            
            /* Generate security context initialization */
            buf_write(&cg->funcs, "    /* === SECURITY CHECKS === */\n");
            
            /* Rate limiting - uses global state for persistence */
            if (decorator->security.rate_limit > 0) {
                buf_printf(&cg->funcs, "    if (!lp_check_func_rate_limit(\"%s\", %d)) {\n", 
                          node->func_def.name, decorator->security.rate_limit);
                buf_printf(&cg->funcs, "        fprintf(stderr, \"[SECURITY] Rate limit exceeded for %s\\n\");\n", 
                          node->func_def.name);
                buf_write(&cg->funcs, "        return 0;\n");
                buf_write(&cg->funcs, "    }\n");
            }
            
            /* Authentication check - uses global context */
            if (decorator->security.require_auth) {
                buf_write(&cg->funcs, "    if (!lp_is_authenticated()) {\n");
                buf_printf(&cg->funcs, "        fprintf(stderr, \"[SECURITY] Authentication required for %s\\n\");\n",
                          node->func_def.name);
                buf_write(&cg->funcs, "        return 0;\n");
                buf_write(&cg->funcs, "    }\n");
            }
            
            /* Access level check */
            int required_level = decorator->security.access_level;
            if (required_level > 0) {
                buf_write(&cg->funcs, "    if (lp_get_access_level() < ");
                buf_printf(&cg->funcs, "%d", required_level);
                buf_write(&cg->funcs, ") {\n");
                buf_printf(&cg->funcs, "        fprintf(stderr, \"[SECURITY] Access denied for %s (insufficient privileges)\\n\");\n",
                          node->func_def.name);
                buf_write(&cg->funcs, "        return 0;\n");
                buf_write(&cg->funcs, "    }\n");
            }
            
            /* Readonly mode check */
            if (decorator->security.readonly) {
                buf_write(&cg->funcs, "    /* Readonly mode: write operations will be blocked */\n");
            }
            
            buf_write(&cg->funcs, "    /* === END SECURITY CHECKS === */\n\n");
            break;  /* Only process first security decorator */
        }
    }

    /* Body */
    for (int i = 0; i < node->func_def.body.count; i++)
        gen_stmt(cg, &cg->funcs, node->func_def.body.items[i], 1);

    buf_write(&cg->funcs, "}\n\n");

    /* Restore scope */
    cg->scope = func_scope->parent;
    scope_free(func_scope);
}

/* Generate async function definition - simplified synchronous implementation
 * Async functions are generated as regular C functions.
 * The 'async' keyword serves as syntactic sugar for documentation/annotation purposes.
 * 'await' expressions inside async functions simply evaluate synchronously.
 * 
 * Future enhancement: Could generate coroutine-based code with:
 * - State machine for suspension points
 * - Promise/Future return types
 * - Event loop integration
 */
static void gen_async_func_def(CodeGen *cg, AstNode *node, const char *class_name) {
    Symbol *func_sym = scope_lookup(cg->scope, node->async_def.name);
    LpType ret;

    if (!func_sym) {
        func_sym = scope_define(cg->scope, node->async_def.name, LP_VOID);
    }
    
    /* Populate function symbol similar to regular functions */
    if (func_sym) {
        /* Infer return type from body or annotation */
        if (node->async_def.ret_type) {
            func_sym->type = type_from_annotation(cg, node->async_def.ret_type);
        } else {
            Scope *temp_scope = scope_new(cg->scope);
            cg->scope = temp_scope;
            for (int i = 0; i < node->async_def.params.count; i++) {
                Param *p = &node->async_def.params.items[i];
                LpType pt = type_from_annotation(cg, p->type_ann);
                if (pt == LP_UNKNOWN) pt = LP_VAL;
                scope_define(cg->scope, p->name, pt);
            }
            func_sym->type = infer_return_type(cg, &node->async_def.body);
            cg->scope = temp_scope->parent;
            scope_free(temp_scope);
            if (func_sym->type == LP_UNKNOWN) func_sym->type = LP_VOID;
        }
        func_sym->is_function = 1;
        func_sym->num_params = node->async_def.params.count;
        func_sym->access = node->async_def.access;
    }
    
    ret = func_sym ? func_sym->type : LP_VOID;

    /* Generate function header declaration */
    buf_printf(&cg->header, "/* async */ %s lp_%s(", lp_type_to_c(ret), node->async_def.name);
    for (int i = 0; i < node->async_def.params.count; i++) {
        Param *p = &node->async_def.params.items[i];
        if (i > 0) buf_write(&cg->header, ", ");
        if (class_name && strcmp(p->name, "self") == 0) {
            buf_printf(&cg->header, "LpObj_%s* lp_%s", class_name, p->name);
        } else if (p->is_vararg) {
            buf_printf(&cg->header, "LpArray lp_%s", p->name);
        } else if (p->is_kwarg) {
            buf_printf(&cg->header, "LpDict* lp_%s", p->name);
        } else {
            LpType pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
            if (pt == LP_OBJECT) {
                buf_printf(&cg->header, "LpObj_%s* lp_%s", p->type_ann, p->name);
            } else {
                buf_printf(&cg->header, "%s lp_%s", lp_type_to_c(pt), p->name);
            }
        }
    }
    if (node->async_def.params.count == 0) {
        buf_write(&cg->header, "void");
    }
    buf_write(&cg->header, ");\n");

    /* Function scope */
    Scope *func_scope = scope_new(cg->scope);
    cg->scope = func_scope;

    /* Generate function definition with async comment */
    buf_write(&cg->funcs, "/* async function - synchronous implementation */\n");
    buf_printf(&cg->funcs, "%s lp_%s(", lp_type_to_c(ret), node->async_def.name);
    for (int i = 0; i < node->async_def.params.count; i++) {
        if (i > 0) buf_write(&cg->funcs, ", ");
        Param *p = &node->async_def.params.items[i];
        if (class_name && strcmp(p->name, "self") == 0) {
            buf_printf(&cg->funcs, "LpObj_%s* lp_%s", class_name, p->name);
            scope_define_obj(func_scope, p->name, LP_OBJECT, class_name);
        } else if (p->is_vararg) {
            buf_printf(&cg->funcs, "LpArray lp_%s", p->name);
            scope_define(func_scope, p->name, LP_ARRAY);
        } else if (p->is_kwarg) {
            buf_printf(&cg->funcs, "LpDict* lp_%s", p->name);
            scope_define(func_scope, p->name, LP_DICT);
        } else {
            LpType pt = type_from_annotation(cg, p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_VAL;
            if (pt == LP_OBJECT) {
                buf_printf(&cg->funcs, "LpObj_%s* lp_%s", p->type_ann, p->name);
                scope_define_obj(func_scope, p->name, pt, p->type_ann);
            } else {
                buf_printf(&cg->funcs, "%s lp_%s", lp_type_to_c(pt), p->name);
                scope_define(func_scope, p->name, pt);
            }
        }
    }
    buf_write(&cg->funcs, ") {\n");

    /* Body */
    for (int i = 0; i < node->async_def.body.count; i++)
        gen_stmt(cg, &cg->funcs, node->async_def.body.items[i], 1);

    buf_write(&cg->funcs, "}\n\n");

    /* Restore scope */
    cg->scope = func_scope->parent;
    scope_free(func_scope);
}

static void gen_class_def(CodeGen *cg, AstNode *node) {
    const char *class_name = node->class_def.name;
    const char *base_class = node->class_def.base_class;
    
    Symbol *class_sym = scope_define(cg->scope, class_name, LP_CLASS);
    if (class_sym && base_class) {
        class_sym->base_class = strdup(base_class);
    }

    Buffer struct_def;
    buf_init(&struct_def);
    buf_printf(&struct_def, "typedef struct {\n");
    
    if (base_class) {
        for (Scope *sc = cg->scope; sc != NULL; sc = sc->parent) {
            int initial_count = sc->count;
            for (int i = 0; i < initial_count; i++) {
                Symbol *sym = &sc->symbols[i];
                size_t blen = strlen(base_class);
                if (strncmp(sym->name, base_class, blen) == 0 && sym->name[blen] == '.') {
                    char new_key[256];
                    const char *dot = strchr(sym->name, '.');
                    if (dot) {
                        snprintf(new_key, sizeof(new_key), "%s%s", class_name, dot);
                        Symbol *nsym = scope_define(cg->scope, new_key, sym->type);
                        if (nsym) {
                            nsym->access = sym->access;
                            nsym->owner_class = sym->owner_class ? strdup(sym->owner_class) : NULL;
                            nsym->is_method = sym->is_method;
                            nsym->is_function = sym->is_function;
                            nsym->is_variadic = sym->is_variadic;
                            nsym->has_kwargs = sym->has_kwargs;
                            nsym->num_params = sym->num_params;
                            nsym->first_param_type = sym->first_param_type;
                            if (sym->first_param_class_name) {
                                nsym->first_param_class_name = strdup(sym->first_param_class_name);
                            }
                        }
                        if (!sym->is_method) {
                            buf_printf(&struct_def, "    %s lp_%s;\n", lp_type_to_c(sym->type), dot + 1);
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < node->class_def.body.count; i++) {
        AstNode *stmt = node->class_def.body.items[i];
        if (stmt->type == NODE_ASSIGN) {
            LpType t = stmt->assign.type_ann ? type_from_annotation(cg, stmt->assign.type_ann) : infer_type(cg, stmt->assign.value);
            if (t == LP_UNKNOWN) t = LP_INT;
            buf_printf(&struct_def, "    %s lp_%s;\n", lp_type_to_c(t), stmt->assign.name);
            
            /* Register field for access checking */
            char field_key[256];
            snprintf(field_key, sizeof(field_key), "%s.%s", class_name, stmt->assign.name);
            Symbol *sym = scope_define(cg->scope, field_key, t);
            if (sym) {
                sym->access = stmt->assign.access;
                sym->owner_class = strdup(class_name);
                sym->is_method = 0;
            }
        }
    }
    buf_printf(&struct_def, "} LpObj_%s;\n\n", class_name);

    /* Generate initialization function for the class */
    buf_printf(&struct_def, "static inline void %s_init(LpObj_%s* self) {\n", class_name, class_name);
    if (base_class) {
        buf_printf(&struct_def, "    %s_init((LpObj_%s*)self);\n", base_class, base_class);
    }
    for (int i = 0; i < node->class_def.body.count; i++) {
        AstNode *stmt = node->class_def.body.items[i];
        if (stmt->type == NODE_ASSIGN) {
            buf_printf(&struct_def, "    self->lp_%s = ", stmt->assign.name);
            gen_expr(cg, &struct_def, stmt->assign.value);
            buf_printf(&struct_def, ";\n");
        }
    }
    buf_printf(&struct_def, "}\n\n");

    /* Write struct definition to the header buffer so it's available everywhere */
    buf_write(&cg->header, struct_def.data);
    buf_free(&struct_def);

    /* Generate class methods */
    size_t class_name_len = strlen(class_name);
    cg->current_class = (char*)malloc(class_name_len + 1);
    if (cg->current_class) {
        strncpy(cg->current_class, class_name, class_name_len + 1);
        cg->current_class[class_name_len] = '\0';
    }
    
    for (int i = 0; i < node->class_def.body.count; i++) {
        AstNode *stmt = node->class_def.body.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            /* Register method for access checking */
            char method_key[256];
            snprintf(method_key, sizeof(method_key), "%s.%s", class_name, stmt->func_def.name);
            LpType rt = stmt->func_def.ret_type ? type_from_annotation(cg, stmt->func_def.ret_type) : LP_VOID;
            Symbol *sym = scope_define(cg->scope, method_key, rt);
            if (sym) {
                sym->access = stmt->func_def.access;
                sym->owner_class = strdup(class_name);
                sym->is_method = 1;
            }
            
            /* Prefix method names with the class name */
            char method_name[256];
            snprintf(method_name, sizeof(method_name), "%s_%s", class_name, stmt->func_def.name);
            
            /* Save old name and temporary rename for code gen */
            char *old_name = stmt->func_def.name;
            stmt->func_def.name = strdup(method_name);
            
            gen_func_def(cg, stmt, class_name);
            
            free(stmt->func_def.name);
            stmt->func_def.name = old_name;
        }
    }
    free(cg->current_class);
    cg->current_class = NULL;
}

/* --- Public API --- */
void codegen_init(CodeGen *cg) {
    buf_init(&cg->header);
    buf_init(&cg->helpers);
    buf_init(&cg->funcs);
    buf_init(&cg->main_body);
    cg->scope = scope_new(NULL);
    cg->had_error = 0;
    cg->error_msg[0] = '\0';
    cg->current_class = NULL;
    cg->import_count = 0;
    cg->uses_python = 0;
    cg->uses_native = 0;
    cg->uses_os = 0;
    cg->uses_sys = 0;
    cg->uses_strings = 0;
    cg->uses_http = 0;
    cg->uses_json = 0;
    cg->uses_sqlite = 0;
    cg->uses_thread = 0;
    cg->uses_memory = 0;
    cg->uses_platform = 0;
    cg->uses_parallel = 0;
    cg->uses_gpu = 0;
    cg->uses_security = 0;
    cg->uses_dsa = 0;
    cg->has_main = 0;
    cg->thread_adapter_count = 0;
}
void codegen_generate(CodeGen *cg, AstNode *program) {
    buf_write(&cg->header, "/* Generated by LP Compiler */\n");
    buf_write(&cg->header, "#define LP_MAIN_FILE\n");
    buf_write(&cg->header, "#include \"lp_runtime.h\"\n");
    buf_write(&cg->header, "#include \"lp_native_strings.h\"\n");

    /* First pass: collect all imports to determine which headers to include */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_IMPORT) {
            const char *module = stmt->import_stmt.module;
            if (!module) {
                codegen_set_error(cg, "Encountered invalid import module (OOM during parse)");
                continue; // BỎ QUA NẾU MODULE BỊ NULL
            }
            const char *alias = stmt->import_stmt.alias ? stmt->import_stmt.alias : module;

            /* Determine tier */
            ModTier tier;
            if (strcmp(module, "math") == 0)        tier = MOD_TIER1_MATH;
            else if (strcmp(module, "random") == 0)  tier = MOD_TIER1_RANDOM;
            else if (strcmp(module, "time") == 0)    tier = MOD_TIER1_TIME;
            else if (strcmp(module, "numpy") == 0)   tier = MOD_TIER2_NUMPY;
            else if (strcmp(module, "os") == 0)       tier = MOD_TIER1_OS;
            else if (strcmp(module, "sys") == 0)      tier = MOD_TIER1_SYS;
            else if (strcmp(module, "string") == 0)   tier = MOD_TIER1_STRING;
            else if (strcmp(module, "http") == 0)     tier = MOD_TIER1_HTTP;
            else if (strcmp(module, "json") == 0)     tier = MOD_TIER1_JSON;
            else if (strcmp(module, "sqlite") == 0)   tier = MOD_TIER1_SQLITE;
            else if (strcmp(module, "thread") == 0)   tier = MOD_TIER1_THREAD;
            else if (strcmp(module, "memory") == 0)   tier = MOD_TIER1_MEMORY;
            else if (strcmp(module, "platform") == 0) tier = MOD_TIER1_PLATFORM;
            else if (strcmp(module, "security") == 0) tier = MOD_TIER1_SECURITY;
            else if (strcmp(module, "dsa") == 0)       tier = MOD_TIER1_DSA;
            else                                     tier = MOD_TIER3_PYTHON;

            /* Register import */
            if (cg->import_count < 64) {
                size_t module_len = strlen(module);
                size_t alias_len = strlen(alias);
                
                char *mod_dup = (char *)malloc(module_len + 1);
                char *alias_dup = (char *)malloc(alias_len + 1);

                if (mod_dup && alias_dup) {
                    strncpy(mod_dup, module, module_len + 1);
                    mod_dup[module_len] = '\0';
                    
                    strncpy(alias_dup, alias, alias_len + 1);
                    alias_dup[alias_len] = '\0';

                    ImportInfo *info = &cg->imports[cg->import_count++];
                    info->module = mod_dup;
                    info->alias = alias_dup;
                    info->tier = tier;

                    if (tier == MOD_TIER3_PYTHON) cg->uses_python = 1;
                    else if (tier == MOD_TIER1_OS) { cg->uses_native = 1; cg->uses_os = 1; }
                    else if (tier == MOD_TIER1_SYS) { cg->uses_native = 1; cg->uses_sys = 1; }
                    else if (tier == MOD_TIER1_STRING) { cg->uses_native = 1; cg->uses_strings = 1; }
                    else if (tier == MOD_TIER1_HTTP) { cg->uses_native = 1; cg->uses_http = 1; }
                    else if (tier == MOD_TIER1_JSON) { cg->uses_native = 1; cg->uses_json = 1; }
                    else if (tier == MOD_TIER1_SQLITE) { cg->uses_native = 1; cg->uses_sqlite = 1; }
                    else if (tier == MOD_TIER1_THREAD) { cg->uses_native = 1; cg->uses_thread = 1; }
                    else if (tier == MOD_TIER1_MEMORY) { cg->uses_native = 1; cg->uses_memory = 1; }
                    else if (tier == MOD_TIER1_PLATFORM) { cg->uses_native = 1; cg->uses_platform = 1; }
                    else if (tier == MOD_TIER1_SECURITY) { cg->uses_native = 1; cg->uses_security = 1; }
                    else if (tier == MOD_TIER1_DSA) { cg->uses_native = 1; cg->uses_dsa = 1; }
                    else cg->uses_native = 1;
                } else {
                    if (mod_dup) free(mod_dup);
                    if (alias_dup) free(alias_dup);
                }
            }
        }
    }

    /* Include appropriate headers */
    if (cg->uses_native) {
        buf_write(&cg->header, "#include \"lp_native_modules.h\"\n");
    }
    if (cg->uses_os) {
        buf_write(&cg->header, "#include \"lp_native_os.h\"\n");
    }
    if (cg->uses_sys) {
        buf_write(&cg->header, "#include \"lp_native_sys.h\"\n");
    }
    if (cg->uses_strings) {
        buf_write(&cg->header, "#include \"lp_native_strings.h\"\n");
    }
    if (cg->uses_http) {
        buf_write(&cg->header, "#include \"lp_http.h\"\n");
    }
    if (cg->uses_json) {
        buf_write(&cg->header, "#include \"lp_json.h\"\n");
    }
    if (cg->uses_sqlite) {
        buf_write(&cg->header, "#include \"lp_sqlite.h\"\n");
    }
    if (cg->uses_thread) {
        buf_write(&cg->header, "#include \"lp_thread.h\"\n");
    }
    if (cg->uses_memory) {
        buf_write(&cg->header, "#include \"lp_memory.h\"\n");
    }
    if (cg->uses_security) {
        buf_write(&cg->header, "#include \"lp_security.h\"\n");
    }
    if (cg->uses_platform) {
        buf_write(&cg->header, "#include \"lp_platform.h\"\n");
    }
    if (cg->uses_dsa) {
        buf_write(&cg->header, "#include \"lp_dsa.h\"\n");
    }
    if (cg->uses_python) {
        buf_write(&cg->header, "#include \"lp_python.h\"\n");
    }
    buf_write(&cg->header, "\n");

    /* Second pass: forward register all function symbols (including async functions) */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            Symbol *func_sym = scope_define(cg->scope, stmt->func_def.name, LP_VOID);
            if (func_sym) {
                populate_function_symbol(cg, func_sym, stmt, NULL);
            }
            /* Track if main() is defined */
            if (strcmp(stmt->func_def.name, "main") == 0) {
                cg->has_main = 1;
            }
        } else if (stmt->type == NODE_ASYNC_DEF) {
            /* Register async functions similarly to regular functions */
            Symbol *func_sym = scope_define(cg->scope, stmt->async_def.name, LP_VOID);
            if (func_sym) {
                func_sym->is_function = 1;
                func_sym->num_params = stmt->async_def.params.count;
                func_sym->access = stmt->async_def.access;
            }
        }
    }
    /* Third pass: generate code */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            gen_func_def(cg, stmt, NULL);
        } else if (stmt->type == NODE_ASYNC_DEF) {
            gen_async_func_def(cg, stmt, NULL);
        } else if (stmt->type == NODE_CLASS_DEF) {
            gen_class_def(cg, stmt);
        } else if (stmt->type == NODE_IMPORT) {
            /* Imports are handled in first pass, skip in code gen */
        } else {
            gen_stmt(cg, &cg->main_body, stmt, 1);
        }
    }
}

void codegen_generate_header(CodeGen *cg, AstNode *program, const char *header_path) {
    FILE *f = fopen(header_path, "w");
    if (!f) {
        snprintf(cg->error_msg, sizeof(cg->error_msg), "Cannot open header path %s for writing", header_path);
        cg->had_error = 1;
        return;
    }

    fprintf(f, "#ifndef LP_EXPORTED_API_H\n");
    fprintf(f, "#define LP_EXPORTED_API_H\n\n");
    fprintf(f, "#include <stdint.h>\n\n");
    fprintf(f, "#if defined(_WIN32)\n");
    fprintf(f, "  #define LP_EXTERN __declspec(dllexport)\n");
    fprintf(f, "#else\n");
    fprintf(f, "  #define LP_EXTERN __attribute__((visibility(\"default\")))\n");
    fprintf(f, "#endif\n\n");
    fprintf(f, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            if (stmt->func_def.access == TOK_PRIVATE) continue;
            if (stmt->func_def.name[0] == '_') continue; /* By convention, _ means private */

            LpType ret = infer_function_return_type(cg, stmt);

            fprintf(f, "LP_EXTERN %s lp_%s(", lp_type_to_c(ret), stmt->func_def.name);
            for (int k = 0; k < stmt->func_def.params.count; k++) {
                if (k > 0) fprintf(f, ", ");
                LpType pt = stmt->func_def.params.items[k].type_ann ? 
                    type_from_annotation(cg, stmt->func_def.params.items[k].type_ann) : LP_FLOAT;
                fprintf(f, "%s lp_%s", lp_type_to_c(pt), stmt->func_def.params.items[k].name);
            }
            if (stmt->func_def.params.count == 0) {
                fprintf(f, "void");
            }
            fprintf(f, ");\n");
        }
    }

    fprintf(f, "\n#ifdef __cplusplus\n}\n#endif\n");
    fprintf(f, "\n#endif /* LP_EXPORTED_API_H */\n");
    fclose(f);
}

char *codegen_get_output(CodeGen *cg) {
    Buffer out;
    buf_init(&out);
    buf_write(&out, cg->header.data ? cg->header.data : "");
    buf_write(&out, cg->helpers.data ? cg->helpers.data : "");
    buf_write(&out, cg->funcs.data ? cg->funcs.data : "");
    buf_write(&out, "int main(void) {\n");

    /* Python init if needed */
    if (cg->uses_python) {
        buf_write(&out, "    lp_py_init();\n");
        /* Declare and import Python modules */
        for (int i = 0; i < cg->import_count; i++) {
            if (cg->imports[i].tier == MOD_TIER3_PYTHON) {
                buf_printf(&out, "    PyObject *lp_pymod_%s = lp_py_import(\"%s\");\n",
                           cg->imports[i].alias, cg->imports[i].module);
            }
        }
        buf_write(&out, "\n");
    }

    buf_write(&out, cg->main_body.data ? cg->main_body.data : "");

    /* Python finalize if needed */
    if (cg->uses_python) {
        buf_write(&out, "\n    /* Cleanup Python modules */\n");
        for (int i = 0; i < cg->import_count; i++) {
            if (cg->imports[i].tier == MOD_TIER3_PYTHON) {
                buf_printf(&out, "    lp_py_decref(lp_pymod_%s);\n", cg->imports[i].alias);
            }
        }
        buf_write(&out, "    lp_py_finalize();\n");
    }

    if (cg->has_main) {
        buf_write(&out, "    return (int)lp_main();\n}\n");
    } else {
        buf_write(&out, "    return 0;\n}\n");
    }
    return out.data;
}
void codegen_free(CodeGen *cg) {
    buf_free(&cg->header);
    buf_free(&cg->helpers);
    buf_free(&cg->funcs);
    buf_free(&cg->main_body);
    if (cg->scope) scope_free(cg->scope);
    for (int i = 0; i < cg->import_count; i++) {
        free(cg->imports[i].module);
        free(cg->imports[i].alias);
    }
}



