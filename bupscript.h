#ifndef BUPSCRIPT_H
#define BUPSCRIPT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {} BS_StructBuilder;
typedef struct {} BS_Context;
typedef uint8_t BS_VarType;

#define BS_void    0x0
#define BS_s8      0x1
#define BS_s16     0x2
#define BS_s32     0x3
#define BS_s64     0x4
#define BS_u8      0x5
#define BS_u16     0x6
#define BS_u32     0x7
#define BS_u64     0x8
#define BS_f32     0x9
#define BS_f64     0xA
#define BS_bool    0xB
#define BS_func    0xC
#define BS_cfunc   0xD
#define BS_ptr(x) ((((((x) >> 4) & 0xF) + 1) << 4) | (x & 0xF))

#define BS_sizeof(x) (                                            \
   (x & 0xF0) || x == BS_func || x == BS_cfunc ? sizeof(void*)   : \
    x == BS_bool                               ? sizeof(bool)    :  \
    x == BS_s8  || x == BS_u8                  ? sizeof(int8_t)  :   \
    x == BS_s16 || x == BS_u16                 ? sizeof(int16_t) :    \
    x == BS_s32 || x == BS_u32 || x == BS_f32  ? sizeof(int32_t) :     \
    x == BS_s64 || x == BS_u64 || x == BS_f64  ? sizeof(int64_t) : 0    \
)

#define ERROR(code, text) code,
enum BS_Error {
#include "errors.h"
};
#undef ERROR

typedef union BS_VariableValue BS_VariableValue;
union BS_VariableValue {
    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    bool boolean;
    char* string;
    void* func;
    BS_VariableValue* ptr;
};

typedef struct {
    BS_VarType vartype;
    BS_VariableValue value;
} BS_Variable;

BS_Context* BS_CreateContext();
BS_Variable BS_Eval(BS_Context* context, const char* script);
BS_Variable BS_Call(BS_Context* context, const char* func, const char* params, ...);
BS_Variable BS_Cast(BS_Variable variable, BS_VarType target_type);
BS_Variable BS_GetVariable(BS_Context* context, const char* name);
void BS_AddVariable(BS_Context* context, const char* name, BS_VarType type, void* value);
void BS_PrintVariable(FILE* stream, BS_Variable variable);
void BS_DestroyContext(BS_Context* context);

BS_StructBuilder* BS_CreateStruct();
void BS_StructNextOffset(BS_StructBuilder* str, int offset);
void BS_StructAdd(BS_StructBuilder* str, const char* type, const char* id);

void BS_AppendError(int code, int row, int col);
bool BS_HasError();
int  BS_GetError(int* row, int* col);
const char* BS_GetErrorString(int error);

#endif