/* loot_system.c -- Loot system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LOOT_ITEMS 16
#define MAX_LOOT_TEMPLATES 1024

typedef struct LootItem {
    uint32_t itemEntry;
    uint32_t minCount;
    uint32_t maxCount;
    float    dropChance;     /* 0.0 - 100.0 */
    uint32_t groupId;        /* items in same group are exclusive */
    bool     questItem;
} LootItem;

typedef struct LootTemplate {
    uint32_t  creatureEntry;
    LootItem  items[MAX_LOOT_ITEMS];
    int       itemCount;
    uint32_t  goldMin;
    uint32_t  goldMax;
} LootTemplate;

typedef struct LootInstance {
    uint64_t  ownerGuid;    /* creature GUID */
    uint32_t  items[MAX_LOOT_ITEMS];
    uint32_t  counts[MAX_LOOT_ITEMS];
    int       itemCount;
    uint32_t  gold;
    bool      looted[MAX_LOOT_ITEMS];
    bool      goldLooted;
    bool      released;
} LootInstance;

static LootTemplate g_lootTemplates[MAX_LOOT_TEMPLATES];
static int g_lootCount = 0;

void LootSystem_Init(void) {
    g_lootCount = 0;
    memset(g_lootTemplates, 0, sizeof(g_lootTemplates));
    srand((unsigned int)time(NULL));

    /* Demo loot tables */
    LootTemplate* lt;

    /* Kobold Vermin drops */
    lt = &g_lootTemplates[g_lootCount++];
    lt->creatureEntry = 6; lt->goldMin = 1; lt->goldMax = 5;
    lt->items[0] = (LootItem){ 50001, 1, 1, 25.0f, 0, false }; /* Welcome Gift */
    lt->itemCount = 1;

    /* World Boss drops */
    lt = &g_lootTemplates[g_lootCount++];
    lt->creatureEntry = 9999; lt->goldMin = 10000; lt->goldMax = 50000;
    lt->items[0] = (LootItem){ 49623, 1, 1, 10.0f, 0, false }; /* Shadowmourne */
    lt->items[1] = (LootItem){ 50000, 1, 5, 100.0f, 0, false }; /* Server Token */
    lt->itemCount = 2;

    printf("[Loot] Registered %d loot templates\n", g_lootCount);
}

LootTemplate* LootSystem_GetTemplate(uint32_t creatureEntry) {
    for (int i = 0; i < g_lootCount; i++)
        if (g_lootTemplates[i].creatureEntry == creatureEntry) return &g_lootTemplates[i];
    return NULL;
}

LootInstance* LootSystem_Generate(uint32_t creatureEntry, uint64_t creatureGuid) {
    LootTemplate* lt = LootSystem_GetTemplate(creatureEntry);
    if (!lt) return NULL;

    LootInstance* li = (LootInstance*)calloc(1, sizeof(LootInstance));
    li->ownerGuid = creatureGuid;

    /* Roll gold */
    if (lt->goldMax > lt->goldMin)
        li->gold = lt->goldMin + (uint32_t)(rand() % (lt->goldMax - lt->goldMin + 1));
    else
        li->gold = lt->goldMin;

    /* Roll items */
    for (int i = 0; i < lt->itemCount; i++) {
        float roll = (float)(rand() % 10000) / 100.0f; /* 0.00 - 99.99 */
        if (roll < lt->items[i].dropChance) {
            uint32_t count = lt->items[i].minCount;
            if (lt->items[i].maxCount > lt->items[i].minCount)
                count += (uint32_t)(rand() % (lt->items[i].maxCount - lt->items[i].minCount + 1));
            li->items[li->itemCount] = lt->items[i].itemEntry;
            li->counts[li->itemCount] = count;
            li->itemCount++;
        }
    }

    printf("[Loot] Generated loot for creature %u: %d items, %u gold\n",
           creatureEntry, li->itemCount, li->gold);
    return li;
}

bool LootSystem_TakeItem(LootInstance* li, int slotIndex) {
    if (!li || slotIndex < 0 || slotIndex >= li->itemCount) return false;
    if (li->looted[slotIndex]) return false;
    li->looted[slotIndex] = true;
    return true;
}

bool LootSystem_TakeGold(LootInstance* li) {
    if (!li || li->goldLooted) return false;
    li->goldLooted = true;
    return true;
}

bool LootSystem_IsEmpty(LootInstance* li) {
    if (!li) return true;
    if (!li->goldLooted && li->gold > 0) return false;
    for (int i = 0; i < li->itemCount; i++)
        if (!li->looted[i]) return false;
    return true;
}

void LootSystem_FreeLoot(LootInstance* li) { free(li); }
