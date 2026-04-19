/* worldsession.c -- WoW 3.3.5a packet dispatcher + session handlers
 *
 * Each connected client has a WorldSocket. This file routes incoming
 * opcodes to the correct handler and manages the per-player session
 * lifecycle (auth, char enum, login, chat, movement, gossip).
 */
#include "worldserver/world_server.h"
#include "scripting/script_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

/* ================================================================
 *  Helpers -- send a world packet to a single client
 * ================================================================ */
static void _send_pkt(WorldSocket* sock, uint16_t opcode,
                      const uint8_t* body, int bodyLen)
{
    if (!sock || sock->dead) return;

    /* WoW server packet header: size(2 BE) + opcode(2 LE) */
    uint16_t totalSize = (uint16_t)(2 + bodyLen); /* opcode(2) + body */
    uint8_t hdr[4];
    hdr[0] = (uint8_t)((totalSize >> 8) & 0xFF);
    hdr[1] = (uint8_t)(totalSize & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);

    send(sock->fd, (const char*)hdr, 4, 0);
    if (body && bodyLen > 0)
        send(sock->fd, (const char*)body, bodyLen, 0);
}

/* ================================================================
 *  CMSG_AUTH_SESSION -- client authenticates with the world server
 * ================================================================ */
static void _handle_auth_session(WorldServer* ws, WorldSocket* sock,
                                 const uint8_t* data, int len)
{
    /* Simplified: extract build + username from the packet.
     * Real WoW uses a crypto digest verified against the session key
     * exchanged with the auth server. */
    if (len < 8) return;

    uint32_t build = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    (void)build;

    /* Skip to username (offset varies; simplified: scan for first printable string) */
    char username[65] = {0};
    int upos = 8; /* skip build(4) + unk(4) */
    int j = 0;
    while (upos < len && j < 64) {
        char c = (char)data[upos++];
        if (c == 0) break;
        username[j++] = c;
    }
    username[j] = 0;

    /* Uppercase */
    for (int i = 0; username[i]; i++) {
        if (username[i] >= 'a' && username[i] <= 'z')
            username[i] -= 32;
    }

    printf("[Session] Auth from '%s'\n", username);
    sock->authenticated = true;
    sock->accountId = 1; /* Demo: account 1 */

    /* Send SMSG_AUTH_RESPONSE -- success */
    uint8_t resp[12];
    memset(resp, 0, sizeof(resp));
    resp[0] = 0x0C; /* AUTH_OK */
    /* BillingTimeRemaining(4), BillingFlags(1), BillingTimeResting(4), Expansion(1) */
    resp[9] = 2; /* Expansion = WotLK */
    _send_pkt(sock, (uint16_t)SMSG_AUTH_RESPONSE, resp, 12);

    /* Send SMSG_ADDON_INFO (empty) */
    _send_pkt(sock, (uint16_t)SMSG_ADDON_INFO, NULL, 0);

    /* Send SMSG_TUTORIAL_FLAGS (8 x uint32 = 32 bytes of zeros) */
    uint8_t tut[32];
    memset(tut, 0xFF, sizeof(tut)); /* All tutorials completed */
    _send_pkt(sock, (uint16_t)SMSG_TUTORIAL_FLAGS, tut, 32);

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, username, "onLogin", sock);
}

/* ================================================================
 *  CMSG_CHAR_ENUM -- list characters for this account
 * ================================================================ */
