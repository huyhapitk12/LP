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
    LP_PYOBJ, LP_ARRAY, LP_STR_ARRAY,
    LP_DICT, LP_SET, LP_TUPLE, LP_FILE, LP_CLASS, LP_OBJECT, LP_UNKNOWN
} LpType;

/* Module import tracking */
typedef enum {
    MOD_TIER1_MATH, MOD_TIER1_RANDOM, MOD_TIER1_TIME,
    MOD_TIER1_OS, MOD_TIER1_SYS, MOD_TIER1_STRING,
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
    int is_variadic; /* True if function accepts *args */
    int has_kwargs;  /* True if function accepts **kwargs */
    int num_params;  /* Number of total parameters defined */
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
    Buffer funcs;     /* function definitions */
    Buffer main_body; /* top-level code → goes into main() */
    Scope *scope;
    int had_error;
    char error_msg[512];
    /* Module tracking */
    ImportInfo imports[64];
    int import_count;
    int uses_python;  /* needs Python/C API (Tier 3) */
    int uses_native;  /* needs lp_native_modules.h (Tier 1/2) */
    int uses_os;      /* needs lp_native_os.h */
    int uses_sys;     /* needs lp_native_sys.h */
    int uses_strings; /* needs lp_native_strings.h */
} CodeGen;

void codegen_init(CodeGen *cg);
void codegen_generate(CodeGen *cg, AstNode *program);
char *codegen_get_output(CodeGen *cg);  /* Returns full C source, caller frees */
void codegen_free(CodeGen *cg);

#endif
