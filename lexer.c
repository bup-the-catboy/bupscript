#include "bupscript_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct Token* append_token(struct Token* head, enum TokenType type, union TokenValue value, int row, int col) {
    struct Token* next = calloc(sizeof(struct Token), 1);
    head->next  = next;
    head->type  = type;
    head->value = value;
    head->row   = row;
    head->col   = col;
    return head->next; 
}

struct Token* append_alphanumeric_token(struct Token* head, char* data, int row, int col) {
    int base = 10;
    int offset = 0;
    if      (data[0] == '0' && data[1] == 'x') { base = 16; offset = 2; }
    else if (data[0] == '0' && data[1] == 'b') { base =  2; offset = 2; }
    else if (data[0] == '0' && data[1] == 'o') { base =  8; offset = 2; }
    else if (data[0] == '0')                   { base =  8; offset = 1; }
    char* endptr;
    double number = strtoll(data + offset, &endptr, base);
    if (*endptr != 0) number = strtod(data, &endptr);
    if (*endptr == 0) {
        free(data);
        return append_token(head, Token_NumberLiteral, (union TokenValue){ .number = number }, row, col);
    }
    return append_token(head, Token_Identifier, (union TokenValue){ .string = data }, row, col);
}

int parse_symbol_token(const char* script, struct Token* head, int row, int col) {
    int ptr = 0;
    int num_tokens = sizeof(token_values) / sizeof(*token_values);
    bool valid[num_tokens];
    memset(valid, true, num_tokens);
    enum TokenType token = Token_None;
    while (1) {
        for (int i = 0; i < num_tokens; i++) {
            if (!valid[i]) continue;
            if (!token_values[i]) valid[i] = false;
            else if (token_values[i][ptr] == 0) {
                token = i;
                valid[i] = false;
            }
            else if (token_values[i][ptr] != script[ptr]) valid[i] = false;
        }
        ptr++;
        bool do_continue = false;
        for (int i = 0; i < num_tokens; i++) {
            if (valid[i]) {
                do_continue = true;
                break;
            }
        }
        if (!do_continue) break;
    }
    if (token == Token_None) return 0;
    head->row = row;
    head->col = col;
    head->next = calloc(sizeof(struct Token), 1);
    head->type = token;
    return strlen(token_values[token]);
}

