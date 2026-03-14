#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lp_compat.h"

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

/* --- Type annotation parsing (supports unions: int | str and generics: Box[int]) --- */
static char *parse_type_annotation(Parser *p) {
    /* Parse a type annotation, which can be a single type, a union of types, or a generic type.
     * Examples: int, str, int | str, Box[int], dict[str, int]
     * Returns a string with types (e.g., "int|str|float" or "Box[int]" or "dict[str,int]")
     */
    if (!check(p, TOK_IDENTIFIER)) {
        error(p, "expected type identifier");
        return NULL;
    }
    
    /* Start with the first type */
    char *first_type = tok_to_str(p->current);
    advance(p);
    
    /* Check for generic type arguments: Box[int] or dict[str, int] */
    if (check(p, TOK_LBRACKET)) {
        /* Build a generic type string: Base[type1, type2, ...] */
        size_t capacity = strlen(first_type) + 64;
        char *result = (char *)malloc(capacity);
        if (!result) {
            free(first_type);
            return NULL;
        }
        size_t len = 0;
        
        /* Copy base type */
        size_t base_len = strlen(first_type);
        memcpy(result, first_type, base_len);
        len = base_len;
        free(first_type);
        
        /* Add opening bracket */
        result[len++] = '[';
        result[len] = '\0';
        
        advance(p); /* consume '[' */
        
        /* Parse type arguments */
        int first_arg = 1;
        while (!check(p, TOK_RBRACKET) && !p->had_error) {
            if (!first_arg) {
                expect(p, TOK_COMMA, "expected ',' between type arguments");
                if (len + 1 >= capacity) {
                    capacity = len + 64;
                    char *new_result = (char *)realloc(result, capacity);
                    if (!new_result) break;
                    result = new_result;
                }
                result[len++] = ',';
                result[len] = '\0';
            }
            first_arg = 0;
            
            /* Parse the type argument (could be another generic) */
            char *arg_type = parse_type_annotation(p);
            if (arg_type) {
                size_t arg_len = strlen(arg_type);
                if (len + arg_len + 1 >= capacity) {
                    capacity = len + arg_len + 64;
                    char *new_result = (char *)realloc(result, capacity);
                    if (!new_result) {
                        free(arg_type);
                        break;
                    }
                    result = new_result;
                }
                memcpy(result + len, arg_type, arg_len + 1);
                len += arg_len;
                free(arg_type);
            }
        }
        
        expect(p, TOK_RBRACKET, "expected ']' after type arguments");
        
        /* Add closing bracket */
        if (len + 1 >= capacity) {
            capacity = len + 2;
            char *new_result = (char *)realloc(result, capacity);
            if (new_result) result = new_result;
        }
        result[len++] = ']';
        result[len] = '\0';
        
        return result;
    }
    
    /* Check for union types */
    if (check(p, TOK_BIT_OR)) {
        /* Build a union type string */
        size_t capacity = 64;
        char *result = (char *)malloc(capacity);
        if (!result) {
            free(first_type);
            return NULL;
        }
        size_t len = 0;
        
        /* Copy first type */
        size_t first_len = strlen(first_type);
        if (first_len >= capacity) {
            capacity = first_len + 64;
            char *new_result = (char *)realloc(result, capacity);
            if (!new_result) {
                free(result);
                free(first_type);
                return NULL;
            }
            result = new_result;
        }
        memcpy(result, first_type, first_len + 1);
        len = first_len;
        free(first_type);
        
        /* Parse additional types in the union */
        while (match(p, TOK_BIT_OR)) {
            /* Skip optional whitespace (already handled by lexer) */
            if (!check(p, TOK_IDENTIFIER)) {
                error(p, "expected type identifier after '|'");
                break;
            }
            char *next_type = tok_to_str(p->current);
            advance(p);
            
            size_t next_len = strlen(next_type);
            size_t needed = len + 1 + next_len + 1; /* |type\0 */
            
            if (needed > capacity) {
                capacity = needed * 2;
                char *new_result = (char *)realloc(result, capacity);
                if (!new_result) {
                    free(next_type);
                    break;
                }
                result = new_result;
            }
            
            result[len++] = '|';
            memcpy(result + len, next_type, next_len + 1);
            len += next_len;
            free(next_type);
        }
        
        return result;
    } else {
        /* Single type, just return it */
        return first_type;
    }
}

/* --- Expression parsing (precedence climbing) --- */

