#ifndef SRP6_H
#define SRP6_H

#include "Types.h"
#include "BigNumber.h"
#include <stdbool.h>

/* SRP6B for WoW 3.3.5a
 * N and g are standard Diffie-Hellman group parameters
 * BUG: WoW uses a MODIFIED generator (g=7) but the formula uses g=3 in some steps
 */
typedef struct {
    BigNumber N;
    BigNumber g;
    BigNumber k;
    BigNumber s;    /* salt */
    BigNumber B;    /* server public */
    BigNumber b;    /* server private */
    BigNumber A;    /* client public */
    BigNumber v;    /* stored verifier */
    BigNumber S;    /* shared secret */
    BigNumber M;    /* session key */
    bool      ready;
    char      username[65];
} SRP6Session;

void  SRP6_Init(SRP6Session* s);
void  SRP6_SetPasswordHash(SRP6Session* s, const uint8* hash, uint32 len);
void  SRP6_GenerateServerChallenge(SRP6Session* s, const char* username, const uint8* A_bytes, uint32 A_len);
bool  SRP6_VerifyClientProof(SRP6Session* s, const uint8* client_proof);
void  SRP6_CalculateServerProof(SRP6Session* s, uint8* out);
void  SRP6_GetSessionKey(SRP6Session* s, uint8* key_out);
const char* SRP6_GetHexSessionKey(SRP6Session* s);

/* Built-in N for WoW (pre-computed, this is the RFC 5054 group) */
extern const char* SRP6_DEFAULT_N_HEX;

#endif
