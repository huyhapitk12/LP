/*
 * LP Compiler - Assembly Code Generator for x86-64
 *
 * Direct to assembly compilation - no C intermediate
 * Optimized output using register allocation and instruction selection
 */

#define _GNU_SOURCE
#include "codegen_asm.h"
#include "asm_optimize.h"
#include "process_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* ══════════════════════════════════════════════════════════════
 * Register Names Table
 * ══════════════════════════════════════════════════════════════ */

static const char *reg_names_64[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};

static const char *reg_names_32[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};

static const char *reg_names_16[] = {
    "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};

static const char *reg_names_8[] = {
    "al",  "cl",  "dl",  "bl",  "spl", "bpl", "sil", "dil",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
};

const char *reg_name(X64Reg reg, OperandSize size) {
    if (reg < 0 || reg >= REG_COUNT) return "???";
    switch (size) {
        case SIZE_8:  return reg_names_8[reg];
        case SIZE_16: return reg_names_16[reg];
        case SIZE_32: return reg_names_32[reg];
        case SIZE_64: return reg_names_64[reg];
        default:      return reg_names_64[reg];
    }
}

/* ══════════════════════════════════════════════════════════════
 * Operand Constructors
 * ══════════════════════════════════════════════════════════════ */

Operand oper_reg(X64Reg reg, OperandSize size) {
    Operand op;
    op.type = OPER_REG;
    op.size = size;
    op.reg = reg;
    return op;
}

Operand oper_imm(int64_t value) {
    Operand op;
    op.type = OPER_IMM;
    op.size = SIZE_64;
    op.imm = value;
    return op;
}

Operand oper_mem(X64Reg base, X64Reg index, int scale, int64_t disp, OperandSize size) {
    Operand op;
    op.type = OPER_MEM;
    op.size = size;
    op.mem.base = base;
    op.mem.index = index;
    op.mem.scale = scale;
    op.mem.disp = disp;
    return op;
}

Operand oper_mem_base_disp(X64Reg base, int64_t disp, OperandSize size) {
    return oper_mem(base, -1, 0, disp, size);
}

Operand oper_label(const char *label) {
    Operand op;
    op.type = OPER_LABEL;
    op.size = SIZE_64;
    op.label = strdup(label);
    return op;
}

Operand oper_sym(const char *sym) {
    Operand op;
    op.type = OPER_SYM;
    op.size = SIZE_64;
    op.sym = strdup(sym);
    return op;
}

/* ══════════════════════════════════════════════════════════════
 * String Buffer Helpers
 * ══════════════════════════════════════════════════════════════ */

static void sb_ensure_cap(char **buf, size_t *len, size_t *cap, size_t need) {
    if (*len + need >= *cap) {
        size_t new_cap = *cap * 2;
        if (new_cap < *len + need + 1024) new_cap = *len + need + 1024;
        *buf = realloc(*buf, new_cap);
        *cap = new_cap;
    }
}

static void sb_append(char **buf, size_t *len, size_t *cap, const char *str) {
    size_t slen = strlen(str);
    sb_ensure_cap(buf, len, cap, slen + 1);
    memcpy(*buf + *len, str, slen + 1);
    *len += slen;
}

static void sb_vprintf(char **buf, size_t *len, size_t *cap, const char *fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (need > 0) {
        sb_ensure_cap(buf, len, cap, need + 1);
        vsnprintf(*buf + *len, need + 1, fmt, ap);
        *len += need;
    }
}

/* ══════════════════════════════════════════════════════════════
 * Initialize/Destroy
 * ══════════════════════════════════════════════════════════════ */

void asm_codegen_init(AsmCodeGen *gen, AsmTarget target, int opt_level) {
    memset(gen, 0, sizeof(AsmCodeGen));

    gen->target = target;
    gen->opt_level = opt_level;

    /* Initialize code buffer */
    gen->code_cap = 65536;
    gen->code = malloc(gen->code_cap);
    gen->code[0] = '\0';
    gen->code_len = 0;

    /* Initialize data buffer */
    gen->data_cap = 16384;
    gen->data = malloc(gen->data_cap);
    gen->data[0] = '\0';
    gen->data_len = 0;

    /* Initialize label generator */
    gen->labels.counter = 0;
    strcpy(gen->labels.prefix, ".L");
}

void asm_codegen_free(AsmCodeGen *gen) {
    free(gen->code);
    free(gen->data);

    /* Free function context if any */
    if (gen->func) {
        for (int i = 0; i < gen->func->var_count; i++) {
            free(gen->func->vars[i].name);
        }
        free(gen->func->vars);
        free(gen->func);
    }
}

/* ══════════════════════════════════════════════════════════════
 * Emit Functions
 * ══════════════════════════════════════════════════════════════ */

void asm_emit_raw(AsmCodeGen *gen, const char *str) {
    sb_append(&gen->code, &gen->code_len, &gen->code_cap, str);
}

void asm_emit(AsmCodeGen *gen, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sb_vprintf(&gen->code, &gen->code_len, &gen->code_cap, fmt, ap);
    va_end(ap);
}

static void asm_emit_data(AsmCodeGen *gen, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sb_vprintf(&gen->data, &gen->data_len, &gen->data_cap, fmt, ap);
    va_end(ap);
}

/* ══════════════════════════════════════════════════════════════
 * Size Suffix
 * ══════════════════════════════════════════════════════════════ */

static char size_suffix(OperandSize size) {
    switch (size) {
        case SIZE_8:  return 'b';
        case SIZE_16: return 'w';
        case SIZE_32: return 'l';
        case SIZE_64: return 'q';
        default:      return 'q';
    }
}

/* ══════════════════════════════════════════════════════════════
 * Instruction Names
 * ══════════════════════════════════════════════════════════════ */

static const char *instr_name(Instruction instr) {
    static const char *names[] = {
        [I_MOV] = "mov", [I_MOVZX] = "movz", [I_MOVSX] = "movs", [I_LEA] = "lea",
        [I_PUSH] = "push", [I_POP] = "pop", [I_XCHG] = "xchg",
        [I_ADD] = "add", [I_SUB] = "sub", [I_MUL] = "mul", [I_IMUL] = "imul",
        [I_DIV] = "div", [I_IDIV] = "idiv", [I_INC] = "inc", [I_DEC] = "dec",
        [I_NEG] = "neg", [I_ADC] = "adc", [I_SBB] = "sbb",
        [I_AND] = "and", [I_OR] = "or", [I_XOR] = "xor", [I_NOT] = "not",
        [I_SHL] = "shl", [I_SHR] = "shr", [I_SAR] = "sar",
        [I_CMP] = "cmp", [I_TEST] = "test",
        [I_JMP] = "jmp", [I_JE] = "je", [I_JNE] = "jne",
        [I_JL] = "jl", [I_JLE] = "jle", [I_JG] = "jg", [I_JGE] = "jge",
        [I_JA] = "ja", [I_JAE] = "jae", [I_JB] = "jb", [I_JBE] = "jbe",
        [I_JZ] = "jz", [I_JNZ] = "jnz",
        [I_CALL] = "call", [I_RET] = "ret",
        [I_ENTER] = "enter", [I_LEAVE] = "leave",
        [I_NOP] = "nop", [I_CPUID] = "cpuid", [I_SYSCALL] = "syscall"
    };
    return names[instr] ? names[instr] : "???";
}

void asm_emit_instr(AsmCodeGen *gen, Instruction instr, OperandSize size,
                    Operand *dst, Operand *src) {
    gen->instr_count++;
    /* Simplified - just emit common patterns */
    (void)dst; (void)src;
    (void)instr; (void)size;
}

/* ══════════════════════════════════════════════════════════════
 * Common Instruction Patterns
 * ══════════════════════════════════════════════════════════════ */

void asm_emit_mov(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src) {
    (void)size; (void)dst; (void)src;
    gen->instr_count++;
}

void asm_emit_add(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src) {
    (void)size; (void)dst; (void)src;
    gen->instr_count++;
}

void asm_emit_sub(AsmCodeGen *gen, OperandSize size, Operand *dst, Operand *src) {
    (void)size; (void)dst; (void)src;
    gen->instr_count++;
}

void asm_emit_cmp(AsmCodeGen *gen, OperandSize size, Operand *left, Operand *right) {
    (void)size; (void)left; (void)right;
    gen->instr_count++;
}

void asm_emit_jmp(AsmCodeGen *gen, const char *label) {
    asm_emit(gen, "    jmp %s\n", label);
}

void asm_emit_jcc(AsmCodeGen *gen, Instruction cond, const char *label) {
    asm_emit(gen, "    %s %s\n", instr_name(cond), label);
}

void asm_emit_call(AsmCodeGen *gen, const char *target) {
    gen->instr_count++;
    asm_emit(gen, "    call %s\n", target);
}

void asm_emit_ret(AsmCodeGen *gen) {
    gen->instr_count++;
    asm_emit(gen, "    ret\n");
}

/* ══════════════════════════════════════════════════════════════
 * Label Management
 * ══════════════════════════════════════════════════════════════ */

char *asm_new_label(AsmCodeGen *gen, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s%d", gen->labels.prefix, gen->labels.counter++);
    return buf;
}

void asm_emit_label(AsmCodeGen *gen, const char *label) {
    asm_emit(gen, "%s:\n", label);
}

/* ══════════════════════════════════════════════════════════════
 * Function Prologue/Epilogue
 * ══════════════════════════════════════════════════════════════ */

void asm_func_prologue(AsmCodeGen *gen, const char *name, int stack_size) {
    /* Align stack to 16 bytes */
    if (stack_size % 16 != 0) {
        stack_size += 16 - (stack_size % 16);
    }

    asm_emit(gen, "\n    .globl %s\n", name);
    asm_emit(gen, "%s:\n", name);
    asm_emit(gen, "    pushq %%rbp\n");
    asm_emit(gen, "    movq %%rsp, %%rbp\n");

    if (stack_size > 0) {
        asm_emit(gen, "    subq $%d, %%rsp\n", stack_size);
    }

    /* Save callee-saved registers we might use */
    asm_emit(gen, "    pushq %%rbx\n");
    asm_emit(gen, "    pushq %%r12\n");
    asm_emit(gen, "    pushq %%r13\n");
    asm_emit(gen, "    pushq %%r14\n");
    asm_emit(gen, "    pushq %%r15\n");
    asm_emit(gen, "    pushq %%rdi\n"); /* Stack alignment padding (6 pushes * 8 = 48 bytes -> keeps 16n) */

    if (gen->func) {
        gen->func->stack_size = stack_size + 40; /* 5 saved regs * 8 bytes */
    }
}

void asm_func_epilogue(AsmCodeGen *gen) {
    if (gen->func && gen->func->name) {
        asm_emit(gen, ".L%s_epilogue:\n", gen->func->name);
    } else {
        asm_emit(gen, ".Lepilogue:\n");
    }
    asm_emit(gen, "    popq %%rdi\n");
    asm_emit(gen, "    popq %%r15\n");
    asm_emit(gen, "    popq %%r14\n");
    asm_emit(gen, "    popq %%r13\n");
    asm_emit(gen, "    popq %%r12\n");
    asm_emit(gen, "    popq %%rbx\n");
    asm_emit(gen, "    movq %%rbp, %%rsp\n");
    asm_emit(gen, "    popq %%rbp\n");
    asm_emit(gen, "    ret\n");
}

void asm_func_alloc_stack(AsmCodeGen *gen, int size) {
    if (size > 0) {
        asm_emit(gen, "    subq $%d, %%rsp\n", size);
    }
}

/* ══════════════════════════════════════════════════════════════
 * Variable Management
 * ══════════════════════════════════════════════════════════════ */

int asm_add_local_var(AsmCodeGen *gen, const char *name, int type_size) {
    if (!gen->func) return -1;

    /* Grow var array if needed */
    if (gen->func->var_count >= gen->func->var_capacity) {
        int new_cap = gen->func->var_capacity * 2;
        if (new_cap < 16) new_cap = 16;
        gen->func->vars = realloc(gen->func->vars, new_cap * sizeof(VarInfo));
        gen->func->var_capacity = new_cap;
    }

    VarInfo *vi = &gen->func->vars[gen->func->var_count];
    vi->name = strdup(name);
    vi->type_size = type_size;
    vi->loc = LOC_STACK;
    vi->offset = gen->func->next_stack_slot + type_size;
    vi->is_array = 0;
    vi->array_size = 1;

    gen->func->next_stack_slot += type_size;
    gen->func->var_count++;
    gen->func->stack_size += type_size;

    return vi->offset;
}

VarInfo *asm_lookup_var(AsmCodeGen *gen, const char *name) {
    if (!gen->func) return NULL;

    for (int i = 0; i < gen->func->var_count; i++) {
        if (strcmp(gen->func->vars[i].name, name) == 0) {
            return &gen->func->vars[i];
        }
    }
    return NULL;
}

int asm_get_var_offset(AsmCodeGen *gen, const char *name) {
    VarInfo *vi = asm_lookup_var(gen, name);
    return vi ? vi->offset : 0;
}

/* ══════════════════════════════════════════════════════════════
 * Register Allocation
 * ══════════════════════════════════════════════════════════════ */

/* Available for allocation: rbx, r12-r15 (callee-saved), r10-r11 (caller-saved) */
static X64Reg allocable_regs[] = {
    REG_R10, REG_R11,  /* Caller-saved, safe to use */
    REG_RBX, REG_R12, REG_R13, REG_R14, REG_R15  /* Callee-saved */
};
#define NUM_ALLOC_REGS 7

X64Reg asm_alloc_reg(AsmCodeGen *gen) {
    if (!gen->func) return REG_RAX;

    /* Find a free register */
    for (int i = 0; i < NUM_ALLOC_REGS; i++) {
        X64Reg reg = allocable_regs[i];
        if (!gen->func->regs[reg].used) {
            gen->func->regs[reg].used = 1;
            return reg;
        }
    }

    /* All registers in use - spill one */
    for (int i = 2; i < NUM_ALLOC_REGS; i++) {
        X64Reg reg = allocable_regs[i];
        if (gen->func->regs[reg].used && gen->func->regs[reg].dirty) {
            asm_spill_reg(gen, reg);
            gen->func->regs[reg].used = 0;
            return reg;
        }
    }

    return REG_RAX;
}

void asm_free_reg(AsmCodeGen *gen, X64Reg reg) {
    if (!gen->func) return;
    gen->func->regs[reg].used = 0;
}

void asm_spill_reg(AsmCodeGen *gen, X64Reg reg) {
    if (!gen->func || !gen->func->regs[reg].dirty) return;

    int slot = gen->func->regs[reg].stack_slot;
    if (slot >= 0) {
        asm_emit(gen, "    movq %%%s, -%d(%%rbp)\n", reg_name(reg, SIZE_64), slot);
    }
    gen->func->regs[reg].dirty = 0;
}

X64Reg asm_ensure_in_reg(AsmCodeGen *gen, const char *var_name) {
    VarInfo *vi = asm_lookup_var(gen, var_name);
    if (!vi) return REG_RAX;

    if (vi->loc == LOC_REGISTER) {
        return vi->reg;
    }

    /* Load from stack to register */
    X64Reg reg = asm_alloc_reg(gen);
    asm_emit(gen, "    movq -%d(%%rbp), %%%s\n", vi->offset, reg_name(reg, SIZE_64));

    vi->loc = LOC_REGISTER;
    vi->reg = reg;
    gen->func->regs[reg].var_name = strdup(var_name);

    return reg;
}

/* ══════════════════════════════════════════════════════════════
 * Token Type to String (for operators)
 * ══════════════════════════════════════════════════════════════ */

static const char *token_to_op(TokenType op) {
    switch (op) {
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_LT: return "<";
        case TOK_GT: return ">";
        case TOK_LTE: return "<=";
        case TOK_GTE: return ">=";
        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_AND: return "and";
        case TOK_OR: return "or";
        case TOK_BIT_AND: return "&";
        case TOK_BIT_OR: return "|";
        case TOK_BIT_XOR: return "^";
        case TOK_LSHIFT: return "<<";
        case TOK_RSHIFT: return ">>";
        default: return "?";
    }
}

/* ══════════════════════════════════════════════════════════════
 * Expression Code Generation
 * ══════════════════════════════════════════════════════════════ */

/* Forward declarations */
static void asm_gen_expr(AsmCodeGen *gen, AstNode *node);
static void asm_gen_stmt_list(AsmCodeGen *gen, NodeList *list);

/* Generate expression and put result in RAX */
static void asm_gen_expr(AsmCodeGen *gen, AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_INT_LIT:
            asm_emit(gen, "    movq $%ld, %%rax\n", node->int_lit.value);
            break;

        case NODE_FLOAT_LIT: {
            int label_id = gen->labels.counter++;
            asm_emit_data(gen, "    .align 8\n");
            asm_emit_data(gen, ".LC%d:\n", label_id);
            asm_emit_data(gen, "    .double %g\n", node->float_lit.value);
            asm_emit(gen, "    movsd .LC%d(%%rip), %%xmm0\n", label_id);
            break;
        }

        case NODE_BOOL_LIT:
            asm_emit(gen, "    movq $%d, %%rax\n", node->bool_lit.value ? 1 : 0);
            break;

        case NODE_NONE_LIT:
            asm_emit(gen, "    movq $0, %%rax\n");
            break;

        case NODE_STRING_LIT: {
            int label_id = gen->labels.counter++;
            asm_emit_data(gen, "    .align 8\n");
            asm_emit_data(gen, ".LC%d:\n", label_id);
            asm_emit_data(gen, "    .asciz \"%s\"\n", node->str_lit.value);
            asm_emit(gen, "    leaq .LC%d(%%rip), %%rax\n", label_id);
            break;
        }

        case NODE_FSTRING: {
            /* F-string: for assembly, we need to call a runtime function to concatenate parts */
            /* This is a simplified implementation - just emit a comment for now */
            asm_emit(gen, "    # F-string with %d parts\n", node->fstring_expr.parts.count);
            /* Generate each part and call string concat */
            for (int i = 0; i < node->fstring_expr.parts.count; i++) {
                asm_gen_expr(gen, node->fstring_expr.parts.items[i]);
                asm_emit(gen, "    pushq %%rax\n");
            }
            /* Pop and concatenate (simplified - just return last part) */
            if (node->fstring_expr.parts.count > 0) {
                for (int i = 0; i < node->fstring_expr.parts.count; i++) {
                    asm_emit(gen, "    popq %%rax\n");
                }
            }
            break;
        }

        case NODE_NAME: {
            VarInfo *vi = asm_lookup_var(gen, node->name_expr.name);
            if (vi) {
                asm_emit(gen, "    movq -%d(%%rbp), %%rax\n", vi->offset);
            } else {
                asm_emit(gen, "    movq %s(%%rip), %%rax\n", node->name_expr.name);
            }
            break;
        }

        case NODE_BIN_OP: {
            /* Generate right side first */
            asm_gen_expr(gen, node->bin_op.right);
            asm_emit(gen, "    pushq %%rax\n");

            /* Generate left side */
            asm_gen_expr(gen, node->bin_op.left);

            /* Pop right side to rcx */
            asm_emit(gen, "    popq %%rcx\n");

            /* Now: left in rax, right in rcx */
            const char *op = token_to_op(node->bin_op.op);

            if (strcmp(op, "+") == 0) {
                asm_emit(gen, "    addq %%rcx, %%rax\n");
            } else if (strcmp(op, "-") == 0) {
                asm_emit(gen, "    subq %%rcx, %%rax\n");
            } else if (strcmp(op, "*") == 0) {
                asm_emit(gen, "    imulq %%rcx, %%rax\n");
            } else if (strcmp(op, "/") == 0) {
                asm_emit(gen, "    cqto\n");
                asm_emit(gen, "    idivq %%rcx\n");
            } else if (strcmp(op, "%") == 0) {
                asm_emit(gen, "    cqto\n");
                asm_emit(gen, "    idivq %%rcx\n");
                asm_emit(gen, "    movq %%rdx, %%rax\n");
            } else if (strcmp(op, "<") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    setl %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, "<=") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    setle %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, ">") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    setg %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, ">=") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    setge %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, "==") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    sete %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, "!=") == 0) {
                asm_emit(gen, "    cmpq %%rcx, %%rax\n");
                asm_emit(gen, "    setne %%al\n");
                asm_emit(gen, "    movzbq %%al, %%rax\n");
            } else if (strcmp(op, "and") == 0) {
                asm_emit(gen, "    andq %%rcx, %%rax\n");
            } else if (strcmp(op, "or") == 0) {
                asm_emit(gen, "    orq %%rcx, %%rax\n");
            } else if (strcmp(op, "&") == 0) {
                asm_emit(gen, "    andq %%rcx, %%rax\n");
            } else if (strcmp(op, "|") == 0) {
                asm_emit(gen, "    orq %%rcx, %%rax\n");
            } else if (strcmp(op, "^") == 0) {
                asm_emit(gen, "    xorq %%rcx, %%rax\n");
            } else if (strcmp(op, "<<") == 0) {
                asm_emit(gen, "    movq %%rcx, %%rcx\n");
                asm_emit(gen, "    shlq %%cl, %%rax\n");
            } else if (strcmp(op, ">>") == 0) {
                asm_emit(gen, "    movq %%rcx, %%rcx\n");
                asm_emit(gen, "    sarq %%cl, %%rax\n");
            }
            break;
        }

        case NODE_UNARY_OP: {
            asm_gen_expr(gen, node->unary_op.operand);

            if (node->unary_op.op == TOK_MINUS) {
                asm_emit(gen, "    negq %%rax\n");
            } else if (node->unary_op.op == TOK_NOT) {
                asm_emit(gen, "    xorq $1, %%rax\n");
            } else if (node->unary_op.op == TOK_BIT_NOT) {
                asm_emit(gen, "    notq %%rax\n");
            }
            break;
        }

        case NODE_CALL: {
            /* Check for native method overrides */
            if (node->call.func->type == NODE_ATTRIBUTE && 
                node->call.func->attribute.obj->type == NODE_NAME && 
                strcmp(node->call.func->attribute.obj->name_expr.name, "time") == 0 && 
                strcmp(node->call.func->attribute.attr, "time") == 0) {
                
                if (gen->target == TARGET_WINDOWS_X64) {
                    asm_emit(gen, "    movq $0, %%rcx\n");
                    asm_emit(gen, "    subq $32, %%rsp\n");
                    asm_emit(gen, "    call time\n");
                    asm_emit(gen, "    addq $32, %%rsp\n");
                } else {
                    asm_emit(gen, "    movq $0, %%rdi\n");
                    asm_emit(gen, "    call time\n");
                }
                break;
            }
            
            if (node->call.func->type == NODE_NAME && strcmp(node->call.func->name_expr.name, "print") == 0) {
                if (node->call.args.count > 0) {
                    AstNode *arg = node->call.args.items[0];
                    if (arg->type == NODE_STRING_LIT) {
                        asm_gen_expr(gen, arg);
                        if (gen->target == TARGET_WINDOWS_X64) {
                            asm_emit(gen, "    movq %%rax, %%rcx\n");
                            asm_emit(gen, "    subq $32, %%rsp\n");
                            asm_emit(gen, "    call puts\n");
                            asm_emit(gen, "    addq $32, %%rsp\n");
                        } else {
                            asm_emit(gen, "    movq %%rax, %%rdi\n");
                            asm_emit(gen, "    call puts\n");
                        }
                        break;
                    } else {
                        int label_fmt = gen->labels.counter++;
                        asm_emit_data(gen, "    .align 8\n");
                        asm_emit_data(gen, ".LC%d:\n    .asciz \"%%lld\\n\"\n", label_fmt);
                        
                        asm_gen_expr(gen, arg);
                        
                        if (gen->target == TARGET_WINDOWS_X64) {
                            asm_emit(gen, "    movq %%rax, %%rdx\n");
                            asm_emit(gen, "    leaq .LC%d(%%rip), %%rcx\n", label_fmt);
                            asm_emit(gen, "    subq $32, %%rsp\n");
                            asm_emit(gen, "    call printf\n");
                            asm_emit(gen, "    addq $32, %%rsp\n");
                        } else {
                            asm_emit(gen, "    movq %%rax, %%rsi\n");
                            asm_emit(gen, "    leaq .LC%d(%%rip), %%rdi\n", label_fmt);
                            asm_emit(gen, "    movq $0, %%rax\n");
                            asm_emit(gen, "    call printf\n");
                        }
                        break;
                    }
                }
            }

            X64Reg arg_regs_sysv[] = {REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};
            X64Reg arg_regs_win[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
            X64Reg *arg_regs = (gen->target == TARGET_WINDOWS_X64) ? arg_regs_win : arg_regs_sysv;
            int max_regs = (gen->target == TARGET_WINDOWS_X64) ? 4 : 6;

            int arg_count = node->call.args.count;

            /* Evaluate all arguments, push onto stack */
            for (int i = arg_count - 1; i >= 0; i--) {
                asm_gen_expr(gen, node->call.args.items[i]);
                asm_emit(gen, "    pushq %%rax\n");
            }

            /* Pop into registers (up to max_regs) */
            for (int i = 0; i < arg_count && i < max_regs; i++) {
                asm_emit(gen, "    popq %%%s\n", reg_name(arg_regs[i], SIZE_64));
            }

            /* Stack arguments and Shadow Space */
            int stack_args = arg_count > max_regs ? arg_count - max_regs : 0;
            int shadow_space = (gen->target == TARGET_WINDOWS_X64) ? 32 : 0;
            int stack_adjust = shadow_space;
            
            if (stack_adjust > 0) {
                asm_emit(gen, "    subq $%d, %%rsp\n", stack_adjust);
            }

            /* Call function */
            if (node->call.func->type == NODE_NAME) {
                if (gen->hybrid_mode) {
                    char prefixed[256];
                    snprintf(prefixed, sizeof(prefixed), "lp_%s", node->call.func->name_expr.name);
                    asm_emit_call(gen, prefixed);
                } else {
                    asm_emit_call(gen, node->call.func->name_expr.name);
                }
            } else {
                /* Indirect call through register */
                asm_gen_expr(gen, node->call.func);
                asm_emit(gen, "    call *%%rax\n");
            }

            /* Clean up */
            if (stack_args > 0 || stack_adjust > 0) {
                int cleanup = stack_args * 8 + stack_adjust;
                asm_emit(gen, "    addq $%d, %%rsp\n", cleanup);
            }
            break;
        }

        case NODE_LIST_EXPR: {
            int count = node->list_expr.elems.count;
            int total_size = count * 8 + 8;

            /* Allocate space on stack */
            asm_emit(gen, "    subq $%d, %%rsp\n", total_size);
            asm_emit(gen, "    movq %%rsp, %%r10\n");

            /* Store count as first element */
            asm_emit(gen, "    movq $%d, (%%r10)\n", count);

            /* Store elements */
            for (int i = 0; i < count; i++) {
                asm_gen_expr(gen, node->list_expr.elems.items[i]);
                asm_emit(gen, "    movq %%rax, %d(%%r10)\n", (i + 1) * 8);
            }

            asm_emit(gen, "    movq %%r10, %%rax\n");
            break;
        }

        case NODE_SUBSCRIPT: {
            /* Array/string indexing */
            asm_gen_expr(gen, node->subscript.obj);
            asm_emit(gen, "    pushq %%rax\n");
            asm_gen_expr(gen, node->subscript.index);
            asm_emit(gen, "    popq %%rcx\n");
            asm_emit(gen, "    movq (%%rcx, %%rax, 8), %%rax\n");
            break;
        }

        case NODE_ATTRIBUTE: {
            /* Object member access */
            asm_gen_expr(gen, node->attribute.obj);
            /* Would need to look up offset from type info */
            asm_emit(gen, "    # attribute %s\n", node->attribute.attr);
            break;
        }

        default:
            asm_emit(gen, "    # Unhandled expression type: %d\n", node->type);
            break;
    }
}

