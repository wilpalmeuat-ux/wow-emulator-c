/* ╔══════════════════════════════════════════════════════════════╗
   ║  Scanner implementation — tokenises .wss source text         ║
   ╚══════════════════════════════════════════════════════════════╝ */

#include <scripting/wss_scanner.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ── Keyword lookup ───────────────────────────────────────────── */
static bool is_keyword(const char* word) {
    static const char* kw[] = {
        "on", "when", "if", "else_if", "else", "end",
        "set", "give", "take", "say", "whisper", "broadcast",
        "teleport", "spawn", "despawn", "set_flag", "clear_flag",
        "wait", "timer", "countdown", "for", "not", "and", "or",
        "true", "false", "null"
    };
    for (size_t i = 0; i < sizeof(kw)/sizeof(*kw); i++)
        if (strcmp(word, kw[i]) == 0) return true;
    return false;
}

static WssTokenType kw_to_token(const char* word) {
    if (strcmp(word, "on")==0 || strcmp(word,"when")==0)    return T_KW_EVENT;
    if (strcmp(word, "if")==0)                              return T_KW_IF;
    if (strcmp(word, "else_if")==0 || strcmp(word,"elif")==0) return T_KW_ELIF;
    if (strcmp(word, "else")==0)                            return T_KW_ELSE;
    if (strcmp(word, "end")==0)                             return T_KW_END;
    if (strcmp(word, "set")==0)                             return T_KW_SET;
    if (strcmp(word, "give")==0)                            return T_KW_GIVE;
    if (strcmp(word, "take")==0)                            return T_KW_TAKE;
    if (strcmp(word, "say")==0)                             return T_KW_SAY;
    if (strcmp(word, "whisper")==0)                         return T_KW_WHISPER;
    if (strcmp(word, "teleport")==0)                        return T_KW_TELEPORT;
    if (strcmp(word, "spawn")==0)                           return T_KW_SPAWN;
    if (strcmp(word, "despawn")==0)                          return T_KW_DESPAWN;
    if (strcmp(word, "set_flag")==0)                        return T_KW_SETFLAG;
    if (strcmp(word, "clear_flag")==0)                      return T_KW_CLEARFLAG;
    if (strcmp(word, "wait")==0)                             return T_KW_WAIT;
    if (strcmp(word, "timer")==0)                            return T_KW_TIMER;
    if (strcmp(word, "broadcast")==0)                        return T_KW_BROADCAST;
    if (strcmp(word, "countdown")==0)                        return T_KW_COUNTDOWN;
    if (strcmp(word, "for")==0)                             return T_KW_FOR;
    return T_WORD;
}

void WssToken_Init(WssToken* t, WssTokenType type, int line) {
    memset(t, 0, sizeof(*t));
    t->type = type;
    t->line = line;
}

void WssToken_Delete(WssToken* t) {
    if (t->type == T_WORD || t->type == T_STRING) free(t->data.str);
    memset(t, 0, sizeof(*t));
}

