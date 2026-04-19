/* guild_system.c -- Guild system for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_GUILDS 500
#define MAX_GUILD_MEMBERS 500
#define MAX_GUILD_RANKS 10
#define GUILD_BANK_TABS 8
#define GUILD_BANK_SLOTS_PER_TAB 98

typedef struct GuildRank {
    char     name[32];
    uint32_t rights;      /* bitfield: invite, kick, promote, etc */
    uint32_t bankRights[GUILD_BANK_TABS];
    uint32_t withdrawPerDay;
} GuildRank;

typedef struct GuildMember {
    uint64_t guid;
    char     name[65];
    uint32_t rankIndex;
    uint32_t level;
    uint32_t class_;
    uint32_t zoneId;
    time_t   lastOnline;
    char     publicNote[128];
    char     officerNote[128];
    bool     online;
} GuildMember;

typedef struct Guild {
    uint32_t    id;
    char        name[64];
    uint64_t    leaderGuid;
    char        motd[256];           /* message of the day */
    char        info[512];           /* guild info text */
    time_t      createdDate;
    uint32_t    emblemStyle, emblemColor, borderStyle, borderColor, bgColor;
    uint64_t    bankMoney;
    GuildRank   ranks[MAX_GUILD_RANKS];
    int         rankCount;
    GuildMember members[MAX_GUILD_MEMBERS];
    int         memberCount;
    bool        active;
} Guild;

static Guild g_guilds[MAX_GUILDS];
static int g_guildCount = 0;

void GuildSystem_Init(void) {
    memset(g_guilds, 0, sizeof(g_guilds));
    g_guildCount = 0;
    printf("[Guild] Guild system initialized (max %d guilds)\n", MAX_GUILDS);
}

uint32_t GuildSystem_Create(const char* name, uint64_t leaderGuid, const char* leaderName) {
    if (g_guildCount >= MAX_GUILDS) return 0;
    Guild* g = &g_guilds[g_guildCount];
    g->id = (uint32_t)(g_guildCount + 1);
    strncpy(g->name, name, 63);
    g->leaderGuid = leaderGuid;
    g->createdDate = time(NULL);
    g->active = true;
    strncpy(g->motd, "Welcome to the guild!", 255);

    /* Default ranks */
    strncpy(g->ranks[0].name, "Guild Master", 31); g->ranks[0].rights = 0xFFFFFFFF;
    strncpy(g->ranks[1].name, "Officer", 31);      g->ranks[1].rights = 0x0000FFFF;
    strncpy(g->ranks[2].name, "Veteran", 31);       g->ranks[2].rights = 0x000000FF;
    strncpy(g->ranks[3].name, "Member", 31);         g->ranks[3].rights = 0x0000000F;
    strncpy(g->ranks[4].name, "Initiate", 31);       g->ranks[4].rights = 0x00000001;
    g->rankCount = 5;

    /* Add leader */
    GuildMember* m = &g->members[0];
    m->guid = leaderGuid;
    strncpy(m->name, leaderName, 64);
    m->rankIndex = 0;
    m->online = true;
    g->memberCount = 1;

    g_guildCount++;
    printf("[Guild] Created guild '%s' (ID %u) by %s\n", name, g->id, leaderName);
    return g->id;
}

Guild* GuildSystem_GetById(uint32_t guildId) {
    for (int i = 0; i < g_guildCount; i++)
        if (g_guilds[i].id == guildId && g_guilds[i].active) return &g_guilds[i];
    return NULL;
}

Guild* GuildSystem_GetByMember(uint64_t playerGuid) {
    for (int i = 0; i < g_guildCount; i++) {
        if (!g_guilds[i].active) continue;
        for (int j = 0; j < g_guilds[i].memberCount; j++)
            if (g_guilds[i].members[j].guid == playerGuid) return &g_guilds[i];
    }
    return NULL;
}

bool GuildSystem_Invite(uint32_t guildId, uint64_t guid, const char* name) {
    Guild* g = GuildSystem_GetById(guildId);
    if (!g || g->memberCount >= MAX_GUILD_MEMBERS) return false;
    GuildMember* m = &g->members[g->memberCount++];
    m->guid = guid; strncpy(m->name, name, 64);
    m->rankIndex = g->rankCount - 1; /* lowest rank */
    m->lastOnline = time(NULL);
    printf("[Guild] %s joined '%s'\n", name, g->name);
    return true;
}

bool GuildSystem_Kick(uint32_t guildId, uint64_t guid) {
    Guild* g = GuildSystem_GetById(guildId);
    if (!g) return false;
    for (int i = 0; i < g->memberCount; i++) {
        if (g->members[i].guid == guid) {
            printf("[Guild] %s removed from '%s'\n", g->members[i].name, g->name);
            g->members[i] = g->members[g->memberCount - 1];
            g->memberCount--;
            return true;
        }
    }
    return false;
}

void GuildSystem_Disband(uint32_t guildId) {
    Guild* g = GuildSystem_GetById(guildId);
    if (g) { g->active = false; printf("[Guild] '%s' disbanded\n", g->name); }
}

void GuildSystem_SetMotd(uint32_t guildId, const char* motd) {
    Guild* g = GuildSystem_GetById(guildId);
    if (g) strncpy(g->motd, motd, 255);
}
