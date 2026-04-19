/* database.c -- MySQL-only implementation of shared/database.h (Windows)
 * Requires libmysql (MySQL Connector/C).
 */
#include "shared/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif

/* ================================================================
 *  Internal helpers
 * ================================================================ */

static int _exec(Database* db, const char* sql) {
    if (!db || !sql || !db->connection) return -1;
    MYSQL* conn = (MYSQL*)db->connection;
    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "[MySQL] Exec error: %s\n  SQL: %.200s\n",
                mysql_error(conn), sql);
        return -1;
    }
    return 0;
}

static QueryResult* _query(Database* db, const char* sql) {
    if (!db || !sql || !db->connection) return NULL;
    MYSQL* conn = (MYSQL*)db->connection;
    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "[MySQL] Query error: %s\n  SQL: %.200s\n",
                mysql_error(conn), sql);
        return NULL;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return NULL;

    QueryResult* qr = (QueryResult*)calloc(1, sizeof(QueryResult));
    qr->raw_result  = res;
    qr->field_count = (int)mysql_num_fields(res);
    qr->row_count   = (int)mysql_num_rows(res);
    qr->current_row = 0;
    return qr;
}

static char* _escape(Database* db, const char* str, char* out, size_t max_len) {
    if (!db || !str || !out || !db->connection) return NULL;
    MYSQL* conn = (MYSQL*)db->connection;
    unsigned long slen = (unsigned long)strlen(str);
    if (slen * 2 + 1 > max_len) slen = (unsigned long)(max_len / 2 - 1);
    mysql_real_escape_string(conn, out, str, slen);
    return out;
}

static void _close(Database* db) {
    if (!db) return;
    if (db->connection) {
        mysql_close((MYSQL*)db->connection);
        db->connection = NULL;
    }
    db->connected = false;
}

static int _begin(Database* db)    { return _exec(db, "START TRANSACTION"); }
static int _commit(Database* db)   { return _exec(db, "COMMIT"); }
static int _rollback(Database* db) { return _exec(db, "ROLLBACK"); }

/* ================================================================
 *  Helper: execute a multi-statement SQL string one statement at
 *  a time (MySQL C API does not allow multi-statement by default).
 * ================================================================ */
static void _exec_statements(MYSQL* conn, const char* sql) {
    const char* p = sql;
    const char* end = p + strlen(p);
    char stmt[4096];

    while (p < end) {
        /* skip whitespace */
        while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            p++;
        if (p >= end) break;

        /* find semicolon */
        const char* q = p;
        while (q < end && *q != ';') q++;
        size_t len = (size_t)(q - p);
        if (len > 0 && len < sizeof(stmt)) {
            memcpy(stmt, p, len);
            stmt[len] = '\0';
            if (mysql_query(conn, stmt) != 0) {
                fprintf(stderr, "[MySQL] Init SQL error: %s\n  SQL: %.200s\n",
                        mysql_error(conn), stmt);
            }
        }
        p = (q < end) ? q + 1 : end;
    }
}

/* ================================================================
 *  Public: database_create  -- connect + auto-create tables
 * ================================================================ */
