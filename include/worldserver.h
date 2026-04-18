#ifndef WORLDSERVER_H
#define WORLDSERVER_H
#define WORLD_PORT 8085
#define MAX_CLIENTS 100
#define SQL_DATA "sql/data.db"
void db_init(void);
void db_save_player(int guid, const char* name, int level, int x, int y, int z);
void db_load_player(int guid);
void ws_broadcast(const char* pkt, size_t len);
#endif
