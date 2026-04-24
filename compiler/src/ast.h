#ifndef LP_AST_H
#define LP_AST_H

#include <stdint.h>
#include "lexer.h"
#include "lp_memory.h"

typedef enum {
    NODE_PROGRAM, NODE_FUNC_DEF, NODE_CLASS_DEF, NODE_IF, NODE_FOR, NODE_WHILE,
    NODE_RETURN, NODE_ASSIGN, NODE_AUG_ASSIGN, NODE_SUBSCRIPT_ASSIGN, NODE_EXPR_STMT,
    NODE_PASS, NODE_BREAK, NODE_CONTINUE, NODE_CONST_DECL,
    NODE_IMPORT, NODE_WITH,
    /* Expressions */
    NODE_BIN_OP, NODE_UNARY_OP, NODE_CALL, NODE_NAME, NODE_KWARG,
    NODE_INT_LIT, NODE_FLOAT_LIT, NODE_STRING_LIT, NODE_FSTRING,
    NODE_BOOL_LIT, NODE_NONE_LIT, NODE_LIST_EXPR,
    NODE_DICT_EXPR, NODE_SET_EXPR, NODE_TUPLE_EXPR,
    NODE_SUBSCRIPT, NODE_ATTRIBUTE, NODE_SLICE,
    NODE_TRY, NODE_RAISE,
    NODE_LIST_COMP, NODE_DICT_COMP, NODE_LAMBDA, NODE_YIELD,
    NODE_PARALLEL_FOR,
    /* Settings/Pragma */
    NODE_SETTINGS, NODE_PARALLEL_SETTINGS,
    /* Security/Pragma */
    NODE_SECURITY,
    /* Pattern Matching */
    NODE_MATCH, NODE_MATCH_CASE,
    /* Async/Await */
    NODE_ASYNC_DEF, NODE_AWAIT_EXPR,
    /* Type Annotations */
    NODE_TYPE_UNION,
    /* Generic Types */
    NODE_GENERIC_INST,  /* Generic instantiation: Box[int] */
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

/* Type parameter for generics (e.g., T in class Box[T]) */
typedef struct {
    char *name;      /* Type parameter name (e.g., "T") */
} TypeParam;

typedef struct {
    TypeParam *items;
    int count;
    int capacity;
} TypeParamList;

typedef struct {
    Param *items;
    int count;
    int capacity;
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
        struct { char *name; TypeParamList type_params; ParamList params; char *ret_type; NodeList body; TokenType access; NodeList decorators; } func_def;
        struct { char *name; TypeParamList type_params; char *base_class; NodeList body; } class_def;
        struct { AstNode *cond; NodeList then_body; AstNode *else_branch; } if_stmt;
        struct { char *var; AstNode *iter; NodeList body; } for_stmt;
        struct { AstNode *cond; NodeList body; } while_stmt;
        struct { AstNode *value; } return_stmt;
        struct { char *name; char *type_ann; AstNode *value; TokenType access; } assign;
        struct { char *name; TokenType op; AstNode *value; } aug_assign;
        struct { AstNode *obj; AstNode *index; AstNode *value; TokenType op; } subscript_assign;
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
        struct { NodeList parts; } fstring_expr;  /* F-string: alternating string parts and expressions */
        struct { int value; } bool_lit;
        struct { NodeList elems; } list_expr;
        struct { NodeList keys; NodeList values; } dict_expr;
        struct { NodeList elems; } set_expr;
        struct { NodeList elems; } tuple_expr;
        struct { AstNode *obj; AstNode *index; } subscript;
        struct { AstNode *obj; AstNode *start; AstNode *stop; AstNode *step; } slice;
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
        /* Parallel for with settings */
        struct {
            char *var;
            AstNode *iter;
            NodeList body;
            /* Parallel settings */
            int num_threads;        /* 0 = auto */
            char *schedule;         /* static, dynamic, guided, auto */
            int64_t chunk_size;     /* 0 = auto */
            int device_type;        /* 0=CPU, 1=GPU, 2=Auto */
            int gpu_id;             /* GPU device ID */
        } parallel_for;
        /* Settings/Pragma for parallel and GPU execution */
        struct {
            int enabled;              /* Enable parallel execution */
            int num_threads;          /* Number of threads (0 = auto) */
            char *schedule;           /* Scheduling policy: static, dynamic, guided, auto */
            int64_t chunk_size;       /* Chunk size for scheduling (0 = auto) */
            int device_type;          /* 0=CPU, 1=GPU, 2=Auto */
            int gpu_id;               /* GPU device ID */
            int unified_memory;       /* Use unified memory for GPU */
            int async_transfer;       /* Enable async memory transfer */
        } settings;
        /* Security/Pragma for input validation and access control */
        struct {
            int enabled;              /* Enable security checks */
            int level;                /* Security level: 0=none, 1=low, 2=medium, 3=high, 4=critical */
            int require_auth;         /* Require authentication */
            char *auth_type;          /* Auth type: "basic", "bearer", "api_key", "oauth" */
            int rate_limit;           /* Rate limit: max requests per minute */
            int validate_input;       /* Enable input validation */
            int sanitize_output;      /* Enable output sanitization */
            int prevent_injection;    /* Prevent SQL/command injection */
            int prevent_xss;          /* Prevent XSS attacks */
            int prevent_csrf;         /* Prevent CSRF attacks */
            int enable_cors;          /* Enable CORS headers */
            char *cors_origins;       /* Allowed CORS origins */
            int secure_headers;       /* Enable security headers */
            int encrypt_data;         /* Enable data encryption */
            char *hash_algorithm;     /* Hash algorithm: "md5", "sha256", "sha512" */
            int access_level;         /* Access level: 0=guest, 1=user, 2=admin, 3=super */
            int readonly;             /* Read-only access mode */
        } security;
        /* Pattern Matching: match value: case pattern: body */
        struct {
            AstNode *value;           /* The value being matched */
            NodeList cases;           /* List of NODE_MATCH_CASE nodes */
        } match_stmt;
        /* Match case: case pattern [if guard]: body */
        struct {
            AstNode *pattern;         /* Pattern to match (literal, name, or _ for wildcard) */
            AstNode *guard;           /* Optional guard expression (NULL if none) */
            NodeList body;            /* Body statements */
            int is_wildcard;          /* 1 if pattern is _ (matches anything) */
        } match_case;
        /* Async function definition: async def name(params): body */
        struct {
            char *name;               /* Function name */
            ParamList params;         /* Parameters */
            char *ret_type;           /* Return type annotation (NULL if none) */
            NodeList body;            /* Function body */
            TokenType access;         /* Access modifier (0, TOK_PRIVATE, TOK_PROTECTED) */
            NodeList decorators;      /* Decorator list */
        } async_def;
        /* Await expression: await expr */
        struct {
            AstNode *expr;            /* Expression to await */
        } await_expr;
        /* Type union: int | str | float */
        struct {
            char **types;             /* Array of type name strings */
            int count;                /* Number of types in the union */
        } type_union;
        /* Generic type instantiation: Box[int] or Box[T, int] */
        struct {
            char *base_name;          /* Base type name (e.g., "Box") */
            NodeList type_args;       /* Type arguments (list of NODE_NAME nodes) */
        } generic_inst;
    };
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
};

/* NodeList helpers */
void node_list_init(NodeList *list);
void node_list_push(NodeList *list, AstNode *node);

/* TypeParamList helpers */
void type_param_list_init(TypeParamList *list);
void type_param_list_push(TypeParamList *list, TypeParam p);

/* ParamList helpers */
void param_list_init(ParamList *list);
void param_list_push(ParamList *list, Param p);

/* Node constructors */
AstNode *ast_new(LpArena *arena, NodeType type, int line);
void ast_free(AstNode *node);

/* Debug: print the AST tree to stdout */
void ast_dump(AstNode *node, int indent);

#endif
