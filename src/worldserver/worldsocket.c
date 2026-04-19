/* worldsocket.c -- WorldSocket utilities for WoW 3.3.5a emulator (Windows)
 *
 * Low-level send/recv helpers for world server client sockets.
 * The actual packet dispatch lives in worldsession.c.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

/* ================================================================
 *  Send a raw world packet to a client
 * ================================================================ */
void WorldSocket_SendPacket(WorldSocket* sock, uint16_t opcode,
                            const void* data, int dataLen)
{
    if (!sock || sock->dead) return;

    /* WoW 3.3.5 server-to-client header:
     * Size (2 bytes, big-endian) = opcode_size(2) + data_size
     * Opcode (2 bytes, little-endian)
     */
    uint16_t totalSize = (uint16_t)(2 + dataLen);
    uint8_t hdr[4];
    hdr[0] = (uint8_t)((totalSize >> 8) & 0xFF);
    hdr[1] = (uint8_t)(totalSize & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);

    int sent = send(sock->fd, (const char*)hdr, 4, 0);
    if (sent <= 0) {
        sock->dead = true;
        return;
    }

    if (data && dataLen > 0) {
        sent = send(sock->fd, (const char*)data, dataLen, 0);
        if (sent <= 0)
            sock->dead = true;
    }
}

/* ================================================================
 *  Send a system chat message to a single client
 * ================================================================ */
void WorldSocket_SendSystemMessage(WorldSocket* sock, const char* msg)
{
    if (!sock || !msg || sock->dead) return;

    size_t msgLen = strlen(msg);
    size_t bodyLen = 1 + 4 + 8 + 4 + 8 + 4 + msgLen + 1 + 1;
    uint8_t* body = (uint8_t*)calloc(1, bodyLen);
    int pos = 0;

    /* ChatType: 0x09 = SYSTEM */
    body[pos++] = 0x09;
    /* Language (4 bytes) = 0 */
    memset(body + pos, 0, 4); pos += 4;
    /* SenderGUID (8 bytes) = 0 */
    memset(body + pos, 0, 8); pos += 8;
    /* Flags (4 bytes) */
    memset(body + pos, 0, 4); pos += 4;
    /* TargetGUID (8 bytes) = 0 */
    memset(body + pos, 0, 8); pos += 8;
    /* Message length (4 bytes) */
    uint32_t ml = (uint32_t)(msgLen + 1);
    memcpy(body + pos, &ml, 4); pos += 4;
    /* Message (null-terminated) */
    memcpy(body + pos, msg, msgLen); pos += (int)msgLen;
    body[pos++] = 0;
    /* ChatTag */
    body[pos++] = 0;

    WorldSocket_SendPacket(sock, (uint16_t)SMSG_MESSAGECHAT, body, pos);
    free(body);
}

/* ================================================================
 *  Check if socket is alive
 * ================================================================ */
bool WorldSocket_IsAlive(WorldSocket* sock) {
    return sock && !sock->dead;
}

/* ================================================================
 *  Disconnect a client gracefully
 * ================================================================ */
void WorldSocket_Disconnect(WorldSocket* sock) {
    if (!sock) return;
    sock->dead = true;
    closesocket(sock->fd);
}
