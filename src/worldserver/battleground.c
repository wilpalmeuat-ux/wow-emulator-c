/* battleground.c -- Battleground and Arena system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BG_INSTANCES 100
#define MAX_BG_PLAYERS 80  /* WSG=20, AB=30, AV=80 */
#define MAX_ARENA_TEAMS 500

typedef enum {
    BG_ALTERAC_VALLEY  = 1,
    BG_WARSONG_GULCH   = 2,
    BG_ARATHI_BASIN    = 3,
    BG_EYE_OF_STORM    = 7,
    BG_STRAND_OF_ANCIENTS = 9,
    BG_ISLE_OF_CONQUEST = 30,
    ARENA_2V2 = 100,
    ARENA_3V3 = 101,
    ARENA_5V5 = 102
} BattlegroundType;

typedef enum {
    BG_STATUS_NONE = 0,
    BG_STATUS_QUEUED,
    BG_STATUS_IN_PROGRESS,
    BG_STATUS_ENDED
} BGStatus;

typedef enum { BG_TEAM_ALLIANCE = 0, BG_TEAM_HORDE = 1 } BGTeam;

typedef struct BGPlayer {
    uint64_t guid;
    char     name[65];
    BGTeam   team;
    uint32_t kills;
    uint32_t deaths;
    uint32_t damage;
    uint32_t healing;
    bool     alive;
} BGPlayer;

typedef struct BGScore {
    uint32_t allianceScore;
    uint32_t hordeScore;
    uint32_t allianceFlags;  /* WSG flags captured */
    uint32_t hordeFlags;
    uint32_t allianceBases;  /* AB bases held */
    uint32_t hordeBases;
} BGScore;

typedef struct Battleground {
    uint32_t           instanceId;
    BattlegroundType   type;
    BGStatus           status;
    MapId              mapId;
    BGPlayer           players[MAX_BG_PLAYERS];
    int                playerCount;
    BGScore            score;
    time_t             startTime;
    uint32_t           maxDuration;  /* seconds */
    uint32_t           minPlayers;
    uint32_t           maxPlayers;
    BGTeam             winner;
    bool               active;
} Battleground;

typedef struct BGQueue {
    uint64_t guids[MAX_BG_PLAYERS];
    BGTeam   teams[MAX_BG_PLAYERS];
    int      count;
    BattlegroundType type;
} BGQueue;

typedef struct ArenaTeam {
    uint32_t id;
    char     name[64];
    uint8_t  type;        /* 2, 3, or 5 */
    uint64_t members[5];
    int      memberCount;
    uint32_t rating;
    uint32_t wins;
    uint32_t losses;
    uint32_t seasonWins;
    uint32_t seasonLosses;
    bool     active;
} ArenaTeam;

static Battleground g_battlegrounds[MAX_BG_INSTANCES];
static BGQueue g_bgQueues[10];
static int g_bgQueueCount = 0;
static ArenaTeam g_arenaTeams[MAX_ARENA_TEAMS];
static int g_arenaTeamCount = 0;
static uint32_t g_nextBgId = 1;

void BattlegroundSystem_Init(void) {
    memset(g_battlegrounds, 0, sizeof(g_battlegrounds));
    memset(g_bgQueues, 0, sizeof(g_bgQueues));
    memset(g_arenaTeams, 0, sizeof(g_arenaTeams));
    g_bgQueueCount = 0;
    g_arenaTeamCount = 0;
    printf("[BG] Battleground system initialized\n");
}

static Battleground* _findFreeBg(void) {
    for (int i = 0; i < MAX_BG_INSTANCES; i++)
        if (!g_battlegrounds[i].active) return &g_battlegrounds[i];
    return NULL;
}

