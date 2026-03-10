#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Helpers --- */
static void advance(Parser *p) {
    p->previous = p->current;
    p->current = lexer_next(&p->lexer);
}

static int check(Parser *p, TokenType t) { return p->current.type == t; }

static int match(Parser *p, TokenType t) {
    if (!check(p, t)) return 0;
    advance(p);
    return 1;
}

static void error(Parser *p, const char *msg) {
    if (p->had_error) return;
    p->had_error = 1;
    snprintf(p->error_msg, sizeof(p->error_msg), "Line %d: %s (got '%s')",
             p->current.line, msg, token_type_name(p->current.type));
}

static void expect(Parser *p, TokenType t, const char *msg) {
    if (!match(p, t)) error(p, msg);
}

static char *tok_to_str(Token t) {
    char *s = (char *)malloc(t.length + 1);
    memcpy(s, t.start, t.length);
    s[t.length] = '\0';
    return s;
}

/* Skip optional newlines */
static void skip_newlines(Parser *p) {
    while (check(p, TOK_NEWLINE)) advance(p);
}

/* --- Forward declarations --- */
static AstNode *parse_expression(Parser *p);
static AstNode *parse_statement(Parser *p);
static void parse_block(Parser *p, NodeList *body);

/* --- Expression parsing (precedence climbing) --- */

