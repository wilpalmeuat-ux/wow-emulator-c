#pragma once

#include "BigNumber.h"
#include <string>
#include <cstdint>

class SRP6 {
public:
    // Server constants (WoW uses these primes)
    static const char* N_HEX;
    static const uint8_t G;

    // Generate salt + server public value
    static void GenerateServerPair(
        uint8_t (&outSalt)[32],
        BigNumber& B,
        const BigNumber& a,
        const BigNumber& b,
        const BigNumber& N,
        uint8_t g
    );

    // Calculate session key
    static void CalculateSessionKey(
        BigNumber& K,
        const BigNumber& N,
        const BigNumber& g,
        const BigNumber& a,
        const BigNumber& B,
        const uint8_t salt[32],
        const uint8_t* usernameHash,
        const uint8_t* sessionKey
    );

    // Verify password proof
    static bool VerifyPasswordProof(
        const uint8_t* salt,
        const char* username,
        const char* password,
        const uint8_t* serverEphemeral,
        const uint8_t* clientProof,
        uint8_t* outServerProof,
        uint8_t (&sessionKey)[40]
    );

    static void HashUsernamePassword(
        const char* username,
        const char* password,
        uint8_t (&out)[32]
    );

    static void CalculateM1(
        const uint8_t* N,
        const uint8_t* g,
        const uint8_t* salt,
        const uint8_t* usernameHash,
        const uint8_t* clientPub,
        const uint8_t* serverPub,
        const uint8_t* sessionKey,
        uint8_t (&outM1)[20]
    );

    static void CalculateM2(
        const uint8_t* N,
        const uint8_t* usernameHash,
        const uint8_t* M1,
        const uint8_t* serverPub,
        const uint8_t* sessionKey,
        uint8_t (&outM2)[20]
    );
};
