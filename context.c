#include "bupscript_internal.h"

#include <string.h>
#include <stdlib.h>

struct Variable* get_variable(BS_Context* context, const char* name) {
    struct _BS_Context* ctx = (struct _BS_Context*)context;
    while (ctx->next) {
        if (strcmp(ctx->varname, name) == 0) return &ctx->var;
        ctx = ctx->next;
    }
    return NULL;
}

BS_Context* BS_CreateContext() {
    return memset(malloc(sizeof(struct _BS_Context)), 0, sizeof(struct _BS_Context));
}

void BS_Eval(BS_Context* context, const char* script) {
    struct Token* tokens = BS_Lex(script);
    BS_Execute(tokens, context);
}

void BS_Call(BS_Context* context, const char* name, const char* params, ...) {
    struct Variable* func = get_variable(context, name);
    if (!func) {
        BS_AppendError(BS_Error_NotFound, 0, 0);
        return;
    }
    if (func->vartype == BS_func) BS_Execute(func->value.func, context);
    
}

void BS_Add(BS_Context* context, const char* name, BS_VarType type, void* value) {

}

void BS_DestroyContext(BS_Context* context) {

}