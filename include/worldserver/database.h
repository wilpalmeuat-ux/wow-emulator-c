#ifndef DATABASE_H
#define DATABASE_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
    uint64_t guid;
    char name[12];
    uint8_t race;
    uint8_t class_;
    uint8_t level;
    uint32_t map_id;
    float pos_x;
    float pos_y;
    float pos_z;
    float orientation;
    uint32_t zone;
    uint32_t health;
    uint32_t mana;
    uint32_t experience;
    uint32_t money;
    uint32_t player_flags;
    uint32_t display_id;
    uint32_t login_flags;
} CharacterData;
int db_init(void);
void db_close(void);
bool db_create_character(uint32_t account_id, const char* name, uint8_t race, uint8_t class_);
bool db_delete_character(uint64_t guid);
CharacterData* db_load_characters(uint32_t account_id, int* count);
CharacterData* db_load_character(uint64_t guid);
bool db_save_character(CharacterData* chr);
bool db_create_account(const char* username, const char* password);
uint32_t db_get_account_id(const char* username);
bool db_check_account_password(uint32_t account_id, const char* password);
#endif