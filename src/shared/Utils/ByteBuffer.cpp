#include "Network/Socket.h"
#include <algorithm>

void ByteBuffer::_resize(size_t) {}
void ByteBuffer::PrintHex() const {
    for (size_t i = 0; i < _storage.size(); ++i)
        printf("%02x ", _storage[i]);
    printf("\n");
}
