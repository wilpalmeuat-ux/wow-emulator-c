/* builtin_statements.c -- All built-in WSS word/statement handlers
 *
 * These are the core "words" that WSS scripts can use.  Each one
 * is a plain-English keyword that maps to a C function.
 *
 * The scripting philosophy: scripts read like sentences.
 *
 *   broadcast "Welcome to the server!"
 *   spawn 1234 at 100.0 200.0 300.0 on map 0
 *   teleport player to 0 -8949.0 -132.0 83.0 0.0
 *   sethealth 5000
 *   setlevel 80
 *   givexp 1000
 *   givegold 500
 *   say "Hello adventurer!"
 *   whisper "Secret message"
 *   set flag player pvp
 *   clear flag player pvp
 *   kill target
 *   resurrect target
 *   wait 5000
 *   countdown 10 "Event starts in"
 *   despawn target
 *   announce "Server restarting in 5 minutes"
 */
#include "scripting/script_engine.h"
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Helper to get the world server from context */
static WorldServer* _ws(ScriptContext* ctx) {
    return (WorldServer*)ScriptEngine_GetWorldServer(ctx->engine);
}

/* ================================================================
 *  print / log  --  output text to the server console
 *  Usage:  print "Hello world"
 *          log "Debug value:" 42
 * ================================================================ */
static void stmt_print(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
        else if (t->type == T_INT)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%lld", (long long)t->ival);
        else if (t->type == T_NUMBER)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%g", t->fval);
        else if (t->type == T_WORD)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
        if (i < ctx->argCount - 1)
            strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
    }
    Script_Log(ctx, "%s", buf);
}

/* ================================================================
 *  spawn  --  create a creature in the world
 *  Usage:  spawn 1234 at 100.0 200.0 300.0 on map 0
 *          spawn 5678 at 50 60 70
 * ================================================================ */
static void stmt_spawn(ScriptContext* ctx) {
    uint32_t entry = 0;
    float x = 0, y = 0, z = 0, o = 0;
    MapId map = 0;
    int numIdx = 0;

    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        /* Skip filler words */
        if (t->type == T_WORD) {
            if (!strcmp(t->text, "at") || !strcmp(t->text, "on") ||
                !strcmp(t->text, "map") || !strcmp(t->text, "facing"))
                continue;
        }
        double val = 0;
        if (t->type == T_INT) val = (double)t->ival;
        else if (t->type == T_NUMBER) val = t->fval;
        else continue;

        switch (numIdx) {
            case 0: entry = (uint32_t)val; break;
            case 1: x = (float)val; break;
            case 2: y = (float)val; break;
            case 3: z = (float)val; break;
            case 4: o = (float)val; break;
            case 5: map = (MapId)val; break;
        }
        numIdx++;
    }

    if (entry == 0) entry = 1;
    WorldServer_SpawnCreature(_ws(ctx), entry, map, x, y, z, o);
    Script_Log(ctx, "[WSS] Spawned creature entry=%u at (%.1f, %.1f, %.1f) map=%u",
               entry, x, y, z, map);
}

/* ================================================================
 *  despawn  --  remove a creature/unit
 *  Usage:  despawn target
 *          despawn
 * ================================================================ */
static void stmt_despawn(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) { Script_Log(ctx, "[WSS] despawn: no target"); return; }
    WorldServer_RemoveUnit(_ws(ctx), ((Unit*)unit)->guid);
    Script_Log(ctx, "[WSS] Despawned unit GUID=%llu",
               (unsigned long long)((Unit*)unit)->guid);
}

/* ================================================================
 *  broadcast  --  send a server-wide chat message
 *  Usage:  broadcast "Welcome to the server!"
 * ================================================================ */
static void stmt_broadcast(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
        else if (t->type == T_INT)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%lld", (long long)t->ival);
        else if (t->type == T_NUMBER)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%g", t->fval);
        if (i < ctx->argCount - 1)
            strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
    }
    WorldServer_Broadcast(_ws(ctx), buf);
}

