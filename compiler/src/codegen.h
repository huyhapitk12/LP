#include <stdint.h>
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
  LP_INT,
  LP_FLOAT,
  LP_STRING,
  LP_BOOL,
  LP_VOID,
  LP_PYOBJ,
  LP_LIST,
  LP_ARRAY,
  LP_TENSOR,
  LP_STR_ARRAY,
  LP_DICT,
  LP_SET,
  LP_TUPLE,
  LP_FILE,
  LP_CLASS,
  LP_OBJECT,
  LP_VAL,
  LP_SQLITE_DB,
  LP_THREAD,
  LP_LOCK,
  LP_ARENA,
  LP_POOL,
  LP_PTR,
  LP_UNKNOWN,
  /* Native arrays for competitive programming - zero overhead */
  LP_NATIVE_ARRAY_1D,
  LP_NATIVE_ARRAY_2D,
  LP_NATIVE_ARRAY_FLOAT_1D,
  LP_NATIVE_ARRAY_FLOAT_2D,
  /* int32 arrays: i32[] annotation - 2x denser than int64, fits L1 cache */
  LP_NATIVE_ARRAY_I32_1D,
  LP_NATIVE_ARRAY_I32_2D,
  /* float32 arrays: f32[] annotation - 4B vs 8B double, 2x memory bandwidth */
  LP_NATIVE_ARRAY_F32_1D,
  LP_NATIVE_ARRAY_F32_2D,
  /* int8 arrays: i8[] annotation - 1B per element, 8x denser than int64 (sieve,
     visited) */
  LP_NATIVE_ARRAY_I8_1D
} LpType;

/* Module import tracking */
typedef enum {
  MOD_TIER1_MATH,
  MOD_TIER1_RANDOM,
  MOD_TIER1_TIME,
  MOD_TIER1_OS,
  MOD_TIER1_SYS,
  MOD_TIER1_STRING,
  MOD_TIER1_HTTP,
  MOD_TIER1_JSON,
  MOD_TIER1_SQLITE,
  MOD_TIER1_THREAD,
  MOD_TIER1_MEMORY,
  MOD_TIER1_PLATFORM,
  MOD_TIER1_SECURITY,
  MOD_TIER1_DSA,
  MOD_TIER1_REGEX,
  MOD_TIER1_HASHLIB,
  MOD_TIER1_BASE64,
  MOD_TIER1_CSV,
  MOD_TIER1_DATETIME,
  MOD_TIER1_SOCKET,
  MOD_TIER1_LOGGING,
  MOD_TIER1_UNITTEST,
  MOD_TIER1_GUI,
  MOD_TIER2_NUMPY,
  MOD_TIER2_TENSOR,
  MOD_TIER3_PYTHON
} ModTier;

typedef struct {
  char *module; /* original module name */
  char *alias;  /* alias used in code (or same as module) */
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
  LpType first_param_type;      /* First positional parameter type for function
                                   metadata */
  char *first_param_class_name; /* Class name when first_param_type == LP_OBJECT
                                 */
  TokenType access;             /* TOK_PRIVATE, TOK_PROTECTED, or 0 (public) */
  char *owner_class; /* Which class owns this member. NULL if global. */
  char *base_class;  /* Parent class constraint if this symbol is an LP_CLASS */
  int is_method; /* True if the symbol is a class method rather than a field */
  /* Native array dimensions for competitive programming */
  int array_dims; /* Number of dimensions (0 = not an array, 1 = 1D, 2 = 2D) */
  char *array_size_expr[4]; /* Size expression for each dimension (can be
                               expression) */
  /* Fix 4: constant propagation */
  int64_t const_int_value; /* 0 = not a known constant */
  /* Fix 7: loop-local array hoisting — array declared inside for/while, hoisted
   * out */
  int is_loop_hoisted; /* 1 = was declared inside a loop, hoisted to function
                          scope */
  int non_negative;    /* 1 = known >= 0: range var, non-neg literal, or
                          range-derived */
  uint32_t non_negative_param_mask; /* bitmask: bit i=1 means param[i] is always
                                       non-negative at all call sites */
  int is_param;         /* 1 if symbol is a function parameter (default LP_VAL) */
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
  Buffer globals;   /* file-scope globals for top-level state */
  Buffer helpers;   /* generated helper structs/wrappers */
  Buffer funcs;     /* function definitions */
  Buffer main_body; /* top-level code goes into main() */
  Scope *scope;
  int had_error;
  char error_msg[512];
  char *current_class; /* Name of the class currently being compiled (for access checks) */
  const char *source_file; /* Original LP source file name for #line directives */
  /* Module tracking */
  ImportInfo imports[64];
  int import_count;
  int uses_python;   /* needs Python/C API (Tier 3) */
  int uses_native;   /* needs lp_native_modules.h (Tier 1/2) */
  int uses_os;       /* needs lp_native_os.h */
  int uses_sys;      /* needs lp_native_sys.h */
  int uses_strings;  /* needs lp_native_strings.h */
  int uses_http;     /* needs lp_http.h */
  int uses_json;     /* needs lp_json.h */
  int uses_sqlite;   /* needs lp_sqlite.h */
  int uses_thread;   /* needs lp_thread.h */
  int uses_memory;   /* needs lp_memory.h */
  int uses_platform; /* needs lp_platform.h */
  int uses_parallel; /* needs lp_parallel.h (OpenMP) */
  int uses_gpu;      /* needs lp_gpu.h */
  int uses_security; /* needs lp_security.h */
  int uses_dsa;      /* needs lp_dsa.h (competitive programming / DSA) */
  int uses_regex;
  int uses_hashlib;
  int uses_base64;
  int uses_csv;
  int uses_datetime;
  int uses_socket;
  int uses_logging;
  int uses_unittest;
  int uses_gui;
  int uses_tensor;   /* needs lp_tensor.h */
  int has_main;            /* True if main() function was defined by user */
  LpType main_return_type; /* Return type of main() function */
  LpType current_func_ret; /* Return type of the function currently being compiled
                              (for type-safe return wrapping) */
  int thread_adapter_count;
  /* Hybrid ASM: functions compiled directly to ASM object files */
  char *asm_compiled_funcs[32];
  int asm_compiled_func_count;
} CodeGen;

void codegen_init(CodeGen *cg);
void codegen_generate(CodeGen *cg, AstNode *program);
void codegen_generate_header(CodeGen *cg, AstNode *program,
                             const char *header_path);
char *codegen_get_output(CodeGen *cg); /* Returns full C source, caller frees */
void codegen_free(CodeGen *cg);

#endif
