#ifndef LP_CODEGEN_H
#define LP_CODEGEN_H

#include "ast.h"
#include <inttypes.h>

/* Dynamic string buffer */
typedef struct {
    char *data;
    int len;
    int cap;
} Buffer;

void buf_init(Buffer *b);
void buf_write(Buffer *b, const char *s);
void buf_printf(Buffer *b, const char *fmt, ...);
void buf_free(Buffer *b);

/* Type tracking */
typedef enum {
    LP_INT, LP_FLOAT, LP_STRING, LP_BOOL, LP_VOID,
    LP_PYOBJ, LP_LIST, LP_ARRAY, LP_STR_ARRAY,
    LP_DICT, LP_SET, LP_TUPLE, LP_FILE, LP_CLASS, LP_OBJECT, LP_VAL, LP_SQLITE_DB,
    LP_THREAD, LP_LOCK, LP_ARENA, LP_POOL, LP_PTR, LP_UNKNOWN,
    /* Native arrays for competitive programming - zero overhead */
    LP_NATIVE_ARRAY_1D, LP_NATIVE_ARRAY_2D,
    LP_NATIVE_ARRAY_FLOAT_1D, LP_NATIVE_ARRAY_FLOAT_2D,
    /* int32 arrays: i32[] annotation - 2x denser than int64, fits L1 cache */
    LP_NATIVE_ARRAY_I32_1D, LP_NATIVE_ARRAY_I32_2D,
    /* float32 arrays: f32[] annotation - 4B vs 8B double, 2x memory bandwidth */
    LP_NATIVE_ARRAY_F32_1D, LP_NATIVE_ARRAY_F32_2D
} LpType;

/* Module import tracking */
typedef enum {
    MOD_TIER1_MATH, MOD_TIER1_RANDOM, MOD_TIER1_TIME,
    MOD_TIER1_OS, MOD_TIER1_SYS, MOD_TIER1_STRING,
    MOD_TIER1_HTTP, MOD_TIER1_JSON, MOD_TIER1_SQLITE,
    MOD_TIER1_THREAD, MOD_TIER1_MEMORY, MOD_TIER1_PLATFORM,
    MOD_TIER1_SECURITY, MOD_TIER1_DSA,
    MOD_TIER2_NUMPY,
    MOD_TIER3_PYTHON
} ModTier;

typedef struct {
    char *module;    /* original module name */
    char *alias;     /* alias used in code (or same as module) */
    ModTier tier;
} ImportInfo;

typedef struct {
    char *name;
    LpType type;
    char *class_name; /* Only used if type == LP_OBJECT */
    int declared;
    int is_function; /* True when symbol represents a function */
    int is_variadic; /* True if function accepts *args */
    int has_kwargs;  /* True if function accepts **kwargs */
    int num_params;  /* Number of total parameters defined */
    LpType param_types[16];
    LpType first_param_type; /* First positional parameter type for function metadata */
    char *first_param_class_name; /* Class name when first_param_type == LP_OBJECT */
    TokenType access;  /* TOK_PRIVATE, TOK_PROTECTED, or 0 (public) */
    char *owner_class; /* Which class owns this member. NULL if global. */
    char *base_class;  /* Parent class constraint if this symbol is an LP_CLASS */
    int is_method;     /* True if the symbol is a class method rather than a field */
    /* Native array dimensions for competitive programming */
    int array_dims;           /* Number of dimensions (0 = not an array, 1 = 1D, 2 = 2D) */
    char *array_size_expr[4]; /* Size expression for each dimension (can be expression) */
} Symbol;

typedef struct Scope Scope;
struct Scope {
    Symbol symbols[512];
    int count;
    Scope *parent;
};

/* Code generator */
typedef struct {
    Buffer header;    /* #include, forward decls */
    Buffer helpers;   /* generated helper structs/wrappers */
    Buffer funcs;     /* function definitions */
    Buffer main_body; /* top-level code goes into main() */
    Scope *scope;
    int had_error;
    char error_msg[512];
    char *current_class; /* Name of the class currently being compiled (for access checks) */
    /* Module tracking */
    ImportInfo imports[64];
    int import_count;
    int uses_python;  /* needs Python/C API (Tier 3) */
    int uses_native;  /* needs lp_native_modules.h (Tier 1/2) */
    int uses_os;      /* needs lp_native_os.h */
    int uses_sys;     /* needs lp_native_sys.h */
    int uses_strings; /* needs lp_native_strings.h */
    int uses_http;    /* needs lp_http.h */
    int uses_json;    /* needs lp_json.h */
    int uses_sqlite;  /* needs lp_sqlite.h */
    int uses_thread;  /* needs lp_thread.h */
    int uses_memory;  /* needs lp_memory.h */
    int uses_platform;/* needs lp_platform.h */
    int uses_parallel;/* needs lp_parallel.h (OpenMP) */
    int uses_gpu;     /* needs lp_gpu.h */
    int uses_security;/* needs lp_security.h */
    int uses_dsa;     /* needs lp_dsa.h (competitive programming / DSA) */
    int has_main;     /* True if main() function was defined by user */
    LpType main_return_type; /* Return type of main() function */
    int thread_adapter_count;
} CodeGen;

void codegen_init(CodeGen *cg);
void codegen_generate(CodeGen *cg, AstNode *program);
void codegen_generate_header(CodeGen *cg, AstNode *program, const char *header_path);
char *codegen_get_output(CodeGen *cg);  /* Returns full C source, caller frees */
void codegen_free(CodeGen *cg);

#endif
