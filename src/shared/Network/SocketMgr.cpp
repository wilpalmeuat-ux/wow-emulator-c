#include "shared/NetworkCompat.h"
#include "Network/Socket.h"
#include <errno.h>
#include <cstring>
#include <thread>
#include <vector>

static std::vector<Socket*> g_sockets;
static int g_serverFd = -1;
static bool g_running = false;

int CreateServerSocket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
    if (listen(fd, 100) < 0) { close(fd); return -1; }
    return fd;
}

static void SetNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void SocketMgr_AcceptLoop(int serverFd, std::function<Socket*(int,std::string,uint16_t)> factory) {
    g_running = true;
    while (g_running) {
        struct sockaddr_in caddr{};
        socklen_t clen = sizeof(caddr);
        int csock = accept(serverFd, (sockaddr*)&caddr, &clen);
        if (csock < 0) continue;
        SetNonBlock(csock);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
        uint16_t port = ntohs(caddr.sin_port);
        auto* s = factory(csock, ip, port);
        g_sockets.push_back(s);
    }
}

void SocketMgr_Stop() { g_running = false; }
void SocketMgr_Cleanup() {
    for (auto* s : g_sockets) delete s;
    g_sockets.clear();
}
