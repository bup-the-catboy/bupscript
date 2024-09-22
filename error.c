#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

const char* error_strings[] = {
    "No error",
    "Invalid character",
    "Unexpected End of Line",
    "Unexpected End of File",
};

struct Error {
    int code;
    int row, col;
    struct Error* next;
};

struct Error* errors;

void BS_AppendError(int code, int row, int col) {
    if (!errors) errors = memset(malloc(sizeof(struct Error)), 0, sizeof(struct Error));
    struct Error* curr = errors;
    while (curr->next) curr = curr->next;
    curr->next = memset(malloc(sizeof(struct Error)), 0, sizeof(struct Error));
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