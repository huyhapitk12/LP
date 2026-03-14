/*
 * LP Compiler - Assembly Code Generator for x86-64
 *
 * Generates optimized x86-64 assembly directly from AST
 * No C intermediate - direct to machine code via assembler
 *
 * Target: x86-64 (System V AMD64 ABI on Linux/macOS, Microsoft x64 on Windows)
 */

#ifndef LP_CODEGEN_ASM_H
#define LP_CODEGEN_ASM_H

#include "ast.h"
#include <stdio.h>
#include <stdint.h>

/* ══════════════════════════════════════════════════════════════
 * Target Platform
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    TARGET_LINUX_X64,      /* System V AMD64 ABI */
    TARGET_WINDOWS_X64,    /* Microsoft x64 calling convention */
    TARGET_MACOS_X64,      /* System V with Mach-O differences */
    TARGET_MACOS_ARM64     /* Apple Silicon - future support */
} AsmTarget;

/* ══════════════════════════════════════════════════════════════
 * x86-64 Registers
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    /* 64-bit general purpose */
    REG_RAX = 0, REG_RCX, REG_RDX, REG_RBX,
    REG_RSP, REG_RBP, REG_RSI, REG_RDI,
    REG_R8, REG_R9, REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,

    /* 32-bit versions (same enum, use mode) */
    REG_EAX = REG_RAX, REG_ECX = REG_RCX, REG_EDX = REG_RDX, REG_EBX = REG_RBX,
    REG_ESP = REG_RSP, REG_EBP = REG_RBP, REG_ESI = REG_RSI, REG_EDI = REG_RDI,
    REG_R8D = REG_R8, REG_R9D = REG_R9, REG_R10D = REG_R10, REG_R11D = REG_R11,
    REG_R12D = REG_R12, REG_R13D = REG_R13, REG_R14D = REG_R14, REG_R15D = REG_R15,

    /* Argument registers (System V) */
    REG_ARG0 = REG_RDI, REG_ARG1 = REG_RSI, REG_ARG2 = REG_RDX,
    REG_ARG3 = REG_RCX, REG_ARG4 = REG_R8, REG_ARG5 = REG_R9,

    /* Return value */
    REG_RET = REG_RAX,

    /* Caller-saved (volatile) */
    /* RAX, RCX, RDX, RSI, RDI, R8-R11 */

    /* Callee-saved (non-volatile) */
    /* RBX, RBP, R12-R15 */

    REG_COUNT = 16
} X64Reg;

typedef enum {
    SIZE_8  = 1,   /* byte  - al, bl, ... */
    SIZE_16 = 2,   /* word  - ax, bx, ... */
    SIZE_32 = 4,   /* dword - eax, ebx, ... */
    SIZE_64 = 8    /* qword - rax, rbx, ... */
} OperandSize;

/* ══════════════════════════════════════════════════════════════
 * Instruction Types
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    /* Data movement */
    I_MOV, I_MOVZX, I_MOVSX, I_LEA,
    I_PUSH, I_POP, I_XCHG,

    /* Arithmetic */
    I_ADD, I_SUB, I_MUL, I_IMUL, I_DIV, I_IDIV,
    I_INC, I_DEC, I_NEG,
    I_ADC, I_SBB,

    /* Logical */
    I_AND, I_OR, I_XOR, I_NOT,
    I_SHL, I_SHR, I_SAR,

    /* Comparison */
    I_CMP, I_TEST,

    /* Control flow */
    I_JMP, I_JE, I_JNE, I_JL, I_JLE, I_JG, I_JGE,
    I_JA, I_JAE, I_JB, I_JBE, I_JZ, I_JNZ,
    I_CALL, I_RET,

    /* Stack frame */
    I_ENTER, I_LEAVE,

    /* String */
    I_NOP, I_CPUID, I_SYSCALL,

    /* SIMD (future optimization) */
    I_MOVDQA, I_PADDD, I_PADDQ, I_MOVAPS, I_ADDPS, I_ADDPD
} Instruction;

