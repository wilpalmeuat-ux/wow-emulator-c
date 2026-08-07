/* achievement_system.c -- Achievement system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ACHIEVEMENTS 4096
#define MAX_CRITERIA 8192
#define MAX_PLAYER_ACHIEVEMENTS 512

typedef enum {
    CRITERIA_KILL_CREATURE=0, CRITERIA_COMPLETE_QUEST=1, CRITERIA_REACH_LEVEL=5,
    CRITERIA_EXPLORE_AREA=7, CRITERIA_LOOT_ITEM=36, CRITERIA_EQUIP_ITEM=57,
    CRITERIA_EARN_GOLD=62, CRITERIA_COMPLETE_ACHIEVEMENT=110,
    CRITERIA_DEATHS=113, CRITERIA_TOTAL_KILLS=130, CRITERIA_FALL_DISTANCE=141
} CriteriaType;

typedef struct AchievementCriteria {
    uint32_t     id;
    uint32_t     achievementId;
    CriteriaType type;
    uint32_t     requirement;    /* target entry/value */
    uint32_t     count;          /* required count */
} AchievementCriteria;

typedef struct AchievementDefinition {
    uint32_t id;
    char     name[128];
    char     description[256];
    uint32_t points;
    uint32_t factionReq;     /* 0=both, 1=alliance, 2=horde */
    uint32_t parentId;       /* category */
    uint32_t criteriaIds[8]; /* up to 8 criteria */
    int      criteriaCount;
    uint32_t rewardTitle;
    uint32_t rewardSpell;
    uint32_t rewardItem;
} AchievementDefinition;

typedef struct PlayerCriteriaProgress {
    uint32_t criteriaId;
    uint32_t counter;
    time_t   date;
} PlayerCriteriaProgress;

typedef struct PlayerAchievement {
    uint32_t achievementId;
    time_t   completedDate;
} PlayerAchievement;

typedef struct AchievementData {
    PlayerAchievement completed[MAX_PLAYER_ACHIEVEMENTS];
    int               completedCount;
    PlayerCriteriaProgress criteria[MAX_CRITERIA];
    int               criteriaCount;
    uint32_t          totalPoints;
} AchievementData;

/* Global achievement registry */
static AchievementDefinition g_achievements[MAX_ACHIEVEMENTS];
static AchievementCriteria g_criteria[MAX_CRITERIA];
static int g_achievementCount = 0;
static int g_criteriaCount = 0;

void AchievementSystem_Init(void) {
    g_achievementCount = 0;
    g_criteriaCount = 0;
    memset(g_achievements, 0, sizeof(g_achievements));

    /* Register demo achievements */
    AchievementDefinition* a;
    AchievementCriteria* c;

    /* Level 10 */
    c = &g_criteria[g_criteriaCount];
    c->id = 1; c->achievementId = 1; c->type = CRITERIA_REACH_LEVEL; c->requirement = 10; c->count = 1;
    g_criteriaCount++;

    a = &g_achievements[g_achievementCount++];
    a->id = 1; strncpy(a->name, "Level 10", 127); a->points = 10;
    a->criteriaIds[0] = 1; a->criteriaCount = 1;

    /* Level 80 */
    c = &g_criteria[g_criteriaCount];
    c->id = 2; c->achievementId = 2; c->type = CRITERIA_REACH_LEVEL; c->requirement = 80; c->count = 1;
    g_criteriaCount++;

    a = &g_achievements[g_achievementCount++];
    a->id = 2; strncpy(a->name, "Level 80", 127); a->points = 10;
    a->criteriaIds[0] = 2; a->criteriaCount = 1;

    /* First Kill */
    c = &g_criteria[g_criteriaCount];
    c->id = 3; c->achievementId = 3; c->type = CRITERIA_TOTAL_KILLS; c->requirement = 0; c->count = 1;
    g_criteriaCount++;

    a = &g_achievements[g_achievementCount++];
    a->id = 3; strncpy(a->name, "First Blood", 127); a->points = 10;
    strncpy(a->description, "Kill your first enemy.", 255);
    a->criteriaIds[0] = 3; a->criteriaCount = 1;

    /* 1000 Kills */
    c = &g_criteria[g_criteriaCount];
    c->id = 4; c->achievementId = 4; c->type = CRITERIA_TOTAL_KILLS; c->requirement = 0; c->count = 1000;
    g_criteriaCount++;

    a = &g_achievements[g_achievementCount++];
    a->id = 4; strncpy(a->name, "The Thousand", 127); a->points = 25;
    a->criteriaIds[0] = 4; a->criteriaCount = 1;

    /* 100 Quests */
    c = &g_criteria[g_criteriaCount];
    c->id = 5; c->achievementId = 5; c->type = CRITERIA_COMPLETE_QUEST; c->requirement = 0; c->count = 100;
    g_criteriaCount++;

    a = &g_achievements[g_achievementCount++];
    a->id = 5; strncpy(a->name, "Loremaster (100 Quests)", 127); a->points = 25;
    a->criteriaIds[0] = 5; a->criteriaCount = 1;

    printf("[Achievement] Registered %d achievements, %d criteria\n", g_achievementCount, g_criteriaCount);
}

