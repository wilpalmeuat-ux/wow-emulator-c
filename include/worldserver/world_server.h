/* world_server.h — WoW 3.3.5 WorldServer core */
#pragma once
#include "scripting/script_engine.h"
#include <time.h>

#include <pthread.h>
#define MAX_PLAYERS 5000
#define MAX_UNITS   100000
#define MAX_GAMEOBJECTS 50000
#define MAX_MAP_THREADS 8

typedef uint64_t ObjectGuid;
typedef uint32_t MapId;
typedef float    Vec3D[3];

typedef enum { TYPEID_PLAYER = 1, TYPEID_UNIT, TYPEID_GAMEOBJECT, TYPEID_ITEM, TYPEID_CONTAINER, TYPEID_DYNAMICOBJECT, TYPEID_CORPSE } TypeId;

typedef enum {
    UNIT_FIELD_BYTES_0, UNIT_FIELD_HEALTH, UNIT_FIELD_MAXHEALTH, UNIT_FIELD_POWER_1,
    UNIT_FIELD_POWER_2, UNIT_FIELD_POWER_3, UNIT_FIELD_POWER_4, UNIT_FIELD_POWER_5,
    UNIT_FIELD_LEVEL, UNIT_FIELD_FACTIONTEMPLATE, UNIT_FIELD_FLAGS, UNIT_FIELD_BASE_MANA,
    UNIT_FIELD_STAT_0, UNIT_FIELD_STAT_1, UNIT_FIELD_STAT_2, UNIT_FIELD_STAT_3, UNIT_FIELD_STAT_4,
    UNIT_FIELD_RESISTANCES_0, UNIT_END
} UnitFields;

typedef enum {
    GAMEOBJECT_DISPLAYID, GAMEOBJECT_TYPE_ID, GAMEOBJECT_POS_X, GAMEOBJECT_POS_Y,
    GAMEOBJECT_POS_Z, GAMEOBJECT_ORIENTATION, GAMEOBJECT_ROT_0, GAMEOBJECT_ROT_1,
    GAMEOBJECT_ROT_2, GAMEOBJECT_ROT_3, GAMEOBJECT_STATE, GAMEOBJECT_END
} GameObjectFields;

typedef enum { PLAYER_BYTES, PLAYER_LEVEL, PLAYER_XP, PLAYER_GOLD, PLAYER_GUILD_ID, PLAYER_END } PlayerFields;

struct WorldServer;
struct WorldSocket;

/* ── WorldSocket (client connection) ── */
typedef struct WorldSocket {
    ObjectGuid  guid;
    struct Player* player;
    char        host[128];
    uint32_t    accountId;
    int         fd;
    bool        authenticated;
    bool        dead;
    char        recvBuf[32768];
    int         recvLen;
    char        sendBuf[32768];
    int         sendLen;
    struct WorldSocket* next;
} WorldSocket;

/* ── Unit (creature/NPC) ── */
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
    uint32_t    level;              /* creature level */
    struct Unit* next;
    struct Unit* prev;
    void*       aiState;
    void*       scriptData;
} Unit;

/* ── Player ── */
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
    struct { uint32_t zoneId; uint32_t areaId; float taximask[8]; } location;
} Player;

/* ── GameObject ── */
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

/* ── Map ── */
typedef struct Map {
    MapId       id;
    char        name[64];
    uint32_t    playerCount;
    Unit*       units;
    GameObject* objects;
    void*       collision;
    pthread_t   thread;
    bool        running;
} Map;

/* ── WorldServer ── */
typedef struct WorldServer {
    uint16_t         port;
    bool             running;
    int              socketFd;
    pthread_t         acceptThread;
    pthread_t         updateThread;
    ScriptEngine*    scriptEngine;
    WorldSocket*     clients;
    Unit*            creatureHash[512];
    GameObject*      objectHash[512];
    Map              maps[100];
    int              mapCount;
    time_t           startTime;
    pthread_mutex_t  lock;
    uint32_t         maxPlayers;
    time_t           nextBroadcast;
} WorldServer;

