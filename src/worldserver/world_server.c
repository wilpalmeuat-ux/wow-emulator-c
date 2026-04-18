/* world_server.c — WoW 3.3.5 WorldServer implementation (Windows)
 * WinSock2 + pthread (via wincompat). No POSIX unistd/signal/dirent.
 */
#include "shared/NetworkCompat.h"
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define LOG printf

static WorldServer* g_ws = NULL;
static uint32_t s_guidCounter = 100000;

static uint32_t _guid_hash(ObjectGuid g) { return (uint32_t)(g % 512); }

Unit* WorldServer_FindUnit(WorldServer* ws, ObjectGuid guid) {
    uint32_t bucket = _guid_hash(guid);
    for (Unit* u = ws->creatureHash[bucket]; u; u = u->next)
        if (u->guid == guid) return u;
    return NULL;
}

GameObject* WorldServer_FindGameObject(WorldServer* ws, ObjectGuid guid) {
    uint32_t bucket = _guid_hash(guid);
    for (GameObject* g = ws->objectHash[bucket]; g; g = g->next)
        if (g->guid == guid) return g;
    return NULL;
}

WorldServer* WorldServer_Create(uint16_t port) {
    WorldServer* ws = calloc(1, sizeof(WorldServer));
    if (!ws) return NULL;
    ws->socketFd = INVALID_SOCKET;
    ws->running = false;
    ws->port = port;
    g_ws = ws;
    return ws;
}

void WorldServer_Destroy(WorldServer* ws) {
    if (!ws) return;
    WorldServer_Stop(ws);
    free(ws);
    if (g_ws == ws) g_ws = NULL;
}

bool WorldServer_Start(WorldServer* ws) {
    if (!ws) return false;
    ws->socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws->socketFd == INVALID_SOCKET) return false;
    int opt = 1; setsockopt(ws->socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(ws->port);
    if (bind(ws->socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        closesocket(ws->socketFd); ws->socketFd = INVALID_SOCKET; return false;
    }
    listen(ws->socketFd, 10);
    ws->running = true;
    return true;
}

void WorldServer_Stop(WorldServer* ws) {
    if (!ws) return;
    ws->running = false;
    if (ws->socketFd != INVALID_SOCKET) {
        closesocket(ws->socketFd);
        ws->socketFd = INVALID_SOCKET;
    }
}

void WorldServer_Update(WorldServer* ws, uint32_t diffMs) {
    (void)ws; (void)diffMs;
}

bool WorldServer_LoadCreatures(WorldServer* ws) {
    (void)ws; return true;
}

bool WorldServer_LoadGameObjects(WorldServer* ws) {
    (void)ws; return true;
}

void WorldServer_SpawnCreature(WorldServer* ws, uint32_t entry, uint32_t map, float x, float y, float z, float o) {
    (void)ws; (void)entry; (void)map; (void)x; (void)y; (void)z; (void)o;
    printf("[WorldServer] Spawning creature entry=%u map=%u at (%.1f, %.1f, %.1f)\n", entry, map, x, y, z);
}

ObjectGuid WorldServer_GenerateGuid(WorldServer* ws, uint32_t high) {
    (void)ws; return (ObjectGuid)(high << 32) | (s_guidCounter++ & 0xFFFFFFFF);
}

bool WorldServer_AddCreature(WorldServer* ws, Unit* u) {
    if (!ws || !u) return false;
    uint32_t bucket = _guid_hash(u->guid);
    u->next = ws->creatureHash[bucket];
    ws->creatureHash[bucket] = u;
    ws->creatureCount++;
    return true;
}

bool WorldServer_RemoveCreature(WorldServer* ws, ObjectGuid guid) {
    if (!ws) return false;
    uint32_t bucket = _guid_hash(guid);
    Unit** prev = &ws->creatureHash[bucket];
    for (Unit* u = ws->creatureHash[bucket]; u; u = u->next) {
        if (u->guid == guid) { *prev = u->next; free(u); ws->creatureCount--; return true; }
        prev = &u->next;
    }
    return false;
}

const char* WorldServer_GetLastError(WorldServer* ws) {
    return ws ? ws->error : "unknown";
}