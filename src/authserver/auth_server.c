/* auth_server.c — WoW 3.3.5a minimal auth server */
#include "shared/NetworkCompat.h"
#include "authserver/auth_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>

#define LOG printf

typedef struct {
    int fd;
    char host[64];
    time_t connectTime;
    uint8_t cmd;
    uint32_t build;
} AuthClient;

struct AuthServer {
    uint16_t     port;
    bool         running;
    int          socketFd;
    AuthClient*  clients[256];
    int          clientCount;
    pthread_t     acceptThread;
    pthread_mutex_t lock;
    char         error[256];
};

static AuthServer* g_auth = NULL;

AuthServer* AuthServer_Create(void) {
    AuthServer* a = calloc(1, sizeof(AuthServer));
    a->port = 3724;
    a->running = false;
    a->socketFd = -1;
    pthread_mutex_init(&a->lock, NULL);
    g_auth = a;
    return a;
}

void AuthServer_Destroy(AuthServer* a) {
    if (!a) return;
    AuthServer_Stop(a);
    pthread_mutex_destroy(&a->lock);
    free(a);
}

bool AuthServer_Start(AuthServer* a, uint16_t port) {
    if (port) a->port = port;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(a->port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return false; }
    if (listen(fd, 16) < 0) { close(fd); return false; }
    a->socketFd = fd;
    a->running = true;
    LOG("[AuthServer] Listening on port %u\n", a->port);
    return true;
}

void AuthServer_Stop(AuthServer* a) {
    if (!a->running) return;
    a->running = false;
    if (a->socketFd >= 0) { close(a->socketFd); a->socketFd = -1; }
    LOG("[AuthServer] Stopped\n");
}

void AuthServer_Update(AuthServer* a) {
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int fd = accept(a->socketFd, (struct sockaddr*)&caddr, &len);
    if (fd < 0) return;

    AuthClient* c = calloc(1, sizeof(AuthClient));
    c->fd = fd;
    snprintf(c->host, sizeof(c->host), "%s", inet_ntoa(caddr.sin_addr));
    c->connectTime = time(NULL);

    pthread_mutex_lock(&a->lock);
    if (a->clientCount < 256) a->clients[a->clientCount++] = c;
    pthread_mutex_unlock(&a->lock);

    LOG("[AuthServer] Client from %s\n", c->host);

    /* Respond with CMD_AUTH_LOGON_CHALLENGE (0x00) */
    uint8_t resp[4] = { 0x00, 0x00, 0x00, 0x00 };
    send(fd, resp, 4, 0);
}

const char* AuthServer_LastError(AuthServer* a) { return a->error; }
