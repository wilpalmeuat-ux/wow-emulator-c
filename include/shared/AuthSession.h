/*
 * AuthSession.h - Shared auth session between authserver and worldserver
 * Handles session token validation and player-to-world handoff
 */

#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SESSION_TOKENS 10000
#define SESSION_TIMEOUT_SECS 600

/* Session states */
typedef enum {
    SESSION_STATE_AUTHED      = 0,  /* authenticated on authserver */
    SESSION_STATE_TRANSFERRING = 1,  /* transitioning to worldserver */
    SESSION_STATE_IN_WORLD    = 2,  /* active in worldserver */
    SESSION_STATE_LOGGED_OUT   = 3  /* cleanly logged out */
} SessionState;

/* Session entry - stored in shared memory or passed via socket */
typedef struct AuthSession {
    uint32_t session_key;       /* unique session identifier */
    uint64_t account_id;        /* account database ID */
    char account_name[32];      /* account login name */
    uint32_t ip_address;        /* client IP as uint32_t */
    SessionState state;         /* current session state */
    uint32_t max_level;         /* expansion/max level allowed */
    uint32_t security_flags;    /* GM level, flags */
    uint64_t current_character_guid;
    time_t created_at;
    time_t last_activity;
    char realm_name[64];
    uint32_t realm_id;
} AuthSession;

/* Session storage - hash map keyed by session_key */
typedef struct SessionStore {
    AuthSession sessions[MAX_SESSION_TOKENS];
    int session_count;
} SessionStore;

/* Create/find sessions */
AuthSession* session_create(uint64_t account_id, const char* account_name);
AuthSession* session_find(uint32_t session_key);
AuthSession* session_find_by_account(uint64_t account_id);
void session_destroy(uint32_t session_key);
void session_update_activity(uint32_t session_key);

/* Validation */
bool session_is_valid(uint32_t session_key);
bool session_has_character(uint32_t session_key);
uint64_t session_get_character(uint32_t session_key);

/* State transitions */
void session_enter_world(uint32_t session_key, uint64_t character_guid);
void session_leave_world(uint32_t session_key);
void session_close(uint32_t session_key);

/* Packet helpers - build session response for worldserver */
int session_build_addon_packet(uint8_t* buf, int max_len);
int session_verify_encryption(uint32_t session_key,
    const uint8_t* encrypted_data, int data_len);

/* Session store globals (shared between authserver and worldserver) */
extern SessionStore g_session_store;

void session_store_init(void);
void session_store_free(void);

#endif /* AUTH_SESSION_H */