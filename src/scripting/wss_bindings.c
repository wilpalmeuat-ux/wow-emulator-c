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

/* Access WssValue union fields via the data. prefix (as defined in wss_value.h) */
static WssValue _fn_player_create(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 3) return WssValue_Null();
    const char* name = wss_to_str(args[0]);
    int64_t race = wss_to_int(args[1]);
    int64_t class_ = wss_to_int(args[2]);
    WorldPlayer* p = player_create(0, name, race, class_);
    WssValue r;
    r.type = VAL_INT;
    r.data.as_int = (int64_t)(intptr_t)p;
    return r;
}

static WssValue _fn_player_teleport(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 5) return WssValue_Null();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    uint32_t map = (uint32_t)wss_to_int(args[1]);
    float x = (float)wss_to_float(args[2]);
    float y = (float)wss_to_float(args[3]);
    float z = (float)wss_to_float(args[4]);
    float o = narg > 5 ? (float)wss_to_float(args[5]) : 0.0f;
    WorldPlayer* p = player_by_guid(guid);
    if (p) player_teleport(p, map, x, y, z, o);
    else LOG_WARN("player_teleport: player guid=%llu not found", (unsigned long long)guid);
    return WssValue_Null();
}

static WssValue _fn_player_send_message(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 2) return WssValue_Null();
    uint64_t guid = (uint64_t)wss_to_int(args[0]);
    const char* msg = wss_to_str(args[1]);
    WorldPlayer* p = player_by_guid(guid);
    if (p) player_send_message(p, msg);
    else LOG_WARN("player_send_message: player guid=%llu not found", (unsigned long long)guid);
    return WssValue_Null();
}

static WssValue _fn_creature_spawn(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 7) return WssValue_Null();
    (void)args;
    uint64_t guid = 0;
    WssValue r;
    r.type = VAL_INT;
    r.data.as_int = (int64_t)guid;
    return r;
}

static WssValue _fn_creature_despawn(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 1) return WssValue_Null();
    (void)args;
    return WssValue_Null();
}

static WssValue _fn_creature_move_to(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 4) return WssValue_Null();
    (void)args;
    return WssValue_Null();
}

static WssValue _fn_creature_cast(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 2) return WssValue_Null();
    (void)args;
    return WssValue_Null();
}

static WssValue _fn_print(WSSState* S, WssValue* args, int narg) {
    (void)S;
    for (int i = 0; i < narg; i++) {
        if (args[i].type == VAL_INT) printf("%lld", (long long)args[i].data.as_int);
        else if (args[i].type == VAL_DBL) printf("%f", args[i].data.as_dbl);
        else if (args[i].type == VAL_STRING) printf("%s", args[i].data.as_string ? args[i].data.as_string : "");
        else if (args[i].type == VAL_BOOL) printf("%s", args[i].data.as_bool ? "true" : "false");
        else if (args[i].type == VAL_NULL) printf("nil");
    }
    printf("\n");
    return WssValue_Null();
}

static WssValue _fn_sleep(WSSState* S, WssValue* args, int narg) {
    (void)S;
    if (narg < 1) return WssValue_Null();
    int64_t ms = wss_to_int(args[0]);
    if (ms > 0) Sleep((DWORD)ms);
    return WssValue_Null();
}

static WssValue _fn_rand(WSSState* S, WssValue* args, int narg) {
    (void)S;
    int64_t max = narg > 0 ? wss_to_int(args[0]) : 100;
    int r = rand() % (int)max;
    WssValue ret;
    ret.type = VAL_INT;
    ret.data.as_int = r;
    return ret;
}

static WssValue _fn_get_time(WSSState* S, WssValue* args, int narg) {
    (void)S; (void)args; (void)narg;
    time_t t = time(NULL);
    WssValue ret;
    ret.type = VAL_INT;
    ret.data.as_int = (int64_t)t;
    return ret;
}

void WssBindings_Register(WSSState* S) {
    wss_register_cfn(S, "player.create", _fn_player_create);
    wss_register_cfn(S, "player.teleport", _fn_player_teleport);
    wss_register_cfn(S, "player.send_message", _fn_player_send_message);
    wss_register_cfn(S, "creature.spawn", _fn_creature_spawn);
    wss_register_cfn(S, "creature.despawn", _fn_creature_despawn);
    wss_register_cfn(S, "creature.move_to", _fn_creature_move_to);
    wss_register_cfn(S, "creature.cast", _fn_creature_cast);
    wss_register_cfn(S, "print", _fn_print);
    wss_register_cfn(S, "sleep", _fn_sleep);
    wss_register_cfn(S, "rand", _fn_rand);
    wss_register_cfn(S, "get_time", _fn_get_time);
}
