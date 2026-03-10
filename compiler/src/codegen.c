#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* --- Buffer --- */
void buf_init(Buffer *b) { b->data = NULL; b->len = 0; b->cap = 0; }

void buf_write(Buffer *b, const char *s) {
    int slen = (int)strlen(s);
    if (b->len + slen >= b->cap) {
        b->cap = (b->len + slen + 1) * 2;
        b->data = (char *)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, slen);
    b->len += slen;
    b->data[b->len] = '\0';
}

void buf_printf(Buffer *b, const char *fmt, ...) {
    char tmp[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    buf_write(b, tmp);
}

void buf_free(Buffer *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }

/* --- Scope / Symbol Table --- */
static Scope *scope_new(Scope *parent) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    s->parent = parent;
    return s;
}

static void scope_free(Scope *s) {
    for (int i = 0; i < s->count; i++) free(s->symbols[i].name);
    free(s);
}

static Symbol *scope_lookup(Scope *s, const char *name) {
    for (; s; s = s->parent)
        for (int i = 0; i < s->count; i++)
            if (strcmp(s->symbols[i].name, name) == 0)
                return &s->symbols[i];
    return NULL;
}

static Symbol *scope_define(Scope *s, const char *name, LpType type) {
    if (s->count >= 512) return NULL;
    Symbol *sym = &s->symbols[s->count++];
    sym->name = (char *)malloc(strlen(name) + 1);
    strcpy(sym->name, name);
    sym->type = type;
    sym->class_name = NULL;
    sym->declared = 0;
    return sym;
}

static Symbol *scope_define_obj(Scope *s, const char *name, LpType type, const char *class_name) {
    Symbol *sym = scope_define(s, name, type);
    if (sym && type == LP_OBJECT && class_name) {
        sym->class_name = (char *)malloc(strlen(class_name) + 1);
        strcpy(sym->class_name, class_name);
    }
    return sym;
}

/* --- Type mapping --- */
static LpType type_from_annotation(const char *ann) {
    if (!ann) return LP_UNKNOWN;
    if (strcmp(ann, "int") == 0 || strcmp(ann, "i64") == 0) return LP_INT;
    if (strcmp(ann, "float") == 0 || strcmp(ann, "f64") == 0) return LP_FLOAT;
    if (strcmp(ann, "str") == 0) return LP_STRING;
    if (strcmp(ann, "bool") == 0) return LP_BOOL;
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
        case LP_ARRAY:  return "LpArray";
        case LP_STR_ARRAY: return "LpStrArray";
        case LP_DICT:   return "LpDict*";
        case LP_SET:    return "LpSet*";
        case LP_TUPLE:  return "LpTuple*";
        case LP_FILE:   return "LpFile*";
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

/* Check if a name is a known module alias */
static int is_module_alias(CodeGen *cg, const char *name) {
    return find_import(cg, name) != NULL;
}

/* --- Infer expression type --- */
static LpType infer_type(CodeGen *cg, AstNode *node) {
    if (!node) return LP_VOID;
    switch (node->type) {
        case NODE_INT_LIT:    return LP_INT;
        case NODE_FLOAT_LIT:  return LP_FLOAT;
        case NODE_STRING_LIT: return LP_STRING;
        case NODE_BOOL_LIT:   return LP_BOOL;
        case NODE_NONE_LIT:   return LP_VOID;
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
                op == TOK_LTE || op == TOK_GTE || op == TOK_AND || op == TOK_OR)
                return LP_BOOL;
            if (lt == LP_FLOAT || rt == LP_FLOAT) return LP_FLOAT;
            if (op == TOK_SLASH) return LP_FLOAT;
            return LP_INT;
        }
        case NODE_UNARY_OP:
            if (node->unary_op.op == TOK_NOT) return LP_BOOL;
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
                    return LP_STRING; /* upper, lower, strip, replace, join */
                }
            }
            if (node->call.func->type == NODE_NAME) {
                const char *func = node->call.func->name_expr.name;
                if (strcmp(func, "open") == 0) return LP_FILE;
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
            return LP_UNKNOWN;
        }
        case NODE_KWARG:
            return infer_type(cg, node->kwarg.value);
        case NODE_LIST_COMP:
            return LP_ARRAY;
        case NODE_LAMBDA:
            return LP_INT; /* function pointer stored as int */
        default: return LP_UNKNOWN;
    }
}

