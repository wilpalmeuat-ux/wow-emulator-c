/*
 * World of Warcraft 3.3.5a Private Server
 * Custom Word-Based Scripting System
 * 
 * Architecture:
 *   - Auth Server (TCP, port 3724)
 *   - World Server (TCP, port 8085)
 *   - Script Engine (word/statement based parser)
 *   - MySQL Database Layer
 */

#include "shared/NetworkCompat.h"
#ifndef WORLD_H
#define WORLD_H

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <windows.h>
    #include <winsock2.h>
    #include <mysql/mysql.h>
#else
                    #include <mysql/mysql.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int BOOL;
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================
 * CORE CONFIGURATION
 * ============================================================ */
#define CONFIG_AUTH_PORT    3724
#define CONFIG_WORLD_PORT   8085
#define CONFIG_MAX_PLAYERS  5000
#define CONFIG_TIMEOUT_MS   30000

/* ============================================================
 * CORE DATA STRUCTURES
 * ============================================================ */

/* Player object structure */
typedef struct {
    uint64_t guid;
    char     name[32];
    uint32_t account_id;
    uint32_t level;
    uint32_t race;
    uint32_t class;
    float    position_x;
    float    position_y;
    float    position_z;
    float    orientation;
    uint32_t map_id;
    uint32_t health;
    uint32_t power;
    time_t   last_update;
} Player;

/* Session structure */
typedef struct {
    SOCKET   socket;
    uint32_t account_id;
    uint32_t server_id;
    char     ip_address[16];
    Player*  player;
    time_t   connected_at;
    BOOL     authenticated;
} Session;

/* World state */
typedef struct {
    Player*    players[CONFIG_MAX_PLAYERS];
    Session*   sessions[CONFIG_MAX_PLAYERS];
    uint32_t   player_count;
    uint32_t   session_count;
    time_t     server_start_time;
    char       realm_name[64];
} WorldState;

/* ============================================================
 * SCRIPT SYSTEM - WORD-BASED PARSER
 * ============================================================ */

/* Token types for word-based scripting */
typedef enum {
    TOKEN_WORD,           /* Single word identifier */
    TOKEN_STRING,         /* Quoted string */
    TOKEN_NUMBER,         /* Integer or float */
    TOKEN_LBRACE,         /* { */
    TOKEN_RBRACE,         /* } */
    TOKEN_LBRACKET,       /* [ */
    TOKEN_RBRACKET,       /* ] */
    TOKEN_COLON,          /* : */
    TOKEN_SEMICOLON,      /* ; */
    TOKEN_PIPE,           /* | (for conditional flow) */
    TOKEN_ARROW,          /* -> */
    TOKEN_EOF
} TokenType;

/* A single word/statement token */
typedef struct {
    TokenType type;
    char      value[256];
    int       line;
    int       column;
} Token;

/* Script command definition (maps words to C handlers) */
typedef struct {
    const char* command_word;
    void (*handler)(Session* session, Token* args, int arg_count);
    const char* description;
} ScriptCommand;

/* Script flow control node */
typedef struct ScriptNode {
    char     word[64];
    char     statement[512];
    void*    data;
    struct ScriptNode* next;
    struct ScriptNode* branch;  /* For | conditional branches */
} ScriptNode;

/* ============================================================
 * CORE FUNCTION DECLARATIONS
 * ============================================================ */

/* Network */
int  network_init(void);
void network_shutdown(void);
int  network_select(void);
void handle_new_connection(SOCKET server_socket);
void handle_client_packet(Session* session, uint8_t* buffer, int length);

/* Session Management */
Session* session_create(SOCKET socket);
void     session_destroy(Session* session);
Session* session_find_by_account(uint32_t account_id);
Session* session_find_by_player(uint64_t guid);

/* Player Management */
Player* player_create(uint64_t guid, const char* name, uint32_t account_id);
void    player_destroy(Player* player);
int     player_save_to_db(Player* player);
Player* player_load_from_db(uint64_t guid);

/* Database */
int  db_init(const char* host, const char* user, const char* password, const char* database);
void db_shutdown(void);
int  db_query(const char* sql);
int  db_query_callback(const char* sql, void (*callback)(MYSQL_ROW row, void* data), void* data);
MYSQL* db_get_handle(void);

/* World State */
WorldState* world_get_state(void);
void world_update(void);
void world_broadcast(uint8_t* packet, int length, uint64_t exclude_guid);
void world_spawnCreature(uint32_t entry, float x, float y, float z, float o, uint32_t map);

/* Packet Handling (opcodes) */
#define CMSG_AUTH_SESSION          0x1A2B
#define CMSG_CHAR_CREATE           0x0369
#define CMSG_CHAR_DELETE           0x036A
#define CMSG_PLAYER_LOGIN          0x038D
#define CMSG_NAME_QUERY            0x0362
#define CMSG_MOVE_WAYPOINT         0x01B2

#define SMSG_AUTH_CHALLENGE        0x1A2C
#define SMSG_AUTH_RESPONSE         0x1A2D
#define SMSG_CHAR_ENUM            0x036B
#define SMSG_CHAR_CREATE_RESPONSE  0x036C
#define SMSG_LOGIN_VERIFY_WORLD    0x038E
#define SMSG_UPDATE_OBJECT         0x0191
#define SMSG_CREATURE_MOVE         0x01B3

/* ============================================================
 * CUSTOM SCRIPT ENGINE
 * ============================================================ */

/* Initialize the script engine */
int script_engine_init(void);
void script_engine_shutdown(void);

/* Load and parse script files */
int script_load_file(const char* filepath);
int script_load_directory(const char* directory_path);

/* Execute a statement */
int script_execute(const char* statement, Session* session);

/* Register custom commands */
int script_register_command(const char* word, void (*handler)(Session*, Token*, int));

/* Script node management */
ScriptNode* script_node_create(const char* word, const char* statement);
void script_node_link(ScriptNode* parent, ScriptNode* child);
void script_node_branch(ScriptNode* node, ScriptNode* branch);

/* Word tokenizer */
Token* tokenize(const char* input, int* out_count);
void token_free(Token* tokens, int count);

/* Default script commands (built-in words) */
void cmd_spawn(Session* session, Token* args, int arg_count);
void cmd_teleport(Session* session, Token* args, int arg_count);
void cmd_message(Session* session, Token* args, int arg_count);
void cmd_give_item(Session* session, Token* args, int arg_count);
void cmd_set_level(Session* session, Token* args, int arg_count);
void cmd_create_npc(Session* session, Token* args, int arg_count);
void cmd_set_attribute(Session* session, Token* args, int arg_count);
void cmd_play_sound(Session* session, Token* args, int arg_count);
void cmd_start_quest(Session* session, Token* args, int arg_count);
void cmd_complete_quest(Session* session, Token* args, int arg_count);

/* ============================================================
 * LOGGING
 * ============================================================ */
#define LOG_INFO    0
#define LOG_WARN    1
#define LOG_ERROR   2
#define LOG_DEBUG   3

void log_init(void);
void log_shutdown(void);
void log_message(int level, const char* fmt, ...);

/* ============================================================
 * UTILITY
 * ============================================================ */
uint32_t generate_guid(void);
const char* get_timestamp(void);
void hex_dump(uint8_t* data, int length);

#endif /* WORLD_H */