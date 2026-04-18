#include <cstdlib>
#include <cstring>

void* MemAlloc(size_t size) { return malloc(size); }
void MemFree(void* ptr) { free(ptr); }
void* MemRealloc(void* ptr, size_t size) { return realloc(ptr, size); }
void MemSet(void* ptr, int val, size_t size) { memset(ptr, val, size); }
void MemCopy(void* dest, const void* src, size_t size) { memcpy(dest, src, size); }
