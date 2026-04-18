#ifndef WOW_CREATURE_H
#define WOW_CREATURE_H

#include <stdint.h>
#include <stdbool.h>

/* ============== CREATURE ============== */
typedef struct Creature Creature;

#define CREATURE_STATIC_NAME 64

typedef enum {
    CREATURE_TYPE_BEAST = 1,
    CREATURE_TYPE_DRAGONKIN = 2,
    CREATURE_TYPE_DEMON = 3,
    CREATURE_TYPE_ELEMENTAL = 4,
    CREATURE_TYPE_GIANT = 5,
    CREATURE_TYPE_UNDEAD = 6,
    CREATURE_TYPE_MECHANICAL = 7,
    CREATURE_TYPE_NOT_SPECIFIED = 8,
    CREATURE_TYPE_TOTEM = 9,
    CREATURE_TYPE_NON_COMBAT_PET = 10,
    CREATURE_TYPE_CRITTER = 11,
    CREATURE_TYPE_OTHER = 12
} CreatureType;

typedef enum {
    CREATURE_FLAG_SKIP_PATH = 0x01,
    CREATURE_FLAG_NO_AGGRO = 0x02,
    CREATURE_FLAG_IGNORE_LOOT = 0x04,
    CREATURE_FLAG_SPAWN_EVENT = 0x08,
    CREATURE_FLAG_TAMPERED = 0x10
} CreatureFlags;

struct Creature {
    /* Identity */
    uint64_t guid;
    uint32_t entry;
    char name[CREATURE_STATIC_NAME];
    char subname[CREATURE_STATIC_NAME];
    
    /* Classification */
    CreatureType type;
    uint32_t faction;
    uint32_t rank;                       /* 0=normal, 1=elite, 2=rare elite, 3=boss, 4=world boss */
    
    /* Equipment / display */
    uint32_t display_id;
    uint32_t equipment_id;
    float bounding_radius;
    float combat_reach;
    
    /* Base stats (scaled by level) */
    uint32_t level_min;
    uint32_t level_max;
    uint32_t health_min;
    uint32_t health_max;
    uint32_t mana_min;
    uint32_t mana_max;
    
    /* Current state */
    int32_t health;
    int32_t mana;
    uint32_t state;                      /* 0=dead, 1=alive, etc */
    
    /* Position */
    uint32_t map_id;
    float x, y, z, o;
    
    /* Movement */
    float walk_speed;
    float run_speed;
    float turn_rate;
    
    /* Loot */
    uint32_t loot_id;
    uint32_t skin_id;
    
    /* AI */
    char ai_name[64];
    void* ai_state;
    
    /* Scripts */
    char script_name[64];
    
    /* Timers */
    uint32_t combat_reach_time;
    uint32_t despawn_timer;
    
    /* Spawn data */
    uint32_t spawn_id;
    uint32_t spawntimesecs;
    float spawndist;
    uint32_t movement_type;
    
    /* Unit flags */
    uint32_t unit_flags;
    uint32_t dynamic_flags;
    
    /* Linked to player who last engaged */
    uint64_t last_engagement_guid;
    bool in_combat;
};

/* ============== CREATURE TEMPLATE ============== */
typedef struct CreatureTemplate {
    uint32_t entry;
    char name[100];
    char subname[100];
    uint32_t minlevel;
    uint32_t maxlevel;
    uint32_t health_min;
    uint32_t health_max;
    uint32_t mana_min;
    uint32_t mana_max;
    uint32_t faction;
    float scale;
    uint8_t rank;
    uint32_t unit_flags;
    uint32_t dynamic_flags;
    uint32_t family;
    uint32_t trainer_type;
    uint32_t spell_id[4];
    char ai_name[64];
    char script_name[64];
} CreatureTemplate;

/* ============== CREATURE METHODS ============== */
Creature* creature_create(uint32_t entry);
void creature_free(Creature* c);

int creature_spawn(Creature* c, uint32_t map_id, float x, float y, float z, float o);
void creature_despawn(Creature* c);
void creature_deal_damage(Creature* c, uint64_t target_guid, uint32_t damage);
void creature_on Death(Creature* c, uint64_t killer_guid);
int creature_set_target(Creature* c, uint64_t guid);
void creature_clear_target(Creature* c);

/* ============== CREATURE AI ============== */
typedef struct CreatureAI CreatureAI;

typedef void (*AIUpdateFn)(Creature* c, uint32_t diff);
typedef void (*AIEnterCombatFn)(Creature* c, uint64_t enemy);
typedef void (*AILeaveCombatFn)(Creature* c);
typedef void (*AIDiedFn)(Creature* c, uint64_t killer);
typedef void (*AIMovementFn)(Creature* c);

struct CreatureAI {
    Creature* owner;
    AIUpdateFn on_update;
    AIEnterCombatFn on_enter_combat;
    AILeaveCombatFn on_leave_combat;
    AIDiedFn on_death;
    AIMovementFn on_movement;
    
    uint32_t update_timer;
    uint32_t combat_update_timer;
    uint32_t movement_timer;
    
    uint64_t target_guid;
    uint32_t state_flags;
    
    /* Waypoint movement */
    uint32_t waypoint_id;
    uint32_t waypoint_count;
    float* waypoint_x;
    float* waypoint_y;
    float* waypoint_z;
};

/* Built-in AI types */
CreatureAI* ai_create_aggressive(Creature* owner);
CreatureAI* ai_create_passive(Creature* owner);
CreatureAI* ai_create_waypoint(Creature* owner, float* x, float* y, float* z, int count);
CreatureAI* ai_create_boss(Creature* owner);
void ai_free(CreatureAI* ai);

#endif /* WOW_CREATURE_H */