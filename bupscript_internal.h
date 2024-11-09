#ifndef BUPSCRIPT_INTERNAL_H
#define BUPSCRIPT_INTERNAL_H

#include "bupscript.h"

#include <stddef.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#define TOKEN(x) ,x
#define VALUE(x)

enum TokenType {
    Token_None
#include "tokens.h"
};

#undef  TOKEN
#define TOKEN(x) ,#x

UNUSED static const char* token_names[] = {
    "Token_None"
#include "tokens.h"
};

#undef  TOKEN
#undef  VALUE
#define TOKEN(x) ,""
#define VALUE(x) x

UNUSED static const char* token_values[] = {
    ""
#include "tokens.h"
};

#undef TOKEN
#undef VALUE

union TokenValue {
    char*   string;
    double  number;
    int64_t integer;
};

struct Token {
    enum TokenType type;
    union TokenValue value;
    int row;
    int col;
    struct Token* next;
};

struct Variables {
    BS_VarType vartype;
    bool lendable;
    int owner, curr_owner, block_owner;
    char* varname;
    BS_VariableValue* address;
    struct Variables* next;
    struct Variables* prev;
};

struct _BS_Context {
    struct Variables* variables;
    struct Variables* variables_head;
    unsigned char* memory;
    int curr_func, curr_block;
    size_t memsize, memused;
};

enum BS_EndState {
    BS_EndState_Semicolon,
    BS_EndState_Range,
    BS_EndState_Paren,
    BS_EndState_Bracket,
    BS_EndState_Brace,
    BS_EndState_List,
    BS_EndState_CodeBlock,
    BS_EndState_Operator,
};

struct Token* BS_Lex(const char* script);
int BS_FetchTokens(struct Token* tokens, enum BS_EndState end_state);
bool BS_Evaluate(struct Token* tokens, int* tokenptr, BS_Context* context, BS_Variable* out, enum BS_EndState end_state);
BS_Variable BS_Execute(struct Token* tokens, BS_Context* context);
struct Variables* BS_GetVariableInternal(struct _BS_Context* context, const char* name);
struct Variables* BS_AddNewVariable(struct _BS_Context* context, const char* varname, BS_Variable var);

#endif