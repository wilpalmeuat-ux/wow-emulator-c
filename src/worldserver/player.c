/* player.c -- Player management for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 *  Player creation helper
 * ================================================================ */
Player* Player_Create(uint64_t guid, const char* name, uint32_t race,
                      uint32_t class_, uint32_t level)
{
    Player* p = (Player*)calloc(1, sizeof(Player));
    if (!p) return NULL;

    p->guid = guid;
    strncpy(p->name, name ? name : "Unknown", sizeof(p->name) - 1);
    p->race = race;
    p->class_ = class_;
    p->level = level;
    p->isInWorld = false;
    p->dead = false;

    /* Default spawn: Northshire (Human start) */
    p->mapId = 0;
    p->position[0] = -8949.95f;
    p->position[1] = -132.49f;
    p->position[2] = 83.53f;
    p->orientation = 0.0f;

    /* Default unit fields */
    p->_unit.guid = guid;
    p->_unit.typeId = TYPEID_PLAYER;
    p->_unit.fields[UNIT_FIELD_HEALTH] = 100;
    p->_unit.fields[UNIT_FIELD_MAXHEALTH] = 100;
    p->_unit.fields[UNIT_FIELD_LEVEL] = level;

    return p;
}

/* ================================================================
 *  Player field helpers
 * ================================================================ */
void Player_SetLevel(Player* p, uint32_t level) {
    if (!p) return;
    p->level = level;
    p->_unit.fields[UNIT_FIELD_LEVEL] = level;
    p->_unit.level = level;
}

uint32_t Player_GetLevel(Player* p) {
    return p ? p->level : 0;
}

void Player_SetHealth(Player* p, int32_t health) {
    if (!p) return;
    if (health < 0) health = 0;
    p->_unit.fields[UNIT_FIELD_HEALTH] = (uint64_t)health;
}

int32_t Player_GetHealth(Player* p) {
    return p ? (int32_t)p->_unit.fields[UNIT_FIELD_HEALTH] : 0;
}

void Player_SetPosition(Player* p, float x, float y, float z, float o) {
    if (!p) return;
    p->position[0] = x;
    p->position[1] = y;
    p->position[2] = z;
    p->orientation = o;
}

void Player_Teleport(Player* p, uint32_t mapId, float x, float y, float z, float o) {
    if (!p) return;
    p->mapId = mapId;
    p->position[0] = x;
    p->position[1] = y;
    p->position[2] = z;
    p->orientation = o;
    printf("[Player] %s teleported to map %u (%.1f, %.1f, %.1f)\n",
           p->name, mapId, x, y, z);
}

bool Player_IsAlive(Player* p) {
    if (!p) return false;
    return !p->dead && p->_unit.fields[UNIT_FIELD_HEALTH] > 0;
}

void Player_Kill(Player* p) {
    if (!p) return;
    p->dead = true;
    p->_unit.fields[UNIT_FIELD_HEALTH] = 0;
}

void Player_Resurrect(Player* p) {
    if (!p) return;
    p->dead = false;
    p->_unit.fields[UNIT_FIELD_HEALTH] = p->_unit.fields[UNIT_FIELD_MAXHEALTH];
}
