/* world_socket.c — WoW 3.3.5 client connection handler */
#include "shared/NetworkCompat.h"
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

static WorldServer* g_ws = NULL;

void WorldSocket_SetServer(WorldServer* ws) { g_ws = ws; }

/* ── Accept a new client ── */
WorldSocket* WorldSocket_Accept(WorldServer* ws) {
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int fd = accept(ws->socketFd, (struct sockaddr*)&caddr, &len);
    if (fd < 0) return NULL;

    WorldSocket* s = calloc(1, sizeof(WorldSocket));
    s->fd = fd;
    s->accountId = 0;
    s->authenticated = false;
    s->dead = false;
    s->guid = 0;
    strncpy(s->host, inet_ntoa(caddr.sin_addr), sizeof(s->host)-1);

    pthread_mutex_lock(&ws->lock);
    s->next = ws->clients;
    ws->clients = s;
    pthread_mutex_unlock(&ws->lock);

    printf("[WorldSocket] Client connected from %s (fd=%d)\n", s->host, fd);
    return s;
}

void WorldSocket_Close(WorldSocket* s) {
    if (!s || s->dead) return;
    s->dead = true;
    if (s->fd >= 0) { close(s->fd); s->fd = -1; }
    if (s->player && s->player->isInWorld) {
        s->player->isInWorld = false;
        ScriptEngine_ExecuteWordEvent(g_ws->scriptEngine, "player", "onLogout", s->player);
    }
    printf("[WorldSocket] Client %s disconnected\n", s->host);
}

/* ── Non-blocking recv ── */
int WorldSocket_Recv(WorldSocket* s, void* buf, size_t len) {
    if (s->fd < 0) return -1;
    int r = recv(s->fd, buf, len, 0);
    if (r == 0) { WorldSocket_Close(s); return -1; }
    if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) { WorldSocket_Close(s); return -1; }
    return r;
}

/* ── Non-blocking send ── */
bool WorldSocket_Send(WorldSocket* s, const void* data, size_t len) {
    if (s->fd < 0 || s->dead) return false;
    size_t sent = 0;
    while (sent < len) {
        int r = send(s->fd, (const uint8_t*)data + sent, len - sent, 0);
        if (r < 0) { WorldSocket_Close(s); return false; }
        sent += r;
    }
    return true;
}

/* ── Send a raw WoW packet ── */
static bool _send_pkt(WorldSocket* s, uint16_t opcode, const void* body, size_t bodyLen) {
    uint8_t header[4];
    size_t total = 4 + bodyLen;
    /* WoW uses a 4-byte header: size (3 bytes, MSB=0) + opcode (2 bytes) */
    header[0] = (uint8_t)((total) & 0xFF);
    header[1] = (uint8_t)((total >> 8) & 0xFF);
    header[2] = (uint8_t)((total >> 16) & 0xFF);
    header[3] = (uint8_t)((opcode) & 0xFF);
    header[2] |= 0x80; /* compression flag — simplified; real impl uses zlib */

    if (!WorldSocket_Send(s, header, 4)) return false;
    if (bodyLen > 0 && body) return WorldSocket_Send(s, body, bodyLen);
    return true;
}

#define SEND_PKT(op, body, len) _send_pkt(s, op, body, len)

/* ── Send auth challenge ── */
static void _send_auth_challenge(WorldSocket* s) {
    struct { uint32_t n; uint32_t seed; uint8_t d[32]; } pkt = {0};
    pkt.n = htonl(0x0C); /* serverId */
    pkt.seed = htonl((uint32_t)time(NULL));
    memset(pkt.d, 0, 32);
    /* Build number 12340 = 3.3.5a */
    pkt.d[0] = 0x38; pkt.d[1] = 0x30; pkt.d[2] = 0x00; pkt.d[3] = 0x00;
    SEND_PKT(SMSG_AUTH_CHALLENGE, &pkt, sizeof(pkt));
}

