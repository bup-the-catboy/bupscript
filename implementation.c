#include "implementation.h"

#include <stdio.h>

#define BS_FUNC_IMPL_DBG(name) BS_Variable name(struct _BS_Context* context, BS_Variable* params, int num_params) { printf("called " #name "\n"); return (BS_Variable){}; }

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
BS_FUNC_IMPL_DBG(bseval_assign_variable);
BS_FUNC_IMPL_DBG(bseval_swap_variables);
BS_FUNC_IMPL_DBG(bseval_array_indexer);
BS_FUNC_IMPL_DBG(bseval_called);
BS_FUNC_IMPL_DBG(bseval_func_call);
BS_FUNC_IMPL_DBG(bseval_function);
BS_FUNC_IMPL_DBG(bseval_function_returns);
BS_FUNC_IMPL_DBG(bseval_expr);
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

BS_FUNC_IMPL_DBG(bseval_string_literal);
BS_FUNC_IMPL_DBG(bseval_identifier);
BS_FUNC_IMPL_DBG(bseval_modifier);
BS_FUNC_IMPL_DBG(bseval_operation);
BS_FUNC_IMPL_DBG(bseval_vararg_get);
BS_FUNC_IMPL_DBG(bseval_vararg_get_at);
BS_FUNC_IMPL_DBG(bseval_return_void);
BS_FUNC_IMPL_DBG(bseval_return_expr);
BS_FUNC_IMPL_DBG(bseval_incorrect_syntax);