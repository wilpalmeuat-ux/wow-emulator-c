/*
 * WSS — Word/Statement Scripting System
 * A statement-based scripting engine where English-like words and statements
 * drive the entire emulator behavior.
 *
 * Syntax overview:
 *   SET variable TO value
 *   IF condition THEN statement
 *   WHEN event DO statements
 *   CREATE object WITH properties
 *   SEND packet TO target
 *   FOR each IN collection DO statements
 *   WHILE condition DO statements
 *   FUNCTION name(params) DO statements END
 *   RETURN value
 *   PRINT message
 *   SPAWN unit AT position
 *   MOVE target TO destination
 *   CAST spell ON target
 */

#ifndef WSS_SCRIPTING_H
#define WSS_SCRIPTING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* — Value union — */
typedef enum {
    VAL_NULL, VAL_INT, VAL_FLOAT, VAL_STRING, VAL_BOOL, VAL_ARRAY, VAL_OBJECT, VAL_FUNC
} WssValType;

typedef struct WssObject WssObject;
typedef struct WssArray WssArray;
typedef struct WssFunction WssFunction;

typedef struct {
    WssValType type;
    union {
        int64_t as_int;
        double as_float;
        bool as_bool;
        char* as_str;
        WssArray* as_array;
        WssObject* as_obj;
        WssFunction* as_func;
    } data;
} WssValue;

WssValue WssValue_MakeNull(void);
WssValue WssValue_MakeInt(int64_t v);
WssValue WssValue_MakeFloat(double v);
WssValue WssValue_MakeString(const char* s);
WssValue WssValue_MakeBool(bool v);
WssValue WssValue_MakeArray(void);
WssValue WssValue_MakeObject(void);
void     WssValue_Destroy(WssValue* v);
void     WssValue_CopyTo(const WssValue* src, WssValue* dst);
bool     WssValue_Truthy(const WssValue* v);
bool     WssValue_Equals(const WssValue* a, const WssValue* b);

/* — Lexer — */
typedef enum {
    TKN_EOF, TKN_WORD, TKN_STRING, TKN_NUMBER, TKN_FLOAT,
    TKN_LPAREN, TKN_RPAREN, TKN_LBRACE, TKN_RBRACE,
    TKN_LBRACKET, TKN_RBRACKET,
    TKN_COMMA, TKN_COLON, TKN_SEMICOLON, TKN_DOT,
    TKN_EQ, TKN_NEQ, TKN_LT, TKN_GT, TKN_LTE, TKN_GTE,
    TKN_AND, TKN_OR, TKN_NOT,
    TKN_PLUS, TKN_MINUS, TKN_STAR, TKN_SLASH, TKN_PERCENT,
    TKN_TO, TKN_IN, TKN_DO, TKN_END, TKN_IF, TKN_THEN,
    TKN_WHEN, TKN_ELSE, TKN_SET, TKN_FUNC, TKN_RETURN,
    TKN_VAR, TKN_PRINT, TKN_TRUE, TKN_FALSE,
    TKN_NEWLINE, TKN_INDENT, TKN_AT, TKN_POUND,
    TKN_PLUSPLUS, TKN_MINUSMINUS,
    TKN_ARROW, TKN_COLONCOLON,
} WssTokenType;

typedef struct {
    WssTokenType type;
    char lexeme[128];
    int line;
    int col;
} WssToken;

typedef struct WssLexer {
    const char* src;
    size_t len;
    size_t pos;
    int line;
    int col;
    WssToken prev;
    WssToken cur;
    bool at_end;
} WssLexer;

void  WssLexer_Init(WssLexer* L, const char* src);
void  WssLexer_Advance(WssLexer* L);
bool  WssLexer_Check(WssLexer* L, WssTokenType t);
bool  WssLexer_Match(WssLexer* L, WssTokenType t);
bool  WssLexer_MatchAny(WssLexer* L, int n, ...);
void  WssLexer_Expect(WssLexer* L, WssTokenType t, const char* msg);

