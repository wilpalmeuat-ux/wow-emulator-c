/* worldserver.c -- WoW 3.3.5a WorldServer core implementation (Windows only)
 *
 * Manages all world-side state: client connections, creature/GO spawns,
 * maps, the scripting engine, and the main update loop.
 */
#include "worldserver/world_server.h"
#include "scripting/script_engine.h"
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
#endif

/* ================================================================
 *  GUID allocator (simple incrementing)
 * ================================================================ */
static uint32_t _next_guid(WorldServer* ws) {
    return ++ws->nextGuid;
}

/* ================================================================
 *  Unit field helpers (used by scripting)
 * ================================================================ */
void Unit_SetUInt32(void* unit_ptr, int field, uint32_t val) {
    if (!unit_ptr) return;
    Unit* u = (Unit*)unit_ptr;
    if (field >= 0 && field < UNIT_END)
        u->fields[field] = (uint64_t)val;
}

uint32_t Unit_GetUInt32(void* unit_ptr, int field) {
    if (!unit_ptr) return 0;
    Unit* u = (Unit*)unit_ptr;
    if (field >= 0 && field < UNIT_END)
        return (uint32_t)u->fields[field];
    return 0;
}

/* ================================================================
 *  WorldServer_Create
 * ================================================================ */
WorldServer* WorldServer_Create(uint16_t port) {
    WorldServer* ws = (WorldServer*)calloc(1, sizeof(WorldServer));
    if (!ws) return NULL;

    ws->port = port;
    ws->running = false;
    ws->socketFd = INVALID_SOCKET;
    ws->startTime = time(NULL);
    ws->maxPlayers = MAX_PLAYERS;
    ws->nextGuid = 1000; /* Reserve low GUIDs */

    InitializeCriticalSection(&ws->lock);

    /* Create script engine */
    ws->scriptEngine = ScriptEngine_Create(ws);

    /* Init default maps */
    const char* mapNames[] = {
        "Eastern Kingdoms", "Kalimdor", "Outland", "Northrend"
    };
    for (int i = 0; i < 4 && i < 100; i++) {
        ws->maps[i].id = (MapId)i;
        strncpy(ws->maps[i].name, mapNames[i], 63);
        ws->maps[i].running = false;
    }
    ws->mapCount = 4;

    printf("[World] WorldServer created on port %u\n", port);
    return ws;
}

/* ================================================================
 *  Accept thread -- listens for new client connections
 * ================================================================ */
static DWORD WINAPI _ws_accept_thread(LPVOID arg) {
    WorldServer* ws = (WorldServer*)arg;

    while (ws->running) {
        struct sockaddr_in caddr;
        int caLen = sizeof(caddr);
        SOCKET cfd = accept(ws->socketFd, (struct sockaddr*)&caddr, &caLen);

        if (cfd == INVALID_SOCKET) {
            Sleep(50);
            continue;
        }

        /* Set non-blocking */
        u_long mode = 1;
        ioctlsocket(cfd, FIONBIO, &mode);

        /* Disable Nagle for low latency */
        int opt = 1;
        setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));

        WorldSocket* sock = (WorldSocket*)calloc(1, sizeof(WorldSocket));
        sock->fd = cfd;
        inet_ntop(AF_INET, &caddr.sin_addr, sock->host, sizeof(sock->host));

        EnterCriticalSection(&ws->lock);
        sock->next = ws->clients;
        ws->clients = sock;
        LeaveCriticalSection(&ws->lock);

        printf("[World] Client connected from %s\n", sock->host);

        /* Send SMSG_AUTH_CHALLENGE
         * Format: opcode(2) + size(2) + one(4) + seed(4) + seed1(16) + seed2(16)
         */
        uint8_t challenge[46];
        memset(challenge, 0, sizeof(challenge));
        /* Size (big-endian, includes opcode) */
        uint16_t pktSize = 44;
        challenge[0] = (uint8_t)((pktSize >> 8) & 0xFF);
        challenge[1] = (uint8_t)(pktSize & 0xFF);
        /* Opcode (little-endian) */
        challenge[2] = (uint8_t)(SMSG_AUTH_CHALLENGE & 0xFF);
        challenge[3] = (uint8_t)((SMSG_AUTH_CHALLENGE >> 8) & 0xFF);
        /* One = 1 */
        challenge[4] = 1;
        /* Server seed (random) */
        srand((unsigned int)(time(NULL) ^ (uintptr_t)sock));
        for (int i = 8; i < 12; i++)
            challenge[i] = (uint8_t)(rand() & 0xFF);
        /* Two random seeds (16 bytes each) */
        for (int i = 12; i < 44; i++)
            challenge[i] = (uint8_t)(rand() & 0xFF);

        send(cfd, (const char*)challenge, 46, 0);
    }
    return 0;
}

