/*
 * LP Compiler - Assembly Optimization Functions
 *
 * Constant folding, dead code elimination, loop unrolling
 */

#define _GNU_SOURCE
#include "codegen_asm.h"
#include "asm_optimize.h"
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════
 * Constant Folding
 * ══════════════════════════════════════════════════════════════ */

/* Check if expression is a compile-time constant */
int asm_is_constant_expr(AstNode *node) {
    if (!node) return 0;
    
    switch (node->type) {
        case NODE_INT_LIT:
        case NODE_BOOL_LIT:
        case NODE_NONE_LIT:
            return 1;
        
        case NODE_BIN_OP:
            return asm_is_constant_expr(node->bin_op.left) &&
                   asm_is_constant_expr(node->bin_op.right);
        
        case NODE_UNARY_OP:
            return asm_is_constant_expr(node->unary_op.operand);
        
        default:
            return 0;
    }
}

/* Get the constant value from an expression */
int64_t asm_get_constant_value(AstNode *node) {
    if (!node) return 0;
    
    switch (node->type) {
        case NODE_INT_LIT:
            return node->int_lit.value;
        
        case NODE_BOOL_LIT:
            return node->bool_lit.value ? 1 : 0;
        
        case NODE_NONE_LIT:
            return 0;
        
        case NODE_BIN_OP: {
            int64_t left = asm_get_constant_value(node->bin_op.left);
            int64_t right = asm_get_constant_value(node->bin_op.right);
            
            switch (node->bin_op.op) {
                case TOK_PLUS:  return left + right;
                case TOK_MINUS: return left - right;
                case TOK_STAR:  return left * right;
                case TOK_SLASH: return right != 0 ? left / right : 0;
                case TOK_PERCENT: return right != 0 ? left % right : 0;
                case TOK_LT:    return left < right;
                case TOK_GT:    return left > right;
                case TOK_LTE:   return left <= right;
                case TOK_GTE:   return left >= right;
                case TOK_EQ:    return left == right;
                case TOK_NEQ:   return left != right;
                case TOK_BIT_AND: return left & right;
                case TOK_BIT_OR:  return left | right;
                case TOK_BIT_XOR: return left ^ right;
                case TOK_LSHIFT:  return left << right;
                case TOK_RSHIFT:  return left >> right;
                case TOK_AND:   return left && right;
                case TOK_OR:    return left || right;
                default: return 0;
            }
        }
        
        case NODE_UNARY_OP: {
            int64_t operand = asm_get_constant_value(node->unary_op.operand);
            
            switch (node->unary_op.op) {
                case TOK_MINUS:   return -operand;
                case TOK_NOT:     return !operand;
                case TOK_BIT_NOT: return ~operand;
                default: return operand;
            }
        }
        
        default:
            return 0;
    }
}

/* Constant folding optimization - modifies AST in place */
static int asm_opt_constant_fold(AstNode *node) {
    if (!node) return 0;
    
    int folded = 0;
    
    switch (node->type) {
        case NODE_BIN_OP:
            /* Recursively fold children first */
            folded += asm_opt_constant_fold(node->bin_op.left);
            folded += asm_opt_constant_fold(node->bin_op.right);
            
            /* If both operands are now constants, fold this node */
            if (asm_is_constant_expr(node->bin_op.left) &&
                asm_is_constant_expr(node->bin_op.right)) {
                int64_t value = asm_get_constant_value(node);
                
                /* Transform this node into an integer literal */
                node->type = NODE_INT_LIT;
                node->int_lit.value = value;
                folded++;
            }
            break;
        
        case NODE_UNARY_OP:
            folded += asm_opt_constant_fold(node->unary_op.operand);
            
            if (asm_is_constant_expr(node->unary_op.operand)) {
                int64_t value = asm_get_constant_value(node);
                node->type = NODE_INT_LIT;
                node->int_lit.value = value;
                folded++;
            }
            break;
        
        case NODE_ASSIGN:
            if (node->assign.value) {
                folded += asm_opt_constant_fold(node->assign.value);
            }
            break;
        
        case NODE_CONST_DECL:
            if (node->const_decl.value) {
                folded += asm_opt_constant_fold(node->const_decl.value);
            }
            break;
        
        case NODE_RETURN:
            if (node->return_stmt.value) {
                folded += asm_opt_constant_fold(node->return_stmt.value);
            }
            break;
        
        case NODE_IF:
            folded += asm_opt_constant_fold(node->if_stmt.cond);
            break;
        
        case NODE_WHILE:
            folded += asm_opt_constant_fold(node->while_stmt.cond);
            break;
        
        case NODE_EXPR_STMT:
            folded += asm_opt_constant_fold(node->expr_stmt.expr);
            break;
        
        case NODE_CALL:
            for (int i = 0; i < node->call.args.count; i++) {
                folded += asm_opt_constant_fold(node->call.args.items[i]);
            }
            break;
        
        default:
            break;
    }
    
    return folded;
}

