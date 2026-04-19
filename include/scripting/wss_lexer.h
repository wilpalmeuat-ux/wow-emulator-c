#ifndef WSS_LEXER_H
#define WSS_LEXER_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef enum {
    TK_EOF=0, TK_ERROR, TK_IDENT, TK_STRING, TK_NUMBER, TK_FLOAT,
    TK_LBRACE, TK_RBRACE, TK_LPAREN, TK_RPAREN,
    TK_LBRACKET, TK_RBRACKET, TK_COMMA, TK_DOT,
    TK_COLON, TK_SEMICOLON, TK_PIPE, TK_AMP,
    TK_EQ, TK_NEQ, TK_LT, TK_LE, TK_GT, TK_GE, TK_ASSIGN,
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
    TK_AND, TK_OR, TK_NOT, TK_XOR,
    TK_IF, TK_ELSE, TK_ELIF, TK_ENDIF,
    TK_WHILE, TK_ENDWHILE, TK_FOR, TK_ENDFOR, TK_IN,
    TK_RETURN, TK_BREAK, TK_CONTINUE, TK_FUNCTION,
    TK_TRUE, TK_FALSE, TK_NIL, TK_GLOBAL, TK_LOCAL,
    TK_AT, TK_HASH, TK_DOLLAR, TK_ARROW, TK_DEFINE,
    TK_NEWLINE, TK_COMMENT
} WssLexTokenType;

/* backward compat */
typedef WssLexTokenType TokenType;

typedef struct { const char* text; int len; } StrView;
typedef struct { StrView text; int len; } Token;

typedef struct {
    const char* src;
    size_t pos;
    size_t len;
    int line;
    StrView cur;
} Lexer;

void        lex_init(Lexer* L, const char* src);
bool        lex_next(Lexer* L, Token* out);
const char* tok_name(WssLexTokenType t);

#endif
