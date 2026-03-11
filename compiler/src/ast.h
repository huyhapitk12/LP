#ifndef LP_AST_H
#define LP_AST_H

#include <stdint.h>
#include "lexer.h"

typedef enum {
    NODE_PROGRAM, NODE_FUNC_DEF, NODE_CLASS_DEF, NODE_IF, NODE_FOR, NODE_WHILE,
    NODE_RETURN, NODE_ASSIGN, NODE_AUG_ASSIGN, NODE_EXPR_STMT,
    NODE_PASS, NODE_BREAK, NODE_CONTINUE, NODE_CONST_DECL,
    NODE_IMPORT, NODE_WITH,
    /* Expressions */
    NODE_BIN_OP, NODE_UNARY_OP, NODE_CALL, NODE_NAME, NODE_KWARG,
    NODE_INT_LIT, NODE_FLOAT_LIT, NODE_STRING_LIT,
    NODE_BOOL_LIT, NODE_NONE_LIT, NODE_LIST_EXPR,
    NODE_DICT_EXPR, NODE_SET_EXPR, NODE_TUPLE_EXPR,
    NODE_SUBSCRIPT, NODE_ATTRIBUTE,
    NODE_TRY, NODE_RAISE,
    NODE_LIST_COMP, NODE_DICT_COMP, NODE_LAMBDA, NODE_YIELD,
    NODE_PARALLEL_FOR,
} NodeType;

typedef struct AstNode AstNode;

typedef struct {
    AstNode **items;
    int count;
    int cap;
} NodeList;

typedef struct {
    char *name;
    char *type_ann;  /* NULL if not annotated */
    int is_vararg;   /* Evaluates to true if *args */
    int is_kwarg;    /* Evaluates to true if **kwargs */
} Param;

typedef struct {
    Param *items;
    int count;
    int cap;
} ParamList;

struct AstNode {
    NodeType type;
    int line;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    union {
        struct { NodeList stmts; } program;
        struct { char *name; ParamList params; char *ret_type; NodeList body; TokenType access; } func_def;
        struct { char *name; char *base_class; NodeList body; } class_def;
        struct { AstNode *cond; NodeList then_body; AstNode *else_branch; } if_stmt;
        struct { char *var; AstNode *iter; NodeList body; } for_stmt;
        struct { AstNode *cond; NodeList body; } while_stmt;
        struct { AstNode *value; } return_stmt;
        struct { char *name; char *type_ann; AstNode *value; TokenType access; } assign;
        struct { char *name; TokenType op; AstNode *value; } aug_assign;
        struct { AstNode *expr; } expr_stmt;
        struct { char *name; AstNode *value; } const_decl;
        struct { AstNode *left; TokenType op; AstNode *right; } bin_op;
        struct { TokenType op; AstNode *operand; } unary_op;
        struct { char *name; AstNode *value; } kwarg;
        struct { 
            AstNode *func; 
            NodeList args; 
            int *is_unpacked_list; /* array of size args.count, 1 if *arg */
            int *is_unpacked_dict; /* array of size args.count, 1 if **arg */
        } call;
        struct { char *name; } name_expr;
        struct { int64_t value; } int_lit;
        struct { double value; } float_lit;
        struct { char *value; } str_lit;
        struct { int value; } bool_lit;
        struct { NodeList elems; } list_expr;
        struct { NodeList keys; NodeList values; } dict_expr;
        struct { NodeList elems; } set_expr;
        struct { NodeList elems; } tuple_expr;
        struct { AstNode *obj; AstNode *index; } subscript;
        struct { AstNode *obj; char *attr; } attribute;
        struct { char *module; char *alias; /* NULL = use module name */ } import_stmt;
        struct { AstNode *expr; char *alias; NodeList body; } with_stmt;
        struct { NodeList body; NodeList except_body; char *except_type; char *except_alias; NodeList finally_body; } try_stmt;
        struct { AstNode *exc; } raise_stmt;
        /* List comprehension: [expr for var in iter if cond] */
        struct { AstNode *expr; char *var; AstNode *iter; AstNode *cond; } list_comp;
        /* Dict comprehension: {key: val for var in iter if cond} */
        struct { AstNode *key; AstNode *value; char *var; AstNode *iter; AstNode *cond; } dict_comp;
        /* Lambda: lambda params: expr  OR  lambda params:\n            block */
        struct { ParamList params; AstNode *body; NodeList body_stmts; int is_multiline; } lambda_expr;
        /* Yield */
        struct { AstNode *value; } yield_expr;
    };
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
};

/* NodeList helpers */
void node_list_init(NodeList *list);
void node_list_push(NodeList *list, AstNode *node);

/* ParamList helpers */
void param_list_init(ParamList *list);
void param_list_push(ParamList *list, Param p);

/* Node constructors */
AstNode *ast_new(NodeType type, int line);
void ast_free(AstNode *node);

#endif
