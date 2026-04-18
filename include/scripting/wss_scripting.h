#ifndef WSS_SCRIPTING_H
#define WSS_SCRIPTING_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
   WSS — Word Statement Scripting System
   Scripts written as natural English words and statements.
   Example:
     set player_health = 100
     if player_health is less than 50 then
         cast "Healing Wave"
     end
   ==================================================================== */

#define WSS_MAX_STACK       256
#define WSS_MAX_LOCALS      128
#define WSS_MAX_CALLDEPTH   64
#define WSS_MAX_TOKEN_LEN   128
#define WSS_MAX_STR_LEN     4096
#define WSS_MAX_ARGS        16
#define WSS_MAX_GLOBALS     512
#define WSS_MAX_CODE        65536
#define WSS_MAX_FUNCS      128
#define WSS_MAX_SCRIPTS     64

typedef enum {
    VAL_NULL = 0, VAL_INT, VAL_FLOAT, VAL_STRING,
    VAL_BOOL, VAL_FUNC, VAL_ARRAY, VAL_OBJECT,
    VAL_HOOK, VAL_NPC, VAL_PLAYER, VAL_SPELL, VAL_UNIT
} WssValType;

typedef enum {
    HOOK_NONE = 0,
    HOOK_ON_INIT, HOOK_ON_UPDATE, HOOK_ON_TICK, HOOK_ON_SPAWN,
    HOOK_ON_GOSSIP_HELLO, HOOK_ON_GOSSIP_SELECT, HOOK_ON_SAY,
    HOOK_ON_EMOTE, HOOK_ON_ATTACK, HOOK_ON_DAMAGE, HOOK_ON_DEATH,
    HOOK_ON_CAST, HOOK_ON_SPELLHIT, HOOK_ON_ENTER_WORLD,
    HOOK_ON_LEAVE_WORLD, HOOK_ON_QUEST_ACCEPT, HOOK_ON_QUEST_COMPLETE,
    HOOK_ON_LEVEL_UP, HOOK_ON_CREATE, HOOK_ON_MAX
} WssHookType;

typedef enum {
    OP_NOP=0, OP_LOADNULL, OP_LOADINT, OP_LOADFLT, OP_LOADSTR, OP_LOADBOOL,
    OP_LOADGLOBAL, OP_STOREGLOBAL, OP_LOADLOCAL, OP_STORELOCAL,
    OP_NEWARRAY, OP_LOADINDEX, OP_STOREINDEX,
    OP_POP, OP_DUP, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG,
    OP_AND, OP_OR, OP_XOR, OP_NOT, OP_SHL, OP_SHR,
    OP_CMP_EQ, OP_CMP_NE, OP_CMP_LT, OP_CMP_GT, OP_CMP_LE, OP_CMP_GE,
    OP_JUMP, OP_JUMPIF, OP_JUMPNOT, OP_CALL, OP_CALLNATIVE, OP_RETURN,
    OP_CONCAT, OP_LEN, OP_INCREMENT, OP_DECREMENT, OP_LOADFIELD, OP_STOREFIELD
} WssOpCode;

typedef struct WssValue {
    WssValType type;
    union {
        int64_t as_int;
        double as_float;
        bool as_bool;
        char as_str[WSS_MAX_STR_LEN];
        void* as_ptr;
    } data;
} WssValue;

typedef struct WssVm WssVm;
typedef struct WssScriptingSystem WssScriptingSystem;
typedef struct WssObjectStore WssObjectStore;

typedef WssValue (*WssNativeFn)(WssScriptingSystem* sys, int argc, WssValue* argv);
typedef void (*WssHookFn)(WssScriptingSystem* sys, int arg_count, WssValue* args);

struct WssScriptingSystem {
    WssVm* vm;
    WssObjectStore* registry;
    WssNativeFn* natives;
    int native_count;
    WssHookFn hooks[HOOK_ON_MAX];
    char loaded_scripts[WSS_MAX_SCRIPTS][64];
    int script_count;
    uint64_t tick_count;
    bool initialized;
};

struct WssObjectStore {
    void** buckets;
    int bucket_count;
    int count;
};

void WssSystem_Init(WssScriptingSystem* sys, WssObjectStore* registry);
void WssSystem_Delete(WssScriptingSystem* sys);
void WssSystem_LoadScript(WssScriptingSystem* sys, const char* name, const char* source);
void WssSystem_RunHook(WssScriptingSystem* sys, WssHookType type, int arg_count, WssValue* args);
void WssSystem_RegisterNative(WssScriptingSystem* sys, const char* name, WssNativeFn fn, int min_args);
WssValue WssSystem_Call(WssScriptingSystem* sys, const char* func_name, int argc, WssValue* argv);
void WssObjectStore_Init(WssObjectStore* store, int bucket_count);
void WssObjectStore_Delete(WssObjectStore* store);
void WssObjectStore_Set(WssObjectStore* store, const char* key, WssValue* val);
bool WssObjectStore_Get(WssObjectStore* store, const char* key, WssValue* out_val);
void WssObjectStore_Remove(WssObjectStore* store, const char* key);

const char* WssValue_ToString(WssValue* v, char* buf, int len);
void WssValue_Copy(WssValue* dst, const WssValue* src);
void WssValue_Clear(WssValue* v);

#endif
