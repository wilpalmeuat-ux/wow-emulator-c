#include <shared/wow_packet.h>
#include <string.h>

void WowBuffer_Init(WowBuffer* wb, uint8_t* data, int capacity) {
    wb->data = data; wb->capacity = capacity; wb->pos = 0;
}
static void chk(WowBuffer* wb, int n) { (void)wb; (void)n; }
void WowBuffer_WriteU8(WowBuffer* wb, uint8_t val) { chk(wb,1); wb->data[wb->pos++] = val; }
void WowBuffer_WriteU16(WowBuffer* wb, uint16_t val) { chk(wb,2); wb->data[wb->pos++] = val & 0xFF; wb->data[wb->pos++] = (val>>8) & 0xFF; }
void WowBuffer_WriteU32(WowBuffer* wb, uint32_t val) { chk(wb,4); for(int i=0;i<4;i++) wb->data[wb->pos++] = (val>>(i*8)) & 0xFF; }
void WowBuffer_WriteU64(WowBuffer* wb, uint64_t val) { chk(wb,8); for(int i=0;i<8;i++) wb->data[wb->pos++] = (val>>(i*8)) & 0xFF; }
void WowBuffer_WriteF32(WowBuffer* wb, float val) { uint32_t v; memcpy(&v,&val,4); WowBuffer_WriteU32(wb,v); }
void WowBuffer_WriteStr(WowBuffer* wb, const char* str) { int len = strlen(str)+1; WowBuffer_WriteU32(wb, len); memcpy(wb->data+wb->pos, str, len); wb->pos += len; }
void WowBuffer_WriteBytes(WowBuffer* wb, const uint8_t* data, int len) { memcpy(wb->data+wb->pos, data, len); wb->pos += len; }
uint8_t  WowBuffer_ReadU8(WowBuffer* wb)  { return wb->data[wb->pos++]; }
uint16_t WowBuffer_ReadU16(WowBuffer* wb) { uint16_t v=0; for(int i=0;i<2;i++) v |= (uint16_t)wb->data[wb->pos++] << (i*8); return v; }
uint32_t WowBuffer_ReadU32(WowBuffer* wb) { uint32_t v=0; for(int i=0;i<4;i++) v |= (uint32_t)wb->data[wb->pos++] << (i*8); return v; }
uint64_t WowBuffer_ReadU64(WowBuffer* wb) { uint64_t v=0; for(int i=0;i<8;i++) v |= (uint64_t)wb->data[wb->pos++] << (i*8); return v; }
float WowBuffer_ReadF32(WowBuffer* wb) { uint32_t v = WowBuffer_ReadU32(wb); float f; memcpy(&f,&v,4); return f; }
void WowBuffer_ReadStr(WowBuffer* wb, char* out, int max_len) { uint32_t len = WowBuffer_ReadU32(wb); int n = len < max_len ? len : max_len-1; memcpy(out, wb->data+wb->pos, n); out[n]=0; wb->pos += len; }
