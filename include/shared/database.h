/* shared/database.h — C MySQL-only DB layer for WoW emulator
 * Requires libmysqlclient. No SQLite.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Database Database;
typedef struct QueryResult QueryResult;

/* Database config — MySQL only */
typedef struct DBConfig {
    char* host;
    int   port;
    char* user;
    char* pass;
    char* database;
    int   type; /* 0=mysql (only supported) */
} DBConfig;

/* Database handle */
struct Database {
    void*          connection;   /* MYSQL* */
    int            type;        /* 0=mysql */
    bool           connected;
    int (*execute)(Database* db, const char* sql);
    QueryResult* (*query)(Database* db, const char* sql);
    char* (*escape)(Database* db, const char* str, char* out, size_t max_len);
    void (*close)(Database* db);
    int (*begin_transaction)(Database* db);
    int (*commit)(Database* db);
    int (*rollback)(Database* db);
};

/* Query result */
struct QueryResult {
    void*  stmt;          /* MYSQL_STMT* (prepared) or unused */
    void*  raw_result;    /* MYSQL_RES* for non-prepared queries */
    void*  row;           /* MYSQL_ROW */
    int    row_count;
    int    field_count;
    int    current_row;
    char** field_names;   /* freed separately */
    char** fields;        /* current row field data */
};

/* ── Core ── */
Database*  database_create(DBConfig* cfg);
void       database_free(Database* db);
int        db_exec(Database* db, const char* sql);

/* ── Character DB ── */
int db_save_character(Database* db, uint64_t guid,
    const char* name, uint32_t race, uint32_t class_,
    uint32_t level, uint32_t xp,
    int32_t health, int32_t mana,
    float x, float y, float z, float o,
    uint32_t map_id, uint32_t zone_id,
    uint32_t flags, uint32_t money,
    const uint64_t* inventory, int inv_size);

int db_load_characters(Database* db, uint64_t account_id,
    void (*callback)(uint64_t, const char*, uint32_t, uint32_t, uint32_t,
                     float, float, float, uint32_t, void*),
    void* user_data);

int db_delete_character(Database* db, uint64_t guid);
int db_character_exists(Database* db, uint64_t guid);

/* ── Realm DB ── */
int db_load_realm(Database* db, uint32_t realm_id,
    char* name, char* address, int* port,
    uint32_t* max_players, uint32_t* current_players);

/* ── SQL utility (caller must free result) ── */
typedef struct {
    uint64_t* guids;
    char**    names;
    uint32_t* races;
    uint32_t* classes;
    uint32_t* levels;
    float*    pos_x;
    float*    pos_y;
    float*    pos_z;
    uint32_t* maps;
    int       count;
} CharacterListResult;

void character_result_free(CharacterListResult* r);

/* ── Prepared statement types (MySQL only) ── */
typedef struct PreparedStmt PreparedStmt;

PreparedStmt* db_prepare(Database* db, const char* sql);
void          db_stmt_free(PreparedStmt* stmt);
int           db_stmt_execute(PreparedStmt* stmt);
const char*   db_last_error(Database* db);
uint64_t      db_last_insert_id(Database* db);
