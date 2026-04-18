#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Util {

std::vector<std::string> StringSplit(const std::string& str, char delim);
std::string StringJoin(const std::vector<std::string>& v, char delim);
std::string StringToUpper(const std::string& str);
std::string StringToLower(const std::string& str);
std::string StringFormat(const char* fmt, ...);
bool StringEqualI(const std::string& a, const std::string& b);

inline uint32_t MakePair(uint16_t left, uint16_t right) {
    return (uint32_t(left) << 16) | uint32_t(right);
}
inline void SplitPair(uint32_t val, uint16_t& left, uint16_t& right) {
    left = uint16_t(val >> 16);
    right = uint16_t(val & 0xFFFF);
}

inline uint32_t HashFnv1a(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x01000193;
    }
    return h;
}
inline uint32_t HashString(const char* str) {
    return HashFnv1a(str, strlen(str));
}

struct Position {
    float x = 0, y = 0, z = 0, o = 0;
    uint32_t mapId = 0;

    Position() = default;
    Position(float _x, float _y, float _z, float _o = 0.0f, uint32_t m = 0)
        : x(_x), y(_y), z(_z), o(_o), mapId(m) {}

    bool IsInDist2D(float cx, float cy, float dist) const;
    bool IsInDist3D(float cx, float cy, float cz, float dist) const;
    float GetExactDist2D(const Position& o) const;
    float GetExactDist3D(const Position& o) const;
    float GetAngle(const Position& o) const;
    void Relocate(float _x, float _y, float _z, float _o = 0) {
        x = _x; y = _y; z = _z; o = _o;
    }
};

uint32_t GetMSTime();
uint64_t GetMSTime64();
void ArrestMSTimeDiff(uint32_t start, uint32_t& diff);

} // namespace Util
