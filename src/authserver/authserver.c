/* authserver.c -- WoW 3.3.5a Authentication Server (Windows only)
 *
 * Handles the login handshake:
 *   1. Client connects to port 3724
 *   2. Server sends AUTH_LOGON_CHALLENGE (SRP6 parameters)
 *   3. Client replies with AUTH_LOGON_PROOF
 *   4. Server verifies and sends realm list
 *
 * Uses WinSock2 + Windows threads.  MySQL for account storage.
 */
#include "authserver/auth_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/* ================================================================
 *  Auth opcodes (login protocol, not world protocol)
 * ================================================================ */
#define AUTH_LOGON_CHALLENGE   0x00
#define AUTH_LOGON_PROOF       0x01
#define AUTH_RECONNECT_CHALLENGE 0x02
#define AUTH_RECONNECT_PROOF   0x03
#define REALM_LIST             0x10

#define AUTH_SUCCESS           0x00
#define AUTH_FAIL_BANNED       0x03
#define AUTH_FAIL_UNKNOWN      0x04
#define AUTH_FAIL_VERSION      0x09
#define AUTH_FAIL_SUSPENDED    0x0C

#define MAX_AUTH_CLIENTS 64

/* ================================================================
 *  Auth client session
 * ================================================================ */
typedef struct AuthClient {
    SOCKET   fd;
    char     ip[64];
    char     username[65];
    uint32_t accountId;
    uint8_t  state;       /* 0=new, 1=challenged, 2=authed */
    uint8_t  recvBuf[4096];
    int      recvLen;
    uint8_t  salt[32];
    uint8_t  sessionKey[40];
    time_t   connectTime;
    struct AuthClient* next;
} AuthClient;

/* ================================================================
 *  AuthServer struct
 * ================================================================ */
struct AuthServer {
    SOCKET            listenFd;
    uint16_t          port;
    volatile int      running;
    HANDLE            acceptThread;
    HANDLE            tickThread;
    CRITICAL_SECTION  lock;
    AuthClient*       clients;
    char              lastError[256];
    /* Realm info */
    char              realmName[64];
    char              realmAddr[128];
    uint16_t          realmPort;
};

/* ================================================================
 *  Helpers
 * ================================================================ */
static void _send_raw(SOCKET fd, const void* data, int len) {
    const char* p = (const char*)data;
    int sent = 0;
    while (sent < len) {
        int n = send(fd, p + sent, len - sent, 0);
        if (n <= 0) break;
        sent += n;
    }
}

/* Build the AUTH_LOGON_CHALLENGE response packet */
static void _send_challenge(AuthClient* c) {
    /* Generate random salt */
    srand((unsigned int)(time(NULL) ^ (uintptr_t)c));
    for (int i = 0; i < 32; i++)
        c->salt[i] = (uint8_t)(rand() & 0xFF);

    /* WoW 3.3.5 auth challenge response format:
     * [0]    = opcode (0x00)
     * [1]    = error  (0x00 = success)
     * [2]    = result (0x00 = success)
     * [3-34] = B (server public, 32 bytes)
     * [35]   = g_len (1)
     * [36]   = g (7)
     * [37]   = N_len (32)
     * [38-69]= N (large prime, 32 bytes)
     * [70-101]= salt (32 bytes)
     * [102-117]= crc_salt (16 bytes)
     * [118]  = security_flags (0)
     */
    uint8_t resp[119];
    memset(resp, 0, sizeof(resp));
    resp[0] = AUTH_LOGON_CHALLENGE;
    resp[1] = 0x00;  /* no error */
    resp[2] = AUTH_SUCCESS;

    /* B - server public key (simplified: random for demo) */
    for (int i = 3; i < 35; i++)
        resp[i] = (uint8_t)(rand() & 0xFF);

    /* g length + g value */
    resp[35] = 1;
    resp[36] = 7;

    /* N length + N (WoW large prime, 32 bytes, little-endian) */
    resp[37] = 32;
    /* Use the standard WoW N value (simplified representation) */
    static const uint8_t wow_N[32] = {
        0xB7, 0x9B, 0x3E, 0x2A, 0x87, 0x82, 0x3C, 0xAB,
        0x8F, 0x5E, 0xBF, 0xBF, 0x8E, 0xB1, 0x01, 0x08,
        0x53, 0x50, 0x06, 0x29, 0x8B, 0x5B, 0xAD, 0xBD,
        0x5B, 0x53, 0xE1, 0x89, 0x5E, 0x64, 0x4B, 0x89
    };
    memcpy(resp + 38, wow_N, 32);

    /* Salt */
    memcpy(resp + 70, c->salt, 32);

    /* CRC salt (16 bytes random) */
    for (int i = 102; i < 118; i++)
        resp[i] = (uint8_t)(rand() & 0xFF);

    /* Security flags */
    resp[118] = 0x00;

    _send_raw(c->fd, resp, sizeof(resp));
    c->state = 1;
    printf("[Auth] Sent challenge to %s (user: %s)\n", c->ip, c->username);
}