static void _handle_char_enum(WorldServer* ws, WorldSocket* sock,
                              const uint8_t* data, int len)
{
    (void)data; (void)len;
    printf("[Session] Char enum for account %u\n", sock->accountId);

    /* Build SMSG_CHAR_ENUM with one demo character */
    uint8_t pkt[512];
    int pos = 0;

    pkt[pos++] = 1; /* Number of characters = 1 */

    /* Character 1 */
    /* GUID (8 bytes LE) */
    uint64_t guid = 1;
    memcpy(pkt + pos, &guid, 8); pos += 8;

    /* Name (null-terminated) */
    const char* charName = "TestChar";
    memcpy(pkt + pos, charName, strlen(charName) + 1);
    pos += (int)strlen(charName) + 1;

    /* Race(1), Class(1), Gender(1) */
    pkt[pos++] = 1;  /* Human */
    pkt[pos++] = 1;  /* Warrior */
    pkt[pos++] = 0;  /* Male */

    /* Skin(1), Face(1), HairStyle(1), HairColor(1), FacialHair(1) */
    pkt[pos++] = 0; pkt[pos++] = 0; pkt[pos++] = 0;
    pkt[pos++] = 0; pkt[pos++] = 0;

    /* Level (1 byte) */
    pkt[pos++] = 1;

    /* Zone (4 bytes) */
    uint32_t zone = 12; /* Elwynn Forest */
    memcpy(pkt + pos, &zone, 4); pos += 4;

    /* Map (4 bytes) */
    uint32_t map = 0; /* Eastern Kingdoms */
    memcpy(pkt + pos, &map, 4); pos += 4;

    /* Position X, Y, Z (floats) */
    float px = -8949.95f, py = -132.49f, pz = 83.53f;
    memcpy(pkt + pos, &px, 4); pos += 4;
    memcpy(pkt + pos, &py, 4); pos += 4;
    memcpy(pkt + pos, &pz, 4); pos += 4;

    /* Guild ID (4 bytes) */
    uint32_t guildId = 0;
    memcpy(pkt + pos, &guildId, 4); pos += 4;

    /* Character flags (4 bytes) */
    uint32_t charFlags = 0;
    memcpy(pkt + pos, &charFlags, 4); pos += 4;

    /* First login (4 bytes -- actually "at login flags") */
    uint32_t atLogin = 0;
    memcpy(pkt + pos, &atLogin, 4); pos += 4;

    /* Pet info (12 bytes: displayId + level + family) */
    memset(pkt + pos, 0, 12); pos += 12;

    /* Equipment (23 slots x 12 bytes = 276 bytes) */
    /* Each slot: displayId(4) + inventoryType(1) + enchant(4) = 9 bytes
     * Actually WoW 3.3.5 uses: displayId(4) + invType(1) + auraId(4) = 9 bytes per slot
     * 23 slots * 9 = 207 bytes -- but for simplicity send zeros */
    memset(pkt + pos, 0, 23 * 9); pos += 23 * 9;

    /* First bag display info (4 bytes) */
    memset(pkt + pos, 0, 4); pos += 4;

    _send_pkt(sock, (uint16_t)SMSG_CHAR_ENUM, pkt, pos);
}

/* ================================================================
 *  CMSG_CHAR_CREATE -- create a new character
 * ================================================================ */
static void _handle_char_create(WorldServer* ws, WorldSocket* sock,
                                const uint8_t* data, int len)
{
    (void)ws;
    printf("[Session] Char create request\n");

    /* Parse character name from data (null-terminated string at start) */
    char name[65] = {0};
    int i = 0;
    while (i < len && i < 64 && data[i] != 0) {
        name[i] = (char)data[i];
        i++;
    }
    printf("[Session] Creating character: %s\n", name);

    /* Respond with success */
    uint8_t resp[1] = { 0x00 }; /* CHAR_CREATE_SUCCESS */
    _send_pkt(sock, (uint16_t)SMSG_CHAR_CREATE, resp, 1);
}

/* ================================================================
 *  CMSG_PLAYER_LOGIN -- enter the world with a character
 * ================================================================ */
static void _handle_player_login(WorldServer* ws, WorldSocket* sock,
                                 const uint8_t* data, int len)
{
    if (len < 8) return;

    uint64_t guid = 0;
    memcpy(&guid, data, 8);
    printf("[Session] Player login: GUID=%llu\n", (unsigned long long)guid);

    /* Create player object */
    Player* p = (Player*)calloc(1, sizeof(Player));
    p->guid = guid;
    strncpy(p->name, "TestChar", sizeof(p->name) - 1);
    p->accountId = sock->accountId;
    p->race = 1;    /* Human */
    p->class_ = 1;  /* Warrior */
    p->level = 1;
    p->mapId = 0;   /* Eastern Kingdoms */
    p->position[0] = -8949.95f;
    p->position[1] = -132.49f;
    p->position[2] = 83.53f;
    p->orientation = 0.0f;
    p->isInWorld = true;
    p->session = sock;
    sock->player = p;
    sock->guid = guid;

    /* Send SMSG_LOGIN_VERIFY_WORLD */
    uint8_t verify[20];
    int pos = 0;
    /* MapId (4 LE) */
    uint32_t mapId = p->mapId;
    memcpy(verify + pos, &mapId, 4); pos += 4;
    /* Position X, Y, Z, O */
    memcpy(verify + pos, &p->position[0], 4); pos += 4;
    memcpy(verify + pos, &p->position[1], 4); pos += 4;
    memcpy(verify + pos, &p->position[2], 4); pos += 4;
    memcpy(verify + pos, &p->orientation, 4); pos += 4;

    _send_pkt(sock, (uint16_t)SMSG_LOGIN_VERIFY_WORLD, verify, 20);

    /* Send SMSG_ACCOUNT_DATA_TIMES (128 bytes of zeros) */
    uint8_t adtimes[128];
    memset(adtimes, 0, sizeof(adtimes));
    _send_pkt(sock, (uint16_t)SMSG_ACCOUNT_DATA_TIMES, adtimes, 128);

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, "player", "onLogin", p);

    printf("[Session] Player '%s' entered the world at (%.1f, %.1f, %.1f) map %u\n",
           p->name, p->position[0], p->position[1], p->position[2], p->mapId);
}

