/* world_server.h -- WoW 3.3.5 WorldServer core (Windows only) */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <pthread.h>
#endif

#include "scripting/script_engine.h"

#define MAX_PLAYERS      5000
#define MAX_UNITS        100000
#define MAX_GAMEOBJECTS  50000
#define MAX_MAP_THREADS  8

typedef uint64_t ObjectGuid;
typedef uint32_t MapId;
typedef float    Vec3D[3];

typedef enum {
    TYPEID_PLAYER = 1, TYPEID_UNIT, TYPEID_GAMEOBJECT,
    TYPEID_ITEM, TYPEID_CONTAINER, TYPEID_DYNAMICOBJECT, TYPEID_CORPSE
} TypeId;

typedef enum {
    UNIT_FIELD_BYTES_0, UNIT_FIELD_HEALTH, UNIT_FIELD_MAXHEALTH,
    UNIT_FIELD_POWER_1, UNIT_FIELD_POWER_2, UNIT_FIELD_POWER_3,
    UNIT_FIELD_POWER_4, UNIT_FIELD_POWER_5,
    UNIT_FIELD_LEVEL, UNIT_FIELD_FACTIONTEMPLATE, UNIT_FIELD_FLAGS,
    UNIT_FIELD_BASE_MANA,
    UNIT_FIELD_STAT_0, UNIT_FIELD_STAT_1, UNIT_FIELD_STAT_2,
    UNIT_FIELD_STAT_3, UNIT_FIELD_STAT_4,
    UNIT_FIELD_RESISTANCES_0, UNIT_END
} UnitFields;

typedef enum {
    GAMEOBJECT_DISPLAYID, GAMEOBJECT_TYPE_ID,
    GAMEOBJECT_POS_X, GAMEOBJECT_POS_Y,
    GAMEOBJECT_POS_Z, GAMEOBJECT_ORIENTATION,
    GAMEOBJECT_ROT_0, GAMEOBJECT_ROT_1,
    GAMEOBJECT_ROT_2, GAMEOBJECT_ROT_3,
    GAMEOBJECT_STATE, GAMEOBJECT_END
} GameObjectFields;

typedef enum {
    PLAYER_BYTES, PLAYER_LEVEL, PLAYER_XP,
    PLAYER_GOLD, PLAYER_GUILD_ID, PLAYER_END
} PlayerFields;

struct WorldServer;
struct WorldSocket;

/* -- WorldSocket (client connection) -- */
typedef struct WorldSocket {
    ObjectGuid          guid;
    struct Player*      player;
    char                host[128];
    uint32_t            accountId;
    SOCKET              fd;           /* WinSock SOCKET */
    bool                authenticated;
    bool                dead;
    char                recvBuf[32768];
    int                 recvLen;
    char                sendBuf[32768];
    int                 sendLen;
    struct WorldSocket* next;
} WorldSocket;

/* -- Unit (creature/NPC) -- */
typedef struct Unit {
    ObjectGuid  guid;
    TypeId      typeId;
    MapId       mapId;
    Vec3D       position;
    float       orientation;
    float       spawnX, spawnY, spawnZ, spawnO;
    uint32_t    entry;
    char        name[64];
    uint32_t    displayId;
    uint32_t    faction;
    uint64_t    fields[UNIT_END];
    bool        inWorld;
    bool        dead;
    uint32_t    unitFlags;
    uint32_t    dynamicFlags;
    float       boundingRadius;
    float       speedWalk;
    float       speedRun;
    time_t      respawnTime;
    uint32_t    level;
    struct Unit* next;
    struct Unit* prev;
    void*       aiState;
    void*       scriptData;
} Unit;

/* -- Player -- */
typedef struct Player {
    ObjectGuid  guid;
    char        name[65];
    uint32_t    accountId;
    uint32_t    race;
    uint32_t    class_;
    uint32_t    level;
    uint64_t    fields[PLAYER_END];
    Unit        _unit;
    MapId       mapId;
    Vec3D       position;
    float       orientation;
    bool        isInWorld;
    bool        dead;
    WorldSocket* session;
    struct {
        uint32_t zoneId;
        uint32_t areaId;
        float    taximask[8];
    } location;
} Player;

/* -- GameObject -- */
typedef struct GameObject {
    ObjectGuid  guid;
    uint32_t    entry;
    MapId       mapId;
    Vec3D       position;
    float       orientation;
    float       rot[4];
    uint32_t    state;
    uint32_t    displayId;
    uint32_t    type;
    bool        inWorld;
    time_t      respawnTime;
    char        name[64];
    uint64_t    fields[GAMEOBJECT_END];
    void*       aiState;
    struct GameObject* next;
} GameObject;

