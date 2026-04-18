/*
 * WordScript Engine - Header
 * A word/statement based scripting engine for WoW 3.3.5
 * All scripts are plain English words and statements
 */

#ifndef WORDSCRIPT_H
#define WORDSCRIPT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Token types - every word/statement is a token */
typedef enum {
    TOKEN_NONE = 0,
    TOKEN_WORD,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_BLOCK_START,
    TOKEN_BLOCK_END,
    TOKEN_STATEMENT_END,
    TOKEN_EOF
} TokenType;

/* Script opcodes - each is a keyword action */
typedef enum {
    /* Flow control */
    OP_IF,
    OP_ELSE,
    OP_ELSEIF,
    OP_ENDIF,
    OP_WHILE,
    OP_ENDWHILE,
    OP_FOR,
    OP_ENDFOR,
    OP_FOREACH,
    OP_ENDFOREACH,
    OP_BREAK,
    OP_CONTINUE,
    OP_RETURN,
    OP_GOTO,

    /* Actions */
    OP_SENDMESSAGE,
    OP_CASTSPELL,
    OP_GIVEITEM,
    OP_REMOVEITEM,
    OP_GIVEGOLD,
    OP_TELEPORT,
    OP_TELEPORTEFFECT,
    OP_FADEOUT,
    OP_FADEIN,
    OP_PLAYSOUND,
    OP_SETVARIABLE,
    OP_SETFLAG,
    OP_CLEARFLAG,
    OP_SPAWNNPC,
    OP_DESPAWN,
    OP_STARTQUEST,
    OP_COMPLETEQUEST,
    OP_FAILQUEST,
    OP_ADDEXPERIENCE,
    OP_MODIFYSTAT,
    OP_SENDMAIL,
    OP_CREATEITEM,
    OP_DESTROYITEM,
    OP_SETWORLDSTATE,
    OP_REGISTER_EVENT,
    OP_UNREGISTER_EVENT,
    OP_CUSTOM,
    OP_NOOP
} OpCode;

/* Comparison operators */
typedef enum {
    CMP_NONE = 0,
    CMP_EQ,        /* == */
    CMP_NEQ,       /* != */
    CMP_GT,        /* >  */
    CMP_LT,        /* <  */
    CMP_GTE,       /* >= */
    CMP_LTE,       /* <= */
    CMP_CONTAINS,
    CMP_HASNPCFLAG,
    CMP_HASITEM,
    CMP_HASQUEST,
    CMP_HASSpell
} CompareOp;

/* Condition types */
typedef enum {
    COND_LEVEL,
    COND_GOLD,
    COND_ITEM,
    COND_QUEST,
    COND_SPELL,
    COND_NPCFLAG,
    COND_PLAYERCLASS,
    COND_PLAYERRACE,
    COND_VARIABLE,
    COND_CUSTOM
} ConditionType;

/* Value types for variables */
typedef enum {
    VAL_NONE = 0,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL
} ValueType;

/* Generic value container */
typedef struct {
    ValueType type;
    union {
        int32_t as_int;
        float as_float;
        char* as_string;
        bool as_bool;
    };
} ScriptValue;

/* Token structure */
typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int column;
    union {
        int32_t as_int;
        float as_float;
    };
} ScriptToken;

/* AST Node types */
typedef enum {
    NODE_SCRIPT,
    NODE_BLOCK,
    NODE_STATEMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_FOREACH,
    NODE_EXPRESSION,
    NODE_CONDITION,
    NODE_ACTION,
    NODE_PARAMETER
} NodeType;

/* Parameter for actions */
typedef struct {
    char* key;
    ScriptValue value;
    struct Parameter* next;
} Parameter;

/* Action node (one statement line) */
typedef struct {
    OpCode opcode;
    char* name;
    Parameter* params;
    struct ActionNode* next;
} ActionNode;

/* Condition node */
typedef struct {
    ConditionType type;
    char* expression;
    CompareOp compare;
    ScriptValue left;
    ScriptValue right;
    char* raw;
} ConditionNode;

/* Control flow structures */
typedef struct {
    ActionNode* actions;     /* then branch */
    ActionNode* else_actions;
    ConditionNode* condition;
} IfNode;

typedef struct {
    ConditionNode* condition;
    ActionNode* actions;
} WhileNode;

typedef struct {
    char* variable;
    char* iterable;
    ActionNode* actions;
} ForeachNode;

/* Script block (one BEGIN...END section) */
typedef struct {
    char* name;
    int script_id;
    ActionNode* actions;
    IfNode* if_stack;
    WhileNode* while_stack;
    ForeachNode* foreach_stack;
    int local_var_count;
    char** local_vars;
    struct ScriptBlock* next;
} ScriptBlock;

/* Root AST */
typedef struct {
    ScriptBlock* blocks;
    int block_count;
} ScriptRoot;

/* VM types */
typedef enum {
    VM_NONE,
    VM_GREETING,
    VM_QUEST,
    VM_ITEM_USE,
    VM_GOSSIP,
    VM_BOSS_ENCOUNTER,
    VM_CUSTOM
} ScriptType;

/* Script context - runtime state */
typedef struct {
    uint64_t player_guid;
    uint64_t target_guid;
    uint64_t npc_entry;
    uint64_t quest_id;
    uint64_t item_entry;
    ScriptType type;
    void* world_state;      /* hook to World object */
    void* player_state;     /* hook to Player object */
    void* script_vars;      /* script-local variable store */
} ScriptContext;

/* Built-in function signature */
typedef int (*BuiltinFunc)(ScriptContext* ctx, Parameter* params);

/* VM state */
typedef struct {
    ScriptBlock* block;
    ActionNode* pc;
    ScriptContext* context;
    int stack_top;
    ScriptValue stack[64];
    int call_depth;
    bool halted;
    char error[256];
} ScriptVM;

/* Engine state */
typedef struct {
    ScriptRoot* ast;
    ScriptVM* vms;
    int vm_count;
    void* world;
    void* module_hooks;
} ScriptEngine;

/* Public API */
ScriptEngine* wordscript_init(void);
void wordscript_shutdown(ScriptEngine* engine);

int wordscript_load_file(ScriptEngine* engine, const char* filepath);
int wordscript_load_string(ScriptEngine* engine, const char* name, const char* source);
int wordscript_execute(ScriptEngine* engine, const char* script_name, ScriptContext* ctx);
int wordscript_execute_id(ScriptEngine* engine, int script_id, ScriptContext* ctx);

ScriptToken* wordscript_tokenize(const char* source, int* out_count);
ScriptRoot* wordscript_parse(ScriptToken* tokens, int token_count);
void wordscript_free_ast(ScriptRoot* ast);

/* Debug */
void wordscript_dump_ast(ScriptRoot* ast);
void wordscript_disasm_block(ScriptBlock* block);

#endif /* WORDSCRIPT_H */
