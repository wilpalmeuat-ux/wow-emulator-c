/* ╔══════════════════════════════════════════════════════════════╗
   ║  WoW 3.3.5 Emulator - Custom Statement Scripting Engine     ║
   ║  Script files (*.wss) use plain-English word statements      ║
   ╚══════════════════════════════════════════════════════════════╝ */

#ifndef WSS_ENGINE_H
#define WSS_ENGINE_H

#include "../shared/Types.h"

/* ── Script handle (opaque) ───────────────────────────────────── */
typedef struct WssEngine WssEngine;

/* ── Engine lifecycle ─────────────────────────────────────────── */
WssEngine* WssEngine_Create(void);
void       WssEngine_Destroy(WssEngine* e);

/* Load & reload a .wss script file.
   path is relative to the scripts/ directory.               */
bool  WssEngine_LoadScript(WssEngine* e, const char* path);
void  WssEngine_UnloadAll(WssEngine* e);

/* ── Event dispatch ──────────────────────────────────────────────
   Call these from the worldserver core whenever an event fires. */
void WssEngine_OnPlayerJoin(WssEngine* e, uint64 guid, const char* name);
void WssEngine_OnPlayerLeave(WssEngine* e, uint64 guid);
void WssEngine_OnPlayerChat(WssEngine* e, uint64 guid, const char* msg, int channel);
void WssEngine_OnPlayerKillUnit(WssEngine* e, uint64 guid, uint64 victim_guid);
void WssEngine_OnPlayerLevelUp(WssEngine* e, uint64 guid, uint32 new_level);
void WssEngine_OnGossipHello(WssEngine* e, uint64 guid, uint64 npc_guid);
void WssEngine_OnGossipSelect(WssEngine* e, uint64 guid, uint64 npc_guid, uint32 menu_id, uint32 option_id);
void WssEngine_OnStartup(WssEngine* e);
void WssEngine_OnTimer600s(WssEngine* e);        /* every 600 s / 10 min heartbeat */

/* ── Script globals (key=value pairs persist for session) ─────── */
void  WssEngine_SetGlobal(WssEngine* e, const char* key, int64 val);
int64 WssEngine_GetGlobal(WssEngine* e, const char* key);
void  WssEngine_SetGlobalStr(WssEngine* e, const char* key, const char* val);
const char* WssEngine_GetGlobalStr(WssEngine* e, const char* key);

/* ── Console commands exposed to .wss scripts ────────────────────
   The worldserver registers these at startup.                  */
typedef void (*WssConsoleCmdFn)(uint64 player_guid, const char* args);
void WssEngine_RegisterCommand(WssEngine* e, const char* name, WssConsoleCmdFn fn);

/* ── Error log ─────────────────────────────────────────────────── */
const char* WssEngine_LastError(WssEngine* e);

#endif /* WSS_ENGINE_H */