uint32_t BG_Create(BattlegroundType type) {
    Battleground* bg = _findFreeBg();
    if (!bg) return 0;
    memset(bg, 0, sizeof(Battleground));
    bg->instanceId = g_nextBgId++;
    bg->type = type;
    bg->status = BG_STATUS_QUEUED;
    bg->active = true;
    bg->startTime = time(NULL);

    switch (type) {
    case BG_WARSONG_GULCH:   bg->mapId = 489; bg->minPlayers = 10; bg->maxPlayers = 20; bg->maxDuration = 1500; break;
    case BG_ARATHI_BASIN:    bg->mapId = 529; bg->minPlayers = 15; bg->maxPlayers = 30; bg->maxDuration = 1500; break;
    case BG_ALTERAC_VALLEY:  bg->mapId = 30;  bg->minPlayers = 20; bg->maxPlayers = 80; bg->maxDuration = 2400; break;
    case BG_EYE_OF_STORM:    bg->mapId = 566; bg->minPlayers = 15; bg->maxPlayers = 30; bg->maxDuration = 1500; break;
    case ARENA_2V2:          bg->mapId = 559; bg->minPlayers = 4;  bg->maxPlayers = 4;  bg->maxDuration = 600;  break;
    case ARENA_3V3:          bg->mapId = 559; bg->minPlayers = 6;  bg->maxPlayers = 6;  bg->maxDuration = 600;  break;
    case ARENA_5V5:          bg->mapId = 559; bg->minPlayers = 10; bg->maxPlayers = 10; bg->maxDuration = 600;  break;
    default: bg->mapId = 489; bg->minPlayers = 10; bg->maxPlayers = 20; bg->maxDuration = 1500; break;
    }

    printf("[BG] Created %s (ID %u, map %u)\n",
           type <= 3 ? "Battleground" : "Arena", bg->instanceId, bg->mapId);
    return bg->instanceId;
}

bool BG_AddPlayer(uint32_t bgId, uint64_t guid, const char* name, BGTeam team) {
    for (int i = 0; i < MAX_BG_INSTANCES; i++) {
        Battleground* bg = &g_battlegrounds[i];
        if (bg->instanceId == bgId && bg->active && bg->playerCount < (int)bg->maxPlayers) {
            BGPlayer* p = &bg->players[bg->playerCount++];
            p->guid = guid; strncpy(p->name, name, 64); p->team = team; p->alive = true;
            if ((uint32_t)bg->playerCount >= bg->minPlayers && bg->status == BG_STATUS_QUEUED)
                bg->status = BG_STATUS_IN_PROGRESS;
            return true;
        }
    }
    return false;
}

void BG_Update(uint32_t diffMs) {
    (void)diffMs;
    time_t now = time(NULL);
    for (int i = 0; i < MAX_BG_INSTANCES; i++) {
        Battleground* bg = &g_battlegrounds[i];
        if (!bg->active || bg->status != BG_STATUS_IN_PROGRESS) continue;
        if ((uint32_t)(now - bg->startTime) >= bg->maxDuration) {
            bg->status = BG_STATUS_ENDED;
            bg->winner = (bg->score.allianceScore >= bg->score.hordeScore) ? BG_TEAM_ALLIANCE : BG_TEAM_HORDE;
            printf("[BG] BG %u ended. Winner: %s\n", bg->instanceId,
                   bg->winner == BG_TEAM_ALLIANCE ? "Alliance" : "Horde");
        }
    }
}

/* Arena team management */
uint32_t ArenaTeam_Create(const char* name, uint8_t type, uint64_t leaderGuid) {
    if (g_arenaTeamCount >= MAX_ARENA_TEAMS) return 0;
    ArenaTeam* at = &g_arenaTeams[g_arenaTeamCount];
    at->id = (uint32_t)(g_arenaTeamCount + 1);
    strncpy(at->name, name, 63);
    at->type = type;
    at->members[0] = leaderGuid;
    at->memberCount = 1;
    at->rating = 1500;
    at->active = true;
    g_arenaTeamCount++;
    printf("[Arena] Created team '%s' (%uv%u, rating %u)\n", name, type, type, at->rating);
    return at->id;
}