/* Build AUTH_LOGON_PROOF response */
static void _send_proof_success(AuthClient* c) {
    /* response: opcode(1) + error(1) + M2(20) + accountFlags(4) + surveyId(4) + unk(2) */
    uint8_t resp[32];
    memset(resp, 0, sizeof(resp));
    resp[0] = AUTH_LOGON_PROOF;
    resp[1] = AUTH_SUCCESS;
    /* M2 = server proof (20 bytes zero for simplified auth) */
    /* accountFlags = 0x00800000 (PRO_PASS enabled) */
    resp[22] = 0x00;
    resp[23] = 0x00;
    resp[24] = 0x80;
    resp[25] = 0x00;
    _send_raw(c->fd, resp, 32);
    c->state = 2;
    printf("[Auth] Login success for '%s' from %s\n", c->username, c->ip);
}

/* Build REALM_LIST response */
static void _send_realm_list(AuthClient* c, const char* realmName,
                             const char* realmAddr, uint16_t realmPort)
{
    /* Build realm address string: "ip:port" */
    char addrStr[192];
    snprintf(addrStr, sizeof(addrStr), "%s:%u", realmAddr, realmPort);

    uint8_t pkt[512];
    int pos = 0;

    /* Packet body (after header) */
    uint8_t body[400];
    int bpos = 0;

    /* padding (4 bytes) */
    body[bpos++] = 0; body[bpos++] = 0;
    body[bpos++] = 0; body[bpos++] = 0;

    /* realm count (2 bytes LE) */
    body[bpos++] = 1; /* 1 realm */
    body[bpos++] = 0;

    /* -- Realm entry -- */
    /* type (1 byte): 0 = Normal, 1 = PvP */
    body[bpos++] = 0;
    /* locked (1 byte) */
    body[bpos++] = 0;
    /* flags (1 byte): 0x20 = recommended */
    body[bpos++] = 0x20;
    /* name (null-terminated string) */
    int nlen = (int)strlen(realmName);
    memcpy(body + bpos, realmName, nlen);
    bpos += nlen;
    body[bpos++] = 0;

    /* address (null-terminated string) */
    int alen = (int)strlen(addrStr);
    memcpy(body + bpos, addrStr, alen);
    bpos += alen;
    body[bpos++] = 0;

    /* population (float, 4 bytes LE) */
    float pop = 0.5f;
    memcpy(body + bpos, &pop, 4);
    bpos += 4;

    /* num characters (1 byte) */
    body[bpos++] = 0;
    /* timezone (1 byte) */
    body[bpos++] = 1;
    /* realm id (1 byte) */
    body[bpos++] = 1;

    /* end padding (2 bytes) */
    body[bpos++] = 0x10;
    body[bpos++] = 0x00;

    /* Header: opcode(1) + size(2 LE) */
    pkt[pos++] = REALM_LIST;
    pkt[pos++] = (uint8_t)(bpos & 0xFF);
    pkt[pos++] = (uint8_t)((bpos >> 8) & 0xFF);
    memcpy(pkt + pos, body, bpos);
    pos += bpos;

    _send_raw(c->fd, pkt, pos);
    printf("[Auth] Sent realm list to %s (%s @ %s)\n", c->username, realmName, addrStr);
}

/* ================================================================
 *  Packet handlers
 * ================================================================ */
