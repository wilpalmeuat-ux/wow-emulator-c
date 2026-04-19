/* auth_main.c -- Standalone auth server entry point (optional)
 *
 * When building the combined binary (main.c), the auth server is
 * started as a thread from WorldServer.  This file provides a
 * standalone entry point if you want to run the auth server
 * separately.
 */
#include "authserver/auth_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

static volatile int g_running = 1;

static BOOL WINAPI _ctrl_handler(DWORD ctrl) {
    (void)ctrl;
    g_running = 0;
    return TRUE;
}

int auth_main(int argc, char** argv) {
    (void)argc; (void)argv;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[AuthMain] WSAStartup failed\n");
        return 1;
    }
    SetConsoleCtrlHandler(_ctrl_handler, TRUE);

    printf("[AuthMain] Starting standalone AuthServer on port 3724...\n");

    AuthServer* auth = AuthServer_Create();
    if (!AuthServer_Start(auth, 3724)) {
        printf("[AuthMain] Failed to start: %s\n", AuthServer_LastError(auth));
        AuthServer_Destroy(auth);
        WSACleanup();
        return 1;
    }

    printf("[AuthMain] AuthServer running. Press Ctrl+C to stop.\n");
    while (g_running) Sleep(100);

    AuthServer_Stop(auth);
    AuthServer_Destroy(auth);
    WSACleanup();
    printf("[AuthMain] Stopped.\n");
    return 0;
}
#endif /* _WIN32 */
