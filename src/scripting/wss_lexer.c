#include "scripting/wss_lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static const char* _tok_names[] = {
    "EOF","ERROR","IDENT","STRING","NUMBER","FLOAT",
    "LBRACE","RBRACE","LPAREN","RPAREN","LBRACKET","RBRACKET",
    "COMMA","DOT","COLON","SEMICOLON","PIPE","AMP",
    "EQ","NEQ","LT","LE","GT","GE","ASSIGN",
    "PLUS","MINUS","STAR","SLASH","PERCENT",
    "AND","OR","NOT","XOR",
    "IF","ELSE","ELIF","ENDIF",
    "WHILE","ENDWHILE","FOR","ENDFOR","IN",
    "RETURN","BREAK","CONTINUE","FUNCTION",
    "TRUE","FALSE","NIL",
    "GLOBAL","LOCAL",
    "AT","HASH","DOLLAR",
    "ARROW","DEFINE",
    "NEWLINE","COMMENT"
};

const char* tok_name(WssLexTokenType t) { return _tok_names[t]; }

static int _is_alpha(int c) { return isalpha(c) || c == '_'; }
static int _is_alnum(int c) { return isalnum(c) || c == '_'; }
static int _cur(Lexer* L) { return L->pos < L->len ? L->src[L->pos] : 0; }
static int _nxt(Lexer* L) { return L->pos + 1 < L->len ? L->src[L->pos + 1] : 0; }
static void _adv(Lexer* L) { if (L->pos < L->len) L->pos++; }
static void _skip_ws(Lexer* L) {
    while (L->pos < L->len && isspace(L->src[L->pos]) && L->src[L->pos] != '\n') L->pos++;
}

static WssLexTokenType _kw(const char* s, int len) {
    if (len == 2 && strncmp(s, "if", 2) == 0) return TK_IF;
    if (len == 4) {
        if (strncmp(s, "else", 4) == 0) return TK_ELSE;
        if (strncmp(s, "elif", 4) == 0) return TK_ELIF;
    }
    if (len == 5) {
        if (strncmp(s, "endif", 5) == 0) return TK_ENDIF;
        if (strncmp(s, "while", 5) == 0) return TK_WHILE;
        if (strncmp(s, "local", 5) == 0) return TK_LOCAL;
        if (strncmp(s, "false", 5) == 0) return TK_FALSE;
    }
    if (len == 6) {
        if (strncmp(s, "global", 6) == 0) return TK_GLOBAL;
        if (strncmp(s, "return", 6) == 0) return TK_RETURN;
        if (strncmp(s, "break", 5) == 0) return TK_BREAK;
    }
    if (len == 7) {
        if (strncmp(s, "endwhile", 7) == 0) return TK_ENDWHILE;
        if (strncmp(s, "continue", 8) == 0) return TK_CONTINUE;
    }
    if (len == 8) {
        if (strncmp(s, "function", 8) == 0) return TK_FUNCTION;
    }
    if (len == 3) {
        if (strncmp(s, "for", 3) == 0) return TK_FOR;
        if (strncmp(s, "nil", 3) == 0) return TK_NIL;
    }
    if (len == 4 && strncmp(s, "true", 4) == 0) return TK_TRUE;
    return TK_IDENT;
}

void lex_init(Lexer* L, const char* src) {
    memset(L, 0, sizeof(*L));
    L->src = src;
    L->len = strlen(src);
    L->line = 1;
    L->pos = 0;
}

bool lex_next(Lexer* L, Token* out) {
    if (L->pos >= L->len) {
        out->text.text = "";
        out->text.len = 0;
        out->len = 0;
        return false;
    }
    const char* start = L->src + L->pos;
    int c = _cur(L);

    if (c == '\n') {
        L->pos++;
        L->line++;
        out->text.text = start;
        out->text.len = 1;
        out->len = 1;
        return true;
    }

    if (isspace(c)) { _skip_ws(L); return lex_next(L, out); }

    if (c == '/' && _nxt(L) == '/') {
        const char* s = L->src + L->pos;
        while (L->pos < L->len && _cur(L) != '\n') L->pos++;
        out->text.text = s;
        out->text.len = L->src + L->pos - s;
        out->len = out->text.len;
        return true;
    }

    if (c == '"' || c == '\'') {
        int delim = c;
        const char* s = L->src + L->pos;
        _adv(L);
        while (L->pos < L->len && _cur(L) != delim && _cur(L) != '\n') _adv(L);
        const char* e = L->src + L->pos;
        if (_cur(L) == delim) _adv(L);
        out->text.text = s;
        out->text.len = L->src + L->pos - s;
        out->len = out->text.len;
        return true;
    }

    if ((c >= '0' && c <= '9') || (c == '.' && _nxt(L) >= '0' && _nxt(L) <= '9')) {
        const char* s = L->src + L->pos;
        int has_dot = 0;
        while (_cur(L) >= '0' && _cur(L) <= '9' || (_cur(L) == '.' && !has_dot)) {
            if (_cur(L) == '.') has_dot = 1;
            _adv(L);
        }
        out->text.text = s;
        out->text.len = L->src + L->pos - s;
        out->len = out->text.len;
        return true;
    }

    if (_is_alpha(c) || c == '_') {
        const char* s = L->src + L->pos;
        while (_is_alnum(_cur(L))) _adv(L);
        int len = L->src + L->pos - s;
        WssLexTokenType tt = _kw(s, len);
        out->text.text = s;
        out->text.len = len;
        out->len = len;
        return true;
    }

    _adv(L);
    out->text.text = start;
    out->text.len = 1;
    out->len = 1;

    if (c == '{') return true;
    if (c == '}') return true;
    if (c == '(') return true;
    if (c == ')') return true;
    if (c == '[') return true;
    if (c == ']') return true;
    if (c == ',') return true;
    if (c == '.') return true;
    if (c == ':') return true;
    if (c == ';') return true;
    if (c == '|') return true;
    if (c == '&') return true;
    if (c == '@') return true;
    if (c == '#') return true;
    if (c == '$') return true;
    if (c == '+') return true;
    if (c == '-') { if (_cur(L) == '>') { out->text.text = start; out->text.len = 2; out->len = 2; _adv(L); } return true; }
    if (c == '*') return true;
    if (c == '/') return true;
    if (c == '%') return true;
    if (c == '^') return true;
    if (c == '=') { if (_cur(L) == '=') { out->text.text = start; out->text.len = 2; out->len = 2; _adv(L); } return true; }
    if (c == '!') { if (_cur(L) == '=') { out->text.text = start; out->text.len = 2; out->len = 2; _adv(L); } return true; }
    if (c == '<') { if (_cur(L) == '=') { out->text.text = start; out->text.len = 2; out->len = 2; _adv(L); } return true; }
    if (c == '>') { if (_cur(L) == '=') { out->text.text = start; out->text.len = 2; out->len = 2; _adv(L); } return true; }

    return true;
}
