/* script_engine.c — Word/statement scripting engine implementation */
#include "scripting/script_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define LOG(...)  do { if (e->logFn)  e->logFn(__VA_ARGS__); } while(0)
#define ERR(...)  do { if (e->errorFn) e->errorFn(__VA_ARGS__); } while(0)
#define SET_ERR(msg) do { e->lastError[0]=0; snprintf(e->lastError, sizeof(e->lastError), "line %d: %s", ctx->lineNo, msg); ctx->error=true; } while(0)

/* ═══════════════════════════ LEXER ═══════════════════════════ */

static const char* _src;
static int _len;
static int _pos;
static int _line;
static Token _tok;

static bool _isident(int c) { return isalpha(c) || c == '_' || c == '@'; }

static void _lex_init(const char* src, int len) {
    _src = src; _len = len; _pos = 0; _line = 1;
}

static void _skip_whitespace_and_comments(void) {
    while (_pos < _len) {
        int c = _src[_pos];
        if (c == '/' && _pos + 1 < _len && _src[_pos+1] == '/') {
            while (_pos < _len && _src[_pos] != '\n') _pos++;
            _line++;
        } else if (isspace(c)) {
            if (c == '\n') _line++;
            _pos++;
        } else break;
    }
}

static void _lex_next(Token* out) {
    _skip_whitespace_and_comments();
    memset(out, 0, sizeof(Token));
    out->line = _line;
    if (_pos >= _len) { out->type = T_EOF; return; }

    int c = _src[_pos];

    if (c == '{') { out->type = T_LBRACE; _pos++; return; }
    if (c == '}') { out->type = T_RBRACE; _pos++; return; }
    if (c == ';') { out->type = T_SEMICOLON; _pos++; return; }

    if (c == '"') {
        out->type = T_STRING;
        _pos++; int j = 0;
        while (_pos < _len && _src[_pos] != '"') {
            if (_src[_pos] == '\\' && _pos+1 < _len) _pos++;
            out->text[j++] = _src[_pos++];
        }
        if (_pos < _len) _pos++;
        return;
    }

    if (isdigit(c) || (c == '-' && _pos+1 < _len && isdigit(_src[_pos+1]))) {
        int start = _pos;
        while (_pos < _len && (isalnum(_src[_pos]) || _src[_pos]=='.' || _src[_pos]=='-' || _src[_pos]=='+')) _pos++;
        int n = _pos - start;
        char buf[64] = {0};
        strncpy(buf, _src+start, n < 63 ? n : 63);
        if (strchr(buf, '.')) { out->type = T_NUMBER; out->fval = atof(buf); }
        else { out->type = T_INT; out->ival = atoll(buf); }
        strncpy(out->text, buf, MAX_TOKEN_LEN-1);
        return;
    }

    if (_isident(c)) {
        out->type = T_WORD;
        int j = 0;
        while (_pos < _len && (_isident(_src[_pos]) || isdigit(_src[_pos]))) {
            if (j < MAX_TOKEN_LEN-1) out->text[j++] = _src[_pos];
            _pos++;
        }
        return;
    }

    out->type = T_EOF;
    _pos++;
}

#define lex_next(tok) _lex_next(tok)

/* ═══════════════════════════ PARSER + EXECUTOR ═══════════════════════════ */

typedef struct {
    char word[MAX_TOKEN_LEN];
    char eventType[MAX_TOKEN_LEN];
    StatementFn fn;
} WordBinding;

typedef struct {
    char     name[MAX_TOKEN_LEN];
    char     params[MAX_TOKEN_LEN];
    char     body[MAX_TOKEN_LEN];
    StatementFn fn;
    int      refs;
} FuncDef;

typedef struct ParseState {
    ScriptEngine* engine;
    ScriptContext* ctx;
    Token  tok;
    int    stmt_count;
    FuncDef funcs[256];
    int    func_count;
    WordBinding events[256];
    int    event_count;
    char   error[512];
} ParseState;

static void _parse_word_event(ParseState* p) {
    /* onDeath creature . spawn at 100 200 300 */
    if (p->tok.type != T_WORD) { snprintf(p->error, sizeof(p->error), "expected word, got %d", p->tok.type); return; }
    char word[MAX_TOKEN_LEN]; strncpy(word, p->tok.text, MAX_TOKEN_LEN-1);
    lex_next(&p->tok);

    if (p->tok.type != T_WORD) { snprintf(p->error, sizeof(p->error), "expected event type"); return; }
    char evt[MAX_TOKEN_LEN]; strncpy(evt, p->tok.text, MAX_TOKEN_LEN-1);
    lex_next(&p->tok);

    char fullWord[MAX_TOKEN_LEN*2];
    snprintf(fullWord, sizeof(fullWord), "%s:%s", word, evt);
    ScriptEngine_RegisterWordEvent(p->engine, word, evt, NULL, NULL);
    (void)fullWord;
}

