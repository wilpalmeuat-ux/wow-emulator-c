/* WSS Scanner — tokenises .wss scripts into a token stream */
#ifndef WSS_SCANNER_H
#define WSS_SCANNER_H

#include <scripting/wss_lexer.h>
#include <stdint.h>

/* Ensure TokenType / WssToken are defined (may already be from wss_lexer.h) */
#ifndef TokenType
typedef WssLexTokenType TokenType;
#endif
#ifndef WssToken
typedef Token WssToken;
#endif

/* ── Scanner (tokeniser) ── */
typedef struct WssScanner {
    const char* src;
    size_t len;
    size_t pos;
    int line;
    int col;
    bool has_peek;
    WssToken peek_tok;
} WssScanner;

void WssScanner_Init(WssScanner* s, const char* src, size_t len);
void WssScanner_Delete(WssScanner* s);
bool WssScanner_Next(WssScanner* s, WssToken* out);
bool WssScanner_Peek(WssScanner* s, WssToken* out);
bool WssScanner_Check(WssScanner* s, TokenType expected);
bool WssScanner_Match(WssScanner* s, TokenType expected);

bool WssScanner_IsWordChar(uint8_t c);
bool WssScanner_IsDigit(uint8_t c);
bool WssScanner_IsWhiteSpace(uint8_t c);
void WssScanner_SkipWhitespace(WssScanner* s);
void WssScanner_SkipComment(WssScanner* s);

/* ── Token type aliases for the WSS parser/compiler ── */
#define T_EOF       TK_EOF
#define T_WORD      TK_IDENT
#define T_STRING    TK_STRING
#define T_INT       TK_NUMBER
#define T_FLOAT     TK_FLOAT
#define T_LBRACE    TK_LBRACE
#define T_RBRACE    TK_RBRACE
#define T_LBRACKET  TK_LBRACKET
#define T_RBRACKET  TK_RBRACKET
#define T_LPAREN    TK_LPAREN
#define T_RPAREN    TK_RPAREN
#define T_SEMICOLON TK_SEMICOLON
#define T_COMMA     TK_COMMA
#define T_DOT       TK_DOT
#define T_COLON     TK_COLON
#define T_PLUS      TK_PLUS
#define T_MINUS     TK_MINUS
#define T_STAR      TK_STAR
#define T_SLASH     TK_SLASH
#define T_PERCENT   TK_PERCENT
#define T_PIPE      TK_PIPE
#define T_AMP       TK_AMP
#define T_EQ        TK_EQ
#define T_NEQ       TK_NEQ
#define T_LT        TK_LT
#define T_GT        TK_GT
#define T_LE        TK_LE
#define T_GE        TK_GE
#define T_ASSIGN    TK_ASSIGN
#define T_IF        TK_IF
#define T_ELSE      TK_ELSE
#define T_ELIF      TK_ELIF
#define T_ENDIF     TK_ENDIF
#define T_WHILE     TK_WHILE
#define T_ENDWHILE  TK_ENDWHILE
#define T_FOR       TK_FOR
#define T_ENDFOR    TK_ENDFOR
#define T_IN        TK_IN
#define T_RETURN    TK_RETURN
#define T_BREAK     TK_BREAK
#define T_CONTINUE  TK_CONTINUE
#define T_FUNCTION  TK_FUNCTION
#define T_TRUE      TK_TRUE
#define T_FALSE     TK_FALSE
#define T_NIL       TK_NIL
#define T_GLOBAL    TK_GLOBAL
#define T_LOCAL     TK_LOCAL
#define T_ARROW     TK_ARROW
#define T_COMMENT   TK_COMMENT
#define T_NEWLINE   TK_NEWLINE
#define T_ERROR     TK_ERROR
#define T_HASH      TK_HASH
#define T_AT        TK_AT
#define T_DOLLAR    TK_DOLLAR
#define T_DEFINE    TK_DEFINE
#define T_NOT       TK_NOT
#define T_OR        TK_OR
#define T_XOR       TK_XOR

/* ── Backward compat: old scanner used T_KW_xxx style ── */
#define T_KW_IF       TK_IF
#define T_KW_ELSE     TK_ELSE
#define T_KW_ELIF     TK_ELIF
#define T_KW_WHILE    TK_WHILE
#define T_KW_FOR      TK_FOR
#define T_KW_RETURN   TK_RETURN
#define T_KW_TRUE     TK_TRUE
#define T_KW_FALSE    TK_FALSE
#define T_KW_NIL      TK_NIL
#define T_KW_FUNCTION TK_FUNCTION
#define T_KW_END      TK_EOF
#define T_KW_BREAK    TK_BREAK
#define T_KW_CONTINUE TK_CONTINUE
#define T_KW_LOCAL    TK_LOCAL
#define T_KW_GLOBAL   TK_GLOBAL
#define T_KW_IN       TK_IN
#define T_KW_SET      TK_EOF
#define T_KW_GIVE     TK_EOF
#define T_KW_TAKE     TK_EOF
#define T_KW_SAY      TK_EOF
#define T_KW_WHISPER  TK_EOF
#define T_KW_TELEPORT TK_EOF
#define T_KW_SPAWN    TK_EOF
#define T_KW_DESPAWN  TK_EOF
#define T_KW_SETFLAG  TK_EOF
#define T_KW_CLEARFLAG TK_EOF
#define T_KW_WAIT     TK_EOF
#define T_KW_TIMER    TK_EOF
#define T_KW_BROADCAST TK_EOF
#define T_KW_COUNTDOWN TK_EOF
#define T_KW_EVENT    TK_EOF

#define T_IDENT     TK_IDENT
#define T_NUMBER    TK_NUMBER
#define T_STR       TK_STRING
#define T_DBL       TK_FLOAT
#define T_NULL      TK_NIL
#define T_SAY       TK_EOF
#define T_WHISPER   TK_EOF
#define T_BROADCAST TK_EOF

#endif /* WSS_SCANNER_H */