/* ── WorldServer API ── */
WorldServer* WorldServer_Create(uint16_t port);
void          WorldServer_Destroy(WorldServer* ws);
bool          WorldServer_Start(WorldServer* ws);
void          WorldServer_Stop(WorldServer* ws);
void          WorldServer_Update(WorldServer* ws, uint32_t diffMs);
Unit*         WorldServer_SpawnCreature(WorldServer* ws, uint32_t entry, MapId mapId, float x, float y, float z, float o);
void          WorldServer_RemoveUnit(WorldServer* ws, ObjectGuid guid);
Player*       WorldServer_GetPlayer(WorldServer* ws, ObjectGuid guid);
void          WorldServer_Broadcast(WorldServer* ws, const char* msg);
Map*          WorldServer_GetMap(WorldServer* ws, MapId id);
void          WorldServer_LoadCreatures(WorldServer* ws);
void          WorldServer_LoadGameObjects(WorldServer* ws);
const char*   WorldServer_GetUptime(WorldServer* ws);
Unit*         WorldServer_FindUnit(WorldServer* ws, ObjectGuid guid);
GameObject*   WorldServer_FindGameObject(WorldServer* ws, ObjectGuid guid);

/* ── Opcodes (partial — WoW 3.3.5) ── */
typedef enum {
    CMSG_CHAR_ENUM             = 0x0037,
    CMSG_CHAR_CREATE           = 0x0039,
    CMSG_CHAR_DELETE           = 0x003B,
    CMSG_PLAYER_LOGIN          = 0x003D,
    CMSG_LOGOUT_REQUEST        = 0x004B,
    CMSG_NAME_QUERY             = 0x0050,
    CMSG_QUERY_TIME            = 0x0051,
    CMSG_GOSSIP_HELLO           = 0x0090,
    CMSG_GOSSIP_SELECT_OPTION   = 0x0091,
    CMSG_NPC_TEXT_QUERY         = 0x0092,
    CMSG_QUEST_GIVER_STATUS_QUERY = 0x00A0,
    CMSG_QUEST_GIVER_HELLO      = 0x00A1,
    SMSG_CHAR_ENUM              = 0x0038,
    SMSG_CHAR_CREATE           = 0x003A,
    SMSG_CHAR_DELETE           = 0x003C,
    SMSG_LOGIN_VERIFY_WORLD     = 0x003E,
    SMSG_TUTORIAL_FLAGS        = 0x0053,
    SMSG_ACCOUNT_DATA_TIMES    = 0x0054,
    SMSG_GOSSIP_MESSAGE        = 0x0092,
    SMSG_GOSSIP_POI            = 0x0093,
    SMSG_MESSAGE_CHAT          = 0x00AD,
    SMSG_DEFENSE_MESSAGE       = 0x00AE,
    SMSG_GAMEOBJECT_SPAWN_ANIM  = 0x00B3,
    SMSG_GAMEOBJECT_DESPAWN    = 0x00B4,
    SMSG_GAMEOBJECT_DATA_UPDATE = 0x00B5,
    SMSG_COMPRESSED_UPDATE_OBJECT = 0x00B9,
    SMSG_UPDATE_OBJECT         = 0x00BA,
    SMSG_DESTROY_OBJECT        = 0x00BB,
    SMSG_PONG                  = 0x00CD,
    SMSG_AUTH_CHALLENGE        = 0x00EC,
    SMSG_AUTH_RESPONSE         = 0x00ED,
    CMSG_AUTH_SESSION          = 0x00EE,
    CMSG_CHAT_IGNORED_ACCOUNT   = 0x005B,
} Opcode;

/* ── WorldSocket API ── */
WorldSocket* WorldSocket_Accept(WorldServer* ws);
void         WorldSocket_SetServer(WorldServer* ws);
void         WorldSocket_Close(WorldSocket* s);
bool         WorldSocket_Send(WorldSocket* s, const void* data, size_t len);
int          WorldSocket_Recv(WorldSocket* s, void* buf, size_t len);
void         WorldSocket_HandlePacket(WorldSocket* s, uint16_t opcode, const uint8_t* body, size_t bodyLen);
void         WorldSocket_Update(WorldSocket* s, uint32_t diffMs);

/* ── Update mask helpers ── */
void Unit_SetUInt32(Unit* u, UnitFields field, uint32_t value);
void Unit_SetUInt64(Unit* u, UnitFields field, uint64_t value);
uint32_t Unit_GetUInt32(Unit* u, UnitFields field);
