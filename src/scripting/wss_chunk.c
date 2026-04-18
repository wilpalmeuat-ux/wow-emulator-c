#include <scripting/wss_chunk.h>
#include <stdlib.h>
#include <string.h>

void WssChunk_Init(WssChunk* c) {
    memset(c, 0, sizeof(*c));
}
void WssChunk_Free(WssChunk* c) {
    free(c->code);
    free(c->lines);
    WssChunk_Init(c);
}
void WssChunk_Write(WssChunk* c, uint8_t byte, int line) {
    if (c->capacity == 0) c->capacity = 64;
    if (c->count >= c->capacity) {
        c->capacity *= 2;
        c->code = realloc(c->code, (size_t)c->capacity);
        c->lines = realloc(c->lines, (size_t)c->capacity * sizeof(int));
    }
    c->code[c->count] = byte;
    c->lines[c->count] = line;
    c->count++;
}