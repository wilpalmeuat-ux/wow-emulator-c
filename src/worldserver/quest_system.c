/* quest_system.c -- Quest system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUEST_OBJECTIVES 4
#define MAX_QUEST_REWARDS    6
#define MAX_PLAYER_QUESTS    25
#define MAX_QUEST_TEMPLATES  2048

typedef enum { QUEST_OBJ_KILL=0, QUEST_OBJ_ITEM=1, QUEST_OBJ_EXPLORE=2, QUEST_OBJ_INTERACT=3, QUEST_OBJ_CAST=4 } QuestObjType;
typedef enum { QUEST_STATUS_NONE=0, QUEST_STATUS_AVAILABLE, QUEST_STATUS_ACTIVE, QUEST_STATUS_COMPLETE, QUEST_STATUS_FAILED, QUEST_STATUS_REWARDED } QuestStatus;

typedef struct {
    QuestObjType type;
    uint32_t     targetEntry;   /* creature/item/go entry */
    uint32_t     count;
    char         description[128];
} QuestObjective;

typedef struct QuestTemplate {
    uint32_t id;
    char     title[128];
    char     description[512];
    char     completionText[256];
    uint32_t requiredLevel;
    uint32_t suggestedPlayers;
    uint32_t rewardXP;
    uint32_t rewardGold;
    uint32_t rewardItems[MAX_QUEST_REWARDS];
    uint32_t rewardItemCounts[MAX_QUEST_REWARDS];
    uint32_t rewardSpell;
    uint32_t questGiverEntry;  /* NPC that gives this quest */
    uint32_t questEnderEntry;  /* NPC that completes this quest */
    QuestObjective objectives[MAX_QUEST_OBJECTIVES];
    int      objectiveCount;
    uint32_t prevQuestId;      /* prerequisite quest */
    uint32_t nextQuestId;      /* follow-up quest */
    uint32_t flags;
} QuestTemplate;

typedef struct {
    uint32_t questId;
    QuestStatus status;
    uint32_t progress[MAX_QUEST_OBJECTIVES]; /* current count per objective */
    time_t   acceptedTime;
} PlayerQuest;

typedef struct QuestLog {
    PlayerQuest quests[MAX_PLAYER_QUESTS];
    int         count;
} QuestLog;

static QuestTemplate g_questTemplates[MAX_QUEST_TEMPLATES];
static int g_questCount = 0;

void QuestSystem_Init(void) {
    g_questCount = 0;
    memset(g_questTemplates, 0, sizeof(g_questTemplates));

    /* Demo quests */
    QuestTemplate* q;

    q = &g_questTemplates[g_questCount++];
    q->id = 1; strncpy(q->title, "A Threat Within", 127);
    strncpy(q->description, "Report to Marshal McBride in Northshire Abbey.", 511);
    q->requiredLevel = 1; q->rewardXP = 40; q->rewardGold = 10;
    q->questGiverEntry = 50; q->questEnderEntry = 50;

    q = &g_questTemplates[g_questCount++];
    q->id = 2; strncpy(q->title, "Kobold Camp Cleanup", 127);
    strncpy(q->description, "Kill 10 Kobold Vermin and report back.", 511);
    q->requiredLevel = 1; q->rewardXP = 250; q->rewardGold = 100;
    q->objectives[0].type = QUEST_OBJ_KILL;
    q->objectives[0].targetEntry = 6; q->objectives[0].count = 10;
    strncpy(q->objectives[0].description, "Kobold Vermin slain", 127);
    q->objectiveCount = 1; q->prevQuestId = 1;

    q = &g_questTemplates[g_questCount++];
    q->id = 3; strncpy(q->title, "Investigate Echo Ridge", 127);
    strncpy(q->description, "Kill 10 Kobold Workers in Echo Ridge Mine.", 511);
    q->requiredLevel = 2; q->rewardXP = 360; q->rewardGold = 150;
    q->objectives[0].type = QUEST_OBJ_KILL;
    q->objectives[0].targetEntry = 7; q->objectives[0].count = 10;
    strncpy(q->objectives[0].description, "Kobold Workers slain", 127);
    q->objectiveCount = 1; q->prevQuestId = 2;
    q->rewardItems[0] = 25; q->rewardItemCounts[0] = 1;

    printf("[Quest] Registered %d quests\n", g_questCount);
}