/* ── Handle incoming packets ── */
void WorldSocket_HandlePacket(WorldSocket* s, uint16_t opcode, const uint8_t* body, size_t bodyLen) {
    switch (opcode) {

    case CMSG_AUTH_SESSION: {
        /* 3.3.5 auth session — just ack success for demo */
        uint8_t resp[5] = { 0x0C, 0,0,0,0 }; /* AUTH_OK = 0x0C */
        resp[1] = 0; resp[2] = 0; resp[3] = 0; resp[4] = 0;
        _send_pkt(s, SMSG_AUTH_RESPONSE, resp, sizeof(resp));
        s->authenticated = true;
        printf("[WorldSocket] Client authenticated\n");

        /* Send character enum (empty for now) */
        uint8_t charEnum[6] = { 0 }; /* count=0 */
        SEND_PKT(SMSG_CHAR_ENUM, charEnum, sizeof(charEnum));
        break;
    }

    case CMSG_CHAR_ENUM: {
        uint8_t resp[6] = { 0 }; /* zero characters */
        SEND_PKT(SMSG_CHAR_ENUM, resp, sizeof(resp));
        break;
    }

    case CMSG_CHAR_CREATE: {
        /* Create character — just return success */
        uint8_t resp[4] = { 0x2E, 0, 0, 0 }; /* CHAR_CREATE_SUCCESS */
        SEND_PKT(SMSG_CHAR_CREATE, resp, sizeof(resp));
        break;
    }

    case CMSG_PLAYER_LOGIN: {
        if (bodyLen < 8) break;
        ObjectGuid guid = *(ObjectGuid*)body;
        s->guid = guid;
        s->authenticated = true;

        /* Allocate a demo player */
        Player* p = calloc(1, sizeof(Player));
        p->guid = guid;
        p->accountId = s->accountId;
        p->level = 80;
        p->mapId = 0;
        p->position[0] = -8949.0f; p->position[1] = -132.0f; p->position[2] = 83.0f;
        p->orientation = 0.0f;
        p->isInWorld = true;
        p->session = s;
        s->player = p;

        /* Send LOGIN_VERIFY_WORLD */
        struct { uint32_t mapId; float x; float y; float z; float o; uint32_t guid; } loginPkt;
        loginPkt.mapId = htonl(0);
        loginPkt.x = -8949.0f; loginPkt.y = -132.0f; loginPkt.z = 83.0f; loginPkt.o = 0.0f;
        loginPkt.guid = htonl((uint32_t)guid);
        SEND_PKT(SMSG_LOGIN_VERIFY_WORLD, &loginPkt, sizeof(loginPkt));

        /* Send initial update block (full create) */
        uint8_t updateBlock[512];
        memset(updateBlock, 0, sizeof(updateBlock));
        SEND_PKT(SMSG_UPDATE_OBJECT, updateBlock, 64);

        ScriptEngine_ExecuteWordEvent(g_ws->scriptEngine, "player", "onLogin", p);
        printf("[WorldSocket] Player %llu logged in\n", (unsigned long long)guid);
        break;
    }

    case CMSG_GOSSIP_HELLO: {
        /* Trigger gossip event on the target unit */
        ObjectGuid guid = 0;
        if (bodyLen >= 8) guid = *(ObjectGuid*)body;
        Unit* target = WorldServer_FindUnit(g_ws, guid);
        if (target) {
            ScriptEngine_ExecuteWordEvent(g_ws->scriptEngine, "npc", "onGossip", target);
        }
        /* Default gossip menu */
        uint8_t gossip[64] = {0};
        SEND_PKT(SMSG_GOSSIP_MESSAGE, gossip, sizeof(gossip));
        break;
    }

    case CMSG_CHAT_IGNORED_ACCOUNT: {
        /* Player said something — trigger onSay event */
        if (s->player && bodyLen > 5) {
            const char* msg = (const char*)(body + 4);
            ScriptContext ctx = {0};
            ctx.engine = g_ws->scriptEngine;
            ctx.userData = s->player;
            strncpy(ctx.args[0].text, msg, MAX_TOKEN_LEN-1);
            ctx.args[0].type = T_STRING;
            ScriptEngine_ExecuteWordEvent(g_ws->scriptEngine, "player", "onSay", s->player);
        }
        break;
    }

    case CMSG_LOGOUT_REQUEST: {
        uint8_t resp[1] = { 0x01 }; /* LOGOUT_COMPLETE */
        SEND_PKT(0x00DC, resp, sizeof(resp)); /* SMSG_LOGOUT_COMPLETE */
        if (s->player) {
            ScriptEngine_ExecuteWordEvent(g_ws->scriptEngine, "player", "onLogout", s->player);
            s->player->isInWorld = false;
        }
        break;
    }

    default:
        break;
    }
}

/* ── Per-socket update (called each world tick) ── */
void WorldSocket_Update(WorldSocket* s, uint32_t diffMs) {
    if (s->dead) return;

    /* Read incoming data */
    uint8_t buf[8192];
    int r = WorldSocket_Recv(s, buf, sizeof(buf));
    if (r <= 0) return;

    /* Simple 4-byte header parse: [size(3BE)][opcode(2BE)] */
    size_t bufPos = 0;
    while (bufPos + 4 <= (size_t)r) {
        uint32_t size = (buf[bufPos] | (buf[bufPos+1]<<8) | ((buf[bufPos+2]&0x7F)<<16));
        uint16_t opcode = buf[bufPos+2] | (buf[bufPos+3]<<8);
        size_t pktSize = size - 4;
        if (bufPos + 4 + pktSize > (size_t)r) break;
        uint8_t* body = (pktSize > 0) ? &buf[bufPos+4] : NULL;
        WorldSocket_HandlePacket(s, opcode, body, pktSize);
        bufPos += 4 + pktSize;
    }
    (void)diffMs;
}
