/* wss_lexer.c — Lexer for WoW WSS scripting language */
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

void WssToken_Init(WssToken* t, WssLexTokenType type, int line) {
    t->type = type;
    t->lexeme[0] = '\0';
    t->line = line;
    t->col = 0;
}

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
        WssToken_Init(out, TK_EOF, L->line);
        return false;
    }

    int c = _cur(L);

    if (c == '\n') {
        L->pos++;
        L->line++;
        WssToken_Init(out, TK_NEWLINE, L->line);
        out->lexeme[0] = '\n'; out->lexeme[1] = '\0';
        return true;
    }

    if (isspace(c)) { _skip_ws(L); return lex_next(L, out); }

    if (c == '/' && _nxt(L) == '/') {
        const char* s = L->src + L->pos;
        while (L->pos < L->len && _cur(L) != '\n') L->pos++;
        WssToken_Init(out, TK_COMMENT, L->line);
        int len = (int)(L->src + L->pos - s);
        if (len > 127) len = 127;
        memcpy(out->lexeme, s, len);
        out->lexeme[len] = '\0';
        return true;
    }

    if (c == '"' || c == '\'') {
        int delim = c;
        const char* s = L->src + L->pos;
        _adv(L);
        while (L->pos < L->len && _cur(L) != delim && _cur(L) != '\n') _adv(L);
        if (_cur(L) == delim) _adv(L);
        WssToken_Init(out, TK_STRING, L->line);
        int len = (int)(L->src + L->pos - s);
        if (len > 127) len = 127;
        memcpy(out->lexeme, s, len);
        out->lexeme[len] = '\0';
        return true;
    }

    if ((c >= '0' && c <= '9') || (c == '.' && _nxt(L) >= '0' && _nxt(L) <= '9')) {
        const char* s = L->src + L->pos;
        int has_dot = 0;
        while (_cur(L) >= '0' && _cur(L) <= '9' || (_cur(L) == '.' && !has_dot)) {
            if (_cur(L) == '.') has_dot = 1;
            _adv(L);
        }
        WssToken_Init(out, has_dot ? TK_FLOAT : TK_NUMBER, L->line);
        int len = (int)(L->src + L->pos - s);
        if (len > 127) len = 127;
        memcpy(out->lexeme, s, len);
        out->lexeme[len] = '\0';
        return true;
    }

    if (_is_alpha(c) || c == '_') {
        const char* s = L->src + L->pos;
        while (_is_alnum(_cur(L))) _adv(L);
        int len = (int)(L->src + L->pos - s);
        WssLexTokenType tt = _kw(s, len);
        WssToken_Init(out, tt, L->line);
        if (len > 127) len = 127;
        memcpy(out->lexeme, s, len);
        out->lexeme[len] = '\0';
        return true;
    }

    _adv(L);
    WssToken_Init(out, TK_ERROR, L->line);
    out->lexeme[0] = (char)c; out->lexeme[1] = '\0';

    if (c == '{') out->type = TK_LBRACE;
    else if (c == '}') out->type = TK_RBRACE;
    else if (c == '(') out->type = TK_LPAREN;
    else if (c == ')') out->type = TK_RPAREN;
    else if (c == '[') out->type = TK_LBRACKET;
    else if (c == ']') out->type = TK_RBRACKET;
    else if (c == ',') out->type = TK_COMMA;
    else if (c == '.') out->type = TK_DOT;
    else if (c == ':') out->type = TK_COLON;
    else if (c == ';') out->type = TK_SEMICOLON;
    else if (c == '|') out->type = TK_PIPE;
    else if (c == '&') out->type = TK_AMP;
    else if (c == '@') out->type = TK_AT;
    else if (c == '#') out->type = TK_HASH;
    else if (c == '$') out->type = TK_DOLLAR;
    else if (c == '+') out->type = TK_PLUS;
    else if (c == '*') out->type = TK_STAR;
    else if (c == '/') out->type = TK_SLASH;
    else if (c == '%') out->type = TK_PERCENT;
    else if (c == '^') out->type = TK_XOR;
    else if (c == '-') {
        if (_cur(L) == '>') { _adv(L); out->type = TK_ARROW; out->lexeme[0] = '-'; out->lexeme[1] = '>'; out->lexeme[2] = '\0'; }
        else out->type = TK_MINUS;
    } else if (c == '=') {
        if (_cur(L) == '=') { _adv(L); out->type = TK_EQ; out->lexeme[0] = '='; out->lexeme[1] = '='; out->lexeme[2] = '\0'; }
        else out->type = TK_ASSIGN;
    } else if (c == '!') {
        if (_cur(L) == '=') { _adv(L); out->type = TK_NEQ; out->lexeme[0] = '!'; out->lexeme[1] = '='; out->lexeme[2] = '\0'; }
    } else if (c == '<') {
        if (_cur(L) == '=') { _adv(L); out->type = TK_LE; out->lexeme[0] = '<'; out->lexeme[1] = '='; out->lexeme[2] = '\0'; }
        else out->type = TK_LT;
    } else if (c == '>') {
        if (_cur(L) == '=') { _adv(L); out->type = TK_GE; out->lexeme[0] = '>'; out->lexeme[1] = '='; out->lexeme[2] = '\0'; }
        else out->type = TK_GT;
    }

    return true;
}
