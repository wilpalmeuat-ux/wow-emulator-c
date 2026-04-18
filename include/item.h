#ifndef WOW_ITEM_H
#define WOW_ITEM_H

#include <stdint.h>
#include <stdbool.h>

/* ============== ITEM ============== */
typedef struct Item Item;
typedef struct ItemTemplate ItemTemplate;

#define ITEM_MAX_SOCKETS 3
#define ITEM_MAX_STATS 10
#define ITEM_MAX_SPELLS 5

typedef enum {
    ITEM_CLASS_WEAPON = 1,
    ITEM_CLASS_ARMOR = 2,
    ITEM_CLASS_CONSUMABLE = 3,
    ITEM_CLASS_CONTAINER = 4,
    ITEM_CLASS_TRADE_GOODS = 5,
    ITEM_CLASS_QUIVER = 11,
    ITEM_CLASS_QUEST = 12,
    ITEM_CLASS_MISC = 15
} ItemClass;

typedef enum {
    ITEM_SUBCLASS_WEAPON_AXE = 0,
    ITEM_SUBCLASS_WEAPON_BOW = 2,
    ITEM_SUBCLASS_WEAPON_DAGGER = 3,
    ITEM_SUBCLASS_WEAPON_STAFF = 4,
    ITEM_SUBCLASS_WEAPON_FIST = 5,
    ITEM_SUBCLASS_WEAPON_MACE = 6,
    ITEM_SUBCLASS_WEAPON_POLEARM = 7,
    ITEM_SUBCLASS_WEAPON_SWORD = 8,
    ITEM_SUBCLASS_WEAPON_MISSILE = 9,
    ITEM_SUBCLASS_WEAPON_GUN = 10,
    ITEM_SUBCLASS_WEAPON_THROWN = 11,
    ITEM_SUBCLASS_WEAPON_CROSSBOW = 13,
    ITEM_SUBCLASS_WEAPON_WAND = 15,
    ITEM_SUBCLASS_ARMOR_CLOTH = 1,
    ITEM_SUBCLASS_ARMOR_LEATHER = 2,
    ITEM_SUBCLASS_ARMOR_MAIL = 3,
    ITEM_SUBCLASS_ARMOR_PLATE = 4,
    ITEM_SUBCLASS_ARMOR_SHIELD = 5,
    ITEM_SUBCLASS_ARMOR_MISC = 6
} ItemSubClass;

typedef enum {
    STAT_TYPE_NONE = 0,
    STAT_TYPE_HEALTH = 1,
    STAT_TYPE_MANA = 2,
    STAT_TYPE_STRENGTH = 3,
    STAT_TYPE_AGILITY = 4,
    STAT_TYPE_STAMINA = 5,
    STAT_TYPE_INTELLECT = 6,
    STAT_TYPE_SPIRIT = 7,
    STAT_TYPE_CRIT_RATING = 14,
    STAT_TYPE_HASTE_RATING = 15,
    STAT_TYPE_HIT_RATING = 16,
    STAT_TYPE_Dodge_RATING = 18,
    STAT_TYPE_PARRY_RATING = 19,
    STAT_TYPE_RESILIENCE = 20
} ItemStatType;

typedef enum {
    SPELL_EFFECT_NONE = 0,
    SPELL_EFFECT_EQUIP = 1,
    SPELL_EFFECT_USE = 2,
    SPELL_EFFECT_CHARGES = 4,
    SPELL_EFFECT_PROC = 6,
    SPELL_EFFECT_ENCHANT = 8
} ItemSpellTriggerType;

struct ItemTemplate {
    uint32_t entry;
    char name[100];
    ItemClass class_;
    ItemSubClass subclass;
    uint32_t display_id;
    uint32_t quality;
    uint32_t flags;
    uint32_t buy_count;
    uint32_t buy_price;
    uint32_t sell_price;
    uint32_t inventory_type;
    uint32_t bonding;
    uint32_t page_text;
    uint32_t language;
    uint32_t page_material;
    uint32_t start_quest;
    uint32_t lock_id;
    uint32_t material;
    uint32_t sheathe;
    uint32_t property_seed;
    uint32_t random_property;
    uint32_t block;
    uint32_t item_level;
    uint32_t required_level;
    uint32_t required_skill;
    uint32_t required_skill_rank;
    uint32_t required_spell;
    uint32_t required_faction;
    uint32_t required_faction_rank;
    uint32_t max_count;
    uint32_t max_stack;
    /* Stats */
    uint32_t stat_count;
    ItemStatType stat_type[ITEM_MAX_STATS];
    int32_t stat_value[ITEM_MAX_STATS];
    /* Spells */
    uint32_t spell_count;
    uint32_t spell_trigger[ITEM_MAX_SOCKETS];
    uint32_t spell_id[ITEM_MAX_SOCKETS];
    int32_t spell_cooldown[ITEM_MAX_SOCKETS];
    /* Resistance */
    uint32_t armor;
    uint32_t fire_res;
    uint32_t nature_res;
    uint32_t frost_res;
    uint32_t shadow_res;
    uint32_t arcane_res;
    /* Damage */
    float dmg_min[2];
    float dmg_max[2];
    uint32_t dmg_type;
    uint32_t weapon_speed;
    uint32_t weapon_damage;
    /* Sockets */
    uint32_t socket_count;
    uint32_t socket_color[ITEM_MAX_SOCKETS];
    uint32_t socket_content[ITEM_MAX_SOCKETS];
};

struct Item {
    uint64_t guid;
    uint32_t entry;
    ItemTemplate* template;
    uint64_t owner_guid;
    uint32_t creator_guid;
    uint32_t gift_creator_guid;
    uint32_t count;
    uint32_t duration;
    char charges[5];
    bool is_enchanted;
    uint32_t enchant_id;
    uint32_t enchant_duration;
    uint32_t durability;
    uint32_t max_durability;
    uint32_t state_flags;
    uint32_t property_seed;
    int32_t random_property_id;
    int32_t random_suffix_id;
    uint32_t item_text_id;
    /* Bag info */
    uint32_t bag_slot;                   /* which bag slot, 0-4 for inventory */
    uint32_t slot;                      /* slot within bag or equipment slot */
    /* Soulbound tracking */
    bool soulbound_to_guid;
};

Item* item_create(uint32_t entry);
void item_free(Item* i);
ItemTemplate* item_get_template(uint32_t entry);
int item_save_to_db(Item* i);
int item_load_from_db(Item* i, uint64_t guid);

#endif /* WOW_ITEM_H */