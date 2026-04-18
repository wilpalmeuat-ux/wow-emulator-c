#ifndef SCRIPTING_SCRIPTENGINE_H
#define SCRIPTING_SCRIPTENGINE_H

#include <stdint.h>

typedef enum {
    TOKEN_WORD,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_EQUAL,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    int line;
    union {
        double num;
        struct { char* ptr; size_t len; };
    } value;
    char lexeme[64];
} Token;

typedef struct Lexer {
    const char* src;
    size_t len;
    size_t pos;
    int line;
    int col;
    Token cur;
} Lexer;

void lexer_init(Lexer* L, const char* src, size_t len);
int lexer_next(Lexer* L);

typedef enum {
    EXPR_NUMBER,
    EXPR_STRING,
    EXPR_WORD,
    EXPR_BINARY
} ExprType;

typedef struct Expr {
    ExprType type;
    union {
        double num;
        struct { char* ptr; size_t len; } str;
        struct { struct Expr* left; int op; struct Expr* right; } binary;
    } v;
} Expr;

typedef enum {
    STMT_EMPTY,
    STMT_EXPRESSION,
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_PRINT
} StmtType;

typedef struct Stmt {
    StmtType type;
    int line;
    union {
        Expr expr;
        struct { char name[64]; Expr init; } assign;
        struct { Expr cond; struct Stmt* body; } if_;
        struct { Expr cond; struct Stmt* body; } while_;
        struct { char name[64]; char args[8][32]; int argc; struct Stmt* body; } func;
        struct { char name[64]; Expr arg; } print;
    } v;
} Stmt;

typedef struct Parser {
    Lexer* lex;
    Token lookahead;
    int has_lookahead;
    const char* err;
    int err_line;
} Parser;

void parser_init(Parser* P, Lexer* L);
int parser_parse(P, Stmt** out);
const char* parser_error(P);

typedef struct ScriptEngine ScriptEngine;
typedef struct VM VM;

ScriptEngine* script_engine_create(void);
void script_engine_destroy(ScriptEngine* E);
int script_engine_load(ScriptEngine* E, const char* name, const char* src, size_t len);
int script_engine_exec(ScriptEngine* E, const char* name, const char* code, size_t len);
int script_engine_call(ScriptEngine* E, const char* func, const char* args);
void script_engine_register_cfunc(ScriptEngine* E, const char* name, void* ctx, int (*fn)(void*, Expr*));
const char* script_engine_error(ScriptEngine* E);

#endif