/* --- Code generation --- */
static void gen_expr(CodeGen *cg, Buffer *buf, AstNode *node);
static void gen_stmt(CodeGen *cg, Buffer *buf, AstNode *node, int indent);

static void write_indent(Buffer *buf, int level) {
    for (int i = 0; i < level; i++) buf_write(buf, "    ");
}

static void emit_lp_val(CodeGen *cg, Buffer *buf, AstNode *expr) {
    LpType t = infer_type(cg, expr);
    switch (t) {
        case LP_INT: buf_write(buf, "lp_val_int("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_FLOAT: buf_write(buf, "lp_val_float("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_STRING: buf_write(buf, "lp_val_str("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
        case LP_BOOL: buf_write(buf, "lp_val_bool("); gen_expr(cg, buf, expr); buf_write(buf, ")"); break;
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
            buf_printf(buf, "\"%s\"", node->str_lit.value);
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
            buf_printf(buf, "({ LpTuple _t = lp_tuple_new(%d); ", node->tuple_expr.elems.count);
            for (int i = 0; i < node->tuple_expr.elems.count; i++) {
                buf_printf(buf, "lp_tuple_set(&_t, %d, (void*)(intptr_t)", i);
                gen_expr(cg, buf, node->tuple_expr.elems.items[i]);
                buf_write(buf, "); ");
            }
            buf_write(buf, "_t; })");
            break;
        }
        case NODE_BIN_OP: {
            TokenType op = node->bin_op.op;
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
                buf_write(buf, "lp_floordiv(");
                gen_expr(cg, buf, node->bin_op.left);
                buf_write(buf, ", ");
                gen_expr(cg, buf, node->bin_op.right);
                buf_write(buf, ")");
            } else if (op == TOK_PERCENT) {
                buf_write(buf, "lp_mod(");
                gen_expr(cg, buf, node->bin_op.left);
                buf_write(buf, ", ");
                gen_expr(cg, buf, node->bin_op.right);
                buf_write(buf, ")");
            } else {
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
                    default: ops = " ? "; break;
                }
                buf_write(buf, "(");
                gen_expr(cg, buf, node->bin_op.left);
                buf_write(buf, ops);
                gen_expr(cg, buf, node->bin_op.right);
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
                    /* ---- TIER 1: math → direct C <math.h> ---- */
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
                    /* ---- TIER 2: numpy → optimized C arrays ---- */
                    if (imp->tier == MOD_TIER2_NUMPY) {
                        /* np.array([1,2,3]) → from_doubles */
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
                            strcmp(func_name, "sort") == 0) {
                            buf_printf(buf, "lp_np_%s(", func_name);
                            for (int i = 0; i < node->call.args.count; i++) {
                                if (i > 0) buf_write(buf, ", ");
                                gen_expr(cg, buf, node->call.args.items[i]);
                            }
                            buf_write(buf, ")");
                            break;
                        }
                        /* np.add, np.subtract, np.multiply → element-wise ops */
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
                            /* Generic numpy function — fallback */
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
            }
            /* Special-case: print() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "print") == 0) {
                for (int i = 0; i < node->call.args.count; i++) {
                    if (i > 0) buf_write(buf, ", ");
                    LpType at = infer_type(cg, node->call.args.items[i]);
                    switch (at) {
                        case LP_FLOAT:  buf_write(buf, "lp_print_float("); break;
                        case LP_STRING: buf_write(buf, "lp_print_str("); break;
                        case LP_BOOL:   buf_write(buf, "lp_print_bool("); break;
                        case LP_ARRAY:  buf_write(buf, "lp_np_print("); break;
                        case LP_PYOBJ:  buf_write(buf, "lp_py_print("); break;
                        case LP_DICT:   buf_write(buf, "lp_print_dict("); break;
                        case LP_SET:    buf_write(buf, "lp_print_set("); break;
                        default:        buf_write(buf, "lp_print_generic("); break;
                    }
                    gen_expr(cg, buf, node->call.args.items[i]);
                    buf_write(buf, ")");
                }
                break;
            }
            /* Special-case: len() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "len") == 0) {
                if (node->call.args.count > 0) {
                    LpType at = infer_type(cg, node->call.args.items[0]);
                    if (at == LP_ARRAY) {
                        buf_write(buf, "lp_np_len(");
                    } else {
                        buf_write(buf, "lp_len(");
                    }
                    gen_expr(cg, buf, node->call.args.items[0]);
                    buf_write(buf, ")");
                }
                break;
            }
            /* Special-case: open() */
            if (node->call.func->type == NODE_NAME &&
                strcmp(node->call.func->name_expr.name, "open") == 0) {
                buf_write(buf, "lp_io_open(");
                for (int i = 0; i < node->call.args.count; i++) {
                    if (i > 0) buf_write(buf, ", ");
                    gen_expr(cg, buf, node->call.args.items[i]);
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
                    buf_printf(buf, "lp_%s_%s(", s->class_name, node->call.func->attribute.attr);
                    buf_printf(buf, "lp_%s", s->name); /* Pass 'self' as first arg */
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
                            /* **dict_var unpacking — merge into _kwargs */
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
                    gen_expr(cg, buf, node->call.args.items[i]);
                }
                buf_write(buf, ")");
            }
            break;
        }
        case NODE_KWARG:
            /* Keyword argument — only used inline when not packed into dict */
            gen_expr(cg, buf, node->kwarg.value);
            break;
        case NODE_LIST_COMP: {
            /* [expr for var in iter if cond]
             * Generates: ({ double *_lc = malloc(n * sizeof(double)); int _lc_len = 0;
             *              for(int lp_var = ...) { if(cond) _lc[_lc_len++] = expr; }
             *              LpArray _lca = {_lc, _lc_len, _lc_len, {_lc_len}}; _lca; })
             *
             * For simplicity, we use range-style iteration since LP's for loops
             * use range() most commonly. We'll emit a fixed-size allocation
             * and grow if needed. */
            buf_write(buf, "({ ");
            buf_write(buf, "int _lc_cap = 16; ");
            buf_write(buf, "double *_lc = (double*)malloc(_lc_cap * sizeof(double)); ");
            buf_write(buf, "int _lc_len = 0; ");

            /* Generate the iteration. The iter expression should be range() or similar */
            /* For now, generate a simple for-each over a range call */
            buf_write(buf, "for (int64_t lp_");
            buf_write(buf, node->list_comp.var);
            buf_write(buf, " = 0; lp_");
            buf_write(buf, node->list_comp.var);
            buf_write(buf, " < (int64_t)");
            gen_expr(cg, buf, node->list_comp.iter);
            buf_write(buf, "; lp_");
            buf_write(buf, node->list_comp.var);
            buf_write(buf, "++) { ");

            /* Optional if condition */
            if (node->list_comp.cond) {
                buf_write(buf, "if (");
                gen_expr(cg, buf, node->list_comp.cond);
                buf_write(buf, ") { ");
            }

            /* Grow if needed */
            buf_write(buf, "if (_lc_len >= _lc_cap) { _lc_cap *= 2; _lc = (double*)realloc(_lc, _lc_cap * sizeof(double)); } ");
            buf_write(buf, "_lc[_lc_len++] = (double)(");
            gen_expr(cg, buf, node->list_comp.expr);
            buf_write(buf, "); ");

            if (node->list_comp.cond) {
                buf_write(buf, "} ");
            }

            buf_write(buf, "} ");
            buf_write(buf, "LpArray _lca; _lca.data = _lc; _lca.len = _lc_len; _lca.cap = _lc_cap; _lca.shape[0] = _lc_len; ");
            buf_write(buf, "_lca; })");
            break;
        }
        case NODE_LAMBDA: {
            /* Lambda: generate a static function in cg->funcs and reference by name */
            static int lambda_counter = 0;
            char fname[64];
            snprintf(fname, sizeof(fname), "_lp_lambda_%d", lambda_counter++);

            /* Determine return type from body expression */
            LpType body_ret = infer_type(cg, node->lambda_expr.body);
            if (body_ret == LP_UNKNOWN) body_ret = LP_FLOAT;

            /* Generate the lambda function definition */
            buf_printf(&cg->funcs, "static %s %s(", lp_type_to_c(body_ret), fname);
            for (int i = 0; i < node->lambda_expr.params.count; i++) {
                if (i > 0) buf_write(&cg->funcs, ", ");
                Param *lp = &node->lambda_expr.params.items[i];
                LpType pt = lp->type_ann ? type_from_annotation(lp->type_ann) : LP_FLOAT;
                buf_printf(&cg->funcs, "%s lp_%s", lp_type_to_c(pt), lp->name);
            }
            if (node->lambda_expr.params.count == 0) {
                buf_write(&cg->funcs, "void");
            }
            buf_write(&cg->funcs, ") { return ");
            gen_expr(cg, &cg->funcs, node->lambda_expr.body);
            buf_write(&cg->funcs, "; }\n\n");

            /* Emit the function name as the expression */
            buf_write(buf, fname);
            break;
        }
        case NODE_SUBSCRIPT:
            gen_expr(cg, buf, node->subscript.obj);
            buf_write(buf, "[");
            gen_expr(cg, buf, node->subscript.index);
            buf_write(buf, "]");
            break;
        case NODE_ATTRIBUTE: {
            /* Object instance method or property access */
            if (node->attribute.obj->type == NODE_NAME) {
                Symbol *s = scope_lookup(cg->scope, node->attribute.obj->name_expr.name);
                if (s && s->type == LP_OBJECT) {
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
                    /* Tier 1: os, sys — attribute access */
                    if (imp->tier == MOD_TIER1_OS || imp->tier == MOD_TIER1_SYS) {
                        buf_printf(buf, "lp_%s_%s", imp->module, node->attribute.attr);
                        break;
                    }
                }
            }
            gen_expr(cg, buf, node->attribute.obj);
            buf_printf(buf, ".%s", node->attribute.attr);
            break;
        }
        default:
            buf_write(buf, "/* unknown expr */");
            break;
    }
}

