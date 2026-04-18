#ifndef WOW_BYTEBUFFER_H
#define WOW_BYTEBUFFER_H
#include <stdint.h>
#include <stdlib.h>
typedef struct { uint8_t* data; size_t rpos, wpos, cap; } ByteBuffer;
ByteBuffer* bb_create(void);
void bb_free(ByteBuffer*);
void bb_reset(ByteBuffer*);
void bb_write(ByteBuffer*, const void* d, size_t len);
void bb_read(ByteBuffer*, void* d, size_t len);
void bb_putUInt8(ByteBuffer*, uint8_t v);
void bb_putUInt16(ByteBuffer*, uint16_t v);
void bb_putUInt32(ByteBuffer*, uint32_t v);
void bb_putUInt64(ByteBuffer*, uint64_t v);
void bb_putInt8(ByteBuffer*, int8_t v);
void bb_putInt16(ByteBuffer*, int16_t v);
void bb_putInt32(ByteBuffer*, int32_t v);
void bb_putInt64(ByteBuffer*, int64_t v);
void bb_putFloat(ByteBuffer*, float v);
void bb_putDouble(ByteBuffer*, double v);
void bb_putString(ByteBuffer*, const char* s);
uint8_t  bb_readUInt8(ByteBuffer*);
uint16_t bb_readUInt16(ByteBuffer*);
uint32_t bb_readUInt32(ByteBuffer*);
uint64_t bb_readUInt64(ByteBuffer*);
int8_t   bb_readInt8(ByteBuffer*);
int16_t  bb_readInt16(ByteBuffer*);
int32_t  bb_readInt32(ByteBuffer*);
int64_t  bb_readInt64(ByteBuffer*);
float    bb_readFloat(ByteBuffer*);
double   bb_readDouble(ByteBuffer*);
char*    bb_readString(ByteBuffer*);
void     bb_appendByteBuffer(ByteBuffer* dst, ByteBuffer* src);
#endif
