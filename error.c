#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ERROR(code, text) text,
const char* error_strings[] = {
#include "errors.h"
};
#undef ERROR

struct Error {
    int code;
    int row, col;
    struct Error* next;
};

struct Error* errors;

void BS_AppendError(int code, int row, int col) {
    if (!errors) errors = calloc(sizeof(struct Error), 1);
    struct Error* curr = errors;
    while (curr->next) curr = curr->next;
    curr->next = calloc(sizeof(struct Error), 1);
    curr->code = code;
    curr->row  = row;
    curr->col  = col;
}

bool BS_HasError() {
    if (!errors) return false;
    if (!errors->next) return false;
    return true;
}

int BS_GetError(int* row, int* col) {
    int r = 0, c = 0;
    int code = 0;
    if (errors && errors->next) {
        r    = errors->row;
        c    = errors->col;
        code = errors->code;
        struct Error* next = errors->next;
        free(errors);
        errors = next;
    }
    if (row) *row = r;
    if (col) *col = c;
    return code;
}

const char* BS_GetErrorString(int error) {
    if (error < 0 || error >= sizeof(error_strings) / sizeof(*error_strings)) return "Invalid error code";
    return error_strings[error];
}