static AstNode *parse_primary(Parser *p) {
    if (p->had_error) return NULL;
    int line = p->current.line;

    if (match(p, TOK_INT_LIT)) {
        AstNode *n = ast_new(p->arena, NODE_INT_LIT, line);
        n->int_lit.value = p->previous.int_val;
        return n;
    }
    if (match(p, TOK_FLOAT_LIT)) {
        AstNode *n = ast_new(p->arena, NODE_FLOAT_LIT, line);
        n->float_lit.value = p->previous.float_val;
        return n;
    }
    if (match(p, TOK_STRING_LIT)) {
        AstNode *n = ast_new(p->arena, NODE_STRING_LIT, line);
        n->str_lit.value = tok_to_str(p->previous);
        return n;
    }
    if (match(p, TOK_FSTRING_LIT)) {
        /* Parse f-string: f"Hello {name}!" 
         * The lexer returns the content without f and quotes.
         * We need to parse {expr} inside and create alternating string/expr parts.
         */
        AstNode *n = ast_new(NODE_FSTRING, line);
        node_list_init(&n->fstring_expr.parts);
        
        /* Get the raw f-string content */
        char *content = tok_to_str(p->previous);
        char *ptr = content;
        char *start = content;
        
        while (*ptr) {
            if (*ptr == '{') {
                /* Save string part before { */
                if (ptr > start) {
                    AstNode *str_part = ast_new(NODE_STRING_LIT, line);
                    size_t len = ptr - start;
                    str_part->str_lit.value = (char *)malloc(len + 1);
                    memcpy(str_part->str_lit.value, start, len);
                    str_part->str_lit.value[len] = '\0';
                    node_list_push(&n->fstring_expr.parts, str_part);
                }
                
                /* Skip { */
                ptr++;
                start = ptr;
                
                /* Handle {{ as escaped { */
                if (*ptr == '{') {
                    ptr++;
                    start = ptr;
                    continue;
                }
                
                /* Find closing } */
                int brace_count = 1;
                while (*ptr && brace_count > 0) {
                    if (*ptr == '{') brace_count++;
                    else if (*ptr == '}') brace_count--;
                    if (brace_count > 0) ptr++;
                }
                
                if (*ptr == '}') {
                    /* Extract expression */
                    size_t len = ptr - start;
                    char *expr_str = (char *)malloc(len + 1);
                    memcpy(expr_str, start, len);
                    expr_str[len] = '\0';
                    
                    /* Parse the expression */
                    Parser sub_parser;
                    parser_init(&sub_parser, expr_str);
                    AstNode *expr = parse_expression(&sub_parser);
                    free(expr_str);
                    
                    if (expr && !sub_parser.had_error) {
                        node_list_push(&n->fstring_expr.parts, expr);
                    } else {
                        /* Add None if parsing failed */
                        node_list_push(&n->fstring_expr.parts, ast_new(NODE_NONE_LIT, line));
                    }
                    
                    ptr++;
                    start = ptr;
                }
            } else if (*ptr == '}') {
                /* Handle }} as escaped } */
                if (*(ptr + 1) == '}') {
                    /* Save string part before }} */
                    if (ptr > start) {
                        AstNode *str_part = ast_new(NODE_STRING_LIT, line);
                        size_t len = ptr - start;
                        str_part->str_lit.value = (char *)malloc(len + 1);
                        memcpy(str_part->str_lit.value, start, len);
                        str_part->str_lit.value[len] = '\0';
                        node_list_push(&n->fstring_expr.parts, str_part);
                    }
                    ptr += 2;
                    start = ptr;
                } else {
                    ptr++;
                }
            } else {
                ptr++;
            }
        }
        
        /* Save remaining string part */
        if (ptr > start) {
            AstNode *str_part = ast_new(NODE_STRING_LIT, line);
            size_t len = ptr - start;
            str_part->str_lit.value = (char *)malloc(len + 1);
            memcpy(str_part->str_lit.value, start, len);
            str_part->str_lit.value[len] = '\0';
            node_list_push(&n->fstring_expr.parts, str_part);
        }
        
        free(content);
        return n;
    }
    if (match(p, TOK_TRUE) || match(p, TOK_FALSE)) {
        AstNode *n = ast_new(p->arena, NODE_BOOL_LIT, line);
        n->bool_lit.value = (p->previous.type == TOK_TRUE) ? 1 : 0;
        return n;
    }
    if (match(p, TOK_NONE)) {
        return ast_new(p->arena, NODE_NONE_LIT, line);
    }
    if (match(p, TOK_IDENTIFIER)) {
        AstNode *n = ast_new(p->arena, NODE_NAME, line);
        n->name_expr.name = tok_to_str(p->previous);
        return n;
    }
    if (match(p, TOK_LPAREN)) {
        int tuple_line = p->previous.line;
        if (match(p, TOK_RPAREN)) {
            AstNode *n = ast_new(p->arena, NODE_TUPLE_EXPR, tuple_line);
            node_list_init(&n->tuple_expr.elems);
            return n;
        }
        AstNode *expr = parse_expression(p);
        if (match(p, TOK_COMMA)) {
            /* It's a tuple */
            AstNode *n = ast_new(p->arena, NODE_TUPLE_EXPR, tuple_line);
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
            AstNode *n = ast_new(p->arena, NODE_LIST_EXPR, line);
            node_list_init(&n->list_expr.elems);
            return n;
        }
        AstNode *first = parse_expression(p);
        if (match(p, TOK_FOR)) {
            /* List comprehension: [expr for var in iter if cond] */
            AstNode *n = ast_new(p->arena, NODE_LIST_COMP, line);
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
        AstNode *n = ast_new(p->arena, NODE_LIST_EXPR, line);
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
            AstNode *n = ast_new(p->arena, NODE_DICT_EXPR, line);
            node_list_init(&n->dict_expr.keys);
            node_list_init(&n->dict_expr.values);
            return n;
        }
        AstNode *first = parse_expression(p);
        if (match(p, TOK_COLON)) {
            /* Could be dict literal OR dict comprehension */
            AstNode *key = first;
            AstNode *value = parse_expression(p);
            if (match(p, TOK_FOR)) {
                /* Dict comprehension: {key: val for var in iter if cond} */
                AstNode *n = ast_new(p->arena, NODE_DICT_COMP, line);
                n->dict_comp.key = key;
                n->dict_comp.value = value;
                expect(p, TOK_IDENTIFIER, "expected loop variable in dict comprehension");
                n->dict_comp.var = tok_to_str(p->previous);
                expect(p, TOK_IN, "expected 'in' in dict comprehension");
                n->dict_comp.iter = parse_expression(p);
                n->dict_comp.cond = NULL;
                if (match(p, TOK_IF)) {
                    n->dict_comp.cond = parse_expression(p);
                }
                expect(p, TOK_RBRACE, "expected '}'");
                return n;
            }
            /* Regular dict literal */
            AstNode *n = ast_new(p->arena, NODE_DICT_EXPR, line);
            node_list_init(&n->dict_expr.keys);
            node_list_init(&n->dict_expr.values);
            node_list_push(&n->dict_expr.keys, key);
            node_list_push(&n->dict_expr.values, value);
            while (match(p, TOK_COMMA) && !check(p, TOK_RBRACE)) {
                node_list_push(&n->dict_expr.keys, parse_expression(p));
                expect(p, TOK_COLON, "expected ':' in dict literal");
                node_list_push(&n->dict_expr.values, parse_expression(p));
            }
            expect(p, TOK_RBRACE, "expected '}'");
            return n;
        } else {
            /* It's a Set */
            AstNode *n = ast_new(p->arena, NODE_SET_EXPR, line);
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
        AstNode *n = ast_new(p->arena, NODE_LAMBDA, line);
        param_list_init(&n->lambda_expr.params);
        n->lambda_expr.is_multiline = 0;
        n->lambda_expr.body = NULL;
        node_list_init(&n->lambda_expr.body_stmts);
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
        /* Check for multiline lambda: lambda params:\n  block */
        if (check(p, TOK_NEWLINE)) {
            n->lambda_expr.is_multiline = 1;
            parse_block(p, &n->lambda_expr.body_stmts);
        } else {
            n->lambda_expr.body = parse_expression(p);
        }
        return n;
    }
    /* Yield expression */
    if (match(p, TOK_YIELD)) {
        AstNode *n = ast_new(NODE_YIELD, line);
        n->yield_expr.value = NULL;
        if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && 
            !check(p, TOK_EOF) && !check(p, TOK_RPAREN) && 
            !check(p, TOK_RBRACKET) && !check(p, TOK_RBRACE) &&
            !check(p, TOK_COMMA)) {
            n->yield_expr.value = parse_expression(p);
        }
        return n;
    }
    /* Await expression */
    if (match(p, TOK_AWAIT)) {
        AstNode *n = ast_new(NODE_AWAIT_EXPR, line);
        n->await_expr.expr = parse_expression(p);
        return n;
    }
    error(p, "expected expression");
    return ast_new(p->arena, NODE_NONE_LIT, line);
}

