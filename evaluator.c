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
#define VARARGS SPECIFIC(Token_Symbol_Vararg)
#define RANGE SPECIFIC(Token_Symbol_Range)
#define PARENOPEN SPECIFIC(Token_Symbol_ParenOpen)
#define PARENCLOSE SPECIFIC(Token_Symbol_ParenClose)
#define BRACKETOPEN SPECIFIC(Token_Symbol_SquareBraceOpen)
#define BRACKETCLOSE SPECIFIC(Token_Symbol_SquareBraceClose)
#define BRACEOPEN SPECIFIC(Token_Symbol_CurlyBraceOpen)
#define BRACECLOSE SPECIFIC(Token_Symbol_CurlyBraceClose)
#define STRUCT SPECIFIC(Token_Keyword_struct)
#define RETURN SPECIFIC(Token_Keyword_return)
#define RETURNS SPECIFIC(Token_Keyword_returns)
#define IF SPECIFIC(Token_Keyword_if)
#define ELIF SPECIFIC(Token_Keyword_elif)
#define ELSE SPECIFIC(Token_Keyword_else)
#define WHILE SPECIFIC(Token_Keyword_while)
#define FOR SPECIFIC(Token_Keyword_for)
#define IN SPECIFIC(Token_Keyword_in)
#define CONTINUE SPECIFIC(Token_Keyword_continue)
#define BREAK SPECIFIC(Token_Keyword_break)
#define LEND SPECIFIC(Token_Keyword_lend)
#define BORROW SPECIFIC(Token_Keyword_borrow)
#define FROM SPECIFIC(Token_Keyword_from)
#define CALLED SPECIFIC(Token_Keyword_called)
#define TYPE 2,
#define IDENT 3,
#define EXPR 4,
#define EXPRLIST 5,
#define ARGLIST 6,
#define CODEBLOCK 7,
#define MODIFIER 8,
#define OPERATOR 9,
#define NUMBER 10,
#define STRLIT 11,

typedef BS_Variable(*BS_EvaluatorFunc)(struct _BS_Context* context, BS_Variable* params, int num_params);

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

#include "implementation.h"

const uintptr_t ruleset[][24] = {
    {
#include "evaluator_ruleset.h"
    0, (uintptr_t)bseval_incorrect_syntax }
};

static bool get_type(struct Token* tokens, int* tokenptr, BS_VarType* vartype) {
    switch (tokens[*tokenptr].type) {
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
    while (tokens[++(*tokenptr)].type == Token_Symbol_Multiply) *vartype = BS_ptr(*vartype);
    return true;
}

#define push(type, val) parameters[num_parameters++].value.type = val
#define pushvar(val) parameters[num_parameters++] = val
#define token func
#define arrsize(arr) (sizeof(arr) / sizeof(*(arr)))
#define get_token_index(tkn, arr) _get_token_index(tkn, arr, arrsize(arr))

static int _get_token_index(enum TokenType type, enum TokenType* arr, int num) {
    int i = 0;
    for (; i < num; i++) {
        if (type == arr[i]) break;
    }
    return i;
}

#define bracket_err() {                                          \
    BS_AppendError(BS_Error_InvalidBracketStack, t->row, t->col); \
    return 0;                                                      \
}

#define validate_bracket_close(symbol)                                     \
    if (paren_stack_ptr == 0 || paren_stack[--paren_stack_ptr] != symbol) { \
        BS_AppendError(BS_Error_InvalidBracketStack, t->row, t->col);        \
        return 0;                                                             \
    }                                                                          \

