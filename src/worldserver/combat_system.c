/* combat_system.c -- Combat system for WoW 3.3.5a emulator
 * Auto-attack, threat, aggro, hit/dodge/parry/crit tables, damage formulas.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ================================================================ */
#define MAX_THREAT_LIST 64
#define AGGRO_RANGE     15.0f
#define EVADE_RANGE     50.0f
#define MELEE_RANGE     5.0f
#define AUTO_ATTACK_SPEED 2000 /* ms */

typedef struct ThreatEntry { uint64_t guid; float threat; } ThreatEntry;
typedef struct ThreatList { ThreatEntry entries[MAX_THREAT_LIST]; int count; } ThreatList;

typedef enum { COMBAT_STATE_IDLE, COMBAT_STATE_COMBAT, COMBAT_STATE_EVADE, COMBAT_STATE_DEAD } CombatState;

typedef struct CombatData {
    CombatState  state;
    ThreatList   threatList;
    uint64_t     targetGuid;
    uint32_t     autoAttackTimer;
    float        aggroRange;
    float        evadeRange;
    bool         inCombat;
} CombatData;

typedef enum { HIT_RESULT_MISS, HIT_RESULT_DODGE, HIT_RESULT_PARRY, HIT_RESULT_BLOCK,
               HIT_RESULT_CRIT, HIT_RESULT_HIT, HIT_RESULT_GLANCING, HIT_RESULT_CRUSHING } HitResult;

/* ================================================================
 *  Distance calculation
 * ================================================================ */