/* Parse postfix: calls, subscripts, attributes, and generic instantiation */
static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);
    while (!p->had_error) {
        int line = p->current.line;
        if (match(p, TOK_LPAREN)) {
            AstNode *call = ast_new(p->arena, NODE_CALL, line);
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
                        AstNode *kw = ast_new(p->arena, NODE_KWARG, arg_expr->line);
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
            /* Check for generic instantiation: Name[Type1, Type2, ...]()
             * We detect this by looking ahead: if we see identifiers followed by comma or ],
             * and then a '(' for a call, it's a generic instantiation.
             */
            if (expr->type == NODE_NAME && check(p, TOK_IDENTIFIER)) {
                /* Look ahead to see if this is a generic type argument list */
                /* Save parser state */
                Token saved_current = p->current;
                Token saved_previous = p->previous;
                
                /* Try to parse as type arguments */
                int is_generic = 1;
                advance(p); /* consume first identifier */
                
                while (match(p, TOK_COMMA)) {
                    if (!check(p, TOK_IDENTIFIER)) {
                        is_generic = 0;
                        break;
                    }
                    advance(p);
                }
                
                if (check(p, TOK_RBRACKET)) {
                    /* This could be a generic type instantiation */
                    advance(p); /* consume ']' */
                    
                    if (check(p, TOK_LPAREN)) {
                        /* This IS a generic instantiation followed by a call! */
                        AstNode *gen_inst = ast_new(p->arena, NODE_GENERIC_INST, line);
                        gen_inst->generic_inst.base_name = expr->name_expr.name;
                        node_list_init(&gen_inst->generic_inst.type_args);
                        
                        /* Now we need to re-parse the type arguments properly */
                        /* Reset and re-parse */
                        p->current = saved_current;
                        p->previous = saved_previous;
                        advance(p); /* consume '[' */
                        
                        do {
                            expect(p, TOK_IDENTIFIER, "expected type argument");
                            AstNode *type_arg = ast_new(p->arena, NODE_NAME, p->previous.line);
                            type_arg->name_expr.name = tok_to_str(p->previous);
                            node_list_push(&gen_inst->generic_inst.type_args, type_arg);
                        } while (match(p, TOK_COMMA));
                        
                        expect(p, TOK_RBRACKET, "expected ']'");
                        
                        /* Now parse the call */
                        expect(p, TOK_LPAREN, "expected '(' after generic instantiation");
                        
                        AstNode *call = ast_new(p->arena, NODE_CALL, line);
                        call->call.func = gen_inst;
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
                                AstNode *arg_expr = parse_expression(p);
                                if (arg_expr->type == NODE_NAME && match(p, TOK_ASSIGN)) {
                                    AstNode *kw = ast_new(p->arena, NODE_KWARG, arg_expr->line);
                                    kw->kwarg.name = arg_expr->name_expr.name;
                                    kw->kwarg.value = parse_expression(p);
                                    arg_expr = kw;
                                }
                                node_list_push(&call->call.args, arg_expr);
                            } while (match(p, TOK_COMMA) && !check(p, TOK_RPAREN));
                        }
                        expect(p, TOK_RPAREN, "expected ')'");
                        
                        /* Free the original name expression since we took ownership of the name */
                        free(expr);
                        expr = call;
                        continue;
                    } else {
                        /* Just a generic type reference (like in a type annotation context) */
                        /* Create a NODE_GENERIC_INST but don't call it */
                        AstNode *gen_inst = ast_new(p->arena, NODE_GENERIC_INST, line);
                        gen_inst->generic_inst.base_name = expr->name_expr.name;
                        node_list_init(&gen_inst->generic_inst.type_args);
                        
                        /* Re-parse the type arguments */
                        p->current = saved_current;
                        p->previous = saved_previous;
                        advance(p); /* consume '[' */
                        
                        do {
                            expect(p, TOK_IDENTIFIER, "expected type argument");
                            AstNode *type_arg = ast_new(p->arena, NODE_NAME, p->previous.line);
                            type_arg->name_expr.name = tok_to_str(p->previous);
                            node_list_push(&gen_inst->generic_inst.type_args, type_arg);
                        } while (match(p, TOK_COMMA));
                        
                        expect(p, TOK_RBRACKET, "expected ']'");
                        
                        free(expr);
                        expr = gen_inst;
                        continue;
                    }
                } else {
                    /* Not a generic type list, restore and parse as subscript */
                    p->current = saved_current;
                    p->previous = saved_previous;
                }
            }
            
            /* Parse as regular subscript */
            AstNode *sub = ast_new(p->arena, NODE_SUBSCRIPT, line);
            sub->subscript.obj = expr;
            sub->subscript.index = parse_expression(p);
            expect(p, TOK_RBRACKET, "expected ']'");
            expr = sub;
        } else if (match(p, TOK_DOT)) {
            expect(p, TOK_IDENTIFIER, "expected attribute name");
            AstNode *attr = ast_new(p->arena, NODE_ATTRIBUTE, line);
            attr->attribute.obj = expr;
            attr->attribute.attr = tok_to_str(p->previous);
            expr = attr;
        } else {
            break;
        }
    }
    return expr;
}

/* Unary: -, +, not, ~ */
static AstNode *parse_unary(Parser *p) {
    int line = p->current.line;
    if (match(p, TOK_MINUS) || match(p, TOK_PLUS) || match(p, TOK_NOT) || match(p, TOK_BIT_NOT)) {
        TokenType op = p->previous.type;
        AstNode *n = ast_new(p->arena, NODE_UNARY_OP, line);
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
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
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
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
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
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_mul(p);
        left = n;
    }
    return left;
}

/* Shift: <<, >> */
static AstNode *parse_shift(Parser *p) {
    AstNode *left = parse_add(p);
    while (check(p, TOK_LSHIFT) || check(p, TOK_RSHIFT)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_add(p);
        left = n;
    }
    return left;
}

/* Bitwise AND: & */
static AstNode *parse_bit_and(Parser *p) {
    AstNode *left = parse_shift(p);
    while (check(p, TOK_BIT_AND)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_shift(p);
        left = n;
    }
    return left;
}