static AstNode *parse_primary(Parser *p) {
    if (p->had_error) return NULL;
    int line = p->current.line;

    if (match(p, TOK_INT_LIT)) {
        AstNode *n = ast_new(NODE_INT_LIT, line);
        n->int_lit.value = p->previous.int_val;
        return n;
    }
    if (match(p, TOK_FLOAT_LIT)) {
        AstNode *n = ast_new(NODE_FLOAT_LIT, line);
        n->float_lit.value = p->previous.float_val;
        return n;
    }
    if (match(p, TOK_STRING_LIT)) {
        AstNode *n = ast_new(NODE_STRING_LIT, line);
        n->str_lit.value = tok_to_str(p->previous);
        return n;
    }
    if (match(p, TOK_TRUE) || match(p, TOK_FALSE)) {
        AstNode *n = ast_new(NODE_BOOL_LIT, line);
        n->bool_lit.value = (p->previous.type == TOK_TRUE) ? 1 : 0;
        return n;
    }
    if (match(p, TOK_NONE)) {
        return ast_new(NODE_NONE_LIT, line);
    }
    if (match(p, TOK_IDENTIFIER)) {
        AstNode *n = ast_new(NODE_NAME, line);
        n->name_expr.name = tok_to_str(p->previous);
        return n;
    }
    if (match(p, TOK_LPAREN)) {
        int line = p->previous.line;
        if (match(p, TOK_RPAREN)) {
            AstNode *n = ast_new(NODE_TUPLE_EXPR, line);
            node_list_init(&n->tuple_expr.elems);
            return n;
        }
        AstNode *expr = parse_expression(p);
        if (match(p, TOK_COMMA)) {
            /* It's a tuple */
            AstNode *n = ast_new(NODE_TUPLE_EXPR, line);
            node_list_init(&n->tuple_expr.elems);
            node_list_push(&n->tuple_expr.elems, expr);
            if (!check(p, TOK_RPAREN)) {
                node_list_push(&n->tuple_expr.elems, parse_expression(p));
                while (match(p, TOK_COMMA) && !check(p, TOK_RPAREN))
                    node_list_push(&n->tuple_expr.elems, parse_expression(p));
            }
            expect(p, TOK_RPAREN, "expected ')'");
            return n;
        }
        expect(p, TOK_RPAREN, "expected ')'");
        return expr; /* Just grouping */
    }
    if (match(p, TOK_LBRACKET)) {
        /* Could be a list literal OR a list comprehension */
        if (check(p, TOK_RBRACKET)) {
            advance(p);
            AstNode *n = ast_new(NODE_LIST_EXPR, line);
            node_list_init(&n->list_expr.elems);
            return n;
        }
        AstNode *first = parse_expression(p);
        if (match(p, TOK_FOR)) {
            /* List comprehension: [expr for var in iter if cond] */
            AstNode *n = ast_new(NODE_LIST_COMP, line);
            n->list_comp.expr = first;
            expect(p, TOK_IDENTIFIER, "expected loop variable in comprehension");
            n->list_comp.var = tok_to_str(p->previous);
            expect(p, TOK_IN, "expected 'in' in comprehension");
            n->list_comp.iter = parse_expression(p);
            n->list_comp.cond = NULL;
            if (match(p, TOK_IF)) {
                n->list_comp.cond = parse_expression(p);
            }
            expect(p, TOK_RBRACKET, "expected ']'");
            return n;
        }
        /* Regular list literal */
        AstNode *n = ast_new(NODE_LIST_EXPR, line);
        node_list_init(&n->list_expr.elems);
        node_list_push(&n->list_expr.elems, first);
        while (match(p, TOK_COMMA) && !check(p, TOK_RBRACKET))
            node_list_push(&n->list_expr.elems, parse_expression(p));
        expect(p, TOK_RBRACKET, "expected ']'");
        return n;
    }
    if (match(p, TOK_LBRACE)) {
        if (match(p, TOK_RBRACE)) {
            /* Empty {} is a dict by default in Python */
            AstNode *n = ast_new(NODE_DICT_EXPR, line);
            node_list_init(&n->dict_expr.keys);
            node_list_init(&n->dict_expr.values);
            return n;
        }
        AstNode *first = parse_expression(p);
        if (match(p, TOK_COLON)) {
            /* It's a Dict */
            AstNode *n = ast_new(NODE_DICT_EXPR, line);
            node_list_init(&n->dict_expr.keys);
            node_list_init(&n->dict_expr.values);
            node_list_push(&n->dict_expr.keys, first);
            node_list_push(&n->dict_expr.values, parse_expression(p));
            while (match(p, TOK_COMMA) && !check(p, TOK_RBRACE)) {
                node_list_push(&n->dict_expr.keys, parse_expression(p));
                expect(p, TOK_COLON, "expected ':' in dict literal");
                node_list_push(&n->dict_expr.values, parse_expression(p));
            }
            expect(p, TOK_RBRACE, "expected '}'");
            return n;
        } else {
            /* It's a Set */
            AstNode *n = ast_new(NODE_SET_EXPR, line);
            node_list_init(&n->set_expr.elems);
            node_list_push(&n->set_expr.elems, first);
            while (match(p, TOK_COMMA) && !check(p, TOK_RBRACE)) {
                node_list_push(&n->set_expr.elems, parse_expression(p));
            }
            expect(p, TOK_RBRACE, "expected '}'");
            return n;
        }
    }
    /* Lambda expression */
    if (match(p, TOK_LAMBDA)) {
        AstNode *n = ast_new(NODE_LAMBDA, line);
        param_list_init(&n->lambda_expr.params);
        /* Parse optional parameters */
        if (!check(p, TOK_COLON)) {
            do {
                Param param;
                param.is_vararg = 0;
                param.is_kwarg = 0;
                param.type_ann = NULL;
                expect(p, TOK_IDENTIFIER, "expected parameter name");
                param.name = tok_to_str(p->previous);
                param_list_push(&n->lambda_expr.params, param);
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_COLON, "expected ':' in lambda");
        n->lambda_expr.body = parse_expression(p);
        return n;
    }
    error(p, "expected expression");
    return ast_new(NODE_NONE_LIT, line);
}

/* Parse postfix: calls, subscripts, attributes */
static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);
    while (!p->had_error) {
        int line = p->current.line;
        if (match(p, TOK_LPAREN)) {
            AstNode *call = ast_new(NODE_CALL, line);
            call->call.func = expr;
            node_list_init(&call->call.args);
            int cap = 4;
            call->call.is_unpacked_list = (int*)malloc(cap * sizeof(int));
            call->call.is_unpacked_dict = (int*)malloc(cap * sizeof(int));
            
            if (!check(p, TOK_RPAREN)) {
                do {
                    int is_list = 0;
                    int is_dict = 0;
                    if (match(p, TOK_STAR)) {
                        is_list = 1;
                    } else if (match(p, TOK_DSTAR)) {
                        is_dict = 1;
                    }
                    
                    if (call->call.args.count >= cap) {
                        cap *= 2;
                        call->call.is_unpacked_list = (int*)realloc(call->call.is_unpacked_list, cap * sizeof(int));
                        call->call.is_unpacked_dict = (int*)realloc(call->call.is_unpacked_dict, cap * sizeof(int));
                    }
                    
                    call->call.is_unpacked_list[call->call.args.count] = is_list;
                    call->call.is_unpacked_dict[call->call.args.count] = is_dict;
                    /* Parse the argument expression */
                    AstNode *arg_expr = parse_expression(p);
                    /* Check for keyword argument: parsed NAME followed by = */
                    if (arg_expr->type == NODE_NAME && match(p, TOK_ASSIGN)) {
                        AstNode *kw = ast_new(NODE_KWARG, arg_expr->line);
                        kw->kwarg.name = arg_expr->name_expr.name;
                        kw->kwarg.value = parse_expression(p);
                        arg_expr = kw;
                    }
                    node_list_push(&call->call.args, arg_expr);
                } while (match(p, TOK_COMMA) && !check(p, TOK_RPAREN));
            }
            expect(p, TOK_RPAREN, "expected ')'");
            expr = call;
        } else if (match(p, TOK_LBRACKET)) {
            AstNode *sub = ast_new(NODE_SUBSCRIPT, line);
            sub->subscript.obj = expr;
            sub->subscript.index = parse_expression(p);
            expect(p, TOK_RBRACKET, "expected ']'");
            expr = sub;
        } else if (match(p, TOK_DOT)) {
            expect(p, TOK_IDENTIFIER, "expected attribute name");
            AstNode *attr = ast_new(NODE_ATTRIBUTE, line);
            attr->attribute.obj = expr;
            attr->attribute.attr = tok_to_str(p->previous);
            expr = attr;
        } else {
            break;
        }
    }
    return expr;
}

