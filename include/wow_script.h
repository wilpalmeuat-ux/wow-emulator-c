#ifndef WOW_SCRIPT_H
#define WOW_SCRIPT_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_STACK_DEPTH 1024
#define MAX_VARIABLES 512
#define MAX_GLOBALS 256
#define MAX_OBJECTS 50000
#define MAX_SCRIPT_LENGTH 65536
#define MAX_FUNCTIONS 128
#define MAX_EVENTS 64
#define MAX_NATIVE_FUNCS 64
#define HASH_MAP_SIZE 1024

/* ============== VALUE TYPES ============== */
typedef enum {
    VAL_NIL = 0,
    VAL_INT,
    VAL_DBL,
    VAL_STR,
    VAL_BOOL,
    VAL_ARRAY,
    VAL_FUNC,
    VAL_NATIVE,
    VAL_OBJECT
} WoWType;

/* ============== OPCODES (for bytecode VM) ============== */
typedef enum {
    OP_HALT,
    OP_LOAD,
    OP_STORE,
    OP_LOADG,
    OP_STOREG,
    OP_PUSH,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_CMP_EQ,
    OP_CMP_NE,
    OP_CMP_LT,
    OP_CMP_GT,
    OP_CMP_LE,
    OP_CMP_GE,
    OP_JMP,
    OP_JZ,
    OP_JNZ,
    OP_CALL,
    OP_RET,
    OP_SPAWN,
    OP_TELEPORT,
    OP_CAST,
    OP_LOG,
    OP_BROADCAST,
    OP_SET_LEVEL,
    OP_ADD_XP,
    OP_GET_HEALTH,
    OP_SET_HEALTH,
    OP_SEND_MESSAGE,
    OP_DESPAWN,
    OP_LOOT,
    OP_SUMMON,
    OP_SET_FACTION,
    OP_SET_MODEL,
    OP_SET_SPEED,
    OP_EVENT_BIND,
    OP_LOOP,
    OP_GET_RANDOM_PLAYER,
    OP_NOP
} Opcode;

/* ============== TOKEN (used by scanner) ============== */
typedef struct Token {
    char* lexeme;
    int type;
    int line;
    int col;
    union {
        int64_t int_val;
        double double_val;
    };
} Token;

/* ============== STATEMENT TYPES ============== */
typedef enum {
    STMT_NONE = 0,
    STMT_CREATE,
    STMT_SPAWN,
    STMT_SET,
    STMT_GET,
    STMT_IF,
    STMT_FOR,
    STMT_WHILE,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_EVENT,
    STMT_BROADCAST,
    STMT_TELEPORT,
    STMT_GIVE_ITEM,
    STMT_SEND_MESSAGE,
    STMT_SET_FLAG,
    STMT_REMOVE_FLAG,
    STMT_SET_LEVEL,
    STMT_SET_HP,
    STMT_SET_MANA,
    STMT_ADD_XP,
    STMT_REGISTER_HOOK,
    STMT_LOG,
    STMT_QUIT
} StmtType;

/* ============== STATEMENT ============== */
typedef struct Stmt Stmt;
typedef struct Word Word;
typedef struct Parser Parser;

struct Stmt {
    StmtType type;
    int line;
    union {
        struct { char* name; Stmt* body; } create;
        struct { char* name; int template_id; float x, y, z, o; uint32_t map_id; } spawn;
        struct { char* target; char* field; int64_t int_val; double double_val; char* str_val; int val_type; } set;
        struct { char* target; char* field; } get;
        struct { Stmt* cond; Stmt* then_; Stmt* else_; } if_;
        struct { Stmt* init; Stmt* cond; Stmt* inc; Stmt* body; } for_;
        struct { Stmt* cond; Stmt* body; } while_;
        struct { char* name; char** params; int param_count; Stmt* body; } function;
        struct { int val_type; int64_t int_val; double double_val; char* str_val; } return_;
        struct { char* event_name; Stmt* handler; } event;
        struct { char* message; } broadcast;
        struct { char* player; float x, y, z; uint32_t map_id; } teleport;
        struct { char* player; int entry; int count; } give_item;
        struct { char* player; char* message; } send_message;
        struct { char* target; int flag; } set_flag;
        struct { char* target; int flag; } remove_flag;
        struct { char* target; int level; } set_level;
        struct { char* target; int hp; } set_hp;
        struct { char* target; int mana; } set_mana;
        struct { char* target; int xp; } add_xp;
        struct { char* hook_name; Stmt* handler; } register_hook;
        struct { char* message; } log;
        struct { char* name; int val_type; int64_t int_val; double double_val; char* str_val; } decl;
    } as;
};

/* ============== PARSER ============== */
struct Parser {
    Token* tokens;
    int current;
    int count;
    int had_error;
    const char* error_msg;
    const char* source;
};

/* ============== WoW VALUE ============== */
typedef struct WoWValue {
    WoWType type;
    union {
        int64_t int_val;
        double double_val;
        char* str;
        bool bool_val;
        void* ptr;
    } as;
} WoWValue;