/* ══════════════════════════════════════════════════════════════
 * Statement Code Generation
 * ══════════════════════════════════════════════════════════════ */

static void asm_gen_stmt_list(AsmCodeGen *gen, NodeList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        AstNode *node = list->items[i];

        switch (node->type) {
            case NODE_ASSIGN: {
                int size = 8;
                if (node->assign.type_ann) {
                    if (strstr(node->assign.type_ann, "int")) size = 8;
                    else if (strstr(node->assign.type_ann, "float")) size = 8;
                    else if (strstr(node->assign.type_ann, "bool")) size = 1;
                    else if (strstr(node->assign.type_ann, "str")) size = 8;
                }

                asm_add_local_var(gen, node->assign.name, size);

                if (node->assign.value) {
                    asm_gen_expr(gen, node->assign.value);
                    VarInfo *vi = asm_lookup_var(gen, node->assign.name);
                    if (vi) {
                        asm_emit(gen, "    movq %%rax, -%d(%%rbp)\n", vi->offset);
                    }
                }
                break;
            }

            case NODE_AUG_ASSIGN: {
                const char *op = token_to_op(node->aug_assign.op);
                const char *name = node->assign.name;

                asm_gen_expr(gen, node->aug_assign.value);
                VarInfo *vi = asm_lookup_var(gen, name);
                if (vi) {
                    if (strcmp(op, "+") == 0) {
                        asm_emit(gen, "    addq %%rax, -%d(%%rbp)\n", vi->offset);
                    } else if (strcmp(op, "-") == 0) {
                        asm_emit(gen, "    subq %%rax, -%d(%%rbp)\n", vi->offset);
                    } else if (strcmp(op, "*") == 0) {
                        asm_emit(gen, "    imulq -%d(%%rbp), %%rax\n", vi->offset);
                        asm_emit(gen, "    movq %%rax, -%d(%%rbp)\n", vi->offset);
                    }
                }
                break;
            }

            case NODE_EXPR_STMT: {
                asm_gen_expr(gen, node->expr_stmt.expr);
                break;
            }

            case NODE_RETURN: {
                if (node->return_stmt.value) {
                    asm_gen_expr(gen, node->return_stmt.value);
                }
                if (gen->func && gen->func->name) {
                    asm_emit(gen, "    jmp .L%s_epilogue\n", gen->func->name);
                } else {
                    asm_emit_jmp(gen, ".Lepilogue");
                }
                break;
            }

            case NODE_IF: {
                char else_label[32], end_label[32];
                asm_new_label(gen, else_label, sizeof(else_label));
                asm_new_label(gen, end_label, sizeof(end_label));

                asm_gen_expr(gen, node->if_stmt.cond);
                asm_emit(gen, "    testq %%rax, %%rax\n");

                if (node->if_stmt.else_branch) {
                    asm_emit_jcc(gen, I_JZ, else_label);
                } else {
                    asm_emit_jcc(gen, I_JZ, end_label);
                }

                asm_gen_stmt_list(gen, &node->if_stmt.then_body);
                asm_emit_jmp(gen, end_label);

                if (node->if_stmt.else_branch) {
                    asm_emit_label(gen, else_label);
                    if (node->if_stmt.else_branch->type == NODE_IF) {
                        /* elif chain - generate the if recursively */
                        /* For simplicity, just generate the then_body of elif */
                        asm_gen_stmt_list(gen, &node->if_stmt.else_branch->if_stmt.then_body);
                        /* Note: nested elif chains not fully supported in this simple backend */
                    } else {
                        /* else block - it's a single statement, wrap it */
                        NodeList temp_list;
                        temp_list.items = &node->if_stmt.else_branch;
                        temp_list.count = 1;
                        asm_gen_stmt_list(gen, &temp_list);
                    }
                }

                asm_emit_label(gen, end_label);
                break;
            }

            case NODE_WHILE: {
                char start_label[32], end_label[32];
                asm_new_label(gen, start_label, sizeof(start_label));
                asm_new_label(gen, end_label, sizeof(end_label));

                asm_emit_label(gen, start_label);
                asm_gen_expr(gen, node->while_stmt.cond);
                asm_emit(gen, "    testq %%rax, %%rax\n");
                asm_emit_jcc(gen, I_JZ, end_label);

                asm_gen_stmt_list(gen, &node->while_stmt.body);
                asm_emit_jmp(gen, start_label);

                asm_emit_label(gen, end_label);
                break;
            }

            case NODE_FOR: {
                char cond_label[32], end_label[32];
                asm_new_label(gen, cond_label, sizeof(cond_label));
                asm_new_label(gen, end_label, sizeof(end_label));

                /* Initialize iterator */
                asm_add_local_var(gen, node->for_stmt.var, 8);

                /* Get iterator start value from iter expression */
                /* Simplified: assumes range() call */
                if (node->for_stmt.iter && node->for_stmt.iter->type == NODE_CALL) {
                    /* range(start, end) or range(end) */
                    AstNode *iter = node->for_stmt.iter;
                    if (iter->call.args.count == 1) {
                        /* range(end) - start from 0 */
                        asm_emit(gen, "    movq $0, %%rax\n");
                    } else if (iter->call.args.count >= 2) {
                        /* range(start, end) */
                        asm_gen_expr(gen, iter->call.args.items[0]);
                    }
                    VarInfo *vi = asm_lookup_var(gen, node->for_stmt.var);
                    if (vi) {
                        asm_emit(gen, "    movq %%rax, -%d(%%rbp)\n", vi->offset);
                    }
                }

                asm_emit_label(gen, cond_label);

                /* Check condition (iter < end) */
                if (node->for_stmt.iter && node->for_stmt.iter->type == NODE_CALL) {
                    AstNode *iter = node->for_stmt.iter;
                    if (iter->call.args.count == 1) {
                        asm_gen_expr(gen, iter->call.args.items[0]);
                    } else if (iter->call.args.count >= 2) {
                        asm_gen_expr(gen, iter->call.args.items[1]);
                    }
                    VarInfo *vi = asm_lookup_var(gen, node->for_stmt.var);
                    if (vi) {
                        asm_emit(gen, "    cmpq -%d(%%rbp), %%rax\n", vi->offset);
                    }
                    asm_emit_jcc(gen, I_JLE, end_label);
                }

                asm_gen_stmt_list(gen, &node->for_stmt.body);

                /* Increment iterator */
                {
                    VarInfo *vi = asm_lookup_var(gen, node->for_stmt.var);
                    if (vi) {
                        asm_emit(gen, "    incq -%d(%%rbp)\n", vi->offset);
                    }
                }

                asm_emit_jmp(gen, cond_label);
                asm_emit_label(gen, end_label);
                break;
            }

            case NODE_PARALLEL_FOR: {
                /* For now, emit as regular for loop */
                /* ASM backend: parallel for runs as serial loop */

                char cond_label[32], end_label[32];
                asm_new_label(gen, cond_label, sizeof(cond_label));
                asm_new_label(gen, end_label, sizeof(end_label));

                asm_add_local_var(gen, node->parallel_for.var, 8);

                /* Initialize iterator */
                if (node->parallel_for.iter && node->parallel_for.iter->type == NODE_CALL) {
                    AstNode *iter = node->parallel_for.iter;
                    if (iter->call.args.count == 1) {
                        asm_emit(gen, "    movq $0, %%rax\n");
                    } else if (iter->call.args.count >= 2) {
                        asm_gen_expr(gen, iter->call.args.items[0]);
                    }
                    VarInfo *vi = asm_lookup_var(gen, node->parallel_for.var);
                    if (vi) {
                        asm_emit(gen, "    movq %%rax, -%d(%%rbp)\n", vi->offset);
                    }
                }

                asm_emit_label(gen, cond_label);

                /* Check condition */
                if (node->parallel_for.iter && node->parallel_for.iter->type == NODE_CALL) {
                    AstNode *iter = node->parallel_for.iter;
                    if (iter->call.args.count == 1) {
                        asm_gen_expr(gen, iter->call.args.items[0]);
                    } else if (iter->call.args.count >= 2) {
                        asm_gen_expr(gen, iter->call.args.items[1]);
                    }
                    VarInfo *vi = asm_lookup_var(gen, node->parallel_for.var);
                    if (vi) {
                        asm_emit(gen, "    cmpq -%d(%%rbp), %%rax\n", vi->offset);
                    }
                }
                asm_emit_jcc(gen, I_JLE, end_label);

                asm_gen_stmt_list(gen, &node->parallel_for.body);

                VarInfo *vi = asm_lookup_var(gen, node->parallel_for.var);
                if (vi) {
                    asm_emit(gen, "    incq -%d(%%rbp)\n", vi->offset);
                }

                asm_emit_jmp(gen, cond_label);
                asm_emit_label(gen, end_label);
                break;
            }

            case NODE_PASS: {
                asm_emit(gen, "    nop\n");
                break;
            }

            case NODE_BREAK: {
                asm_emit_jmp(gen, ".Lloop_end");
                break;
            }

            case NODE_CONTINUE: {
                asm_emit_jmp(gen, ".Lloop_start");
                break;
            }

            case NODE_CONST_DECL: {
                int size = 8;
                asm_add_local_var(gen, node->const_decl.name, size);
                if (node->const_decl.value) {
                    asm_gen_expr(gen, node->const_decl.value);
                    VarInfo *vi = asm_lookup_var(gen, node->const_decl.name);
                    if (vi) {
                        asm_emit(gen, "    movq %%rax, -%d(%%rbp)\n", vi->offset);
                    }
                }
                break;
            }

            case NODE_MATCH: {
                /* Match statement: generate as if-else chain */
                /* Store match value in a register */
                asm_gen_expr(gen, node->match_stmt.value);
                asm_emit(gen, "    pushq %%rax\n");  /* Save match value on stack */
                
                char end_label[32];
                asm_new_label(gen, end_label, sizeof(end_label));
                
                int first_case = 1;
                for (int i = 0; i < node->match_stmt.cases.count; i++) {
                    AstNode *case_node = node->match_stmt.cases.items[i];
                    char case_label[32], next_label[32];
                    asm_new_label(gen, case_label, sizeof(case_label));
                    asm_new_label(gen, next_label, sizeof(next_label));
                    
                    if (first_case) {
                        /* First case - compare directly */
                        asm_emit(gen, "    popq %%rax\n");  /* Get match value */
                        asm_emit(gen, "    pushq %%rax\n"); /* Put it back */
                        
                        if (!case_node->match_case.is_wildcard && case_node->match_case.pattern) {
                            asm_gen_expr(gen, case_node->match_case.pattern);
                            asm_emit(gen, "    popq %%rcx\n");  /* pattern in rcx */
                            asm_emit(gen, "    pushq %%rcx\n");
                            asm_emit(gen, "    cmpq %%rax, %%rcx\n");
                            asm_emit_jcc(gen, I_JNE, next_label);
                        }
                        
                        asm_emit_label(gen, case_label);
                        asm_gen_stmt_list(gen, &case_node->match_case.body);
                        asm_emit_jmp(gen, end_label);
                        asm_emit_label(gen, next_label);
                    } else {
                        /* Subsequent cases */
                        if (!case_node->match_case.is_wildcard && case_node->match_case.pattern) {
                            asm_emit(gen, "    movq -8(%%rbp), %%rax\n");  /* Get match value */
                            asm_gen_expr(gen, case_node->match_case.pattern);
                            asm_emit(gen, "    cmpq %%rax, %%rcx\n");
                            asm_emit_jcc(gen, I_JNE, next_label);
                        }
                        
                        asm_emit_label(gen, case_label);
                        asm_gen_stmt_list(gen, &case_node->match_case.body);
                        asm_emit_jmp(gen, end_label);
                        asm_emit_label(gen, next_label);
                    }
                    
                    first_case = 0;
                }
                
                asm_emit_label(gen, end_label);
                asm_emit(gen, "    addq $8, %%rsp\n");  /* Clean up match value */
                break;
            }

            default:
                asm_emit(gen, "    # Unhandled stmt type: %d\n", node->type);
                break;
        }
    }
}

