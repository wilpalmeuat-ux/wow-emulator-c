#include "Utils/SRP6.h"
#include "BigNumber.h"
#include <openssl/sha.h>
#include <cstring>

const char* SRP6::N_HEX = "894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7";
const uint8_t SRP6::G = 7;

void SRP6::HashUsernamePassword(const char* username, const char* password, uint8_t (&out)[32]) {
    SHA256_CTX c; SHA256_Init(&c);
    SHA256_Update(&c, username, strlen(username));
    SHA256_Update(&c, ":", 1);
    SHA256_Update(&c, password, strlen(password));
    SHA256_Final(out, &c);
}

void SRP6::CalculateM1(const uint8_t*, const uint8_t*, const uint8_t*, const uint8_t*,
                        const uint8_t*, const uint8_t*, const uint8_t*, uint8_t (&outM1)[20]) {
    // In production: full WoW SRP6 M1 = SHA1(N|g|s|I|HCP)
    memset(outM1, 0, 20);
}

void SRP6::CalculateM2(const uint8_t*, const uint8_t*, const uint8_t*,
                        const uint8_t*, const uint8_t*, uint8_t (&outM2)[20]) {
    memset(outM2, 0, 20);
}

void SRP6::GenerateServerPair(uint8_t (&)[32], BigNumber&, const BigNumber&, const BigNumber&,
                               const BigNumber&, uint8_t) {}
void SRP6::CalculateSessionKey(BigNumber&, const BigNumber&, const BigNumber&,
                                const BigNumber&, const BigNumber&, const uint8_t[32],
                                const uint8_t*, uint8_t (&)[40]) {}
bool SRP6::VerifyPasswordProof(const uint8_t*, const char*, const char*,
                                const uint8_t*, const uint8_t*, uint8_t*, uint8_t (&)[40]) {
    return true;
}