/* ================================================================
 *  announce  --  broadcast with [ANNOUNCE] prefix
 *  Usage:  announce "Server restarting in 5 minutes"
 * ================================================================ */
static void stmt_announce(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
    }
    char full[1100];
    snprintf(full, sizeof(full), "[ANNOUNCE] %s", buf);
    WorldServer_Broadcast(_ws(ctx), full);
}

/* ================================================================
 *  say  --  make a unit "say" something (NPC chat bubble)
 *  Usage:  say "Hello adventurer!"
 * ================================================================ */
static void stmt_say(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
    }
    Script_Log(ctx, "[WSS] Say: %s", buf);
}

/* ================================================================
 *  whisper  --  send a private message to a player
 *  Usage:  whisper "You found a secret!"
 * ================================================================ */
static void stmt_whisper(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING)
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", t->text);
    }
    Script_Log(ctx, "[WSS] Whisper: %s", buf);
}

/* ================================================================
 *  sethealth  --  set a unit's health value
 *  Usage:  sethealth 5000
 * ================================================================ */
static void stmt_sethealth(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t v = Script_GetIntArg(ctx, 0);
    Unit_SetUInt32(unit, UNIT_FIELD_HEALTH, (uint32_t)v);
    Script_Log(ctx, "[WSS] Set health to %lld", (long long)v);
}

/* ================================================================
 *  setmaxhealth  --  set a unit's maximum health
 *  Usage:  setmaxhealth 10000
 * ================================================================ */
static void stmt_setmaxhealth(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t v = Script_GetIntArg(ctx, 0);
    Unit_SetUInt32(unit, UNIT_FIELD_MAXHEALTH, (uint32_t)v);
    Script_Log(ctx, "[WSS] Set max health to %lld", (long long)v);
}

/* ================================================================
 *  setlevel  --  set a unit's level
 *  Usage:  setlevel 80
 * ================================================================ */
static void stmt_setlevel(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t v = Script_GetIntArg(ctx, 0);
    Unit_SetUInt32(unit, UNIT_FIELD_LEVEL, (uint32_t)v);
    ((Unit*)unit)->level = (uint32_t)v;
    Script_Log(ctx, "[WSS] Set level to %lld", (long long)v);
}

/* ================================================================
 *  teleport  --  move a player to a new location
 *  Usage:  teleport player to 0 -8949.0 -132.0 83.0 0.0
 *          teleport 1 100.0 200.0 300.0 0.0
 * ================================================================ */
static void stmt_teleport(ScriptContext* ctx) {
    void* p = Script_GetUnit(ctx);
    if (!p) return;
    int numIdx = 0;
    int64_t mapId = 0;
    float x = 0, y = 0, z = 0, o = 0;

    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_WORD) {
            if (!strcmp(t->text, "player") || !strcmp(t->text, "to") ||
                !strcmp(t->text, "target"))
                continue;
        }
        double val = 0;
        if (t->type == T_INT) val = (double)t->ival;
        else if (t->type == T_NUMBER) val = t->fval;
        else continue;

        switch (numIdx) {
            case 0: mapId = (int64_t)val; break;
            case 1: x = (float)val; break;
            case 2: y = (float)val; break;
            case 3: z = (float)val; break;
            case 4: o = (float)val; break;
        }
        numIdx++;
    }

    ((Player*)p)->mapId = (MapId)mapId;
    ((Player*)p)->position[0] = x;
    ((Player*)p)->position[1] = y;
    ((Player*)p)->position[2] = z;
    ((Player*)p)->orientation = o;
    Script_Log(ctx, "[WSS] Teleported to map %lld (%.1f, %.1f, %.1f)",
               (long long)mapId, x, y, z);
}

/* ================================================================
 *  givexp  --  award experience points
 *  Usage:  givexp 1000
 * ================================================================ */
