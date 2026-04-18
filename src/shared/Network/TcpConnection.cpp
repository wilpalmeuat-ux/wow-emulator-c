#include "shared/NetworkCompat.h"
#include "Network/Socket.h"

int CreateTcpConnection(const char* ip, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
    return fd;
}

bool SetSocketBlocking(int fd, bool blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) >= 0;
}
