/*
 * LP Semantic Checker - Implementation
 * Validates semantics and detects common errors
 */
#include "semantic_check.h"
#include <string.h>
#include <ctype.h>

/* Valid types in LP */
const char *LP_VALID_TYPES[] = {
    "int", "float", "str", "string", "bool", "boolean",
    "list", "dict", "set", "tuple", "void", "null", "any",
    "bytes", "bytearray", "complex", "frozenset", "range",
    "Iterator", "Iterable", "Sequence", "Mapping",
    "Callable", "TypeVar", "Generic", "Union", "Optional",
    NULL
};

/* Valid modules in LP */
const char *LP_VALID_MODULES[] = {
    "dsa", "math", "os", "time", "random", "json",
    "http", "sqlite", "sys", "io",
    NULL
};

/* Symbol table functions */
void lp_symbol_table_init(LpSymbolTable *table) {
    table->head = NULL;
    table->tail = NULL;
    table->count = 0;
}

void lp_symbol_table_add(LpSymbolTable *table, const char *name, int line, int is_function, int is_imported) {
    /* Check if already exists */
    LpSymbol *existing = lp_symbol_table_find(table, name);
    if (existing) {
        return; /* Don't add duplicate */
    }
    
    LpSymbol *sym = (LpSymbol *)malloc(sizeof(LpSymbol));
    if (!sym) return;
    
    sym->name = strdup(name);
    sym->line_defined = line;
    sym->line_used = 0;
    sym->is_function = is_function;
    sym->is_imported = is_imported;
    sym->is_used = is_imported; /* Imports are considered used initially */
    sym->next = NULL;
    
    if (table->tail) {
        table->tail->next = sym;
        table->tail = sym;
    } else {
        table->head = sym;
        table->tail = sym;
    }
    table->count++;
}