static void _handle_logon_challenge(AuthServer* a, AuthClient* c) {
    /* Parse client challenge packet
     * [0]  = opcode (already consumed)
     * [1]  = error
     * [2-3] = size
     * [4-7] = gamename ("WoW\0")
     * [8-10] = version (3.3.5)
     * [11-12]= build (12340)
     * ...
     * [33] = username_len
     * [34+] = username
     */
    if (c->recvLen < 35) return;

    uint8_t* buf = c->recvBuf;
    uint8_t ulen = buf[33];
    if (ulen > 64) ulen = 64;
    if (c->recvLen < (int)(34 + ulen)) return;

    memset(c->username, 0, sizeof(c->username));
    memcpy(c->username, buf + 34, ulen);
    /* Uppercase the username (WoW convention) */
    for (int i = 0; c->username[i]; i++) {
        if (c->username[i] >= 'a' && c->username[i] <= 'z')
            c->username[i] -= 32;
    }

    printf("[Auth] Logon challenge from '%s' @ %s\n", c->username, c->ip);

    /* Consume the packet */
    int consumed = 34 + ulen;
    memmove(c->recvBuf, c->recvBuf + consumed, c->recvLen - consumed);
    c->recvLen -= consumed;

    _send_challenge(c);
}

static void _handle_logon_proof(AuthServer* a, AuthClient* c) {
    /* Client sends: opcode(1) + A(32) + M1(20) + crc(20) + numKeys(1) + securityFlags(1) */
    int needed = 1 + 32 + 20 + 20 + 1 + 1;
    if (c->recvLen < needed) return;

    /* For the simplified auth we accept all proofs */
    int consumed = needed;
    memmove(c->recvBuf, c->recvBuf + consumed, c->recvLen - consumed);
    c->recvLen -= consumed;

    _send_proof_success(c);
}

static void _handle_realm_list(AuthServer* a, AuthClient* c) {
    /* Client sends: opcode(1) + padding(4) */
    int needed = 5;
    if (c->recvLen < needed) return;

    memmove(c->recvBuf, c->recvBuf + needed, c->recvLen - needed);
    c->recvLen -= needed;

    _send_realm_list(c, a->realmName, a->realmAddr, a->realmPort);
}

static void _process_client(AuthServer* a, AuthClient* c) {
    if (c->recvLen < 1) return;
    uint8_t opcode = c->recvBuf[0];

    switch (opcode) {
    case AUTH_LOGON_CHALLENGE:
    case AUTH_RECONNECT_CHALLENGE:
        _handle_logon_challenge(a, c);
        break;
    case AUTH_LOGON_PROOF:
    case AUTH_RECONNECT_PROOF:
        _handle_logon_proof(a, c);
        break;
    case REALM_LIST:
        _handle_realm_list(a, c);
        break;
    default:
        printf("[Auth] Unknown opcode 0x%02X from %s\n", opcode, c->ip);
        /* Skip the byte */
        memmove(c->recvBuf, c->recvBuf + 1, c->recvLen - 1);
        c->recvLen--;
        break;
    }
}

/* ================================================================
 *  Thread functions
 * ================================================================ */
static DWORD WINAPI _auth_accept_thread(LPVOID arg) {
    AuthServer* a = (AuthServer*)arg;

    while (a->running) {
        struct sockaddr_in caddr;
        int caLen = sizeof(caddr);
        SOCKET cfd = accept(a->listenFd, (struct sockaddr*)&caddr, &caLen);

        if (cfd == INVALID_SOCKET) {
            Sleep(50);
            continue;
        }

        /* Set non-blocking */
        u_long mode = 1;
        ioctlsocket(cfd, FIONBIO, &mode);

        AuthClient* c = (AuthClient*)calloc(1, sizeof(AuthClient));
        c->fd = cfd;
        c->connectTime = time(NULL);
        inet_ntop(AF_INET, &caddr.sin_addr, c->ip, sizeof(c->ip));

        EnterCriticalSection(&a->lock);
        c->next = a->clients;
        a->clients = c;
        LeaveCriticalSection(&a->lock);

        printf("[Auth] Client connected from %s\n", c->ip);
    }
    return 0;
}

