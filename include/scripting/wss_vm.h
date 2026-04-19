#ifndef WSS_VM_H
#define WSS_VM_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <scripting/wss_value.h>

typedef enum {
    WSS_NIL=0, WSS_INT, WSS_FLOAT, WSS_STRING, WSS_BOOL, WSS_ARRAY, WSS_FUNCTION, WSS_CFUNCTION
} WSSValueType;

/* WSSValue is defined in wss_value.h — this typedef is for forward reference only */
typedef struct WSSValue WSSValue;
typedef struct WSSArray WSSArray;
typedef struct WSSFunction WSSFunction;

/* ── VM state + result ── */
typedef enum { RUN_OK=0, RUN_EXIT=1, RUN_ERROR=2 } WssResult;

typedef struct WssChunk WssChunk; /* forward */

typedef struct WSSState {
    WSSValue* globals;
    size_t    global_cap;
    WSSValue* locals;
    size_t    local_cap;
    WSSValue  retval;
    bool      has_retval;
    WSSCFunction* cfuncs;
    const char** cfn_names;
    size_t    cfn_cap, cfn_count;
    WSSValue  stack[256];
    int       top;
    int       frame_count;
    uint8_t*  ip;
    WssChunk* chunk;
    char      err[512];
} WSSState;

WSSState* wss_new(void);
void       wss_free(WSSState* S);
void       wss_register_cfn(WSSState* S, const char* name, WSSCFunction fn);
void       wss_set(WSSState* S, const char* name, size_t name_len, WSSValue v, bool local);
WSSValue   wss_get(WSSState* S, const char* name, size_t name_len);
bool       wss_call(WSSState* S, const char* name, size_t name_len, WSSValue* args, int narg);
void       wss_return(WSSState* S, WSSValue v);
WSSValue   wss_mk_nil(void);
WSSValue   wss_mk_int(int64_t v);
WSSValue   wss_mk_float(double v);
WSSValue   wss_mk_string(const char* s);
WSSValue   wss_mk_stringf(const char* fmt, ...);
WSSValue   wss_mk_bool(bool v);
WSSValue   wss_mk_array(WSSArray* a);
int64_t    wss_to_int(WSSValue v);
double     wss_to_float(WSSValue v);
bool       wss_to_bool(WSSValue v);
const char* wss_to_str(WSSValue v);
void       wss_dump(WSSState* S);
bool       wss_load_script(WSSState* S, const char* path);
void       wss_call_script(WSSState* S, const char* src);

/* ── WssVM wrapper (called from wss_wow_hooks.c) ── */
void  WssVM_Init(WSSState* S, void* registry);
void  WssVM_Delete(WSSState* S);
WssResult WssVM_Run(WSSState* S, WssChunk* chunk);
const char* WssVM_ResultString(WssResult r);

#endif