/* — Parser — */
typedef enum {
    AST_LITERAL, AST_VARIABLE, AST_ARRAY_LIT, AST_OBJECT_LIT,
    AST_BINARY, AST_UNARY, AST_CALL, AST_INDEX, AST_MEMBER,
    AST_ASSIGN, AST_SET, AST_DECLARE, AST_IF, AST_WHEN,
    AST_FOR, AST_FORIN, AST_WHILE, AST_FUNC_DECL, AST_RETURN,
    AST_BLOCK, AST_PRINT, AST_SEND, AST_SPAWN, AST_MOVE,
    AST_CAST, AST_CREATE, AST_WITH, AST_EVENT_HOOK, AST_GOSUB,
    AST_STATEMENT_LIST, AST_COMPOUND, AST_TRY, AST_THROW,
    AST_NOP
} WssAstType;

typedef struct WssAstNode {
    WssAstType type;
    int line;
    union {
        struct { WssValue val; } literal;
        struct { char name[64]; } variable;
        struct { int64_t idx; struct WssAstNode* expr; } index;
        struct { struct WssAstNode* obj; char member[64]; } member;
        struct { int op; struct WssAstNode* left; struct WssAstNode* right; } binary;
        struct { int op; struct WssAstNode* operand; } unary;
        struct { struct WssAstNode* callee; struct WssAstNode* args; } call;
        struct { char name[64]; struct WssAstNode* value; } assign;
        struct { char target[64]; struct WssAstNode* value; } set_stmt;
        struct { char name[64]; struct WssAstNode* initial; } declare;
        struct { struct WssAstNode* cond; struct WssAstNode* then_; struct WssAstNode* else_; } if_node;
        struct { char event[32]; struct WssAstNode* body; } when;
        struct { char var[64]; struct WssAstNode* start; struct WssAstNode* end; struct WssAstNode* body; } for_node;
        struct { char var[64]; struct WssAstNode* collection; struct WssAstNode* body; } forin;
        struct { struct WssAstNode* cond; struct WssAstNode* body; } while_node;
        struct { char name[64]; struct WssAstNode* params; struct WssAstNode* body; } func_decl;
        struct { struct WssAstNode* expr; } return_node;
        struct { struct WssAstNode* stmts; } block;
        struct { struct WssAstNode* value; } print;
        struct { struct WssAstNode* packet; struct WssAstNode* target; } send;
        struct { struct WssAstNode* unit; struct WssAstNode* pos; } spawn;
        struct { struct WssAstNode* target; struct WssAstNode* dest; } move;
        struct { struct WssAstNode* spell; struct WssAstNode* target; } cast;
        struct { struct WssAstNode* template; struct WssAstNode* props; } create;
        struct { struct WssAstNode* name; struct WssAstNode* props; } with;
        struct { struct WssAstNode* list; } stmt_list;
        struct { struct WssAstNode* expr; } nop;
    } node;
} WssAstNode;

typedef struct WssParser {
    WssLexer* lex;
    WssToken curtok;
    WssToken prevtok;
    bool panic;
} WssParser;

void WssParser_Init(WssParser* P, WssLexer* L);
WssAstNode* WssParser_Parse(WssParser* P);
void WssAstNode_Destroy(WssAstNode* n);

/* — Bytecode Compiler — */
typedef enum {
    OP_LOAD_CONST, OP_LOAD_VAR, OP_STORE_VAR, OP_LOAD_NULL,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE,
    OP_AND, OP_OR, OP_NOT,
    OP_JUMP, OP_JUMP_IF_FALSE, OP_JUMP_IF_TRUE, OP_LOOP,
    OP_ENTER_SCOPE, OP_LEAVE_SCOPE,
    OP_CALL, OP_RETURN, OP_YIELD,
    OP_BUILD_ARRAY, OP_BUILD_OBJECT, OP_INDEX_GET, OP_INDEX_SET,
    OP_MEMBER_GET, OP_MEMBER_SET,
    OP_PRINT, OP_SEND, OP_SPAWN, OP_MOVE, OP_CAST,
    OP_CREATE, OP_HOOK, OP_AWAIT,
    OP_POP, OP_DUP, OP_HALT
} WssOpCode;