/* ============== BYTECODE COMPILER ============== */
typedef struct BytecodeCompiler {
    int* code;
    int code_size;
    int code_cap;
    int local_count;
    char local_names[MAX_STACK_DEPTH][64];
    char local_types[MAX_STACK_DEPTH];
    int next_label;
    int* label_addrs;
    int label_count;
} BytecodeCompiler;

/* ============== SCRIPT VM ============== */
typedef struct ScriptVM {
    int* ip;
    int* code;
    int code_size;
    WoWValue locals[MAX_STACK_DEPTH];
    int local_count;
    WoWValue globals[MAX_GLOBALS];
    int stack[MAX_STACK_DEPTH];
    int sp;
    int bp;
    bool halted;
    bool crashed;
    char error[512];
} ScriptVM;

/* ============== HASH MAP ============== */
typedef struct HashEntry {
    char* key;
    void* value;
    struct HashEntry* next;
} HashEntry;

typedef struct HashMap {
    HashEntry* buckets[HASH_MAP_SIZE];
    int count;
} HashMap;

/* ============== GAME OBJECT ============== */
typedef struct Object {
    uint64_t guid;
    int type;
    int npc_id;
    float x, y, z, orient;
    uint32_t map_id;
    int32_t health;
    int32_t max_health;
    int32_t level;
    int faction;
    int model;
    float speed_walk;
    float speed_run;
    bool in_world;
    char name[64];
} Object;

/* ============== OBJECT MANAGER ============== */
typedef struct ObjectManager {
    Object objects[MAX_OBJECTS];
    int object_count;
    HashMap object_map;
} ObjectManager;

/* ============== NATIVE FUNCTION ============== */
typedef WoWValue (*NativeFn)(int argc, WoWValue* argv);

typedef struct NativeFuncEntry {
    char name[64];
    NativeFn fn;
} NativeFuncEntry;

/* ============== SCRIPT ENGINE ============== */
typedef struct ScriptEngine {
    BytecodeCompiler* compiler;
    ScriptVM* main_vm;
    ObjectManager object_mgr;
    HashMap global_vars;
    HashMap string_intern;
    NativeFuncEntry natives[MAX_NATIVE_FUNCS];
    int native_count;
    Stmt* entry_point;
    Stmt** all_scripts;
    int script_count;
    int max_scripts;
    char script_source[MAX_SCRIPT_LENGTH];
    int source_len;
    char last_error[512];
} ScriptEngine;

/* ============== FUNCTION DECLARATIONS ============== */

/* Scanner / Lexer */
Token* scanner_scan(const char* source, int* out_count, bool* had_error, const char** error_msg);

/* Parser */
Stmt* parse(const char* source);
void parser_free(void);

/* Script Engine */
ScriptEngine* script_engine_create(void);
void script_engine_free(ScriptEngine* engine);
void script_engine_register_native(ScriptEngine* e, const char* name, NativeFn fn);
int script_engine_load_source(ScriptEngine* e, const char* source);
int script_engine_execute(ScriptEngine* e);
int script_engine_trigger_event(ScriptEngine* e, const char* event_name, uint64_t object_guid);
const char* script_engine_get_error(ScriptEngine* e);

/* Bytecode Compiler */
BytecodeCompiler* compiler_create(void);
void compiler_free(BytecodeCompiler* c);
int compiler_compile(BytecodeCompiler* c, Token* tokens, int token_count, const char* source);
int compiler_get_opcode(BytecodeCompiler* c, const char* name);

/* VM */
ScriptVM* vm_create(void);
void vm_free(ScriptVM* vm);
void vm_reset(ScriptVM* vm);
int vm_exec(ScriptVM* vm, int* code, int code_size, ScriptEngine* engine);
void vm_push(ScriptVM* vm, WoWValue val);
WoWValue vm_pop(ScriptVM* vm);

/* Hash Map */
HashMap* hashmap_create(void);
void hashmap_free(HashMap* m);
void hashmap_set(HashMap* m, const char* key, void* value);
void* hashmap_get(HashMap* m, const char* key);
bool hashmap_contains(HashMap* m, const char* key);

/* Object Manager */
ObjectManager* objmgr_create(void);
void objmgr_free(ObjectManager* m);
uint64_t objmgr_spawn(ObjectManager* m, int npc_id, float x, float y, float z, float o, uint32_t map);
void objmgr_despawn(ObjectManager* m, uint64_t guid);
Object* objmgr_get(ObjectManager* m, uint64_t guid);
Object* objmgr_get_by_name(ObjectManager* m, const char* name);
Object* objmgr_get_by_entry(ObjectManager* m, int entry);
int objmgr_object_count(ObjectManager* m);

/* Utility */
char* scripting_strdup(const char* s);
void scripting_log(ScriptEngine* e, const char* msg);

#endif /* WOW_SCRIPT_H */