bool WssScanner_IsWordChar(uint8 c) {
    return isalpha(c) || c == '_' || (c & 0x80);
}
bool WssScanner_IsDigit(uint8 c) {
    return isdigit(c);
}
bool WssScanner_IsWhiteSpace(uint8 c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void WssScanner_SkipWhitespace(WssScanner* s) {
    while (s->pos < s->len && WssScanner_IsWhiteSpace((uint8)s->src[s->pos]))
        if (s->src[s->pos++] == '\n') { s->line++; s->col = 0; }
}

void WssScanner_SkipComment(WssScanner* s) {
    if (s->src[s->pos] == '/' && s->pos+1 < s->len && s->src[s->pos+1] == '/') {
        while (s->pos < s->len && s->src[s->pos] != '\n')
            s->pos++;
    }
}

void WssScanner_Init(WssScanner* s, const char* src, int len) {
    memset(s, 0, sizeof(*s));
    s->src = src;
    s->len = len;
    s->pos = 0;
    s->line = 1;
    s->col = 0;
    s->has_peek = false;
}

void WssScanner_Delete(WssScanner* s) {
    (void)s;
}

static uint8 _cur(const WssScanner* s) {
    if (s->pos < s->len) return (uint8)s->src[s->pos];
    return 0;
}
static uint8 _peek(const WssScanner* s) {
    if (s->pos + 1 < s->len) return (uint8)s->src[s->pos + 1];
    return 0;
}
static void _adv(WssScanner* s) {
    s->pos++;
    s->col++;
}

bool WssScanner_Next(WssScanner* s, WssToken* out) {
    if (s->has_peek) {
        *out = s->peek_tok;
        s->has_peek = false;
        return out->type != T_EOF;
    }

    WssToken_Init(out, T_EOF, s->line);
    WssScanner_SkipWhitespace(s);
    _adv(s);

    switch (_cur(s)) {
        case 0:
            out->type = T_EOF;
            break;
        case '{': out->type = T_LBRACE; break;
        case '}': out->type = T_RBRACE; break;
        case '[': out->type = T_LBRACKET; break;
        case ']': out->type = T_RBRACKET; break;
        case '(': out->type = T_LPAREN; break;
        case ')': out->type = T_RPAREN; break;
        case ';': out->type = T_SEMICOLON; break;
        case ':': out->type = T_COLON; break;
        case ',': out->type = T_COMMA; break;
        case '.': out->type = T_DOT; break;
        case '|': out->type = T_PIPE; break;
        case '+': out->type = T_PLUS; break;
        case '*': out->type = T_STAR; break;
        case '%': out->type = T_MOD; break;
        case '/':
            _adv(s);
            if (_cur(s) == '/') {
                while (s->pos < s->len && s->src[s->pos] != '\n') _adv(s);
                return WssScanner_Next(s, out);
            }
            out->type = T_SLASH;
            break;
        case '-':
            if (isdigit(_peek(s))) {
                int start = s->pos;
                char buf[32];
                int i = 0;
                buf[i++] = '-'; _adv(s);
                while (s->pos < s->len && isdigit((uint8)s->src[s->pos]) && i < 31)
                    buf[i++] = s->src[s->pos], _adv(s);
                buf[i] = 0;
                out->type = T_INT;
                out->data.i = strtoll(buf, NULL, 10);
            } else
                out->type = T_MINUS;
            break;
        case '!':
            _adv(s);
            if (_cur(s) == '=') { out->type = T_NEQ; _adv(s); }
            else out->type = T_BANG;
            break;
        case '=':
            _adv(s);
            if (_cur(s) == '=') { out->type = T_EQ; _adv(s); }
            else out->type = T_EQ; // single = is both assignment and equality
            break;
        case '<':
            _adv(s);
            if (_cur(s) == '=') { out->type = T_LTE; _adv(s); }
            else out->type = T_LT;
            break;
        case '>':
            _adv(s);
            if (_cur(s) == '=') { out->type = T_GTE; _adv(s); }
            else out->type = T_GT;
            break;
        case '"': {
            int start = s->pos; // include opening quote
            _adv(s); // skip opening "
            char buf[1024];
            int i = 0;
            while (s->pos < s->len && s->src[s->pos] != '"') {
                if (s->src[s->pos] == '\\' && s->pos+1 < s->len) {
                    _adv(s);
                    char esc = s->src[s->pos];
                    switch (esc) {
                        case 'n': buf[i++] = '\n'; break;
                        case 't': buf[i++] = '\t'; break;
                        case '"': buf[i++] = '"'; break;
                        case '\\': buf[i++] = '\\'; break;
                        default: buf[i++] = esc; break;
                    }
                } else
                    buf[i++] = s->src[s->pos];
                _adv(s);
            }
            _adv(s); // skip closing "
            buf[i] = 0;
            out->type = T_STRING;
            out->data.str = strndup(buf, i);
            break;
        }
        default:
            if (isdigit(_cur(s))) {
                int start = s->pos;
                char buf[32];
                int i = 0;
                bool is_float = false;
                while (s->pos < s->len && (isdigit((uint8)s->src[s->pos]) || s->src[s->pos]=='.')) {
                    if (s->src[s->pos]=='.') {
                        if (is_float) break;
                        is_float = true;
                    }
                    buf[i++] = s->src[s->pos];
                    _adv(s);
                }
                buf[i] = 0;
                if (is_float) {
                    out->type = T_FLOAT;
                    out->data.f = strtod(buf, NULL);
                } else {
                    out->type = T_INT;
                    out->data.i = strtoll(buf, NULL, 10);
                }
            } else if (WssScanner_IsWordChar(_cur(s))) {
                int start = s->pos;
                char buf[128];
                int i = 0;
                while (s->pos < s->len && WssScanner_IsWordChar((uint8)s->src[s->pos]) && i < 127)
                    buf[i++] = s->src[s->pos++];
                buf[i] = 0;
                s->col += i;
                out->col = s->col - i + 1;
                if (is_keyword(buf))
                    out->type = kw_to_token(buf);
                else
                    out->type = T_WORD, out->data.str = strdup(buf);
            } else {
                // Unknown char — treat as word so error recovery is gentle
                char buf[2] = { (char)_cur(s), 0 };
                out->type = T_WORD;
                out->data.str = strdup(buf);
            }
            break;
    }
    return out->type != T_EOF;
}

bool WssScanner_Peek(WssScanner* s, WssToken* out) {
    if (!s->has_peek) {
        WssToken t;
        WssScanner_Next(s, &t);
        s->peek_tok = t;
        s->has_peek = true;
    }
    *out = s->peek_tok;
    return out->type != T_EOF;
}

bool WssScanner_Check(WssScanner* s, WssTokenType expected) {
    WssToken t;
    if (!WssScanner_Peek(s, &t)) return false;
    return t.type == expected;
}

bool WssScanner_Match(WssScanner* s, WssTokenType expected) {
    if (!WssScanner_Check(s, expected)) return false;
    s->has_peek = false;
    return true;
}