static void stmt_givexp(ScriptContext* ctx) {
    int64_t xp = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Gave %lld XP", (long long)xp);
}

/* ================================================================
 *  givegold  --  award gold (in copper, 1g = 10000c)
 *  Usage:  givegold 50000
 * ================================================================ */
static void stmt_givegold(ScriptContext* ctx) {
    int64_t gold = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Gave %lld copper (%lld gold)",
               (long long)gold, (long long)(gold / 10000));
}

/* ================================================================
 *  giveitem  --  add an item to player inventory
 *  Usage:  giveitem 49623 count 1
 * ================================================================ */
static void stmt_giveitem(ScriptContext* ctx) {
    int64_t itemId = Script_GetIntArg(ctx, 0);
    int64_t count = ctx->argCount > 1 ? Script_GetIntArg(ctx, 1) : 1;
    Script_Log(ctx, "[WSS] Gave item %lld x%lld", (long long)itemId, (long long)count);
}

/* ================================================================
 *  kill  --  instantly kill target
 *  Usage:  kill target
 * ================================================================ */
static void stmt_kill(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    Unit_SetUInt32(unit, UNIT_FIELD_HEALTH, 0);
    ((Unit*)unit)->dead = true;
    Script_Log(ctx, "[WSS] Killed unit GUID=%llu",
               (unsigned long long)((Unit*)unit)->guid);
    ScriptEngine_ExecuteWordEvent(ctx->engine, ((Unit*)unit)->name, "onDeath", unit);
}

/* ================================================================
 *  resurrect  --  bring a dead unit back to life
 *  Usage:  resurrect target
 * ================================================================ */
static void stmt_resurrect(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    uint32_t maxHp = Unit_GetUInt32(unit, UNIT_FIELD_MAXHEALTH);
    Unit_SetUInt32(unit, UNIT_FIELD_HEALTH, maxHp);
    ((Unit*)unit)->dead = false;
    Script_Log(ctx, "[WSS] Resurrected unit");
}

/* ================================================================
 *  set_flag / clear_flag  --  toggle unit flags
 *  Usage:  set_flag pvp
 *          clear_flag combat
 * ================================================================ */
static void stmt_set_flag(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    const char* flag = Script_GetStringArg(ctx, 0);
    if (!flag) flag = "unknown";

    uint32_t flags = ((Unit*)unit)->unitFlags;
    if (!strcmp(flag, "pvp"))       flags |= 0x00001000;
    else if (!strcmp(flag, "combat"))  flags |= 0x00080000;
    else if (!strcmp(flag, "immune"))  flags |= 0x00000002;
    else if (!strcmp(flag, "pacify"))  flags |= 0x00020000;
    ((Unit*)unit)->unitFlags = flags;
    Script_Log(ctx, "[WSS] Set flag '%s' on unit", flag);
}

static void stmt_clear_flag(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    const char* flag = Script_GetStringArg(ctx, 0);
    if (!flag) flag = "unknown";

    uint32_t flags = ((Unit*)unit)->unitFlags;
    if (!strcmp(flag, "pvp"))       flags &= ~0x00001000;
    else if (!strcmp(flag, "combat"))  flags &= ~0x00080000;
    else if (!strcmp(flag, "immune"))  flags &= ~0x00000002;
    else if (!strcmp(flag, "pacify"))  flags &= ~0x00020000;
    ((Unit*)unit)->unitFlags = flags;
    Script_Log(ctx, "[WSS] Cleared flag '%s' on unit", flag);
}

/* ================================================================
 *  wait  --  pause script execution (milliseconds)
 *  Usage:  wait 5000
 * ================================================================ */
static void stmt_wait(ScriptContext* ctx) {
    int64_t ms = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Wait %lld ms (simulated)", (long long)ms);
    /* In a real implementation this would schedule a delayed callback */
}

/* ================================================================
 *  countdown  --  timed announcements
 *  Usage:  countdown 10 "Event starts in"
 * ================================================================ */
