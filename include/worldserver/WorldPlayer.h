#ifndef WOW_WORLDPER_H
#define WOW_WORLDPER_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef struct WorldPlayer {
    uint64_t guid;
    char     name[256];
    uint32_t race;
    uint32_t class_;
    uint32_t level;
    float    x, y, z, o;
    uint32_t map;
    uint32_t health;
    uint32_t power;
    bool     in_world;
} WorldPlayer;
typedef struct PlayerRegistry {
    WorldPlayer* players[1000];
    size_t count;
} PlayerRegistry;
extern PlayerRegistry g_players;
WorldPlayer* player_create(uint64_t guid, const char* name, uint32_t race, uint32_t class_);
void         player_destroy(WorldPlayer* p);
WorldPlayer* player_by_guid(uint64_t guid);
void         player_teleport(WorldPlayer* p, uint32_t map, float x, float y, float z, float o);
void         player_set_level(WorldPlayer* p, uint32_t level);
void         player_add_item(WorldPlayer* p, uint32_t entry, uint32_t count);
void         player_set_health(WorldPlayer* p, uint32_t hp);
void         player_set_power(WorldPlayer* p, uint32_t pw);
void         player_respawn(WorldPlayer* p);
#endif
