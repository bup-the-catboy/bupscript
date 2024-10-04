#include "bupscript_internal.h"

#define RUN(func) 0, (uintptr_t)func,
#define SPECIFIC(type) 1, type,
#define EQUALS SPECIFIC(Token_Symbol_Assign)
#define SEPARATOR SPECIFIC(Token_Symbol_Comma)
#define COLON SPECIFIC(Token_Symbol_Colon)
#define SWAP SPECIFIC(Token_Symbol_Swap)
#define ARRLEN SPECIFIC(Token_Symbol_ArrayIndexerLength)
#define VARARGS SPECIFIC(Token_Symbol_Vararg)
#define PARENOPEN SPECIFIC(Token_Symbol_ParenOpen)
#define PARENCLOSE SPECIFIC(Token_Symbol_ParenClose)
#define BRACKETOPEN SPECIFIC(Token_Symbol_SquareBraceOpen)
#define BRACKETCLOSE SPECIFIC(Token_Symbol_SquareBraceClose)
#define NUMBER SPECIFIC(Token_NumberLiteral)
#define STRLIT SPECIFIC(Token_StringLiteral)
#define STRUCT SPECIFIC(Token_Keyword_struct)
#define RETURN SPECIFIC(Token_Keyword_return)
#define RETURNS SPECIFIC(Token_Keyword_returns)
#define IF SPECIFIC(Token_Keyword_if)
#define ELSE SPECIFIC(Token_Keyword_else)
#define WHILE SPECIFIC(Token_Keyword_while)
#define FOR SPECIFIC(Token_Keyword_for)
#define IN SPECIFIC(Token_Keyword_in)
#define CONTINUE SPECIFIC(Token_Keyword_continue)
#define BREAK SPECIFIC(Token_Keyword_break)
#define TYPE 2,
#define EXPR 3,
#define IDENT 4,
#define PAREN 5,
#define BRACKETS 6,
#define BRACES 7,
#define ARGLIST 8,
#define CODEBLOCK 9,
#define MODIFIER 10,
#define OPERATOR 11,

void bseval_create_variable() {}
void bseval_create_and_assign_variable() {}
void bseval_assign_variable() {}
void bseval_swap_variables() {}
void bseval_array_indexer() {}
void bseval_array_length() {}
void bseval_function() {}
void bseval_function_returns() {}
void bseval_expr() {}
void bseval_if() {}
void bseval_else() {}
void bseval_while() {}
void bseval_for_inin() {}
void bseval_for_exin() {}
void bseval_for_inex() {}
void bseval_for_exex() {}
void bseval_continue() {}
void bseval_break() {}
void bseval_break_multilevel() {}
void bseval_struct() {}
void bseval_struct_entry() {}
void bseval_codeblock() {}
void bseval_number() {}
void bseval_string_literal() {}
void bseval_modifier() {}
void bseval_operation() {}
void bseval_vararg_get() {}
void bseval_vararg_get_at() {}
void bseval_return_void() {}
void bseval_return_expr() {}

const uintptr_t ruleset[] = {
#include "evaluator_ruleset.h"
};

struct Variable BS_Execute(struct Token* tokens, BS_Context* context) {
    // TODO
    return (struct Variable){};
}