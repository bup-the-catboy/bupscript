#include "bupscript_internal.h"

#include <stdio.h>

#define BS_FUNC_IMPL(name) struct Variable name(BS_Context* context, struct Variable* params, int num_params) { printf("called " #name "\n"); return (struct Variable){}; }

BS_FUNC_IMPL(bseval_create_variable);
BS_FUNC_IMPL(bseval_create_and_assign_variable);
BS_FUNC_IMPL(bseval_create_lendable_variable);
BS_FUNC_IMPL(bseval_create_and_assign_lendable_variable);
BS_FUNC_IMPL(bseval_borrow_from);
BS_FUNC_IMPL(bseval_assign_variable);
BS_FUNC_IMPL(bseval_swap_variables);
BS_FUNC_IMPL(bseval_array_indexer);
BS_FUNC_IMPL(bseval_array_length);
BS_FUNC_IMPL(bseval_called);
BS_FUNC_IMPL(bseval_func_call);
BS_FUNC_IMPL(bseval_function);
BS_FUNC_IMPL(bseval_function_returns);
BS_FUNC_IMPL(bseval_expr);
BS_FUNC_IMPL(bseval_if);
BS_FUNC_IMPL(bseval_elif);
BS_FUNC_IMPL(bseval_else);
BS_FUNC_IMPL(bseval_while);
BS_FUNC_IMPL(bseval_for);
BS_FUNC_IMPL(bseval_continue);
BS_FUNC_IMPL(bseval_break);
BS_FUNC_IMPL(bseval_break_multilevel);
BS_FUNC_IMPL(bseval_struct);
BS_FUNC_IMPL(bseval_struct_entry);
BS_FUNC_IMPL(bseval_codeblock);
BS_FUNC_IMPL(bseval_number);
BS_FUNC_IMPL(bseval_string_literal);
BS_FUNC_IMPL(bseval_identifier);
BS_FUNC_IMPL(bseval_modifier);
BS_FUNC_IMPL(bseval_operation);
BS_FUNC_IMPL(bseval_vararg_get);
BS_FUNC_IMPL(bseval_vararg_get_at);
BS_FUNC_IMPL(bseval_return_void);
BS_FUNC_IMPL(bseval_return_expr);
BS_FUNC_IMPL(bseval_incorrect_syntax);