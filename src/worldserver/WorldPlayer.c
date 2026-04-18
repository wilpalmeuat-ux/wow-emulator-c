#include "worldserver/WorldPlayer.h"
#include "shared/log.h"
#include <stdlib.h>
#include <string.h>
PlayerRegistry g_players = {0};
WorldPlayer* player_create(uint64_t guid, const char* name, uint32_t race, uint32_t class_) {
    WorldPlayer* p = calloc(1,sizeof(WorldPlayer));
    p->guid = guid; p->race = race; p->class_ = class_;
    p->level = 1; p->health = 100; p->power = 100;
    p->x = 0; p->y = 0; p->z = 0; p->o = 0;
    p->map = 0; p->in_world = false;
    if(name) strncpy(p->name, name, 255);
    if(g_players.count < 999) g_players.players[g_players.count++] = p;
    LOG_INFO("Player created: %s (GUID: %llu)", name, (unsigned long long)guid);
    return p;
}
void player_destroy(WorldPlayer* p){ free(p); }
WorldPlayer* player_by_guid(uint64_t guid) {
    for(size_t i=0;i<g_players.count;i++)
        if(g_players.players[i]->guid==guid) return g_players.players[i];
    return NULL;
}
void player_teleport(WorldPlayer* p, uint32_t map, float x, float y, float z, float o) {
    LOG_INFO("Player %s teleporting to map=%u x=%.2f y=%.2f z=%.2f", p->name, map, x, y, z);
    p->map=map; p->x=x; p->y=y; p->z=z; p->o=o;
}
void player_set_level(WorldPlayer* p, uint32_t level) {
    LOG_INFO("Player %s set level %u", p->name, level);
    p->level = level;
}
void player_add_item(WorldPlayer* p, uint32_t entry, uint32_t count) {
    (void)p; (void)entry; (void)count;
    LOG_INFO("Player %s gains item %u x%u", p->name, entry, count);
}
void player_set_health(WorldPlayer* p, uint32_t hp){ p->health=hp; }
void player_set_power(WorldPlayer* p, uint32_t pw){ p->power=pw; }
void player_respawn(WorldPlayer* p){ p->health=100; p->power=100; LOG_INFO("Player %s respawned",p->name); }
void player_destroy_client(WorldPlayer* p){ p->in_world=false; LOG_INFO("Player %s left world",p->name); }
void player_save(WorldPlayer* p){ (void)p; LOG_INFO("Player %s saved", p->name); }