/* Check if a call is to range() */
static int is_range_call(AstNode *node) {
    return node && node->type == NODE_CALL &&
           node->call.func->type == NODE_NAME &&
           strcmp(node->call.func->name_expr.name, "range") == 0;
}

static void gen_stmt(CodeGen *cg, Buffer *buf, AstNode *node, int indent) {
    if (!node) return;
    switch (node->type) {
        case NODE_ASSIGN: {
            LpType t = LP_UNKNOWN;
            if (node->assign.type_ann)
                t = type_from_annotation(node->assign.type_ann);
            else if (node->assign.value)
                t = infer_type(cg, node->assign.value);
            if (t == LP_UNKNOWN) t = LP_INT;

            /* Check if it's an attribute assignment (e.g. obj.attr) */
            char *dot = strchr(node->assign.name, '.');
            if (dot) {
                *dot = '\0'; /* split into obj and attr */
                const char *obj_name = node->assign.name;
                const char *attr_name = dot + 1;
                
                write_indent(buf, indent);
                buf_printf(buf, "lp_%s->lp_%s = ", obj_name, attr_name);
                gen_expr(cg, buf, node->assign.value);
                buf_write(buf, ";\n");
                
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
                    }
                    scope_define_obj(cg->scope, node->assign.name, t, class_name);
                    write_indent(buf, indent);
                    buf_printf(buf, "%s lp_%s", lp_type_to_c_obj(t, class_name), node->assign.name);
                    if (node->assign.value) {
                        buf_write(buf, " = ");
                        gen_expr(cg, buf, node->assign.value);
                    }
                    buf_write(buf, ";\n");
                } else {
                    write_indent(buf, indent);
                    buf_printf(buf, "lp_%s = ", node->assign.name);
                    gen_expr(cg, buf, node->assign.value);
                    buf_write(buf, ";\n");
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
                } else if (args->count >= 2) {
                    buf_printf(buf, "for (int64_t lp_%s = ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[0]);
                    buf_printf(buf, "; lp_%s < ", node->for_stmt.var);
                    gen_expr(cg, buf, args->items[1]);
                    if (args->count >= 3) {
                        buf_printf(buf, "; lp_%s += ", node->for_stmt.var);
                        gen_expr(cg, buf, args->items[2]);
                    } else {
                        buf_printf(buf, "; lp_%s++", node->for_stmt.var);
                    }
                    buf_write(buf, ") {\n");
                }
            } else {
                write_indent(buf, indent);
                buf_write(buf, "/* TODO: generic for loop */\n");
                write_indent(buf, indent);
                buf_write(buf, "{\n");
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
        case NODE_WITH: {
            write_indent(buf, indent);
            buf_write(buf, "{\n");
            int inner_indent = indent + 1;
            /* Assign alias if provided */
            if (node->with_stmt.alias) {
                scope_define(cg->scope, node->with_stmt.alias, LP_FILE);
                write_indent(buf, inner_indent);
                buf_printf(buf, "LpFile* lp_%s = ", node->with_stmt.alias);
                gen_expr(cg, buf, node->with_stmt.expr);
                buf_write(buf, ";\n");
            } else {
                /* Evaluate expression anyway */
                write_indent(buf, inner_indent);
                buf_write(buf, "LpFile* __lp_with_tmp = ");
                gen_expr(cg, buf, node->with_stmt.expr);
                buf_write(buf, ";\n");
            }
            for (int i = 0; i < node->with_stmt.body.count; i++)
                gen_stmt(cg, buf, node->with_stmt.body.items[i], inner_indent);
            
            /* finally: close the file implicitly */
            write_indent(buf, inner_indent);
            if (node->with_stmt.alias) {
                buf_printf(buf, "lp_io_close(lp_%s);\n", node->with_stmt.alias);
            } else {
                buf_write(buf, "lp_io_close(__lp_with_tmp);\n");
            }
            write_indent(buf, indent);
            buf_write(buf, "}\n");
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
        default:
            write_indent(buf, indent);
            buf_write(buf, "/* unknown statement */\n");
            break;
    }
}

static void gen_func_def(CodeGen *cg, AstNode *node, const char *class_name) {
    LpType ret = node->func_def.ret_type ?
                 type_from_annotation(node->func_def.ret_type) : LP_FLOAT; /* LP lacks full dynamic yet, default to float for sum */

    /* Find pre-registered function in parent scope */
    Symbol *func_sym = scope_lookup(cg->scope, node->func_def.name);
    if (!func_sym) {
        func_sym = scope_define(cg->scope, node->func_def.name, ret);
        if (func_sym) {
            func_sym->is_variadic = 0;
            func_sym->num_params = node->func_def.params.count;
            func_sym->has_kwargs = 0;
            for (int i = 0; i < node->func_def.params.count; i++) {
                if (node->func_def.params.items[i].is_vararg) {
                    func_sym->is_variadic = 1;
                } else if (node->func_def.params.items[i].is_kwarg) {
                    func_sym->has_kwargs = 1;
                }
            }
        }
    }

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
            LpType pt = type_from_annotation(p->type_ann);
            if (pt == LP_UNKNOWN) pt = LP_INT;
            buf_printf(&cg->funcs, "%s lp_%s", lp_type_to_c(pt), p->name);
            scope_define(func_scope, p->name, pt);
        }
    }
    buf_write(&cg->funcs, ") {\n");

    /* Body */
    for (int i = 0; i < node->func_def.body.count; i++)
        gen_stmt(cg, &cg->funcs, node->func_def.body.items[i], 1);

    buf_write(&cg->funcs, "}\n\n");

    /* Restore scope */
    cg->scope = func_scope->parent;
    scope_free(func_scope);
}