QuestTemplate* QuestSystem_GetQuest(uint32_t id) {
    for (int i = 0; i < g_questCount; i++)
        if (g_questTemplates[i].id == id) return &g_questTemplates[i];
    return NULL;
}

void QuestLog_Init(QuestLog* ql) { memset(ql, 0, sizeof(QuestLog)); }

bool QuestLog_Accept(QuestLog* ql, uint32_t questId) {
    if (ql->count >= MAX_PLAYER_QUESTS) return false;
    QuestTemplate* qt = QuestSystem_GetQuest(questId);
    if (!qt) return false;
    for (int i = 0; i < ql->count; i++)
        if (ql->quests[i].questId == questId) return false; /* already have it */
    PlayerQuest* pq = &ql->quests[ql->count++];
    pq->questId = questId;
    pq->status = QUEST_STATUS_ACTIVE;
    pq->acceptedTime = time(NULL);
    printf("[Quest] Accepted: %s (ID %u)\n", qt->title, questId);
    return true;
}

bool QuestLog_Abandon(QuestLog* ql, uint32_t questId) {
    for (int i = 0; i < ql->count; i++) {
        if (ql->quests[i].questId == questId) {
            ql->quests[i] = ql->quests[ql->count - 1];
            ql->count--;
            return true;
        }
    }
    return false;
}

void QuestLog_UpdateKill(QuestLog* ql, uint32_t creatureEntry) {
    for (int i = 0; i < ql->count; i++) {
        PlayerQuest* pq = &ql->quests[i];
        if (pq->status != QUEST_STATUS_ACTIVE) continue;
        QuestTemplate* qt = QuestSystem_GetQuest(pq->questId);
        if (!qt) continue;
        for (int j = 0; j < qt->objectiveCount; j++) {
            if (qt->objectives[j].type == QUEST_OBJ_KILL &&
                qt->objectives[j].targetEntry == creatureEntry) {
                if (pq->progress[j] < qt->objectives[j].count) {
                    pq->progress[j]++;
                    printf("[Quest] %s: %u/%u %s\n", qt->title,
                           pq->progress[j], qt->objectives[j].count,
                           qt->objectives[j].description);
                }
            }
        }
        /* Check completion */
        bool complete = true;
        for (int j = 0; j < qt->objectiveCount; j++) {
            if (pq->progress[j] < qt->objectives[j].count) { complete = false; break; }
        }
        if (complete && pq->status == QUEST_STATUS_ACTIVE) {
            pq->status = QUEST_STATUS_COMPLETE;
            printf("[Quest] COMPLETE: %s\n", qt->title);
        }
    }
}

bool QuestLog_TurnIn(QuestLog* ql, uint32_t questId, uint32_t* xpReward, uint32_t* goldReward) {
    for (int i = 0; i < ql->count; i++) {
        if (ql->quests[i].questId == questId && ql->quests[i].status == QUEST_STATUS_COMPLETE) {
            QuestTemplate* qt = QuestSystem_GetQuest(questId);
            if (!qt) return false;
            *xpReward = qt->rewardXP;
            *goldReward = qt->rewardGold;
            ql->quests[i].status = QUEST_STATUS_REWARDED;
            ql->quests[i] = ql->quests[ql->count - 1];
            ql->count--;
            printf("[Quest] Turned in: %s (+%u XP, +%u gold)\n", qt->title, *xpReward, *goldReward);
            return true;
        }
    }
    return false;
}
