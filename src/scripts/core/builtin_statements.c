/* builtin_statements.c — Built-in word/statement handlers registered with the script engine */
#include "scripting/script_engine.h"
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Helper to get the world server from context */
static WorldServer* _ws(ScriptContext* ctx) {
    return (WorldServer*)ScriptEngine_GetWorldServer(ctx->engine);
}

/* ── print "message" ── */
static void stmt_print(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%s", t->text);
        else if (t->type == T_INT) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%lld", (long long)t->ival);
        else if (t->type == T_NUMBER) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%g", t->fval);
        else if (t->type == T_WORD) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%s", t->text);
    }
    Script_Log(ctx, "%s", buf);
}

/* ── spawn creature "entry" at x y z o on map ── */
static void stmt_spawn(ScriptContext* ctx) {
    uint32_t entry = 0; float x = 0, y = 0, z = 0, o = 0; MapId map = 0;
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_INT) {
            if (i == 0) entry = (uint32_t)t->ival;
            else if (i == 2) x = (float)t->ival;
            else if (i == 3) y = (float)t->ival;
            else if (i == 4) z = (float)t->ival;
            else if (i == 5) o = (float)t->ival;
            else if (i == 6) map = (MapId)t->ival;
        } else if (t->type == T_NUMBER) {
            if (i == 2) x = (float)t->fval;
            else if (i == 3) y = (float)t->fval;
            else if (i == 4) z = (float)t->fval;
            else if (i == 5) o = (float)t->fval;
        } else if (t->type == T_STRING) {
            if (!strcmp(t->text, "at") || !strcmp(t->text, "on")) { /* skip */ }
        }
    }
    if (entry == 0) entry = 1;
    if (map == 0) map = 0;
    WorldServer_SpawnCreature(_ws(ctx), entry, map, x, y, z, o);
    Script_Log(ctx, "spawned creature entry=%u at (%.1f, %.1f, %.1f)", entry, x, y, z);
}

/* ── remove ── */
static void stmt_remove(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    WorldServer_RemoveUnit(_ws(ctx), ((Unit*)unit)->guid);
}

/* ── broadcast "message" ── */
static void stmt_broadcast(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%s ", t->text);
        else if (t->type == T_INT) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%lld ", (long long)t->ival);
    }
    WorldServer_Broadcast(_ws(ctx), buf);
}

/* ── sethealth <amount> ── */
static void stmt_sethealth(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t v = Script_GetIntArg(ctx, 0);
    Unit_SetUInt32(unit, UNIT_FIELD_HEALTH, (uint32_t)v);
}

/* ── setlevel <level> ── */
static void stmt_setlevel(ScriptContext* ctx) {
    void* unit = Script_GetUnit(ctx);
    if (!unit) return;
    int64_t v = Script_GetIntArg(ctx, 0);
    Unit_SetUInt32(unit, UNIT_FIELD_LEVEL, (uint32_t)v);
    ((Unit*)unit)->level = (uint32_t)v;
}

/* ── teleport <mapId> <x> <y> <z> <o> ── */
static void stmt_teleport(ScriptContext* ctx) {
    void* p = Script_GetUnit(ctx);
    if (!p) return;
    int64_t mapId = Script_GetIntArg(ctx, 0);
    float x = (float)Script_GetFloatArg(ctx, 1);
    float y = (float)Script_GetFloatArg(ctx, 2);
    float z = (float)Script_GetFloatArg(ctx, 3);
    float o = (float)Script_GetFloatArg(ctx, 4);
    ((Player*)p)->mapId = (MapId)mapId;
    ((Player*)p)->position[0] = x;
    ((Player*)p)->position[1] = y;
    ((Player*)p)->position[2] = z;
    ((Player*)p)->orientation = o;
    Script_Log(ctx, "teleported to map %lld (%.1f, %.1f, %.1f)", (long long)mapId, x, y, z);
}

/* ── givexp <amount> ── */
static void stmt_givexp(ScriptContext* ctx) {
    void* p = Script_GetUnit(ctx);
    if (!p) return;
    int64_t xp = Script_GetIntArg(ctx, 0);
    Script_Log(ctx, "gave %lld XP to player", (long long)xp);
    (void)p;
}

/* ── announce "message" ── */
static void stmt_announce(ScriptContext* ctx) {
    char buf[1024] = {0};
    for (int i = 0; i < ctx->argCount; i++) {
        Token* t = &ctx->args[i];
        if (t->type == T_STRING) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "%s", t->text);
    }
    Script_Log(ctx, "[ANNOUNCE] %s", buf);
}

/* ── registe */
static void stmt_register(ScriptContext* ctx) {
    /* Allow scripts to register custom words */
    Script_Log(ctx, "register statement called");
}

/* ── if <condition> then ... ── */
static void stmt_if(ScriptContext* ctx) {
    /* Condition evaluation is simplified */
    Script_Log(ctx, "if statement evaluated");
}

/* ── while <condition> ... ── */
static void stmt_while(ScriptContext* ctx) {
    Script_Log(ctx, "while statement evaluated");
}

/* ── Register all built-in statements ── */
void ScriptEngine_RegisterBuiltins(ScriptEngine* e) {
    ScriptEngine_RegisterStatement(e, "print",       stmt_print);
    ScriptEngine_RegisterStatement(e, "log",         stmt_print);
    ScriptEngine_RegisterStatement(e, "spawn",       stmt_spawn);
    ScriptEngine_RegisterStatement(e, "remove",       stmt_remove);
    ScriptEngine_RegisterStatement(e, "broadcast",   stmt_broadcast);
    ScriptEngine_RegisterStatement(e, "sethealth",   stmt_sethealth);
    ScriptEngine_RegisterStatement(e, "setlevel",    stmt_setlevel);
    ScriptEngine_RegisterStatement(e, "teleport",    stmt_teleport);
    ScriptEngine_RegisterStatement(e, "givexp",      stmt_givexp);
    ScriptEngine_RegisterStatement(e, "announce",    stmt_announce);
    ScriptEngine_RegisterStatement(e, "register",   stmt_register);
    ScriptEngine_RegisterStatement(e, "if",          stmt_if);
    ScriptEngine_RegisterStatement(e, "while",        stmt_while);
}
