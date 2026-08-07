/* item_system.c -- Item and inventory system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INVENTORY_SLOTS 80      /* bag slots + equipped + bank */
#define EQUIPMENT_SLOT_START 0
#define EQUIPMENT_SLOT_END   18
#define BAG_SLOT_START       19
#define BAG_SLOT_END         22
#define BACKPACK_SLOT_START  23
#define BACKPACK_SLOT_END    38
#define BANK_SLOT_START      39
#define BANK_SLOT_END        66

typedef enum {
    ITEM_QUALITY_POOR    = 0, /* Gray */
    ITEM_QUALITY_COMMON  = 1, /* White */
    ITEM_QUALITY_UNCOMMON= 2, /* Green */
    ITEM_QUALITY_RARE    = 3, /* Blue */
    ITEM_QUALITY_EPIC    = 4, /* Purple */
    ITEM_QUALITY_LEGENDARY=5, /* Orange */
    ITEM_QUALITY_ARTIFACT= 6, /* Red */
    ITEM_QUALITY_HEIRLOOM= 7  /* Gold */
} ItemQuality;

typedef enum {
    INVTYPE_NON_EQUIP=0, INVTYPE_HEAD=1, INVTYPE_NECK=2, INVTYPE_SHOULDERS=3,
    INVTYPE_BODY=4, INVTYPE_CHEST=5, INVTYPE_WAIST=6, INVTYPE_LEGS=7,
    INVTYPE_FEET=8, INVTYPE_WRISTS=9, INVTYPE_HANDS=10, INVTYPE_FINGER=11,
    INVTYPE_TRINKET=12, INVTYPE_WEAPON=13, INVTYPE_SHIELD=14, INVTYPE_RANGED=15,
    INVTYPE_CLOAK=16, INVTYPE_2HWEAPON=17, INVTYPE_BAG=18, INVTYPE_TABARD=19,
    INVTYPE_ROBE=20, INVTYPE_WEAPONMAINHAND=21, INVTYPE_WEAPONOFFHAND=22,
    INVTYPE_HOLDABLE=23, INVTYPE_THROWN=25, INVTYPE_RANGEDRIGHT=26, INVTYPE_RELIC=28
} InvType;

typedef struct ItemTemplate {
    uint32_t entry;
    char     name[100];
    uint32_t displayId;
    ItemQuality quality;
    InvType  inventoryType;
    uint32_t itemLevel;
    uint32_t requiredLevel;
    int32_t  statType[10];
    int32_t  statValue[10];
    uint32_t armor;
    float    damageMin;
    float    damageMax;
    uint32_t attackSpeed;
    uint32_t buyPrice;
    uint32_t sellPrice;
    uint32_t maxStack;
    uint32_t bagSlots;      /* for bags */
    uint32_t spellId[5];    /* on-use/on-equip spells */
} ItemTemplate;

typedef struct ItemInstance {
    uint64_t guid;
    uint32_t entry;
    uint32_t stackCount;
    int32_t  durability;
    int32_t  maxDurability;
    uint32_t enchantments[3];
    uint32_t randomSuffix;
    uint32_t charges;
    bool     soulbound;
} ItemInstance;

typedef struct Inventory {
    ItemInstance slots[MAX_INVENTORY_SLOTS];
    int          slotCount;
    uint64_t     nextItemGuid;
} Inventory;

/* ================================================================ */
#define MAX_ITEM_TEMPLATES 2048
static ItemTemplate g_itemTemplates[MAX_ITEM_TEMPLATES];
static int g_itemTemplateCount = 0;

