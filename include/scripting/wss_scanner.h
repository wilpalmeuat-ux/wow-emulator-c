/* ╔══════════════════════════════════════════════════════════════╗
   ║  Scanner / Lexer — tokenises a .wss script into a token     ║
   ║  stream.  Plain-English word-based scripting language.       ║
   ╚══════════════════════════════════════════════════════════════╝ */

#ifndef WSS_SCANNER_H
#define WSS_SCANNER_H

#include <stdint.h>
#include <stdbool.h>
#include <shared/Types.h>

/* ── Token types ─────────────────────────────────────────────── */
typedef enum {
    T_EOF       = 0,
    T_WORD,          /* bare identifier word                     */
    T_STRING,        /* "double-quoted string"                   */
    T_INT,           /* integer literal  -123                    */
    T_FLOAT,         /* floating literal  3.14                   */
    T_LBRACE,        /* {                                        */
    T_RBRACE,        /* }                                        */
    T_LBRACKET,      /* [                                        */
    T_RBRACKET,      /* ]                                        */
    T_LPAREN,        /* (                                        */
    T_RPAREN,        /* )                                        */
    T_SEMICOLON,     /* ;                                        */
    T_COLON,         /* :  label target or key:value             */
    T_COMMA,         /* ,                                        */
    T_DOT,           /* .  property accessor                     */
    T_PIPE,          /* |  alternate / OR in condition           */
    T_EQ,            /* =  assignment or equality                */
    T_NEQ,           /* !=                                       */
    T_LT,            /* <  less-than                             */
    T_GT,            /* >  greater-than                          */
    T_LTE,           /* <=                                       */
    T_GTE,           /* >=                                       */
    T_PLUS,          /* +                                        */
    T_MINUS,         /* -                                        */
    T_STAR,          /* *                                        */
    T_SLASH,         /* /  comment or division                   */
    T_MOD,           /* %  modulo                                */
    T_BANG,          /* !  not                                   */
    T_KW_EVENT,      /* on_event                                 */
    T_KW_IF,         /* if                                       */
    T_KW_ELIF,       /* else_if / elif                           */
    T_KW_ELSE,       /* else                                     */
    T_KW_END,        /* end                                      */
    T_KW_SET,        /* set                                      */
    T_KW_GIVE,       /* give                                     */
    T_KW_TAKE,       /* take                                     */
    T_KW_SAY,        /* say / broadcast                           */
    T_KW_WHISPER,    /* whisper                                  */
    T_KW_TELEPORT,   /* teleport                                 */
    T_KW_SPAWN,      /* spawn                                    */
    T_KW_DESPAWN,    /* despawn                                  */
    T_KW_SETFLAG,    /* set_flag                                 */
    T_KW_CLEARFLAG,  /* clear_flag                               */
    T_KW_WAIT,       /* wait                                     */
    T_KW_TIMER,      /* timer                                    */
    T_KW_BROADCAST,  /* broadcast                                */
    T_KW_COUNTDOWN,  /* countdown                                */
    T_KW_FOR,        /* for                                      */
    T_TOTAL_KEYWORDS /* not a real token — sentinel              */
} WssTokenType;

/* ── Token structure ────────────────────────────────────────────── */
typedef struct {
    WssTokenType type;
    int         line;
    int         col;
    union {
        int64_t   i;
        double    f;
        char*     str;   /* owned for T_WORD (keyword or ident), T_STRING */
    } data;
} WssToken;

void WssToken_Init(WssToken* t, WssTokenType type, int line);
void WssToken_Delete(WssToken* t);

/* ── Scanner object ────────────────────────────────────────────── */
typedef struct WssScanner {
    const char* src;
    int        len;
    int        pos;       /* current byte offset in src            */
    int        line;
    int        col;
    WssToken   peek_tok;
    bool       has_peek;
} WssScanner;

void  WssScanner_Init(WssScanner* s, const char* src, int len);
void  WssScanner_Delete(WssScanner* s);
bool  WssScanner_Next(WssScanner* s, WssToken* out);
bool  WssScanner_Peek(WssScanner* s, WssToken* out);
bool  WssScanner_Check(WssScanner* s, WssTokenType expected);
bool  WssScanner_Match(WssScanner* s, WssTokenType expected);
void  WssScanner_SkipWhitespace(WssScanner* s);
void  WssScanner_SkipComment(WssScanner* s);
bool  WssScanner_IsWordChar(uint8 c);
bool  WssScanner_IsDigit(uint8 c);
bool  WssScanner_IsWhiteSpace(uint8 c);

#endif /* WSS_SCANNER_H */
