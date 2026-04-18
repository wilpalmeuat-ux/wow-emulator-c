#include "shared/memory.h"
#include <stdlib.h>
#include <string.h>

static byte* _heap = NULL;
static size_t _heap_size = 0;
static size_t _alloc_ptr = 0;

#define HEAP_INIT_SIZE (8 * 1024 * 1024)

int memory_init(void) {
    _heap_size = HEAP_INIT_SIZE;
    _heap = (byte*)malloc(_heap_size);
    if (!_heap) return -1;
    memset(_heap, 0, _heap_size);
    _alloc_ptr = 0;
    printf("[Memory] Arena initialized: %zu bytes\n", _heap_size);
    return 0;
}

void* vmalloc(size_t size) {
    if (!_heap) memory_init();
    size = (size + 7) & ~7;
    if (_alloc_ptr + size > _heap_size) {
        size_t new_size = _heap_size * 2;
        byte* new_heap = (byte*)realloc(_heap, new_size);
        if (!new_heap) return NULL;
        _heap = new_heap;
        _heap_size = new_size;
        printf("[Memory] Expanded arena to %zu bytes\n", _heap_size);
    }
    void* ptr = &_heap[_alloc_ptr];
    _alloc_ptr += size;
    memset(ptr, 0, size);
    return ptr;
}

void memory_cleanup(void) {
    if (_heap) {
        free(_heap);
        _heap = NULL;
        _alloc_ptr = 0;
        _heap_size = 0;
    }
}

size_t memory_used(void) {
    return _alloc_ptr;
}

size_t memory_available(void) {
    return _heap_size - _alloc_ptr;
}