Database* database_create(DBConfig* cfg) {
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "[MySQL] mysql_init() failed\n");
        return NULL;
    }

    const char* host     = (cfg && cfg->host)     ? cfg->host     : "127.0.0.1";
    int         port     = (cfg && cfg->port)      ? cfg->port     : 3306;
    const char* user     = (cfg && cfg->user)      ? cfg->user     : "root";
    const char* password = (cfg && cfg->password)  ? cfg->password : "";
    const char* database = (cfg && cfg->database)  ? cfg->database : "wow_emulator";

    /* Try to connect without DB first to create it if missing */
    if (!mysql_real_connect(conn, host, user, password, NULL, port, NULL, 0)) {
        fprintf(stderr, "[MySQL] Connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    /* Create database if it does not exist */
    char createDb[256];
    snprintf(createDb, sizeof(createDb),
             "CREATE DATABASE IF NOT EXISTS `%s` CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci",
             database);
    mysql_query(conn, createDb);

    /* Select the database */
    if (mysql_select_db(conn, database) != 0) {
        fprintf(stderr, "[MySQL] Cannot select database '%s': %s\n",
                database, mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    printf("[MySQL] Connected to %s@%s:%d/%s\n", user, host, port, database);

    Database* db = (Database*)calloc(1, sizeof(Database));
    db->type       = 0; /* MySQL */
    db->connection = conn;
    db->execute    = _exec;
    db->query      = (QueryResult* (*)(Database*, const char*))_query;
    db->escape     = _escape;
    db->close      = _close;
    db->begin_transaction = _begin;
    db->commit     = _commit;
    db->rollback   = _rollback;
    db->connected  = true;

    /* ---- Auto-create core tables ---- */
    static const char* init_sql =
        /* Accounts (used by auth server) */
        "CREATE TABLE IF NOT EXISTS accounts ("
        "  id           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  username     VARCHAR(32)  NOT NULL UNIQUE,"
        "  salt         CHAR(64)     NOT NULL DEFAULT '',"
        "  verifier     CHAR(128)    NOT NULL DEFAULT '',"
        "  session_key  CHAR(80)     NOT NULL DEFAULT '',"
        "  gmlevel      TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  banned       TINYINT      NOT NULL DEFAULT 0,"
        "  last_ip      VARCHAR(45)  NOT NULL DEFAULT '',"
        "  last_login   DATETIME     NULL,"
        "  created_at   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* Characters */
        "CREATE TABLE IF NOT EXISTS characters ("
        "  guid         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  account_id   INT UNSIGNED NOT NULL,"
        "  name         VARCHAR(32)  NOT NULL,"
        "  race         TINYINT UNSIGNED NOT NULL,"
        "  class        TINYINT UNSIGNED NOT NULL,"
        "  gender       TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  level        SMALLINT UNSIGNED NOT NULL DEFAULT 1,"
        "  xp           INT UNSIGNED NOT NULL DEFAULT 0,"
        "  money        BIGINT UNSIGNED NOT NULL DEFAULT 0,"
        "  skin         TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  face         TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  hairstyle    TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  haircolor    TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  facialstyle  TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  pos_x        FLOAT NOT NULL DEFAULT 0,"
        "  pos_y        FLOAT NOT NULL DEFAULT 0,"
        "  pos_z        FLOAT NOT NULL DEFAULT 0,"
        "  ori          FLOAT NOT NULL DEFAULT 0,"
        "  map_id       INT UNSIGNED NOT NULL DEFAULT 0,"
        "  zone_id      INT UNSIGNED NOT NULL DEFAULT 0,"
        "  health       INT NOT NULL DEFAULT 100,"
        "  mana         INT NOT NULL DEFAULT 100,"
        "  online       TINYINT NOT NULL DEFAULT 0,"
        "  flags        INT UNSIGNED NOT NULL DEFAULT 0,"
        "  INDEX idx_account (account_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* Creature spawns */
        "CREATE TABLE IF NOT EXISTS creature_spawns ("
        "  guid         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  entry_id     INT UNSIGNED NOT NULL,"
        "  map_id       INT UNSIGNED NOT NULL DEFAULT 0,"
        "  pos_x        FLOAT NOT NULL DEFAULT 0,"
        "  pos_y        FLOAT NOT NULL DEFAULT 0,"
        "  pos_z        FLOAT NOT NULL DEFAULT 0,"
        "  ori          FLOAT NOT NULL DEFAULT 0,"
        "  spawntime    INT UNSIGNED NOT NULL DEFAULT 300,"
        "  INDEX idx_map (map_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* Creature templates */
        "CREATE TABLE IF NOT EXISTS creature_template ("
        "  entry        INT UNSIGNED NOT NULL PRIMARY KEY,"
        "  name         VARCHAR(100) NOT NULL DEFAULT 'Unknown',"
        "  subname      VARCHAR(100) NOT NULL DEFAULT '',"
        "  min_level    TINYINT UNSIGNED NOT NULL DEFAULT 1,"
        "  max_level    TINYINT UNSIGNED NOT NULL DEFAULT 1,"
        "  display_id   INT UNSIGNED NOT NULL DEFAULT 0,"
        "  faction      INT UNSIGNED NOT NULL DEFAULT 0,"
        "  health       INT UNSIGNED NOT NULL DEFAULT 100,"
        "  mana         INT UNSIGNED NOT NULL DEFAULT 0,"
        "  armor        INT UNSIGNED NOT NULL DEFAULT 0,"
        "  attack_power INT UNSIGNED NOT NULL DEFAULT 10,"
        "  damage_min   FLOAT NOT NULL DEFAULT 1,"
        "  damage_max   FLOAT NOT NULL DEFAULT 5,"
        "  speed_walk   FLOAT NOT NULL DEFAULT 1.0,"
        "  speed_run    FLOAT NOT NULL DEFAULT 1.14,"
        "  npc_flags    INT UNSIGNED NOT NULL DEFAULT 0,"
        "  unit_flags   INT UNSIGNED NOT NULL DEFAULT 0,"
        "  type         TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  loot_id      INT UNSIGNED NOT NULL DEFAULT 0,"
        "  script       VARCHAR(64) NOT NULL DEFAULT ''"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* GameObject spawns */
        "CREATE TABLE IF NOT EXISTS gameobject_spawns ("
        "  guid         INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  entry_id     INT UNSIGNED NOT NULL,"
        "  map_id       INT UNSIGNED NOT NULL DEFAULT 0,"
        "  pos_x        FLOAT NOT NULL DEFAULT 0,"
        "  pos_y        FLOAT NOT NULL DEFAULT 0,"
        "  pos_z        FLOAT NOT NULL DEFAULT 0,"
        "  ori          FLOAT NOT NULL DEFAULT 0,"
        "  rot0         FLOAT NOT NULL DEFAULT 0,"
        "  rot1         FLOAT NOT NULL DEFAULT 0,"
        "  rot2         FLOAT NOT NULL DEFAULT 0,"
        "  rot3         FLOAT NOT NULL DEFAULT 0,"
        "  state        INT UNSIGNED NOT NULL DEFAULT 1,"
        "  INDEX idx_map (map_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* World variables (key/value store) */
        "CREATE TABLE IF NOT EXISTS world_state ("
        "  var_name     VARCHAR(64)  NOT NULL PRIMARY KEY,"
        "  value        VARCHAR(255) NOT NULL DEFAULT ''"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* Realm list */
        "CREATE TABLE IF NOT EXISTS realmlist ("
        "  id           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  name         VARCHAR(64)  NOT NULL DEFAULT 'WoW Emulator',"
        "  address      VARCHAR(128) NOT NULL DEFAULT '127.0.0.1',"
        "  port         INT UNSIGNED NOT NULL DEFAULT 8085,"
        "  type         TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  flags        TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "  population   FLOAT NOT NULL DEFAULT 0,"
        "  timezone     TINYINT UNSIGNED NOT NULL DEFAULT 1"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

        /* Insert default realm */
        "INSERT IGNORE INTO realmlist (id, name, address, port) "
        "VALUES (1, 'WoW 3.3.5a Emulator', '127.0.0.1', 8085);"

        /* Insert default test account (password: testpassword) */
        "INSERT IGNORE INTO accounts (id, username, salt, verifier, gmlevel) "
        "VALUES (1, 'TEST', "
        "'0000000000000000000000000000000000000000000000000000000000000000', "
        "'0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000', 3);"
    ;

    _exec_statements(conn, init_sql);
    printf("[MySQL] Database tables initialized.\n");
    return db;
}

void database_free(Database* db) {
    if (db) {
        if (db->close) db->close(db);
        free(db);
    }
}

int db_exec(Database* db, const char* sql) {
    return (db && db->execute) ? db->execute(db, sql) : -1;
}

const char* db_last_error(Database* db) {
    if (!db || !db->connection) return "no connection";
    return mysql_error((MYSQL*)db->connection);
}

uint64_t db_last_insert_id(Database* db) {
    if (!db || !db->connection) return 0;
    return (uint64_t)mysql_insert_id((MYSQL*)db->connection);
}

/* ================================================================
 *  Character CRUD
 * ================================================================ */
int db_save_character(Database* db, uint64_t guid,
    const char* name, uint32_t race, uint32_t class_,
    uint32_t level, uint32_t xp,
    int32_t health, int32_t mana,
    float x, float y, float z, float o,
    uint32_t map_id, uint32_t zone_id,
    uint32_t flags, uint32_t money,
    const uint64_t* inventory, int inv_size)
{
    (void)inventory; (void)inv_size;
    char sql[2048];
    char esc_name[128];
    db->escape(db, name ? name : "Unknown", esc_name, sizeof(esc_name));

    snprintf(sql, sizeof(sql),
        "INSERT INTO characters "
        "(guid,name,race,class,level,xp,health,mana,pos_x,pos_y,pos_z,ori,"
        "map_id,zone_id,flags,money) "
        "VALUES (%llu,'%s',%u,%u,%u,%u,%d,%d,%.4f,%.4f,%.4f,%.4f,%u,%u,%u,%u) "
        "ON DUPLICATE KEY UPDATE "
        "name=VALUES(name),race=VALUES(race),class=VALUES(class),"
        "level=VALUES(level),xp=VALUES(xp),health=VALUES(health),"
        "mana=VALUES(mana),pos_x=VALUES(pos_x),pos_y=VALUES(pos_y),"
        "pos_z=VALUES(pos_z),ori=VALUES(ori),map_id=VALUES(map_id),"
        "zone_id=VALUES(zone_id),flags=VALUES(flags),money=VALUES(money)",
        (unsigned long long)guid, esc_name, race, class_, level, xp,
        health, mana, x, y, z, o, map_id, zone_id, flags, money);
    return db_exec(db, sql);
}

int db_load_characters(Database* db, uint64_t account_id,
    void (*callback)(uint64_t, const char*, uint32_t, uint32_t, uint32_t,
                     float, float, float, uint32_t, void*),
    void* user_data)
{
    if (!db || !db->connection) return 0;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT guid,name,race,class,level,pos_x,pos_y,pos_z,map_id "
        "FROM characters WHERE account_id=%llu",
        (unsigned long long)account_id);

    if (mysql_query((MYSQL*)db->connection, sql) != 0) return 0;
    MYSQL_RES* res = mysql_store_result((MYSQL*)db->connection);
    if (!res) return 0;

    int count = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        uint64_t guid = row[0] ? (uint64_t)strtoull(row[0], NULL, 10) : 0;
        const char* name = row[1] ? row[1] : "Unknown";
        uint32_t race  = row[2] ? (uint32_t)atoi(row[2]) : 1;
        uint32_t cls   = row[3] ? (uint32_t)atoi(row[3]) : 1;
        uint32_t lvl   = row[4] ? (uint32_t)atoi(row[4]) : 1;
        float px = row[5] ? (float)atof(row[5]) : 0;
        float py = row[6] ? (float)atof(row[6]) : 0;
        float pz = row[7] ? (float)atof(row[7]) : 0;
        uint32_t map   = row[8] ? (uint32_t)atoi(row[8]) : 0;
        if (callback) callback(guid, name, race, cls, lvl, px, py, pz, map, user_data);
        count++;
    }
    mysql_free_result(res);
    return count;
}

int db_delete_character(Database* db, uint64_t guid) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "DELETE FROM characters WHERE guid=%llu",
        (unsigned long long)guid);
    return db_exec(db, sql);
}

