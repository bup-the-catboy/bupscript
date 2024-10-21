#include "bupscript_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRACE_OPEN {
#define BRACE_CLOSE }

#define RUN(func) 0, (uintptr_t)func BRACE_CLOSE, BRACE_OPEN
#define SPECIFIC(type) 1, type,
#define EQUALS SPECIFIC(Token_Symbol_Assign)
#define COLON SPECIFIC(Token_Symbol_Colon)
#define SWAP SPECIFIC(Token_Symbol_Swap)
#define ARRLEN SPECIFIC(Token_Symbol_ArrayIndexerLength)
#define VARARGS SPECIFIC(Token_Symbol_Vararg)
#define RANGE_ARROW SPECIFIC(Token_Symbol_RangeArrow)
#define REVERSE_ARROW SPECIFIC(Token_Symbol_ReverseArrow)
#define PARENOPEN SPECIFIC(Token_Symbol_ParenOpen)
#define PARENCLOSE SPECIFIC(Token_Symbol_ParenClose)
#define BRACKETOPEN SPECIFIC(Token_Symbol_SquareBraceOpen)
#define BRACKETCLOSE SPECIFIC(Token_Symbol_SquareBraceClose)
#define BRACEOPEN SPECIFIC(Token_Symbol_CurlyBraceOpen)
#define BRACECLOSE SPECIFIC(Token_Symbol_CurlyBraceClose)
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
#define IDENT 3,
#define EXPR 4,
#define EXPRLIST 5,
#define ARGLIST 6,
#define CODEBLOCK 7,
#define MODIFIER 8,
#define OPERATOR 9,

typedef struct Variable(*BS_EvaluatorFunc)(BS_Context* context, struct Variable* params, int num_params);

enum TokenType operation_tokens[] = {
    Token_Symbol_Plus,
    Token_Symbol_Minus,
    Token_Symbol_Multiply,
    Token_Symbol_Divide,
    Token_Symbol_Modulo,
    Token_Symbol_Power,
    Token_Symbol_BitShiftLeft,
    Token_Symbol_BitShiftRight,
    Token_Symbol_And,
    Token_Symbol_Or,
    Token_Symbol_Tilde,
    Token_Symbol_EqualTo,
    Token_Symbol_NotEqualTo,
    Token_Symbol_LessThan,
    Token_Symbol_LessEqualTo,
    Token_Symbol_GreaterThan,
    Token_Symbol_GreaterEqualTo,
};

enum TokenType modification_tokens[] = {
    Token_Symbol_Tilde,
    Token_Symbol_DoubleTilde,
    Token_Symbol_Minus,
    Token_Symbol_Invert,
    Token_Symbol_And,
    Token_Symbol_Multiply
};

enum BS_Operations {
    BS_Operation_Addition,
    BS_Operation_Subtraction,
    BS_Operation_Multiplication,
    BS_Operation_Division,
    BS_Operation_Exponentiation,
    BS_Operation_Modulo,
    BS_Operation_BitShiftLeft,
    BS_Operation_BitShiftRight,
    BS_Operation_And,
    BS_Operation_Or,
    BS_Operation_Xor,
    BS_Operation_EqualTo,
    BS_Operation_NotEqualTo,
    BS_Operation_LessThan,
    BS_Operation_LessEqualTo,
    BS_Operation_GreaterThan,
    BS_Operation_GreaterEqualTo,
};

enum BS_Modifications {
    BS_Modification_Flip,
    BS_Modification_FlipBytes,
    BS_Modification_Negate,
    BS_Modification_Invert,
    BS_Modification_Reference,
    BS_Modification_Dereference,
};

enum BS_EndState {
    BS_EndState_Semicolon,
    BS_EndState_Arrow,
    BS_EndState_ReverseArrow,
    BS_EndState_Paren,
    BS_EndState_Bracket,
    BS_EndState_Brace,
    BS_EndState_List,
    BS_EndState_CodeBlock,
    BS_EndState_Operator,
};

#include "implementation.h"

const uintptr_t ruleset[][24] = {
    {
#include "evaluator_ruleset.h"
    0, (uintptr_t)bseval_incorrect_syntax }
};

bool get_type(struct Token** tokens, BS_VarType* vartype) {
    struct Token* t = *tokens;
    switch (t->type) {
        case Token_VarType_s8:   *vartype = BS_s8;   break;
        case Token_VarType_s16:  *vartype = BS_s16;  break;
        case Token_VarType_s32:  *vartype = BS_s32;  break;
        case Token_VarType_s64:  *vartype = BS_s64;  break;
        case Token_VarType_u8:   *vartype = BS_u8;   break;
        case Token_VarType_u16:  *vartype = BS_u16;  break;
        case Token_VarType_u32:  *vartype = BS_u32;  break;
        case Token_VarType_u64:  *vartype = BS_u64;  break;
        case Token_VarType_f32:  *vartype = BS_f32;  break;
        case Token_VarType_f64:  *vartype = BS_f64;  break;
        case Token_VarType_bool: *vartype = BS_bool; break;
        case Token_VarType_func: *vartype = BS_func; break;
        case Token_VarType_void: *vartype = BS_void; break;
        default: return false;
    }
    while ((++t)->type == Token_Symbol_Multiply) *vartype = BS_ptr(*vartype);
    *tokens = t;
    return true;
}