/* ================================================================
 *  WorldServer_Start
 * ================================================================ */
bool WorldServer_Start(WorldServer* ws) {
    if (!ws) return false;

    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        printf("[World] socket() failed: %d\n", WSAGetLastError());
        return false;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ws->port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[World] bind() failed on port %u: %d\n", ws->port, WSAGetLastError());
        closesocket(fd);
        return false;
    }

    if (listen(fd, 64) == SOCKET_ERROR) {
        printf("[World] listen() failed: %d\n", WSAGetLastError());
        closesocket(fd);
        return false;
    }

    /* Non-blocking */
    u_long nbmode = 1;
    ioctlsocket(fd, FIONBIO, &nbmode);

    ws->socketFd = fd;
    ws->running = true;

    /* Start accept thread */
    ws->acceptThread = CreateThread(NULL, 0, _ws_accept_thread, ws, 0, NULL);
    if (!ws->acceptThread) {
        printf("[World] Failed to create accept thread\n");
        closesocket(fd);
        ws->running = false;
        return false;
    }

    return true;
}

/* ================================================================
 *  WorldServer_Stop
 * ================================================================ */
void WorldServer_Stop(WorldServer* ws) {
    if (!ws || !ws->running) return;
    ws->running = false;

    if (ws->socketFd != INVALID_SOCKET) {
        closesocket(ws->socketFd);
        ws->socketFd = INVALID_SOCKET;
    }

    if (ws->acceptThread) {
        WaitForSingleObject(ws->acceptThread, 3000);
        CloseHandle(ws->acceptThread);
        ws->acceptThread = NULL;
    }
    if (ws->updateThread) {
        WaitForSingleObject(ws->updateThread, 3000);
        CloseHandle(ws->updateThread);
        ws->updateThread = NULL;
    }

    /* Close all client connections */
    EnterCriticalSection(&ws->lock);
    while (ws->clients) {
        WorldSocket* c = ws->clients;
        ws->clients = c->next;
        closesocket(c->fd);
        free(c);
    }
    LeaveCriticalSection(&ws->lock);

    printf("[World] WorldServer stopped.\n");
}

/* ================================================================
 *  WorldServer_Destroy
 * ================================================================ */
void WorldServer_Destroy(WorldServer* ws) {
    if (!ws) return;
    WorldServer_Stop(ws);

    /* Free all creatures */
    for (int i = 0; i < 512; i++) {
        Unit* u = ws->creatureHash[i];
        while (u) {
            Unit* next = u->next;
            free(u);
            u = next;
        }
    }

    /* Free all game objects */
    for (int i = 0; i < 512; i++) {
        GameObject* go = ws->objectHash[i];
        while (go) {
            GameObject* next = go->next;
            free(go);
            go = next;
        }
    }

    if (ws->scriptEngine)
        ScriptEngine_Destroy(ws->scriptEngine);

    DeleteCriticalSection(&ws->lock);
    free(ws);
}

/* ================================================================
 *  WorldServer_Update  (called from main loop every tick)
 * ================================================================ */
