/* talent_system.c -- Talent tree system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TALENT_TREES 3
#define MAX_TALENTS_PER_TREE 30
#define MAX_TALENT_RANKS 5
#define MAX_TALENT_SPECS 2   /* dual spec in 3.3.5 */

typedef struct TalentDefinition {
    uint32_t id;
    uint32_t treeIndex;      /* 0, 1, 2 */
    uint32_t row;            /* tier (0-10) */
    uint32_t column;         /* position in row (0-3) */
    uint32_t maxRank;
    uint32_t spellIds[MAX_TALENT_RANKS]; /* spell per rank */
    uint32_t prereqTalentId; /* talent that must be learned first */
    uint32_t prereqRank;
    char     name[64];
} TalentDefinition;

typedef struct TalentSpec {
    uint8_t  points[MAX_TALENT_TREES][MAX_TALENTS_PER_TREE]; /* ranks learned */
    uint32_t pointsSpent[MAX_TALENT_TREES]; /* total per tree */
    uint32_t totalPointsSpent;
} TalentSpec;

typedef struct TalentData {
    TalentSpec specs[MAX_TALENT_SPECS];
    int        activeSpec;
    uint32_t   freePoints;
} TalentData;

/* Class talent tree names */
static const char* g_talentTreeNames[11][3] = {
    { "", "", "" },                          /* 0: unused */
    { "Arms", "Fury", "Protection" },       /* 1: Warrior */
    { "Holy", "Protection", "Retribution" },/* 2: Paladin */
    { "Beast Mastery", "Marksmanship", "Survival" }, /* 3: Hunter */
    { "Assassination", "Combat", "Subtlety" },       /* 4: Rogue */
    { "Discipline", "Holy", "Shadow" },     /* 5: Priest */
    { "Blood", "Frost", "Unholy" },         /* 6: DK */
    { "Elemental", "Enhancement", "Restoration" },   /* 7: Shaman */
    { "Arcane", "Fire", "Frost" },          /* 8: Mage */
    { "Affliction", "Demonology", "Destruction" },    /* 9: Warlock */
    { "", "", "" },                          /* 10: unused */
};

void TalentSystem_Init(void) {
    printf("[Talent] Talent system initialized (dual spec enabled)\n");
}

void TalentData_Init(TalentData* td) {
    memset(td, 0, sizeof(TalentData));
    td->activeSpec = 0;
    td->freePoints = 0;
}

/* Calculate free talent points based on level */
uint32_t TalentSystem_GetFreePoints(uint32_t level) {
    if (level < 10) return 0;
    return level - 9; /* 1 point per level starting at 10, max 71 at level 80 */
}

bool TalentSystem_LearnTalent(TalentData* td, uint32_t treeIndex, uint32_t talentIndex) {
    if (treeIndex >= MAX_TALENT_TREES || talentIndex >= MAX_TALENTS_PER_TREE) return false;
    if (td->freePoints == 0) return false;

    TalentSpec* spec = &td->specs[td->activeSpec];

    /* Check if already at max rank (simplified: max 5) */
    if (spec->points[treeIndex][talentIndex] >= MAX_TALENT_RANKS) return false;

    /* Check prerequisite: need 5 points per tier */
    uint32_t tier = talentIndex / 4; /* rough tier calculation */
    if (spec->pointsSpent[treeIndex] < tier * 5) return false;

    spec->points[treeIndex][talentIndex]++;
    spec->pointsSpent[treeIndex]++;
    spec->totalPointsSpent++;
    td->freePoints--;

    printf("[Talent] Learned talent tree=%u idx=%u (rank %u, %u free points left)\n",
           treeIndex, talentIndex, spec->points[treeIndex][talentIndex], td->freePoints);
    return true;
}

void TalentSystem_Reset(TalentData* td) {
    TalentSpec* spec = &td->specs[td->activeSpec];
    td->freePoints += spec->totalPointsSpent;
    memset(spec, 0, sizeof(TalentSpec));
    printf("[Talent] Talents reset (%u points refunded)\n", td->freePoints);
}

void TalentSystem_SwitchSpec(TalentData* td) {
    td->activeSpec = (td->activeSpec + 1) % MAX_TALENT_SPECS;
    printf("[Talent] Switched to spec %d\n", td->activeSpec);
}

const char* TalentSystem_GetTreeName(uint32_t classId, uint32_t treeIndex) {
    if (classId > 10 || treeIndex > 2) return "Unknown";
    return g_talentTreeNames[classId][treeIndex];
}