LpSymbol *lp_symbol_table_find(LpSymbolTable *table, const char *name) {
    if (!table || !name) return NULL;
    
    LpSymbol *sym = table->head;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

void lp_symbol_table_mark_used(LpSymbolTable *table, const char *name, int line) {
    LpSymbol *sym = lp_symbol_table_find(table, name);
    if (sym) {
        sym->is_used = 1;
        if (sym->line_used == 0) {
            sym->line_used = line;
        }
    }
}

void lp_symbol_table_free(LpSymbolTable *table) {
    LpSymbol *sym = table->head;
    while (sym) {
        LpSymbol *next = sym->next;
        if (sym->name) free(sym->name);
        free(sym);
        sym = next;
    }
    table->head = NULL;
    table->tail = NULL;
    table->count = 0;
}

/* Initialize semantic checker */
void lp_semantic_init(LpSemanticChecker *checker, LpErrorList *errors, const char *source) {
    checker->errors = errors;
    checker->source = source;
    lp_symbol_table_init(&checker->symbols);
    
    checker->uses_dsa_module = 0;
    checker->has_dsa_flush = 0;
    checker->has_dsa_write = 0;
    checker->dsa_write_line = 0;
    checker->dsa_flush_line = 0;
    
    checker->current_function = NULL;
    checker->in_function = 0;
    
    checker->has_main_func = 0;
    checker->has_solve_func = 0;
    checker->main_func_line = 0;
    checker->solve_func_line = 0;
    checker->calls_main = 0;
    checker->calls_solve = 0;
    
    checker->last_func_line = 0;
    checker->last_func_name = NULL;
}

/* Free semantic checker */
void lp_semantic_free(LpSemanticChecker *checker) {
    lp_symbol_table_free(&checker->symbols);
    if (checker->current_function) free(checker->current_function);
    if (checker->last_func_name) free(checker->last_func_name);
}

/* Check if type is valid */
int lp_is_valid_type(const char *type_name) {
    if (!type_name) return 1; /* No type is okay for some cases */
    
    /* Skip if it's a generic type like list[int] or dict[str, int] */
    const char *bracket = strchr(type_name, '[');
    if (bracket) {
        /* Extract base type */
        size_t base_len = bracket - type_name;
        char *base_type = (char *)malloc(base_len + 1);
        if (base_type) {
            memcpy(base_type, type_name, base_len);
            base_type[base_len] = '\0';
            int result = lp_is_valid_type(base_type);
            free(base_type);
            return result;
        }
    }
    
    /* Check for union types like int|str|float */
    if (strchr(type_name, '|')) {
        /* For now, consider union types as valid */
        return 1;
    }
    
    /* Check against valid types list */
    for (int i = 0; LP_VALID_TYPES[i] != NULL; i++) {
        if (strcmp(type_name, LP_VALID_TYPES[i]) == 0) {
            return 1;
        }
    }
    
    /* Check for common Python types that might be used */
    if (strcmp(type_name, "None") == 0) return 0; /* Should use null */
    if (strcmp(type_name, "ptr") == 0) return 0;  /* Not valid in LP */
    if (strcmp(type_name, "pointer") == 0) return 0;
    
    /* Check for uppercase class names (could be custom classes) */
    if (isupper(type_name[0])) {
        return 1; /* Assume it's a class name */
    }
    
    return 1; /* Default to valid for unknown types */
}

/* Check if module is valid */
int lp_is_valid_module(const char *module_name) {
    if (!module_name) return 0;
    
    for (int i = 0; LP_VALID_MODULES[i] != NULL; i++) {
        if (strcmp(module_name, LP_VALID_MODULES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Extract base type from annotation string */
char *lp_extract_base_type(const char *type_annotation) {
    if (!type_annotation) return NULL;
    
    const char *bracket = strchr(type_annotation, '[');
    const char *pipe = strchr(type_annotation, '|');
    
    if (!bracket && !pipe) {
        return strdup(type_annotation);
    }
    
    size_t len;
    if (bracket && (!pipe || bracket < pipe)) {
        len = bracket - type_annotation;
    } else if (pipe) {
        len = pipe - type_annotation;
    } else {
        len = strlen(type_annotation);
    }
    
    char *result = (char *)malloc(len + 1);
    if (result) {
        memcpy(result, type_annotation, len);
        result[len] = '\0';
    }
    return result;
}

/* Forward declarations for recursive traversal */
static void traverse_node(LpSemanticChecker *checker, AstNode *node);
static void traverse_list(LpSemanticChecker *checker, NodeList *list);

/* Traverse a list of nodes */
static void traverse_list(LpSemanticChecker *checker, NodeList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        traverse_node(checker, list->items[i]);
    }
}

/* Traverse a single node */
static void traverse_node(LpSemanticChecker *checker, AstNode *node) {
    if (!node || !checker) return;
    
    switch (node->type) {
        case NODE_PROGRAM:
            traverse_list(checker, &node->program.stmts);
            break;
            
        case NODE_IMPORT: {
            /* Check if module is valid */
            char *module = node->import_stmt.module;
            if (module && !lp_is_valid_module(module)) {
                /* Check for common typo: DSA instead of dsa */
                if (strcmp(module, "DSA") == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Module '%s' not found. Did you mean 'dsa'?", module);
                    lp_error_list_add(checker->errors, ERR_UNKNOWN_MODULE, SEVERITY_ERROR,
                                     node->line, 8, msg, "Use 'import dsa' (lowercase)");
                }
            }
            
            /* Track DSA module usage */
            if (module && strcmp(module, "dsa") == 0) {
                checker->uses_dsa_module = 1;
            }
            
            /* Add to symbol table */
            if (module) {
                lp_symbol_table_add(&checker->symbols, module, node->line, 0, 1);
            }
            break;
        }
            
        case NODE_FUNC_DEF: {
            /* Track function definition */
            char *func_name = node->func_def.name;
            
            if (func_name) {
                if (strcmp(func_name, "main") == 0) {
                    checker->has_main_func = 1;
                    checker->main_func_line = node->line;
                } else if (strcmp(func_name, "solve") == 0) {
                    checker->has_solve_func = 1;
                    checker->solve_func_line = node->line;
                }
                
                /* Track last function for "not called" warning */
                checker->last_func_line = node->line;
                if (checker->last_func_name) free(checker->last_func_name);
                checker->last_func_name = strdup(func_name);
                
                /* Add function to symbol table */
                lp_symbol_table_add(&checker->symbols, func_name, node->line, 1, 0);
            }
            
            /* Check return type annotation */
            if (node->func_def.ret_type) {
                if (strcmp(node->func_def.ret_type, "ptr") == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Invalid return type 'ptr' for function '%s'", 
                             func_name ? func_name : "unknown");
                    lp_error_list_add(checker->errors, ERR_INVALID_TYPE, SEVERITY_ERROR,
                                     node->line, 1, msg, "Use 'list', 'int', 'float', 'str', 'dict', or 'bool'");
                }
            }
            
            /* Check parameter types */
            for (int i = 0; i < node->func_def.params.count; i++) {
                Param *p = &node->func_def.params.items[i];
                if (p->type_ann) {
                    char *base_type = lp_extract_base_type(p->type_ann);
                    if (base_type && strcmp(base_type, "ptr") == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Invalid parameter type 'ptr' for '%s'", p->name);
                        lp_error_list_add(checker->errors, ERR_INVALID_TYPE, SEVERITY_ERROR,
                                         node->line, 1, msg, "Use 'list', 'int', 'float', 'str', 'dict', or 'bool'");
                    }
                    if (base_type && strcmp(base_type, "None") == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Use 'null' instead of 'None' for parameter '%s'", p->name);
                        lp_error_list_add(checker->errors, ERR_USE_NULL_NOT_NONE, SEVERITY_ERROR,
                                         node->line, 1, msg, "LP uses 'null' instead of Python's 'None'");
                    }
                    if (base_type) free(base_type);
                }
            }
            
            /* Enter function context */
            char *old_func = checker->current_function;
            checker->current_function = func_name ? strdup(func_name) : NULL;
            checker->in_function = 1;
            
            /* Register parameters in symbol table BEFORE traversing body */
            for (int i = 0; i < node->func_def.params.count; i++) {
                Param *p = &node->func_def.params.items[i];
                if (p->name) {
                    lp_symbol_table_add(&checker->symbols, p->name, node->line, 0, 0);
                }
            }
            
            /* Traverse function body */
            traverse_list(checker, &node->func_def.body);
            
            /* Exit function context */
            if (checker->current_function) free(checker->current_function);
            checker->current_function = old_func;
            checker->in_function = 0;
            break;
        }
        
        case NODE_CLASS_DEF:
            /* Register class name */
            if (node->class_def.name) {
                lp_symbol_table_add(&checker->symbols, node->class_def.name, node->line, 0, 0);
            }
            traverse_list(checker, &node->class_def.body);
            break;
            
        case NODE_IF:
            traverse_node(checker, node->if_stmt.cond);
            traverse_list(checker, &node->if_stmt.then_body);
            if (node->if_stmt.else_branch) {
                traverse_node(checker, node->if_stmt.else_branch);
            }
            break;
            
        case NODE_FOR:
            /* Register loop variable before traversing body */
            if (node->for_stmt.var) {
                lp_symbol_table_add(&checker->symbols, node->for_stmt.var, node->line, 0, 0);
            }
            traverse_node(checker, node->for_stmt.iter);
            traverse_list(checker, &node->for_stmt.body);
            break;
            
        case NODE_WHILE:
            traverse_node(checker, node->while_stmt.cond);
            traverse_list(checker, &node->while_stmt.body);
            break;
            
        case NODE_RETURN:
            if (node->return_stmt.value) {
                traverse_node(checker, node->return_stmt.value);
            }
            break;
            
        case NODE_ASSIGN: {
            /* Check type annotation */
            if (node->assign.type_ann) {
                char *base_type = lp_extract_base_type(node->assign.type_ann);
                
                if (base_type && strcmp(base_type, "ptr") == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Invalid type 'ptr' for variable '%s'", 
                             node->assign.name ? node->assign.name : "unknown");
                    lp_error_list_add(checker->errors, ERR_INVALID_TYPE, SEVERITY_ERROR,
                                     node->line, 1, msg, "Use 'list', 'int', 'float', 'str', 'dict', or 'bool'");
                }
                
                if (base_type && strcmp(base_type, "None") == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Use 'null' instead of 'None' for variable '%s'",
                             node->assign.name ? node->assign.name : "unknown");
                    lp_error_list_add(checker->errors, ERR_USE_NULL_NOT_NONE, SEVERITY_ERROR,
                                     node->line, 1, msg, "LP uses 'null' instead of Python's 'None'");
                }
                
                if (base_type) free(base_type);
            }
            
            /* Add variable to symbol table */
            if (node->assign.name) {
                lp_symbol_table_add(&checker->symbols, node->assign.name, node->line, 0, 0);
            }
            
            traverse_node(checker, node->assign.value);
            break;
        }
        
        
        case NODE_SUBSCRIPT_ASSIGN:
            /* arr[i] = expr — traverse obj, index, and value to track usage */
            traverse_node(checker, node->subscript_assign.obj);
            traverse_node(checker, node->subscript_assign.index);
            traverse_node(checker, node->subscript_assign.value);
            break;

        case NODE_AUG_ASSIGN:
            traverse_node(checker, node->aug_assign.value);
            break;
            
        case NODE_CALL: {
            /* Check for dsa.flush() */
            if (node->call.func->type == NODE_ATTRIBUTE) {
                AstNode *obj = node->call.func->attribute.obj;
                char *attr = node->call.func->attribute.attr;
                
                if (obj->type == NODE_NAME && obj->name_expr.name &&
                    strcmp(obj->name_expr.name, "dsa") == 0) {
                    
                    if (attr && strcmp(attr, "flush") == 0) {
                        checker->has_dsa_flush = 1;
                        checker->dsa_flush_line = node->line;
                    } else if (attr && strncmp(attr, "write_", 6) == 0) {
                        checker->has_dsa_write = 1;
                        checker->dsa_write_line = node->line;
                    }
                }
            }
            
            /* Check for main() or solve() calls */
            if (node->call.func->type == NODE_NAME) {
                char *func_name = node->call.func->name_expr.name;
                if (func_name) {
                    if (strcmp(func_name, "main") == 0) {
                        checker->calls_main = 1;
                    } else if (strcmp(func_name, "solve") == 0) {
                        checker->calls_solve = 1;
                    }
                    lp_symbol_table_mark_used(&checker->symbols, func_name, node->line);
                }
            }
            
            traverse_node(checker, node->call.func);
            traverse_list(checker, &node->call.args);
            break;
        }
        
        case NODE_ATTRIBUTE:
            traverse_node(checker, node->attribute.obj);
            break;
            
        case NODE_SUBSCRIPT: {
            /* 2D array access arr[i][j] is valid for int[][]/float[][] arrays */
            traverse_node(checker, node->subscript.obj);
            traverse_node(checker, node->subscript.index);
            break;
        }
        
        case NODE_BIN_OP:
            traverse_node(checker, node->bin_op.left);
            traverse_node(checker, node->bin_op.right);
            break;
            
        case NODE_UNARY_OP:
            traverse_node(checker, node->unary_op.operand);
            break;
            
        case NODE_LIST_EXPR:
            traverse_list(checker, &node->list_expr.elems);
            break;
            
        case NODE_DICT_EXPR:
            traverse_list(checker, &node->dict_expr.keys);
            traverse_list(checker, &node->dict_expr.values);
            break;
            
        case NODE_SET_EXPR:
            traverse_list(checker, &node->set_expr.elems);
            break;
            
        case NODE_TUPLE_EXPR:
            traverse_list(checker, &node->tuple_expr.elems);
            break;
            
        case NODE_NAME:
            /* Check for None usage */
            if (node->name_expr.name && strcmp(node->name_expr.name, "None") == 0) {
                lp_error_list_add(checker->errors, ERR_USE_NULL_NOT_NONE, SEVERITY_ERROR,
                                 node->line, 1,
                                 "Use 'null' instead of 'None' in LP",
                                 "LP uses 'null' instead of Python's 'None'");
            }
            
            /* Check for use-before-define */
            if (node->name_expr.name) {
                LpSymbol *sym = lp_symbol_table_find(&checker->symbols, node->name_expr.name);
                if (!sym) {
                    /* Skip builtins and common global names */
                    const char *name = node->name_expr.name;
                    int is_builtin = (
                        strcmp(name, "print") == 0 || strcmp(name, "len") == 0 ||
                        strcmp(name, "range") == 0 || strcmp(name, "input") == 0 ||
                        strcmp(name, "int") == 0 || strcmp(name, "float") == 0 ||
                        strcmp(name, "str") == 0 || strcmp(name, "bool") == 0 ||
                        strcmp(name, "list") == 0 || strcmp(name, "dict") == 0 ||
                        strcmp(name, "set") == 0 || strcmp(name, "tuple") == 0 ||
                        strcmp(name, "type") == 0 || strcmp(name, "abs") == 0 ||
                        strcmp(name, "max") == 0 || strcmp(name, "min") == 0 ||
                        strcmp(name, "sum") == 0 || strcmp(name, "sorted") == 0 ||
                        strcmp(name, "enumerate") == 0 || strcmp(name, "zip") == 0 ||
                        strcmp(name, "map") == 0 || strcmp(name, "filter") == 0 ||
                        strcmp(name, "open") == 0 || strcmp(name, "exit") == 0 ||
                        strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
                        strcmp(name, "null") == 0 || strcmp(name, "self") == 0 ||
                        strcmp(name, "super") == 0 || strcmp(name, "isinstance") == 0 ||
                        strcmp(name, "hasattr") == 0 || strcmp(name, "getattr") == 0 ||
                        strcmp(name, "setattr") == 0 || strcmp(name, "chr") == 0 ||
                        strcmp(name, "ord") == 0 || strcmp(name, "hex") == 0 ||
                        strcmp(name, "round") == 0 || strcmp(name, "reversed") == 0 ||
                        strcmp(name, "any") == 0 || strcmp(name, "all") == 0
                    );
                    if (!is_builtin) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Variable '%s' used before definition", name);
                        lp_error_list_add(checker->errors, WARN_UNUSED_VAR, SEVERITY_WARNING,
                                         node->line, 1, msg, "Define the variable before using it");
                    }
                }
                /* Mark as used if it's a known symbol */
                lp_symbol_table_mark_used(&checker->symbols, node->name_expr.name, node->line);
            }
            break;
            
        case NODE_NONE_LIT:
            /* This is fine - null is valid */
            break;
            
        case NODE_INT_LIT:
        case NODE_FLOAT_LIT:
        case NODE_STRING_LIT:
        case NODE_BOOL_LIT:
        case NODE_FSTRING:
            break;
            
        case NODE_LIST_COMP:
            traverse_node(checker, node->list_comp.expr);
            traverse_node(checker, node->list_comp.iter);
            if (node->list_comp.cond) traverse_node(checker, node->list_comp.cond);
            break;
            
        case NODE_DICT_COMP:
            traverse_node(checker, node->dict_comp.key);
            traverse_node(checker, node->dict_comp.value);
            traverse_node(checker, node->dict_comp.iter);
            if (node->dict_comp.cond) traverse_node(checker, node->dict_comp.cond);
            break;
            
        case NODE_LAMBDA:
            if (node->lambda_expr.body) {
                traverse_node(checker, node->lambda_expr.body);
            }
            traverse_list(checker, &node->lambda_expr.body_stmts);
            break;
            
        case NODE_MATCH:
            traverse_node(checker, node->match_stmt.value);
            traverse_list(checker, &node->match_stmt.cases);
            break;
            
        case NODE_MATCH_CASE:
            /* Register pattern variable if it's a name binding (e.g., case n if n > 10) */
            if (node->match_case.pattern && node->match_case.pattern->type == NODE_NAME) {
                lp_symbol_table_add(&checker->symbols, node->match_case.pattern->name_expr.name, node->line, 0, 0);
            }
            traverse_node(checker, node->match_case.pattern);
            if (node->match_case.guard) traverse_node(checker, node->match_case.guard);
            traverse_list(checker, &node->match_case.body);
            break;
            
        case NODE_PARALLEL_FOR:
            /* Register loop variable before traversing body */
            if (node->parallel_for.var) {
                lp_symbol_table_add(&checker->symbols, node->parallel_for.var, node->line, 0, 0);
            }
            traverse_node(checker, node->parallel_for.iter);
            traverse_list(checker, &node->parallel_for.body);
            break;
            
        case NODE_YIELD:
            if (node->yield_expr.value) {
                traverse_node(checker, node->yield_expr.value);
            }
            break;
            
        case NODE_AWAIT_EXPR:
            traverse_node(checker, node->await_expr.expr);
            break;
            
        case NODE_WITH:
            traverse_node(checker, node->with_stmt.expr);
            /* Register the alias variable from 'with ... as alias:' */
            if (node->with_stmt.alias) {
                lp_symbol_table_add(&checker->symbols, node->with_stmt.alias, node->line, 0, 0);
            }
            traverse_list(checker, &node->with_stmt.body);
            break;
            
        case NODE_TRY:
            traverse_list(checker, &node->try_stmt.body);
            traverse_list(checker, &node->try_stmt.except_body);
            traverse_list(checker, &node->try_stmt.finally_body);
            break;
            
        case NODE_RAISE:
            if (node->raise_stmt.exc) {
                traverse_node(checker, node->raise_stmt.exc);
            }
            break;
            
        case NODE_EXPR_STMT:
            traverse_node(checker, node->expr_stmt.expr);
            break;
            
        case NODE_PASS:
        case NODE_BREAK:
        case NODE_CONTINUE:
            break;
            
        default:
            break;
    }
}

