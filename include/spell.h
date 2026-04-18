#ifndef WOW_SPELL_H
#define WOW_SPELL_H

#include <stdint.h>
#include <stdbool.h>

/* ============== SPELL ============== */
typedef struct Spell Spell;
typedef struct SpellInfo SpellInfo;

#define SPELL_MAX_EFFECTS 3
#define SPELL_MAX_AURAS 32

typedef enum {
    SPELL_FAMILY_GENERIC = 0,
    SPELL_FAMILY_UNK1 = 1,
    SPELL_FAMILY_ROGUE = 2,
    SPELL_FAMILY_WARRIOR = 4,
    SPELL_FAMILY_PRIEST = 5,
    SPELL_FAMILY_MAGE = 6,
    SPELL_FAMILY_WARLOCK = 7,
    SPELL_FAMILY_DRUID = 8,
    SPELL_FAMILY_HUNTER = 9,
    SPELL_FAMILY_PALADIN = 10,
    SPELL_FAMILY_SHAMAN = 11,
    SPELL_FAMILY_DEATH_KNIGHT = 12
} SpellFamily;

typedef enum {
    SPELL_ATTR0_UNK0 = 0,
    SPELL_ATTR0_INSTANT = 1,
    SPELL_ATTR0_PROVIDE_ITEM = 2,
    SPELL_ATTR0_ENCHANT_ITEM = 3,
    SPELL_ATTR0_ABILITY = 4,
    SPELL_ATTR0_TRADESPELL = 5,
    SPELL_ATTR0_PASSIVE = 6,
    SPELL_ATTR0_HIDE_FROM_CLIENT = 7,
    SPELL_ATTR0_ONCE_PER_TARGET = 8,
    SPELL_ATTR0_UNK9 = 9,
    SPELL_ATTR0_ONCE_PER_PLAYER = 10,
    SPELL_ATTR0_REQUIRES_SWORD_PROFICIENCY = 11,
    SPELL_ATTR0_UNK12 = 12,
    SPELL_ATTR0_CASTABLE_WHILE_MOVING = 16,
    SPELL_ATTR0_CASTABLE_WHILE_SITTING = 17,
    SPELL_ATTR0_COSTS_RUNIC_POWER = 22
} SpellAttr0;

typedef enum {
    SPELL_EFFECT_HEAL = 2,
    SPELL_EFFECT_DAMAGE = 3,
    SPELL_EFFECT_SCRIPT_EFFECT = 39,
    SPELL_EFFECT_APPLY_AURA = 6,
    SPELL_EFFECT_ENERGIZE = 5,
    SPELL_EFFECT_SUMMON = 29,
    SPELL_EFFECT_TELEPORT = 30,
    SPELL_EFFECT_TRIGGER_SPELL = 54,
    SPELL_EFFECT_HEAL_PCT = 8,
    SPELL_EFFECT_DAMAGE_PCT = 9,
    SPELL_EFFECT_SEND_EVENT = 25,
    SPELL_EFFECT_CREATE_ITEM = 33
} SpellEffectIndex;

typedef enum {
    SPELL_AURA_MOD_STAT = 0,
    SPELL_AURA_MOD_SPEED = 1,
    SPELL_AURA_MOD_DAMAGE = 2,
    SPELL_AURA_MOD_DAMAGE_PERCENT = 3,
    SPELL_AURA_MOD_HEAL = 4,
    SPELL_AURA_MOD_HEAL_PCT = 5,
    SPELL_AURA_MOD_DAMAGE_DONE = 6,
    SPELL_AURA_MOD_ATTACK_POWER = 11,
    SPELL_AURA_MOD_RESISTANCE = 12,
    SPELL_AURA_TRIGGER_SPELL = 14,
    SPELL_AURA_PROC_TRIGGER_SPELL = 42,
    SPELL_AURA_MOD_RATING = 24,
    SPELL_AURA_MOD_CRIT_PCT = 31,
    SPELL_AURA_MOD_HASTE =  36,
    SPELL_AURA_MOD_TAXI = 45
} AuraType;

