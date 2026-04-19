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
#define T_EOF_      TK_EOF
#define T_ERROR     TK_ERROR
#define T_HASH      TK_HASH
#define T_AT        TK_AT
#define T_DOLLAR    TK_DOLLAR
#define T_DEFINE    TK_DEFINE
#define T_NOT       TK_NOT
#define T_OR        TK_OR
#define T_XOR       TK_XOR
#define T_LBRACKET  TK_LBRACKET
#define T_RBRACKET  TK_RBRACKET
/* Backward compat for old scanner using T_KW_xxx style */
#define T_KW_IF     TK_IF
#define T_KW_ELSE   TK_ELSE
#define T_KW_WHILE  TK_WHILE
#define T_KW_FOR    TK_FOR
#define T_KW_RETURN TK_RETURN
#define T_KW_TRUE   TK_TRUE
#define T_KW_FALSE  TK_FALSE
#define T_KW_NIL    TK_NIL
#define T_KW_FUNCTION TK_FUNCTION
#define T_KW_END    TK_EOF
#define T_KW_BREAK  TK_BREAK
#define T_KW_CONTINUE TK_CONTINUE
#define T_KW_LOCAL  TK_LOCAL
#define T_KW_GLOBAL TK_GLOBAL
#define T_KW_IN     TK_IN
#define T_KW_VAR    TK_EOF
#define T_KW_CONST  TK_EOF
#define T_KW_NEW    TK_EOF
#define T_IDENT     TK_IDENT
#define T_NUMBER    TK_NUMBER
#define T_STR       TK_STRING
#define T_DBL       TK_FLOAT
#define T_NULL      TK_NIL
#define T_SAY       TK_EOF
#define T_WHISPER   TK_EOF
#define T_BROADCAST TK_EOF
#define T_GLOBALMSG TK_EOF
#define T_ZONEMSG   TK_EOF
#define T_YELL      TK_EOF
#endif /* WSS_SCANNER_H */