/* Unary: -, +, not */
static AstNode *parse_unary(Parser *p) {
    int line = p->current.line;
    if (match(p, TOK_MINUS) || match(p, TOK_PLUS) || match(p, TOK_NOT)) {
        TokenType op = p->previous.type;
        AstNode *n = ast_new(NODE_UNARY_OP, line);
        n->unary_op.op = op;
        n->unary_op.operand = parse_unary(p);
        return n;
    }
    return parse_postfix(p);
}

/* Power: ** (right-associative) */
static AstNode *parse_power(Parser *p) {
    AstNode *left = parse_unary(p);
    if (match(p, TOK_DSTAR)) {
        int line = p->previous.line;
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = TOK_DSTAR;
        n->bin_op.right = parse_unary(p);
        return n;
    }
    return left;
}

/* Multiplicative: *, /, //, % */
static AstNode *parse_mul(Parser *p) {
    AstNode *left = parse_power(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_DSLASH) || check(p, TOK_PERCENT)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_power(p);
        left = n;
    }
    return left;
}

/* Additive: +, - */
static AstNode *parse_add(Parser *p) {
    AstNode *left = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_mul(p);
        left = n;
    }
    return left;
}

/* Comparison: ==, !=, <, >, <=, >= */
static AstNode *parse_comparison(Parser *p) {
    AstNode *left = parse_add(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ) || check(p, TOK_LT) ||
           check(p, TOK_GT) || check(p, TOK_LTE) || check(p, TOK_GTE)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_add(p);
        left = n;
    }
    return left;
}

/* not */
static AstNode *parse_not(Parser *p) {
    if (match(p, TOK_NOT)) {
        int line = p->previous.line;
        AstNode *n = ast_new(NODE_UNARY_OP, line);
        n->unary_op.op = TOK_NOT;
        n->unary_op.operand = parse_not(p);
        return n;
    }
    return parse_comparison(p);
}

/* and */
static AstNode *parse_and(Parser *p) {
    AstNode *left = parse_not(p);
    while (match(p, TOK_AND)) {
        int line = p->previous.line;
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = TOK_AND;
        n->bin_op.right = parse_not(p);
        left = n;
    }
    return left;
}

/* or (top-level expression) */
static AstNode *parse_expression(Parser *p) {
    AstNode *left = parse_and(p);
    while (match(p, TOK_OR)) {
        int line = p->previous.line;
        AstNode *n = ast_new(NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = TOK_OR;
        n->bin_op.right = parse_and(p);
        left = n;
    }
    return left;
}

/* --- Block parsing --- */
static void parse_block(Parser *p, NodeList *body) {
    expect(p, TOK_NEWLINE, "expected newline before block");
    skip_newlines(p);
    expect(p, TOK_INDENT, "expected indented block");
    while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF) && !p->had_error) {
        skip_newlines(p);
        if (check(p, TOK_DEDENT) || check(p, TOK_EOF)) break;
        AstNode *stmt = parse_statement(p);
        if (stmt) node_list_push(body, stmt);
    }
    if (check(p, TOK_DEDENT)) advance(p);
}

