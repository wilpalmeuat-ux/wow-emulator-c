/* Network.c — Cross-platform network layer using NetworkCompat.h */
#include "shared/Network.h"
#include "shared/Logging.h"
#include "shared/NetworkCompat.h"
#include <errno.h>
#include <string.h>

#ifdef _WIN32
#define close(fd) closesocket(fd)
#define usleep(us) Sleep((us) / 1000)
#endif

ClientSocket* ClientSocket_New(int fd, struct sockaddr_in* addr) {
    ClientSocket* cs = calloc(1, sizeof(ClientSocket));
    cs->fd = fd;
    if (addr) memcpy(&cs->addr, addr, sizeof(struct sockaddr_in));
    cs->alive = true;
    pthread_mutex_init(&cs->writeLock, NULL);
    return cs;
}

void ClientSocket_Delete(ClientSocket* cs) {
    if (!cs) return;
    if (cs->fd != SOCKET_INVALID) closesocket(cs->fd);
    pthread_mutex_destroy(&cs->writeLock);
    free(cs);
}

void ClientSocket_SendRaw(ClientSocket* cs, const uint8* data, uint32 len) {
    if (!cs || !cs->alive || !data) return;
    pthread_mutex_lock(&cs->writeLock);
    uint32 sent = 0;
    while (sent < len) {
        int n = send(cs->fd, (const char*)(data + sent), (int)(len - sent), 0);
        if (n <= 0) { cs->alive = false; pthread_mutex_unlock(&cs->writeLock); return; }
        sent += n;
    }
    pthread_mutex_unlock(&cs->writeLock);
}

void ClientSocket_Send(ClientSocket* cs, uint16 opcode, ByteBuffer* data) {
    if (!cs || !cs->alive) return;
    uint8 header[6];
    uint32 size = 2 + 2 + (data ? ByteBuffer_Size(data) : 0);
    header[0] = (uint8)(size & 0xFF);
    header[1] = (uint8)((size >> 8) & 0xFF);
    header[2] = (uint8)(opcode & 0xFF);
    header[3] = (uint8)((opcode >> 8) & 0xFF);
    header[4] = 0; header[5] = 0;
    ClientSocket_SendRaw(cs, header, 6);
    if (data && ByteBuffer_Size(data) > 0)
        ClientSocket_SendRaw(cs, ByteBuffer_Data(data), ByteBuffer_Size(data));
}

void ClientSocket_Receive(ClientSocket* cs) {
    if (!cs || !cs->alive) return;
    int r = recv(cs->fd, (char*)(cs->recvBuf + cs->recvLen), (int)(sizeof(cs->recvBuf) - cs->recvLen - 1), 0);
    if (r <= 0) { cs->alive = false; return; }
    cs->recvLen += r;
}

bool ClientSocket_Tick(ClientSocket* cs) {
    if (!cs || !cs->alive) return false;
    ClientSocket_Receive(cs);
    return cs->alive;
}

const char* ClientSocket_GetIP(ClientSocket* cs) {
    static char ip[INET_ADDRSTRLEN];
    if (cs) inet_ntop(AF_INET, &cs->addr.sin_addr, ip, sizeof(ip));
    return ip;
}

/* ── Server ── */
static void* _accept_loop(void* arg) {
    Server* s = arg;
    while (s->running) {
        struct sockaddr_in caddr;
        socklen_t caLen = sizeof(caddr);
        int cfd = accept(s->listenFd, (struct sockaddr*)&caddr, &caLen);
        if (SOCKET_IS_ERROR(cfd)) { usleep(100000); continue; }
        /* set non-blocking */
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(cfd, FIONBIO, &mode);
#else
        fcntl(cfd, F_SETFL, O_NONBLOCK);
#endif
        ClientSocket* cs = ClientSocket_New(cfd, &caddr);
        pthread_mutex_lock(&s->clientsLock);
        cs->next = s->clients; s->clients = cs;
        pthread_mutex_unlock(&s->clientsLock);
        LOG_INFO("Client connected from %s", ClientSocket_GetIP(cs));
    }
    return NULL;
}

static void* _tick_loop(void* arg) {
    Server* s = arg;
    while (s->running) {
        usleep(50000);
        pthread_mutex_lock(&s->clientsLock);
        for (ClientSocket* cs = s->clients; cs; cs = cs->next) {
            if (!cs->alive) continue;
            while (cs->recvLen >= 6) {
                uint32 size = cs->recvBuf[0] | ((uint32)cs->recvBuf[1] << 8);
                uint32 opcode = cs->recvBuf[2] | ((uint32)cs->recvBuf[3] << 8);
                uint32 total = 2 + 2 + size;
                if (cs->recvLen < total) break;
                ByteBuffer* body = ByteBuffer_New(size + 1);
                if (size > 0) { memcpy(body->data, cs->recvBuf + 6, size); body->wpos = size; }
                memmove(cs->recvBuf, cs->recvBuf + total, cs->recvLen - total);
                cs->recvLen -= total;
                if (opcode < OPCODE_TABLE_SIZE && s->handlers[opcode]) {
                    s->handlers[opcode](cs->recvBuf, body, s->context);
                } else {
                    LOG_DEBUG("Unhandled opcode 0x%04X", opcode);
                }
                ByteBuffer_Delete(body);
            }
        }
        pthread_mutex_unlock(&s->clientsLock);
    }
    return NULL;
}

Server* Server_New(uint16 port, void* ctx) {
    Server* s = calloc(1, sizeof(Server));
    s->port = port;
    s->context = ctx;
    s->running = false;
    s->listenFd = SOCKET_INVALID;
    pthread_mutex_init(&s->clientsLock, NULL);
    return s;
}

void Server_Delete(Server* s) {
    if (!s) return;
    Server_Stop(s);
    pthread_mutex_destroy(&s->clientsLock);
    free(s);
}

bool Server_Start(Server* s) {
    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET_IS_ERROR(fd)) { LOG_ERROR("socket() failed"); return false; }
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(s->port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() failed on port %u", s->port);
        closesocket(fd);
        return false;
    }
    if (listen(fd, 64) < 0) { LOG_ERROR("listen() failed"); closesocket(fd); return false; }
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
    s->listenFd = fd;
    s->running = true;
    pthread_create(&s->acceptThread, NULL, _accept_loop, s);
    pthread_create(&s->tickThread, NULL, _tick_loop, s);
    LOG_INFO("Server listening on port %u", s->port);
    return true;
}

void Server_Stop(Server* s) {
    if (!s->running) return;
    s->running = false;
    if (s->listenFd != SOCKET_INVALID) { closesocket(s->listenFd); s->listenFd = SOCKET_INVALID; }
    pthread_join(s->acceptThread, NULL);
    pthread_join(s->tickThread, NULL);
    pthread_mutex_lock(&s->clientsLock);
    for (ClientSocket* cs = s->clients; cs; ) { ClientSocket* n = cs->next; ClientSocket_Delete(cs); cs = n; }
    s->clients = NULL;
    pthread_mutex_unlock(&s->clientsLock);
    LOG_INFO("Server stopped");
}

void Server_RegisterHandler(Server* s, uint16 opcode, PacketHandler fn) {
    if (opcode < OPCODE_TABLE_SIZE) s->handlers[opcode] = fn;
}

void Server_Broadcast(Server* s, uint16 opcode, ByteBuffer* buf) {
    pthread_mutex_lock(&s->clientsLock);
    for (ClientSocket* cs = s->clients; cs; cs = cs->next) {
        if (cs->alive) ClientSocket_Send(cs, opcode, buf);
    }
    pthread_mutex_unlock(&s->clientsLock);
}
