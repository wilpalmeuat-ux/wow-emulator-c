#ifndef BIGNUM_H
#define BIGNUM_H

#include "Types.h"
#include <stdbool.h>

#ifndef uint64
typedef uint64_t uint64;
#endif

typedef struct {
    uint32 data[32];
} BigNumber;

void       BigNumber_Init(BigNumber* bn);
void       BigNumber_SetUInt32(BigNumber* bn, uint32 val);
void       BigNumber_SetUInt64(BigNumber* bn, uint64 val);
void       BigNumber_SetBinary(BigNumber* bn, const uint8* bytes, uint32 len);
void       BigNumber_SetHex(BigNumber* bn, const char* hex);
void       BigNumber_Add(BigNumber* bn, const BigNumber* a, const BigNumber* b);
void       BigNumber_Sub(BigNumber* bn, const BigNumber* a, const BigNumber* b);
void       BigNumber_Mul(BigNumber* bn, const BigNumber* a, const BigNumber* b);
void       BigNumber_Mod(BigNumber* bn, const BigNumber* a, const BigNumber* m);
void       BigNumber_ModExp(BigNumber* r, const BigNumber* a, const BigNumber* e, const BigNumber* m);
void       BigNumber_Invert(BigNumber* bn, const BigNumber* a, const BigNumber* m);
bool       BigNumber_IsZero(const BigNumber* bn);
bool       BigNumber_IsOne(const BigNumber* bn);
bool       BigNumber_Equals(const BigNumber* a, const BigNumber* b);
int        BigNumber_Cmp(const BigNumber* a, const BigNumber* b);
void       BigNumber_BN2Bin(const BigNumber* bn, uint8* out, uint32* len);
uint32     BigNumber_GetNumBits(const BigNumber* bn);
void       BigNumber_Random(BigNumber* bn, uint32 bits);
const char* BigNumber_ToHex(const BigNumber* bn);

#endif