/* --- Statement parsing --- */

static AstNode *parse_func_def(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_DEF, "expected 'def'");
    expect(p, TOK_IDENTIFIER, "expected function name");

    AstNode *n = ast_new(NODE_FUNC_DEF, line);
    n->func_def.name = tok_to_str(p->previous);
    param_list_init(&n->func_def.params);
    node_list_init(&n->func_def.body);
    n->func_def.ret_type = NULL;

    expect(p, TOK_LPAREN, "expected '('");
    if (!check(p, TOK_RPAREN)) {
        do {
            Param param;
            param.is_vararg = 0;
            param.is_kwarg = 0;

            if (match(p, TOK_STAR)) {
                param.is_vararg = 1;
            } else if (match(p, TOK_DSTAR)) {
                param.is_kwarg = 1;
            }

            expect(p, TOK_IDENTIFIER, "expected parameter name");
            param.name = tok_to_str(p->previous);
            param.type_ann = NULL;
            if (match(p, TOK_COLON)) {
                expect(p, TOK_IDENTIFIER, "expected type");
                param.type_ann = tok_to_str(p->previous);
            }
            /* Skip default value: = expr */
            if (match(p, TOK_ASSIGN)) {
                parse_expression(p); /* discard default for now */
            }
            param_list_push(&n->func_def.params, param);
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, "expected ')'");

    if (match(p, TOK_ARROW)) {
        expect(p, TOK_IDENTIFIER, "expected return type");
        n->func_def.ret_type = tok_to_str(p->previous);
    }
    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->func_def.body);
    return n;
}

static AstNode *parse_class_def(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_CLASS, "expected 'class'");
    expect(p, TOK_IDENTIFIER, "expected class name");

    AstNode *n = ast_new(NODE_CLASS_DEF, line);
    n->class_def.name = tok_to_str(p->previous);
    node_list_init(&n->class_def.body);

    /* Optional inheritance (MyClass(BaseClass)): Not implemented for Phase 3 initially, skip */
    if (match(p, TOK_LPAREN)) {
        /* Accept but ignore for now */
        parse_expression(p);
        expect(p, TOK_RPAREN, "expected ')' after base class");
    }

    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->class_def.body);
    return n;
}

static AstNode *parse_if_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume if/elif */

    AstNode *n = ast_new(NODE_IF, line);
    n->if_stmt.cond = parse_expression(p);
    node_list_init(&n->if_stmt.then_body);
    n->if_stmt.else_branch = NULL;

    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->if_stmt.then_body);
    skip_newlines(p);

    if (check(p, TOK_ELIF)) {
        n->if_stmt.else_branch = parse_if_stmt(p);
    } else if (match(p, TOK_ELSE)) {
        expect(p, TOK_COLON, "expected ':'");
        AstNode *else_node = ast_new(NODE_PROGRAM, p->current.line);
        node_list_init(&else_node->program.stmts);
        parse_block(p, &else_node->program.stmts);
        n->if_stmt.else_branch = else_node;
    }
    return n;
}

static AstNode *parse_for_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'for' */
    expect(p, TOK_IDENTIFIER, "expected loop variable");

    AstNode *n = ast_new(NODE_FOR, line);
    n->for_stmt.var = tok_to_str(p->previous);
    node_list_init(&n->for_stmt.body);

    expect(p, TOK_IN, "expected 'in'");
    n->for_stmt.iter = parse_expression(p);
    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->for_stmt.body);
    return n;
}

static AstNode *parse_while_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'while' */

    AstNode *n = ast_new(NODE_WHILE, line);
    n->while_stmt.cond = parse_expression(p);
    node_list_init(&n->while_stmt.body);

    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->while_stmt.body);
    return n;
}

static AstNode *parse_return_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'return' */
    AstNode *n = ast_new(NODE_RETURN, line);
    n->return_stmt.value = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && !check(p, TOK_EOF))
        n->return_stmt.value = parse_expression(p);
    return n;
}

