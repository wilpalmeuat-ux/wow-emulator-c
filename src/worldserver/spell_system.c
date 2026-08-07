/* spell_system.c -- Spell casting system for WoW 3.3.5a emulator
 *
 * Handles spell casting, cooldowns, effects, damage/heal calculations,
 * and aura management.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ================================================================
 *  Spell effect types
 * ================================================================ */
typedef enum {
    SPELL_EFFECT_NONE           = 0,
    SPELL_EFFECT_DAMAGE         = 1,
    SPELL_EFFECT_HEAL           = 2,
    SPELL_EFFECT_APPLY_AURA     = 3,
    SPELL_EFFECT_TELEPORT       = 4,
    SPELL_EFFECT_ENERGIZE       = 5,
    SPELL_EFFECT_SUMMON         = 6,
    SPELL_EFFECT_DISPEL         = 7,
    SPELL_EFFECT_CREATE_ITEM    = 8,
    SPELL_EFFECT_RESURRECT      = 9,
    SPELL_EFFECT_KNOCKBACK      = 10,
    SPELL_EFFECT_STUN           = 11,
    SPELL_EFFECT_ROOT           = 12,
    SPELL_EFFECT_SILENCE        = 13,
    SPELL_EFFECT_DOT            = 14,  /* Damage over time */
    SPELL_EFFECT_HOT            = 15,  /* Heal over time */
    SPELL_EFFECT_MAX
} SpellEffectType;

typedef enum {
    SPELL_SCHOOL_PHYSICAL = 0,
    SPELL_SCHOOL_HOLY     = 1,
    SPELL_SCHOOL_FIRE     = 2,
    SPELL_SCHOOL_NATURE   = 3,
    SPELL_SCHOOL_FROST    = 4,
    SPELL_SCHOOL_SHADOW   = 5,
    SPELL_SCHOOL_ARCANE   = 6
} SpellSchool;

/* ================================================================
 *  Spell definition
 * ================================================================ */
typedef struct SpellInfo {
    uint32_t id;
    char     name[64];
    uint32_t manaCost;
    uint32_t castTime;        /* milliseconds */
    uint32_t cooldown;        /* milliseconds */
    float    rangeMin;
    float    rangeMax;
    SpellSchool school;
    SpellEffectType effect[3];
    int32_t  effectValue[3];  /* base value for each effect */
    float    effectMultiplier[3];
    uint32_t duration;        /* for auras, ms */
    uint32_t tickInterval;    /* for DOT/HOT, ms */
    bool     isChanneled;
    bool     isInstant;
    uint32_t targetType;      /* 0=self, 1=enemy, 2=friend, 3=aoe */
} SpellInfo;

/* ================================================================
 *  Aura (active buff/debuff on a unit)
 * ================================================================ */
#define MAX_AURAS 64

typedef struct Aura {
    uint32_t spellId;
    uint32_t duration;        /* remaining ms */
    uint32_t maxDuration;
    uint32_t tickTimer;       /* ms until next tick */
    uint32_t tickInterval;
    int32_t  value;           /* effect value per tick */
    SpellEffectType effect;
    uint64_t casterGuid;
    bool     active;
} Aura;

typedef struct AuraList {
    Aura auras[MAX_AURAS];
    int  count;
} AuraList;

/* ================================================================
 *  Spell cooldown tracker
 * ================================================================ */
#define MAX_COOLDOWNS 256

typedef struct CooldownEntry {
    uint32_t spellId;
    uint32_t remaining;       /* ms */
} CooldownEntry;

typedef struct CooldownList {
    CooldownEntry entries[MAX_COOLDOWNS];
    int count;
} CooldownList;

/* ================================================================
 *  Spell registry (loaded from DB or hardcoded)
 * ================================================================ */
#define MAX_SPELLS 1024
static SpellInfo g_spells[MAX_SPELLS];
static int g_spellCount = 0;