static void _parse_block(ParseState* p, int depth);
static void _parse_statement(ParseState* p) {
    if (p->tok.type == T_WORD) {
        /* Check for built-in statements */
        char* w = p->tok.text;
        lex_next(&p->tok);

        /* word event binding: onDeath creature . */
        if (!strcmp(w, "onDeath") || !strcmp(w, "onSpawn") || !strcmp(w, "onGossip") ||
            !strcmp(w, "onEnter")  || !strcmp(w, "onLeave")  || !strcmp(w, "onTimer") ||
            !strcmp(w, "onKill")  || !strcmp(w, "onHit")    || !strcmp(w, "onUse") ||
            !strcmp(w, "onLogin") || !strcmp(w, "onLogout") || !strcmp(w, "onSay")) {
            char evt[MAX_TOKEN_LEN]; strncpy(evt, w, MAX_TOKEN_LEN-1);
            if (p->tok.type == T_WORD) {
                char target[MAX_TOKEN_LEN]; strncpy(target, p->tok.text, MAX_TOKEN_LEN-1);
                lex_next(&p->tok);
                ScriptEngine_RegisterWordEvent(p->engine, target, evt, NULL, NULL);
            }
            if (p->tok.type == T_LBRACE) { lex_next(&p->tok); _parse_block(p, 1); }
            return;
        }

        /* var creation: creature . myvar = 5 */
        if (!strcmp(w, "var")) {
            /* Handled as a lookup; skip for now */
            return;
        }

        /* if / while / for blocks */
        if (!strcmp(w, "if")) {
            lex_next(&p->tok);
            _parse_block(p, 1);
            if (p->tok.type == T_WORD && !strcmp(p->tok.text, "else")) {
                lex_next(&p->tok);
                if (p->tok.type == T_LBRACE) { lex_next(&p->tok); _parse_block(p, 1); }
                else _parse_statement(p);
            }
            return;
        }
        if (!strcmp(w, "while")) { lex_next(&p->tok); _parse_block(p, 1); return; }
        if (!strcmp(w, "for"))   { lex_next(&p->tok); _parse_block(p, 1); return; }
        if (!strcmp(w, "return")) { p->ctx->returnEarly = true; return; }

        /* Built-in print/log */
        if (!strcmp(w, "print") || !strcmp(w, "log")) {
            char buf[1024] = {0};
            int i = 0;
            while (p->tok.type != T_SEMICOLON && p->tok.type != T_EOF) {
                if (p->tok.type == T_STRING) { strncat(buf, p->tok.text, sizeof(buf)-strlen(buf)-1); }
                else if (p->tok.type == T_INT) { char tmp[64]; snprintf(tmp, sizeof(tmp), "%lld", (long long)p->tok.ival); strncat(buf, tmp, sizeof(buf)-strlen(buf)-1); }
                else if (p->tok.type == T_NUMBER) { char tmp[64]; snprintf(tmp, sizeof(tmp), "%g", p->tok.fval); strncat(buf, tmp, sizeof(buf)-strlen(buf)-1); }
                else if (p->tok.type == T_WORD) { strncat(buf, p->tok.text, sizeof(buf)-strlen(buf)-1); }
                lex_next(&p->tok);
            }
            Script_Log(p->ctx, "%s", buf);
            return;
        }

        /* try finding a registered statement handler */
        for (int i = 0; i < p->engine->statementCount; i++) {
            if (!strcmp(p->engine->statementNames[i], w)) {
                p->ctx->argCount = 0;
                while (p->tok.type != T_SEMICOLON && p->tok.type != T_EOF) {
                    if (p->ctx->argCount < MAX_ARGS) {
                        memcpy(&p->ctx->args[p->ctx->argCount++], &p->tok, sizeof(Token));
                    }
                    lex_next(&p->tok);
                }
                p->engine->statements[i](p->ctx);
                return;
            }
        }
        return;
    }
    if (p->tok.type == T_LBRACE) { lex_next(&p->tok); _parse_block(p, 1); }
    if (p->tok.type == T_SEMICOLON) lex_next(&p->tok);
}

static void _parse_block(ParseState* p, int depth) {
    while (p->tok.type != T_RBRACE && p->tok.type != T_EOF) {
        _parse_statement(p);
        if (p->ctx->error) return;
        if (p->ctx->returnEarly) return;
    }
    if (p->tok.type == T_RBRACE) lex_next(&p->tok);
}

/* ═══════════════════════════ ENGINE API ═══════════════════════════ */

ScriptEngine* ScriptEngine_Create(void* worldServer) {
    ScriptEngine* e = calloc(1, sizeof(ScriptEngine));
    e->worldServer = worldServer;
    e->logFn = printf;
    e->errorFn = fprintf;
    return e;
}

void ScriptEngine_Destroy(ScriptEngine* e) {
    if (!e) return;
    for (int i = 0; i < e->wordEventCount; i++) {
        free(e->wordEvents[i]);
    }
    free(e->statements);
    free(e->statementNames);
    free(e);
}

void ScriptEngine_SetLogFn(ScriptEngine* e, void (*fn)(const char*, ...)) {
    e->logFn = fn;
}