static AstNode *parse_const_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'const' */
    expect(p, TOK_IDENTIFIER, "expected constant name");
    AstNode *n = ast_new(NODE_CONST_DECL, line);
    n->const_decl.name = tok_to_str(p->previous);
    expect(p, TOK_ASSIGN, "expected '='");
    n->const_decl.value = parse_expression(p);
    return n;
}

static AstNode *parse_with_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'with' */

    AstNode *n = ast_new(NODE_WITH, line);
    n->with_stmt.alias = NULL;
    n->with_stmt.expr = parse_expression(p);

    if (match(p, TOK_AS)) {
        expect(p, TOK_IDENTIFIER, "expected alias name after 'as'");
        n->with_stmt.alias = tok_to_str(p->previous);
    }

    expect(p, TOK_COLON, "expected ':'");
    node_list_init(&n->with_stmt.body);
    parse_block(p, &n->with_stmt.body);

    return n;
}

static AstNode *parse_import_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'import' */
    expect(p, TOK_IDENTIFIER, "expected module name");

    AstNode *n = ast_new(NODE_IMPORT, line);
    n->import_stmt.module = tok_to_str(p->previous);
    n->import_stmt.alias = NULL;

    if (match(p, TOK_AS)) {
        expect(p, TOK_IDENTIFIER, "expected alias name");
        n->import_stmt.alias = tok_to_str(p->previous);
    }
    return n;
}

static AstNode *parse_try_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'try' */
    expect(p, TOK_COLON, "expected ':' after try");

    AstNode *n = ast_new(NODE_TRY, line);
    node_list_init(&n->try_stmt.body);
    node_list_init(&n->try_stmt.except_body);
    node_list_init(&n->try_stmt.finally_body);
    n->try_stmt.except_type = NULL;
    n->try_stmt.except_alias = NULL;

    parse_block(p, &n->try_stmt.body);

    skip_newlines(p);
    if (match(p, TOK_EXCEPT)) {
        if (!check(p, TOK_COLON)) {
            expect(p, TOK_IDENTIFIER, "expected Exception type");
            n->try_stmt.except_type = tok_to_str(p->previous);
            if (match(p, TOK_AS)) {
                expect(p, TOK_IDENTIFIER, "expected alias name after 'as'");
                n->try_stmt.except_alias = tok_to_str(p->previous);
            }
        }
        expect(p, TOK_COLON, "expected ':' after except");
        parse_block(p, &n->try_stmt.except_body);
    }

    skip_newlines(p);
    if (match(p, TOK_FINALLY)) {
        expect(p, TOK_COLON, "expected ':' after finally");
        parse_block(p, &n->try_stmt.finally_body);
    }

    return n;
}

static AstNode *parse_raise_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'raise' */
    AstNode *n = ast_new(NODE_RAISE, line);
    n->raise_stmt.exc = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && !check(p, TOK_EOF))
        n->raise_stmt.exc = parse_expression(p);
    return n;
}

