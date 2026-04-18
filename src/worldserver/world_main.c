/* World Server main — spawns units, handles movement, hooks WSS scripts (Windows)
 * WinSock2 + Windows API only. No POSIX.
 */
#include "shared/NetworkCompat.h"
#include <shared/wow_packet.h>
#include <scripting/wss_wow_hooks.h>
#include <scripting/wss_objectstore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define WORLD_PORT 8085
#define MAX_PLAYERS 100
#define MAX_UNITS 10000
#define PI 3.14159265358979323846

typedef struct {
    uint64_t guid;
    char name[64];
    float x, y, z;
    float facing;
    uint32_t map_id;
    uint32_t level;
    uint32_t health;
    uint32_t max_health;
    uint32_t power;
    uint32_t max_power;
    uint8_t unit_type; /* 0=player, 1=npc, 2=object */
    uint8_t race;
    uint8_t class_;
    uint32_t model_id;
    bool in_combat;
    bool dead;
    uint32_t npc_flags;
    float speed_walk;
    float speed_run;
    uint32_t faction;
    float bounding_radius;
    uint32_t guild_id;
    uint64_t player_guid;
} WoWUnit;

typedef struct {
    socket_t fd;
    char ip[32];
    uint8_t recv_buf[32768];
    int recv_len;
    uint8_t send_buf[32768];
    int send_len;
    uint64_t player_guid;
    char username[32];
    bool authenticated;
    float pos_x, pos_y, pos_z;
    float facing;
    uint32_t map_id;
    uint32_t zone_id;
} WorldPlayer;

static WssObjectStore g_registry;
static WssScriptingSystem g_scripts;
static WoWUnit units[MAX_UNITS];
static int num_units = 0;
static WorldPlayer players[MAX_PLAYERS];
static int num_players = 0;
static volatile int g_running = 1;
static socket_t listen_fd = INVALID_SOCKET;

static BOOL WINAPI _sig(DWORD ctrl) { (void)ctrl; g_running = 0; return TRUE; }

static socket_t create_socket(int port) {
    socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) return INVALID_SOCKET;
    int opt = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { closesocket(fd); return INVALID_SOCKET; }
    return fd;
}

static WoWUnit* spawn_unit(uint64_t guid, const char* name, float x, float y, float z, uint32_t model, uint32_t level, uint32_t npc_flags, uint32_t faction) {
    if (num_units >= MAX_UNITS) return NULL;
    WoWUnit* u = &units[num_units++];
    memset(u, 0, sizeof(*u));
    u->guid = guid;
    strncpy(u->name, name, sizeof(u->name)-1);
    u->x = x; u->y = y; u->z = z;
    u->model_id = model;
    u->level = level;
    u->npc_flags = npc_flags;
    u->faction = faction;
    u->speed_walk = 2.5f;
    u->speed_run = 8.0f;
    u->max_health = level * 100 + 500;
    u->health = u->max_health;
    u->max_power = level * 50 + 100;
    u->power = u->max_power;
    u->bounding_radius = 0.5f;
    u->unit_type = 1;
    return u;
}

static void move_unit(WoWUnit* u, float dx, float dz) {
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist < 0.001f) return;
    float step = (dist > u->speed_run * 0.1f) ? u->speed_run * 0.1f : dist;
    float nx = u->x + (dx / dist) * step;
    float nz = u->z + (dz / dist) * step;
    u->x = nx; u->z = nz;
    if (dx != 0 || dz != 0) u->facing = atan2f(dz, dx);
}

static void send_to_player(WorldPlayer* p, uint32_t opcode, const uint8_t* data, int len) {
    uint8_t hdr[6];
    int pkt_len = len + 4;
    hdr[0] = (uint8_t)(pkt_len & 0xFF);
    hdr[1] = (uint8_t)((pkt_len >> 8) & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);
    hdr[4] = (uint8_t)((opcode >> 16) & 0xFF);
    hdr[5] = (uint8_t)((opcode >> 24) & 0xFF);
    memcpy(p->send_buf, hdr, 6);
    if (data && len > 0) memcpy(p->send_buf + 6, data, len);
    p->send_len = len + 6;
    (void)send;
}

