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

    REG_COUNT = 16
} X64Reg;

typedef enum {
    SIZE_8  = 1,
    SIZE_16 = 2,
    SIZE_32 = 4,
    SIZE_64 = 8
} OperandSize;

/* ══════════════════════════════════════════════════════════════
 * Instruction Types
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    I_MOV, I_MOVZX, I_MOVSX, I_LEA,
    I_PUSH, I_POP, I_XCHG,
    I_ADD, I_SUB, I_MUL, I_IMUL, I_DIV, I_IDIV,
    I_INC, I_DEC, I_NEG, I_ADC, I_SBB,
    I_AND, I_OR, I_XOR, I_NOT,
    I_SHL, I_SHR, I_SAR,
    I_CMP, I_TEST,
    I_JMP, I_JE, I_JNE, I_JL, I_JLE, I_JG, I_JGE,
    I_JA, I_JAE, I_JB, I_JBE, I_JZ, I_JNZ,
    I_CALL, I_RET,
    I_ENTER, I_LEAVE,
    I_NOP, I_CPUID, I_SYSCALL
} Instruction;

/* ══════════════════════════════════════════════════════════════
 * Operand Types
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    OPER_REG, OPER_IMM, OPER_MEM, OPER_LABEL, OPER_SYM
} OperandType;

typedef struct {
    OperandType type;
    OperandSize size;
    X64Reg reg;
    int64_t imm;
    struct {
        X64Reg base;
        X64Reg index;
        int scale;
        int64_t disp;
    } mem;
    char *label;
    char *sym;
} Operand;

/* ══════════════════════════════════════════════════════════════
 * Variable Location
 * ══════════════════════════════════════════════════════════════ */

typedef enum {
    LOC_STACK, LOC_REGISTER, LOC_IMMEDIATE, LOC_GLOBAL
} VarLocation;

typedef struct {
    char *name;
    VarLocation loc;
    int64_t offset;
    X64Reg reg;
    int type_size;
    int is_array;
    int array_size;
    int is_constant;
    int64_t const_value;
} VarInfo;

/* ══════════════════════════════════════════════════════════════
 * Register Allocation Entry
 * ══════════════════════════════════════════════════════════════ */

typedef struct {
    int used;
    int spilled;
    int stack_slot;
    int last_use;
    int dirty;
    char *var_name;
} RegAllocEntry;

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
    int stack_size;
    int param_count;
    int local_count;
    int has_varargs;
    RegAllocEntry regs[16];
    int free_regs;
    VarInfo *vars;
    int var_count;
    int var_capacity;
    int next_stack_slot;
    struct FuncContext *parent;
} FuncContext;

/* ══════════════════════════════════════════════════════════════
 * Assembly Code Generator
 * ══════════════════════════════════════════════════════════════ */

typedef struct {
    char *code;
    size_t code_len;
    size_t code_cap;
    char *data;
    size_t data_len;
    size_t data_cap;
    FuncContext *func;
    AsmTarget target;
    int opt_level;
    int constants_folded;
    int dead_code_removed;
    int loops_unrolled;
    LabelGen labels;
    int had_error;
    char error_msg[256];
    int func_count;
    int instr_count;
    int uses_printf;
    int uses_malloc;
    int uses_memcpy;
    int uses_pthread;
    int uses_openmp;
} AsmCodeGen;

/* ══════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════ */

void asm_codegen_init(AsmCodeGen *gen, AsmTarget target, int opt_level);
void asm_codegen_free(AsmCodeGen *gen);
int asm_generate(AsmCodeGen *gen, AstNode *program);
char *asm_get_code(AsmCodeGen *gen);
char *asm_get_full_output(AsmCodeGen *gen);
int asm_write_to_file(AsmCodeGen *gen, const char *filename);

/* Instruction emission */
void asm_emit(AsmCodeGen *gen, const char *fmt, ...);
void asm_emit_raw(AsmCodeGen *gen, const char *str);
void asm_emit_instr(AsmCodeGen *gen, Instruction instr, OperandSize size, Operand *dst, Operand *src);
void asm_emit_mov(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_add(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_sub(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src);
void asm_emit_cmp(AsmCodeGen *gen, OperandSize size, Operand *left, Operand *right);
void asm_emit_jmp(AsmCodeGen *gen, const char *label);
void asm_emit_jcc(AsmCodeGen *gen, Instruction cond, const char *label);
void asm_emit_call(AsmCodeGen *gen, const char *target);
void asm_emit_ret(AsmCodeGen *gen);

/* Function codegen */
void asm_func_prologue(AsmCodeGen *gen, const char *name, int stack_size);
void asm_func_epilogue(AsmCodeGen *gen);
void asm_func_alloc_stack(AsmCodeGen *gen, int size);

/* Register allocation */
X64Reg asm_alloc_reg(AsmCodeGen *gen);
void asm_free_reg(AsmCodeGen *gen, X64Reg reg);
void asm_spill_reg(AsmCodeGen *gen, X64Reg reg);
X64Reg asm_ensure_in_reg(AsmCodeGen *gen, const char *var_name);

/* Variable management */
int asm_add_local_var(AsmCodeGen *gen, const char *name, int type_size);
VarInfo *asm_lookup_var(AsmCodeGen *gen, const char *name);
int asm_get_var_offset(AsmCodeGen *gen, const char *name);

/* Label management */
char *asm_new_label(AsmCodeGen *gen, char *buf, size_t buf_size);
void asm_emit_label(AsmCodeGen *gen, const char *label);

/* Operand constructors */
Operand oper_reg(X64Reg reg, OperandSize size);
Operand oper_imm(int64_t value);
Operand oper_mem(X64Reg base, X64Reg index, int scale, int64_t disp, OperandSize size);
Operand oper_mem_base_disp(X64Reg base, int64_t disp, OperandSize size);
Operand oper_label(const char *label);
Operand oper_sym(const char *sym);

/* Register name helper */
const char *reg_name(X64Reg reg, OperandSize size);

/* Compile to executable */
int asm_compile_to_exe(AsmCodeGen *gen, const char *asm_file, const char *exe_file);

/* ══════════════════════════════════════════════════════════════
 * Optimization Functions (in asm_optimize.c)
 * ══════════════════════════════════════════════════════════════ */

int asm_is_constant_expr(AstNode *node);
int64_t asm_get_constant_value(AstNode *node);
int asm_opt_unroll_loop(AstNode *node, int max_iterations);
int asm_optimize_ast(AsmCodeGen *gen, AstNode *node);

#endif /* LP_CODEGEN_ASM_H */
