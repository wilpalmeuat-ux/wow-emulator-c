/* instance_system.c -- Dungeon/Raid instance system for WoW 3.3.5a */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_INSTANCES 500
#define MAX_INSTANCE_PLAYERS 40
#define MAX_INSTANCE_BOSSES 12

typedef enum { INST_DUNGEON=0, INST_RAID=1, INST_HEROIC_DUNGEON=2, INST_HEROIC_RAID=3 } InstanceType;
typedef enum { DIFF_NORMAL_5=0, DIFF_HEROIC_5=1, DIFF_NORMAL_10=0, DIFF_NORMAL_25=1, DIFF_HEROIC_10=2, DIFF_HEROIC_25=3 } Difficulty;
typedef enum { BOSS_ALIVE=0, BOSS_DEAD=1 } BossState;

typedef struct InstanceBoss {
    uint32_t  entry;
    char      name[64];
    BossState state;
    time_t    killTime;
} InstanceBoss;

typedef struct InstanceTemplate {
    uint32_t     mapId;
    char         name[64];
    InstanceType type;
    uint32_t     maxPlayers;
    uint32_t     resetTime;     /* seconds */
    uint32_t     bossEntries[MAX_INSTANCE_BOSSES];
    char         bossNames[MAX_INSTANCE_BOSSES][64];
    int          bossCount;
    uint32_t     minLevel;
    uint32_t     maxLevel;
} InstanceTemplate;

typedef struct Instance {
    uint32_t       id;
    uint32_t       mapId;
    Difficulty     difficulty;
    InstanceBoss   bosses[MAX_INSTANCE_BOSSES];
    int            bossCount;
    uint64_t       playerGuids[MAX_INSTANCE_PLAYERS];
    int            playerCount;
    time_t         createTime;
    time_t         resetTime;
    uint64_t       groupId;    /* group/raid that owns this */
    bool           active;
} Instance;

typedef struct InstanceLock {
    uint64_t guid;           /* player GUID */
    uint32_t instanceId;
    uint32_t mapId;
    Difficulty difficulty;
    time_t   expiresAt;
} InstanceLock;

#define MAX_INSTANCE_TEMPLATES 64
static InstanceTemplate g_instanceTemplates[MAX_INSTANCE_TEMPLATES];
static int g_instanceTemplateCount = 0;
static Instance g_instances[MAX_INSTANCES];
static uint32_t g_nextInstanceId = 1;
static InstanceLock g_locks[5000];
static int g_lockCount = 0;

void InstanceSystem_Init(void) {
    memset(g_instanceTemplates, 0, sizeof(g_instanceTemplates));
    memset(g_instances, 0, sizeof(g_instances));
    g_instanceTemplateCount = 0;

    /* Demo instances */
    InstanceTemplate* t;

    t = &g_instanceTemplates[g_instanceTemplateCount++];
    t->mapId = 33; strncpy(t->name, "Shadowfang Keep", 63);
    t->type = INST_DUNGEON; t->maxPlayers = 5; t->resetTime = 86400;
    t->minLevel = 18; t->maxLevel = 21;
    strncpy(t->bossNames[0], "Baron Silverlaine", 63); t->bossEntries[0] = 3887;
    strncpy(t->bossNames[1], "Commander Springvale", 63); t->bossEntries[1] = 4278;
    strncpy(t->bossNames[2], "Lord Walden", 63); t->bossEntries[2] = 46963;
    strncpy(t->bossNames[3], "Lord Godfrey", 63); t->bossEntries[3] = 46964;
    t->bossCount = 4;

    t = &g_instanceTemplates[g_instanceTemplateCount++];
    t->mapId = 631; strncpy(t->name, "Icecrown Citadel", 63);
    t->type = INST_RAID; t->maxPlayers = 25; t->resetTime = 604800;
    t->minLevel = 80; t->maxLevel = 80;
    strncpy(t->bossNames[0], "Lord Marrowgar", 63); t->bossEntries[0] = 36612;
    strncpy(t->bossNames[1], "Lady Deathwhisper", 63); t->bossEntries[1] = 36855;
    strncpy(t->bossNames[2], "Deathbringer Saurfang", 63); t->bossEntries[2] = 37813;
    strncpy(t->bossNames[3], "Festergut", 63); t->bossEntries[3] = 36626;
    strncpy(t->bossNames[4], "Rotface", 63); t->bossEntries[4] = 36627;
    strncpy(t->bossNames[5], "Professor Putricide", 63); t->bossEntries[5] = 36678;
    strncpy(t->bossNames[6], "Blood Prince Council", 63); t->bossEntries[6] = 37970;
    strncpy(t->bossNames[7], "Blood-Queen Lana'thel", 63); t->bossEntries[7] = 37955;
    strncpy(t->bossNames[8], "Valithria Dreamwalker", 63); t->bossEntries[8] = 36789;
    strncpy(t->bossNames[9], "Sindragosa", 63); t->bossEntries[9] = 36853;
    strncpy(t->bossNames[10], "The Lich King", 63); t->bossEntries[10] = 36597;
    t->bossCount = 11;

    t = &g_instanceTemplates[g_instanceTemplateCount++];
    t->mapId = 249; strncpy(t->name, "Onyxia's Lair", 63);
    t->type = INST_RAID; t->maxPlayers = 25; t->resetTime = 604800;
    t->minLevel = 80; t->maxLevel = 80;
    strncpy(t->bossNames[0], "Onyxia", 63); t->bossEntries[0] = 10184;
    t->bossCount = 1;

    printf("[Instance] Registered %d instance templates\n", g_instanceTemplateCount);
}

