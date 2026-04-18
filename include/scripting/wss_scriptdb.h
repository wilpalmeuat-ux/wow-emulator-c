/* ╔══════════════════════════════════════════════════════════════╗
   ║  Script File Index — registers all loaded .wss scripts and    ║
   ║  exposes lookup by name and by event type for dispatch.        ║
   ╚══════════════════════════════════════════════════════════════╝ */

#ifndef WSS_SCRIPTDB_H
#define WSS_SCRIPTDB_H

#include "wss_engine.h"
#include <uthash/utarray.h>
#include <uthash/utstring.h>
#include <uthash/uthash.h>

/* ── Registered script file ───────────────────────────────────── */
typedef struct WssScriptFile {
    char                   filename[256];
    char                   filepath[512];
    UT_array*              chunks;     /* WssChunk* array            */
    UT_array*              events;    /* WssEventEntry* array       */
    struct WssScriptFile*  next;
    int                    load_order;
    time_t                 mtime;
} WssScriptFile;

/* ── Event binding ─────────────────────────────────────────────── */
typedef enum {
    EVT_PLAYER_JOIN,
    EVT_PLAYER_LEAVE,
    EVT_PLAYER_CHAT,
    EVT_PLAYER_KILL,
    EVT_PLAYER_LEVELUP,
    EVT_GOSSIP_HELLO,
    EVT_GOSSIP_SELECT,
    EVT_STARTUP,
    EVT_TIMER_600S,
    EVT_TOTAL
} WssEventId;

typedef struct {
    WssEventId    event;
    const char*   handler_name;   /* e.g. "on_event"             */
    int           chunk_offset;   /* first bytecode chunk for this */
    int           line;
} WssEventEntry;

/* ── Script database ─────────────────────────────────────────────── */
typedef struct WssScriptDB {
    WssScriptFile* files;
    WssScriptFile* last_loaded;
    int            file_count;
    int            next_order;
} WssScriptDB;

void        WssScriptDB_Init(WssScriptDB* db);
void        WssScriptDB_Delete(WssScriptDB* db);
const char* WssScriptDB_RegisterScript(WssScriptDB* db, const char* filename,
                                        const char* filepath, UT_array* chunks);
const char* WssScriptDB_AddEvent(WssScriptDB* db, const char* filename,
                                  WssEventId event, const char* handler,
                                  int chunk_offset, int line);
WssScriptFile* WssScriptDB_FindScript(WssScriptDB* db, const char* filename);
WssEventEntry* WssScriptDB_FindEventHandler(WssScriptDB* db, WssEventId event);

#endif /* WSS_SCRIPTDB_H */
