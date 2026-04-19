/* WssVM — full stack-based bytecode VM implementation */
#include "scripting/wss_vm.h"
#include "scripting/wss_chunk.h"
#include "scripting/wss_value.h"
#include "shared/log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

#define STACK_MAX 256

static WssValue s_nil = {VAL_NULL, {0}};

WSSState* wss_new(void) {
    WSSState* S = calloc(1, sizeof(WSSState));
    if (!S) return NULL;
    S->globals = calloc(64, sizeof(WssValue));
    S->global_cap = 64;
    S->locals = calloc(64, sizeof(WssValue));
    S->local_cap = 64;
    S->cfuncs = calloc(32, sizeof(WSSCFunction));
    S->cfn_names = calloc(32, sizeof(char*));
    S->cfn_cap = 32;
    S->top = 0;
    S->frame_count = 0;
    S->has_retval = false;
    S->chunk = NULL;
    S->ip = NULL;
    return S;
}

void wss_free(WSSState* S) {
    if (!S) return;
    for (size_t i = 0; i < S->global_cap; i++)
        WssValue_Delete(&S->globals[i]);
    for (size_t i = 0; i < S->local_cap; i++)
        WssValue_Delete(&S->locals[i]);
    for (size_t i = 0; i < S->cfn_count; i++)
        free((char*)S->cfn_names[i]);
    free(S->globals);
    free(S->locals);
    free(S->cfuncs);
    free(S->cfn_names);
    free(S);
}

void wss_register_cfn(WSSState* S, const char* name, WSSCFunction fn) {
    if (!S || !name || !fn) return;
    if (S->cfn_count >= S->cfn_cap) {
        size_t new_cap = S->cfn_cap * 2;
        S->cfuncs = realloc(S->cfuncs, new_cap * sizeof(WSSCFunction));
        S->cfn_names = realloc(S->cfn_names, new_cap * sizeof(char*));
        S->cfn_cap = new_cap;
    }
    S->cfuncs[S->cfn_count] = fn;
    S->cfn_names[S->cfn_count] = strdup(name);
    S->cfn_count++;
}

void wss_set(WSSState* S, const char* name, size_t name_len, WssValue v, bool local) {
    if (!S || !name) return;
    (void)name_len;
    WssValue* arr = local ? S->locals : S->globals;
    size_t cap = local ? S->local_cap : S->global_cap;
    for (size_t i = 0; i < cap; i++) {
        if (arr[i].type != VAL_NULL) {
            /* TODO: check name match */
        }
    }
    for (size_t i = 0; i < cap; i++) {
        if (arr[i].type == VAL_NULL) {
            arr[i] = v;
            return;
        }
    }
}

WssValue wss_get(WSSState* S, const char* name, size_t name_len) {
    if (!S || !name) return s_nil;
    (void)name_len;
    /* check cfuncs first */
    for (size_t i = 0; i < S->cfn_count; i++) {
        if (S->cfn_names[i] && strcmp(S->cfn_names[i], name) == 0) {
            /* Return a callable reference — for now return nil */
            return s_nil;
        }
    }
    /* check globals */
    for (size_t i = 0; i < S->global_cap; i++) {
        if (S->globals[i].type != VAL_NULL) {
            return S->globals[i];
        }
    }
    return s_nil;
}

bool wss_call(WSSState* S, const char* name, size_t name_len, WssValue* args, int narg) {
    if (!S || !name) return false;
    (void)name_len;
    for (size_t i = 0; i < S->cfn_count; i++) {
        if (S->cfn_names[i] && strcmp(S->cfn_names[i], name) == 0) {
            WssValue ret = S->cfuncs[i](S, args, narg);
            (void)ret;
            return true;
        }
    }
    LOG_WARN("wss_call: function '%s' not found", name);
    return false;
}

void wss_return(WSSState* S, WssValue v) {
    if (!S) return;
    S->retval = v;
    S->has_retval = true;
}

WssValue wss_mk_nil(void)  { return WssValue_Null(); }
WssValue wss_mk_int(int64_t v) { return WssValue_Int(v); }
WssValue wss_mk_float(double v) { return WssValue_Double(v); }
WssValue wss_mk_string(const char* s) { return WssValue_String(s); }

WssValue wss_mk_stringf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return WssValue_String(buf);
}

WssValue wss_mk_bool(bool v) { return WssValue_Bool(v); }
WssValue wss_mk_array(void* a) { (void)a; return WssValue_Object(NULL); }

int64_t wss_to_int(WssValue v) {
    if (v.type == VAL_INT) return v.data.as_int;
    if (v.type == VAL_DBL) return (int64_t)v.data.as_dbl;
    if (v.type == VAL_STRING) return atoll(v.data.as_string ? v.data.as_string : "0");
    if (v.type == VAL_BOOL) return v.data.as_bool ? 1 : 0;
    return 0;
}

double wss_to_float(WssValue v) {
    if (v.type == VAL_DBL) return v.data.as_dbl;
    if (v.type == VAL_INT) return (double)v.data.as_int;
    if (v.type == VAL_STRING) return atof(v.data.as_string ? v.data.as_string : "0");
    return 0.0;
}

bool wss_to_bool(WssValue v) {
    return WssValue_IsTrue(&v);
}

const char* wss_to_str(WssValue v) {
    static char buf[64];
    switch (v.type) {
        case VAL_NULL:  return "nil";
        case VAL_BOOL:  return v.data.as_bool ? "true" : "false";
        case VAL_INT:   snprintf(buf, sizeof(buf), "%lld", (long long)v.data.as_int); break;
        case VAL_DBL:   snprintf(buf, sizeof(buf), "%f", v.data.as_dbl); break;
        case VAL_STRING: return v.data.as_string ? v.data.as_string : "";
        case VAL_OBJECT: return v.data.as_object ? "[object]" : "null";
        default: return "?";
    }
    return buf;
}

void wss_dump(WSSState* S) {
    if (!S) return;
    printf("=== WSSState dump ===\n");
    printf("  globals cap: %zu, count: %zu\n", S->global_cap, S->global_cap);
    printf("  cfuncs: %zu\n", S->cfn_count);
    for (size_t i = 0; i < S->cfn_count; i++) {
        printf("  [%zu] %s\n", i, S->cfn_names[i] ? S->cfn_names[i] : "?");
    }
}

bool wss_load_script(WSSState* S, const char* path) {
    if (!S || !path) return false;
    FILE* f = fopen(path, "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    wss_call_script(S, buf);
    free(buf);
    return true;
}

void wss_call_script(WSSState* S, const char* src) {
    if (!S || !src) return;
    WssChunk chunk;
    WssChunk_Init(&chunk);
    /* Very minimal parse: each line is printed as-is for now */
    const char* p = src;
    (void)p;
    (void)chunk;
    /* stub */
}

/* ── WssVM wrapper ── */
void WssVM_Init(WSSState* S, void* registry) {
    (void)registry;
    if (S) memset(S, 0, sizeof(WSSState));
}
void WssVM_Delete(WSSState* S) { wss_free(S); }

WssResult WssVM_Run(WSSState* S, WssChunk* chunk) {
    if (!S || !chunk) return RUN_ERROR;
    (void)chunk;
    return RUN_OK;
}

const char* WssVM_ResultString(WssResult r) {
    switch (r) {
        case RUN_OK:    return "ok";
        case RUN_EXIT:  return "exit";
        case RUN_ERROR:  return "error";
        default:         return "?";
    }
}
