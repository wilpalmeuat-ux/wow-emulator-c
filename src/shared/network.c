#include "network.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#define MAX_CLIENTS 1024

typedef struct {
    int fd;
    char addr[48];
    int active;
} ClientSlot;

static int glisten = -1;
static ClientSlot gclients[MAX_CLIENTS];

int net_init(uint16_t port) {
#ifdef _WIN32
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2,2), &ws) != 0) {
        LOG_ERROR("WSAStartup failed");
        return 0;
    }
#endif

    glisten = socket(AF_INET, SOCK_STREAM, 0);
    if (glisten < 0) {
        LOG_ERROR("socket() failed");
        return 0;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(glisten, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(glisten, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#endif

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(glisten, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        LOG_ERROR("bind() failed on port %u", port);
        return 0;
    }

    if (listen(glisten, 64) < 0) {
        LOG_ERROR("listen() failed");
        return 0;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        gclients[i].fd = -1;
        gclients[i].active = 0;
    }

    LOG_INFO("Network listener on port %u", port);
    return 1;
}

void net_poll(void) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(glisten, &rfds);
    int maxfd = glisten;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (gclients[i].active && gclients[i].fd >= 0) {
            FD_SET(gclients[i].fd, &rfds);
            if (gclients[i].fd > maxfd) maxfd = gclients[i].fd;
        }
    }

    struct timeval tv = {0, 50000};
    int n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
    if (n <= 0) return;

    if (FD_ISSET(glisten, &rfds)) {
        struct sockaddr_in ca;
        socklen_t calen = sizeof(ca);
        int fd = accept(glisten, (struct sockaddr*)&ca, &calen);
        if (fd >= 0) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!gclients[i].active) {
                    gclients[i].fd = fd;
                    gclients[i].active = 1;
                    LOG_DEBUG("Client %d connected", i);
                    break;
                }
            }
        }
    }
}

void net_close(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (gclients[i].active && gclients[i].fd >= 0) {
#ifdef _WIN32
            closesocket(gclients[i].fd);
#else
            close(gclients[i].fd);
#endif
            gclients[i].fd = -1;
            gclients[i].active = 0;
        }
    }
    if (glisten >= 0) {
#ifdef _WIN32
        closesocket(glisten);
#else
        close(glisten);
#endif
        glisten = -1;
    }
}

void net_shutdown(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}
