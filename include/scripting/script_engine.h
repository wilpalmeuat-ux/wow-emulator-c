/* script_engine.h — Word/statement scripting engine for WoW 3.3.5 emulator */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

#define MAX_TOKEN_LEN 256
#define MAX_ARGS      16
#define MAX_STACK     64
#define MAX_CALLSTACK 32
#define MAX_WORDEVENTS 512

typedef uint64_t ObjectGuid;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;
#define GUID_LOPART(x) ((uint32)((x) & 0xFFFFFFFFULL))

/* ── Token types ── */
typedef enum { T_WORD, T_STRING, T_NUMBER, T_INT, T_LBRACE, T_RBRACE, T_SEMICOLON, T_EOF } WssEngineTokenType;

typedef struct { WssEngineTokenType type; char text[MAX_TOKEN_LEN]; int64_t ival; double fval; int line; } Token;

/* ── Forward declarations ── */
typedef struct ScriptEngine    ScriptEngine;
typedef struct ScriptContext   ScriptContext;
typedef struct ScriptObject    ScriptObject;
typedef struct WordEvent       WordEvent;
typedef void (*StatementFn)(ScriptContext*);

/* ── Context passed into every statement handler ── */
struct ScriptContext {
    ScriptEngine* engine;
    ScriptObject*  target;
    void*          userData;
    Token          args[MAX_ARGS];
    int            argCount;
    void*          callStack[MAX_CALLSTACK];
    int            callDepth;
    bool           returnEarly;
    int64_t        jumpTarget;
    int            lineNo;
    bool           error;
    char           errorMsg[512];
    struct { bool active; int64_t jumpTo; } scopeStack[MAX_STACK];
    int            scopeDepth;
};

/* ── Word/Statement event binding ── */
struct WordEvent {
    char       word[MAX_TOKEN_LEN];    /* the keyword/trigger word */
    char       eventType[MAX_TOKEN_LEN];/* "onDeath", "onGossip", "onSpawn"... */
    StatementFn handler;
    ScriptObject* owner;
    int         refs;
    struct WordEvent* next;
};

/* ── Core engine ── */
struct ScriptEngine {
    void*        worldServer;
    StatementFn* statements;
    char**       statementNames;
    int          statementCount;
    WordEvent*   wordEvents[MAX_WORDEVENTS];
    int          wordEventCount;
    void (*logFn)(const char* msg, ...);
    void (*errorFn)(const char* fmt, ...);
    char         lastError[512];
};

/* ═══════════════════════════════════════════════════════
   CORE API
═══════════════════════════════════════════════════════ */
ScriptEngine* ScriptEngine_Create(void* worldServer);
void           ScriptEngine_Destroy(ScriptEngine* e);
bool           ScriptEngine_LoadScript(ScriptEngine* e, const char* filename);
bool           ScriptEngine_LoadScriptString(ScriptEngine* e, const char* code, const char* name);
void           ScriptEngine_RegisterStatement(ScriptEngine* e, const char* name, StatementFn fn);
void           ScriptEngine_RegisterWordEvent(ScriptEngine* e, const char* word, const char* eventType, StatementFn fn, ScriptObject* owner);
void           ScriptEngine_ExecuteWordEvent(ScriptEngine* e, const char* word, const char* eventType, void* trigger);
void           ScriptEngine_SetLogFn(ScriptEngine* e, void (*fn)(const char*, ...));
const char*    ScriptEngine_LastError(ScriptEngine* e);

/* ═══════════════════════════════════════════════════════
   CONTEXT HELPERS
═══════════════════════════════════════════════════════ */
int64_t      Script_GetIntArg(ScriptContext* ctx, int idx);
double       Script_GetFloatArg(ScriptContext* ctx, int idx);
const char*  Script_GetStringArg(ScriptContext* ctx, int idx);
ObjectGuid   Script_GetGuidArg(ScriptContext* ctx, int idx);
void         Script_Error(ScriptContext* ctx, const char* fmt, ...);
void         Script_Log(ScriptContext* ctx, const char* fmt, ...);
void         Script_PushScope(ScriptContext* ctx, bool active);
void         Script_PopScope(ScriptContext* ctx);
void         Script_Jump(ScriptContext* ctx, int64_t target);
void         Script_BindUnit(ScriptContext* ctx, void* unit);
void*        Script_GetUnit(ScriptContext* ctx);
const char*  Script_Word(ScriptContext* ctx);
void*        ScriptEngine_GetWorldServer(ScriptEngine* e);