static DWORD WINAPI _auth_tick_thread(LPVOID arg) {
    AuthServer* a = (AuthServer*)arg;

    while (a->running) {
        Sleep(50);

        EnterCriticalSection(&a->lock);
        AuthClient** pp = &a->clients;
        while (*pp) {
            AuthClient* c = *pp;

            /* Receive data */
            if (c->recvLen < (int)sizeof(c->recvBuf) - 1) {
                int r = recv(c->fd, (char*)(c->recvBuf + c->recvLen),
                             (int)(sizeof(c->recvBuf) - c->recvLen - 1), 0);
                if (r > 0) {
                    c->recvLen += r;
                } else if (r == 0) {
                    /* Disconnect */
                    printf("[Auth] Client %s disconnected\n", c->ip);
                    closesocket(c->fd);
                    *pp = c->next;
                    free(c);
                    continue;
                }
                /* r < 0: WSAEWOULDBLOCK is OK, real error would show up eventually */
            }

            /* Process packets */
            if (c->recvLen > 0)
                _process_client(a, c);

            /* Timeout (60 seconds with no auth) */
            if (c->state < 2 && (time(NULL) - c->connectTime) > 60) {
                printf("[Auth] Client %s timed out\n", c->ip);
                closesocket(c->fd);
                *pp = c->next;
                free(c);
                continue;
            }

            pp = &c->next;
        }
        LeaveCriticalSection(&a->lock);
    }
    return 0;
}

/* ================================================================
 *  Public API
 * ================================================================ */
AuthServer* AuthServer_Create(void) {
    AuthServer* a = (AuthServer*)calloc(1, sizeof(AuthServer));
    a->listenFd = INVALID_SOCKET;
    InitializeCriticalSection(&a->lock);
    strncpy(a->realmName, "WoW 3.3.5a Emulator", sizeof(a->realmName) - 1);
    strncpy(a->realmAddr, "127.0.0.1", sizeof(a->realmAddr) - 1);
    a->realmPort = 8085;
    return a;
}

void AuthServer_Destroy(AuthServer* a) {
    if (!a) return;
    AuthServer_Stop(a);
    DeleteCriticalSection(&a->lock);
    free(a);
}

bool AuthServer_Start(AuthServer* a, uint16_t port) {
    a->port = port;

    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        snprintf(a->lastError, sizeof(a->lastError), "socket() failed: %d", WSAGetLastError());
        return false;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        snprintf(a->lastError, sizeof(a->lastError),
                 "bind() failed on port %u: %d", port, WSAGetLastError());
        closesocket(fd);
        return false;
    }

    if (listen(fd, 32) == SOCKET_ERROR) {
        snprintf(a->lastError, sizeof(a->lastError), "listen() failed: %d", WSAGetLastError());
        closesocket(fd);
        return false;
    }

    /* Set non-blocking */
    u_long nbmode = 1;
    ioctlsocket(fd, FIONBIO, &nbmode);

    a->listenFd = fd;
    a->running = 1;

    a->acceptThread = CreateThread(NULL, 0, _auth_accept_thread, a, 0, NULL);
    a->tickThread   = CreateThread(NULL, 0, _auth_tick_thread, a, 0, NULL);

    return true;
}

void AuthServer_Stop(AuthServer* a) {
    if (!a || !a->running) return;
    a->running = 0;

    if (a->listenFd != INVALID_SOCKET) {
        closesocket(a->listenFd);
        a->listenFd = INVALID_SOCKET;
    }

    if (a->acceptThread) { WaitForSingleObject(a->acceptThread, 3000); CloseHandle(a->acceptThread); a->acceptThread = NULL; }
    if (a->tickThread)   { WaitForSingleObject(a->tickThread, 3000);   CloseHandle(a->tickThread);   a->tickThread = NULL; }

    /* Clean up clients */
    EnterCriticalSection(&a->lock);
    while (a->clients) {
        AuthClient* c = a->clients;
        a->clients = c->next;
        closesocket(c->fd);
        free(c);
    }
    LeaveCriticalSection(&a->lock);
}

void AuthServer_Update(AuthServer* a) {
    (void)a; /* Tick threads handle updates */
}

const char* AuthServer_LastError(AuthServer* a) {
    return a ? a->lastError : "null";
}