void SpellSystem_Init(void) {
    g_spellCount = 0;
    memset(g_spells, 0, sizeof(g_spells));

    /* Register some demo spells */
    SpellInfo* s;

    /* 1: Fireball */
    s = &g_spells[g_spellCount++];
    s->id = 133; s->manaCost = 200; s->castTime = 3500; s->cooldown = 0;
    s->rangeMax = 35.0f; s->school = SPELL_SCHOOL_FIRE;
    s->effect[0] = SPELL_EFFECT_DAMAGE; s->effectValue[0] = 500;
    strncpy(s->name, "Fireball", 63);

    /* 2: Frostbolt */
    s = &g_spells[g_spellCount++];
    s->id = 116; s->manaCost = 180; s->castTime = 3000; s->cooldown = 0;
    s->rangeMax = 30.0f; s->school = SPELL_SCHOOL_FROST;
    s->effect[0] = SPELL_EFFECT_DAMAGE; s->effectValue[0] = 400;
    strncpy(s->name, "Frostbolt", 63);

    /* 3: Flash Heal */
    s = &g_spells[g_spellCount++];
    s->id = 2061; s->manaCost = 300; s->castTime = 1500; s->cooldown = 0;
    s->rangeMax = 40.0f; s->school = SPELL_SCHOOL_HOLY;
    s->effect[0] = SPELL_EFFECT_HEAL; s->effectValue[0] = 700;
    strncpy(s->name, "Flash Heal", 63);

    /* 4: Renew (HoT) */
    s = &g_spells[g_spellCount++];
    s->id = 139; s->manaCost = 250; s->castTime = 0; s->isInstant = true;
    s->rangeMax = 40.0f; s->school = SPELL_SCHOOL_HOLY;
    s->effect[0] = SPELL_EFFECT_HOT; s->effectValue[0] = 100;
    s->duration = 15000; s->tickInterval = 3000;
    strncpy(s->name, "Renew", 63);

    /* 5: Shadow Word: Pain (DoT) */
    s = &g_spells[g_spellCount++];
    s->id = 589; s->manaCost = 200; s->castTime = 0; s->isInstant = true;
    s->rangeMax = 30.0f; s->school = SPELL_SCHOOL_SHADOW;
    s->effect[0] = SPELL_EFFECT_DOT; s->effectValue[0] = 150;
    s->duration = 18000; s->tickInterval = 3000;
    strncpy(s->name, "Shadow Word: Pain", 63);

    /* 6: Heroic Strike */
    s = &g_spells[g_spellCount++];
    s->id = 78; s->manaCost = 0; s->castTime = 0; s->isInstant = true;
    s->rangeMax = 5.0f; s->school = SPELL_SCHOOL_PHYSICAL;
    s->effect[0] = SPELL_EFFECT_DAMAGE; s->effectValue[0] = 200;
    strncpy(s->name, "Heroic Strike", 63);

    printf("[Spell] Registered %d spells\n", g_spellCount);
}

SpellInfo* SpellSystem_GetSpell(uint32_t id) {
    for (int i = 0; i < g_spellCount; i++) {
        if (g_spells[i].id == id) return &g_spells[i];
    }
    return NULL;
}

/* ================================================================
 *  Cast a spell
 * ================================================================ */
typedef struct {
    bool     success;
    int32_t  damage;
    int32_t  healing;
    uint32_t auraApplied;
    char     message[256];
} SpellResult;

