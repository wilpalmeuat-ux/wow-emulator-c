#ifndef WSS_VM_H
#define WSS_VM_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <scripting/wss_value.h>

/* WSSState and WSSValue are already defined in wss_value.h */

typedef enum { RUN_OK=0, RUN_EXIT=1, RUN_ERROR=2 } WssResult;

typedef struct WssChunk WssChunk; /* forward */

typedef struct WSSState {
    WssValue* globals;
    size_t    global_cap;
    WssValue* locals;
    size_t    local_cap;
    WssValue  retval;
    bool      has_retval;
    WSSCFunction* cfuncs;
    const char** cfn_names;
    size_t    cfn_cap, cfn_count;
    WssValue  stack[256];
    int       top;
    int       frame_count;
    uint8_t*  ip;
    WssChunk* chunk;
    char      err[512];
} WSSState;

WSSState* wss_new(void);
void       wss_free(WSSState* S);
void       wss_register_cfn(WSSState* S, const char* name, WSSCFunction fn);
void       wss_set(WSSState* S, const char* name, size_t name_len, WssValue v, bool local);
WssValue   wss_get(WSSState* S, const char* name, size_t name_len);
bool       wss_call(WSSState* S, const char* name, size_t name_len, WssValue* args, int narg);
void       wss_return(WSSState* S, WssValue v);
WssValue   wss_mk_nil(void);
WssValue   wss_mk_int(int64_t v);
WssValue   wss_mk_float(double v);
WssValue   wss_mk_string(const char* s);
WssValue   wss_mk_stringf(const char* fmt, ...);
WssValue   wss_mk_bool(bool v);
WssValue   wss_mk_array(void* a);
int64_t    wss_to_int(WssValue v);
double     wss_to_float(WssValue v);
bool       wss_to_bool(WssValue v);
const char* wss_to_str(WssValue v);
void       wss_dump(WSSState* S);
bool       wss_load_script(WSSState* S, const char* path);
void       wss_call_script(WSSState* S, const char* src);

/* ── WssVM wrapper (called from wss_wow_hooks.c) ── */
void  WssVM_Init(WSSState* S, void* registry);
void  WssVM_Delete(WSSState* S);
WssResult WssVM_Run(WSSState* S, WssChunk* chunk);
const char* WssVM_ResultString(WssResult r);

#endif