static void send_spawn_update(WorldPlayer* p, WoWUnit* u) {
    uint8_t buf[256];
    WowBuffer wb; WowBuffer_Init(&wb, buf, sizeof(buf));
    WowBuffer_WriteU64(&wb, u->guid);
    WowBuffer_WriteStr(&wb, u->name);
    WowBuffer_WriteU32(&wb, u->model_id);
    WowBuffer_WriteU32(&wb, u->guild_id ? u->guild_id : 0);
    WowBuffer_WriteU32(&wb, u->player_guid ? u->player_guid : 0);
    send_to_player(p, 0x16, buf, wb.pos);
}

static void init_default_scripts(void) {
    const char* init_script =
        "var world_name = \"WoW 3.3.5a Server\";\n"
        "var max_players = 100;\n"
        "print(\"World loaded: \" + world_name);\n"
        "print(\"Max players: \" + max_players);\n";

    const char* spawn_script =
        "func on_spawn(unit) {\n"
        "  print(\"Unit spawned: \" + unit.name);\n"
        "  return null;\n"
        "}\n";

    const char* gossip_script =
        "func on_gossip_hello(player, npc) {\n"
        "  print(\"Gossip: \" + player.name + \" talking to \" + npc.name);\n"
        "  return null;\n"
        "}\n";

    WssSystem_LoadScript(&g_scripts, "init", init_script);
    WssSystem_LoadScript(&g_scripts, "hook_on_spawn", spawn_script);
    WssSystem_LoadScript(&g_scripts, "hook_on_gossip_hello", gossip_script);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    SetConsoleCtrlHandler(_sig, TRUE);

    WssObjectStore_Init(&g_registry, 128);
    WssSystem_Init(&g_scripts, &g_registry);

    printf("=== WoW 3.3.5a World Server ===\n");
    init_default_scripts();

    /* Spawn starter NPCs */
    WoWUnit* test_npc = spawn_unit(1, "WSS Test NPC", 50.0f, 0.0f, 50.0f, 12345, 60, 0x00000001 | 0x00001000, 35);
    if (test_npc) printf("Spawned test NPC at (%.1f, %.1f, %.1f)\n", test_npc->x, test_npc->y, test_npc->z);

    WoWUnit* gossip_npc = spawn_unit(2, "WoW Scripted Vendor", 55.0f, 0.0f, 50.0f, 12346, 60, 0x00000001 | 0x00001000 | 0x00002000, 35);
    if (gossip_npc) printf("Spawned gossip NPC\n");

    WoWUnit* quest_npc = spawn_unit(3, "Quest Giver", 60.0f, 0.0f, 50.0f, 12347, 60, 0x00001000, 35);
    if (quest_npc) printf("Spawned quest NPC\n");

    printf("Loaded %d units\n", num_units);
    printf("Listening on UDP port %d\n", WORLD_PORT);

    listen_fd = create_socket(WORLD_PORT);
    if (listen_fd == INVALID_SOCKET) { printf("Failed to create socket\n"); return 1; }

    /* Main world update loop */
    uint32_t tick = 0;
    while (g_running) {
        tick++;
        if (tick % 10 == 0) {
            for (int i = 0; i < num_units; i++) {
                WoWUnit* u = &units[i];
                if (u->in_combat && tick % 2 == 0) { /* Combat AI tick */ }
                if (!u->in_combat && tick % 50 == 0) {
                    float ox = ((float)(rand() % 100) - 50.0f) * 0.01f;
                    float oz = ((float)(rand() % 100) - 50.0f) * 0.01f;
                    u->x += ox; u->z += oz;
                }
            }
        }
        if (tick % 100 == 0) {
            WssValue args[2];
            args[0].type = VAL_INT; args[0].data.as_int = tick;
            args[1].type = VAL_INT; args[1].data.as_int = num_units;
            WssSystem_RunHook(&g_scripts, HOOK_ON_UPDATE, 2, args);
        }
        Sleep(100); /* 10 Hz world tick */
    }

    if (listen_fd != INVALID_SOCKET) closesocket(listen_fd);
    WssSystem_Delete(&g_scripts);
    WssObjectStore_Delete(&g_registry);
    printf("[WorldServer] Shutdown complete\n");
    return 0;
}

uint32_t WoWUnit_GetGuildId(WoWUnit* u) { return u->guild_id; }
uint64_t WoWUnit_GetPlayerGuid(WoWUnit* u) { return u->player_guid; }