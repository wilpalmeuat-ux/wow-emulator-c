#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include "Types.h"

typedef struct {
    uint8*  data;
    uint32  rpos;
    uint32  wpos;
    uint32  capacity;
} ByteBuffer;

ByteBuffer* ByteBuffer_New(uint32 initialCapacity);
void        ByteBuffer_Delete(ByteBuffer* bb);
void        ByteBuffer_Reset(ByteBuffer* bb);
void        ByteBuffer_Clear(ByteBuffer* bb);
void        ByteBuffer_Grow(ByteBuffer* bb, uint32 needed);
void        ByteBuffer_WriteUByte(ByteBuffer* bb, uint8 v);
void        ByteBuffer_WriteUShort(ByteBuffer* bb, uint16 v);
void        ByteBuffer_WriteUInt(ByteBuffer* bb, uint32 v);
void        ByteBuffer_WriteUInt64(ByteBuffer* bb, uint64 v);
void        ByteBuffer_WriteFloat(ByteBuffer* bb, float v);
void        ByteBuffer_WriteDouble(ByteBuffer* bb, double v);
void        ByteBuffer_WriteString(ByteBuffer* bb, const char* s);
void        ByteBuffer_WriteBytes(ByteBuffer* bb, const void* data, uint32 len);
void        ByteBuffer_WriteGuid(ByteBuffer* bb, uint64 guid);
uint8       ByteBuffer_ReadUByte(ByteBuffer* bb);
uint16      ByteBuffer_ReadUShort(ByteBuffer* bb);
uint32      ByteBuffer_ReadUInt(ByteBuffer* bb);
uint64      ByteBuffer_ReadUInt64(ByteBuffer* bb);
float       ByteBuffer_ReadFloat(ByteBuffer* bb);
double      ByteBuffer_ReadDouble(ByteBuffer* bb);
const char* ByteBuffer_ReadString(ByteBuffer* bb, char* out, uint32 maxLen);
uint32      ByteBuffer_ReadBytes(ByteBuffer* bb, void* out, uint32 len);
uint64      ByteBuffer_ReadGuid(ByteBuffer* bb);
uint8       ByteBuffer_PeekUByte(ByteBuffer* bb);
uint16      ByteBuffer_PeekUShort(ByteBuffer* bb);
bool        ByteBuffer_Eof(ByteBuffer* bb);
uint32      ByteBuffer_Size(ByteBuffer* bb);
const uint8* ByteBuffer_Data(ByteBuffer* bb);

#endif
