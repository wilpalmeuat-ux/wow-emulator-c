#include "shared/BigNumber.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static void _copy(BigNumber* dst, const BigNumber* src) { memcpy(dst->data, src->data, sizeof(src->data)); }
static void _zero(BigNumber* bn) { memset(bn->data, 0, sizeof(bn->data)); }

static void _normalize(BigNumber* bn) {
    for (int i = 31; i >= 0; i--) {
        if (bn->data[i] != 0) return;
        if (i == 0) return;
    }
}

static uint32 _add_with_carry(uint32* dest, uint32 a, uint32 b, uint32 carry) {
    uint64 sum = (uint64)a + (uint64)b + (uint64)carry;
    *dest = (uint32)sum;
    return (uint32)(sum >> 32);
}

static uint32 _sub_with_borrow(uint32* dest, uint32 a, uint32 b, uint32 borrow) {
    uint64 diff = (uint64)a - (uint64)b - (uint64)borrow;
    *dest = (uint32)diff;
    return (uint32)(-(int64)(diff >> 32));
}

void BigNumber_Init(BigNumber* bn) { _zero(bn); }
void BigNumber_SetUInt32(BigNumber* bn, uint32 val) { _zero(bn); bn->data[0] = val; }
void BigNumber_SetUInt64(BigNumber* bn, uint64 val) { _zero(bn); bn->data[0] = (uint32)(val & 0xFFFFFFFFULL); bn->data[1] = (uint32)(val >> 32); }

void BigNumber_SetBinary(BigNumber* bn, const uint8* bytes, uint32 len) {
    _zero(bn);
    for (uint32 i = 0; i < len && i < 128; i++) {
        uint32 wordIdx = i / 4;
        uint32 byteOffset = i % 4;
        bn->data[wordIdx] |= ((uint32)bytes[len - 1 - i]) << (byteOffset * 8);
    }
}

void BigNumber_SetHex(BigNumber* bn, const char* hex) {
    _zero(bn);
    size_t len = strlen(hex);
    for (size_t i = 0; i < len && i < 256; i++) {
        char c = hex[len - 1 - i];
        uint32 nibble = 0;
        if (c >= '0' && c <= '9') nibble = c - '0';
        else if (c >= 'a' && c <= 'f') nibble = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') nibble = 10 + (c - 'A');
        else continue;
        uint32 wordIdx = i / 8;
        bn->data[wordIdx] |= (nibble << ((i % 8) * 4));
    }
}

void BigNumber_Add(BigNumber* bn, const BigNumber* a, const BigNumber* b) {
    uint32 carry = 0;
    for (int i = 0; i < 32; i++) { carry = _add_with_carry(&bn->data[i], a->data[i], b->data[i], carry); }
    (void)_normalize;
}

void BigNumber_Sub(BigNumber* bn, const BigNumber* a, const BigNumber* b) {
    uint32 borrow = 0;
    for (int i = 0; i < 32; i++) { borrow = _sub_with_borrow(&bn->data[i], a->data[i], b->data[i], borrow); }
}

void BigNumber_Mul(BigNumber* bn, const BigNumber* a, const BigNumber* b) {
    _zero(bn);
    for (int i = 0; i < 32; i++) {
        uint64 carry = 0;
        for (int j = 0; j < 32 - i; j++) {
            uint64 prod = (uint64)a->data[i] * (uint64)b->data[j] + bn->data[i + j] + carry;
            bn->data[i + j] = (uint32)prod;
            carry = prod >> 32;
        }
    }
}

void BigNumber_Mod(BigNumber* bn, const BigNumber* a, const BigNumber* m) {
    BigNumber t; _copy(&t, a);
    int bits = BigNumber_GetNumBits(m);
    while (BigNumber_Cmp(&t, m) >= 0) {
        int shift = 0;
        int taBits = BigNumber_GetNumBits(&t);
        shift = taBits - bits;
        if (shift < 0) shift = 0;
        BigNumber c; BigNumber_Init(&c);
        c.data[shift / 32] = 1;
        BigNumber_Mul(&c, &c, m);
        BigNumber_Sub(&t, &t, &c);
        _copy(&t, &t);
    }
    _copy(bn, &t);
}

