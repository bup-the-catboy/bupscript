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

union VariableValue {
    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    bool boolean;
    char* string;
    struct Token* func;
    void* cfunc;
    union VariableValue* ptr;
};

struct Variable {
    BS_VarType vartype;
    union VariableValue value;
};

struct Variables {
    char* varname;
    BS_VarType vartype;
    uintptr_t address;
    struct Variables* next;
};

struct _BS_Context {
    struct Variables* variables;
    unsigned char* memory;
};

struct Token* BS_Lex(const char* script);
struct Variable BS_Execute(struct Token* tokens, BS_Context* context);

void BS_AddVariable(struct _BS_Context* context, char* varname, struct Variable var);

#endif