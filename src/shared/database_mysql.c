/* database.c — MySQL implementation of shared/database.h for Windows */
#include "shared/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql/mysql.h>

typedef struct QueryResult_t {
    MYSQL_STMT* stmt;
    MYSQL*       db;
    int          field_count;
    int          row_count;
    MYSQL_RES*   result;
    int          row_pos;
} QueryResult;

static int _exec(Database* db, const char* sql) {
    if (!db || !sql) return -1;
    if (mysql_query((MYSQL*)db->connection, sql) != 0) {
        fprintf(stderr, "[MySQL] Exec error: %s\n", mysql_error((MYSQL*)db->connection));
        return -1;
    }
    return 0;
}

static QueryResult* _query(Database* db, const char* sql) {
    if (!db || !sql) return NULL;
    MYSQL* conn = (MYSQL*)db->connection;
    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "[MySQL] Query error: %s\n", mysql_error(conn));
        return NULL;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return NULL;
    QueryResult* qr = (QueryResult*)calloc(1, sizeof(QueryResult));
    qr->result = res;
    qr->db = conn;
    qr->field_count = (int)mysql_num_fields(res);
    qr->row_count = (int)mysql_num_rows(res);
    qr->row_pos = 0;
    return qr;
}

static int _next_row(QueryResult* qr) {
    if (!qr || !qr->result) return 0;
    MYSQL_ROW row = mysql_fetch_row(qr->result);
    if (!row) return 0;
    qr->row_pos++;
    return 1;
}

static const char* _get_field(QueryResult* qr, int col) {
    if (!qr || !qr->result) return NULL;
    MYSQL_ROW row = mysql_fetch_row(qr->result);
    (void)col;
    return row ? row[col] : NULL;
}

static char* _escape(Database* db, const char* str, char* out, size_t max_len) {
    if (!db || !str || !out) return NULL;
    MYSQL* conn = (MYSQL*)db->connection;
    mysql_real_escape_string(conn, out, str, (unsigned long)strlen(str));
    return out;
}

static void _close(Database* db) {
    if (!db) return;
    if (db->connection) {
        mysql_close((MYSQL*)db->connection);
        db->connection = NULL;
    }
    free(db);
}

static int _begin(Database* db)   { return _exec(db, "BEGIN"); }
static int _commit(Database* db) { return _exec(db, "COMMIT"); }
static int _rollback(Database* db){ return _exec(db, "ROLLBACK"); }