/* Bitwise XOR: ^ */
static AstNode *parse_bit_xor(Parser *p) {
    AstNode *left = parse_bit_and(p);
    while (check(p, TOK_BIT_XOR)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_bit_and(p);
        left = n;
    }
    return left;
}

/* Bitwise OR: | */
static AstNode *parse_bit_or(Parser *p) {
    AstNode *left = parse_bit_xor(p);
    while (check(p, TOK_BIT_OR)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        n->bin_op.right = parse_bit_xor(p);
        left = n;
    }
    return left;
}

/* Comparison: ==, !=, <, >, <=, >=, is */
static AstNode *parse_comparison(Parser *p) {
    AstNode *left = parse_bit_or(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ) || check(p, TOK_LT) ||
           check(p, TOK_GT) || check(p, TOK_LTE) || check(p, TOK_GTE) ||
           check(p, TOK_IS)) {
        int line = p->current.line;
        TokenType op = p->current.type;
        advance(p);
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
        n->bin_op.left = left;
        n->bin_op.op = op;
        /* For 'is' operator, parse the type name (identifier) */
        if (op == TOK_IS) {
            /* Expect a type name (identifier) after 'is' */
            if (check(p, TOK_IDENTIFIER)) {
                AstNode *type_name = ast_new(NODE_NAME, line);
                type_name->name_expr.name = tok_to_str(p->current);
                advance(p);
                n->bin_op.right = type_name;
            } else {
                error(p, "expected type name after 'is'");
                n->bin_op.right = ast_new(NODE_NONE_LIT, line);
            }
        } else {
            n->bin_op.right = parse_add(p);
        }
        left = n;
    }
    return left;
}

/* not */
static AstNode *parse_not(Parser *p) {
    if (match(p, TOK_NOT)) {
        int line = p->previous.line;
        AstNode *n = ast_new(p->arena, NODE_UNARY_OP, line);
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
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
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
        AstNode *n = ast_new(p->arena, NODE_BIN_OP, line);
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

    AstNode *n = ast_new(p->arena, NODE_FUNC_DEF, line);
    n->func_def.name = tok_to_str(p->previous);
    param_list_init(&n->func_def.params);
    node_list_init(&n->func_def.body);
    node_list_init(&n->func_def.decorators);
    n->func_def.ret_type = NULL;
    n->func_def.access = 0;

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
                param.type_ann = parse_type_annotation(p);
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
        n->func_def.ret_type = parse_type_annotation(p);
    }
    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->func_def.body);
    return n;
}

static AstNode *parse_class_def(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_CLASS, "expected 'class'");
    expect(p, TOK_IDENTIFIER, "expected class name");

    AstNode *n = ast_new(p->arena, NODE_CLASS_DEF, line);
    n->class_def.name = tok_to_str(p->previous);
    n->class_def.base_class = NULL;
    type_param_list_init(&n->class_def.type_params);
    node_list_init(&n->class_def.body);

    /* Parse generic type parameters: class Name[T, U] */
    if (match(p, TOK_LBRACKET)) {
        do {
            expect(p, TOK_IDENTIFIER, "expected type parameter name");
            TypeParam tp;
            tp.name = tok_to_str(p->previous);
            type_param_list_push(&n->class_def.type_params, tp);
        } while (match(p, TOK_COMMA));
        expect(p, TOK_RBRACKET, "expected ']' after type parameters");
    }

    if (match(p, TOK_LPAREN)) {
        expect(p, TOK_IDENTIFIER, "expected base class name");
        n->class_def.base_class = tok_to_str(p->previous);
        expect(p, TOK_RPAREN, "expected ')' after base class");
    }

    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->class_def.body);
    return n;
}

static AstNode *parse_if_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume if/elif */

    AstNode *n = ast_new(p->arena, NODE_IF, line);
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
        AstNode *else_node = ast_new(p->arena, NODE_PROGRAM, p->current.line);
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

    AstNode *n = ast_new(p->arena, NODE_FOR, line);
    n->for_stmt.var = tok_to_str(p->previous);
    node_list_init(&n->for_stmt.body);

    expect(p, TOK_IN, "expected 'in'");
    n->for_stmt.iter = parse_expression(p);
    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->for_stmt.body);
    return n;
}

static AstNode *parse_parallel_for_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'parallel' */
    expect(p, TOK_FOR, "expected 'for' after 'parallel'");
    expect(p, TOK_IDENTIFIER, "expected loop variable");

    AstNode *n = ast_new(p->arena, NODE_PARALLEL_FOR, line);
    
    /* Use parallel_for struct for settings */
    n->parallel_for.var = tok_to_str(p->previous);
    node_list_init(&n->parallel_for.body);
    
    /* Initialize default settings */
    n->parallel_for.num_threads = 0;      /* auto */
    n->parallel_for.schedule = NULL;      /* default: static */
    n->parallel_for.chunk_size = 0;       /* auto */
    n->parallel_for.device_type = 0;      /* CPU */
    n->parallel_for.gpu_id = 0;

    expect(p, TOK_IN, "expected 'in'");
    n->parallel_for.iter = parse_expression(p);
    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->parallel_for.body);
    return n;
}