static void gen_class_def(CodeGen *cg, AstNode *node) {
    /* 
     * In Phase 3, a Python class translates to a C struct.
     * We scan the class body for field assignments (e.g. name = "LP")
     * and methods (def ...). 
     */
    const char *class_name = node->class_def.name;
    
    /* Register class in current scope */
    scope_define(cg->scope, class_name, LP_CLASS);

    /* Collect fields for the struct */
    Buffer struct_def;
    buf_init(&struct_def);
    buf_printf(&struct_def, "typedef struct {\n");
    
    for (int i = 0; i < node->class_def.body.count; i++) {
        AstNode *stmt = node->class_def.body.items[i];
        if (stmt->type == NODE_ASSIGN) {
            LpType t = stmt->assign.type_ann ? type_from_annotation(stmt->assign.type_ann) : infer_type(cg, stmt->assign.value);
            if (t == LP_UNKNOWN) t = LP_INT;
            buf_printf(&struct_def, "    %s lp_%s;\n", lp_type_to_c(t), stmt->assign.name);
        }
    }
    buf_printf(&struct_def, "} LpObj_%s;\n\n", class_name);

    /* Generate initialization function for the class */
    buf_printf(&struct_def, "static inline void %s_init(LpObj_%s* self) {\n", class_name, class_name);
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

    /* 
     * Generate class methods 
     * TBD Phase 3.2: We will pass `LpObj_Class* self` implicitly.
     */
    for (int i = 0; i < node->class_def.body.count; i++) {
        AstNode *stmt = node->class_def.body.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
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
}

/* --- Public API --- */
void codegen_init(CodeGen *cg) {
    buf_init(&cg->header);
    buf_init(&cg->funcs);
    buf_init(&cg->main_body);
    cg->scope = scope_new(NULL);
    cg->had_error = 0;
    cg->error_msg[0] = '\0';
    cg->import_count = 0;
    cg->uses_python = 0;
    cg->uses_native = 0;
    cg->uses_os = 0;
    cg->uses_sys = 0;
    cg->uses_strings = 0;
}

void codegen_generate(CodeGen *cg, AstNode *program) {
    buf_write(&cg->header, "/* Generated by LP Compiler */\n");
    buf_write(&cg->header, "#define LP_MAIN_FILE\n");
    buf_write(&cg->header, "#include \"lp_runtime.h\"\n");

    /* First pass: collect all imports to determine which headers to include */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_IMPORT) {
            const char *module = stmt->import_stmt.module;
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
            else                                     tier = MOD_TIER3_PYTHON;

            /* Register import */
            if (cg->import_count < 64) {
                ImportInfo *info = &cg->imports[cg->import_count++];
                info->module = (char *)malloc(strlen(module) + 1);
                strcpy(info->module, module);
                info->alias = (char *)malloc(strlen(alias) + 1);
                strcpy(info->alias, alias);
                info->tier = tier;

                if (tier == MOD_TIER3_PYTHON) cg->uses_python = 1;
                else if (tier == MOD_TIER1_OS) { cg->uses_native = 1; cg->uses_os = 1; }
                else if (tier == MOD_TIER1_SYS) { cg->uses_native = 1; cg->uses_sys = 1; }
                else if (tier == MOD_TIER1_STRING) { cg->uses_native = 1; cg->uses_strings = 1; }
                else cg->uses_native = 1;
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
    if (cg->uses_python) {
        buf_write(&cg->header, "#include \"lp_python.h\"\n");
    }
    buf_write(&cg->header, "\n");

    /* Second pass: forward register all function symbols */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            LpType ret = stmt->func_def.ret_type ? type_from_annotation(stmt->func_def.ret_type) : LP_VOID;
            Symbol *func_sym = scope_define(cg->scope, stmt->func_def.name, ret);
            if (func_sym) {
                func_sym->is_variadic = 0;
                func_sym->num_params = stmt->func_def.params.count;
                func_sym->has_kwargs = 0;
                for (int j = 0; j < stmt->func_def.params.count; j++) {
                    if (stmt->func_def.params.items[j].is_vararg) {
                        func_sym->is_variadic = 1;
                    } else if (stmt->func_def.params.items[j].is_kwarg) {
                        func_sym->has_kwargs = 1;
                    }
                }
            }
        }
    }

    /* Third pass: generate code */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            gen_func_def(cg, stmt, NULL);
        } else if (stmt->type == NODE_CLASS_DEF) {
            gen_class_def(cg, stmt);
        } else if (stmt->type == NODE_IMPORT) {
            /* Imports are handled in first pass, skip in code gen */
        } else {
            gen_stmt(cg, &cg->main_body, stmt, 1);
        }
    }
}

char *codegen_get_output(CodeGen *cg) {
    Buffer out;
    buf_init(&out);
    buf_write(&out, cg->header.data ? cg->header.data : "");
    /* Forward declarations */
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

    buf_write(&out, "    return 0;\n}\n");
    return out.data;
}

void codegen_free(CodeGen *cg) {
    buf_free(&cg->header);
    buf_free(&cg->funcs);
    buf_free(&cg->main_body);
    if (cg->scope) scope_free(cg->scope);
    for (int i = 0; i < cg->import_count; i++) {
        free(cg->imports[i].module);
        free(cg->imports[i].alias);
    }
}