int db_character_exists(Database* db, uint64_t guid) {
    if (!db || !db->connection) return 0;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT 1 FROM characters WHERE guid=%llu LIMIT 1",
        (unsigned long long)guid);
    if (mysql_query((MYSQL*)db->connection, sql) != 0) return 0;
    MYSQL_RES* res = mysql_store_result((MYSQL*)db->connection);
    if (!res) return 0;
    int exists = (mysql_fetch_row(res) != NULL) ? 1 : 0;
    mysql_free_result(res);
    return exists;
}

int db_load_realm(Database* db, uint32_t realm_id,
    char* name, char* address, int* port,
    uint32_t* max_players, uint32_t* current_players)
{
    if (!db || !db->connection) return 0;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT name,address,port FROM realmlist WHERE id=%u",
        realm_id);
    if (mysql_query((MYSQL*)db->connection, sql) != 0) return 0;
    MYSQL_RES* res = mysql_store_result((MYSQL*)db->connection);
    if (!res) return 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) { mysql_free_result(res); return 0; }
    if (name && row[0]) strncpy(name, row[0], 63);
    if (address && row[1]) strncpy(address, row[1], 127);
    if (port && row[2]) *port = atoi(row[2]);
    if (max_players) *max_players = 5000;
    if (current_players) *current_players = 0;
    mysql_free_result(res);
    return 1;
}

void character_result_free(CharacterListResult* r) {
    if (!r) return;
    for (int i = 0; i < r->count; i++) {
        if (r->names && r->names[i]) free(r->names[i]);
    }
    free(r->guids);
    free(r->names);
    free(r->races);
    free(r->classes);
    free(r->levels);
    free(r->pos_x);
    free(r->pos_y);
    free(r->pos_z);
    free(r->maps);
}

PreparedStmt* db_prepare(Database* db, const char* sql) {
    (void)db; (void)sql;
    /* Prepared statements stub -- extend as needed */
    return NULL;
}

void db_stmt_free(PreparedStmt* stmt) { (void)stmt; }
int  db_stmt_execute(PreparedStmt* stmt) { (void)stmt; return -1; }