/* Parse @settings pragma - assumes '@' has already been consumed */
static AstNode *parse_settings_pragma(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_SETTINGS, "expected 'settings' after '@'");
    
    AstNode *n = ast_new(NODE_SETTINGS, line);
    
    /* Initialize with defaults */
    n->settings.enabled = 1;
    n->settings.num_threads = 0;  /* auto */
    n->settings.schedule = NULL;
    n->settings.chunk_size = 0;
    n->settings.device_type = 0;  /* CPU */
    n->settings.gpu_id = 0;
    n->settings.unified_memory = 0;
    n->settings.async_transfer = 0;
    
    /* Parse optional parentheses with settings */
    if (match(p, TOK_LPAREN)) {
        while (!check(p, TOK_RPAREN) && !p->had_error) {
            if (check(p, TOK_THREADS)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'threads'");
                if (check(p, TOK_INT_LIT)) {
                    n->settings.num_threads = (int)p->current.int_val;
                    advance(p);
                }
            } else if (check(p, TOK_SCHEDULE)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'schedule'");
                if (check(p, TOK_STRING_LIT)) {
                    n->settings.schedule = tok_to_str(p->previous);
                    advance(p);
                }
            } else if (check(p, TOK_CHUNK)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'chunk'");
                if (check(p, TOK_INT_LIT)) {
                    n->settings.chunk_size = p->current.int_val;
                    advance(p);
                }
            } else if (check(p, TOK_DEVICE)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'device'");
                if (check(p, TOK_STRING_LIT)) {
                    char *dev = tok_to_str(p->previous);
                    if (strcmp(dev, "gpu") == 0 || strcmp(dev, "cuda") == 0) {
                        n->settings.device_type = 1;
                    } else if (strcmp(dev, "auto") == 0) {
                        n->settings.device_type = 2;
                    } else {
                        n->settings.device_type = 0; /* CPU */
                    }
                    free(dev);
                    advance(p);
                } else if (check(p, TOK_GPU)) {
                    n->settings.device_type = 1;
                    advance(p);
                } else if (check(p, TOK_CPU)) {
                    n->settings.device_type = 0;
                    advance(p);
                }
            } else if (check(p, TOK_GPU)) {
                advance(p);
                n->settings.device_type = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_INT_LIT)) {
                        n->settings.gpu_id = (int)p->current.int_val;
                        advance(p);
                    }
                }
            } else if (check(p, TOK_UNIFIED)) {
                advance(p);
                n->settings.unified_memory = 1;
            } else if (check(p, TOK_IDENTIFIER)) {
                /* Generic keyword argument: name = value */
                char *key = tok_to_str(p->previous);
                advance(p);
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_INT_LIT)) {
                        /* Handle various integer settings */
                        if (strcmp(key, "threads") == 0) {
                            n->settings.num_threads = (int)p->current.int_val;
                        } else if (strcmp(key, "chunk") == 0) {
                            n->settings.chunk_size = p->current.int_val;
                        } else if (strcmp(key, "gpu_id") == 0) {
                            n->settings.gpu_id = (int)p->current.int_val;
                        }
                        advance(p);
                    } else if (check(p, TOK_STRING_LIT)) {
                        char *val = tok_to_str(p->previous);
                        if (strcmp(key, "schedule") == 0) {
                            n->settings.schedule = val;
                        } else if (strcmp(key, "device") == 0) {
                            if (strcmp(val, "gpu") == 0 || strcmp(val, "cuda") == 0) {
                                n->settings.device_type = 1;
                            } else if (strcmp(val, "auto") == 0) {
                                n->settings.device_type = 2;
                            }
                            free(val);
                        } else {
                            free(val);
                        }
                        advance(p);
                    } else if (check(p, TOK_TRUE)) {
                        if (strcmp(key, "unified") == 0) n->settings.unified_memory = 1;
                        advance(p);
                    } else if (check(p, TOK_FALSE)) {
                        if (strcmp(key, "unified") == 0) n->settings.unified_memory = 0;
                        advance(p);
                    }
                }
                free(key);
            }
            
            if (!match(p, TOK_COMMA)) break;
        }
        expect(p, TOK_RPAREN, "expected ')' after settings");
    }
    
    return n;
}