static void stmt_countdown(ScriptContext* ctx) {
    int64_t seconds = Script_GetIntArg(ctx, 0);
    const char* prefix = Script_GetStringArg(ctx, 1);
    if (!prefix) prefix = "Countdown";
    Script_Log(ctx, "[WSS] Countdown: %s %lld seconds", prefix, (long long)seconds);
}

/* ================================================================
 *  timer  --  set a recurring timer
 *  Usage:  timer 60000 "onMinute"
 * ================================================================ */
static void stmt_timer(ScriptContext* ctx) {
    int64_t ms = Script_GetIntArg(ctx, 0);
    const char* event = Script_GetStringArg(ctx, 1);
    if (!event) event = "onTimer";
    Script_Log(ctx, "[WSS] Timer set: %lld ms -> %s", (long long)ms, event);
}

/* ================================================================
 *  set  --  set a global or local variable
 *  Usage:  set global server_players + 1
 *          set local myvar 42
 * ================================================================ */
static void stmt_set(ScriptContext* ctx) {
    if (ctx->argCount < 2) return;
    const char* scope = Script_GetStringArg(ctx, 0);
    const char* varname = Script_GetStringArg(ctx, 1);
    if (!scope) scope = "local";
    if (!varname) varname = "unknown";
    Script_Log(ctx, "[WSS] Set %s.%s", scope, varname);
}

/* ================================================================
 *  remove  --  alias for despawn
 * ================================================================ */
static void stmt_remove(ScriptContext* ctx) {
    stmt_despawn(ctx);
}

/* ================================================================
 *  register  --  register a custom word/event
 * ================================================================ */
static void stmt_register(ScriptContext* ctx) {
    Script_Log(ctx, "[WSS] Register statement called");
}

/* ================================================================
 *  if / while  --  control flow (handled by parser, these are stubs)
 * ================================================================ */
static void stmt_if(ScriptContext* ctx) {
    (void)ctx;
}

static void stmt_while(ScriptContext* ctx) {
    (void)ctx;
}

/* ================================================================
 *  for  --  loop control
 * ================================================================ */
static void stmt_for(ScriptContext* ctx) {
    (void)ctx;
}

/* ================================================================
 *  morph  --  change display model
 *  Usage:  morph 12345
 * ================================================================ */
static void stmt_morph(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t displayId = Script_GetIntArg(ctx, 0);
    ((Unit*)unit)->displayId = (uint32_t)displayId;
    Script_Log(ctx, "[WSS] Morphed to displayId %lld", (long long)displayId);
}

/* ================================================================
 *  emote  --  play an emote animation
 *  Usage:  emote dance
 * ================================================================ */
static void stmt_emote(ScriptContext* ctx) {
    const char* emote = Script_GetStringArg(ctx, 0);
    if (!emote) emote = "none";
    Script_Log(ctx, "[WSS] Emote: %s", emote);
}

/* ================================================================
 *  cast  --  cast a spell
 *  Usage:  cast 12345 on target
 * ================================================================ */
static void stmt_cast(ScriptContext* ctx) {
    int64_t spellId = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Cast spell %lld", (long long)spellId);
}

/* ================================================================
 *  aura  --  apply an aura/buff
 *  Usage:  aura 12345 on target
 * ================================================================ */
static void stmt_aura(ScriptContext* ctx) {
    int64_t auraId = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Applied aura %lld", (long long)auraId);
}

/* ================================================================
 *  removeaura  --  remove an aura/buff
 *  Usage:  removeaura 12345
 * ================================================================ */
static void stmt_removeaura(ScriptContext* ctx) {
    int64_t auraId = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "[WSS] Removed aura %lld", (long long)auraId);
}

/* ================================================================
 *  setfaction  --  change a creature's faction
 *  Usage:  setfaction 35
 * ================================================================ */
