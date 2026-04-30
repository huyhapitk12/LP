#ifndef LP_TYPE_INFERENCE_H
#define LP_TYPE_INFERENCE_H

#include "ast.h"
#include "codegen.h" /* For LpType */

/* Type environment for variable inference tracking */
typedef struct TypeEnv TypeEnv;
struct TypeEnv {
    struct {
        char *name;
        LpType type;
    } vars[512];
    int count;
    TypeEnv *parent;
};

void type_inference_init(TypeEnv *env);
void type_infer_program(TypeEnv *env, AstNode *program);
void type_infer_node(TypeEnv *env, AstNode *node);

#endif /* LP_TYPE_INFERENCE_H */