const char* ScriptEngine_LastError(ScriptEngine* e) {
    return e->lastError;
}

void ScriptEngine_RegisterStatement(ScriptEngine* e, const char* name, StatementFn fn) {
    int newCount = e->statementCount + 1;
    StatementFn* p1 = realloc(e->statements, (size_t)newCount * sizeof(StatementFn));
    char** p2 = realloc(e->statementNames, (size_t)newCount * sizeof(char*));
    if (!p1 || !p2) { free(p1); free(p2); return; }
    e->statements = p1;
    e->statementNames = p2;
    e->statements[e->statementCount] = fn;
    e->statementNames[e->statementCount] = strdup(name);
    e->statementCount++;
}

void ScriptEngine_RegisterWordEvent(ScriptEngine* e, const char* word, const char* eventType, StatementFn fn, ScriptObject* owner) {
    if (e->wordEventCount >= MAX_WORDEVENTS-1) return;
    WordEvent* ev = calloc(1, sizeof(WordEvent));
    strncpy(ev->word, word, MAX_TOKEN_LEN-1);
    strncpy(ev->eventType, eventType, MAX_TOKEN_LEN-1);
    ev->handler = fn;
    ev->owner = owner;
    e->wordEvents[e->wordEventCount++] = ev;
}

void ScriptEngine_ExecuteWordEvent(ScriptEngine* e, const char* word, const char* eventType, void* trigger) {
    char fullWord[MAX_TOKEN_LEN*2];
    snprintf(fullWord, sizeof(fullWord), "%s:%s", word, eventType);
    for (int i = 0; i < e->wordEventCount; i++) {
        WordEvent* ev = e->wordEvents[i];
        if (!strcmp(ev->word, word) && !strcmp(ev->eventType, eventType)) {
            ScriptContext ctx = {0};
            ctx.engine = e;
            ctx.userData = trigger;
            ctx.lineNo = 0;
            if (ev->handler) ev->handler(&ctx);
        }
    }
    (void)fullWord;
}

bool ScriptEngine_LoadScriptString(ScriptEngine* e, const char* code, const char* name) {
    ParseState p = {0};
    p.engine = e;
    ScriptContext ctx = {0};
    ctx.engine = e;
    p.ctx = &ctx;

    _lex_init(code, strlen(code));
    lex_next(&p.tok);
    _parse_block(&p, 0);

    if (p.error[0]) { snprintf(e->lastError, sizeof(e->lastError), "%s", p.error); return false; }
    return !ctx.error;
}

bool ScriptEngine_LoadScript(ScriptEngine* e, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) { snprintf(e->lastError, sizeof(e->lastError), "cannot open %s", filename); return false; }
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    fread(buf, 1, len, f); buf[len] = 0; fclose(f);
    bool ok = ScriptEngine_LoadScriptString(e, buf, filename);
    free(buf);
    return ok;
}

/* ═══════════════════════════ CONTEXT HELPERS ═══════════════════════════ */

int64_t Script_GetIntArg(ScriptContext* ctx, int idx) {
    if (idx < 0 || idx >= ctx->argCount) return 0;
    return ctx->args[idx].ival;
}
double Script_GetFloatArg(ScriptContext* ctx, int idx) {
    if (idx < 0 || idx >= ctx->argCount) return 0.0;
    return ctx->args[idx].fval;
}
const char* Script_GetStringArg(ScriptContext* ctx, int idx) {
    if (idx < 0 || idx >= ctx->argCount) return "";
    return ctx->args[idx].text;
}
ObjectGuid Script_GetGuidArg(ScriptContext* ctx, int idx) {
    if (idx < 0 || idx >= ctx->argCount) return 0;
    return (ObjectGuid)ctx->args[idx].ival;
}
void Script_Error(ScriptContext* ctx, const char* fmt, ...) {
    ctx->error = true;
    va_list ap; va_start(ap, fmt); vsnprintf(ctx->errorMsg, sizeof(ctx->errorMsg), fmt, ap); va_end(ap);
}
void Script_Log(ScriptContext* ctx, const char* fmt, ...) {
    char buf[1024];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    if (ctx->engine->logFn) ctx->engine->logFn("[script] %s\n", buf);
}
void Script_PushScope(ScriptContext* ctx, bool active) {
    if (ctx->scopeDepth < MAX_STACK) { ctx->scopeStack[ctx->scopeDepth].active = active; ctx->scopeDepth++; }
}
void Script_PopScope(ScriptContext* ctx) {
    if (ctx->scopeDepth > 0) ctx->scopeDepth--;
}
void Script_Jump(ScriptContext* ctx, int64_t target) {
    ctx->jumpTarget = target;
}
void Script_BindUnit(ScriptContext* ctx, void* unit) { ctx->userData = unit; }
void* Script_GetUnit(ScriptContext* ctx) { return ctx->userData; }
const char* Script_Word(ScriptContext* ctx) {
    return ctx->argCount > 0 ? ctx->args[0].text : "";
}
void* ScriptEngine_GetWorldServer(ScriptEngine* e) { return e->worldServer; }