SpellResult SpellSystem_Cast(uint32_t spellId, Unit* caster, Unit* target) {
    SpellResult result = {0};

    SpellInfo* spell = SpellSystem_GetSpell(spellId);
    if (!spell) {
        result.success = false;
        snprintf(result.message, sizeof(result.message), "Unknown spell %u", spellId);
        return result;
    }

    /* Check mana */
    uint32_t casterMana = (uint32_t)caster->fields[UNIT_FIELD_POWER_1];
    if (spell->manaCost > 0 && casterMana < spell->manaCost) {
        result.success = false;
        snprintf(result.message, sizeof(result.message), "Not enough mana for %s", spell->name);
        return result;
    }

    /* Deduct mana */
    if (spell->manaCost > 0) {
        caster->fields[UNIT_FIELD_POWER_1] = casterMana - spell->manaCost;
    }

    /* Apply effects */
    for (int i = 0; i < 3; i++) {
        if (spell->effect[i] == SPELL_EFFECT_NONE) continue;
        int32_t val = spell->effectValue[i];

        switch (spell->effect[i]) {
        case SPELL_EFFECT_DAMAGE:
            if (target) {
                uint32_t hp = (uint32_t)target->fields[UNIT_FIELD_HEALTH];
                if ((int32_t)hp - val < 0) val = (int32_t)hp;
                target->fields[UNIT_FIELD_HEALTH] = hp - val;
                if (target->fields[UNIT_FIELD_HEALTH] == 0) target->dead = true;
                result.damage += val;
            }
            break;

        case SPELL_EFFECT_HEAL:
            if (target) {
                uint32_t hp = (uint32_t)target->fields[UNIT_FIELD_HEALTH];
                uint32_t maxHp = (uint32_t)target->fields[UNIT_FIELD_MAXHEALTH];
                uint32_t newHp = hp + val;
                if (newHp > maxHp) newHp = maxHp;
                target->fields[UNIT_FIELD_HEALTH] = newHp;
                result.healing += val;
            }
            break;

        case SPELL_EFFECT_DOT:
        case SPELL_EFFECT_HOT:
        case SPELL_EFFECT_APPLY_AURA:
            result.auraApplied = spellId;
            break;

        default:
            break;
        }
    }

    result.success = true;
    snprintf(result.message, sizeof(result.message), "%s cast %s: dmg=%d heal=%d",
             caster->name, spell->name, result.damage, result.healing);
    printf("[Spell] %s\n", result.message);
    return result;
}

/* ================================================================
 *  Aura tick processing (called every update tick)
 * ================================================================ */
void SpellSystem_UpdateAuras(AuraList* auras, Unit* owner, uint32_t diffMs) {
    if (!auras || !owner) return;
    for (int i = 0; i < auras->count; i++) {
        Aura* a = &auras->auras[i];
        if (!a->active) continue;

        /* Reduce duration */
        if (a->duration <= diffMs) {
            a->active = false;
            printf("[Aura] Aura spell=%u expired on %s\n", a->spellId, owner->name);
            continue;
        }
        a->duration -= diffMs;

        /* Tick processing */
        if (a->tickInterval > 0) {
            if (a->tickTimer <= diffMs) {
                /* Apply tick effect */
                if (a->effect == SPELL_EFFECT_DOT) {
                    uint32_t hp = (uint32_t)owner->fields[UNIT_FIELD_HEALTH];
                    int32_t dmg = a->value;
                    if ((int32_t)hp - dmg < 0) dmg = (int32_t)hp;
                    owner->fields[UNIT_FIELD_HEALTH] = hp - dmg;
                    if (owner->fields[UNIT_FIELD_HEALTH] == 0) owner->dead = true;
                } else if (a->effect == SPELL_EFFECT_HOT) {
                    uint32_t hp = (uint32_t)owner->fields[UNIT_FIELD_HEALTH];
                    uint32_t maxHp = (uint32_t)owner->fields[UNIT_FIELD_MAXHEALTH];
                    uint32_t newHp = hp + a->value;
                    if (newHp > maxHp) newHp = maxHp;
                    owner->fields[UNIT_FIELD_HEALTH] = newHp;
                }
                a->tickTimer = a->tickInterval;
            } else {
                a->tickTimer -= diffMs;
            }
        }
    }
}

/* ================================================================
 *  Cooldown management
 * ================================================================ */
bool SpellSystem_IsOnCooldown(CooldownList* cds, uint32_t spellId) {
    for (int i = 0; i < cds->count; i++) {
        if (cds->entries[i].spellId == spellId && cds->entries[i].remaining > 0)
            return true;
    }
    return false;
}

void SpellSystem_AddCooldown(CooldownList* cds, uint32_t spellId, uint32_t durationMs) {
    for (int i = 0; i < cds->count; i++) {
        if (cds->entries[i].spellId == spellId) {
            cds->entries[i].remaining = durationMs;
            return;
        }
    }
    if (cds->count < MAX_COOLDOWNS) {
        cds->entries[cds->count].spellId = spellId;
        cds->entries[cds->count].remaining = durationMs;
        cds->count++;
    }
}

void SpellSystem_UpdateCooldowns(CooldownList* cds, uint32_t diffMs) {
    for (int i = 0; i < cds->count; i++) {
        if (cds->entries[i].remaining > 0) {
            if (cds->entries[i].remaining <= diffMs)
                cds->entries[i].remaining = 0;
            else
                cds->entries[i].remaining -= diffMs;
        }
    }
}