/* ================================================================
 *  CMSG_NAME_QUERY -- lookup a player name by GUID
 * ================================================================ */
static void _handle_name_query(WorldServer* ws, WorldSocket* sock,
                               const uint8_t* data, int len)
{
    if (len < 8) return;
    uint64_t guid = 0;
    memcpy(&guid, data, 8);

    /* Find player */
    const char* name = "Unknown";
    uint8_t race = 1, gender = 0, cls = 1;

    Player* p = WorldServer_GetPlayer(ws, guid);
    if (p) {
        name = p->name;
        race = (uint8_t)p->race;
        cls  = (uint8_t)p->class_;
    }

    /* Build SMSG_NAME_QUERY_RESPONSE */
    uint8_t pkt[256];
    int pos = 0;

    /* PackedGUID */
    memcpy(pkt + pos, &guid, 8); pos += 8;

    /* DeclinedNames (0 = no) */
    pkt[pos++] = 0;

    /* Name (null-terminated) */
    int nlen = (int)strlen(name);
    memcpy(pkt + pos, name, nlen + 1); pos += nlen + 1;

    /* RealmName (null-terminated, empty for same realm) */
    pkt[pos++] = 0;

    /* Race, Gender, Class */
    pkt[pos++] = race;
    pkt[pos++] = gender;
    pkt[pos++] = cls;

    _send_pkt(sock, (uint16_t)SMSG_NAME_QUERY_RESPONSE, pkt, pos);
}

/* ================================================================
 *  CMSG_MESSAGECHAT -- player sends a chat message
 * ================================================================ */
static void _handle_chat(WorldServer* ws, WorldSocket* sock,
                         const uint8_t* data, int len)
{
    if (!sock->player || len < 9) return;

    /* Type(4) + Language(4) + message... */
    uint32_t chatType = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    uint32_t lang     = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
    (void)lang;

    /* Extract message (null-terminated after some target data) */
    const char* msg = NULL;
    int msgStart = 8;

    /* For SAY/YELL/EMOTE: Language(4) + msgLen(4) + msg */
    if (chatType == 0 || chatType == 1 || chatType == 8) { /* SAY, PARTY, EMOTE */
        if (len > msgStart + 4) {
            uint32_t msgLen = data[msgStart] | (data[msgStart+1] << 8) |
                             (data[msgStart+2] << 16) | (data[msgStart+3] << 24);
            msgStart += 4;
            if (msgStart + (int)msgLen <= len) {
                msg = (const char*)(data + msgStart);
            }
        }
    }

    if (!msg) msg = "(empty)";

    printf("[Chat] [%s]: %s\n", sock->player->name, msg);

    /* Check for server commands (prefixed with .) */
    if (msg[0] == '.') {
        if (strncmp(msg, ".reloadscripts", 14) == 0) {
            printf("[CMD] Reloading scripts...\n");
            WorldServer_Broadcast(ws, "Scripts reloaded.");
        } else if (strncmp(msg, ".info", 5) == 0) {
            char info[256];
            snprintf(info, sizeof(info), "Uptime: %s | Players online: 1",
                     WorldServer_GetUptime(ws));
            WorldServer_Broadcast(ws, info);
        } else if (strncmp(msg, ".spawn ", 7) == 0) {
            uint32_t entry = (uint32_t)atoi(msg + 7);
            if (entry > 0 && sock->player) {
                WorldServer_SpawnCreature(ws, entry, sock->player->mapId,
                    sock->player->position[0], sock->player->position[1],
                    sock->player->position[2], sock->player->orientation);
                WorldServer_Broadcast(ws, "Creature spawned.");
            }
        }
        return;
    }

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, "player", "onSay", sock->player);

    /* Broadcast the chat to all players */
    size_t msgLen = strlen(msg);
    uint8_t* pkt = (uint8_t*)calloc(1, 256 + msgLen);
    int pos = 0;

    /* ChatType(1) */
    pkt[pos++] = (uint8_t)chatType;
    /* Language(4) */
    pkt[pos++] = 0; pkt[pos++] = 0; pkt[pos++] = 0; pkt[pos++] = 0;
    /* SenderGUID(8) */
    memcpy(pkt + pos, &sock->player->guid, 8); pos += 8;
    /* Flags(4) */
    memset(pkt + pos, 0, 4); pos += 4;
    /* TargetGUID(8) */
    memcpy(pkt + pos, &sock->player->guid, 8); pos += 8;
    /* MessageLength(4) */
    uint32_t ml = (uint32_t)(msgLen + 1);
    memcpy(pkt + pos, &ml, 4); pos += 4;
    /* Message */
    memcpy(pkt + pos, msg, msgLen + 1); pos += (int)msgLen + 1;
    /* ChatTag(1) */
    pkt[pos++] = 0;

    /* Send to all authenticated clients */
    EnterCriticalSection(&ws->lock);
    for (WorldSocket* c = ws->clients; c; c = c->next) {
        if (c->authenticated && !c->dead)
            _send_pkt(c, (uint16_t)SMSG_MESSAGECHAT, pkt, pos);
    }
    LeaveCriticalSection(&ws->lock);
    free(pkt);
}