#define push(type, val) parameters[num_parameters++].value.type = val
#define pushvar(val) parameters[num_parameters++] = val
#define token cfunc
#define curr_token ({      \
    inner_t = t + tokenptr; \
    &inner_t;                \
})
#define update_tokenptr() tokenptr = ((uintptr_t)inner_t - (uintptr_t)t) / sizeof(struct Token)
#define arrsize(arr) (sizeof(arr) / sizeof(*(arr)))
#define get_token_index(tkn, arr) _get_token_index(tkn, arr, arrsize(arr))

int _get_token_index(enum TokenType type, enum TokenType* arr, int num) {
    int i = 0;
    for (; i < num; i++) {
        if (type == arr[i]) break;
    }
    return i;
}

#define bracket_err() {                                          \
    BS_AppendError(BS_Error_InvalidBracketStack, t->row, t->col); \
    return NULL;                                                   \
}

#define validate_bracket_close(symbol)                                     \
    if (paren_stack_ptr == 0 || paren_stack[--paren_stack_ptr] != symbol) { \
        BS_AppendError(BS_Error_InvalidBracketStack, t->row, t->col);        \
        return NULL;                                                          \
    }                                                                          \


struct Token* fetch_tokens(struct Token** tokens, enum BS_EndState end_state) {
    struct Token* t = *tokens;
    struct Token* prev_t = NULL;
    if (!t) return NULL;
    char paren_stack[1024];
    int paren_stack_ptr = 0;
    int orig_paren_stack_ptr = 0;
    bool can_end = true;
    bool do_loop = true;
    int size = 0;
    while (do_loop) {
        if (!t) {
            BS_AppendError(BS_Error_UnexpectedEOE, prev_t->row, prev_t->col);
            return NULL;
        }
        if (can_end && paren_stack_ptr == 0) {
            if ((
                end_state == BS_EndState_List && (
                    t->type == Token_Symbol_Comma ||
                    t->type == Token_Symbol_ParenClose
                )
            ) || (
                end_state == BS_EndState_CodeBlock && (
                    t->type == Token_Symbol_CurlyBraceOpen ||
                    t->type == Token_Symbol_Arrow
                )
            ) || (
                end_state == BS_EndState_Operator && (
                    t->type == Token_Symbol_Plus ||
                    t->type == Token_Symbol_Minus ||
                    t->type == Token_Symbol_Multiply ||
                    t->type == Token_Symbol_Divide ||
                    t->type == Token_Symbol_Modulo ||
                    t->type == Token_Symbol_Power ||
                    t->type == Token_Symbol_BitShiftLeft ||
                    t->type == Token_Symbol_BitShiftRight ||
                    t->type == Token_Symbol_And ||
                    t->type == Token_Symbol_Or ||
                    t->type == Token_Symbol_Tilde ||
                    t->type == Token_Symbol_EqualTo ||
                    t->type == Token_Symbol_NotEqualTo ||
                    t->type == Token_Symbol_LessThan ||
                    t->type == Token_Symbol_LessEqualTo ||
                    t->type == Token_Symbol_GreaterThan ||
                    t->type == Token_Symbol_GreaterEqualTo
                )
            )) do_loop = false;
        }
        if (do_loop) switch (t->type) {
            case Token_Symbol_ParenOpen:
                paren_stack[paren_stack_ptr++] = '(';
                break;
            case Token_Symbol_SquareBraceOpen:
                paren_stack[paren_stack_ptr++] = '[';
                break;
            case Token_Symbol_CurlyBraceOpen:
                if (!can_end) orig_paren_stack_ptr = paren_stack_ptr;
                paren_stack[paren_stack_ptr++] = '{';
                can_end = false;
                break;
            case Token_Symbol_ParenClose:
                validate_bracket_close(')')
                if (end_state == BS_EndState_Paren && paren_stack_ptr == 0) do_loop = false;
                break;
            case Token_Symbol_SquareBraceClose:
                validate_bracket_close(']')
                if (end_state == BS_EndState_Bracket && paren_stack_ptr == 0) do_loop = false;
                break;
            case Token_Symbol_CurlyBraceClose:
                validate_bracket_close('}')
                if (orig_paren_stack_ptr == paren_stack_ptr) can_end = true;
                if (end_state == BS_EndState_Brace && paren_stack_ptr == 0) do_loop = false;
                break;
            case Token_Symbol_Semicolon:
                if (can_end) {
                    if (paren_stack_ptr != 0) bracket_err()
                    else if (end_state == BS_EndState_Semicolon) {
                        do_loop = true;
                        break;
                    }
                }
                BS_AppendError(BS_Error_UnexpectedSemicolon, t->row, t->col);
                return NULL;
            case Token_Symbol_RangeArrow:
                if (paren_stack_ptr == 0 && end_state == BS_EndState_Arrow) break;
                break;
            case Token_Symbol_ReverseArrow:
                if (paren_stack_ptr == 0 && end_state == BS_EndState_ReverseArrow) break;
                break;
            default: break;
        }
        prev_t = t;
        t = t->next;
        size++;
    }
    t = *tokens;
    struct Token* fetched = calloc(sizeof(struct Token), size);
    for (int i = 0; i < size; i++) {
        memcpy(fetched + i, t, sizeof(struct Token));
        fetched[i].next = NULL;
        fetched[i].prev = NULL;
        if (i > 0) {
            fetched[i - 1].next = fetched + i;
            fetched[i].prev = fetched + i - 1;
        }
        t = t->next;
    }
    *tokens = t;
    return fetched;
}

