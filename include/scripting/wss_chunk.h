/* wss_chunk.h — Bytecode chunk for WSS VM */
#ifndef WSS_CHUNK_H
#define WSS_CHUNK_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "wss_value.h"
#include "wss_objectstore.h"

typedef enum {
    OP_LOAD_NULL,
    OP_LOAD_BOOL,
    OP_LOAD_INT,
    OP_LOAD_DBL,
    OP_LOAD_STR,
    OP_LOAD_GLOBAL,
    OP_STORE_GLOBAL,
    OP_LOAD_LOCAL,
    OP_STORE_LOCAL,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG,
    OP_CMP_EQ, OP_CMP_NE, OP_CMP_LT, OP_CMP_GT, OP_CMP_LTE, OP_CMP_GTE,
    OP_AND, OP_OR, OP_NOT,
    OP_JUMP_IF_FALSE, OP_JUMP_IF_TRUE, OP_JUMP, OP_LOOP,
    OP_POP,
    OP_CALL, OP_RETURN,
    OP_EXIT,
    OP_NEW_OBJECT,
    OP_NEW_ARRAY,
    OP_INDEX_GET, OP_INDEX_SET,
    OP_PRINT,
    OP_HALT,
} OpCode;

typedef struct {
    uint8_t* code;
    int count;
    int cap;
    int* lines;
    WssValue* constants;
    int num_consts;
    int cap_consts;
    int num_locals;
    int max_stack;
    char name[64];
} WssChunk;

static inline void WssChunk_Init(WssChunk* c) {
    memset(c, 0, sizeof(*c));
    c->cap = 64;
    c->code = malloc(c->cap);
    c->lines = malloc(c->cap * sizeof(int));
    c->cap_consts = 16;
    c->constants = malloc(c->cap_consts * sizeof(WssValue));
}

static inline void WssChunk_Write(WssChunk* c, uint8_t byte, int line) {
    if (c->count >= c->cap) {
        c->cap *= 2;
        c->code = realloc(c->code, c->cap);
        c->lines = realloc(c->lines, c->cap * sizeof(int));
    }
    c->code[c->count] = byte;
    c->lines[c->count] = line;
    c->count++;
}

static inline int WssChunk_AddConstant(WssChunk* c, WssValue v) {
    if (c->num_consts >= c->cap_consts) {
        c->cap_consts *= 2;
        c->constants = realloc(c->constants, c->cap_consts * sizeof(WssValue));
    }
    c->constants[c->num_consts] = v;
    return c->num_consts++;
}

static inline void WssChunk_Free(WssChunk* c) {
    free(c->code);
    free(c->lines);
    for (int i = 0; i < c->num_consts; i++)
        WssValue_Delete(&c->constants[i]);
    free(c->constants);
    memset(c, 0, sizeof(*c));
}

#endif /* WSS_CHUNK_H */
