#include "shared/bytebuffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ByteBuffer* bb_create(void) {
    ByteBuffer* bb = calloc(1, sizeof(ByteBuffer));
    bb->cap = 4096;
    bb->data = malloc(bb->cap);
    return bb;
}

void bb_free(ByteBuffer* bb) { free(bb->data); free(bb); }
void bb_reset(ByteBuffer* bb) { bb->rpos = bb->wpos = 0; }

static void grow(ByteBuffer* bb, size_t need) {
    while (bb->wpos + need > bb->cap) {
        bb->cap *= 2;
        bb->data = realloc(bb->data, bb->cap);
    }
}

void bb_write(ByteBuffer* bb, const void* d, size_t len) {
    grow(bb, len);
    memcpy(bb->data + bb->wpos, d, len);
    bb->wpos += len;
}

void bb_read(ByteBuffer* bb, void* d, size_t len) {
    memcpy(d, bb->data + bb->rpos, len);
    bb->rpos += len;
}

void bb_appendByteBuffer(ByteBuffer* dst, ByteBuffer* src) {
    bb_write(dst, src->data + src->rpos, src->wpos - src->rpos);
}

void bb_putUInt8(ByteBuffer* bb, uint8_t v) { bb_write(bb, &v, sizeof(uint8_t)); }
void bb_putUInt16(ByteBuffer* bb, uint16_t v) { uint16_t v2 = v; bb_write(bb, &v2, sizeof(uint16_t)); }
void bb_putUInt32(ByteBuffer* bb, uint32_t v) { uint32_t v2 = v; bb_write(bb, &v2, sizeof(uint32_t)); }
void bb_putUInt64(ByteBuffer* bb, uint64_t v) { uint64_t v2 = v; bb_write(bb, &v2, sizeof(uint64_t)); }
void bb_putInt8(ByteBuffer* bb, int8_t v) { bb_write(bb, &v, sizeof(int8_t)); }
void bb_putInt16(ByteBuffer* bb, int16_t v) { int16_t v2 = v; bb_write(bb, &v2, sizeof(int16_t)); }
void bb_putInt32(ByteBuffer* bb, int32_t v) { int32_t v2 = v; bb_write(bb, &v2, sizeof(int32_t)); }
void bb_putInt64(ByteBuffer* bb, int64_t v) { int64_t v2 = v; bb_write(bb, &v2, sizeof(int64_t)); }
void bb_putFloat(ByteBuffer* bb, float v) { float v2 = v; bb_write(bb, &v2, sizeof(float)); }
void bb_putDouble(ByteBuffer* bb, double v) { double v2 = v; bb_write(bb, &v2, sizeof(double)); }

void bb_putString(ByteBuffer* bb, const char* s) {
    size_t len = strlen(s) + 1;
    bb_putUInt16(bb, (uint16_t)len);
    bb_write(bb, s, len);
}

uint8_t  bb_readUInt8(ByteBuffer* bb) { uint8_t v; bb_read(bb, &v, sizeof(uint8_t)); return v; }
uint16_t bb_readUInt16(ByteBuffer* bb) { uint16_t v; bb_read(bb, &v, sizeof(uint16_t)); return v; }
uint32_t bb_readUInt32(ByteBuffer* bb) { uint32_t v; bb_read(bb, &v, sizeof(uint32_t)); return v; }
uint64_t bb_readUInt64(ByteBuffer* bb) { uint64_t v; bb_read(bb, &v, sizeof(uint64_t)); return v; }
int8_t   bb_readInt8(ByteBuffer* bb) { int8_t v; bb_read(bb, &v, sizeof(int8_t)); return v; }
int16_t  bb_readInt16(ByteBuffer* bb) { int16_t v; bb_read(bb, &v, sizeof(int16_t)); return v; }
int32_t  bb_readInt32(ByteBuffer* bb) { int32_t v; bb_read(bb, &v, sizeof(int32_t)); return v; }
int64_t  bb_readInt64(ByteBuffer* bb) { int64_t v; bb_read(bb, &v, sizeof(int64_t)); return v; }
float    bb_readFloat(ByteBuffer* bb) { float v; bb_read(bb, &v, sizeof(float)); return v; }
double   bb_readDouble(ByteBuffer* bb) { double v; bb_read(bb, &v, sizeof(double)); return v; }

char* bb_readString(ByteBuffer* bb) {
    uint16_t len = bb_readUInt16(bb);
    char* s = malloc(len);
    memcpy(s, bb->data + bb->rpos, len);
    bb->rpos += len;
    return s;
}