/* Parse @security pragma - assumes '@' has already been consumed */
static AstNode *parse_security_pragma(Parser *p) {
    int line = p->current.line;
    expect(p, TOK_SECURITY, "expected 'security' after '@'");
    
    AstNode *n = ast_new(NODE_SECURITY, line);
    
    /* Initialize with defaults */
    n->security.enabled = 1;
    n->security.level = 2;              /* medium by default */
    n->security.require_auth = 0;
    n->security.auth_type = NULL;
    n->security.rate_limit = 0;         /* no limit by default */
    n->security.validate_input = 1;
    n->security.sanitize_output = 1;
    n->security.prevent_injection = 1;
    n->security.prevent_xss = 1;
    n->security.prevent_csrf = 0;
    n->security.enable_cors = 0;
    n->security.cors_origins = NULL;
    n->security.secure_headers = 1;
    n->security.encrypt_data = 0;
    n->security.hash_algorithm = NULL;
    n->security.access_level = 1;       /* user by default */
    n->security.readonly = 0;
    
    /* Parse optional parentheses with security settings */
    if (match(p, TOK_LPAREN)) {
        while (!check(p, TOK_RPAREN) && !p->had_error) {
            if (check(p, TOK_LEVEL)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'level'");
                if (check(p, TOK_INT_LIT)) {
                    n->security.level = (int)p->current.int_val;
                    if (n->security.level > 4) n->security.level = 4;
                    if (n->security.level < 0) n->security.level = 0;
                    advance(p);
                } else if (check(p, TOK_STRING_LIT)) {
                    char *lvl = tok_to_str(p->previous);
                    advance(p);
                    if (strcmp(lvl, "none") == 0) n->security.level = 0;
                    else if (strcmp(lvl, "low") == 0) n->security.level = 1;
                    else if (strcmp(lvl, "medium") == 0) n->security.level = 2;
                    else if (strcmp(lvl, "high") == 0) n->security.level = 3;
                    else if (strcmp(lvl, "critical") == 0) n->security.level = 4;
                    free(lvl);
                }
            } else if (check(p, TOK_AUTH)) {
                advance(p);
                n->security.require_auth = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_STRING_LIT)) {
                        n->security.auth_type = tok_to_str(p->previous);
                        advance(p);
                    }
                }
            } else if (check(p, TOK_RATE)) {
                advance(p);
                expect(p, TOK_LIMIT, "expected 'limit' after 'rate'");
                expect(p, TOK_ASSIGN, "expected '=' after 'limit'");
                if (check(p, TOK_INT_LIT)) {
                    n->security.rate_limit = (int)p->current.int_val;
                    advance(p);
                }
            } else if (check(p, TOK_LIMIT)) {
                advance(p);
                expect(p, TOK_ASSIGN, "expected '=' after 'limit'");
                if (check(p, TOK_INT_LIT)) {
                    n->security.rate_limit = (int)p->current.int_val;
                    advance(p);
                }
            } else if (check(p, TOK_VALIDATE)) {
                advance(p);
                n->security.validate_input = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.validate_input = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.validate_input = 0; advance(p); }
                }
            } else if (check(p, TOK_SANITIZE)) {
                advance(p);
                n->security.sanitize_output = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.sanitize_output = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.sanitize_output = 0; advance(p); }
                }
            } else if (check(p, TOK_INJECTION)) {
                advance(p);
                n->security.prevent_injection = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.prevent_injection = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.prevent_injection = 0; advance(p); }
                }
            } else if (check(p, TOK_XSS)) {
                advance(p);
                n->security.prevent_xss = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.prevent_xss = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.prevent_xss = 0; advance(p); }
                }
            } else if (check(p, TOK_CSRF)) {
                advance(p);
                n->security.prevent_csrf = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.prevent_csrf = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.prevent_csrf = 0; advance(p); }
                }
            } else if (check(p, TOK_CORS)) {
                advance(p);
                n->security.enable_cors = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_STRING_LIT)) {
                        n->security.cors_origins = tok_to_str(p->previous);
                        advance(p);
                    } else if (check(p, TOK_TRUE)) { 
                        n->security.enable_cors = 1; 
                        advance(p); 
                    } else if (check(p, TOK_FALSE)) { 
                        n->security.enable_cors = 0; 
                        advance(p); 
                    }
                }
            } else if (check(p, TOK_HEADERS)) {
                advance(p);
                n->security.secure_headers = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.secure_headers = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.secure_headers = 0; advance(p); }
                }
            } else if (check(p, TOK_ENCRYPT)) {
                advance(p);
                n->security.encrypt_data = 1;
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_TRUE)) { n->security.encrypt_data = 1; advance(p); }
                    else if (check(p, TOK_FALSE)) { n->security.encrypt_data = 0; advance(p); }
                }
            } else if (check(p, TOK_HASH)) {
                advance(p);
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_STRING_LIT)) {
                        n->security.hash_algorithm = tok_to_str(p->previous);
                        advance(p);
                    }
                }
            } else if (check(p, TOK_READONLY)) {
                advance(p);
                n->security.readonly = 1;
            } else if (check(p, TOK_WRITE)) {
                advance(p);
                n->security.readonly = 0;
            } else if (check(p, TOK_ADMIN)) {
                advance(p);
                n->security.access_level = 2;
            } else if (check(p, TOK_USER)) {
                advance(p);
                n->security.access_level = 1;
            } else if (check(p, TOK_GUEST)) {
                advance(p);
                n->security.access_level = 0;
            } else if (check(p, TOK_IDENTIFIER)) {
                /* Generic keyword argument: name = value */
                char *key = tok_to_str(p->current);
                advance(p);
                if (match(p, TOK_ASSIGN)) {
                    if (check(p, TOK_INT_LIT)) {
                        int val = (int)p->current.int_val;
                        if (strcmp(key, "level") == 0) n->security.level = val;
                        else if (strcmp(key, "rate_limit") == 0 || strcmp(key, "limit") == 0) n->security.rate_limit = val;
                        else if (strcmp(key, "access_level") == 0) n->security.access_level = val;
                        advance(p);
                    } else if (check(p, TOK_STRING_LIT)) {
                        char *val = tok_to_str(p->previous);
                        advance(p);
                        if (strcmp(key, "auth_type") == 0) n->security.auth_type = val;
                        else if (strcmp(key, "hash") == 0 || strcmp(key, "hash_algorithm") == 0) n->security.hash_algorithm = val;
                        else if (strcmp(key, "cors_origins") == 0) n->security.cors_origins = val;
                        else if (strcmp(key, "level") == 0) {
                            if (strcmp(val, "none") == 0) n->security.level = 0;
                            else if (strcmp(val, "low") == 0) n->security.level = 1;
                            else if (strcmp(val, "medium") == 0) n->security.level = 2;
                            else if (strcmp(val, "high") == 0) n->security.level = 3;
                            else if (strcmp(val, "critical") == 0) n->security.level = 4;
                            free(val);
                        } else {
                            free(val);
                        }
                    } else if (check(p, TOK_TRUE)) {
                        if (strcmp(key, "auth") == 0) n->security.require_auth = 1;
                        else if (strcmp(key, "validate") == 0) n->security.validate_input = 1;
                        else if (strcmp(key, "sanitize") == 0) n->security.sanitize_output = 1;
                        else if (strcmp(key, "injection") == 0) n->security.prevent_injection = 1;
                        else if (strcmp(key, "xss") == 0) n->security.prevent_xss = 1;
                        else if (strcmp(key, "csrf") == 0) n->security.prevent_csrf = 1;
                        else if (strcmp(key, "cors") == 0) n->security.enable_cors = 1;
                        else if (strcmp(key, "headers") == 0) n->security.secure_headers = 1;
                        else if (strcmp(key, "encrypt") == 0) n->security.encrypt_data = 1;
                        else if (strcmp(key, "readonly") == 0) n->security.readonly = 1;
                        advance(p);
                    } else if (check(p, TOK_FALSE)) {
                        if (strcmp(key, "auth") == 0) n->security.require_auth = 0;
                        else if (strcmp(key, "validate") == 0) n->security.validate_input = 0;
                        else if (strcmp(key, "sanitize") == 0) n->security.sanitize_output = 0;
                        else if (strcmp(key, "injection") == 0) n->security.prevent_injection = 0;
                        else if (strcmp(key, "xss") == 0) n->security.prevent_xss = 0;
                        else if (strcmp(key, "csrf") == 0) n->security.prevent_csrf = 0;
                        else if (strcmp(key, "cors") == 0) n->security.enable_cors = 0;
                        else if (strcmp(key, "headers") == 0) n->security.secure_headers = 0;
                        else if (strcmp(key, "encrypt") == 0) n->security.encrypt_data = 0;
                        else if (strcmp(key, "readonly") == 0) n->security.readonly = 0;
                        advance(p);
                    }
                }
                free(key);
            }
            
            if (!match(p, TOK_COMMA)) break;
        }
        expect(p, TOK_RPAREN, "expected ')' after security settings");
    }
    
    return n;
}

static AstNode *parse_while_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'while' */

    AstNode *n = ast_new(p->arena, NODE_WHILE, line);
    n->while_stmt.cond = parse_expression(p);
    node_list_init(&n->while_stmt.body);

    expect(p, TOK_COLON, "expected ':'");
    parse_block(p, &n->while_stmt.body);
    return n;
}

static AstNode *parse_return_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'return' */
    AstNode *n = ast_new(p->arena, NODE_RETURN, line);
    n->return_stmt.value = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && !check(p, TOK_EOF))
        n->return_stmt.value = parse_expression(p);
    return n;
}

static AstNode *parse_const_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'const' */
    expect(p, TOK_IDENTIFIER, "expected constant name");
    AstNode *n = ast_new(p->arena, NODE_CONST_DECL, line);
    n->const_decl.name = tok_to_str(p->previous);
    expect(p, TOK_ASSIGN, "expected '='");
    n->const_decl.value = parse_expression(p);
    return n;
}

