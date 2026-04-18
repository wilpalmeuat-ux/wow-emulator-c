#include "shared/ByteBuffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const uint32 INITIAL_CAP = 4096;

ByteBuffer* ByteBuffer_New(uint32 initialCapacity) {
    ByteBuffer* bb = calloc(1, sizeof(ByteBuffer));
    bb->capacity = initialCapacity > 0 ? initialCapacity : INITIAL_CAP;
    bb->data = malloc(bb->capacity);
    memset(bb->data, 0, bb->capacity);
    return bb;
}

void ByteBuffer_Delete(ByteBuffer* bb) {
    if (!bb) return;
    free(bb->data);
    free(bb);
}

void ByteBuffer_Reset(ByteBuffer* bb) { bb->rpos = 0; bb->wpos = 0; }
void ByteBuffer_Clear(ByteBuffer* bb) { memset(bb->data, 0, bb->capacity); bb->rpos = 0; bb->wpos = 0; }

void ByteBuffer_Grow(ByteBuffer* bb, uint32 needed) {
    uint32 required = bb->wpos + needed;
    if (required <= bb->capacity) return;
    uint32 newCap = bb->capacity * 2;
    while (newCap < required) newCap *= 2;
    uint8* newData = malloc(newCap);
    memset(newData, 0, newCap);
    memcpy(newData, bb->data, bb->wpos);
    free(bb->data);
    bb->data = newData;
    bb->capacity = newCap;
}

void ByteBuffer_WriteUByte(ByteBuffer* bb, uint8 v) { ByteBuffer_Grow(bb, 1); bb->data[bb->wpos++] = v; }
void ByteBuffer_WriteUShort(ByteBuffer* bb, uint16 v) { ByteBuffer_Grow(bb, 2); bb->data[bb->wpos++] = (uint8)(v & 0xFF); bb->data[bb->wpos++] = (uint8)((v >> 8) & 0xFF); }
void ByteBuffer_WriteUInt(ByteBuffer* bb, uint32 v) { ByteBuffer_Grow(bb, 4); bb->data[bb->wpos++] = (uint8)(v & 0xFF); bb->data[bb->wpos++] = (uint8)((v >> 8) & 0xFF); bb->data[bb->wpos++] = (uint8)((v >> 16) & 0xFF); bb->data[bb->wpos++] = (uint8)((v >> 24) & 0xFF); }
void ByteBuffer_WriteUInt64(ByteBuffer* bb, uint64 v) { ByteBuffer_Grow(bb, 8); for (int i = 0; i < 8; i++) bb->data[bb->wpos++] = (uint8)((v >> (i * 8)) & 0xFF); }

void ByteBuffer_WriteFloat(ByteBuffer* bb, float v) { union { float f; uint32 i; } u; u.f = v; ByteBuffer_WriteUInt(bb, u.i); }
void ByteBuffer_WriteDouble(ByteBuffer* bb, double v) { union { double d; uint64 i; } u; u.d = v; ByteBuffer_WriteUInt64(bb, u.i); }

void ByteBuffer_WriteString(ByteBuffer* bb, const char* s) {
    if (!s) { ByteBuffer_WriteUInt(bb, 0); return; }
    uint32 len = (uint32)strlen(s) + 1;
    ByteBuffer_WriteUInt(bb, len);
    ByteBuffer_WriteBytes(bb, s, len);
}

void ByteBuffer_WriteBytes(ByteBuffer* bb, const void* data, uint32 len) { ByteBuffer_Grow(bb, len); memcpy(bb->data + bb->wpos, data, len); bb->wpos += len; }

void ByteBuffer_WriteGuid(ByteBuffer* bb, uint64 guid) {
    uint8 mask = 0; uint8 bytes[8] = {0};
    for (int i = 0; i < 8; i++) { if ((guid >> (i * 8)) & 0xFF) { mask |= (1 << i); bytes[i] = (uint8)((guid >> (i * 8)) & 0xFF); } }
    ByteBuffer_WriteUByte(bb, mask);
    for (int i = 0; i < 8; i++) if (mask & (1 << i)) ByteBuffer_WriteUByte(bb, bytes[i]);
}

uint8  ByteBuffer_ReadUByte(ByteBuffer* bb)  { return bb->rpos < bb->wpos ? bb->data[bb->rpos++] : 0; }
uint16 ByteBuffer_ReadUShort(ByteBuffer* bb) { uint16 v = 0; v |= (uint16)ByteBuffer_ReadUByte(bb); v |= ((uint16)ByteBuffer_ReadUByte(bb) << 8); return v; }
uint32 ByteBuffer_ReadUInt(ByteBuffer* bb)   { uint32 v = 0; for (int i = 0; i < 4; i++) v |= ((uint32)ByteBuffer_ReadUByte(bb) << (i * 8)); return v; }
uint64 ByteBuffer_ReadUInt64(ByteBuffer* bb) { uint64 v = 0; for (int i = 0; i < 8; i++) v |= ((uint64)ByteBuffer_ReadUByte(bb) << (i * 8)); return v; }
float  ByteBuffer_ReadFloat(ByteBuffer* bb)  { union { float f; uint32 i; } u; u.i = ByteBuffer_ReadUInt(bb); return u.f; }
double ByteBuffer_ReadDouble(ByteBuffer* bb) { union { double d; uint64 i; } u; u.i = ByteBuffer_ReadUInt64(bb); return u.d; }

const char* ByteBuffer_ReadString(ByteBuffer* bb, char* out, uint32 maxLen) {
    uint32 len = ByteBuffer_ReadUInt(bb);
    if (len == 0 || len > maxLen) { if (out) out[0] = 0; return out; }
    uint32 r = bb->rpos; bb->rpos += len;
    uint32 c = 0;
    while (c < len - 1 && c < maxLen - 1) { out[c] = (char)bb->data[r + c]; c++; }
    out[c] = 0;
    return out;
}

uint32 ByteBuffer_ReadBytes(ByteBuffer* bb, void* out, uint32 len) { uint32 available = bb->wpos - bb->rpos; uint32 take = MIN(len, available); memcpy(out, bb->data + bb->rpos, take); bb->rpos += take; return take; }
uint64 ByteBuffer_ReadGuid(ByteBuffer* bb) { uint8 mask = ByteBuffer_ReadUByte(bb); uint64 guid = 0; for (int i = 0; i < 8; i++) if (mask & (1 << i)) guid |= ((uint64)ByteBuffer_ReadUByte(bb) << (i * 8)); return guid; }
uint8  ByteBuffer_PeekUByte(ByteBuffer* bb) { return bb->rpos < bb->wpos ? bb->data[bb->rpos] : 0; }
uint16 ByteBuffer_PeekUShort(ByteBuffer* bb) { uint32 saved = bb->rpos; uint16 v = ByteBuffer_ReadUShort(bb); bb->rpos = saved; return v; }
bool   ByteBuffer_Eof(ByteBuffer* bb) { return bb->rpos >= bb->wpos; }
uint32 ByteBuffer_Size(ByteBuffer* bb) { return bb->wpos; }
const uint8* ByteBuffer_Data(ByteBuffer* bb) { return bb->data; }