bool is_alphanumeric(char c) {
    return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

int get_identifier_length(const char* script) {
    int len = 0;
    char c;
    while ((c = script[len])) {
        if (!is_alphanumeric(c)) break;
        len++;
    }
    return len;
}

struct Token* token_array(struct Token* tokens) {
    int num_tokens = 0;
    struct Token* curr = tokens;
    while (curr) {
        num_tokens++;
        curr = curr->next;
    }
    struct Token* arr = calloc(sizeof(struct Token), num_tokens);
    curr = tokens;
    for (int i = 0; i < num_tokens; i++) {
        struct Token* next = curr->next;
        memcpy(&arr[i], curr, sizeof(struct Token));
        arr[i].next = NULL;
        if (i > 0) arr[i - 1].next = arr + i;
        free(curr);
        curr = next;
    }
    return arr;
}

void print_token_list(struct Token* token) {
    struct Token* curr = token;
    while (curr->next) {
        if (curr->type == Token_Identifier) printf("(%2d:%2d %-28s) %s\n", curr->row, curr->col, token_names[curr->type], curr->value.string);
        else if (curr->type == Token_StringLiteral) printf("(%2d:%2d %-28s) \"%s\"\n", curr->row, curr->col, token_names[curr->type], curr->value.string);
        else if (curr->type == Token_CharacterLiteral) printf("(%2d:%2d %-28s) '%c'", curr->row, curr->col, token_names[curr->type], (char)curr->value.integer);
        else if (curr->type == Token_NumberLiteral) printf("(%2d:%2d %-28s) %g\n", curr->row, curr->col, token_names[curr->type], curr->value.number);
        else printf("(%2d:%2d %-28s) %s\n", curr->row, curr->col, token_names[curr->type], token_values[curr->type]);
        curr = curr->next;
    }
}

struct Token* BS_Lex(const char* script) {
    struct Token* list = calloc(sizeof(struct Token), 1);
    struct Token* head = list;
    int ptr = 0;
    char c;
    int row = 1, col = 0;
    while ((c = script[ptr])) {
        col++;
        if (c == '\n') {
            col = 0;
            row++;
            ptr++;
            continue;
        }
        if (c == ' ' || c == '\t') {
            ptr++;
            continue;
        }
        if ((c < 32 || c > 126) && c != '\n' && c != '\t') {
            ptr++;
            BS_AppendError(BS_Error_InvalidCharacter, row, col);
            continue;
        }
        int len = 0;
        if ((len = parse_symbol_token(script + ptr, head, row, col))) {
            ptr += len;
            col += len - 1;
            head = head->next;
            continue;
        }
        if (is_alphanumeric(c)) {
            int length = get_identifier_length(script + ptr);
            char* identifier = malloc(length + 1);
            int p = 0;
            while (1) {
                c = script[ptr + p];
                if (!is_alphanumeric(c)) break;
                identifier[p] = c;
                p++;
            }
            identifier[p] = 0;
            head = append_alphanumeric_token(head, identifier, row, col);
            ptr += p - 1;
            col += p;
        }
        else if (c == '"') {
            int length = 0;
            int prev_ptr = ptr;
            int utf8_digit = 0;
            bool escaped = false;
            bool invalid = false;
            while (1) {
                c = script[++ptr];
                if (c == '\n') {
                    BS_AppendError(BS_Error_UnexpectedEOL, row, col);
                    invalid = true;
                    break;
                }
                if (c == 0) {
                    BS_AppendError(BS_Error_UnexpectedEOF, row, col);
                    invalid = true;
                    break;
                }
                if (c < 32) {
                    BS_AppendError(BS_Error_InvalidCharacter, row, col);
                    invalid = true;
                    break;
                }
                if (utf8_digit) utf8_digit--;
                else if (escaped) {
                    if (c == 'x') utf8_digit = 8;
                    length++;
                    escaped = false;
                }
                else if (c == '"') break;
                else if (c == '\\') escaped = true;
                else length++;
            }
            if (invalid) {
                ptr++;
                continue;
            }
            char* literal = malloc(length + 1);
            char* lithead = literal;
            ptr = prev_ptr;
            while (1) {
                c = script[++ptr];
                if (utf8_digit) {
                    lithead--;
                    int value = 0;
                    if (c >= '0' && c <= '9') value = c - '0';
                    if (c >= 'A' && c <= 'F') value = c - 'A' + 10;
                    if (c >= 'a' && c <= 'f') value = c - 'a' + 10;
                    *lithead += value * pow(16, utf8_digit - 1);
                    lithead++;
                    utf8_digit--;
                }
                if (escaped) {
                    switch (c) {
                        case 'a':  c = '\a'; break;
                        case 'b':  c = '\b'; break;
                        case 'e':  c = '\e'; break;
                        case 'f':  c = '\f'; break;
                        case 'n':  c = '\n'; break;
                        case 'r':  c = '\r'; break;
                        case 't':  c = '\t'; break;
                        case 'v':  c = '\v'; break;
                        case '\\': c = '\\'; break;
                        case '\'': c = '\''; break;
                        case '"':  c = '\"'; break;
                        case 'x':
                            c = 0;
                            utf8_digit = 2;
                            break;
                    }
                    *lithead = c;
                    lithead++;
                    escaped = false;
                    continue;
                }
                if (c == '"') break;
                if (c == '\\') {
                    escaped = true;
                    continue;
                }
                *lithead = c;
                lithead++;
            }
            *lithead = 0;
            head = append_token(head, Token_StringLiteral, (union TokenValue){ .string = literal }, row, col);
            col += ptr - prev_ptr;
        }
        ptr++;
    }
    list = token_array(list);
    print_token_list(list);
    return list;
}