/* ╔══════════════════════════════════════════════════════════════╗
   ║  WSS Scripting System — WoW event hooks integration         ║
   ╚══════════════════════════════════════════════════════════════╝
   WSS = WoW Scripting System
   
   This is the main header for the scripting engine that drives the
   entire emulator. Scripts are plain-text files with C-like syntax.
   The scripting system is hot-reloaded and event-driven.
   
   Script example:
   
     var max_players = 100
     var world_name = "My WoW Server"
     
     func on_player_login(player) {
         print("Player " + player.name + " has joined " + world_name)
         return null
     }
     
     func on_gossip_hello(player, npc) {
         if npc.has_flag("vendor") {
             print("Vendor gossip from " + npc.name)
         }
         return null
     }
     
     func on_unit_spawn(unit) {
         if unit.type == "npc" and unit.level > 50 {
             print("Boss spawned: " + unit.name)
         }
         return null
     }
*/

#ifndef WSS_WOW_HOOKS_H
#define WSS_WOW_HOOKS_H

#include <scripting/wss_vm.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_compiler.h>
#include <scripting/wss_objectstore.h>

#define WSS_MAX_SCRIPTS 256
#define WSS_MAX_HOOKS 32

/* WoW event hook types — each maps to a plain-text script function */
typedef enum {
    HOOK_ON_INIT,              /* called when world server starts */
    HOOK_ON_UPDATE,            /* called every world tick (10Hz) */
    HOOK_ON_SPAWN,             /* called when a unit spawns in the world */
    HOOK_ON_INTERACT,          /* called when player interacts with object/NPC */
    HOOK_ON_QUEST_ACCEPT,      /* called when player accepts a quest */
    HOOK_ON_QUEST_COMPLETE,    /* called when player completes a quest */
    HOOK_ON_COMBAT_START,      /* called when combat begins */
    HOOK_ON_COMBAT_END,        /* called when combat ends */
    HOOK_ON_DEATH,             /* called when a unit dies */
    HOOK_ON_RESURRECT,         /* called when a unit is resurrected */
    HOOK_ON_GOSSIP_HELLO,      /* called when player opens gossip menu */
    HOOK_ON_GOSSIP_SELECT,     /* called when player selects gossip option */
    HOOK_ON_NPC_SAY,           /* called when NPC says something */
    HOOK_ON_PLAYER_LOGIN,      /* called when player logs into world */
    HOOK_ON_PLAYER_LOGOUT,     /* called when player logs out */
    HOOK_ON_CHAT_MESSAGE,      /* called on every chat message */
    HOOK_ON_CAST_START,        /* called when a spell cast begins */
    HOOK_ON_CAST_COMPLETE,     /* called when a spell cast succeeds */
    HOOK_ON_ITEM_USE,          /* called when player uses an item */
    HOOK_COUNT
} WssHookType;

typedef struct {
    WssObjectStore  scripts;       /* name → source string */
    WssObjectStore  chunks;        /* name → compiled WssChunk* bytecode */
    WssObjectStore  constants;    /* shared constants pool */
    WssObjectStore  registry;     /* global variable store */
    WSSState        vm;           /* shared VM for all scripts */
    char            last_error[512];
    int             tick_count;
} WssScriptingSystem;

/* Initialize / destroy the scripting system */
void WssSystem_Init(WssScriptingSystem* sys, WssObjectStore* registry);
void WssSystem_Delete(WssScriptingSystem* sys);

/* Load a named script from source text */
bool WssSystem_LoadScript(WssScriptingSystem* sys, const char* name, const char* source);

/* Run a standalone script by name */
bool WssSystem_RunScript(WssScriptingSystem* sys, const char* name);

/* Fire a hook — calls all scripts that define the hook function */
bool WssSystem_RunHook(WssScriptingSystem* sys, WssHookType hook, int nargs, WssValue* args);

/* Evaluate a one-liner command string (REPL-style) */
bool WssSystem_RunCommand(WssScriptingSystem* sys, const char* cmd);

/* Returns the last error string */
const char* WssSystem_GetError(WssScriptingSystem* sys);

/* Get hook name as string */
const char* WssHook_Name(WssHookType h);

#endif