/* Assignment or expression statement */
static AstNode *parse_assign_or_expr(Parser *p) {
    int line = p->current.line;
    AstNode *expr = parse_expression(p);
    if (p->had_error) return expr;

    /* Check for assignment: NAME = value or OBJ.ATTR = value */
    if (check(p, TOK_ASSIGN) && (expr->type == NODE_NAME || expr->type == NODE_ATTRIBUTE)) {
        advance(p);
        AstNode *n = ast_new(NODE_ASSIGN, line);
        
        if (expr->type == NODE_NAME) {
            n->assign.name = expr->name_expr.name;
            expr->name_expr.name = NULL; /* transfer ownership */
        } else {
            /* For phase 3/4 simplify property assignment to store the AST directly if needed, or stringify.
             * We will format it as obj.attr for the name struct.
             */
             /* Hack for now: we serialize obj.attr into a single string for assignment tracking */
             char buf[256];
             if (expr->attribute.obj->type == NODE_NAME) {
                 snprintf(buf, sizeof(buf), "%s.%s", expr->attribute.obj->name_expr.name, expr->attribute.attr);
                 n->assign.name = strdup(buf);
             } else {
                 n->assign.name = strdup(expr->attribute.attr); /* fallback */
             }
        }
        
        n->assign.type_ann = NULL;
        n->assign.value = parse_expression(p);
        free(expr);
        return n;
    }

    /* Check for augmented assignment: NAME += value or OBJ.ATTR += value */
    if ((check(p, TOK_PLUS_ASSIGN) || check(p, TOK_MINUS_ASSIGN) ||
         check(p, TOK_STAR_ASSIGN) || check(p, TOK_SLASH_ASSIGN)) &&
        (expr->type == NODE_NAME || expr->type == NODE_ATTRIBUTE)) {
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(NODE_AUG_ASSIGN, line);
        
        if (expr->type == NODE_NAME) {
            n->aug_assign.name = expr->name_expr.name;
            expr->name_expr.name = NULL;
        } else {
             char buf[256];
             if (expr->attribute.obj->type == NODE_NAME) {
                 snprintf(buf, sizeof(buf), "%s.%s", expr->attribute.obj->name_expr.name, expr->attribute.attr);
                 n->aug_assign.name = strdup(buf);
             } else {
                 n->aug_assign.name = strdup(expr->attribute.attr);
             }
        }
        
        n->aug_assign.op = op;
        n->aug_assign.value = parse_expression(p);
        free(expr);
        return n;
    }

    /* Check for annotated assignment: NAME: type = value */
    if (check(p, TOK_COLON) && expr->type == NODE_NAME) {
        advance(p); /* consume ':' */
        expect(p, TOK_IDENTIFIER, "expected type");
        char *type_ann = tok_to_str(p->previous);
        AstNode *n = ast_new(NODE_ASSIGN, line);
        n->assign.name = expr->name_expr.name;
        expr->name_expr.name = NULL;
        n->assign.type_ann = type_ann;
        n->assign.value = NULL;
        if (match(p, TOK_ASSIGN))
            n->assign.value = parse_expression(p);
        free(expr);
        return n;
    }

    /* Expression statement */
    AstNode *n = ast_new(NODE_EXPR_STMT, line);
    n->expr_stmt.expr = expr;
    return n;
}

static AstNode *parse_statement(Parser *p) {
    skip_newlines(p);
    if (p->had_error || check(p, TOK_EOF)) return NULL;

    AstNode *stmt = NULL;
    switch (p->current.type) {
        case TOK_DEF:       stmt = parse_func_def(p); break;
        case TOK_CLASS:     stmt = parse_class_def(p); break;
        case TOK_IF:        stmt = parse_if_stmt(p); break;
        case TOK_FOR:       stmt = parse_for_stmt(p); break;
        case TOK_WHILE:     stmt = parse_while_stmt(p); break;
        case TOK_RETURN:    stmt = parse_return_stmt(p); break;
        case TOK_CONST:     stmt = parse_const_stmt(p); break;
        case TOK_IMPORT:    stmt = parse_import_stmt(p); break;
        case TOK_WITH:      stmt = parse_with_stmt(p); break;
        case TOK_TRY:       stmt = parse_try_stmt(p); break;
        case TOK_RAISE:     stmt = parse_raise_stmt(p); break;
        case TOK_PASS:      advance(p); stmt = ast_new(NODE_PASS, p->previous.line); break;
        case TOK_BREAK:     advance(p); stmt = ast_new(NODE_BREAK, p->previous.line); break;
        case TOK_CONTINUE:  advance(p); stmt = ast_new(NODE_CONTINUE, p->previous.line); break;
        default:            stmt = parse_assign_or_expr(p); break;
    }

    /* Consume trailing newline */
    if (check(p, TOK_NEWLINE)) advance(p);
    return stmt;
}

/* --- Public API --- */
void parser_init(Parser *p, const char *source) {
    lexer_init(&p->lexer, source);
    p->had_error = 0;
    p->error_msg[0] = '\0';
    advance(p); /* load first token */
}

AstNode *parser_parse(Parser *p) {
    AstNode *prog = ast_new(NODE_PROGRAM, 1);
    node_list_init(&prog->program.stmts);
    skip_newlines(p);
    while (!check(p, TOK_EOF) && !p->had_error) {
        AstNode *stmt = parse_statement(p);
        if (stmt) node_list_push(&prog->program.stmts, stmt);
        skip_newlines(p);
    }
    return prog;
}
