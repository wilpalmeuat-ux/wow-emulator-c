/* ╔══════════════════════════════════════════════════════════════╗
   ║  WSS Bytecode Chunk — compile target + stack frame layout     ║
   ╚══════════════════════════════════════════════════════════════╝ */
#ifndef WSS_CHUNK_H
#define WSS_CHUNK_H

#include <stdint.h>

typedef enum {
    OP_NOP,
    OP_LOAD_NULL, OP_LOAD_BOOL, OP_LOAD_INT, OP_LOAD_DBL, OP_LOAD_STR,
    OP_LOAD_SELF,
    OP_STORE_GLOBAL, OP_LOAD_GLOBAL, OP_DELETE_GLOBAL,
    OP_STORE_LOCAL, OP_LOAD_LOCAL,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_NEG, OP_NOT,
    OP_CMP_EQ, OP_CMP_NE, OP_CMP_LT, OP_CMP_GT, OP_CMP_LE, OP_CMP_GE,
    OP_JUMP_IF_FALSE, OP_JUMP_IF_TRUE, OP_JUMP,
    OP_CALL, OP_RETURN,
    OP_PRINT, OP_INPUT,
    OP_NEW_OBJECT, OP_OBJ_SET, OP_OBJ_GET,
    OP_ARRAY_NEW, OP_ARRAY_LEN, OP_ARRAY_GET, OP_ARRAY_SET,
    OP_ITER_START, OP_ITER_NEXT, OP_ITER_END,
    OP_EXIT,
} OpCode;

typedef struct {
    uint8_t*   code;
    int        count;
    int        capacity;
    int*       lines;
} WssChunk;

void WssChunk_Init(WssChunk* c);
void WssChunk_Write(WssChunk* c, uint8_t byte, int line);
void WssChunk_Free(WssChunk* c);

#endif