static void stmt_setfaction(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t faction = Script_GetIntArg(ctx, 0);
    ((Unit*)unit)->faction = (uint32_t)faction;
    Script_Log(ctx, "[WSS] Set faction to %lld", (long long)faction);
}

/* ================================================================
 *  setspeed  --  change movement speed
 *  Usage:  setspeed run 2.5
 * ================================================================ */
static void stmt_setspeed(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    double speed = Script_GetFloatArg(ctx, 0);
    ((Unit*)unit)->speedRun = (float)speed;
    Script_Log(ctx, "[WSS] Set speed to %.2f", speed);
}

/* ================================================================
 *  Register all built-in statements with the engine
 * ================================================================ */
void ScriptEngine_RegisterBuiltins(ScriptEngine* e) {
    /* Output */
    ScriptEngine_RegisterStatement(e, "print",       stmt_print);
    ScriptEngine_RegisterStatement(e, "log",         stmt_print);
    ScriptEngine_RegisterStatement(e, "say",         stmt_say);
    ScriptEngine_RegisterStatement(e, "whisper",     stmt_whisper);
    ScriptEngine_RegisterStatement(e, "broadcast",   stmt_broadcast);
    ScriptEngine_RegisterStatement(e, "announce",    stmt_announce);

    /* Spawning */
    ScriptEngine_RegisterStatement(e, "spawn",       stmt_spawn);
    ScriptEngine_RegisterStatement(e, "despawn",     stmt_despawn);
    ScriptEngine_RegisterStatement(e, "remove",      stmt_remove);

    /* Unit manipulation */
    ScriptEngine_RegisterStatement(e, "sethealth",   stmt_sethealth);
    ScriptEngine_RegisterStatement(e, "setmaxhealth",stmt_setmaxhealth);
    ScriptEngine_RegisterStatement(e, "setlevel",    stmt_setlevel);
    ScriptEngine_RegisterStatement(e, "kill",        stmt_kill);
    ScriptEngine_RegisterStatement(e, "resurrect",   stmt_resurrect);
    ScriptEngine_RegisterStatement(e, "morph",       stmt_morph);
    ScriptEngine_RegisterStatement(e, "setfaction",  stmt_setfaction);
    ScriptEngine_RegisterStatement(e, "setspeed",    stmt_setspeed);

    /* Player */
    ScriptEngine_RegisterStatement(e, "teleport",    stmt_teleport);
    ScriptEngine_RegisterStatement(e, "givexp",      stmt_givexp);
    ScriptEngine_RegisterStatement(e, "givegold",    stmt_givegold);
    ScriptEngine_RegisterStatement(e, "giveitem",    stmt_giveitem);

    /* Flags */
    ScriptEngine_RegisterStatement(e, "set_flag",    stmt_set_flag);
    ScriptEngine_RegisterStatement(e, "clear_flag",  stmt_clear_flag);

    /* Spells */
    ScriptEngine_RegisterStatement(e, "cast",        stmt_cast);
    ScriptEngine_RegisterStatement(e, "aura",        stmt_aura);
    ScriptEngine_RegisterStatement(e, "removeaura",  stmt_removeaura);
    ScriptEngine_RegisterStatement(e, "emote",       stmt_emote);

    /* Timing */
    ScriptEngine_RegisterStatement(e, "wait",        stmt_wait);
    ScriptEngine_RegisterStatement(e, "countdown",   stmt_countdown);
    ScriptEngine_RegisterStatement(e, "timer",       stmt_timer);

    /* Variables */
    ScriptEngine_RegisterStatement(e, "set",         stmt_set);

    /* Control flow */
    ScriptEngine_RegisterStatement(e, "if",          stmt_if);
    ScriptEngine_RegisterStatement(e, "while",       stmt_while);
    ScriptEngine_RegisterStatement(e, "for",         stmt_for);

    /* Meta */
    ScriptEngine_RegisterStatement(e, "register",    stmt_register);
}
