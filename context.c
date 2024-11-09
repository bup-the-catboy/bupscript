#include "bupscript_internal.h"

#include <string.h>
#include <stdlib.h>

#define MEMORY_BLOCK_SIZE 0x1000

struct Variables* BS_GetVariableInternal(struct _BS_Context* context, const char* name) {
    struct Variables* curr = context->variables;
    while (curr) {
        if ((curr->curr_owner == 0 || curr->curr_owner == context->curr_func) && strcmp(curr->varname, name) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

BS_Variable BS_GetVariable(BS_Context* context, const char* name) {
    char* script = malloc(strlen(name) + 2);
    strcpy(script, name);
    strcat(script, ";");
    return BS_Eval(context, script);
}

BS_Context* BS_CreateContext() {
    struct _BS_Context* context = calloc(sizeof(struct _BS_Context), 1);
    context->memory = calloc(MEMORY_BLOCK_SIZE, 1);
    return (BS_Context*)context;
}

BS_Variable BS_Eval(BS_Context* context, const char* script) {
    struct Token* tokens = BS_Lex(script);
    BS_Variable value = BS_Execute(tokens, context);
    free(tokens);
    return value;
}

void* BS_AllocateMemory(struct _BS_Context* context, size_t size) {
    size_t prev_memsize = context->memsize;
    uintptr_t prev_memptr = (uintptr_t)context->memory;
    while (context->memused + size >= context->memsize) context->memsize += MEMORY_BLOCK_SIZE;
    if (prev_memsize != context->memsize) context->memory = realloc(context->memory, context->memsize);
    if (prev_memptr  != (uintptr_t)context->memory) {
        struct Variables* curr = context->variables;
        while (curr) {
            if (prev_memptr <= (uintptr_t)curr->address && (uintptr_t)curr->address < prev_memptr + prev_memsize) {
                uintptr_t offset = (uintptr_t)curr->address - prev_memptr;
                curr->address = (BS_VariableValue*)(context->memory + offset);
            }
            curr = curr->next;
        }
    }
    void* ptr = context->memory + context->memused;
    context->memused += size;
    return ptr;
}

BS_Variable BS_Call(BS_Context* context, const char* name, const char* params, ...) {
    
}

#define CASTFUNC(type) \
type __BS_Cast_##type(BS_Variable var) {          \
    switch (var.vartype) {                         \
        case BS_s8:    return (type)var.value.s8;   \
        case BS_s16:   return (type)var.value.s16;   \
        case BS_s32:   return (type)var.value.s32;    \
        case BS_s64:   return (type)var.value.s64;     \
        case BS_u8:    return (type)var.value.u8;       \
        case BS_u16:   return (type)var.value.u16;       \
        case BS_u32:   return (type)var.value.u32;        \
        case BS_u64:   return (type)var.value.u64;         \
        case BS_f32:   return (type)var.value.f32;          \
        case BS_f64:   return (type)var.value.f64;           \
        case BS_bool:  return (type)var.value.boolean;        \
        case BS_func:  return (type)(uintptr_t)var.value.func; \
        case BS_cfunc: return (type)(uintptr_t)var.value.func;  \
        case BS_void:  return (type)0;                           \
        default: return (type)(uintptr_t)var.value.ptr;           \
    }                                                              \
    return (type)0;                                                 \
}

CASTFUNC(int8_t);
CASTFUNC(int16_t);
CASTFUNC(int32_t);
CASTFUNC(int64_t);
CASTFUNC(uint8_t);
CASTFUNC(uint16_t);
CASTFUNC(uint32_t);
CASTFUNC(uint64_t);
CASTFUNC(uintptr_t);
CASTFUNC(float);
CASTFUNC(double);
CASTFUNC(bool);

BS_Variable BS_Cast(BS_Variable variable, BS_VarType target_type) {
    switch (target_type) {
        case BS_s8:    variable.value.s8      = __BS_Cast_int8_t  (variable); break;
        case BS_s16:   variable.value.s16     = __BS_Cast_int16_t (variable); break;
        case BS_s32:   variable.value.s32     = __BS_Cast_int32_t (variable); break;
        case BS_s64:   variable.value.s64     = __BS_Cast_int64_t (variable); break;
        case BS_u8:    variable.value.u8      = __BS_Cast_uint8_t (variable); break;
        case BS_u16:   variable.value.u16     = __BS_Cast_uint16_t(variable); break;
        case BS_u32:   variable.value.u32     = __BS_Cast_uint32_t(variable); break;
        case BS_u64:   variable.value.u64     = __BS_Cast_uint64_t(variable); break;
        case BS_f32:   variable.value.f32     = __BS_Cast_float   (variable); break;
        case BS_f64:   variable.value.f64     = __BS_Cast_double  (variable); break;
        case BS_bool:  variable.value.boolean = __BS_Cast_bool    (variable); break;
        case BS_void:  variable.value.func    =   NULL;                           break;
        default: variable.value.func = (void*)__BS_Cast_uintptr_t(variable);
    }
    variable.vartype = target_type;
    return variable;
}

struct Variables* BS_AddAddress(struct _BS_Context* context, const char* name, BS_VarType type, void* ptr) {
    struct Variables* entry = calloc(sizeof(struct Variables), 1);
    entry->varname = strdup(name);
    entry->vartype = type;
    entry->address = ptr;
    entry->owner = entry->curr_owner = context->curr_func;
    entry->block_owner = context->curr_block;
    if (!context->variables) context->variables = context->variables_head = entry;
    else {
        context->variables_head->next = entry;
        entry->prev = context->variables_head;
        context->variables_head = entry;
    }
    return entry;
}

struct Variables* BS_AddNewVariable(struct _BS_Context* context, const char* varname, BS_Variable var) {
    int size = BS_sizeof(var.vartype);
    void* ptr = BS_AllocateMemory(context, size);
    memcpy((void*)ptr, &var.value, size);
    return BS_AddAddress(context, varname, var.vartype, ptr);
}

void BS_AddVariable(BS_Context* context, const char* name, BS_VarType type, void* value) {
    if (type == BS_func) type = BS_cfunc;
    struct Variables* curr = BS_AddAddress((struct _BS_Context*)context, name, type, value);
    curr->owner = curr->curr_owner = curr->block_owner = 0; // global context
}

#define PRINT(fmt, prop) if (!(variable.vartype & 0xF0)) { \
    fprintf(stream, " " fmt, variable.value.prop);          \
    fflush (stream);                                         \
    return;                                                   \
}

void BS_PrintVariable(FILE* stream, BS_Variable variable) {
    if (variable.vartype == BS_void) {
        fputs("void", stream);
        return;
    }
    int num_ptrs = (variable.vartype >> 4) >> 0xF;
    int type = variable.vartype & 0xF;
    switch (type) {
        case BS_s8:    fputs("s8",    stream); PRINT("%d",  s8);  break;
        case BS_s16:   fputs("s16",   stream); PRINT("%d",  s16); break;
        case BS_s32:   fputs("s32",   stream); PRINT("%d",  s32); break;
        case BS_s64:   fputs("s64",   stream); PRINT("%ld", s64); break;
        case BS_u8:    fputs("u8",    stream); PRINT("%u",  u8);  break;
        case BS_u16:   fputs("u16",   stream); PRINT("%u",  u16); break;
        case BS_u32:   fputs("u32",   stream); PRINT("%u",  u32); break;
        case BS_u64:   fputs("u64",   stream); PRINT("%lu", u64); break;
        case BS_f32:   fputs("f32",   stream); PRINT("%g",  f32); break;
        case BS_f64:   fputs("f64",   stream); PRINT("%g",  f64); break;
        case BS_func:  fputs("func",  stream); PRINT("%p",  ptr); break;
        case BS_cfunc: fputs("cfunc", stream); PRINT("%p",  ptr); break;
        case BS_bool:  fputs("bool",  stream); PRINT("%s",  boolean ? "true" : "false"); break;
    }
    for (int i = 0; i < num_ptrs; i++) {
        fputc('*', stream);
    }
    fputc(' ', stream);
    if (variable.vartype == BS_ptr(BS_s8)) fprintf(stream, "%s", (char*)variable.value.ptr);
    else fprintf(stream, "%p", variable.value.ptr);
    fflush(stream);
}

void BS_DestroyContext(BS_Context* context) {

}