#ifndef WOW_WORLD_H
#define WOW_WORLD_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define PI 3.14159265358979323846

typedef struct {
    uint64_t guid;
    char name[64];
    float x, y, z;
    float facing;
    float o;
    uint32_t map_id;
    uint32_t level;
    uint32_t health;
    uint32_t max_health;
    uint32_t power;
    uint32_t max_power;
    uint32_t mana;
    uint32_t max_mana;
    uint8_t unit_type;     /* 0=player, 1=npc, 2=object */
    uint8_t race;
    uint8_t class_;
    uint32_t model_id;
    bool in_combat;
    bool dead;
    bool stand_state;
    uint32_t npc_flags;
    float speed_walk;
    float speed_run;
    float speed_swim;
    float speed_fly;
    uint32_t faction;
    float bounding_radius;
    uint32_t guild_id;
    uint64_t player_guid;
    float mount_display_id;
    float bytes_0;          /* various byte flags */
    uint32_t sheath_state;
    uint32_t bytes_1;
    uint32_t bytes_2;
    uint32_t emote_state;
    uint32_t auras[16];     /* 16 aura slots */
    uint32_t aura_counts[16];
} WoWUnit;

typedef struct {
    uint64_t guid;
    uint32_t account_id;
    char name[64];
    char ip[32];
    uint8_t recv_buf[65536];
    int recv_len;
    uint8_t send_buf[65536];
    int send_len;
    uint64_t player_guid;
    char username[32];
    bool authenticated;
    bool force_exit;
    uint8_t logged_in;
    float pos_x, pos_y, pos_z;
    float facing;
    float o;
    uint32_t map_id;
    uint32_t zone_id;
    uint32_t character_id;
    uint32_t level;
    uint32_t race;
    uint32_t class_;
    uint32_t health;
    uint32_t power;
    uint8_t sex;
    uint32_t model;
    uint32_t guild_id;
    uint8_t at_login;
} WorldPlayer;

typedef struct {
    uint64_t guid;
    char name[64];
    float x, y, z;
    float o;
    uint32_t map_id;
    uint32_t zone_id;
    uint32_t id;
    uint32_t level;
    uint32_t health;
    uint32_t max_health;
    uint32_t mana;
    uint32_t max_mana;
    uint32_t model;
    uint32_t faction;
    uint32_t npc_flags;
    uint32_t speed_walk;
    uint32_t speed_run;
    uint32_t speed_swim;
    uint32_t speed_fly;
    uint8_t unit_type;
    uint8_t rank;
    uint32_t elite;
    uint32_t max_level;
    uint32_t spawn_mask;
    float bounding_radius;
    uint32_t attack_time;
    uint32_t damage_min;
    uint32_t damage_max;
    float combat_reach;
    uint32_t school_mask;
} WoWCreature;

typedef struct {
    uint32_t id;
    char name[64];
    uint32_t map_id;
    float x, y, z;
    float o;
    float radius;
    uint32_t zone_id;
    uint32_t area_id;
} WoWAreaTrigger;

typedef struct {
    uint32_t spell_id;
    uint32_t caster;
    uint32_t target;
    uint32_t effect;
    uint32_t base_damage;
    uint32_t bonus_damage;
    float crit_chance;
    uint32_t speed;
    uint32_t flags;
} WoWSpell;

typedef struct {
    uint32_t quest_id;
    uint64_t giver_guid;
    uint64_t taker_guid;
    char title[256];
    char description[512];
    uint32_t level;
    uint32_t experience;
    uint32_t gold_reward;
    uint32_t item_rewards[4];
    uint32_t item_choice_rewards[6];
    uint32_t quest_flags;
    uint32_t zone_id;
    uint32_t npc_credit;
    uint32_t required_kills;
    uint32_t required_quests[4];
    uint32_t area_id;
} WoWQuest;

typedef struct {
    uint32_t id;
    uint64_t guid;
    uint32_t player_guid;
    uint32_t entry;
    float pos_x, pos_y, pos_z;
    float o;
    uint32_t map_id;
    uint32_t zone_id;
    uint32_t spawntime;
    uint32_t spawnmask;
    uint32_t curhealth;
    uint32_t curpower;
    uint8_t unit_type;
    uint32_t npc_flags;
    uint32_t faction;
    uint32_t model_id;
    uint32_t level;
    float speed_walk;
    float speed_run;
    uint32_t auras[16];
} WoWSpawnPoint;

#endif /* WOW_WORLD_H */
