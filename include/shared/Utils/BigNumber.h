#pragma once

#include <string>
#include <openssl/bn.h>
#include <cstdint>

class BigNumber {
public:
    BigNumber();
    explicit BigNumber(uint32_t val);
    ~BigNumber();
    BigNumber(const BigNumber& o);
    BigNumber& operator=(const BigNumber& o);

    void SetUInt32(uint32_t val);
    void SetUInt64(uint64_t val);
    void SetBinary(const uint8_t* bytes, size_t len);
    void SetHexString(const char* str);

    std::string AsHexStr() const;
    std::string AsDecStr() const;
    std::vector<uint8_t> AsByteArray(size_t minSize = 0, bool littleEndian = true) const;

    BigNumber operator+(const BigNumber& o) const;
    BigNumber operator-(const BigNumber& o) const;
    BigNumber operator*(const BigNumber& o) const;
    BigNumber operator/(const BigNumber& o) const;
    BigNumber operator%(const BigNumber& o) const;

    BigNumber& operator+=(const BigNumber& o);
    BigNumber& operator-=(const BigNumber& o);
    BigNumber& operator*=(const BigNumber& o);

    bool IsZero() const;
    bool IsNegative() const;
    int Compare(const BigNumber& o) const;

    BIGNUM* BN() { return _bn; }
    const BIGNUM* BN() const { return _bn; }

private:
    BIGNUM* _bn = nullptr;
};
