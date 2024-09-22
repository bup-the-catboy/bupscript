#ifndef BUPSCRIPT_H
#define BUPSCRIPT_H

#include <stdint.h>
#include <stdbool.h>

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
#define BS_string  0xC
#define BS_struct  0xD
#define BS_func    0xE
#define BS_cfunc   0xF
#define BS_ptr(x) ((((((x) >> 4) & 0xF) + 1) << 4) | (x & 0xF))
#define BS_value(x) (void*)(x)

enum BS_Error {
    BS_Error_NoError,
    BS_Error_InvalidCharacter,
    BS_Error_UnexpectedEOL,
    BS_Error_UnexpectedEOF,
};

BS_Context* BS_CreateContext();
void BS_Eval(BS_Context* context, const char* script);
void BS_Call(BS_Context* context, const char* func, ...);
void BS_Add(BS_Context* context, BS_VarType type, void* value);
void BS_DestroyContext(BS_Context* context);

BS_StructBuilder* BS_CreateStruct();
void BS_StructNextOffset(BS_StructBuilder* str, int offset);
void BS_StructAdd(BS_StructBuilder* str, const char* type, const char* id);

void BS_AppendError(int code, int row, int col);
bool BS_HasError();
int  BS_GetError(int* row, int* col);
const char* BS_GetErrorString(int error);

#endif