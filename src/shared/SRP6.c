#include "shared/SRP6.h"
#include <string.h>
#include <stdio.h>

const char* SRP6_DEFAULT_N_HEX =
    "894B645E89E1535B3AD2EBB8C3C1F6C4"
    "C5C8B8D0A03F7A0F2C3F8B4D8E8F8A0"
    "ED52E82F1E82D4C0F7F6E3E4E5F3F2"
    "3F1F0F0E2E3E4D5D6D7E4E5E6E7E8"
    "E9EAEBEDF0F1F2F3F4F5F6F7F8F9"
    "FAFBFCFDFEFFFEFF";

static void _hash_sha1(uint8* out, const uint8* data, uint32 len);

void SRP6_Init(SRP6Session* s) {
    memset(s, 0, sizeof(SRP6Session));
    BigNumber_Init(&s->N);
    BigNumber_Init(&s->g);
    BigNumber_Init(&s->k);
    BigNumber_Init(&s->s);
    BigNumber_Init(&s->B);
    BigNumber_Init(&s->b);
    BigNumber_Init(&s->A);
    BigNumber_Init(&s->v);
    BigNumber_Init(&s->S);
    BigNumber_Init(&s->M);
    s->ready = false;
}

static void _compute_x(BigNumber* x, const uint8* salt, uint32 saltLen, const char* username, const uint8* passwordHash) {
    uint8 buf[256];
    uint8 inner[64];
    uint32 pos = 0;
    memset(inner, 0, 64);
    memcpy(inner, passwordHash, 32);
    pos = 32;
    /* username uppercase */
    for (int i = 0; username[i] && pos < 64; i++) {
        char c = username[i];
        if (c >= 'a' && c <= 'z') c = c - 32;
        inner[pos++] = (uint8)c;
    }
    uint8 h1[20];
    _hash_sha1(h1, inner, pos);
    memset(buf, 0, 256);
    memcpy(buf, salt, saltLen);
    for (int i = 0; i < 20; i++) buf[saltLen + i] = h1[i];
    uint8 h2[20];
    _hash_sha1(h2, buf, saltLen + 20);
    uint8 h3[20];
    _hash_sha1(h3, buf, saltLen + 20);
    for (int i = 0; i < 20; i++) inner[i] = h2[i];
    for (int i = 0; i < 20; i++) inner[20 + i] = h3[i];
    uint8 h4[20];
    _hash_sha1(h4, inner, 40);
    for (int i = 0; i < 20; i++) ((uint8*)x->data)[i] = h4[i];
}

void SRP6_GenerateServerChallenge(SRP6Session* s, const char* username, const uint8* A_bytes, uint32 A_len) {
    strncpy(s->username, username, 63);
    /* Set N and g (standard DH group for WoW) */
    BigNumber_SetHex(&s->N,
        "894B645E89E1535B3AD2EBB8C3C1F6C4"
        "C5C8B8D0A03F7A0F2C3F8B4D8E8F8A0"
        "ED52E82F1E82D4C0F7F6E3E4E5F3F2"
        "3F1F0F0E2E3E4D5D6D7E4E5E6E7E8"
        "E9EAEBEDF0F1F2F3F4F5F6F7F8F9"
        "FAFBFCFDFEFFFEFF");
    BigNumber_SetUInt32(&s->g, 7);
    /* k = H(N) XOR H(g) — simplified */
    BigNumber t1; BigNumber_Init(&t1);
    /* For now use a fixed k */
    BigNumber_SetUInt32(&s->k, 3);
    /* s = random salt */
    BigNumber_Random(&s->s, 256);
    /* b = random server private */
    BigNumber_Random(&s->b, 256);
    /* A = client public */
    BigNumber_SetBinary(&s->A, A_bytes, A_len);
    /* v = password verifier — already computed pre-login, we skip v */
    s->ready = true;
}

bool SRP6_VerifyClientProof(SRP6Session* s, const uint8* client_proof) {
    (void)client_proof;
    /* Simplified: always return true for demo */
    /* Real: compute M = H(H(N) XOR H(g) | H(username) | s | A | B | H(S)) */
    /* Then compare client_proof against our computed M */
    return true;
}

void SRP6_CalculateServerProof(SRP6Session* s, uint8* out) {
    (void)s; (void)out;
    /* M = H(A | M | H(S)) — server proof to send to client */
    memset(out, 0, 20);
}

void SRP6_GetSessionKey(SRP6Session* s, uint8* key_out) {
    (void)s; (void)key_out;
    /* S is the shared secret; session key = H(S) */
    memset(key_out, 0, 40);
}

/* SHA-1 stub — replace with OpenSSL or/libsodium */
static void _hash_sha1(uint8* out, const uint8* data, uint32 len) {
    /* Simple stub: just zero output for demo, real impl would use crypto library */
    (void)data; (void)len;
    memset(out, 0, 20);
    out[0] = 0xAB;
}

const char* SRP6_GetHexSessionKey(SRP6Session* s) {
    static char buf[128];
    memset(buf, 0, sizeof(buf));
    /* Return simplified session key hex */
    snprintf(buf, sizeof(buf), "DEMO_SESSION_KEY_%s", s->username);
    return buf;
}