void BigNumber_ModExp(BigNumber* r, const BigNumber* base, const BigNumber* exp, const BigNumber* mod) {
    BigNumber base_copy; _copy(&base_copy, base);
    BigNumber result; BigNumber_Init(&result); BigNumber_SetUInt32(&result, 1);
    BigNumber exp_copy; _copy(&exp_copy, exp);
    BigNumber mod_copy; _copy(&mod_copy, mod);
    while (!BigNumber_IsZero(&exp_copy)) {
        if (exp_copy.data[0] & 1) { BigNumber_Mul(&result, &result, &base_copy); BigNumber_Mod(&result, &result, &mod_copy); }
        BigNumber_Mul(&base_copy, &base_copy, &base_copy);
        BigNumber_Mod(&base_copy, &base_copy, &mod_copy);
        for (int i = 0; i < 32; i++) { exp_copy.data[i] >>= 1; }
    }
    _copy(r, &result);
}

bool BigNumber_IsZero(const BigNumber* bn) {
    for (int i = 0; i < 32; i++) if (bn->data[i]) return false;
    return true;
}

bool BigNumber_IsOne(const BigNumber* bn) {
    for (int i = 1; i < 32; i++) if (bn->data[i]) return false;
    return bn->data[0] == 1;
}

bool BigNumber_Equals(const BigNumber* a, const BigNumber* b) { return memcmp(a->data, b->data, sizeof(a->data)) == 0; }

int BigNumber_Cmp(const BigNumber* a, const BigNumber* b) {
    for (int i = 31; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0;
}

void BigNumber_BN2Bin(const BigNumber* bn, uint8* out, uint32* len) {
    int bits = BigNumber_GetNumBits(bn);
    int lenBytes = (bits + 7) / 8;
    if (lenBytes <= 0) lenBytes = 1;
    memset(out, 0, lenBytes);
    for (int i = lenBytes - 1; i >= 0; i--) {
        int word = i / 4;
        int byte = i % 4;
        out[lenBytes - 1 - i] = (uint8)((bn->data[word] >> (byte * 8)) & 0xFF);
    }
    if (len) *len = lenBytes;
}

uint32 BigNumber_GetNumBits(const BigNumber* bn) {
    for (int i = 31; i >= 0; i--) {
        uint32 v = bn->data[i];
        if (!v) continue;
        for (int b = 31; b >= 0; b--) {
            if (v & (1U << b)) return (uint32)(i * 32 + b + 1);
        }
    }
    return 0;
}

static uint64 _next_rand(uint64* s) { *s = (*s * 6364136223846793005ULL + 1442695040888963407ULL); return *s; }
void BigNumber_Random(BigNumber* bn, uint32 bits) {
    _zero(bn);
    uint64 seed = (uint64)time(NULL) ^ (uint64)clock();
    uint32 words = (bits + 31) / 32;
    if (words > 32) words = 32;
    for (uint32 i = 0; i < words; i++) bn->data[i] = (uint32)_next_rand(&seed);
    if (words > 0 && bits < 32) bn->data[0] &= ((1U << (bits % 32)) - 1);
}

static char _hexBuf[256];
const char* BigNumber_ToHex(const BigNumber* bn) {
    int pos = 0;
    memset(_hexBuf, 0, sizeof(_hexBuf));
    bool leading = true;
    for (int i = 31; i >= 0; i--) {
        for (int nibble = 7; nibble >= 0; nibble--) {
            uint32 part = (bn->data[i] >> (nibble * 4)) & 0xF;
            if (leading && part == 0) continue;
            leading = false;
            _hexBuf[pos++] = part < 10 ? ('0' + part) : ('a' + part - 10);
        }
    }
    if (leading) _hexBuf[pos++] = '0';
    _hexBuf[pos] = 0;
    return _hexBuf;
}
