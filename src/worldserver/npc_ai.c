/* npc_ai.c -- NPC AI system for WoW 3.3.5a emulator
 * Patrol paths, aggro, combat behavior, evade/reset.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_WAYPOINTS 32
#define AGGRO_RANGE   15.0f
#define EVADE_RANGE   50.0f
#define CHASE_SPEED   1.14f

typedef enum { AI_IDLE, AI_PATROL, AI_COMBAT, AI_EVADE, AI_DEAD, AI_SCRIPTED } AIState;

typedef struct { float x, y, z; uint32_t waitTime; } Waypoint;

typedef struct NpcAI {
    AIState    state;
    uint64_t   targetGuid;
    float      homeX, homeY, homeZ;   /* spawn position */
    Waypoint   waypoints[MAX_WAYPOINTS];
    int        waypointCount;
    int        currentWaypoint;
    uint32_t   waitTimer;
    uint32_t   combatTimer;
    uint32_t   evadeTimer;
    uint32_t   autoAttackTimer;
    float      aggroRange;
    bool       isElite;
    bool       isBoss;
    /* Threat */
    struct { uint64_t guid; float threat; } threatList[16];
    int        threatCount;
} NpcAI;

static float _dist3d(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2-x1, dy = y2-y1, dz = z2-z1;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

NpcAI* NpcAI_Create(Unit* unit) {
    NpcAI* ai = (NpcAI*)calloc(1, sizeof(NpcAI));
    ai->state = AI_IDLE;
    ai->homeX = unit->spawnX;
    ai->homeY = unit->spawnY;
    ai->homeZ = unit->spawnZ;
    ai->aggroRange = AGGRO_RANGE;
    unit->aiState = ai;
    return ai;
}

void NpcAI_Destroy(NpcAI* ai) { free(ai); }

void NpcAI_AddWaypoint(NpcAI* ai, float x, float y, float z, uint32_t waitMs) {
    if (ai->waypointCount >= MAX_WAYPOINTS) return;
    Waypoint* wp = &ai->waypoints[ai->waypointCount++];
    wp->x = x; wp->y = y; wp->z = z; wp->waitTime = waitMs;
    if (ai->state == AI_IDLE) ai->state = AI_PATROL;
}

void NpcAI_AddThreat(NpcAI* ai, uint64_t guid, float amount) {
    for (int i = 0; i < ai->threatCount; i++) {
        if (ai->threatList[i].guid == guid) {
            ai->threatList[i].threat += amount;
            return;
        }
    }
    if (ai->threatCount < 16) {
        ai->threatList[ai->threatCount].guid = guid;
        ai->threatList[ai->threatCount].threat = amount;
        ai->threatCount++;
    }
}

static uint64_t _getTopThreat(NpcAI* ai) {
    float max = -1; uint64_t g = 0;
    for (int i = 0; i < ai->threatCount; i++) {
        if (ai->threatList[i].threat > max) {
            max = ai->threatList[i].threat;
            g = ai->threatList[i].guid;
        }
    }
    return g;
}

/* Move toward a point at given speed */
static void _moveToward(Unit* u, float tx, float ty, float tz, float speed, uint32_t diffMs) {
    float dx = tx - u->position[0], dy = ty - u->position[1], dz = tz - u->position[2];
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    if (dist < 0.5f) return;
    float step = speed * (float)diffMs / 1000.0f;
    if (step > dist) step = dist;
    u->position[0] += (dx / dist) * step;
    u->position[1] += (dy / dist) * step;
    u->position[2] += (dz / dist) * step;
    u->orientation = atan2f(dy, dx);
}

void NpcAI_Update(Unit* unit, WorldServer* ws, uint32_t diffMs) {
    if (!unit || !unit->aiState || unit->dead) return;
    NpcAI* ai = (NpcAI*)unit->aiState;

    switch (ai->state) {
    case AI_IDLE:
        /* Check for nearby players to aggro */
        if (unit->faction != 35 && unit->faction != 0) { /* Not friendly */
            for (WorldSocket* c = ws->clients; c; c = c->next) {
                if (!c->player || !c->player->isInWorld) continue;
                float d = _dist3d(unit->position[0], unit->position[1], unit->position[2],
                                  c->player->position[0], c->player->position[1], c->player->position[2]);
                if (d < ai->aggroRange && c->player->mapId == unit->mapId) {
                    ai->state = AI_COMBAT;
                    ai->targetGuid = c->player->guid;
                    NpcAI_AddThreat(ai, c->player->guid, 1.0f);
                    printf("[AI] %s aggros player %s\n", unit->name, c->player->name);
                    break;
                }
            }
        }
        break;

    case AI_PATROL:
        if (ai->waitTimer > 0) {
            ai->waitTimer = (ai->waitTimer > diffMs) ? ai->waitTimer - diffMs : 0;
            break;
        }
        if (ai->waypointCount > 0) {
            Waypoint* wp = &ai->waypoints[ai->currentWaypoint];
            float d = _dist3d(unit->position[0], unit->position[1], unit->position[2], wp->x, wp->y, wp->z);
            if (d < 1.0f) {
                ai->waitTimer = wp->waitTime;
                ai->currentWaypoint = (ai->currentWaypoint + 1) % ai->waypointCount;
            } else {
                _moveToward(unit, wp->x, wp->y, wp->z, unit->speedWalk, diffMs);
            }
        }
        break;

    case AI_COMBAT: {
        /* Check evade distance */
        float homeD = _dist3d(unit->position[0], unit->position[1], unit->position[2],
                              ai->homeX, ai->homeY, ai->homeZ);
        if (homeD > EVADE_RANGE) {
            ai->state = AI_EVADE;
            ai->threatCount = 0;
            printf("[AI] %s evading (too far from home)\n", unit->name);
            break;
        }

        /* Find target */
        ai->targetGuid = _getTopThreat(ai);
        Player* target = WorldServer_GetPlayer(ws, ai->targetGuid);
        if (!target || target->dead || !target->isInWorld) {
            ai->state = AI_EVADE;
            ai->threatCount = 0;
            break;
        }

        /* Move toward target and attack */
        float td = _dist3d(unit->position[0], unit->position[1], unit->position[2],
                           target->position[0], target->position[1], target->position[2]);
        if (td > 5.0f) {
            _moveToward(unit, target->position[0], target->position[1], target->position[2],
                        unit->speedRun, diffMs);
        } else {
            /* Melee range -- auto attack */
            ai->autoAttackTimer += diffMs;
            if (ai->autoAttackTimer >= 2000) {
                ai->autoAttackTimer = 0;
                int32_t dmg = (int32_t)(5 + unit->level * 2 + rand() % (unit->level * 3 + 1));
                uint32_t hp = (uint32_t)target->_unit.fields[UNIT_FIELD_HEALTH];
                if (dmg >= (int32_t)hp) {
                    target->_unit.fields[UNIT_FIELD_HEALTH] = 0;
                    target->dead = true;
                    printf("[AI] %s killed player %s!\n", unit->name, target->name);
                } else {
                    target->_unit.fields[UNIT_FIELD_HEALTH] = hp - dmg;
                }
                NpcAI_AddThreat(ai, target->guid, (float)dmg);
            }
        }
        break;
    }

    case AI_EVADE:
        /* Return to spawn position */
        {
            float d = _dist3d(unit->position[0], unit->position[1], unit->position[2],
                              ai->homeX, ai->homeY, ai->homeZ);
            if (d < 1.0f) {
                unit->position[0] = ai->homeX;
                unit->position[1] = ai->homeY;
                unit->position[2] = ai->homeZ;
                /* Restore full health */
                unit->fields[UNIT_FIELD_HEALTH] = unit->fields[UNIT_FIELD_MAXHEALTH];
                unit->dead = false;
                ai->state = (ai->waypointCount > 0) ? AI_PATROL : AI_IDLE;
            } else {
                _moveToward(unit, ai->homeX, ai->homeY, ai->homeZ, unit->speedRun * 2.0f, diffMs);
            }
        }
        break;

    case AI_DEAD:
    case AI_SCRIPTED:
        break;
    }
}