/* ══════════════════════════════════════════════════════════════
 * Function Code Generation
 * ══════════════════════════════════════════════════════════════ */

static void asm_gen_func(AsmCodeGen *gen, AstNode *node) {
    if (node->type != NODE_FUNC_DEF) return;

    const char *name = node->func_def.name;

    /* Create function context */
    gen->func = calloc(1, sizeof(FuncContext));
    gen->func->name = strdup(name);
    gen->func->next_stack_slot = 0;
    gen->func->stack_size = 0;

    /* Prologue */
    if (gen->hybrid_mode) {
        char prefixed_name[256];
        snprintf(prefixed_name, sizeof(prefixed_name), "lp_%s", name);
        asm_func_prologue(gen, prefixed_name, 256);
    } else {
        asm_func_prologue(gen, name, 256);
    }

    /* Handle parameters - use correct ABI for target */
    X64Reg arg_regs_sysv[] = {REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};
    X64Reg arg_regs_win[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
    X64Reg *arg_regs = (gen->target == TARGET_WINDOWS_X64) ? arg_regs_win : arg_regs_sysv;
    int max_param_regs = (gen->target == TARGET_WINDOWS_X64) ? 4 : 6;
    for (int i = 0; i < node->func_def.params.count && i < max_param_regs; i++) {
        asm_add_local_var(gen, node->func_def.params.items[i].name, 8);
        VarInfo *vi = asm_lookup_var(gen, node->func_def.params.items[i].name);
        if (vi) {
            asm_emit(gen, "    movq %%%s, -%d(%%rbp)\n",
                reg_name(arg_regs[i], SIZE_64), vi->offset);
        }
    }

    if (!gen->hybrid_mode && strcmp(name, "main") == 0 && gen->target == TARGET_WINDOWS_X64) {
        asm_emit(gen, "    /* SetConsoleOutputCP(65001) for UTF-8 */\n");
        asm_emit(gen, "    movq $65001, %%rcx\n");
        asm_emit(gen, "    subq $32, %%rsp\n");
        asm_emit(gen, "    call SetConsoleOutputCP\n");
        asm_emit(gen, "    addq $32, %%rsp\n");
    }

    /* Generate body */
    asm_gen_stmt_list(gen, &node->func_def.body);

    /* Epilogue */
    asm_func_epilogue(gen);

    gen->func_count++;

    /* Free function context */
    for (int i = 0; i < gen->func->var_count; i++) {
        free(gen->func->vars[i].name);
    }
    free(gen->func->vars);
    free(gen->func->name);
    free(gen->func);
    gen->func = NULL;
}

/* ══════════════════════════════════════════════════════════════
 * Top-Level Generation
 * ══════════════════════════════════════════════════════════════ */

int asm_generate(AsmCodeGen *gen, AstNode *program) {
    if (!program || program->type != NODE_PROGRAM) {
        gen->had_error = 1;
        strcpy(gen->error_msg, "Invalid AST: expected NODE_PROGRAM");
        return 0;
    }

    /* Apply optimizations if enabled */
    if (gen->opt_level > 0) {
        int opt_count = asm_optimize_ast(gen, program);
        if (opt_count > 0) {
            fprintf(stderr, "[LP ASM] Optimizations applied: %d (folded: %d, dead_code: %d, unrolled: %d)\n",
                    opt_count, gen->constants_folded, gen->dead_code_removed, gen->loops_unrolled);
        }
    }

    /* File header */
    asm_emit(gen, "# Generated by LP Compiler (Assembly Backend)\n");
    asm_emit(gen, "# Target: x86-64 (System V AMD64 ABI)\n\n");
    asm_emit(gen, "    .text\n");

    /* Generate all functions */
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF) {
            asm_gen_func(gen, stmt);
        }
    }

    /* Check for main function */
    int has_main = 0;
    for (int i = 0; i < program->program.stmts.count; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF && strcmp(stmt->func_def.name, "main") == 0) {
            has_main = 1;
            break;
        }
    }

    /* Generate _start for standalone executables */
    if (has_main && gen->target != TARGET_WINDOWS_X64) {
        asm_emit(gen, "\n    .globl _start\n");
        asm_emit(gen, "_start:\n");
        asm_emit(gen, "    call main\n");
        asm_emit(gen, "    movq %%rax, %%rdi\n");
        asm_emit(gen, "    movq $60, %%rax\n");
        asm_emit(gen, "    syscall\n");
    }

    return 1;
}

