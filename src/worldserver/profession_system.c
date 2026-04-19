/* profession_system.c -- Trade skill / profession system for WoW 3.3.5a */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROFESSIONS 11
#define MAX_RECIPES 2048
#define MAX_PLAYER_SKILLS 128

typedef enum {
    SKILL_ALCHEMY=171, SKILL_BLACKSMITHING=164, SKILL_ENCHANTING=333,
    SKILL_ENGINEERING=202, SKILL_HERBALISM=182, SKILL_INSCRIPTION=773,
    SKILL_JEWELCRAFTING=755, SKILL_LEATHERWORKING=165, SKILL_MINING=186,
    SKILL_SKINNING=393, SKILL_TAILORING=197,
    SKILL_COOKING=185, SKILL_FIRST_AID=129, SKILL_FISHING=356, SKILL_RIDING=762
} SkillType;

typedef struct Recipe {
    uint32_t  id;
    uint32_t  skillId;
    uint32_t  skillRequired;     /* min skill to learn */
    uint32_t  resultItemEntry;
    uint32_t  resultCount;
    uint32_t  reagents[8];       /* item entries */
    uint32_t  reagentCounts[8];
    int       reagentCount;
    uint32_t  castTime;          /* ms */
    char      name[64];
    uint32_t  orangeSkill;       /* guaranteed skill-up */
    uint32_t  yellowSkill;       /* likely skill-up */
    uint32_t  greenSkill;        /* unlikely skill-up */
    uint32_t  graySkill;         /* no skill-up */
} Recipe;

typedef struct PlayerSkill {
    uint32_t skillId;
    uint32_t current;
    uint32_t maximum;       /* 75/150/225/300/375/450 */
} PlayerSkill;

typedef struct ProfessionData {
    PlayerSkill skills[MAX_PLAYER_SKILLS];
    int         skillCount;
    uint32_t    knownRecipes[MAX_RECIPES]; /* recipe IDs */
    int         recipeCount;
} ProfessionData;

static Recipe g_recipes[MAX_RECIPES];
static int g_recipeCount = 0;

typedef struct ProfessionInfo {
    SkillType id;
    char      name[32];
    bool      isPrimary;    /* primary = 2 max, secondary = unlimited */
} ProfessionInfo;

static const ProfessionInfo g_professions[] = {
    { SKILL_ALCHEMY,        "Alchemy",         true },
    { SKILL_BLACKSMITHING,  "Blacksmithing",   true },
    { SKILL_ENCHANTING,     "Enchanting",      true },
    { SKILL_ENGINEERING,    "Engineering",      true },
    { SKILL_HERBALISM,      "Herbalism",        true },
    { SKILL_INSCRIPTION,    "Inscription",      true },
    { SKILL_JEWELCRAFTING,  "Jewelcrafting",    true },
    { SKILL_LEATHERWORKING, "Leatherworking",   true },
    { SKILL_MINING,         "Mining",            true },
    { SKILL_SKINNING,       "Skinning",          true },
    { SKILL_TAILORING,      "Tailoring",         true },
    { SKILL_COOKING,        "Cooking",           false },
    { SKILL_FIRST_AID,      "First Aid",         false },
    { SKILL_FISHING,        "Fishing",           false },
};

void ProfessionSystem_Init(void) {
    g_recipeCount = 0;
    memset(g_recipes, 0, sizeof(g_recipes));

    /* Demo recipes */
    Recipe* r;

    r = &g_recipes[g_recipeCount++];
    r->id = 2329; r->skillId = SKILL_ALCHEMY; r->skillRequired = 1;
    strncpy(r->name, "Minor Healing Potion", 63);
    r->resultItemEntry = 118; r->resultCount = 1;
    r->reagents[0] = 2447; r->reagentCounts[0] = 1; /* Peacebloom */
    r->reagents[1] = 3371; r->reagentCounts[1] = 1; /* Empty Vial */
    r->reagentCount = 2;
    r->orangeSkill = 1; r->yellowSkill = 55; r->greenSkill = 75; r->graySkill = 95;

    r = &g_recipes[g_recipeCount++];
    r->id = 2660; r->skillId = SKILL_BLACKSMITHING; r->skillRequired = 1;
    strncpy(r->name, "Rough Sharpening Stone", 63);
    r->resultItemEntry = 2862; r->resultCount = 1;
    r->reagents[0] = 2835; r->reagentCounts[0] = 1; /* Rough Stone */
    r->reagentCount = 1;

    r = &g_recipes[g_recipeCount++];
    r->id = 7751; r->skillId = SKILL_COOKING; r->skillRequired = 1;
    strncpy(r->name, "Brilliant Smallfish", 63);
    r->resultItemEntry = 6290; r->resultCount = 1;
    r->reagents[0] = 6291; r->reagentCounts[0] = 1; /* Raw Brilliant Smallfish */
    r->reagentCount = 1;

    printf("[Profession] Registered %d recipes\n", g_recipeCount);
}

void ProfessionData_Init(ProfessionData* pd) {
    memset(pd, 0, sizeof(ProfessionData));
}

bool Profession_LearnSkill(ProfessionData* pd, uint32_t skillId) {
    /* Check if already known */
    for (int i = 0; i < pd->skillCount; i++)
        if (pd->skills[i].skillId == skillId) return false;
    if (pd->skillCount >= MAX_PLAYER_SKILLS) return false;

    PlayerSkill* s = &pd->skills[pd->skillCount++];
    s->skillId = skillId;
    s->current = 1;
    s->maximum = 75; /* Apprentice */
    printf("[Profession] Learned skill %u (max %u)\n", skillId, s->maximum);
    return true;
}

PlayerSkill* Profession_GetSkill(ProfessionData* pd, uint32_t skillId) {
    for (int i = 0; i < pd->skillCount; i++)
        if (pd->skills[i].skillId == skillId) return &pd->skills[i];
    return NULL;
}

bool Profession_LearnRecipe(ProfessionData* pd, uint32_t recipeId) {
    if (pd->recipeCount >= MAX_RECIPES) return false;
    for (int i = 0; i < pd->recipeCount; i++)
        if (pd->knownRecipes[i] == recipeId) return false;
    pd->knownRecipes[pd->recipeCount++] = recipeId;
    return true;
}

bool Profession_Craft(ProfessionData* pd, uint32_t recipeId) {
    Recipe* recipe = NULL;
    for (int i = 0; i < g_recipeCount; i++) {
        if (g_recipes[i].id == recipeId) { recipe = &g_recipes[i]; break; }
    }
    if (!recipe) return false;

    PlayerSkill* skill = Profession_GetSkill(pd, recipe->skillId);
    if (!skill || skill->current < recipe->skillRequired) return false;

    /* Skill-up chance */
    if (skill->current < skill->maximum) {
        bool skillUp = false;
        if (skill->current < recipe->orangeSkill) skillUp = true;
        else if (skill->current < recipe->yellowSkill) skillUp = (rand() % 100) < 75;
        else if (skill->current < recipe->greenSkill) skillUp = (rand() % 100) < 25;
        if (skillUp) {
            skill->current++;
            printf("[Profession] Skill up! %u -> %u\n", skill->skillId, skill->current);
        }
    }

    printf("[Profession] Crafted: %s\n", recipe->name);
    return true;
}