struct SpellInfo {
    uint32_t id;
    SpellFamily spell_family;
    uint32_t spell_family_flags;
    uint32_t school;
    uint32_t category;
    uint32_t field4;
    uint64_t attributes;
    uint64_t attributesEx;
    uint64_t attributesEx2;
    uint64_t attributesEx3;
    uint64_t attributesEx4;
    uint64_t attributesEx5;
    uint64_t attributesEx6;
    uint64_t attributesEx7;
    uint64_t targets;
    uint32_t target_scripts;
    uint32_t caster_aura_state;
    uint32_t target_aura_state;
    uint32_t exclude_caster_aura_state;
    uint32_t exclude_target_aura_state;
    uint32_t spell_class_mask;
    uint32_tunk7;
    uint32_tunk8;
    uint32_tunk9;
    float speed;
    uint32_t unk320_1;
    uint32_t spell_priority;
    char* name;
    uint32_t name_flags;
    uint32_t rank;
    char* rank_flags;
    uint32_t description;
    uint32_t description_flags;
    uint32_t tool;
    uint32_t target_type;
    uint32_t spell_unk1;
    uint32_t spell_unk2;
    uint32_t spell_unk3;
    uint32_t spell_unk4;
    uint32_t spell_unk5;
    uint32_t spell_unk6;
    float unk320_3;
    uint32_t spell_unk7;
    uint32_t spell_unk8;
    uint32_t spell_unk9;
    uint32_t spell_unk10;
    uint32_t spell_unk11;
    uint32_t spell_unk12;
    uint32_t spell_unk13;
    uint32_t spell_unk14;
    uint32_t spell_level;
    uint32_t base_level;
    uint32_t active_icon;
    uint32_t general_case;
    uint32_t spell_dmg_coefficient;
    /* Spell effects */
    uint32_t effect_count;
    uint32_t effect[SPELL_MAX_EFFECTS];
    int32_t effect_base_points[SPELL_MAX_EFFECTS];
    float effect_dice_per_level[SPELL_MAX_EFFECTS];
    float effect_real_per_level[SPELL_MAX_EFFECTS];
    uint32_t effect_mechanic[SPELL_MAX_EFFECTS];
    uint32_t effect_implicit_targetA[SPELL_MAX_EFFECTS];
    uint32_t effect_implicit_targetB[SPELL_MAX_EFFECTS];
    float effect_radius_index[SPELL_MAX_EFFECTS];
    float effect_max_range[SPELL_MAX_EFFECTS];
    uint32_t effect_chain_target[SPELL_MAX_EFFECTS];
    uint32_t effect_spell_class_mask[SPELL_MAX_EFFECTS];
    uint32_t effect_misc_value[SPELL_MAX_EFFECTS];
    float effect_combine_targets[SPELL_MAX_EFFECTS];
    uint32_t effect_trigger_spell[SPELL_MAX_EFFECTS];
    uint32_t effect_points_per_combo[SPELL_MAX_EFFECTS];
    /* Scaling */
    float dmg_multiplier;
    float bonus_multiplier;
    uint32_t bonus_coefficient;
    /* Power cost */
    uint32_t power_type;
    uint32_t mana_cost;
    uint32_t mana_cost_per_level;
    uint32_t mana_cost_per_second;
    uint32_t mana_cost_percent;
    /* Cast times */
    uint32_t cast_time_index;
    uint32_t cast_time_min;
    float cast_time_max;
    /* Duration */
    int32_t duration_index;
    int32_t duration_min;
    int32_t duration_max;
    /* Range */
    float range_index;
    float range_min;
    float range_max;
    /* Interrupts */
    uint32_t interrupt_flags;
    uint32_t aura_interrupt_flags;
    uint32_t channel_interrupt_flags;
    /* GCD */
    uint32_t gcd_category;
    uint32_t gcd_category_cooldown;
    /* Flags */
    uint32_t spelldescription_vars;
    uint32_t scaling_id;
    uint32_t spell_cofficounters;
    uint32_t spell_cooldown_duration;
    uint32_t spell_category;
    uint32_t spell_category_cooldown;
    uint32_t spell_max_creature_target;
    uint32_t spell_attributes[4];
    /* Aura info */
    uint32_t aura_count;
    uint32_t aura[SPELL_MAX_AURAS];
    uint32_t aura_flags[SPELL_MAX_AURAS];
    int32_t aura_base_points[SPELL_MAX_AURAS];
    uint32_t aura_mechanic[SPELL_MAX_AURAS];
    uint32_t aura_radius_index[SPELL_MAX_AURAS];
    uint32_t aura_periodic_bonus[SPELL_MAX_AURAS];
    uint32_t aura_applies_aura_index[SPELL_MAX_AURAS];
};

struct Spell {
    uint64_t caster_guid;
    uint64_t target_guid;
    uint64_t original_caster_guid;
    SpellInfo* info;
    uint32_t cast_time;
    uint32_t cast_end_time;
    int32_t mana_cost;
    uint32_t school_mask;
    uint32_t trigger_spell_id;
    float damage;
    float heal;
    bool is_channeled;
    bool is_periodic;
    uint32_t spell_state_flags;
    uint32_t aura_group;
    uint32_t chain_target_count;
    float chain_damage;
    uint32_t proc_charges;
    uint32_t proc_flags;
    uint32_t max_effect_index;
};

SpellInfo* spell_get_info(uint32_t spell_id);
Spell* spell_create_cast(uint64_t caster, uint64_t target, uint32_t spell_id, uint32_t trigger_mask);
void spell_free(Spell* s);
int spell_cast(Spell* s);
int spell_execute(Spell* s);
void spell_trigger_damage(Spell* s, uint64_t target, float amount);
void spell_trigger_heal(Spell* s, uint64_t target, float amount);
int spell_has_cooldown(uint64_t caster, uint32_t spell_id);
int spell_start_cooldown(uint64_t caster, uint32_t spell_id, uint32_t duration);
int spell_is_ready(uint64_t caster, uint32_t spell_id);

#endif /* WOW_SPELL_H */