InstanceTemplate* InstanceSystem_GetTemplate(uint32_t mapId) {
    for (int i = 0; i < g_instanceTemplateCount; i++)
        if (g_instanceTemplates[i].mapId == mapId) return &g_instanceTemplates[i];
    return NULL;
}

uint32_t InstanceSystem_Create(uint32_t mapId, Difficulty diff, uint64_t groupId) {
    InstanceTemplate* tmpl = InstanceSystem_GetTemplate(mapId);
    if (!tmpl) return 0;

    Instance* inst = NULL;
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (!g_instances[i].active) { inst = &g_instances[i]; break; }
    }
    if (!inst) return 0;

    memset(inst, 0, sizeof(Instance));
    inst->id = g_nextInstanceId++;
    inst->mapId = mapId;
    inst->difficulty = diff;
    inst->groupId = groupId;
    inst->createTime = time(NULL);
    inst->resetTime = inst->createTime + tmpl->resetTime;
    inst->active = true;

    for (int i = 0; i < tmpl->bossCount; i++) {
        inst->bosses[i].entry = tmpl->bossEntries[i];
        strncpy(inst->bosses[i].name, tmpl->bossNames[i], 63);
        inst->bosses[i].state = BOSS_ALIVE;
    }
    inst->bossCount = tmpl->bossCount;

    printf("[Instance] Created %s (ID %u, diff %d)\n", tmpl->name, inst->id, diff);
    return inst->id;
}

Instance* InstanceSystem_Get(uint32_t instanceId) {
    for (int i = 0; i < MAX_INSTANCES; i++)
        if (g_instances[i].id == instanceId && g_instances[i].active) return &g_instances[i];
    return NULL;
}

bool InstanceSystem_AddPlayer(uint32_t instanceId, uint64_t guid) {
    Instance* inst = InstanceSystem_Get(instanceId);
    if (!inst || inst->playerCount >= MAX_INSTANCE_PLAYERS) return false;
    inst->playerGuids[inst->playerCount++] = guid;
    return true;
}

void InstanceSystem_SetBossDead(uint32_t instanceId, uint32_t bossEntry) {
    Instance* inst = InstanceSystem_Get(instanceId);
    if (!inst) return;
    for (int i = 0; i < inst->bossCount; i++) {
        if (inst->bosses[i].entry == bossEntry && inst->bosses[i].state == BOSS_ALIVE) {
            inst->bosses[i].state = BOSS_DEAD;
            inst->bosses[i].killTime = time(NULL);
            printf("[Instance] Boss '%s' killed in instance %u\n", inst->bosses[i].name, instanceId);
            break;
        }
    }
}

bool InstanceSystem_IsCleared(uint32_t instanceId) {
    Instance* inst = InstanceSystem_Get(instanceId);
    if (!inst) return false;
    for (int i = 0; i < inst->bossCount; i++)
        if (inst->bosses[i].state == BOSS_ALIVE) return false;
    return true;
}

void InstanceSystem_AddLock(uint64_t guid, uint32_t instanceId, uint32_t mapId, Difficulty diff, time_t expires) {
    if (g_lockCount >= 5000) return;
    InstanceLock* lock = &g_locks[g_lockCount++];
    lock->guid = guid;
    lock->instanceId = instanceId;
    lock->mapId = mapId;
    lock->difficulty = diff;
    lock->expiresAt = expires;
}

void InstanceSystem_ResetExpired(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_INSTANCES; i++) {
        Instance* inst = &g_instances[i];
        if (inst->active && inst->resetTime <= now) {
            printf("[Instance] Instance %u (map %u) reset\n", inst->id, inst->mapId);
            inst->active = false;
        }
    }
    /* Clean expired locks */
    for (int i = g_lockCount - 1; i >= 0; i--) {
        if (g_locks[i].expiresAt <= now) {
            g_locks[i] = g_locks[g_lockCount - 1];
            g_lockCount--;
        }
    }
}