/* ══════════════════════════════════════════════════════════════
 * Single Function Generation (Hybrid Mode)
 * ══════════════════════════════════════════════════════════════ */

int asm_generate_single_func(AsmCodeGen *gen, AstNode *func_node) {
    if (!func_node || func_node->type != NODE_FUNC_DEF) {
        gen->had_error = 1;
        strcpy(gen->error_msg, "Expected NODE_FUNC_DEF for single function generation");
        return 0;
    }

    gen->hybrid_mode = 1;

    /* File header */
    asm_emit(gen, "# Generated by LP Compiler (Hybrid ASM - Hot Function)\n");
    asm_emit(gen, "# Function: %s\n\n", func_node->func_def.name);
    asm_emit(gen, "    .text\n");

    /* Generate the function */
    asm_gen_func(gen, func_node);

    return !gen->had_error;
}

/* ══════════════════════════════════════════════════════════════
 * Output Functions
 * ══════════════════════════════════════════════════════════════ */

char *asm_get_code(AsmCodeGen *gen) {
    return gen->code;
}

char *asm_get_full_output(AsmCodeGen *gen) {
    size_t total_len = gen->data_len + gen->code_len + 256;
    char *output = malloc(total_len);

    strcpy(output, gen->data);
    strcat(output, gen->code);

    return output;
}