static float _dist(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

/* ================================================================
 *  Roll hit table (simplified WoW 3.3.5 combat table)
 * ================================================================ */
HitResult Combat_RollHitTable(Unit* attacker, Unit* defender) {
    int atkLvl = (int)attacker->level;
    int defLvl = (int)defender->level;
    int lvlDiff = defLvl - atkLvl;
    int roll = rand() % 10000; /* 0-9999 = 0.00% - 99.99% */

    /* Miss chance: 5% base + 1% per level difference */
    int missChance = 500 + lvlDiff * 100;
    if (missChance < 100) missChance = 100;
    if (missChance > 6000) missChance = 6000;
    if (roll < missChance) return HIT_RESULT_MISS;
    roll -= missChance;

    /* Dodge: 5% base */
    int dodgeChance = 500;
    if (roll < dodgeChance) return HIT_RESULT_DODGE;
    roll -= dodgeChance;

    /* Parry: 5% (if defender can parry -- simplified) */
    int parryChance = 500;
    if (roll < parryChance) return HIT_RESULT_PARRY;
    roll -= parryChance;

    /* Block: 5% */
    int blockChance = 500;
    if (roll < blockChance) return HIT_RESULT_BLOCK;
    roll -= blockChance;

    /* Crit: 5% base + 1% per attacker level advantage */
    int critChance = 500 + (atkLvl - defLvl) * 100;
    if (critChance < 100) critChance = 100;
    if (critChance > 5000) critChance = 5000;
    if (roll < critChance) return HIT_RESULT_CRIT;
    roll -= critChance;

    /* Glancing blow (player vs higher level mob) */
    if (lvlDiff > 0 && rand() % 100 < 25)
        return HIT_RESULT_GLANCING;

    /* Crushing blow (mob 3+ levels above player) */
    if (lvlDiff >= 3 && rand() % 100 < 15)
        return HIT_RESULT_CRUSHING;

    return HIT_RESULT_HIT;
}

/* ================================================================
 *  Calculate melee damage
 * ================================================================ */
int32_t Combat_CalculateDamage(Unit* attacker, Unit* target, HitResult* outResult) {
    *outResult = Combat_RollHitTable(attacker, target);

    if (*outResult == HIT_RESULT_MISS || *outResult == HIT_RESULT_DODGE ||
        *outResult == HIT_RESULT_PARRY)
        return 0;

    /* Base damage from attack power */
    float minDmg = 5.0f + attacker->level * 2.0f;
    float maxDmg = 10.0f + attacker->level * 3.0f;
    float dmg = minDmg + ((float)(rand() % 1000) / 1000.0f) * (maxDmg - minDmg);

    /* Armor reduction (simplified) */
    uint32_t armor = 0; /* target->armor */
    float reduction = (float)armor / ((float)armor + 400.0f + 85.0f * attacker->level);
    if (reduction > 0.75f) reduction = 0.75f;
    dmg *= (1.0f - reduction);

    switch (*outResult) {
        case HIT_RESULT_CRIT:     dmg *= 2.0f; break;
        case HIT_RESULT_BLOCK:    dmg *= 0.7f; break;
        case HIT_RESULT_GLANCING: dmg *= 0.75f; break;
        case HIT_RESULT_CRUSHING: dmg *= 1.5f; break;
        default: break;
    }

    return (int32_t)dmg;
}

/* ================================================================
 *  Threat management
 * ================================================================ */
void Combat_AddThreat(ThreatList* tl, uint64_t guid, float amount) {
    for (int i = 0; i < tl->count; i++) {
        if (tl->entries[i].guid == guid) {
            tl->entries[i].threat += amount;
            return;
        }
    }
    if (tl->count < MAX_THREAT_LIST) {
        tl->entries[tl->count].guid = guid;
        tl->entries[tl->count].threat = amount;
        tl->count++;
    }
}

uint64_t Combat_GetHighestThreat(ThreatList* tl) {
    float highest = -1.0f;
    uint64_t target = 0;
    for (int i = 0; i < tl->count; i++) {
        if (tl->entries[i].threat > highest) {
            highest = tl->entries[i].threat;
            target = tl->entries[i].guid;
        }
    }
    return target;
}

void Combat_ClearThreat(ThreatList* tl) {
    tl->count = 0;
}

void Combat_RemoveFromThreat(ThreatList* tl, uint64_t guid) {
    for (int i = 0; i < tl->count; i++) {
        if (tl->entries[i].guid == guid) {
            tl->entries[i] = tl->entries[tl->count - 1];
            tl->count--;
            return;
        }
    }
}

/* ================================================================
 *  Apply melee damage to a target
 * ================================================================ */
void Combat_DealDamage(Unit* attacker, Unit* target, int32_t damage) {
    if (!target || target->dead) return;
    uint32_t hp = (uint32_t)target->fields[UNIT_FIELD_HEALTH];
    if (damage >= (int32_t)hp) {
        target->fields[UNIT_FIELD_HEALTH] = 0;
        target->dead = true;
        printf("[Combat] %s killed %s!\n", attacker->name, target->name);
    } else {
        target->fields[UNIT_FIELD_HEALTH] = hp - damage;
    }
}

/* ================================================================
 *  Auto-attack tick
 * ================================================================ */
void Combat_AutoAttackTick(Unit* attacker, Unit* target, uint32_t diffMs) {
    if (!attacker || !target || attacker->dead || target->dead) return;

    CombatData* cd = (CombatData*)attacker->aiState;
    if (!cd) return;

    cd->autoAttackTimer += diffMs;
    if (cd->autoAttackTimer < AUTO_ATTACK_SPEED) return;
    cd->autoAttackTimer = 0;

    /* Check melee range */
    float dist = _dist(attacker->position[0], attacker->position[1], attacker->position[2],
                       target->position[0], target->position[1], target->position[2]);
    if (dist > MELEE_RANGE) return;

    HitResult result;
    int32_t damage = Combat_CalculateDamage(attacker, target, &result);

    const char* resultStr[] = { "MISS", "DODGE", "PARRY", "BLOCK", "CRIT", "HIT", "GLANCING", "CRUSHING" };
    printf("[Combat] %s -> %s: %s for %d damage\n",
           attacker->name, target->name, resultStr[result], damage);

    if (damage > 0) {
        Combat_DealDamage(attacker, target, damage);
        Combat_AddThreat(&cd->threatList, target->guid, (float)damage);
    }
}

/* ================================================================
 *  XP reward calculation (WoW 3.3.5 formula simplified)
 * ================================================================ */
uint32_t Combat_CalculateXPReward(uint32_t playerLevel, uint32_t mobLevel) {
    if (mobLevel == 0) return 0;
    int diff = (int)mobLevel - (int)playerLevel;
    int baseXP = (int)(mobLevel * 5 + 45);
    float mult = 1.0f;
    if (diff > 0) mult = 1.0f + diff * 0.05f;
    else if (diff < 0) mult = 1.0f + diff * 0.1f; /* reduces */
    if (mult < 0.1f) mult = 0.0f; /* gray mob */
    if (mult > 2.0f) mult = 2.0f;
    return (uint32_t)(baseXP * mult);
}
