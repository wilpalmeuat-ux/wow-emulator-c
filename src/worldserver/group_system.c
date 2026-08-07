/* group_system.c -- Party/raid group system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_GROUPS 500
#define MAX_PARTY_MEMBERS 5
#define MAX_RAID_MEMBERS 40

typedef enum { GROUP_TYPE_PARTY=0, GROUP_TYPE_RAID=1 } GroupType;
typedef enum { LOOT_FREE_FOR_ALL=0, LOOT_ROUND_ROBIN=1, LOOT_MASTER=2, LOOT_GROUP=3, LOOT_NEED_GREED=4 } LootMethod;

typedef struct GroupMember {
    uint64_t guid;
    char     name[65];
    uint32_t level;
    uint32_t class_;
    uint8_t  subGroup;    /* 0-7 for raid groups */
    bool     assistant;
    bool     online;
} GroupMember;

typedef struct Group {
    uint32_t    id;
    GroupType   type;
    uint64_t    leaderGuid;
    LootMethod  lootMethod;
    uint64_t    masterLooter;
    uint32_t    lootThreshold;   /* item quality threshold for group loot */
    GroupMember members[MAX_RAID_MEMBERS];
    int         memberCount;
    uint32_t    targetIcons[8];  /* raid target icons */
    bool        active;
    uint32_t    dungeonDifficulty;
    uint32_t    raidDifficulty;
} Group;

static Group g_groups[MAX_GROUPS];
static int g_groupNextId = 1;

void GroupSystem_Init(void) {
    memset(g_groups, 0, sizeof(g_groups));
    g_groupNextId = 1;
    printf("[Group] Group system initialized\n");
}

static Group* _findFreeSlot(void) {
    for (int i = 0; i < MAX_GROUPS; i++)
        if (!g_groups[i].active) return &g_groups[i];
    return NULL;
}

uint32_t GroupSystem_Create(uint64_t leaderGuid, const char* leaderName) {
    Group* g = _findFreeSlot();
    if (!g) return 0;
    memset(g, 0, sizeof(Group));
    g->id = (uint32_t)g_groupNextId++;
    g->type = GROUP_TYPE_PARTY;
    g->leaderGuid = leaderGuid;
    g->lootMethod = LOOT_GROUP;
    g->lootThreshold = 2; /* uncommon+ */
    g->active = true;
    g->dungeonDifficulty = 0;
    g->raidDifficulty = 0;
    g->members[0].guid = leaderGuid;
    strncpy(g->members[0].name, leaderName, 64);
    g->memberCount = 1;
    printf("[Group] Created party (ID %u) by %s\n", g->id, leaderName);
    return g->id;
}

Group* GroupSystem_GetById(uint32_t groupId) {
    for (int i = 0; i < MAX_GROUPS; i++)
        if (g_groups[i].id == groupId && g_groups[i].active) return &g_groups[i];
    return NULL;
}

Group* GroupSystem_GetByMember(uint64_t playerGuid) {
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (!g_groups[i].active) continue;
        for (int j = 0; j < g_groups[i].memberCount; j++)
            if (g_groups[i].members[j].guid == playerGuid) return &g_groups[i];
    }
    return NULL;
}

bool GroupSystem_AddMember(uint32_t groupId, uint64_t guid, const char* name) {
    Group* g = GroupSystem_GetById(groupId);
    if (!g) return false;
    int maxMembers = (g->type == GROUP_TYPE_RAID) ? MAX_RAID_MEMBERS : MAX_PARTY_MEMBERS;
    if (g->memberCount >= maxMembers) return false;
    GroupMember* m = &g->members[g->memberCount++];
    m->guid = guid; strncpy(m->name, name, 64); m->online = true;
    printf("[Group] %s joined group %u\n", name, groupId);
    return true;
}

bool GroupSystem_RemoveMember(uint32_t groupId, uint64_t guid) {
    Group* g = GroupSystem_GetById(groupId);
    if (!g) return false;
    for (int i = 0; i < g->memberCount; i++) {
        if (g->members[i].guid == guid) {
            g->members[i] = g->members[g->memberCount - 1];
            g->memberCount--;
            if (g->memberCount <= 0) { g->active = false; }
            else if (guid == g->leaderGuid) g->leaderGuid = g->members[0].guid;
            return true;
        }
    }
    return false;
}

void GroupSystem_ConvertToRaid(uint32_t groupId) {
    Group* g = GroupSystem_GetById(groupId);
    if (g) { g->type = GROUP_TYPE_RAID; printf("[Group] Group %u converted to raid\n", groupId); }
}

void GroupSystem_SetLootMethod(uint32_t groupId, LootMethod method) {
    Group* g = GroupSystem_GetById(groupId);
    if (g) g->lootMethod = method;
}

void GroupSystem_SetTargetIcon(uint32_t groupId, int index, uint64_t targetGuid) {
    Group* g = GroupSystem_GetById(groupId);
    if (g && index >= 0 && index < 8) g->targetIcons[index] = (uint32_t)targetGuid;
}

bool GroupSystem_IsLeader(uint32_t groupId, uint64_t guid) {
    Group* g = GroupSystem_GetById(groupId);
    return g && g->leaderGuid == guid;
}
