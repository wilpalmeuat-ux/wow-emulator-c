#include "Utils/BigNumber.h"
#include <cstring>
#include <openssl/bn.h>

BigNumber::BigNumber() { _bn = BN_new(); }
BigNumber::BigNumber(uint32_t val) { _bn = BN_new(); SetUInt32(val); }
BigNumber::~BigNumber() { if (_bn) BN_free(_bn); }
BigNumber::BigNumber(const BigNumber& o) { _bn = BN_dup(o._bn); }
BigNumber& BigNumber::operator=(const BigNumber& o) {
    if (this != &o) { BN_free(_bn); _bn = BN_dup(o._bn); }
    return *this;
}
void BigNumber::SetUInt32(uint32_t val) { BN_set_word(_bn, val); }
void BigNumber::SetUInt64(uint64_t val) {
    unsigned char buf[8];
    for (int i = 0; i < 8; ++i) buf[i] = (val >> (56 - i*8)) & 0xFF;
    BN_bin2bn(buf, 8, _bn);
}
void BigNumber::SetBinary(const uint8_t* bytes, size_t len) {
    BN_bin2bn(bytes, (int)len, _bn);
}
void BigNumber::SetHexString(const char* str) { BN_hex2bn(&_bn, str); }

std::string BigNumber::AsHexStr() const {
    char* s = BN_bn2hex(_bn);
    std::string r = s;
    OPENSSL_free(s);
    return r;
}
std::string BigNumber::AsDecStr() const {
    char* s = BN_bn2dec(_bn);
    std::string r = s;
    OPENSSL_free(s);
    return r;
}
std::vector<uint8_t> BigNumber::AsByteArray(size_t minSize, bool le) const {
    int n = BN_num_bytes(_bn);
    int sz = (int)(minSize > (size_t)n ? minSize : (size_t)n);
    std::vector<uint8_t> r(sz, 0);
    BN_bn2bin(_bn, r.data() + (le ? 0 : sz - n));
    if (le) std::reverse(r.begin(), r.end());
    return r;
}
BigNumber BigNumber::operator+(const BigNumber& o) const {
    BigNumber r; BN_add(r._bn, _bn, o._bn); return r;
}
BigNumber BigNumber::operator-(const BigNumber& o) const {
    BigNumber r; BN_sub(r._bn, _bn, o._bn); return r;
}
BigNumber BigNumber::operator*(const BigNumber& o) const {
    BigNumber r; BN_CTX* ctx = BN_CTX_new(); BN_mul(r._bn, _bn, o._bn, ctx); BN_CTX_free(ctx); return r;
}
BigNumber& BigNumber::operator+=(const BigNumber& o) { BN_add(_bn, _bn, o._bn); return *this; }
bool BigNumber::IsZero() const { return BN_is_zero(_bn) != 0; }
bool BigNumber::IsNegative() const { return BN_is_negative(_bn) != 0; }
int BigNumber::Compare(const BigNumber& o) const { return BN_cmp(_bn, o._bn); }
