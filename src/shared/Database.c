#include "shared/Database.h"
#include <sqlite3.h>
#include <string.h>

Database* Database_New(void) {
    Database* db = calloc(1, sizeof(Database));
    return db;
}

void Database_Delete(Database* db) {
    if (!db) return;
    if (db->db) sqlite3_close(db->db);
    free(db);
}

bool Database_Connect(Database* db, const char* path) {
    if (sqlite3_open(path, (sqlite3**)&db->db) != SQLITE_OK) return false;
    db->ready = true;
    return true;
}

bool Database_Exec(Database* db, const char* sql) {
    if (!db->db) return false;
    char* err = NULL;
    int rc = sqlite3_exec(db->db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

uint64 Database_LastInsertRowid(Database* db) {
    if (!db->db) return 0;
    return (uint64)sqlite3_last_insert_rowid(db->db);
}

const char* Database_ErrMsg(Database* db) {
    if (!db->db) return "no database";
    return sqlite3_errmsg(db->db);
}
