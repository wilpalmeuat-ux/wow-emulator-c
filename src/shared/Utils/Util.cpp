#include "Utils/Util.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sys/time.h>
#include <sstream>

namespace Util {

std::vector<std::string> StringSplit(const std::string& str, char delim) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream ss(str);
    while (std::getline(ss, cur, delim)) out.push_back(cur);
    return out;
}
std::string StringToUpper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}
std::string StringToLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}
bool StringEqualI(const std::string& a, const std::string& b) {
    return StringToLower(a) == StringToLower(b);
}

bool Position::IsInDist2D(float cx, float cy, float dist) const {
    float dx = x - cx, dy = y - cy;
    return (dx*dx + dy*dy) < (dist*dist);
}
bool Position::IsInDist3D(float cx, float cy, float cz, float dist) const {
    float dx = x-cx, dy = y-cy, dz = z-cz;
    return (dx*dx + dy*dy + dz*dz) < (dist*dist);
}
float Position::GetExactDist2D(const Position& o) const {
    float dx=x-o.x, dy=y-o.y;
    return sqrtf(dx*dx+dy*dy);
}
float Position::GetExactDist3D(const Position& o) const {
    float dx=x-o.x, dy=y-o.y, dz=z-o.z;
    return sqrtf(dx*dx+dy*dy+dz*dz);
}
float Position::GetAngle(const Position& o) const {
    return atan2f(y-o.y, x-o.x);
}

uint32_t GetMSTime() {
    struct timeval tv; gettimeofday(&tv, nullptr);
    return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}
uint64_t GetMSTime64() {
    struct timeval tv; gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}
void ArrestMSTimeDiff(uint32_t start, uint32_t& diff) {
    diff = GetMSTime() - start;
}

} // namespace Util