void ItemSystem_Init(void) {
    g_itemTemplateCount = 0;
    memset(g_itemTemplates, 0, sizeof(g_itemTemplates));

    /* Demo items */
    ItemTemplate* t;

    t = &g_itemTemplates[g_itemTemplateCount++];
    t->entry = 49623; strncpy(t->name, "Shadowmourne", 99);
    t->quality = ITEM_QUALITY_LEGENDARY; t->inventoryType = INVTYPE_2HWEAPON;
    t->itemLevel = 284; t->requiredLevel = 80; t->damageMin = 954; t->damageMax = 1592;
    t->attackSpeed = 3600; t->sellPrice = 500000;

    t = &g_itemTemplates[g_itemTemplateCount++];
    t->entry = 50000; strncpy(t->name, "Server Token", 99);
    t->quality = ITEM_QUALITY_EPIC; t->maxStack = 200; t->sellPrice = 100;

    t = &g_itemTemplates[g_itemTemplateCount++];
    t->entry = 50001; strncpy(t->name, "Welcome Gift", 99);
    t->quality = ITEM_QUALITY_UNCOMMON; t->maxStack = 1; t->sellPrice = 0;

    /* Basic equipment */
    t = &g_itemTemplates[g_itemTemplateCount++];
    t->entry = 25; strncpy(t->name, "Worn Shortsword", 99);
    t->quality = ITEM_QUALITY_POOR; t->inventoryType = INVTYPE_WEAPON;
    t->itemLevel = 2; t->damageMin = 2; t->damageMax = 5; t->attackSpeed = 1900;

    t = &g_itemTemplates[g_itemTemplateCount++];
    t->entry = 35; strncpy(t->name, "Bent Staff", 99);
    t->quality = ITEM_QUALITY_POOR; t->inventoryType = INVTYPE_2HWEAPON;
    t->itemLevel = 2; t->damageMin = 3; t->damageMax = 5; t->attackSpeed = 2900;

    printf("[Item] Registered %d item templates\n", g_itemTemplateCount);
}

ItemTemplate* ItemSystem_GetTemplate(uint32_t entry) {
    for (int i = 0; i < g_itemTemplateCount; i++)
        if (g_itemTemplates[i].entry == entry) return &g_itemTemplates[i];
    return NULL;
}

void Inventory_Init(Inventory* inv) {
    memset(inv, 0, sizeof(Inventory));
    inv->slotCount = MAX_INVENTORY_SLOTS;
    inv->nextItemGuid = 0x4000000000000000ULL;
}

int Inventory_FindFreeSlot(Inventory* inv) {
    for (int i = BACKPACK_SLOT_START; i <= BACKPACK_SLOT_END; i++)
        if (inv->slots[i].entry == 0) return i;
    return -1;
}

bool Inventory_AddItem(Inventory* inv, uint32_t entry, uint32_t count) {
    ItemTemplate* tmpl = ItemSystem_GetTemplate(entry);
    if (!tmpl) return false;

    /* Try stacking first */
    if (tmpl->maxStack > 1) {
        for (int i = BACKPACK_SLOT_START; i <= BACKPACK_SLOT_END; i++) {
            if (inv->slots[i].entry == entry && inv->slots[i].stackCount < tmpl->maxStack) {
                uint32_t space = tmpl->maxStack - inv->slots[i].stackCount;
                uint32_t add = count < space ? count : space;
                inv->slots[i].stackCount += add;
                count -= add;
                if (count == 0) return true;
            }
        }
    }

    /* New slot */
    while (count > 0) {
        int slot = Inventory_FindFreeSlot(inv);
        if (slot < 0) return false;
        inv->slots[slot].guid = inv->nextItemGuid++;
        inv->slots[slot].entry = entry;
        uint32_t stack = tmpl->maxStack > 0 ? tmpl->maxStack : 1;
        inv->slots[slot].stackCount = count < stack ? count : stack;
        count -= inv->slots[slot].stackCount;
    }
    return true;
}

bool Inventory_RemoveItem(Inventory* inv, uint32_t entry, uint32_t count) {
    for (int i = 0; i < MAX_INVENTORY_SLOTS && count > 0; i++) {
        if (inv->slots[i].entry == entry) {
            if (inv->slots[i].stackCount <= count) {
                count -= inv->slots[i].stackCount;
                memset(&inv->slots[i], 0, sizeof(ItemInstance));
            } else {
                inv->slots[i].stackCount -= count;
                count = 0;
            }
        }
    }
    return count == 0;
}

uint32_t Inventory_CountItem(Inventory* inv, uint32_t entry) {
    uint32_t total = 0;
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++)
        if (inv->slots[i].entry == entry) total += inv->slots[i].stackCount;
    return total;
}

bool Inventory_EquipItem(Inventory* inv, int fromSlot, int toSlot) {
    if (fromSlot < 0 || fromSlot >= MAX_INVENTORY_SLOTS) return false;
    if (toSlot < EQUIPMENT_SLOT_START || toSlot > EQUIPMENT_SLOT_END) return false;
    /* Swap */
    ItemInstance tmp = inv->slots[toSlot];
    inv->slots[toSlot] = inv->slots[fromSlot];
    inv->slots[fromSlot] = tmp;
    return true;
}
