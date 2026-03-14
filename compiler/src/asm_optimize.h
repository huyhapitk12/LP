/*
 * LP Compiler - Assembly Optimization Header
 *
 * Constant folding, dead code elimination, loop unrolling
 */

#ifndef LP_ASM_OPTIMIZE_H
#define LP_ASM_OPTIMIZE_H

#include "ast.h"
#include "codegen_asm.h"

/* ══════════════════════════════════════════════════════════════
 * Constant Expression Analysis
 * ══════════════════════════════════════════════════════════════ */

/* Check if expression can be evaluated at compile time */
int asm_is_constant_expr(AstNode *node);

/* Get the compile-time value of a constant expression */
int64_t asm_get_constant_value(AstNode *node);

/* ══════════════════════════════════════════════════════════════
 * Loop Unrolling
 * ══════════════════════════════════════════════════════════════ */

/* Attempt to unroll a loop if bounds are constant and small */
int asm_opt_unroll_loop(AstNode *node, int max_iterations);

/* ══════════════════════════════════════════════════════════════
 * Main Optimization Entry Point
 * ══════════════════════════════════════════════════════════════ */

/* Apply all enabled optimizations to an AST
 * Returns total number of optimizations applied
 */
int asm_optimize_ast(AsmCodeGen *gen, AstNode *node);

#endif /* LP_ASM_OPTIMIZE_H */
