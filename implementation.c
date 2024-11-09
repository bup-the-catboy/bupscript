#include "implementation.h"

#include <stdio.h>
#include <string.h>

#define BS_FUNC_IMPL_DBG(name) BS_Variable name(struct _BS_Context* context, BS_Variable* params, int num_params) { printf("called " #name "\n"); return (BS_Variable){}; }

// TODO: row and col error report

BS_FUNC_IMPL(bseval_create_variable) {
    BS_Variable var;
    var.vartype = params[0].value.u8;
    var.value.ptr = NULL;
    BS_AddNewVariable(context, params[1].value.string, var);
    return var;
}

BS_FUNC_IMPL(bseval_create_and_assign_variable) {
    params[2] = BS_Cast(params[2], params[0].value.s8);
    BS_AddNewVariable(context, params[1].value.string, params[2]);
    return params[2];
}

BS_FUNC_IMPL(bseval_create_lendable_variable) {
    BS_Variable var;
    var.vartype = params[0].value.u8;
    var.value.ptr = NULL;
    struct Variables* entry = BS_AddNewVariable(context, params[1].value.string, var);
    entry->lendable = true;
    return var;
}

BS_FUNC_IMPL(bseval_create_and_assign_lendable_variable) {
    params[2] = BS_Cast(params[2], params[0].value.s8);
    struct Variables* entry = BS_AddNewVariable(context, params[1].value.string, params[2]);
    entry->lendable = true;
    return params[2];
}

BS_FUNC_IMPL_DBG(bseval_borrow_from);

BS_FUNC_IMPL(bseval_assign_variable) {
    struct Variables* entry = BS_GetVariableInternal(context, params[0].value.string);
    if (!entry) {
        BS_AppendError(BS_Error_NotFound, 0, 0);
        return (BS_Variable){ .vartype = BS_void };
    }
    BS_Variable out = BS_Cast(params[1], entry->vartype);
    memcpy(entry->address, &out.value, BS_sizeof(entry->vartype));
    return out;
}

BS_FUNC_IMPL(bseval_swap_variables) {
    struct Variables* left  = BS_GetVariableInternal(context, params[0].value.string);
    struct Variables* right = BS_GetVariableInternal(context, params[1].value.string);
    if (!left || !right) {
        BS_AppendError(BS_Error_NotFound, 0, 0);
        return (BS_Variable){ .vartype = BS_void };
    }
    if (left->vartype != right->vartype) {
        BS_AppendError(BS_Error_VariableNotSameType, 0, 0);
        return (BS_Variable){ .vartype = BS_void };
    }
    BS_VariableValue temp = *left->address;
    memcpy(left->address, right->address, BS_sizeof(left->vartype));
    memcpy(right->address, &temp, BS_sizeof(left->vartype));
    return (BS_Variable){ .vartype = right->vartype, .value = *right->address };
}

BS_FUNC_IMPL_DBG(bseval_array_indexer);
BS_FUNC_IMPL_DBG(bseval_called);
BS_FUNC_IMPL_DBG(bseval_func_call);
BS_FUNC_IMPL_DBG(bseval_function);
BS_FUNC_IMPL_DBG(bseval_function_returns);

BS_FUNC_IMPL(bseval_expr) {
    return params[0];
}

BS_FUNC_IMPL_DBG(bseval_if);
BS_FUNC_IMPL_DBG(bseval_elif);
BS_FUNC_IMPL_DBG(bseval_else);
BS_FUNC_IMPL_DBG(bseval_while);
BS_FUNC_IMPL_DBG(bseval_for);
BS_FUNC_IMPL_DBG(bseval_continue);
BS_FUNC_IMPL_DBG(bseval_break);
BS_FUNC_IMPL_DBG(bseval_break_multilevel);
BS_FUNC_IMPL_DBG(bseval_struct);
BS_FUNC_IMPL_DBG(bseval_struct_entry);
BS_FUNC_IMPL_DBG(bseval_codeblock);

BS_FUNC_IMPL(bseval_number) {
    return params[0];
}

BS_FUNC_IMPL(bseval_string_literal) {
    params[0].vartype = BS_ptr(BS_s8);
    return params[0];
}

BS_FUNC_IMPL(bseval_identifier) {
    struct Variables* entry = BS_GetVariableInternal(context, params[0].value.string);
    if (!entry) {
        BS_AppendError(BS_Error_NotFound, 0, 0);
        return (BS_Variable){ .vartype = BS_void };
    }
    return (BS_Variable){ .vartype = entry->vartype, .value = *entry->address };
}

BS_FUNC_IMPL_DBG(bseval_modifier);
BS_FUNC_IMPL_DBG(bseval_operation);
BS_FUNC_IMPL_DBG(bseval_vararg_get);
BS_FUNC_IMPL_DBG(bseval_vararg_get_at);
BS_FUNC_IMPL_DBG(bseval_return_void);
BS_FUNC_IMPL_DBG(bseval_return_expr);
BS_FUNC_IMPL_DBG(bseval_incorrect_syntax);