void WorldServer_Update(WorldServer* ws, uint32_t diffMs) {
    if (!ws || !ws->running) return;
    (void)diffMs;

    EnterCriticalSection(&ws->lock);

    /* Process incoming data from clients */
    WorldSocket** pp = &ws->clients;
    while (*pp) {
        WorldSocket* c = *pp;

        /* Receive data */
        if (c->recvLen < (int)sizeof(c->recvBuf) - 1) {
            int r = recv(c->fd, c->recvBuf + c->recvLen,
                         (int)(sizeof(c->recvBuf) - c->recvLen - 1), 0);
            if (r > 0) {
                c->recvLen += r;
            } else if (r == 0) {
                /* Disconnected */
                printf("[World] Client %s disconnected\n", c->host);
                closesocket(c->fd);
                *pp = c->next;
                if (c->player) free(c->player);
                free(c);
                continue;
            }
            /* r < 0: WSAEWOULDBLOCK is normal for non-blocking */
        }

        /* Process packets (simplified: 6-byte header = size(2 BE) + opcode(4 LE)) */
        while (c->recvLen >= 6) {
            uint16_t pktSize = ((uint8_t)c->recvBuf[0] << 8) | (uint8_t)c->recvBuf[1];
            uint32_t opcode  = (uint8_t)c->recvBuf[2] |
                              ((uint32_t)(uint8_t)c->recvBuf[3] << 8) |
                              ((uint32_t)(uint8_t)c->recvBuf[4] << 16) |
                              ((uint32_t)(uint8_t)c->recvBuf[5] << 24);
            uint32_t totalLen = 2 + pktSize; /* 2 bytes for size field + body */

            if ((int)totalLen > c->recvLen) break; /* Incomplete packet */

            /* Dispatch opcode */
            extern void WorldSession_HandlePacket(WorldServer* ws,
                WorldSocket* sock, uint32_t opcode,
                const uint8_t* data, int dataLen);
            WorldSession_HandlePacket(ws, c, opcode,
                (const uint8_t*)c->recvBuf + 6,
                (int)(totalLen - 6));

            /* Consume packet */
            memmove(c->recvBuf, c->recvBuf + totalLen, c->recvLen - totalLen);
            c->recvLen -= (int)totalLen;
        }

        pp = &c->next;
    }

    LeaveCriticalSection(&ws->lock);
}

/* ================================================================
 *  Creature / GameObject spawning
 * ================================================================ */
Unit* WorldServer_SpawnCreature(WorldServer* ws, uint32_t entry,
    MapId mapId, float x, float y, float z, float o)
{
    if (!ws) return NULL;

    Unit* u = (Unit*)calloc(1, sizeof(Unit));
    u->guid = (ObjectGuid)_next_guid(ws);
    u->typeId = TYPEID_UNIT;
    u->entry = entry;
    u->mapId = mapId;
    u->position[0] = x;
    u->position[1] = y;
    u->position[2] = z;
    u->orientation = o;
    u->spawnX = x; u->spawnY = y; u->spawnZ = z; u->spawnO = o;
    u->inWorld = true;
    u->dead = false;
    u->level = 1;
    u->speedWalk = 1.0f;
    u->speedRun = 1.14f;
    u->boundingRadius = 0.5f;

    snprintf(u->name, sizeof(u->name), "Creature_%u", entry);

    /* Set default fields */
    u->fields[UNIT_FIELD_HEALTH] = 100;
    u->fields[UNIT_FIELD_MAXHEALTH] = 100;
    u->fields[UNIT_FIELD_LEVEL] = 1;

    /* Hash insert */
    int bucket = (int)(u->guid % 512);
    EnterCriticalSection(&ws->lock);
    u->next = ws->creatureHash[bucket];
    ws->creatureHash[bucket] = u;
    LeaveCriticalSection(&ws->lock);

    /* Also add to map */
    if (mapId < 100) {
        Unit* mu = ws->maps[mapId].units;
        u->prev = NULL;
        u->next = mu;
        if (mu) mu->prev = u;
        ws->maps[mapId].units = u;
    }

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, u->name, "onSpawn", u);

    return u;
}

void WorldServer_RemoveUnit(WorldServer* ws, ObjectGuid guid) {
    if (!ws) return;
    int bucket = (int)(guid % 512);

    EnterCriticalSection(&ws->lock);
    Unit** pp = &ws->creatureHash[bucket];
    while (*pp) {
        if ((*pp)->guid == guid) {
            Unit* u = *pp;
            *pp = u->next;

            /* Fire script event */
            ScriptEngine_ExecuteWordEvent(ws->scriptEngine, u->name, "onDeath", u);

            free(u);
            LeaveCriticalSection(&ws->lock);
            return;
        }
        pp = &(*pp)->next;
    }
    LeaveCriticalSection(&ws->lock);
}

Unit* WorldServer_FindUnit(WorldServer* ws, ObjectGuid guid) {
    if (!ws) return NULL;
    int bucket = (int)(guid % 512);
    for (Unit* u = ws->creatureHash[bucket]; u; u = u->next) {
        if (u->guid == guid) return u;
    }
    return NULL;
}

/* ================================================================
 *  Player lookup
 * ================================================================ */
