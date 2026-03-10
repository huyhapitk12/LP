#ifndef LP_PARSER_H
#define LP_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    int had_error;
    char error_msg[512];
} Parser;

void parser_init(Parser *p, const char *source);
AstNode *parser_parse(Parser *p);  /* Returns NODE_PROGRAM */

#endif
