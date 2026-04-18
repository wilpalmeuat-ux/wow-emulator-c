#include "authserver/AuthClient.h"
#include "shared/network.h"
#include "shared/log.h"
#include "shared/bytebuffer.h"
#include <string.h>
#include <stdio.h>
typedef enum {
    AUTH_SUCCESS = 0x00,
    AUTH_FAIL_BAD_PASS = 0x01,
    AUTH_FAIL_NO_ACCOUNT = 0x02,
    AUTH_FAIL_BUSY = 0x03,
    AUTH_FAIL_BAD_VERSION = 0x04,
    AUTH_FAIL_OUT_OF_DATE = 0x06,
    AUTH_FAIL_LOCKED = 0x08,
} AuthResult;
static uint8_t _cmd_auth_logon_challenge(uint8_t* data) {
    uint8_t error = AUTH_SUCCESS;
    char username[256]={0}; size_t i=0;
    for(size_t b=1; data[b]!=0 && i<254; b++) username[i++] = data[b];
    LOG_INFO("[Auth] Logon challenge for user: %s", username);
    return error;
}
static uint8_t _cmd_auth_logon_proof(uint8_t* data) {
    (void)data;
    LOG_INFO("[Auth] Logon proof received");
    return AUTH_SUCCESS;
}
void auth_handle_logon(uint8_t* packetData) {
    uint8_t cmd = packetData[0];
    switch(cmd) {
        case 0x00: _cmd_auth_logon_challenge(packetData+1); break;
        case 0x01: _cmd_auth_logon_proof(packetData+1); break;
        default: LOG_WARN("[Auth] Unknown auth cmd: 0x%02X", cmd); break;
    }
}
void auth_handle_reconnect(uint8_t* packetData) {
    (void)packetData;
    LOG_INFO("[Auth] Reconnect packet received");
}
