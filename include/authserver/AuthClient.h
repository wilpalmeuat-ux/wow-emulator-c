#ifndef WOW_AUTHSERVER_CLIENT_H
#define WOW_AUTHSERVER_CLIENT_H
#include <stdint.h>
void auth_handle_logon(uint8_t* packetData);
void auth_handle_reconnect(uint8_t* packetData);
#endif
