#include "Network/Socket.h"
#include <algorithm>

static uint8_t g_compressBuf[32768];
static uint8_t g_decompressBuf[32768];
