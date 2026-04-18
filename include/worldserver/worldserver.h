#include "shared/NetworkCompat.h"
#ifndef WORLDSERVER_H
#define WORLDSERVER_H
#include <stdint.h>
#include <sqlite3.h>

#define MAX_PLAYERS 256
#define MAX_GROUPS 50
#define MAX_GUILDS 50
#define MAX_CREATURES 512
#define MAX_GAMEOBJECTS 256
#define MAXQUESTS 128
#define MAXZONES 32

typedef struct Player {
    uint64_t guid;
    char name[32];
    uint32_t account_id;
    uint32_t session_key;
    uint8_t race;
    uint8_t class_;
    uint8_t gender;
    uint8_t skin_id, face_id, hair_style, hair_color, facial_features;
    uint8_t level;
    uint32_t exp;
    uint8_t slot;
    float x, y, z, o;
    uint32_t map_id;
    uint32_t zone_id;
    uint32_t health;
    uint32_t max_health;
    uint32_t mana;
    uint32_t max_mana;
    uint32_t strength;
    uint32_t agility;
    uint32_t stamina;
    uint32_t spirit;
    uint32_t intellect;
    uint32_t armor;
    float dodge, parry, block, crit;
    uint32_t gold;
    uint8_t stand_state;
    uint8_t sheath_state;
    uint32_t chat_tag;
    int socket;
    time_t logout_time;
    uint8_t active;
    int combat_timer;
    int health_regen_timer;
    int mana_regen_timer;
} Player;

typedef struct WorldObject {
    uint64_t guid;
    uint32_t entry;
    float x, y, z, o;
    uint32_t map_id;
    uint32_t zone_id;
    uint32_t flags;
} WorldObject;

typedef struct CreatureTemplate {
    uint32_t entry;
    char name[64];
    uint32_t level;
    uint32_t health;
    uint32_t mana;
    uint32_t flags;
    uint32_t model_id;
} CreatureTemplate;

typedef struct QuestData {
    uint32_t id;
    char title[128];
    char description[512];
    char objective[256];
    uint32_t required_count;
    uint32_t creature_entry;
    uint32_t reward_exp;
    uint32_t reward_gold;
} QuestData;

typedef struct ZoneData {
    uint32_t id;
    char name[64];
    uint32_t level_min;
    uint32_t level_max;
    uint8_t pvp_flag;
} ZoneData;

typedef struct ChatMessage {
    char sender[32];
    char message[256];
    uint32_t type;
    uint32_t timestamp;
} ChatMessage;

typedef struct Group {
    uint32_t id;
    uint32_t leader_guid;
    uint32_t members[40];
    int member_count;
} Group;

typedef struct Guild {
    uint32_t id;
    char name[64];
    uint32_t leader_guid;
    uint32_t members[500];
    int member_count;
    uint32_t emblem_color;
    uint32_t created_at;
} Guild;



int start_world_server(const char* ip, int port);
void stop_world_server(void);
int db_init(const char* path);
int db_save_player(Player* p);
int db_load_characters(int account_id, Player* out, int max);
void handle_packet(int sock, uint16_t opcode, const uint8_t* data, size_t len);
void send_to_client(int sock, const uint8_t* data, size_t len);
void send_world_update(Player* p);
void update_player_stats(Player* p);
void level_up_player(Player* p);
#endif
