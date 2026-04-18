#ifndef WOW_PLAYER_H
#define WOW_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

/* ============== PLAYER DEFINITION ============== */
typedef struct Player Player;

#define PLAYER_MAX_LEVEL 80
#define PLAYER_MAX_SPELL 256
#define PLAYER_MAX_QUEST_LOG 25
#define PLAYER_MAX_GUILD_SLOTS 1
#define PLAYER_MAX_BANK_SLOTS 28
#define PLAYER_MAX_INVENTORY 118

typedef enum {
    PLAYER_STATE_NONE = 0,
    PLAYER_STATE_IN_WORLD = 1,
    PLAYER_STATE_LOGGING_OUT = 2,
    PLAYER_STATE_DEAD = 3,
    PLAYER_STATE_GHOST = 4,
    PLAYER_STATE_TELEPORTING = 5
} PlayerState;

typedef enum {
    RACE_HUMAN = 1, RACE_ORC = 2, RACE_DWARF = 3, RACE_NIGHTELF = 4,
    RACE_UNDEAD = 5, RACE_TAUREN = 6, RACE_GNOME = 7, RACE_TROLL = 8,
    RACE_BLOODELF = 10, RACE_DRAENEI = 11
} Race;

typedef enum {
    CLASS_WARRIOR = 1, CLASS_PALADIN = 2, CLASS_HUNTER = 3, CLASS_ROGUE = 4,
    CLASS_PRIEST = 5, CLASS_SHAMAN = 7, CLASS_MAGE = 8, CLASS_WARLOCK = 9,
    CLASS_DRUID = 11
} Class;

/* ============== CORE PLAYER ============== */
struct Player {
    /* Identity */
    uint64_t guid;
    char name[65];
    uint32_t account_id;
    Race race;
    Class class_;
    uint8_t gender;
    uint8_t skin, face, hair_style, hair_color, facial_hair, outfit_id;
    
    /* Stats */
    uint8_t level;
    uint32_t xp;
    uint32_t max_xp_at_level[PLAYER_MAX_LEVEL + 1];
    uint32_t next_level_xp;
    
    /* Attributes (base stats) */
    uint32_t strength, agility, stamina, intellect, spirit;
    uint32_t max_health, health;
    uint32_t max_power, power;           /* mana/rage/energy */
    uint32_t max_power_rune;             /* runic power for DKs */
    
    /* Combat ratings */
    uint32_t weapon_skill;
    uint32_t defense;
    uint32_t dodge;
    uint32_t parry;
    uint32_t block;
    uint32_t hit;
    uint32_t crit;
    uint32_t expertise;
    uint32_t mastery;
    
    /* Position */
    uint32_t map_id;
    uint32_t zone_id;
    float x, y, z, o;
    
    /* Movement */
    float walk_speed;
    float run_speed;
    float swim_speed;
    float fly_speed;
    float turn_rate;
    
    /* Guild */
    uint32_t guild_id;
    uint32_t guild_rank;
    
    /* Group */
    uint32_t group_id;
    uint64_t group_invite_guid;
    
    /* Inventory slots (item guid or 0) */
    uint64_t inventory[PLAYER_MAX_INVENTORY];
    uint64_t bank_slots[PLAYER_MAX_BANK_SLOTS];
    uint64_t equipped[19];              /* equipment slots */
    
    /* Currency (honor, arena, etc) */
    uint32_t honor_points;
    uint32_t arena_points;
    uint32_t kill_count;
    
    /* Glyphs */
    uint32_t glyph1_id;
    uint32_t glyph2_id;
    uint32_t glyph3_id;
    
    /* Talents */
    uint32_t talent_spec;                /* 0 or 1 for dual spec */
    uint32_t talent_points;
    
    /* Spells */
    uint32_t learned_spells[PLAYER_MAX_SPELL];
    uint32_t spell_count;
    
    /* Quests */
    uint32_t quest_log[PLAYER_MAX_QUEST_LOG];
    uint32_t quest_status[PLAYER_MAX_QUEST_LOG];
    uint32_t quest_rewarded[PLAYER_MAX_QUEST_LOG];
    
    /* Timers */
    uint32_t logout_timer;               /* ms remaining before logout */
    time_t last_tick;
    
    /* Flags */
    uint32_t flags;
    uint32_t unit_flags;
    
    /* Network */
    int fd;
    
    /* State */
    PlayerState state;
    bool in_world;
};

/* ============== PLAYER METHODS ============== */
Player* player_create(uint64_t guid);
void player_free(Player* p);

int player_set_level(Player* p, uint8_t level);
int player_add_xp(Player* p, uint32_t xp);
int player_add_item(Player* p, uint32_t entry, uint32_t count);
int player_remove_item(Player* p, uint32_t entry, uint32_t count);
int player_has_item(Player* p, uint32_t entry, uint32_t count);

int player_learn_spell(Player* p, uint32_t spell_id);
int player_cast_spell(Player* p, uint32_t spell_id, uint64_t target_guid);

int player_teleport(Player* p, uint32_t map_id, float x, float y, float z, float o);

void player_send_packet(Player* p, const uint8_t* data, size_t len);
void player_send_message(Player* p, const char* message);
void player_send_notification(Player* p, const char* text);

int player_save_to_db(Player* p);
int player_load_from_db(Player* p, uint64_t guid);

void player_set_health(Player* p, uint32_t health);
void player_set_power(Player* p, uint32_t power);
void player_modify_health(Player* p, int32_t delta);
void player_modify_power(Player* p, int32_t delta);

void player_add_to_world(Player* p);
void player_remove_from_world(Player* p);

#endif /* WOW_PLAYER_H */