/* wss_bindings.c — WoW WSS bindings (Windows)
 * Sleep() instead of usleep(), no POSIX headers.
 */
#include "scripting/wss_bindings.h"
#include "scripting/wss_vm.h"
#include "worldserver/WorldPlayer.h"
#include "shared/log.h"
#include <string.h>
#include <stdlib.h>
#include <windows.h>

// PLAYER BINDINGS
static WSSValue _fn_player_create(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<3) return wss_mk_nil();
    const char* name = wss_to_str(args[0]);
    int64_t race = wss_to_int(args[1]);
    int64_t class_ = wss_to_int(args[2]);
    WorldPlayer* p = player_create(0, name, race, class_);
    WSSValue r; r.type=WSS_INT; r.i=(int64_t)(intptr_t)p; return r;
}

static WSSValue _fn_player_teleport(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<5) return wss_mk_nil();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    uint32_t map = (uint32_t)wss_to_int(args[1]);
    float x = (float)wss_to_float(args[2]);
    float y = (float)wss_to_float(args[3]);
    float z = (float)wss_to_float(args[4]);
    float o = narg>5 ? (float)wss_to_float(args[5]) : 0.0f;
    WorldPlayer* p = player_by_guid(guid);
    if(p) player_teleport(p, map, x, y, z, o);
    else LOG_WARN("player_teleport: player guid=%llu not found", (unsigned long long)guid);
    return wss_mk_nil();
}

static WSSValue _fn_player_send_message(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<2) return wss_mk_nil();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    const char* msg = wss_to_str(args[1]);
    WorldPlayer* p = player_by_guid(guid);
    if(p) player_send_message(p, msg);
    else LOG_WARN("player_send_message: player guid=%llu not found", (unsigned long long)guid);
    return wss_mk_nil();
}

// CREATURE BINDINGS
static WSSValue _fn_creature_spawn(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<7) return wss_mk_nil();
    const char* name = wss_to_str(args[0]);
    uint32_t entry = (uint32_t)wss_to_int(args[1]);
    uint32_t map = (uint32_t)wss_to_int(args[2]);
    float x = (float)wss_to_float(args[3]);
    float y = (float)wss_to_float(args[4]);
    float z = (float)wss_to_float(args[5]);
    float o = (float)wss_to_float(args[6]);
    uint64_t guid = creature_spawn(name, entry, map, x, y, z, o);
    WSSValue r; r.type=WSS_INT; r.i=(int64_t)guid; return r;
}

static WSSValue _fn_creature_despawn(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<1) return wss_mk_nil();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    creature_despawn(guid);
    return wss_mk_nil();
}

static WSSValue _fn_creature_move_to(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<4) return wss_mk_nil();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    float x = (float)wss_to_float(args[1]);
    float y = (float)wss_to_float(args[2]);
    float z = (float)wss_to_float(args[3]);
    creature_move_to(guid, x, y, z);
    return wss_mk_nil();
}

static WSSValue _fn_creature_cast(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<2) return wss_mk_nil();
    uint64_t caster_guid = (uint64_t)wss_to_int(args[0]);
    uint32_t spell_id = (uint32_t)wss_to_int(args[1]);
    creature_cast_spell(caster_guid, spell_id);
    return wss_mk_nil();
}

// UTILITY BINDINGS
static WSSValue _fn_print(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    for(int i=0;i<narg;i++) {
        if(args[i].type==WSS_INT) printf("%lld", (long long)args[i].i);
        else if(args[i].type==WSS_FLOAT) printf("%f", args[i].f);
        else if(args[i].type==WSS_STRING || args[i].type==WSS_STRING_LITERAL) printf("%s", args[i].str ? args[i].str : "");
        else if(args[i].type==WSS_NIL) printf("nil");
    }
    printf("\n");
    return wss_mk_nil();
}

static WSSValue _fn_sleep(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    if(narg<1) return wss_mk_nil();
    int64_t ms = wss_to_int(args[0]);
    if(ms > 0) Sleep((DWORD)ms);
    return wss_mk_nil();
}

static WSSValue _fn_rand(WSSState* S, WSSValue* args, int narg) {
    (void)S;
    int64_t max = narg>0 ? wss_to_int(args[0]) : 100;
    int r = rand() % (int)max;
    WSSValue ret; ret.type=WSS_INT; ret.i=r; return ret;
}

static WSSValue _fn_get_time(WSSState* S, WSSValue* args, int narg) {
    (void)S; (void)args; (void)narg;
    time_t t = time(NULL);
    WSSValue ret; ret.type=WSS_INT; ret.i=(int64_t)t; return ret;
}

// Register all built-in WSS bindings
void WssBindings_Register(WSSState* S) {
    wss_register_fn(S, "player.create", _fn_player_create);
    wss_register_fn(S, "player.teleport", _fn_player_teleport);
    wss_register_fn(S, "player.send_message", _fn_player_send_message);
    wss_register_fn(S, "creature.spawn", _fn_creature_spawn);
    wss_register_fn(S, "creature.despawn", _fn_creature_despawn);
    wss_register_fn(S, "creature.move_to", _fn_creature_move_to);
    wss_register_fn(S, "creature.cast", _fn_creature_cast);
    wss_register_fn(S, "print", _fn_print);
    wss_register_fn(S, "sleep", _fn_sleep);
    wss_register_fn(S, "rand", _fn_rand);
    wss_register_fn(S, "get_time", _fn_get_time);
}