void AchievementData_Init(AchievementData* ad) {
    memset(ad, 0, sizeof(AchievementData));
}

bool AchievementSystem_IsCompleted(AchievementData* ad, uint32_t achievementId) {
    for (int i = 0; i < ad->completedCount; i++)
        if (ad->completed[i].achievementId == achievementId) return true;
    return false;
}

static PlayerCriteriaProgress* _findOrCreateProgress(AchievementData* ad, uint32_t criteriaId) {
    for (int i = 0; i < ad->criteriaCount; i++)
        if (ad->criteria[i].criteriaId == criteriaId) return &ad->criteria[i];
    if (ad->criteriaCount < MAX_CRITERIA) {
        PlayerCriteriaProgress* p = &ad->criteria[ad->criteriaCount++];
        p->criteriaId = criteriaId;
        p->counter = 0;
        return p;
    }
    return NULL;
}

/* Update criteria and check for achievement completion */
void AchievementSystem_UpdateCriteria(AchievementData* ad, CriteriaType type,
                                       uint32_t value, uint32_t count) {
    for (int i = 0; i < g_criteriaCount; i++) {
        AchievementCriteria* crit = &g_criteria[i];
        if (crit->type != type) continue;
        if (crit->requirement != 0 && crit->requirement != value) continue;

        PlayerCriteriaProgress* prog = _findOrCreateProgress(ad, crit->id);
        if (!prog) continue;
        prog->counter += count;
        prog->date = time(NULL);

        /* Check if criteria is met */
        if (prog->counter >= crit->count) {
            /* Check if achievement is completed */
            if (!AchievementSystem_IsCompleted(ad, crit->achievementId)) {
                /* Find the achievement */
                for (int j = 0; j < g_achievementCount; j++) {
                    if (g_achievements[j].id == crit->achievementId) {
                        /* Check all criteria for this achievement */
                        bool allMet = true;
                        for (int k = 0; k < g_achievements[j].criteriaCount; k++) {
                            uint32_t cid = g_achievements[j].criteriaIds[k];
                            PlayerCriteriaProgress* cp = NULL;
                            for (int m = 0; m < ad->criteriaCount; m++) {
                                if (ad->criteria[m].criteriaId == cid) { cp = &ad->criteria[m]; break; }
                            }
                            if (!cp) { allMet = false; break; }
                            /* Find the criteria definition */
                            for (int m = 0; m < g_criteriaCount; m++) {
                                if (g_criteria[m].id == cid && cp->counter < g_criteria[m].count) {
                                    allMet = false; break;
                                }
                            }
                            if (!allMet) break;
                        }
                        if (allMet && ad->completedCount < MAX_PLAYER_ACHIEVEMENTS) {
                            ad->completed[ad->completedCount].achievementId = g_achievements[j].id;
                            ad->completed[ad->completedCount].completedDate = time(NULL);
                            ad->completedCount++;
                            ad->totalPoints += g_achievements[j].points;
                            printf("[Achievement] EARNED: %s (+%u points, total: %u)\n",
                                   g_achievements[j].name, g_achievements[j].points, ad->totalPoints);
                        }
                        break;
                    }
                }
            }
        }
    }
}