/* -- Map -- */
typedef struct Map {
    MapId       id;
    char        name[64];
    uint32_t    playerCount;
    Unit*       units;
    GameObject* objects;
    void*       collision;
    HANDLE      thread;     /* Windows thread handle */
    bool        running;
} Map;

/* -- WorldServer -- */
typedef struct WorldServer {
    uint16_t            port;
    bool                running;
    SOCKET              socketFd;        /* WinSock SOCKET */
    HANDLE              acceptThread;    /* Windows thread */
    HANDLE              updateThread;    /* Windows thread */
    ScriptEngine*       scriptEngine;
    WorldSocket*        clients;
    Unit*               creatureHash[512];
    GameObject*         objectHash[512];
    Map                 maps[100];
    int                 mapCount;
    time_t              startTime;
    CRITICAL_SECTION    lock;            /* Windows mutex */
    uint32_t            maxPlayers;
    time_t              nextBroadcast;
    uint32_t            nextGuid;
} WorldServer;

/* -- WorldServer API -- */
WorldServer* WorldServer_Create(uint16_t port);
void         WorldServer_Destroy(WorldServer* ws);
bool         WorldServer_Start(WorldServer* ws);
void         WorldServer_Stop(WorldServer* ws);
void         WorldServer_Update(WorldServer* ws, uint32_t diffMs);
Unit*        WorldServer_SpawnCreature(WorldServer* ws, uint32_t entry,
                 MapId mapId, float x, float y, float z, float o);
void         WorldServer_RemoveUnit(WorldServer* ws, ObjectGuid guid);
Player*      WorldServer_GetPlayer(WorldServer* ws, ObjectGuid guid);
void         WorldServer_Broadcast(WorldServer* ws, const char* msg);
Map*         WorldServer_GetMap(WorldServer* ws, MapId id);
void         WorldServer_LoadCreatures(WorldServer* ws);
void         WorldServer_LoadGameObjects(WorldServer* ws);
const char*  WorldServer_GetUptime(WorldServer* ws);
Unit*        WorldServer_FindUnit(WorldServer* ws, ObjectGuid guid);
GameObject*  WorldServer_FindGameObject(WorldServer* ws, ObjectGuid guid);

/* -- Unit helpers (used by scripting) -- */
void         Unit_SetUInt32(void* unit, int field, uint32_t val);
uint32_t     Unit_GetUInt32(void* unit, int field);

/* -- Opcodes (WoW 3.3.5a subset) -- */
typedef enum {
    CMSG_CHAR_ENUM              = 0x0037,
    CMSG_CHAR_CREATE            = 0x0039,
    CMSG_CHAR_DELETE            = 0x003B,
    CMSG_PLAYER_LOGIN           = 0x003D,
    CMSG_LOGOUT_REQUEST         = 0x004B,
    CMSG_NAME_QUERY             = 0x0050,
    CMSG_QUERY_TIME             = 0x01CE,
    CMSG_PING                   = 0x01DC,
    CMSG_AUTH_SESSION           = 0x01ED,
    CMSG_CHAR_RENAME            = 0x0238,
    CMSG_MOVE_HEARTBEAT         = 0x00EE,
    CMSG_MESSAGECHAT            = 0x0095,
    CMSG_GOSSIP_HELLO           = 0x017B,
    CMSG_GOSSIP_SELECT_OPTION   = 0x017C,
    CMSG_CREATURE_QUERY         = 0x0060,
    CMSG_GAMEOBJECT_QUERY       = 0x005E,
    CMSG_MOVE_WORLDPORT_ACK     = 0x00DC,
    SMSG_CHAR_ENUM              = 0x003B,
    SMSG_CHAR_CREATE            = 0x003A,
    SMSG_CHAR_DELETE            = 0x003C,
    SMSG_LOGIN_VERIFY_WORLD     = 0x0236,
    SMSG_TUTORIAL_FLAGS         = 0x00FD,
    SMSG_ACCOUNT_DATA_TIMES     = 0x0209,
    SMSG_MESSAGECHAT            = 0x0096,
    SMSG_UPDATE_OBJECT          = 0x00A9,
    SMSG_DESTROY_OBJECT         = 0x00AA,
    SMSG_PONG                   = 0x01DD,
    SMSG_AUTH_CHALLENGE         = 0x01EC,
    SMSG_AUTH_RESPONSE          = 0x01EE,
    SMSG_NAME_QUERY_RESPONSE    = 0x0051,
    SMSG_CREATURE_QUERY_RESPONSE = 0x0061,
    SMSG_QUERY_TIME_RESPONSE    = 0x01CF,
    SMSG_GOSSIP_MESSAGE         = 0x017D,
    SMSG_ADDON_INFO             = 0x02EF,
    MSG_MOVE_WORLDPORT_ACK      = 0x00DC
} WowOpcode;
