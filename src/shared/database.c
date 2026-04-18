/* database.c — SQLite implementation of shared/database.h */
#include "shared/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static int _exec(Database* db, const char* sql) {
    if (!db || !sql) return -1;
    sqlite3* conn = (sqlite3*)db->connection;
    char* err = NULL;
    int rc = sqlite3_exec(conn, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) { if (err) sqlite3_free(err); return -1; }
    return 0;
}

static QueryResult* _query(Database* db, const char* sql) {
    if (!db || !sql) return NULL;
    sqlite3* conn = (sqlite3*)db->connection;
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return NULL;
    QueryResult* qr = calloc(1, sizeof(QueryResult));
    qr->stmt = stmt;
    qr->db = conn;
    qr->field_count = sqlite3_column_count(stmt);
    int rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) rows++;
    qr->row_count = rows;
    sqlite3_reset(stmt);
    return qr;
}

static char* _escape(Database* db, const char* str, char* out, size_t max_len) {
    if (!db || !str || !out) return NULL;
    sqlite3* conn = (sqlite3*)db->connection;
    (void)conn;
    sqlite3_snprintf((int)max_len, out, "%q", str);
    return out;
}

static void _close(Database* db) {
    if (!db) return;
    if (db->connection) { sqlite3_close((sqlite3*)db->connection); db->connection = NULL; }
    free(db);
}

static int _begin(Database* db) { return _exec(db, "BEGIN"); }
static int _commit(Database* db) { return _exec(db, "COMMIT"); }
static int _rollback(Database* db) { return _exec(db, "ROLLBACK"); }

Database* database_create(DBConfig* cfg) {
    Database* db = calloc(1, sizeof(Database));
    db->type = cfg ? cfg->type : 1;
    if (cfg && cfg->type == 0) {
        fprintf(stderr, "[DB] MySQL not compiled; using SQLite\n");
    }
    sqlite3* conn = NULL;
    const char* path = (cfg && cfg->database) ? cfg->database : ":memory:";
    if (sqlite3_open(path, &conn) != SQLITE_OK) {
        if (conn) sqlite3_close(conn);
        free(db); return NULL;
    }
    db->connection = conn;
    db->execute = _exec;
    db->query = _query;
    db->escape = _escape;
    db->close = _close;
    db->begin_transaction = _begin;
    db->commit = _commit;
    db->rollback = _rollback;
    db->connected = true;

    const char* init =
        "CREATE TABLE IF NOT EXISTS characters("
        "guid INTEGER PRIMARY KEY,name TEXT,race INTEGER,class INTEGER,"
        "level INTEGER DEFAULT 1,xp INTEGER DEFAULT 0,money INTEGER DEFAULT 0,"
        "pos_x REAL,pos_y REAL,pos_z REAL,ori REAL,map_id INTEGER,zone_id INTEGER,"
        "account_id INTEGER,health INTEGER,mana INTEGER,online INTEGER DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS creature_spawns("
        "guid INTEGER PRIMARY KEY,id INTEGER,map_id INTEGER,"
        "pos_x REAL,pos_y REAL,pos_z REAL,ori REAL,spawntime INTEGER);"
        "CREATE TABLE IF NOT EXISTS gameobject_spawns("
        "guid INTEGER PRIMARY KEY,id INTEGER,map_id INTEGER,"
        "pos_x REAL,pos_y REAL,pos_z REAL,ori REAL);"
        "CREATE TABLE IF NOT EXISTS world_state(var TEXT PRIMARY KEY,val TEXT);";
    sqlite3_exec(conn, init, NULL, NULL, NULL);
    return db;
}

void database_free(Database* db) { if (db && db->close) db->close(db); }
int db_exec(Database* db, const char* sql) { return db && db->execute ? db->execute(db, sql) : -1; }

int db_save_character(Database* db, uint64_t guid,
    const char* name, uint32_t race, uint32_t class_,
    uint32_t level, uint32_t xp,
    int32_t health, int32_t mana,
    float x, float y, float z, float o,
    uint32_t map_id, uint32_t zone_id,
    uint32_t flags, uint32_t money,
    const uint64_t* inventory, int inv_size) {
    (void)inventory;(void)inv_size;
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT OR REPLACE INTO characters "
        "(guid,name,race,class,level,xp,health,mana,pos_x,pos_y,pos_z,ori,map_id,zone_id,flags,money) "
        "VALUES (%llu,'%s',%u,%u,%u,%u,%d,%d,%.2f,%.2f,%.2f,%.4f,%u,%u,%u,%u)",
        (unsigned long long)guid, name, race, class_, level, xp,
        health, mana, x, y, z, o, map_id, zone_id, flags, money);
    return db_exec(db, sql);
}

int db_load_characters(Database* db, uint64_t account_id,
    void (*callback)(uint64_t, const char*, uint32_t, uint32_t, uint32_t, float, float, float, uint32_t, void*),
    void* user_data) {
    if (!db) return 0;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT guid,name,race,class,level,pos_x,pos_y,pos_z,map_id FROM characters WHERE account_id=%llu",
        (unsigned long long)account_id);
    QueryResult* qr = db->query(db, sql);
    if (!qr) return 0;
    int count = 0;
    sqlite3_stmt* stmt = (sqlite3_stmt*)qr->stmt;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        uint64_t guid = (uint64_t)sqlite3_column_int64(stmt, 0);
        const char* name = (const char*)sqlite3_column_text(stmt, 1);
        uint32_t race = (uint32_t)sqlite3_column_int(stmt, 2);
        uint32_t cls  = (uint32_t)sqlite3_column_int(stmt, 3);
        uint32_t lvl  = (uint32_t)sqlite3_column_int(stmt, 4);
        float x = (float)sqlite3_column_double(stmt, 5);
        float y = (float)sqlite3_column_double(stmt, 6);
        float z = (float)sqlite3_column_double(stmt, 7);
        uint32_t map  = (uint32_t)sqlite3_column_int(stmt, 8);
        if (callback) callback(guid, name, race, cls, lvl, x, y, z, map, user_data);
        count++;
    }
    sqlite3_finalize(stmt);
    free(qr);
    return count;
}

int db_delete_character(Database* db, uint64_t guid) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM characters WHERE guid=%llu", (unsigned long long)guid);
    return db_exec(db, sql);
}

int db_character_exists(Database* db, uint64_t guid) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT 1 FROM characters WHERE guid=%llu", (unsigned long long)guid);
    QueryResult* qr = db->query(db, sql);
    if (!qr) return 0;
    sqlite3_stmt* stmt = (sqlite3_stmt*)qr->stmt;
    int ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    free(qr);
    return ok;
}

int db_load_realm(Database* db, uint32_t realm_id,
    char* name, char* address, int* port,
    uint32_t* max_players, uint32_t* current_players) {
    (void)db;(void)realm_id;(void)name;(void)address;(void)port;(void)max_players;(void)current_players;
    return 0;
}