int asm_write_to_file(AsmCodeGen *gen, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        gen->had_error = 1;
        snprintf(gen->error_msg, sizeof(gen->error_msg),
            "Cannot open output file: %s", filename);
        return 0;
    }

    /* Write data section first */
    if (gen->data_len > 0) {
        fprintf(f, "    .section .rodata\n");
        fprintf(f, "%s", gen->data);
    }

    /* Write code section */
    fprintf(f, "%s", gen->code);

    fclose(f);
    return 1;
}

/* ══════════════════════════════════════════════════════════════
 * Compile to Executable
 * ══════════════════════════════════════════════════════════════ */

int asm_compile_to_exe(AsmCodeGen *gen, const char *asm_file, const char *exe_file) {
    char obj_file[1024];

    (void)gen;

    /* SECURITY FIX: Validate file paths to prevent command injection */
    if (!lp_path_is_safe(asm_file) || !lp_path_is_safe(exe_file)) {
        fprintf(stderr, "[LP ASM] Error: Invalid characters in filename\n");
        return 0;
    }

    /* Build object file path */
    snprintf(obj_file, sizeof(obj_file), "%s.o", exe_file);

    /* SECURITY FIX: Use spawn instead of system() to prevent command injection */
    /* Assemble: as -o file.o file.s */
    {
        const char *as_argv[] = {"as", "-o", obj_file, asm_file, NULL};
        int ret = _spawnvp(_P_WAIT, "as", as_argv);
        if (ret != 0) {
            fprintf(stderr, "[LP ASM] Assembly failed\n");
            return 0;
        }
    }

    /* SECURITY FIX: Use spawn for linking too */
    /* Link: ld -o file file.o -lc --dynamic-linker /lib64/ld-linux-x86-64.so.2 */
    {
        const char *ld_argv[] = {"ld", "-o", exe_file, obj_file, "-lc", 
            "--dynamic-linker", "/lib64/ld-linux-x86-64.so.2", NULL};
        int ret = _spawnvp(_P_WAIT, "ld", ld_argv);
        if (ret != 0) {
            fprintf(stderr, "[LP ASM] Linking failed\n");
            remove(obj_file);
            return 0;
        }
    }

    /* Cleanup object file */
    remove(obj_file);

    printf("[LP ASM] Created executable: %s\n", exe_file);
    return 1;
}