/* ══════════════════════════════════════════════════════════════
 * Operand Types
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    OPER_REG,       /* Register: rax, rbx, ... */
    OPER_IMM,       /* Immediate: 42, -100, ... */
    OPER_MEM,       /* Memory: [rbp-8], [rax+rbx*4], ... */
    OPER_LABEL,     /* Label: L1, _main, ... */
    OPER_SYM        /* Symbol: printf, global_var */
} OperandType;

typedef struct {
    OperandType type;
    OperandSize size;
    X64Reg reg;
    int64_t imm;
    struct {
        X64Reg base;
        X64Reg index;      /* -1 if no index */
        int scale;         /* 1, 2, 4, 8 */
        int64_t disp;      /* displacement */
    } mem;
    char *label;
    char *sym;
} Operand;

/* ══════════════════════════════════════════════════════════════
 * Register Allocator
 * ══════════════════════════════════════════════════════════════ */

typedef struct {
    int used;           /* Is this register currently allocated? */
    int spilled;        /* Is this register spilled to stack? */
    int stack_slot;     /* Stack slot if spilled */
    int last_use;       /* Last use position (for liveness) */
    int dirty;          /* Needs to be saved back to memory? */
    char *var_name;     /* Variable name (for debugging) */
} RegAllocEntry;

/* ══════════════════════════════════════════════════════════════
 * Variable Location Tracker
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    LOC_STACK,          /* On stack */
    LOC_REGISTER,       /* In register */
    LOC_IMMEDIATE,      /* Constant value */
    LOC_GLOBAL          /* Global memory */
} VarLocation;

typedef struct {
    char *name;
    VarLocation loc;
    int64_t offset;     /* Stack offset from rbp, or immediate value */
    X64Reg reg;         /* Register if LOC_REGISTER */
    int type_size;      /* Size of the type (1, 2, 4, 8) */
    int is_array;       /* Is this an array? */
    int array_size;     /* Array size if array */
} VarInfo;

/* ══════════════════════════════════════════════════════════════
 * Label Generator
 * ══════════════════════════════════════════════════════════════ */

typedef struct {
    int counter;
    char prefix[32];
} LabelGen;

/* ══════════════════════════════════════════════════════════════
 * Function Context
 * ══════════════════════════════════════════════════════════════ */

typedef struct FuncContext {
    char *name;
    int stack_size;             /* Total stack allocation */
    int param_count;            /* Number of parameters */
    int local_count;            /* Number of local variables */
    int has_varargs;            /* Variadic function? */

    /* Register allocation state */
    RegAllocEntry regs[REG_COUNT];
    int free_regs;              /* Bitmap of free registers */

    /* Variable table */
    VarInfo *vars;
    int var_count;
    int var_capacity;

    /* Stack layout */
    int next_stack_slot;        /* Next available stack slot */

    /* Current location for expression evaluation */
    struct FuncContext *parent;
} FuncContext;

/* ══════════════════════════════════════════════════════════════
 * Assembly Code Generator
 * ══════════════════════════════════════════════════════════════ */

typedef struct {
    /* Output buffer */
    char *code;
    size_t code_len;
    size_t code_cap;

    /* Data section for strings and globals */
    char *data;
    size_t data_len;
    size_t data_cap;

    /* Current function context */
    FuncContext *func;

    /* Target platform */
    AsmTarget target;

    /* Optimization level */
    int opt_level;          /* 0=none, 1=basic, 2=aggressive */

    /* Label generator */
    LabelGen labels;

    /* Error handling */
    int had_error;
    char error_msg[256];

    /* Statistics */
    int func_count;
    int instr_count;

    /* Runtime dependency flags */
    int uses_printf;
    int uses_malloc;
    int uses_memcpy;
    int uses_pthread;
    int uses_openmp;
} AsmCodeGen;