static AstNode *parse_with_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'with' */

    AstNode *n = ast_new(p->arena, NODE_WITH, line);
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

    AstNode *n = ast_new(p->arena, NODE_IMPORT, line);
    
    /* Accumulate dotted names */
    char *first_sub = tok_to_str(p->previous);
    size_t current_len = strlen(first_sub);
    
    /* Tính cap an toàn chống tràn số */
    size_t cap = current_len + 64;
    if (cap < current_len) cap = current_len + 1; 

    char *mod_name = (char *)malloc(cap);
    if (mod_name) {
        memcpy(mod_name, first_sub, current_len + 1);
    }
    free(first_sub);

    while (match(p, TOK_DOT)) {
        expect(p, TOK_IDENTIFIER, "expected sub-module name after '.'");
        char *sub = tok_to_str(p->previous);
        size_t sub_len = strlen(sub);
        
        /* Tính toán needed an toàn */
        size_t needed = current_len + 1 + sub_len + 1;
        
        if (needed > cap) {
            size_t new_cap = needed * 2;
            if (new_cap < needed) new_cap = needed; /* Chống tràn số khi x2 */
            
            char *new_mod = (char *)realloc(mod_name, new_cap);
            if (!new_mod) {
                /* OOM Guard: Ngừng nối chuỗi an toàn thay vì crash */
                free(sub);
                break; 
            }
            mod_name = new_mod;
            cap = new_cap;
        }
        
        if (mod_name) {
            mod_name[current_len++] = '.';
            memcpy(mod_name + current_len, sub, sub_len + 1);
            current_len += sub_len;
        }
        free(sub);
    }
    
    n->import_stmt.module = mod_name;
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

    AstNode *n = ast_new(p->arena, NODE_TRY, line);
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
    AstNode *n = ast_new(p->arena, NODE_RAISE, line);
    n->raise_stmt.exc = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && !check(p, TOK_EOF))
        n->raise_stmt.exc = parse_expression(p);
    return n;
}

/* Parse match statement: match value: case pattern: body ... */
static AstNode *parse_match_stmt(Parser *p) {
    int line = p->current.line;
    advance(p); /* consume 'match' */
    
    AstNode *n = ast_new(NODE_MATCH, line);
    n->match_stmt.value = parse_expression(p);
    node_list_init(&n->match_stmt.cases);
    
    expect(p, TOK_COLON, "expected ':' after match value");
    expect(p, TOK_NEWLINE, "expected newline before match cases");
    skip_newlines(p);
    expect(p, TOK_INDENT, "expected indented block for match cases");
    
    while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF) && !p->had_error) {
        skip_newlines(p);
        if (check(p, TOK_DEDENT) || check(p, TOK_EOF)) break;
        
        /* Must be a case */
        if (!match(p, TOK_CASE)) {
            error(p, "expected 'case' in match statement");
            break;
        }
        
        AstNode *case_node = ast_new(NODE_MATCH_CASE, p->previous.line);
        node_list_init(&case_node->match_case.body);
        case_node->match_case.guard = NULL;
        case_node->match_case.is_wildcard = 0;
        
        /* Parse pattern: could be literal, identifier (for capture), or _ for wildcard */
        if (match(p, TOK_IDENTIFIER)) {
            char *pattern_name = tok_to_str(p->previous);
            if (strcmp(pattern_name, "_") == 0) {
                case_node->match_case.is_wildcard = 1;
                case_node->match_case.pattern = NULL;
            } else {
                /* It's a capture variable - we'll treat it as a name expression */
                case_node->match_case.pattern = ast_new(NODE_NAME, p->previous.line);
                case_node->match_case.pattern->name_expr.name = pattern_name;
            }
        } else if (check(p, TOK_INT_LIT) || check(p, TOK_FLOAT_LIT) || 
                   check(p, TOK_STRING_LIT) || check(p, TOK_TRUE) || 
                   check(p, TOK_FALSE) || check(p, TOK_NONE)) {
            /* Literal pattern */
            case_node->match_case.pattern = parse_primary(p);
        } else {
            error(p, "expected pattern in case clause");
            case_node->match_case.pattern = ast_new(NODE_NONE_LIT, line);
        }
        
        /* Optional guard: if condition */
        if (match(p, TOK_IF)) {
            case_node->match_case.guard = parse_expression(p);
        }
        
        expect(p, TOK_COLON, "expected ':' after case pattern");
        parse_block(p, &case_node->match_case.body);
        
        node_list_push(&n->match_stmt.cases, case_node);
    }
    
    if (check(p, TOK_DEDENT)) advance(p);
    return n;
}

/* Assignment or expression statement */
static AstNode *parse_assign_or_expr(Parser *p) {
    int line = p->current.line;
    AstNode *expr = parse_expression(p);
    if (p->had_error) return expr;

    /* Check for assignment: NAME = value or OBJ.ATTR = value or OBJ[INDEX] = value */
    if (check(p, TOK_ASSIGN) && (expr->type == NODE_NAME || expr->type == NODE_ATTRIBUTE || expr->type == NODE_SUBSCRIPT)) {
        advance(p);
        
        if (expr->type == NODE_SUBSCRIPT) {
            AstNode *n = ast_new(p->arena, NODE_SUBSCRIPT_ASSIGN, line);
            n->subscript_assign.obj = expr->subscript.obj;
            n->subscript_assign.index = expr->subscript.index;
            n->subscript_assign.value = parse_expression(p);
            n->subscript_assign.op = TOK_ASSIGN;
            
            /* Nullify to prevent double free */
            expr->subscript.obj = NULL;
            expr->subscript.index = NULL;

            return n;
        }

        AstNode *n = ast_new(p->arena, NODE_ASSIGN, line);
        
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
        n->assign.access = 0;

        return n;
    }

    /* Check for augmented assignment: NAME += value or OBJ.ATTR += value or OBJ[INDEX] += value */
    if ((check(p, TOK_PLUS_ASSIGN) || check(p, TOK_MINUS_ASSIGN) ||
         check(p, TOK_STAR_ASSIGN) || check(p, TOK_SLASH_ASSIGN) ||
         check(p, TOK_BIT_AND_ASSIGN) || check(p, TOK_BIT_OR_ASSIGN) ||
         check(p, TOK_BIT_XOR_ASSIGN) || check(p, TOK_LSHIFT_ASSIGN) ||
         check(p, TOK_RSHIFT_ASSIGN)) &&
        (expr->type == NODE_NAME || expr->type == NODE_ATTRIBUTE || expr->type == NODE_SUBSCRIPT)) {
        TokenType op = p->current.type;
        advance(p);
        
        if (expr->type == NODE_SUBSCRIPT) {
            AstNode *n = ast_new(p->arena, NODE_SUBSCRIPT_ASSIGN, line);
            n->subscript_assign.obj = expr->subscript.obj;
            n->subscript_assign.index = expr->subscript.index;
            n->subscript_assign.value = parse_expression(p);
            n->subscript_assign.op = op;
            
            /* Nullify to prevent double free */
            expr->subscript.obj = NULL;
            expr->subscript.index = NULL;

            return n;
        }

        AstNode *n = ast_new(p->arena, NODE_AUG_ASSIGN, line);
        
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

        return n;
    }

    /* Check for annotated assignment: NAME: type = value */
    if (check(p, TOK_COLON) && expr->type == NODE_NAME) {
        advance(p); /* consume ':' */
        char *type_ann = parse_type_annotation(p);
        AstNode *n = ast_new(p->arena, NODE_ASSIGN, line);
        n->assign.name = expr->name_expr.name;
        expr->name_expr.name = NULL;
        n->assign.type_ann = type_ann;
        n->assign.value = NULL;
        n->assign.access = 0;
        if (match(p, TOK_ASSIGN))
            n->assign.value = parse_expression(p);

        return n;
    }

    /* Expression statement */
    AstNode *n = ast_new(p->arena, NODE_EXPR_STMT, line);
    n->expr_stmt.expr = expr;
    return n;
}

