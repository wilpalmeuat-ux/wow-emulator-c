/* pet_system.c -- Pet and companion system for WoW 3.3.5a
 * Hunter pets, warlock demons, DK ghouls, and non-combat companions.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PETS 5000
#define MAX_PET_SPELLS 10
#define MAX_PET_TALENTS 20

typedef enum { PET_HUNTER=0, PET_WARLOCK=1, PET_DK=2, PET_MAGE=3, PET_COMPANION=4 } PetType;
typedef enum { PET_MODE_PASSIVE=0, PET_MODE_DEFENSIVE=1, PET_MODE_AGGRESSIVE=2 } PetMode;
typedef enum { PET_ACT_STAY=0, PET_ACT_FOLLOW=1, PET_ACT_ATTACK=2 } PetAction;
typedef enum { PET_FAMILY_WOLF=1, PET_FAMILY_CAT=2, PET_FAMILY_SPIDER=3, PET_FAMILY_BEAR=4,
               PET_FAMILY_BOAR=5, PET_FAMILY_RAPTOR=7, PET_FAMILY_SERPENT=35,
               PET_FAMILY_DEVILSAUR=39, PET_FAMILY_SPIRIT_BEAST=46 } PetFamily;

typedef struct Pet {
    uint64_t    guid;
    uint64_t    ownerGuid;
    uint32_t    entry;          /* creature entry */
    char        name[64];
    PetType     type;
    PetFamily   family;
    uint32_t    level;
    uint32_t    xp;
    uint32_t    health;
    uint32_t    maxHealth;
    uint32_t    mana;
    uint32_t    maxMana;
    uint32_t    happiness;      /* 0-1000 for hunter pets */
    uint32_t    loyalty;        /* training points */
    uint32_t    displayId;
    float       x, y, z;
    float       orientation;
    MapId       mapId;
    PetMode     mode;
    PetAction   action;
    uint32_t    spells[MAX_PET_SPELLS];
    int         spellCount;
    bool        alive;
    bool        summoned;
    bool        active;
} Pet;

static Pet g_pets[MAX_PETS];
static int g_petCount = 0;
static uint64_t g_nextPetGuid = 0x5000000000000000ULL;

void PetSystem_Init(void) {
    memset(g_pets, 0, sizeof(g_pets));
    g_petCount = 0;
    printf("[Pet] Pet system initialized\n");
}

Pet* PetSystem_Create(uint64_t ownerGuid, uint32_t creatureEntry,
                       const char* name, PetType type, PetFamily family) {
    if (g_petCount >= MAX_PETS) return NULL;
    Pet* pet = &g_pets[g_petCount++];
    memset(pet, 0, sizeof(Pet));
    pet->guid = g_nextPetGuid++;
    pet->ownerGuid = ownerGuid;
    pet->entry = creatureEntry;
    strncpy(pet->name, name ? name : "Pet", 63);
    pet->type = type;
    pet->family = family;
    pet->level = 1;
    pet->health = 100;
    pet->maxHealth = 100;
    pet->happiness = 500;
    pet->mode = PET_MODE_DEFENSIVE;
    pet->action = PET_ACT_FOLLOW;
    pet->alive = true;
    pet->summoned = false;
    pet->active = true;

    printf("[Pet] Created pet '%s' (entry %u, type %d) for owner %llu\n",
           pet->name, creatureEntry, type, (unsigned long long)ownerGuid);
    return pet;
}

Pet* PetSystem_GetByOwner(uint64_t ownerGuid) {
    for (int i = 0; i < g_petCount; i++)
        if (g_pets[i].ownerGuid == ownerGuid && g_pets[i].active && g_pets[i].summoned)
            return &g_pets[i];
    return NULL;
}

void PetSystem_Summon(Pet* pet) {
    if (!pet) return;
    pet->summoned = true;
    pet->alive = true;
    printf("[Pet] '%s' summoned\n", pet->name);
}

void PetSystem_Dismiss(Pet* pet) {
    if (!pet) return;
    pet->summoned = false;
    printf("[Pet] '%s' dismissed\n", pet->name);
}

void PetSystem_Rename(Pet* pet, const char* newName) {
    if (!pet || !newName) return;
    printf("[Pet] '%s' renamed to '%s'\n", pet->name, newName);
    strncpy(pet->name, newName, 63);
}

void PetSystem_Feed(Pet* pet, uint32_t foodQuality) {
    if (!pet || pet->type != PET_HUNTER) return;
    uint32_t gain = foodQuality * 50;
    pet->happiness += gain;
    if (pet->happiness > 1000) pet->happiness = 1000;
    printf("[Pet] '%s' fed (happiness: %u)\n", pet->name, pet->happiness);
}

void PetSystem_SetMode(Pet* pet, PetMode mode) {
    if (pet) { pet->mode = mode; }
}

void PetSystem_SetAction(Pet* pet, PetAction action) {
    if (pet) { pet->action = action; }
}

void PetSystem_LearnSpell(Pet* pet, uint32_t spellId) {
    if (!pet || pet->spellCount >= MAX_PET_SPELLS) return;
    pet->spells[pet->spellCount++] = spellId;
    printf("[Pet] '%s' learned spell %u\n", pet->name, spellId);
}

void PetSystem_GainXP(Pet* pet, uint32_t xp) {
    if (!pet) return;
    pet->xp += xp;
    uint32_t xpNeeded = pet->level * 200;
    while (pet->xp >= xpNeeded && pet->level < 80) {
        pet->xp -= xpNeeded;
        pet->level++;
        pet->maxHealth += 50;
        pet->health = pet->maxHealth;
        xpNeeded = pet->level * 200;
        printf("[Pet] '%s' leveled up to %u\n", pet->name, pet->level);
    }
}