typedef struct {
    WssOpCode op;
    int arg;       /* immediate operand or jump offset */
    const char* sym; /* symbol name for variable/function refs */
} WssInstruction;

typedef struct {
    WssInstruction* code;
    int count;
    int cap;
    WssValue* constants;
    int num_consts;
    int num_locals;
    int max_stack;
    char name[64];
} WssBytecode;

WssBytecode* WssBytecode_New(void);
void WssBytecode_Free(WssBytecode* bc);
void WssBytecode_Emit(WssBytecode* bc, WssOpCode op, int arg, const char* sym);
int  WssBytecode_EmitJump(WssBytecode* bc, WssOpCode op, int target);
void WssBytecode_PatchJump(WssBytecode* bc, int offset, int target);
int  WssBytecode_AddConstant(WssBytecode* bc, WssValue v);
void WssCompiler_Compile(WssParser* P, WssBytecode* bc);
void WssCompiler_CompileFile(const char* path, WssBytecode* bc);

/* — VM — */
typedef struct {
    WssValue* stack;
    int sp;       /* stack pointer */
    int bp;       /* base pointer / frame start */
    WssValue* locals;
    int num_locals;
    int ip;       /* instruction pointer */
    WssBytecode* bc;
    bool halt;
    WssValue retval;
    char error[256];
} WssVm;

void  WssVm_Init(WssVm* vm, WssBytecode* bc, int max_stack, int num_globals);
void  WssVm_Free(WssVm* vm);
WssValue WssVm_Run(WssVm* vm);
WssValue WssVm_RunChunk(WssVm* vm, size_t offset, size_t count);

/* — Scripting System — */
typedef struct WssHookEntry {
    char name[64];
    WssBytecode* bc;
    struct WssHookEntry* next;
} WssHookEntry;

typedef struct {
    WssBytecode** scripts;
    int num_scripts;
    int cap_scripts;
    WssHookEntry* hooks[32];
    WssVm* active_vm;
} WssScriptingSystem;

void  WssScriptingSystem_Init(WssScriptingSystem* S);
void  WssScriptingSystem_LoadScript(WssScriptingSystem* S, const char* name, const char* src);
bool  WssScriptingSystem_RunNamed(WssScriptingSystem* S, const char* name, int argc, WssValue* argv, WssValue* out);
WssValue WssScriptingSystem_CallFunc(WssScriptingSystem* S, const char* script_name, const char* func_name, int argc, WssValue* argv);
bool  WssScriptingSystem_RunHook(WssScriptingSystem* S, const char* hook_name, int argc, WssValue* argv);
bool  WssScriptingSystem_LoadScriptFile(WssScriptingSystem* S, const char* name, const char* filepath);
void  WssScriptingSystem_Delete(WssScriptingSystem* S);

/* — Hook IDs — */
#define HOOK_ON_UPDATE        "on_update"
#define HOOK_ON_SPAWN         "on_spawn"
#define HOOK_ON_GOSSIP_HELLO  "on_gossip_hello"
#define HOOK_ON_GOSSIP_SELECT "on_gossip_select"
#define HOOK_ON_SPELL_CAST    "on_spell_cast"
#define HOOK_ON_COMBAT_START  "on_combat_start"
#define HOOK_ON_COMBAT_END    "on_combat_end"
#define HOOK_ON_DEATH         "on_death"
#define HOOK_ON_LOGIN         "on_login"
#define HOOK_ON_LOGOUT        "on_logout"
#define HOOK_ON_QUEST_ACCEPT  "on_quest_accept"
#define HOOK_ON_QUEST_COMPLETE "on_quest_complete"
#define HOOK_ON_NPC_TALK      "on_npc_talk"
#define HOOK_ON_PLAYER_MOVE   "on_player_move"

#ifdef __cplusplus
}
#endif

#endif /* WSS_SCRIPTING_H */
