#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define WOW_VERSION 15595
#define MAX_PLAYERS 10000
#define MAX_NPCS 50000
#define MAX_ITEMS 100000
#define MAX_SPELLS 50000

typedef uint64_t ObjectGuid;
typedef uint32_t PlayerId;
typedef uint32_t NpcId;
typedef uint32_t ItemId;

typedef enum {
    TYPE_PLAYER,
    TYPE_NPC,
    TYPE_ITEM,
    TYPE_GAMEOBJECT,
    TYPE_DYNAMICOBJECT
} ObjectType;

typedef enum {
    CONNECTED,
    CONNECTING,
    DISCONNECTED,
    RECONNECTING
} ConnectionState;

typedef enum {
    ONLINE,
    OFFLINE,
    IN_GAME
} PlayerStatus;

#pragma pack(push, 1)

typedef struct {
    ObjectGuid guid;
    ObjectType type;
    uint32_t entry;
    float x, y, z;
    float orientation;
    uint32_t map;
} Object;

typedef struct {
    Object base;
    char name[64];
    uint32_t level;
    uint32_t health;
    uint32_t power;
    uint32_t max_health;
    uint32_t max_power;
    float speed;
    uint32_t flags;
} Player;

typedef struct {
    Object base;
    char name[64];
    uint32_t level;
    uint32_t health;
    uint32_t max_health;
    uint32_t flags;
    uint32_t faction;
    uint32_t model;
    uint32_t ai_template;
} Npc;

typedef struct {
    Object base;
    uint32_t entry;
    uint32_t item_class;
    uint32_t item_subclass;
    char name[64];
    uint32_t quality;
    uint32_t flags;
} Item;

typedef struct {
    ObjectGuid owner_guid;
    ItemId item_id;
    uint32_t entry;
    uint8_t bag_slot;
    uint8_t count;
    uint32_t duration;
} ItemInstance;

#pragma pack(pop)

typedef struct WorldPacket {
    uint16_t opcode;
    uint32_t size;
    uint8_t data[8192];
    struct WorldPacket* next;
} WorldPacket;

typedef struct ClientSocket ClientSocket;
typedef struct NpcScript NpcScript;

typedef void (*ScriptCallback)(void* target, int argc, char** argv);

#endif // WOW_SHARED_H
