#ifndef WOW_WORLDSESSION_H
#define WOW_WORLDSESSION_H
#include <stdint.h>
#include <stdbool.h>
#include "shared/bytebuffer.h"
typedef struct WorldSession {
    uint32_t account;
    uint32_t guid;
    uint8_t  security;
    bool     logged_in;
    char     username[256];
} WorldSession;
void ws_handle_packet(uint32_t account, ByteBuffer* buf);
#endif