/* Check for unused symbols and report warnings */
void lp_check_unused_symbols(LpSemanticChecker *checker) {
    LpSymbol *sym = checker->symbols.head;
    while (sym) {
        if (!sym->is_used && !sym->is_imported) {
            /* Only warn about unused functions, not variables */
            if (sym->is_function) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Function '%s' is defined but never called", sym->name);
                lp_error_list_add(checker->errors, WARN_UNUSED_VAR, SEVERITY_WARNING,
                                 sym->line_defined, 1, msg, "Add a call to this function");
            }
        }
        sym = sym->next;
    }
}

/* Run all semantic checks on AST */
int lp_semantic_check(LpSemanticChecker *checker, AstNode *program) {
    if (!checker || !program) return 0;
    
    /* First pass: collect all symbols and check basic errors */
    traverse_node(checker, program);
    
    /* Check for DSA module issues */
    if (checker->uses_dsa_module && checker->has_dsa_write && !checker->has_dsa_flush) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Missing 'dsa.flush()' when using DSA write functions. Add 'dsa.flush()' before program exit.");
        
        /* Find the end of the function or program */
        int line = checker->dsa_write_line > 0 ? checker->dsa_write_line + 2 : 1;
        lp_error_list_add(checker->errors, ERR_MISSING_DSA_FLUSH, SEVERITY_ERROR,
                         line, 1, msg, "Add 'dsa.flush()' at the end of your program");
    }
    
    /* Check for main/solve function not called */
    if ((checker->has_main_func && !checker->calls_main) ||
        (checker->has_solve_func && !checker->calls_solve)) {
        
        int func_line = checker->has_solve_func ? checker->solve_func_line : checker->main_func_line;
        char *func_name = checker->has_solve_func ? "solve" : "main";
        
        if (checker->last_func_line > 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Function '%s' is defined but never called", func_name);
            lp_error_list_add(checker->errors, ERR_FUNC_NOT_CALLED, SEVERITY_WARNING,
                             func_line, 1, msg, 
                             "Add a call to this function at the end of your file");
        }
    }
    
    /* Check for unused symbols */
    lp_check_unused_symbols(checker);
    
    return !lp_error_list_has_errors(checker->errors);
}
