/* phasing_system.c -- Quest-based phasing for WoW 3.3.5a (WotLK feature)
 * Players can be in different "phases" of the same zone, seeing different
 * NPCs and objects based on quest progress.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PHASE_DEFINITIONS 256
#define MAX_PHASE_OBJECTS 64
#define DEFAULT_PHASE 0x00000001

typedef struct PhaseDefinition {
    uint32_t phaseId;         /* bitmask value */
    uint32_t zoneId;          /* affected zone */
    uint32_t requiredQuestId; /* quest that activates this phase */
    bool     requireComplete; /* true = must be completed, false = just accepted */
    char     description[128];
} PhaseDefinition;

typedef struct PhaseObjectOverride {
    uint64_t guid;
    uint32_t phaseMask;       /* phase(s) where this object exists */
    uint32_t entry;
    bool     isCreature;      /* true=creature, false=gameobject */
} PhaseObjectOverride;

static PhaseDefinition g_phases[MAX_PHASE_DEFINITIONS];
static int g_phaseCount = 0;
static PhaseObjectOverride g_phaseObjects[MAX_PHASE_OBJECTS * 16];
static int g_phaseObjectCount = 0;

void PhasingSystem_Init(void) {
    memset(g_phases, 0, sizeof(g_phases));
    g_phaseCount = 0;
    g_phaseObjectCount = 0;

    /* Demo phases */
    PhaseDefinition* p;

    /* Dragonblight: Wrathgate event phases */
    p = &g_phases[g_phaseCount++];
    p->phaseId = 0x00000002; p->zoneId = 65; /* Dragonblight */
    p->requiredQuestId = 12499; p->requireComplete = true;
    strncpy(p->description, "Post-Wrathgate phase", 127);

    /* Icecrown: quest progression */
    p = &g_phases[g_phaseCount++];
    p->phaseId = 0x00000004; p->zoneId = 210; /* Icecrown */
    p->requiredQuestId = 13141; p->requireComplete = false;
    strncpy(p->description, "Icecrown quest phase 1", 127);

    p = &g_phases[g_phaseCount++];
    p->phaseId = 0x00000008; p->zoneId = 210;
    p->requiredQuestId = 13264; p->requireComplete = true;
    strncpy(p->description, "Icecrown quest phase 2", 127);

    /* Storm Peaks */
    p = &g_phases[g_phaseCount++];
    p->phaseId = 0x00000010; p->zoneId = 67; /* Storm Peaks */
    p->requiredQuestId = 12843; p->requireComplete = true;
    strncpy(p->description, "Storm Peaks Thorim phase", 127);

    printf("[Phase] Registered %d phase definitions\n", g_phaseCount);
}

/* Calculate the phase mask for a player based on quest progress */
uint32_t PhasingSystem_CalculatePhaseMask(uint64_t playerGuid, uint32_t zoneId,
                                           const uint32_t* completedQuests, int questCount,
                                           const uint32_t* activeQuests, int activeQuestCount) {
    uint32_t phaseMask = DEFAULT_PHASE; /* always include default phase */
    (void)playerGuid;

    for (int i = 0; i < g_phaseCount; i++) {
        PhaseDefinition* pd = &g_phases[i];
        if (pd->zoneId != zoneId) continue;

        bool hasQuest = false;
        if (pd->requireComplete) {
            for (int q = 0; q < questCount; q++) {
                if (completedQuests[q] == pd->requiredQuestId) { hasQuest = true; break; }
            }
        } else {
            for (int q = 0; q < activeQuestCount; q++) {
                if (activeQuests[q] == pd->requiredQuestId) { hasQuest = true; break; }
            }
        }

        if (hasQuest) {
            phaseMask |= pd->phaseId;
        }
    }

    return phaseMask;
}

/* Check if a player can see an object */
bool PhasingSystem_CanSee(uint32_t playerPhaseMask, uint32_t objectPhaseMask) {
    return (playerPhaseMask & objectPhaseMask) != 0;
}

/* Register a phased object */
void PhasingSystem_AddObject(uint64_t guid, uint32_t phaseMask, uint32_t entry, bool isCreature) {
    if (g_phaseObjectCount >= MAX_PHASE_OBJECTS * 16) return;
    PhaseObjectOverride* po = &g_phaseObjects[g_phaseObjectCount++];
    po->guid = guid;
    po->phaseMask = phaseMask;
    po->entry = entry;
    po->isCreature = isCreature;
}

/* Get the phase mask for a specific object */
uint32_t PhasingSystem_GetObjectPhase(uint64_t guid) {
    for (int i = 0; i < g_phaseObjectCount; i++) {
        if (g_phaseObjects[i].guid == guid) return g_phaseObjects[i].phaseMask;
    }
    return DEFAULT_PHASE; /* default: visible in all phases */
}
