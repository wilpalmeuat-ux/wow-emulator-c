#include "scripting/wss_parser.h"
#include "shared/log.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Lexer* _L;
static Token  _tok;
static WSSState* _S;
static bool _at_eof = false;

static void _adv(void) {
    if (!lex_next(_L, &_tok)) { _at_eof = true; _tok.text.text = ""; _tok.text.len = 0; _tok.len = 0; }
}
static int _eat(TokenType t) {
    (void)t;
    return (_tok.text.len == 0 && t == TK_EOF) ? 1 : (_tok.text.len > 0 ? 1 : 0);
}
static void _expect(TokenType t, const char* msg) {
    if (!_eat(t)) LOG_FATAL("Parse error: expected %s, got %.*s at line %d", tok_name(t), _tok.text.len, _tok.text.text, _L->line);
}
static WSSValue _eval_primary(void) {
    if (_tok.text.len == 0) return wss_mk_nil();
    if (_tok.text.len == 4 && !strncmp(_tok.text.text, "true", 4)) { _adv(); return wss_mk_bool(true); }
    if (_tok.text.len == 5 && !strncmp(_tok.text.text, "false", 5)) { _adv(); return wss_mk_bool(false); }
    if (_tok.text.len == 3 && !strncmp(_tok.text.text, "nil", 3)) { _adv(); return wss_mk_nil(); }
    if (_tok.text.text[0] == '"' || _tok.text.text[0] == '\'') {
        char* s = malloc(_tok.text.len + 1);
        int j = 0;
        for (int i = 1; i < _tok.text.len - 1; i++) s[j++] = _tok.text.text[i];
        s[j] = 0;
        _adv();
        return wss_mk_string(s);
    }
    if (isdigit(_tok.text.text[0]) || (_tok.text.text[0] == '.' && _tok.text.len > 1)) {
        char* end;
        double d = strtod(_tok.text.text, &end);
        _adv();
        return (d == (int64_t)d) ? wss_mk_int((int64_t)d) : wss_mk_float(d);
    }
    char* name = malloc(_tok.text.len + 1);
    memcpy(name, _tok.text.text, _tok.text.len);
    name[_tok.text.len] = 0;
    _adv();
    WSSValue v = wss_get(_S, name, strlen(name));
    free(name);
    return v;
}
static WSSValue _eval_unary(void) {
    if (_tok.text.len == 1 && _tok.text.text[0] == '!') { _adv(); return wss_mk_bool(!wss_to_bool(_eval_unary())); }
    if (_tok.text.len == 1 && _tok.text.text[0] == '-') { _adv(); WSSValue v = _eval_unary(); return wss_mk_int(-wss_to_int(v)); }
    return _eval_primary();
}
static WSSValue _eval_mul(void) {
    WSSValue v = _eval_unary();
    while (_tok.text.len == 1 && (_tok.text.text[0] == '*' || _tok.text.text[0] == '/' || _tok.text.text[0] == '%')) {
        char op = _tok.text.text[0]; _adv();
        WSSValue r = _eval_unary();
        if (op == '*') v = wss_mk_int(wss_to_int(v) * wss_to_int(r));
        else if (op == '/') v = wss_mk_int(wss_to_int(v) / wss_to_int(r));
        else v = wss_mk_int(wss_to_int(v) % wss_to_int(r));
    }
    return v;
}
static WSSValue _eval_add(void) {
    WSSValue v = _eval_mul();
    while (_tok.text.len == 1 && (_tok.text.text[0] == '+' || _tok.text.text[0] == '-')) {
        char op = _tok.text.text[0]; _adv();
        WSSValue r = _eval_mul();
        if (op == '+') {
            if (v.type == WSS_STRING || r.type == WSS_STRING) {
                char buf[512];
                snprintf(buf, sizeof(buf), "%s%s", wss_to_str(v), wss_to_str(r));
                v = wss_mk_string(strdup(buf));
            } else { v = wss_mk_int(wss_to_int(v) + wss_to_int(r)); }
        } else { v = wss_mk_int(wss_to_int(v) - wss_to_int(r)); }
    }
    return v;
}
static WSSValue _eval_cmp(void) {
    WSSValue v = _eval_add();
    int neg = 0;
    if (_tok.text.len == 2 && _tok.text.text[0] == '!' && _tok.text.text[1] == '=') { neg = 1; _adv(); }
    else if (_tok.text.len == 1 && _tok.text.text[0] == '<') { neg = 0; _adv(); }
    else if (_tok.text.len == 2 && _tok.text.text[0] == '<') { neg = 0; _adv(); }
    else if (_tok.text.len == 1 && _tok.text.text[0] == '>') { neg = 0; _adv(); }
    else if (_tok.text.len == 2 && _tok.text.text[0] == '>') { neg = 0; _adv(); }
    else return v;
    WSSValue r = _eval_add();
    int res = 0;
    char* p = strchr(wss_to_str(v), '.');
    if (p || strchr(wss_to_str(r), '.')) { double a = strtod(wss_to_str(v), NULL), b = strtod(wss_to_str(r), NULL); res = (a != b); }
    else { int64_t a = wss_to_int(v), b = wss_to_int(r); res = (a != b); }
    return wss_mk_bool(neg ? !res : res);
}
static WSSValue _eval_and(void) {
    WSSValue v = _eval_cmp();
    while (_tok.text.len == 2 && _tok.text.text[0] == '&' && _tok.text.text[1] == '&') { _adv(); WSSValue r = _eval_cmp(); v = wss_mk_bool(wss_to_bool(v) && wss_to_bool(r)); }
    return v;
}
static WSSValue _eval_or(void) {
    WSSValue v = _eval_and();
    while (_tok.text.len == 2 && _tok.text.text[0] == '|' && _tok.text.text[1] == '|') { _adv(); WSSValue r = _eval_and(); v = wss_mk_bool(wss_to_bool(v) || wss_to_bool(r)); }
    return v;
}
static void _exec_block(void);
static void _exec_if(void) {
    WSSValue cond = _eval_or();
    (void)cond;
    _expect(TK_LBRACE, "{");
    _exec_block();
    _expect(TK_RBRACE, "}");
    while (_tok.text.len == 4 && !strncmp(_tok.text.text, "elif", 4)) {
        (void)_eval_or();
        _expect(TK_LBRACE, "{"); _exec_block(); _expect(TK_RBRACE, "}");
    }
    if (_tok.text.len == 4 && !strncmp(_tok.text.text, "else", 4)) {
        _adv(); _expect(TK_LBRACE, "{"); _exec_block(); _expect(TK_RBRACE, "}");
    }
    _expect(TK_RBRACE, "endif"); _adv();
}
static void _exec_while(void) {
    size_t loop_top = _S->ip;
    WSSValue cond = _eval_or();
    _expect(TK_LBRACE, "{");
    _exec_block();
    _expect(TK_RBRACE, "}");
    _S->ip = loop_top;
    (void)cond;
}
static void _exec_for(void) {
    char var[256] = {0}, iter[256] = {0};
    memcpy(var, _tok.text.text, _tok.text.len);
    _adv();
    _expect(TK_ASSIGN, "="); _adv();
    memcpy(iter, _tok.text.text, _tok.text.len); _adv();
    _expect(TK_LBRACE, "{"); _exec_block();
    _expect(TK_RBRACE, "}");
    (void)var; (void)iter;
}
static void _exec_fn(const char* name, size_t name_len) {
    _expect(TK_LPAREN, "(");
    char arg1[256] = {0}, arg2[256] = {0};
    if (_tok.text.len > 0 && _tok.text.text[0] != ')') {
        memcpy(arg1, _tok.text.text, _tok.text.len); _adv();
        if (_tok.text.text[0] == ',') { _adv(); memcpy(arg2, _tok.text.text, _tok.text.len); _adv(); }
    }
    _expect(TK_RPAREN, ")");
    _expect(TK_LBRACE, "{"); _exec_block(); _expect(TK_RBRACE, "}");
    WSSValue ret = _eval_or();
    wss_set(_S, name, name_len, ret, false);
    (void)arg1; (void)arg2;
}
static void _exec_block(void) {
    while (_tok.text.len > 0 && _tok.text.text[0] != '}') {
        if (_tok.text.len == 2 && _tok.text.text[0] == 'i' && _tok.text.text[1] == 'f') { _adv(); _exec_if(); continue; }
        if (_tok.text.len == 5 && !strncmp(_tok.text.text, "while", 5)) { _adv(); _exec_while(); continue; }
        if (_tok.text.len == 3 && !strncmp(_tok.text.text, "for", 3)) { _adv(); _exec_for(); continue; }
        if (_tok.text.len == 8 && !strncmp(_tok.text.text, "function", 8)) { _adv();
            char fname[256]; memcpy(fname, _tok.text.text, _tok.text.len); _adv(); _exec_fn(fname, strlen(fname)); continue; }
        if (_tok.text.len == 6 && !strncmp(_tok.text.text, "return", 6)) { _adv(); WSSValue rv = _eval_or(); wss_return(_S, rv); return; }
        if (_tok.text.len == 5 && !strncmp(_tok.text.text, "break", 5)) { _adv(); return; }
        if (_tok.text.len == 8 && !strncmp(_tok.text.text, "continue", 8)) { _adv(); return; }
        if (_tok.text.len == 6 && !strncmp(_tok.text.text, "global", 6)) { _adv();
            char gname[256]; memcpy(gname, _tok.text.text, _tok.text.len); _adv();
            WSSValue v = _eval_or(); wss_set(_S, gname, strlen(gname), v, false); continue; }
        if (_tok.text.len == 5 && !strncmp(_tok.text.text, "local", 5)) { _adv();
            char lname[256]; memcpy(lname, _tok.text.text, _tok.text.len); _adv();
            WSSValue v = _eval_or(); wss_set(_S, lname, strlen(lname), v, true); continue; }
        if (_tok.text.len > 0) {
            char name[256]; memcpy(name, _tok.text.text, _tok.text.len); _adv();
            if (_tok.text.len == 1 && _tok.text.text[0] == '=') { _adv(); WSSValue v = _eval_or(); wss_set(_S, name, strlen(name), v, false); }
            else if (_tok.text.text[0] == '(') {
                WSSValue args[8] = {0};
                int narg = 0;
                _adv();
                if (_tok.text.len > 0 && _tok.text.text[0] != ')') { args[narg++] = _eval_or(); while (_tok.text.len > 0 && _tok.text.text[0] == ',') { _adv(); args[narg++] = _eval_or(); } }
                _expect(TK_RPAREN, ")");
                wss_call(_S, name, strlen(name), args, narg);
            }
        }
    }
}
bool parse_script(const char* src, WSSState* S) {
    _S = S; Lexer L; lex_init(&L, src); _L = &L; _at_eof = false;
    _adv();
    while (!_at_eof && _tok.text.len > 0) {
        if (_tok.text.text[0] == '\n') { _adv(); continue; }
        if (_tok.text.len == 8 && !strncmp(_tok.text.text, "function", 8)) { _adv();
            char fname[256]; memcpy(fname, _tok.text.text, _tok.text.len); _adv(); _exec_fn(fname, strlen(fname)); continue; }
        if (_tok.text.len == 6 && !strncmp(_tok.text.text, "global", 6)) { _adv();
            char gname[256]; memcpy(gname, _tok.text.text, _tok.text.len); _adv();
            WSSValue v = _eval_or(); wss_set(_S, gname, strlen(gname), v, false); continue; }
        char name[256]; memcpy(name, _tok.text.text, _tok.text.len); _adv();
        if (_tok.text.len == 1 && _tok.text.text[0] == '=') { _adv(); WSSValue v = _eval_or(); wss_set(_S, name, strlen(name), v, false); }
        else if (_tok.text.text[0] == '(') {
            WSSValue args[8] = {0};
            int narg = 0;
            _adv();
            if (_tok.text.len > 0 && _tok.text.text[0] != ')') { args[narg++] = _eval_or(); while (_tok.text.len > 0 && _tok.text.text[0] == ',') { _adv(); args[narg++] = _eval_or(); } }
            _expect(TK_RPAREN, ")");
            wss_call(_S, name, strlen(name), args, narg);
        }
    }
    return true;
}
bool parse_file(const char* path, WSSState* S) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    bool ok = parse_script(buf, S);
    free(buf);
    return ok;
}
