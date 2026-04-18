#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "wow_script.h"

typedef enum {
    RACE_NONE = 0, RACE_HUMAN = 1, RACE_ORC = 2, RACE_DWARF = 3,
    RACE_NIGHTELF = 4, RACE_UNDEAD = 5, RACE_TAUREN = 6, RACE_GNOME = 7, RACE_TROLL = 8,
    RACE_BLOODELF = 10, RACE_DRAENEI = 11
} Race;

typedef enum {
    CLASS_NONE = 0, CLASS_WARRIOR = 1, CLASS_PALADIN = 2, CLASS_HUNTER = 3,
    CLASS_ROGUE = 4, CLASS_PRIEST = 5, CLASS_SHAMAN = 7, CLASS_MAGE = 8,
    CLASS_WARLOCK = 9, CLASS_DRUID = 11
} Class;

typedef enum {
    STATE_ALIVE = 0, STATE_DEAD = 1, STATE_GHOST = 2, STATE_FOLLOWING = 3
} PlayerState;

typedef enum {
    TEAM_ALLIANCE = 0, TEAM_HORDE = 1
} Team;

typedef struct Vector3 {
    float x, y, z;
} Vector3;

typedef struct Player {
    uint64_t guid;
    char name[32];
    uint32_t level;
    Race race;
    Class class;
    Team team;
    PlayerState state;
    Vector3 position;
    float orientation;
    uint32_t map_id;
    uint32_t zone_id;
    uint64_t experience;
    uint32_t health;
    uint32_t power;
    uint32_t max_health;
    uint32_t max_power;
    uint32_t strength;
    uint32_t agility;
    uint32_t stamina;
    uint32_t intellect;
    uint32_t spirit;
    uint32_t armor;
    uint32_t resistance[6];
    uint64_t money;
    uint32_t guild_id;
    uint32_t area_id;
    float bounding_radius;
    float combat_reach;
    uint32_t mount_display_id;
    uint32_t bytes_1;
    uint32_t bytes_2;
    uint32_t flags;
    uint32_t pvp_flags;
    uint32_t chat_channels[8];
    int fd;
    WoWValue script_object;
} Player;

Player* player_create(uint64_t guid, const char* name, Race race, Class class);
void player_free(Player* p);
void player_send_to_client(Player* p, const uint8_t* data, size_t len);
void player_teleport(Player* p, uint32_t map, float x, float y, float z, float o);
void player_addExperience(Player* p, uint32_t xp);
void player_set_level(Player* p, uint32_t level);
void player_resurrect(Player* p);
void player_kill(Player* p);

Team player_get_team(Player* p);
bool player_is_alliance(Player* p);
bool player_is_horde(Player* p);

#endif