/* ================================================================
 *  CMSG_PING
 * ================================================================ */
static void _handle_ping(WorldServer* ws, WorldSocket* sock,
                         const uint8_t* data, int len)
{
    (void)ws;
    if (len < 4) return;
    /* Echo the sequence back */
    _send_pkt(sock, (uint16_t)SMSG_PONG, data, 4);
}

/* ================================================================
 *  CMSG_QUERY_TIME
 * ================================================================ */
static void _handle_query_time(WorldServer* ws, WorldSocket* sock,
                               const uint8_t* data, int len)
{
    (void)ws; (void)data; (void)len;
    uint8_t resp[8];
    uint32_t t = (uint32_t)time(NULL);
    memcpy(resp, &t, 4);
    uint32_t dailyReset = 0;
    memcpy(resp + 4, &dailyReset, 4);
    _send_pkt(sock, (uint16_t)SMSG_QUERY_TIME_RESPONSE, resp, 8);
}

/* ================================================================
 *  CMSG_LOGOUT_REQUEST
 * ================================================================ */
static void _handle_logout(WorldServer* ws, WorldSocket* sock,
                           const uint8_t* data, int len)
{
    (void)ws; (void)data; (void)len;
    printf("[Session] Player logout: %s\n",
           sock->player ? sock->player->name : "(none)");

    /* Fire script event */
    if (sock->player)
        ScriptEngine_ExecuteWordEvent(ws->scriptEngine, "player", "onLogout", sock->player);

    /* Send SMSG_LOGOUT_COMPLETE (opcode 0x004E, no body) */
    _send_pkt(sock, 0x004E, NULL, 0);

    if (sock->player) {
        free(sock->player);
        sock->player = NULL;
    }
    sock->authenticated = false;
}

/* ================================================================
 *  CMSG_MOVE_HEARTBEAT -- position update from client
 * ================================================================ */
static void _handle_movement(WorldServer* ws, WorldSocket* sock,
                             const uint8_t* data, int len)
{
    (void)ws;
    if (!sock->player || len < 28) return;

    /* Movement packets have movementFlags(4), time(4), then position:
     * x(4) y(4) z(4) o(4) */
    int off = 8; /* skip flags + time */
    if (off + 16 > len) return;

    float x, y, z, o;
    memcpy(&x, data + off, 4);      off += 4;
    memcpy(&y, data + off, 4);      off += 4;
    memcpy(&z, data + off, 4);      off += 4;
    memcpy(&o, data + off, 4);

    sock->player->position[0] = x;
    sock->player->position[1] = y;
    sock->player->position[2] = z;
    sock->player->orientation = o;
}

/* ================================================================
 *  CMSG_CREATURE_QUERY
 * ================================================================ */