static AstNode *parse_statement(Parser *p) {
    skip_newlines(p);
    if (p->had_error || check(p, TOK_EOF)) return NULL;

    /* Collect decorators */
    NodeList decorators;
    node_list_init(&decorators);
    
    while (check(p, TOK_AT)) {
        advance(p);  /* consume '@' */
        
        /* Check for @settings pragma */
        if (check(p, TOK_SETTINGS)) {
            /* Parse settings pragma - we'll handle this specially */
            AstNode *settings = parse_settings_pragma(p);
            if (settings) {
                node_list_push(&decorators, settings);
            }
        } else if (check(p, TOK_SECURITY)) {
            /* Parse security pragma - for input validation and access control */
            AstNode *security = parse_security_pragma(p);
            if (security) {
                node_list_push(&decorators, security);
            }
        } else {
            /* Regular decorator: @decorator or @decorator(args) */
            AstNode *decorator = parse_expression(p);
            if (decorator) {
                node_list_push(&decorators, decorator);
            }
        }
        skip_newlines(p);
    }

    TokenType access = 0;
    if (match(p, TOK_PRIVATE)) {
        access = TOK_PRIVATE;
    } else if (match(p, TOK_PROTECTED)) {
        access = TOK_PROTECTED;
    }

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
        case TOK_PARALLEL:  stmt = parse_parallel_for_stmt(p); break;
        case TOK_MATCH:     stmt = parse_match_stmt(p); break;
        case TOK_PASS:      advance(p); stmt = ast_new(p->arena, NODE_PASS, p->previous.line); break;
        case TOK_BREAK:     advance(p); stmt = ast_new(p->arena, NODE_BREAK, p->previous.line); break;
        case TOK_CONTINUE:  advance(p); stmt = ast_new(p->arena, NODE_CONTINUE, p->previous.line); break;
        case TOK_ASYNC: {
            /* async def name(params): body */
            advance(p); /* consume 'async' */
            if (!match(p, TOK_DEF)) {
                error(p, "expected 'def' after 'async'");
                stmt = NULL;
            } else {
                /* Parse async function - similar to regular function but mark as async */
                int async_line = p->previous.line;
                expect(p, TOK_IDENTIFIER, "expected function name");
                
                AstNode *n = ast_new(p->arena, NODE_ASYNC_DEF, async_line);
                n->async_def.name = tok_to_str(p->previous);
                param_list_init(&n->async_def.params);
                node_list_init(&n->async_def.body);
                node_list_init(&n->async_def.decorators);
                n->async_def.ret_type = NULL;
                n->async_def.access = access;
                
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
                            param.type_ann = parse_type_annotation(p);
                        }
                        /* Skip default value: = expr */
                        if (match(p, TOK_ASSIGN)) {
                            parse_expression(p); /* discard default for now */
                        }
                        param_list_push(&n->async_def.params, param);
                    } while (match(p, TOK_COMMA));
                }
                expect(p, TOK_RPAREN, "expected ')'");
                
                if (match(p, TOK_ARROW)) {
                    n->async_def.ret_type = parse_type_annotation(p);
                }
                expect(p, TOK_COLON, "expected ':'");
                parse_block(p, &n->async_def.body);
                
                /* Attach decorators */
                if (decorators.count > 0) {
                    n->async_def.decorators = decorators;
                }
                
                stmt = n;
            }
            break;
        }
        default:            stmt = parse_assign_or_expr(p); break;
    }

    /* Attach decorators to function definition */
    if (stmt && stmt->type == NODE_FUNC_DEF && decorators.count > 0) {
        stmt->func_def.decorators = decorators;
    } else if (stmt && stmt->type == NODE_ASYNC_DEF && decorators.count > 0) {
        stmt->async_def.decorators = decorators;
    } else if (decorators.count > 0) {
        /* Decorators on non-function statements - for now, just free them */
        for (int i = 0; i < decorators.count; i++) {
            ast_free(decorators.items[i]);
        }
        free(decorators.items);
    }

    if (stmt) {
        if (stmt->type == NODE_FUNC_DEF) {
            stmt->func_def.access = access;
        } else if (stmt->type == NODE_ASYNC_DEF) {
            stmt->async_def.access = access;
        } else if (stmt->type == NODE_ASSIGN) {
            stmt->assign.access = access;
        } else if (access != 0) {
            error(p, "access modifier only allowed on functions and assignments");
        }
    }

    /* Consume trailing newline */
    if (check(p, TOK_NEWLINE)) advance(p);
    return stmt;
}

/* --- Public API --- */
void parser_init(Parser *p, const char *source, LpArena *arena) {
    lexer_init(&p->lexer, source);
    p->had_error = 0;
    p->error_msg[0] = '\0';
    p->arena = arena;
    advance(p); /* load first token */
}

AstNode *parser_parse(Parser *p) {
    AstNode *prog = ast_new(p->arena, NODE_PROGRAM, 1);
    node_list_init(&prog->program.stmts);
    skip_newlines(p);
    while (!check(p, TOK_EOF) && !p->had_error) {
        AstNode *stmt = parse_statement(p);
        if (stmt) node_list_push(&prog->program.stmts, stmt);
        skip_newlines(p);
    }
    return prog;
}