Database* database_create(DBConfig* cfg) {
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "[MySQL] Init failed\n");
        return NULL;
    }

    const char* host     = cfg && cfg->host     ? cfg->host     : "127.0.0.1";
    int         port     = cfg && cfg->port     ? cfg->port     : 3306;
    const char* user     = cfg && cfg->user     ? cfg->user     : "root";
    const char* password = cfg && cfg->password ? cfg->password : "";
    const char* database = cfg && cfg->database ? cfg->database : "wow_emulator";

    if (!mysql_real_connect(conn, host, user, password, database, port, NULL, 0)) {
        fprintf(stderr, "[MySQL] Connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    fprintf(stderr, "[MySQL] Connected to %s:%d/%s\n", host, port, database);

    Database* db = (Database*)calloc(1, sizeof(Database));
    db->type = 0; /* MySQL */
    db->connection = conn;
    db->execute    = _exec;
    db->query      = _query;
    db->escape     = _escape;
    db->close      = _close;
    db->begin_transaction = _begin;
    db->commit     = _commit;
    db->rollback   = _rollback;
    db->connected  = true;

    /* Create tables if they don't exist */
    const char* create_sql =
        "CREATE TABLE IF NOT EXISTS characters("
        "  guid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  account_id INT UNSIGNED NOT NULL,"
        "  name VARCHAR(32) NOT NULL,"
        "  race INT UNSIGNED NOT NULL,"
        "  class INT UNSIGNED NOT NULL,"
        "  level INT UNSIGNED NOT NULL DEFAULT 1,"
        "  xp INT UNSIGNED NOT NULL DEFAULT 0,"
        "  money BIGINT UNSIGNED NOT NULL DEFAULT 0,"
        "  pos_x FLOAT NOT NULL DEFAULT 0,"
        "  pos_y FLOAT NOT NULL DEFAULT 0,"
        "  pos_z FLOAT NOT NULL DEFAULT 0,"
        "  ori FLOAT NOT NULL DEFAULT 0,"
        "  map_id INT UNSIGNED NOT NULL DEFAULT 0,"
        "  zone_id INT UNSIGNED NOT NULL DEFAULT 0,"
        "  health INT UNSIGNED NOT NULL DEFAULT 100,"
        "  mana INT UNSIGNED NOT NULL DEFAULT 100,"
        "  online TINYINT NOT NULL DEFAULT 0"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
        "CREATE TABLE IF NOT EXISTS creature_spawns("
        "  guid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  entry_id INT UNSIGNED NOT NULL,"
        "  map_id INT UNSIGNED NOT NULL,"
        "  pos_x FLOAT NOT NULL,"
        "  pos_y FLOAT NOT NULL,"
        "  pos_z FLOAT NOT NULL,"
        "  ori FLOAT NOT NULL,"
        "  spawntime_secs INT UNSIGNED NOT NULL DEFAULT 300"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
        "CREATE TABLE IF NOT EXISTS gameobject_spawns("
        "  guid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "  entry_id INT UNSIGNED NOT NULL,"
        "  map_id INT UNSIGNED NOT NULL,"
        "  pos_x FLOAT NOT NULL,"
        "  pos_y FLOAT NOT NULL,"
        "  pos_z FLOAT NOT NULL,"
        "  ori FLOAT NOT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
        "CREATE TABLE IF NOT EXISTS world_state("
        "  var_name VARCHAR(64) NOT NULL PRIMARY KEY,"
        "  value VARCHAR(255) NOT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;";

    /* Execute each statement individually */
    char stmt[4096];
    const char* p = create_sql;
    const char* end = p + strlen(p);
    while (p < end) {
        size_t len = 0;
        /* find end of statement */
        const char* q = p;
        while (q < end && *q != ';') q++;
        len = (size_t)(q - p);
        if (len > 0 && len < sizeof(stmt)) {
            memcpy(stmt, p, len);
            stmt[len] = '\0';
            mysql_query(conn, stmt);
        }
        if (*q == ';') q++;
        while (q < end && (*q == ' ' || *q == '\n' || *q == '\r' || *q == '\t')) q++;
        p = q;
    }

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
    uint32_t flags, uint64_t money,
    const uint64_t* inventory, int inv_size) {
    (void)inventory; (void)inv_size;
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO characters "
        "(guid,name,race,class,level,xp,health,mana,pos_x,pos_y,pos_z,ori,map_id,zone_id,flags,money) "
        "VALUES (%llu,'%s',%u,%u,%u,%u,%d,%d,%.2f,%.2f,%.2f,%.4f,%u,%u,%u,%llu) "
        "ON DUPLICATE KEY UPDATE "
        "name=VALUES(name),race=VALUES(race),class=VALUES(class),level=VALUES(level),"
        "xp=VALUES(xp),health=VALUES(health),mana=VALUES(mana),"
        "pos_x=VALUES(pos_x),pos_y=VALUES(pos_y),pos_z=VALUES(pos_z),ori=VALUES(ori),"
        "map_id=VALUES(map_id),zone_id=VALUES(zone_id),money=VALUES(money)",
        (unsigned long long)guid, name, race, class_, level, xp,
        health, mana, x, y, z, o, map_id, zone_id, flags, (unsigned long long)money);
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
    while (_next_row(qr)) {
        const char* row = mysql_fetch_row(qr->result);
        uint64_t guid = (uint64_t)atoll(row[0]);
        const char* name = row[1] ? row[1] : "";
        uint32_t race = (uint32_t)atoi(row[2]);
        uint32_t cls  = (uint32_t)atoi(row[3]);
        uint32_t lvl  = (uint32_t)atoi(row[4]);
        float x = (float)atof(row[5]);
        float y = (float)atof(row[6]);
        float z = (float)atof(row[7]);
        uint32_t map  = (uint32_t)atoi(row[8]);
        if (callback) callback(guid, name, race, cls, lvl, x, y, z, map, user_data);
        count++;
    }
    mysql_free_result(qr->result);
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
    snprintf(sql, sizeof(sql), "SELECT 1 FROM characters WHERE guid=%llu LIMIT 1", (unsigned long long)guid);
    QueryResult* qr = db->query(db, sql);
    if (!qr) return 0;
    int ok = qr->row_count > 0;
    mysql_free_result(qr->result);
    free(qr);
    return ok;
}

int db_load_realm(Database* db, uint32_t realm_id,
    char* name, char* address, int* port,
    uint32_t* max_players, uint32_t* current_players) {
    (void)db; (void)realm_id;
    if (name) strncpy(name, "WoW 3.3.5a Emulator", 64);
    if (address) strncpy(address, "127.0.0.1", 32);
    if (port) *port = 8085;
    if (max_players) *max_players = 5000;
    if (current_players) *current_players = 0;
    return 0;
}
