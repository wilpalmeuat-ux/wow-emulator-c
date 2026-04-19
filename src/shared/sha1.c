/* sha1.c -- SHA-1 implementation (RFC 3174) for WoW 3.3.5a authentication
 * Pure C, no external dependencies. Used by SRP6 and packet encryption.
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t  buffer[64];
} SHA1_CTX;

#define ROL(v,n) (((v)<<(n))|((v)>>(32-(n))))

#define BLK0(i) (block[i] = (block[i]<<24) | ((block[i]&0xFF00)<<8) | \
                 ((block[i]>>8)&0xFF00) | (block[i]>>24))
#define BLK(i) (block[i&15] = ROL(block[(i+13)&15]^block[(i+8)&15]^ \
                block[(i+2)&15]^block[i&15], 1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK0(i)+0x5A827999+ROL(v,5);w=ROL(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK(i)+0x5A827999+ROL(v,5);w=ROL(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+BLK(i)+0x6ED9EBA1+ROL(v,5);w=ROL(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+BLK(i)+0x8F1BBCDC+ROL(v,5);w=ROL(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+BLK(i)+0xCA62C1D6+ROL(v,5);w=ROL(w,30);

static void sha1_transform(uint32_t state[5], const uint8_t buf[64]) {
    uint32_t a, b, c, d, e;
    uint32_t block[16];
    memcpy(block, buf, 64);

    a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

void SHA1_Init(SHA1_CTX* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

void SHA1_Update(SHA1_CTX* ctx, const uint8_t* data, uint32_t len) {
    uint32_t i, j;
    j = (uint32_t)(ctx->count & 63);
    ctx->count += len;
    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64)
            sha1_transform(ctx->state, &data[i]);
        j = 0;
    } else i = 0;
    memcpy(&ctx->buffer[j], &data[i], len - i);
}

void SHA1_Final(uint8_t digest[20], SHA1_CTX* ctx) {
    uint8_t finalcount[8];
    uint64_t bits = ctx->count * 8;
    for (int i = 0; i < 8; i++)
        finalcount[i] = (uint8_t)(bits >> ((7 - i) * 8));
    uint8_t c = 0x80;
    SHA1_Update(ctx, &c, 1);
    while ((ctx->count & 63) != 56) {
        c = 0; SHA1_Update(ctx, &c, 1);
    }
    SHA1_Update(ctx, finalcount, 8);
    for (int i = 0; i < 20; i++)
        digest[i] = (uint8_t)(ctx->state[i >> 2] >> ((3 - (i & 3)) * 8));
}

void SHA1_Hash(uint8_t digest[20], const uint8_t* data, uint32_t len) {
    SHA1_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    SHA1_Final(digest, &ctx);
}

/* HMAC-SHA1 for packet encryption */
void HMAC_SHA1(uint8_t out[20], const uint8_t* key, uint32_t keyLen,
               const uint8_t* data, uint32_t dataLen) {
    uint8_t ipad[64], opad[64], kbuf[64];
    memset(kbuf, 0, 64);
    if (keyLen > 64) {
        SHA1_Hash(kbuf, key, keyLen);
    } else {
        memcpy(kbuf, key, keyLen);
    }
    for (int i = 0; i < 64; i++) {
        ipad[i] = kbuf[i] ^ 0x36;
        opad[i] = kbuf[i] ^ 0x5C;
    }
    /* inner hash */
    SHA1_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, ipad, 64);
    SHA1_Update(&ctx, data, dataLen);
    uint8_t inner[20];
    SHA1_Final(inner, &ctx);
    /* outer hash */
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, opad, 64);
    SHA1_Update(&ctx, inner, 20);
    SHA1_Final(out, &ctx);
}

/* ARC4 (RC4) stream cipher for WoW packet header encryption */
typedef struct {
    uint8_t S[256];
    uint8_t i, j;
} ARC4_CTX;

void ARC4_Init(ARC4_CTX* ctx, const uint8_t* key, uint32_t keyLen) {
    for (int i = 0; i < 256; i++) ctx->S[i] = (uint8_t)i;
    ctx->i = ctx->j = 0;
    uint8_t j = 0;
    for (int i = 0; i < 256; i++) {
        j = j + ctx->S[i] + key[i % keyLen];
        uint8_t tmp = ctx->S[i]; ctx->S[i] = ctx->S[j]; ctx->S[j] = tmp;
    }
}

void ARC4_Process(ARC4_CTX* ctx, uint8_t* data, uint32_t len) {
    for (uint32_t n = 0; n < len; n++) {
        ctx->i++;
        ctx->j += ctx->S[ctx->i];
        uint8_t tmp = ctx->S[ctx->i]; ctx->S[ctx->i] = ctx->S[ctx->j]; ctx->S[ctx->j] = tmp;
        data[n] ^= ctx->S[(uint8_t)(ctx->S[ctx->i] + ctx->S[ctx->j])];
    }
}

/* Drop first 1024 bytes (WoW uses "drop-1024" RC4) */
void ARC4_Drop(ARC4_CTX* ctx, uint32_t count) {
    uint8_t dummy[256];
    while (count > 0) {
        uint32_t n = count > 256 ? 256 : count;
        memset(dummy, 0, n);
        ARC4_Process(ctx, dummy, n);
        count -= n;
    }
}