/* ══════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════ */

/* Initialize/destroy */
void asm_codegen_init(AsmCodeGen *gen, AsmTarget target, int opt_level);
void asm_codegen_free(AsmCodeGen *gen);

/* Generate assembly from AST */
int asm_generate(AsmCodeGen *gen, AstNode *program);

/* Get output */
char *asm_get_code(AsmCodeGen *gen);
char *asm_get_full_output(AsmCodeGen *gen);

/* Write to file */
int asm_write_to_file(AsmCodeGen *gen, const char *filename);

/* ══════════════════════════════════════════════════════════════
 * Low-level Instruction Emission
 * ══════════════════════════════════════════════════════════════ */

/* Emit raw string */
void asm_emit(AsmCodeGen *gen, const char *fmt, ...);
void asm_emit_raw(AsmCodeGen *gen, const char *str);

/* Emit instruction with operands */
void asm_emit_instr(AsmCodeGen *gen, Instruction instr, OperandSize size,
                    Operand *dst, Operand *src);

/* Emit common patterns */
void asm_emit_mov(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_add(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_sub(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_cmp(AsmCodeGen *gen, OperandSize size, Operand *left, Operand *right);
void asm_emit_jmp(AsmCodeGen *gen, const char *label);
void asm_emit_jcc(AsmCodeGen *gen, Instruction cond, const char *label);
void asm_emit_call(AsmCodeGen *gen, const char *target);
void asm_emit_ret(AsmCodeGen *gen);

/* ══════════════════════════════════════════════════════════════
 * Function Codegen
 * ══════════════════════════════════════════════════════════════ */

void asm_func_prologue(AsmCodeGen *gen, const char *name, int stack_size);
void asm_func_epilogue(AsmCodeGen *gen);
void asm_func_alloc_stack(AsmCodeGen *gen, int size);

/* ══════════════════════════════════════════════════════════════
 * Register Allocation
 * ══════════════════════════════════════════════════════════════ */

X64Reg asm_alloc_reg(AsmCodeGen *gen);
void asm_free_reg(AsmCodeGen *gen, X64Reg reg);
void asm_spill_reg(AsmCodeGen *gen, X64Reg reg);
X64Reg asm_ensure_in_reg(AsmCodeGen *gen, const char *var_name);

/* ══════════════════════════════════════════════════════════════
 * Variable Management
 * ══════════════════════════════════════════════════════════════ */

int asm_add_local_var(AsmCodeGen *gen, const char *name, int type_size);
VarInfo *asm_lookup_var(AsmCodeGen *gen, const char *name);
int asm_get_var_offset(AsmCodeGen *gen, const char *name);

/* ══════════════════════════════════════════════════════════════
 * Label Management
 * ══════════════════════════════════════════════════════════════ */

char *asm_new_label(AsmCodeGen *gen, char *buf, size_t buf_size);
void asm_emit_label(AsmCodeGen *gen, const char *label);

/* ══════════════════════════════════════════════════════════════
 * Operand Constructors
 * ══════════════════════════════════════════════════════════════ */

Operand oper_reg(X64Reg reg, OperandSize size);
Operand oper_imm(int64_t value);
Operand oper_mem(X64Reg base, X64Reg index, int scale, int64_t disp, OperandSize size);
Operand oper_mem_base_disp(X64Reg base, int64_t disp, OperandSize size);
Operand oper_label(const char *label);
Operand oper_sym(const char *sym);

/* ══════════════════════════════════════════════════════════════
 * Register Name Helpers
 * ══════════════════════════════════════════════════════════════ */

const char *reg_name(X64Reg reg, OperandSize size);

/* ══════════════════════════════════════════════════════════════
 * Compile to executable
 * ══════════════════════════════════════════════════════════════ */

int asm_compile_to_exe(AsmCodeGen *gen, const char *asm_file, const char *exe_file);

#endif /* LP_CODEGEN_ASM_H */
