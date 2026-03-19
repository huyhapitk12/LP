/*
 * LP Semantic Checker - Validates semantics and detects common errors
 * Checks for: invalid types, missing flush, unused functions, etc.
 */
#ifndef LP_SEMANTIC_CHECK_H
#define LP_SEMANTIC_CHECK_H

#include "ast.h"
#include "error_reporter.h"

/* Valid types in LP */
#define LP_VALID_TYPES_COUNT 10
extern const char *LP_VALID_TYPES[];

/* Valid modules in LP */
#define LP_VALID_MODULES_COUNT 10
extern const char *LP_VALID_MODULES[];

/* Symbol table entry */
typedef struct LpSymbol {
    char *name;
    int line_defined;
    int line_used;
    int is_function;
    int is_imported;
    int is_used;
    struct LpSymbol *next;
} LpSymbol;

/* Symbol table */
typedef struct {
    LpSymbol *head;
    LpSymbol *tail;
    int count;
} LpSymbolTable;

/* Semantic checker context */
typedef struct {
    LpErrorList *errors;
    LpSymbolTable symbols;
    const char *source;
    
    /* Flags for DSA module checks */
    int uses_dsa_module;
    int has_dsa_flush;
    int has_dsa_write;
    int dsa_write_line;
    int dsa_flush_line;
    
    /* Current function context */
    char *current_function;
    int in_function;
    
    /* For tracking main/solve functions */
    int has_main_func;
    int has_solve_func;
    int main_func_line;
    int solve_func_line;
    int calls_main;
    int calls_solve;
    
    /* For unused variable tracking */
    int last_func_line;
    char *last_func_name;
} LpSemanticChecker;

/* Initialize semantic checker */
void lp_semantic_init(LpSemanticChecker *checker, LpErrorList *errors, const char *source);

/* Free semantic checker */
void lp_semantic_free(LpSemanticChecker *checker);

/* Run all semantic checks on AST */
int lp_semantic_check(LpSemanticChecker *checker, AstNode *program);

/* Individual checks */
void lp_check_valid_types(LpSemanticChecker *checker, AstNode *node);
void lp_check_valid_modules(LpSemanticChecker *checker, AstNode *node);
void lp_check_dsa_flush(LpSemanticChecker *checker, AstNode *node);
void lp_check_function_calls(LpSemanticChecker *checker, AstNode *node);
void lp_check_none_vs_null(LpSemanticChecker *checker, AstNode *node);
void lp_check_2d_array_access(LpSemanticChecker *checker, AstNode *node);
void lp_check_type_annotations(LpSemanticChecker *checker, AstNode *node);
void lp_check_assignment_in_condition(LpSemanticChecker *checker, AstNode *node);

/* Symbol table functions */
void lp_symbol_table_init(LpSymbolTable *table);
void lp_symbol_table_add(LpSymbolTable *table, const char *name, int line, int is_function, int is_imported);
LpSymbol *lp_symbol_table_find(LpSymbolTable *table, const char *name);
void lp_symbol_table_mark_used(LpSymbolTable *table, const char *name, int line);
void lp_symbol_table_free(LpSymbolTable *table);

/* Check for unused symbols and report warnings */
void lp_check_unused_symbols(LpSemanticChecker *checker);

/* Helper: Check if type is valid */
int lp_is_valid_type(const char *type_name);

/* Helper: Check if module is valid */
int lp_is_valid_module(const char *module_name);

/* Helper: Extract type from annotation string */
char *lp_extract_base_type(const char *type_annotation);

#endif /* LP_SEMANTIC_CHECK_H */
