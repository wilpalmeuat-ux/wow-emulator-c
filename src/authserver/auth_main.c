/* Auth Server main (Windows)
 * WinSock2 + Windows API only. No POSIX.
 */
#include "shared/NetworkCompat.h"
#include <shared/wow_packet.h>
#include <scripting/wss_wow_hooks.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define AUTH_SERVER_PORT 3724
#define MAX_CLIENTS 64
#define BUFFER_SIZE 32768

typedef struct {
    socket_t fd;
    char ip[32];
    uint8_t recv_buf[BUFFER_SIZE];
    int recv_len;
    uint8_t send_buf[BUFFER_SIZE];
    int send_len;
    bool authenticated;
    uint8_t account_id[40];
    char username[32];
} AuthClient;

static AuthClient clients[MAX_CLIENTS];
static int num_clients = 0;
static volatile int running = 1;

static BOOL WINAPI _sig(DWORD ctrl) { (void)ctrl; running = 0; return TRUE; }

static socket_t create_listen_socket(int port) {
    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) return INVALID_SOCKET;
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { closesocket(fd); return INVALID_SOCKET; }
    listen(fd, 16);
    return fd;
}

static void send_packet(AuthClient* c, uint32_t opcode, const uint8_t* data, int len) {
    uint8_t hdr[6];
    hdr[0] = (uint8_t)((len + 4) & 0xFF);
    hdr[1] = (uint8_t)((len + 4) >> 8);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);
    hdr[4] = (uint8_t)((opcode >> 16) & 0xFF);
    hdr[5] = (uint8_t)((opcode >> 24) & 0xFF);
    memcpy(c->send_buf, hdr, 6);
    if (data && len > 0) memcpy(c->send_buf + 6, data, len);
    c->send_len = len + 6;
}

static void handle_auth_challenge(AuthClient* c, WowBuffer* rbuf) {
    (void)rbuf;
    uint8_t response[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    send_packet(c, 0x00, response, sizeof(response));
}

static void handle_auth_proof(AuthClient* c, WowBuffer* rbuf) {
    (void)rbuf;
    uint8_t response[4] = {0x00, 0x00, 0x00, 0x00};
    send_packet(c, 0x01, response, sizeof(response));
    c->authenticated = true;
}

static void handle_realm_list(AuthClient* c, WowBuffer* rbuf) {
    (void)rbuf;
    char realm_data[128] = "Realm 1\x00 127.0.0.1\x00 127.0.0.1\x00 0\x00 1\x00 0\x00 1\x00";
    int len = (int)strlen(realm_data) + 1;
    send_packet(c, 0x10, (uint8_t*)realm_data, len);
}

static void process_packet(AuthClient* c, uint32_t opcode, uint8_t* data, int len) {
    WowBuffer rbuf; WowBuffer_Init(&rbuf, data, len);
    switch (opcode) {
        case 0x00: handle_auth_challenge(c, &rbuf); break;
        case 0x01: handle_auth_proof(c, &rbuf); break;
        case 0x10: handle_realm_list(c, &rbuf); break;
        default: printf("[AuthServer] Unknown opcode: 0x%X\n", opcode); break;
    }
}

static bool read_packet(socket_t fd, uint8_t* buf, int* out_len) {
    uint8_t hdr[4];
    int n = recv(fd, (char*)hdr, 4, MSG_PEEK);
    if (n == 0) return false;
    if (n < 4) return true;
    uint16_t size = (uint16_t)(hdr[0] | (hdr[1] << 8));
    int total = size + 4;
    int r = recv(fd, (char*)buf, total, 0);
    if (r > 0) { *out_len = r; return true; }
    return false;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    SetConsoleCtrlHandler(_sig, TRUE);

    printf("=== WoW 3.3.5a Auth Server ===\n");
    printf("Listening on port %d\n", AUTH_SERVER_PORT);

    socket_t listen_fd = create_listen_socket(AUTH_SERVER_PORT);
    if (listen_fd == INVALID_SOCKET) { printf("Failed to create socket\n"); return 1; }

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        socket_t fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (fd != INVALID_SOCKET && num_clients < MAX_CLIENTS) {
            AuthClient* c = &clients[num_clients++];
            memset(c, 0, sizeof(*c));
            c->fd = fd;
            snprintf(c->ip, sizeof(c->ip), "%s", inet_ntoa(client_addr.sin_addr));
            printf("[AuthServer] Client connected: %s\n", c->ip);
        }

        for (int i = 0; i < num_clients; i++) {
            AuthClient* c = &clients[i];
            int len = 0;
            if (read_packet(c->fd, c->recv_buf, &len)) {
                uint32_t opcode = (uint32_t)(c->recv_buf[2] | (c->recv_buf[3] << 8) | (c->recv_buf[4] << 16) | (c->recv_buf[5] << 24));
                process_packet(c, opcode, c->recv_buf + 6, len - 6);
            }
            if (c->send_len > 0) {
                send(c->fd, (const char*)c->send_buf, (size_t)c->send_len, 0);
                c->send_len = 0;
            }
        }
        Sleep(10);
    }

    for (int i = 0; i < num_clients; i++) closesocket(clients[i].fd);
    if (listen_fd != INVALID_SOCKET) closesocket(listen_fd);
    printf("[AuthServer] Shutdown complete\n");
    return 0;
}