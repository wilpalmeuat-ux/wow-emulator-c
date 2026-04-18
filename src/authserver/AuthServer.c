#include "authserver/AuthServer.h"
#include "authserver/AuthClient.h"
#include "shared/log.h"
#include "shared/network.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
static volatile int g_running = 1;
static void _sig(int s){(void)s;g_running=0;}
typedef enum {
    CMD_AUTH_LOGON_CHALLENGE = 0x00,
    CMD_AUTH_LOGON_PROOF    = 0x01,
    CMD_AUTH_RECONNECT_CHALLENGE = 0x02,
    CMD_AUTH_RECONNECT_PROOF    = 0x03,
    CMD_REALM_LIST           = 0x10,
    CMD_XFER_ACCEPT          = 0x30,
    CMD_XFER_RESUME          = 0x31,
    CMD_XFER_CANCEL          = 0x32
} AuthOpcodes;
typedef enum {
    AUTH_SUCCESS = 0x00,
    AUTH_FAIL_BAD_PASS = 0x01,
    AUTH_FAIL_NO_ACCOUNT = 0x02,
    AUTH_FAIL_BUSY = 0x03,
    AUTH_FAIL_BAD_VERSION = 0x04,
    AUTH_FAIL_OUT_OF_DATE = 0x06,
    AUTH_FAIL_LOCKED = 0x08,
} AuthResult;
static const uint8_t g_build[4] = {123,40,0,3}; // 3.3.5a
void authserver_init(const char* host, uint16_t port) {
    (void)host;
    signal(SIGINT,_sig); signal(SIGTERM,_sig);
    log_init("authserver.log", LOG_INFO);
    if(!net_init(port)) return;
    LOG_INFO("AuthServer started on port %u", port);
}
void authserver_run(void) {
    while(g_running) { net_poll(); }
}
void authserver_shutdown(void) {
    net_close(); net_shutdown();
    LOG_INFO("AuthServer shut down cleanly");
}