static void _handle_creature_query(WorldServer* ws, WorldSocket* sock,
                                   const uint8_t* data, int len)
{
    if (len < 12) return;
    uint32_t entry = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    uint64_t guid  = 0;
    memcpy(&guid, data + 4, 8);

    /* Look up creature */
    char name[64] = "Unknown Creature";
    Unit* u = WorldServer_FindUnit(ws, guid);
    if (u) strncpy(name, u->name, sizeof(name) - 1);

    /* Build response */
    uint8_t pkt[512];
    int pos = 0;

    memcpy(pkt + pos, &entry, 4); pos += 4;

    /* 4 Name strings (null-terminated) */
    int nlen = (int)strlen(name);
    memcpy(pkt + pos, name, nlen + 1); pos += nlen + 1;
    pkt[pos++] = 0; /* name2 */
    pkt[pos++] = 0; /* name3 */
    pkt[pos++] = 0; /* name4 */

    /* SubName */
    pkt[pos++] = 0;

    /* IconName */
    pkt[pos++] = 0;

    /* TypeFlags(4), Type(4), Family(4), Rank(4), KillCredit1(4), KillCredit2(4) */
    memset(pkt + pos, 0, 24); pos += 24;

    /* DisplayId (4 x uint32) */
    uint32_t dispId = u ? u->displayId : 0;
    memcpy(pkt + pos, &dispId, 4); pos += 4;
    memset(pkt + pos, 0, 12); pos += 12; /* 3 more display IDs */

    /* HealthMultiplier(4), ManaMultiplier(4) */
    float hpMult = 1.0f, manaMult = 1.0f;
    memcpy(pkt + pos, &hpMult, 4); pos += 4;
    memcpy(pkt + pos, &manaMult, 4); pos += 4;

    /* RacialLeader(1) */
    pkt[pos++] = 0;

    /* QuestItems (6 x uint32) + MovementId(4) */
    memset(pkt + pos, 0, 28); pos += 28;

    _send_pkt(sock, (uint16_t)SMSG_CREATURE_QUERY_RESPONSE, pkt, pos);
}

/* ================================================================
 *  CMSG_GOSSIP_HELLO -- NPC interaction
 * ================================================================ */
static void _handle_gossip_hello(WorldServer* ws, WorldSocket* sock,
                                 const uint8_t* data, int len)
{
    if (len < 8) return;
    uint64_t npcGuid = 0;
    memcpy(&npcGuid, data, 8);

    Unit* npc = WorldServer_FindUnit(ws, npcGuid);
    const char* npcName = npc ? npc->name : "Unknown";

    printf("[Gossip] Player %s talks to NPC %s (GUID %llu)\n",
           sock->player ? sock->player->name : "?",
           npcName, (unsigned long long)npcGuid);

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, npcName, "onGossip",
                                 sock->player);

    /* Send a simple gossip menu */
    uint8_t pkt[256];
    int pos = 0;

    /* NPC GUID */
    memcpy(pkt + pos, &npcGuid, 8); pos += 8;
    /* MenuId */
    uint32_t menuId = 0;
    memcpy(pkt + pos, &menuId, 4); pos += 4;
    /* TextId (references npc_text table) */
    uint32_t textId = 1;
    memcpy(pkt + pos, &textId, 4); pos += 4;
    /* Number of gossip items */
    uint32_t itemCount = 0;
    memcpy(pkt + pos, &itemCount, 4); pos += 4;
    /* Number of quest items */
    uint32_t questCount = 0;
    memcpy(pkt + pos, &questCount, 4); pos += 4;

    _send_pkt(sock, (uint16_t)SMSG_GOSSIP_MESSAGE, pkt, pos);
}

/* ================================================================
 *  Main packet dispatcher
 * ================================================================ */
void WorldSession_HandlePacket(WorldServer* ws, WorldSocket* sock,
                               uint32_t opcode,
                               const uint8_t* data, int dataLen)
{
    switch (opcode) {
    case CMSG_AUTH_SESSION:
        _handle_auth_session(ws, sock, data, dataLen);
        break;
    case CMSG_CHAR_ENUM:
        _handle_char_enum(ws, sock, data, dataLen);
        break;
    case CMSG_CHAR_CREATE:
        _handle_char_create(ws, sock, data, dataLen);
        break;
    case CMSG_PLAYER_LOGIN:
        _handle_player_login(ws, sock, data, dataLen);
        break;
    case CMSG_NAME_QUERY:
        _handle_name_query(ws, sock, data, dataLen);
        break;
    case CMSG_PING:
        _handle_ping(ws, sock, data, dataLen);
        break;
    case CMSG_QUERY_TIME:
        _handle_query_time(ws, sock, data, dataLen);
        break;
    case CMSG_LOGOUT_REQUEST:
        _handle_logout(ws, sock, data, dataLen);
        break;
    case CMSG_MESSAGECHAT:
        _handle_chat(ws, sock, data, dataLen);
        break;
    case CMSG_MOVE_HEARTBEAT:
    case CMSG_MOVE_WORLDPORT_ACK:
        _handle_movement(ws, sock, data, dataLen);
        break;
    case CMSG_CREATURE_QUERY:
        _handle_creature_query(ws, sock, data, dataLen);
        break;
    case CMSG_GOSSIP_HELLO:
        _handle_gossip_hello(ws, sock, data, dataLen);
        break;
    default:
        /* Silently ignore unknown opcodes -- there are hundreds
         * of WoW opcodes, most of which are not needed for a
         * basic functional server. */
        break;
    }
}