bool evaluate(struct Token** tokens, BS_Context* context, struct Variable* out, enum BS_EndState end_state) {
    struct Token* t = fetch_tokens(tokens, BS_EndState_Semicolon);
    struct Token* inner_t;
    if (!t) return false;
    int cmd = 0;
    int ptr = 0;
    int tokenptr = 0;
    struct Variable parameters[256];
    int num_parameters = 0;
    while (true) {
        bool failed = false;
        switch (ruleset[cmd][ptr++]) {
            case 0: { // RUN
                if (t[tokenptr].next) {
                    failed = true;
                    break;
                }
                *out = ((BS_EvaluatorFunc)ruleset[cmd][ptr++])(context, parameters, num_parameters);
                return true;
            } break;
            case 1: { // SPECIFIC
                if (ruleset[cmd][ptr++] != t[tokenptr++].type) failed = true;
            } break;
            case 2: { // TYPE
                BS_VarType vartype;
                if (get_type(curr_token, &vartype)) push(u8, vartype);
                else failed = true;
                update_tokenptr();
            } break;
            case 4: { // EXPR
                enum BS_EndState next_end_state = end_state;
                if (ruleset[cmd][ptr + 1] == 1) { // SPECIFIC
                    switch (ruleset[cmd][ptr + 2]) {
                        case Token_Symbol_ParenOpen: next_end_state = BS_EndState_Paren; break;
                        case Token_Symbol_SquareBraceOpen: next_end_state = BS_EndState_Bracket; break;
                        case Token_Symbol_CurlyBraceOpen: next_end_state = BS_EndState_Brace; break;
                        case Token_Symbol_RangeArrow: next_end_state = BS_EndState_Arrow; break;
                        case Token_Symbol_ReverseArrow: next_end_state = BS_EndState_ReverseArrow; break;
                    }
                }
                if (ruleset[cmd][ptr + 1] == 7) { // CODEBLOCK
                    next_end_state = BS_EndState_CodeBlock;
                }
                if (ruleset[cmd][ptr + 1] == 9) { // OPERATOR
                    next_end_state = BS_EndState_Operator;
                }
                struct Variable retval;
                bool is_empty = !evaluate(curr_token, context, &retval, next_end_state);
                update_tokenptr();
                if (BS_HasError() || is_empty) return false;
                pushvar(retval);
            } break;
            case 5: { // EXPRLIST
                if (t[tokenptr].type == Token_Symbol_ParenClose) break;
                while (true) {
                    struct Variable retval;
                    bool is_empty = !evaluate(curr_token, context, &retval, BS_EndState_List);
                    update_tokenptr();
                    if (BS_HasError() || is_empty) return false;
                    pushvar(retval);
                    if (t[tokenptr - 1].type == Token_Symbol_ParenClose) break;
                }
            } break;
            case 6: { // ARGLIST
            } break;
            case 7: { // CODEBLOCK
                struct Token* token_list;
                enum TokenType type = t[tokenptr++].type;
                if (type == Token_Symbol_Arrow) token_list = fetch_tokens(curr_token, BS_EndState_Semicolon);
                else if (type == Token_Symbol_CurlyBraceOpen) token_list = fetch_tokens(curr_token, BS_EndState_Brace);
                else {
                    failed = true;
                    break;
                }
                update_tokenptr();
                push(token, token_list);
            } break;
            case 8: { // MODIFIER
                int index = get_token_index(t[tokenptr++].type, modification_tokens);
                if (index == arrsize(modification_tokens)) failed = true;
                else push(s32, index);
            } break;
            case 9: { // OPERATOR
                int index = get_token_index(t[tokenptr++].type, operation_tokens);
                if (index == arrsize(operation_tokens)) failed = true;
                else push(s32, index);
            } break;
        }
        if (failed) {
            cmd++;
            ptr = tokenptr = num_parameters = 0;
        }
        else ptr++;
    }
}

struct Variable BS_Execute(struct Token* tokens, BS_Context* context) {
    struct Variable retval = (struct Variable){};
    while (1) {
        printf("executing at offset (%d;%d)\n", tokens->row, tokens->col);
        if (!evaluate(&tokens, context, &retval, BS_EndState_Semicolon)) break;
    }
    return retval;
}