int BS_FetchTokens(struct Token* tokens, enum BS_EndState end_state) {
    struct Token* t = tokens;
    struct Token* prev_t = NULL;
    if (!t) return -1;
    char paren_stack[1024];
    int paren_stack_ptr = 0;
    int orig_paren_stack_ptr = 0;
    bool can_end = true;
    bool do_loop = true;
    int size = 0;
    while (do_loop) {
        if (!t) {
            BS_AppendError(BS_Error_UnexpectedEOE, prev_t->row, prev_t->col);
            return -1;
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
            ) || (
                end_state == BS_EndState_Range && (
                    t->type == Token_Symbol_Range
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
                if (end_state == BS_EndState_Paren && paren_stack_ptr == 0) do_loop = false;
                else validate_bracket_close('(')
                break;
            case Token_Symbol_SquareBraceClose:
                if (end_state == BS_EndState_Bracket && paren_stack_ptr == 0) do_loop = false;
                else validate_bracket_close('[')
                break;
            case Token_Symbol_CurlyBraceClose:
                if ((end_state == BS_EndState_Brace || end_state == BS_EndState_Semicolon) && paren_stack_ptr == 1 && !can_end) do_loop = false;
                else {
                    validate_bracket_close('{')
                    if (orig_paren_stack_ptr == paren_stack_ptr) can_end = true;   
                }
                break;
            case Token_Symbol_Semicolon:
                if (can_end) {
                    if (paren_stack_ptr != 0) bracket_err()
                    else if (end_state == BS_EndState_Semicolon) {
                        do_loop = false;
                        break;
                    }
                    BS_AppendError(BS_Error_UnexpectedSemicolon, t->row, t->col);
                    return -1;
                }
                break;
            default: break;
        }
        prev_t = t;
        t = t->next;
        size++;
    }
    return size - 1;
}

bool BS_Evaluate(struct Token* tokens, int* tokenptr, BS_Context* context, BS_Variable* out, enum BS_EndState end_state) {
    int num_tokens = BS_FetchTokens(tokens + *tokenptr, end_state);
    if (num_tokens == -1) return false;
    if (num_tokens ==  0) return true;
    int max_tokenptr = *tokenptr + num_tokens;
    struct Token* inner_t;
    int cmd = 0;
    int ptr = 0;
    BS_Variable parameters[256];
    int num_parameters = 0;
    while (true) {
        bool failed = false;
        switch (ruleset[cmd][ptr++]) {
            case 0: { // RUN
                if (*tokenptr != max_tokenptr && ptr != 1) {
                    failed = true;
                    break;
                }
                *out = ((BS_EvaluatorFunc)ruleset[cmd][ptr++])((struct _BS_Context*)context, parameters, num_parameters);
                return true;
            } break;
            case 1: { // SPECIFIC
                if (ruleset[cmd][ptr++] != tokens[(*tokenptr)++].type) failed = true;
            } break;
            case 2: { // TYPE
                BS_VarType vartype;
                if (get_type(tokens, tokenptr, &vartype)) push(u8, vartype);
                else failed = true;
            } break;
            case 3: { // IDENT
                if (Token_Identifier != tokens[*tokenptr].type) failed = true;
                else push(string, tokens[*tokenptr].value.string);
                (*tokenptr)++;
            } break;
            case 4: { // EXPR
                enum BS_EndState next_end_state = end_state;
                if (ruleset[cmd][ptr] == 1) { // SPECIFIC
                    switch (ruleset[cmd][ptr + 1]) {
                        case Token_Symbol_ParenClose: next_end_state = BS_EndState_Paren; break;
                        case Token_Symbol_SquareBraceClose: next_end_state = BS_EndState_Bracket; break;
                        case Token_Symbol_CurlyBraceClose: next_end_state = BS_EndState_Brace; break;
                        case Token_Symbol_Range: next_end_state = BS_EndState_Range; break;
                    }
                }
                if (ruleset[cmd][ptr] == 7) { // CODEBLOCK
                    next_end_state = BS_EndState_CodeBlock;
                }
                if (ruleset[cmd][ptr] == 9) { // OPERATOR
                    next_end_state = BS_EndState_Operator;
                }
                BS_Variable retval;
                bool is_empty = !BS_Evaluate(tokens, tokenptr, context, &retval, next_end_state);
                if (BS_HasError() || is_empty) return false;
                pushvar(retval);
            } break;
            case 5: { // EXPRLIST
                if (tokens[*tokenptr].type == Token_Symbol_ParenClose) {
                    (*tokenptr)++;
                    break;
                }
                while (true) {
                    BS_Variable retval;
                    bool is_empty = !BS_Evaluate(tokens, tokenptr, context, &retval, BS_EndState_List);
                    if (BS_HasError() || is_empty) return false;
                    pushvar(retval);
                    if (tokens[(*tokenptr) - 1].type == Token_Symbol_ParenClose) break;
                }
            } break;
            case 6: { // ARGLIST
                if (tokens[*tokenptr].type == Token_Symbol_ParenClose) {
                    (*tokenptr)++;
                    break;
                }
                while (true) {
                    BS_VarType vartype;
                    if (get_type(tokens, tokenptr, &vartype)) push(u8, vartype);
                    else {
                        failed = true;
                        break;
                    }

                    if (tokens[*tokenptr].type == Token_Identifier) push(string, tokens[*tokenptr].value.string);
                    else {
                        failed = true;
                        break;
                    }
                    (*tokenptr) += 2;

                    if (tokens[*tokenptr - 1].type == Token_Symbol_ParenClose) break;
                    else if (tokens[*tokenptr - 1].type == Token_Symbol_Comma) continue;

                    failed = true;
                    break;
                }
            } break;
            case 7: { // CODEBLOCK
                struct Token* token_list;
                enum TokenType type = tokens[*tokenptr].type;
                int num_tokens = 0;
                if (type == Token_Symbol_Arrow) num_tokens = BS_FetchTokens(tokens + *tokenptr, BS_EndState_Semicolon);
                else if (type == Token_Symbol_CurlyBraceOpen) num_tokens = BS_FetchTokens(tokens + *tokenptr, BS_EndState_Brace);
                else {
                    failed = true;
                    break;
                }
                *tokenptr += num_tokens;
                push(token, token_list);
            } break;
            case 8: { // MODIFIER
                int index = get_token_index(tokens[(*tokenptr)++].type, modification_tokens);
                if (index == arrsize(modification_tokens)) failed = true;
                else push(s32, index);
            } break;
            case 9: { // OPERATOR
                int index = get_token_index(tokens[(*tokenptr)++].type, operation_tokens);
                if (index == arrsize(operation_tokens)) failed = true;
                else push(s32, index);
            } break;
            case 10: { // NUMBER
                BS_Variable variable;
                variable.vartype = BS_void;
                switch (tokens[*tokenptr].type) {
                    case Token_NumberLiteral: {
                        variable.vartype = BS_f64;
                        variable.value.f64 = tokens[*tokenptr].value.number;
                    } break;
                    case Token_IntegerLiteral: {
                        variable.vartype = BS_s64;
                        variable.value.s64 = tokens[*tokenptr].value.integer;
                    } break;
                    case Token_CharacterLiteral: {
                        variable.vartype = BS_s8;
                        variable.value.s8 = tokens[*tokenptr].value.integer;
                    } break;
                    case Token_Keyword_true: {
                        variable.vartype = BS_bool;
                        variable.value.boolean = 1;
                    } break;
                    case Token_Keyword_false: {
                        variable.vartype = BS_bool;
                        variable.value.boolean = 0;
                    } break;
                    default:
                        failed = true;
                        break;
                }
                if (variable.vartype == BS_void) break;
                (*tokenptr)++;
                pushvar(variable);
            } break;
            case 11: { // STRLIT
                if (tokens[*tokenptr].type != Token_StringLiteral) failed = true;
                else push(string, tokens[(*tokenptr)++].value.string);
            } break;
        }
        if (failed) {
            cmd++;
            ptr = num_parameters = 0;
            *tokenptr = max_tokenptr - num_tokens;
        }
    }
}

BS_Variable BS_Execute(struct Token* tokens, BS_Context* context) {
    BS_Variable retval = (BS_Variable){};
    int tokenptr = 0;
    while (1) {
        if (!tokens[tokenptr].next) {
            printf("end of script\n");
            break;
        }
        printf("next expression (%d:%d)\n", tokens[tokenptr].row, tokens[tokenptr].col);
        if (!BS_Evaluate(tokens, &tokenptr, context, &retval, BS_EndState_Semicolon)) break;
        tokenptr++;
    }
    return retval;
}