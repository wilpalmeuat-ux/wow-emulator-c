#ifndef DATABASE_H
#define DATABASE_H

#include "Types.h"

typedef struct {
    void* db; /* SQLite handle */
    bool  ready;
} Database;

Database* Database_New(void);
void       Database_Delete(Database* db);
bool       Database_Connect(Database* db, const char* path);
bool       Database_Exec(Database* db, const char* sql);

uint64     Database_LastInsertRowid(Database* db);
const char* Database_ErrMsg(Database* db);

#endif
