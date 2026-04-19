/* ╔══════════════════════════════════════════════════════════════╗
   ║  Scanner / Lexer — tokenises a .wss script into a token     ║
   ║  stream.  Plain-English word-based scripting language.       ║
   ╚══════════════════════════════════════════════════════════════╝ */

#ifndef WSS_SCANNER_H
#define WSS_SCANNER_H
#include <scripting/wss_lexer.h>  /* use unified lexer; T_xxx are aliases */
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
#define T_COLON     TK_COLON
#define T_COMMA     TK_COMMA
#define T_DOT       TK_DOT
#define T_PIPE      TK_PIPE
#define T_AMP       TK_AMP
#define T_EQ        TK_EQ
#define T_NEQ       TK_NEQ
#define T_LT        TK_LT
#define T_LE        TK_LE
#define T_GT        TK_GT
#define T_GE        TK_GE
#define T_ASSIGN    TK_ASSIGN
#define T_PLUS      TK_PLUS
#define T_MINUS     TK_MINUS
#define T_STAR      TK_STAR
#define T_SLASH     TK_SLASH
#define T_PERCENT   TK_PERCENT
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
#define T_NEWLINE   TK_NEWLINE
#define T_AT        TK_AT
#define T_HASH      TK_HASH
#define T_DOLLAR    TK_DOLLAR
#define T_ARROW     TK_ARROW
typedef Lexer Scanner;
typedef Token ScannerToken;
#define scanner_init(L, src)  lex_init((L), (src))
#define scanner_next(L, out)   lex_next((L), (out))
#define tok_name(t)            tok_name((WssLexTokenType)(t))
#endif /* WSS_SCANNER_H */