Player* WorldServer_GetPlayer(WorldServer* ws, ObjectGuid guid) {
    if (!ws) return NULL;
    for (WorldSocket* c = ws->clients; c; c = c->next) {
        if (c->player && c->player->guid == guid) return c->player;
    }
    return NULL;
}

/* ================================================================
 *  Broadcast a chat message to all connected players
 * ================================================================ */
void WorldServer_Broadcast(WorldServer* ws, const char* msg) {
    if (!ws || !msg) return;
    printf("[World] BROADCAST: %s\n", msg);

    /* Build SMSG_MESSAGECHAT packet for system message */
    size_t msgLen = strlen(msg);
    size_t pktBodyLen = 1 + 4 + 8 + 4 + 8 + 4 + msgLen + 1 + 1;
    size_t pktTotalLen = 4 + pktBodyLen; /* header(4) + body */

    uint8_t* pkt = (uint8_t*)calloc(1, pktTotalLen);
    int pos = 0;

    /* Header: size(2 BE) + opcode(2 LE) */
    uint16_t sz = (uint16_t)(pktBodyLen + 2);
    pkt[pos++] = (uint8_t)((sz >> 8) & 0xFF);
    pkt[pos++] = (uint8_t)(sz & 0xFF);
    pkt[pos++] = (uint8_t)(SMSG_MESSAGECHAT & 0xFF);
    pkt[pos++] = (uint8_t)((SMSG_MESSAGECHAT >> 8) & 0xFF);

    /* Chat type: 0x09 = CHAT_MSG_SYSTEM */
    pkt[pos++] = 0x09;
    /* Language: 0 = universal */
    pkt[pos++] = 0; pkt[pos++] = 0; pkt[pos++] = 0; pkt[pos++] = 0;
    /* Sender GUID (0 for system) */
    memset(pkt + pos, 0, 8); pos += 8;
    /* Unknown (4 bytes) */
    memset(pkt + pos, 0, 4); pos += 4;
    /* Target GUID (0) */
    memset(pkt + pos, 0, 8); pos += 8;
    /* Message length */
    pkt[pos++] = (uint8_t)((msgLen + 1) & 0xFF);
    pkt[pos++] = (uint8_t)(((msgLen + 1) >> 8) & 0xFF);
    pkt[pos++] = (uint8_t)(((msgLen + 1) >> 16) & 0xFF);
    pkt[pos++] = (uint8_t)(((msgLen + 1) >> 24) & 0xFF);
    /* Message */
    memcpy(pkt + pos, msg, msgLen);
    pos += (int)msgLen;
    pkt[pos++] = 0; /* null terminator */
    /* Chat tag */
    pkt[pos++] = 0;

    EnterCriticalSection(&ws->lock);
    for (WorldSocket* c = ws->clients; c; c = c->next) {
        if (!c->dead && c->authenticated)
            send(c->fd, (const char*)pkt, pos, 0);
    }
    LeaveCriticalSection(&ws->lock);
    free(pkt);
}

/* ================================================================
 *  Map management
 * ================================================================ */
Map* WorldServer_GetMap(WorldServer* ws, MapId id) {
    if (!ws || id >= 100) return NULL;
    return &ws->maps[id];
}

/* ================================================================
 *  DB loaders
 * ================================================================ */
void WorldServer_LoadCreatures(WorldServer* ws) {
    printf("[World] Loading creatures from database...\n");
    /* This would query creature_spawns + creature_template and spawn them.
     * For now it's a placeholder -- the demo spawns in main.c suffice. */
    printf("[World] Creature loading complete (DB stub).\n");
}

void WorldServer_LoadGameObjects(WorldServer* ws) {
    printf("[World] Loading game objects from database...\n");
    printf("[World] GameObject loading complete (DB stub).\n");
}

/* ================================================================
 *  GameObject support
 * ================================================================ */
GameObject* WorldServer_FindGameObject(WorldServer* ws, ObjectGuid guid) {
    if (!ws) return NULL;
    int bucket = (int)(guid % 512);
    for (GameObject* go = ws->objectHash[bucket]; go; go = go->next) {
        if (go->guid == guid) return go;
    }
    return NULL;
}

/* ================================================================
 *  Uptime string
 * ================================================================ */
const char* WorldServer_GetUptime(WorldServer* ws) {
    static char buf[64];
    if (!ws) return "0:00:00";
    int elapsed = (int)(time(NULL) - ws->startTime);
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    return buf;
}