/* ══════════════════════════════════════════════════════════════
 * Dead Code Elimination
 * ══════════════════════════════════════════════════════════════ */

static int asm_opt_eliminate_dead_code(AstNode *node) {
    if (!node) return 0;
    
    int removed = 0;
    
    switch (node->type) {
        case NODE_IF:
            /* If condition is constant, we can eliminate dead branch */
            if (asm_is_constant_expr(node->if_stmt.cond)) {
                removed++;
            }
            break;
        
        case NODE_WHILE:
            /* If condition is constant false, loop never executes */
            if (asm_is_constant_expr(node->while_stmt.cond)) {
                int64_t cond_value = asm_get_constant_value(node->while_stmt.cond);
                if (!cond_value) {
                    /* Loop never executes - can be removed */
                    removed++;
                }
            }
            break;
        
        default:
            break;
    }
    
    return removed;
}

/* ══════════════════════════════════════════════════════════════
 * Loop Unrolling
 * ══════════════════════════════════════════════════════════════ */

int asm_opt_unroll_loop(AstNode *node, int max_iterations) {
    if (!node) return 0;
    
    int unrolled = 0;
    
    if (node->type == NODE_FOR) {
        AstNode *iter = node->for_stmt.iter;
        if (iter && iter->type == NODE_CALL) {
            if (iter->call.args.count >= 1) {
                /* Try to determine iteration count */
                int can_unroll = 0;
                int64_t iterations = 0;
                
                if (iter->call.args.count == 1) {
                    if (asm_is_constant_expr(iter->call.args.items[0])) {
                        iterations = asm_get_constant_value(iter->call.args.items[0]);
                        can_unroll = 1;
                    }
                } else if (iter->call.args.count >= 2) {
                    if (asm_is_constant_expr(iter->call.args.items[0]) &&
                        asm_is_constant_expr(iter->call.args.items[1])) {
                        int64_t start = asm_get_constant_value(iter->call.args.items[0]);
                        int64_t end = asm_get_constant_value(iter->call.args.items[1]);
                        iterations = end - start;
                        can_unroll = 1;
                    }
                }
                
                if (can_unroll && iterations > 0 && iterations <= max_iterations) {
                    unrolled++;
                }
            }
        }
    }
    
    return unrolled;
}

/* ══════════════════════════════════════════════════════════════
 * Main Optimization Entry Point
 * ══════════════════════════════════════════════════════════════ */

int asm_optimize_ast(AsmCodeGen *gen, AstNode *node) {
    if (!gen || !node) return 0;
    if (gen->opt_level == 0) return 0;
    
    int total = 0;
    
    /* Pass 1: Constant folding */
    gen->constants_folded = asm_opt_constant_fold(node);
    total += gen->constants_folded;
    
    /* Pass 2: Dead code elimination */
    gen->dead_code_removed = asm_opt_eliminate_dead_code(node);
    total += gen->dead_code_removed;
    
    /* Pass 3: Loop unrolling (opt_level >= 2) */
    if (gen->opt_level >= 2) {
        gen->loops_unrolled = asm_opt_unroll_loop(node, 8);
        total += gen->loops_unrolled;
    }
    
    return total;
}
