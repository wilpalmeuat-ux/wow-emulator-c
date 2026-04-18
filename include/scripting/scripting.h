#pragma once

#include "shared/shared.h"

// Word-statement scripting engine

typedef enum {
    TOKEN_WORD,          // single word (command)
    TOKEN_STRING,        // "quoted string"
    TOKEN_NUMBER,        // 123 or 1.5
    TOKEN_GUID,          // 0x0000000000000001
    TOKEN_NEWLINE,       // end of statement
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_COMMA,         // ,
    TOKEN_EOF,           // end of file
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[256];
    int line;
    int column;
} Token;

typedef enum {
    NODE_COMMAND,            // single word command
    NODE_STATEMENT,           // WORD arg1 arg2 ...
    NODE_BLOCK,               // ON EVENT ... END
    NODE_SCRIPT_FILE
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char command[64];
    char args[16][256];
    int arg_count;
    char event_name[64];
    struct ASTNode* children;
    int child_count;
    struct ASTNode* next;
    int line;
} ASTNode;

typedef struct {
    char event_name[64];
    ASTNode* handler;
    ASTNode* next;
} ScriptEventHandler;

typedef struct {
    char script_name[128];
    char file_path[256];
    ASTNode* ast;
    ScriptEventHandler* handlers;
    int handler_count;
} Script;

typedef struct ScriptingEngine {
    Script* loaded_scripts;
    int script_count;
    ScriptCallback callbacks[256];
    char last_error[512];
    bool debug_mode;
} ScriptingEngine;

typedef struct {
    void* target;
    ASTNode* node;
    ScriptingEngine* engine;
} ScriptContext;

// Core scripting engine API
ScriptingEngine* ScriptEngine_Create(void);
void ScriptEngine_Destroy(ScriptingEngine* engine);
void ScriptEngine_SetDebug(ScriptingEngine* engine, bool debug);
bool ScriptEngine_LoadScript(ScriptingEngine* engine, const char* name, const char* file_path);
bool ScriptEngine_ExecuteEvent(ScriptingEngine* engine, const char* event_name, void* target, int argc, char** argv);
bool ScriptEngine_ExecuteNode(ScriptingEngine* engine, ASTNode* node, void* target);
const char* ScriptEngine_GetLastError(ScriptingEngine* engine);

// Lexer API
Token* Lexer_Tokenize(const char* source, int* token_count);
void Lexer_FreeTokens(Token* tokens, int count);
void Lexer_DumpTokens(Token* tokens, int count);

// Parser API
ASTNode* Parser_Parse(Token* tokens, int token_count);
void Parser_FreeAST(ASTNode* node);
void Parser_DumpAST(ASTNode* node, int indent);

// Built-in command registry
typedef void (*BuiltinCommand)(ScriptContext* ctx);
void ScriptEngine_RegisterBuiltin(ScriptingEngine* engine, const char* word, BuiltinCommand cmd);
void ScriptEngine_RegisterAllBuiltins(ScriptingEngine* engine);

// Built-in commands
void builtin_say(ScriptContext* ctx);
void builtin_whisper(ScriptContext* ctx);
void builtin_give_item(ScriptContext* ctx);
void builtin_take_item(ScriptContext* ctx);
void builtin_teleport(ScriptContext* ctx);
void builtin_set_var(ScriptContext* ctx);
void builtin_get_var(ScriptContext* ctx);
void builtin_spawn_npc(ScriptContext* ctx);
void builtin_despawn_npc(ScriptContext* ctx);
void builtin_start_quest(ScriptContext* ctx);
void builtin_complete_quest(ScriptContext* ctx);
void builtin_modify_health(ScriptContext* ctx);
void builtin_modify_power(ScriptContext* ctx);
void builtin_set_level(ScriptContext* ctx);
void builtin_send_message(ScriptContext* ctx);
void builtin_play_sound(ScriptContext* ctx);
void builtin_cast_spell(ScriptContext* ctx);
void builtin_add_affect(ScriptContext* ctx);
void builtin_remove_affect(ScriptContext* ctx);
void builtin_set_ai(ScriptContext* ctx);
void builtin_set_name(ScriptContext* ctx);
void builtin_set_model(ScriptContext* ctx);
void builtin_set_faction(ScriptContext* ctx);
void builtin_emit(ScriptContext* ctx);
void builtin_grant_item(ScriptContext* ctx);
void builtin_if_cond(ScriptContext* ctx);
void builtin_goto_label(ScriptContext* ctx);
void builtin_return(ScriptContext* ctx);
void builtin_foreach(ScriptContext* ctx);
void builtin_while_loop(ScriptContext* ctx);
