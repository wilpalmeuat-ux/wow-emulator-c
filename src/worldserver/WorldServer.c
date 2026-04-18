/* WorldServer.c — WoW 3.3.5a World Server (Windows)
 * WinSock2 + Windows API. No POSIX.
 */
#include "worldserver/WorldServer.h"
#include "worldserver/WorldSession.h"
#include "worldserver/WorldPlayer.h"
#include "shared/log.h"
#include "shared/network.h"
#include <stdio.h>
#include <windows.h>

static volatile int g_running = 1;

static BOOL WINAPI _sig(DWORD ctrl) { (void)ctrl; g_running = 0; return TRUE; }

void worldserver_init(const char* host, uint16_t port) {
    (void)host;
    SetConsoleCtrlHandler(_sig, TRUE);
    log_init("worldserver.log", LOG_INFO);
    if (!net_init(port)) return;
    LOG_INFO("WorldServer started on port %u", port);
}

void worldserver_run(void) {
    while (g_running) { net_poll(); }
}

void worldserver_shutdown(void) {
    net_close(); net_shutdown();
    LOG_INFO("WorldServer shut down cleanly");
}