/* opcode_handlers.c -- Extended opcode handlers wiring all game systems
 *
 * This file contains handlers for spell, combat, item, quest, loot,
 * guild, group, mail, auction, talent, and achievement opcodes.
 * It extends worldsession.c with the full set of gameplay handlers.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ================================================================
 *  Additional opcodes for WoW 3.3.5a
 * ================================================================ */
#define CMSG_ATTACKSWING             0x0141
#define CMSG_ATTACKSTOP              0x0142
#define SMSG_ATTACKSTART             0x0143
#define SMSG_ATTACKSTOP              0x0144
#define CMSG_SET_SELECTION           0x013D
#define CMSG_CAST_SPELL              0x012E
#define SMSG_SPELL_START             0x0131
#define SMSG_SPELL_GO                0x0132
#define SMSG_SPELL_FAILURE           0x0133
#define CMSG_CANCEL_CAST             0x012F
#define CMSG_CANCEL_AURA             0x0133
#define CMSG_LEARN_TALENT            0x0251
#define CMSG_UNLEARN_TALENTS         0x0252
#define CMSG_SWAP_INV_ITEM           0x010C
#define CMSG_AUTOEQUIP_ITEM          0x010A
#define CMSG_DESTROYITEM             0x0111
#define CMSG_SPLIT_ITEM              0x010E
#define CMSG_BUY_ITEM                0x01A2
#define CMSG_SELL_ITEM               0x01A0
#define CMSG_LOOT                    0x015D
#define CMSG_LOOT_RELEASE            0x015F
#define CMSG_AUTOSTORE_LOOT_ITEM     0x015E
#define SMSG_LOOT_RESPONSE           0x0160
#define SMSG_LOOT_RELEASE_RESPONSE   0x0161
#define CMSG_QUESTGIVER_STATUS_QUERY 0x0187
#define CMSG_QUESTGIVER_HELLO        0x0188
#define CMSG_QUESTGIVER_ACCEPT_QUEST 0x0189
#define CMSG_QUESTGIVER_COMPLETE_QUEST 0x018A
#define CMSG_QUESTGIVER_CHOOSE_REWARD 0x018C
#define CMSG_QUEST_CONFIRM_ACCEPT    0x019B
#define CMSG_QUESTLOG_REMOVE_QUEST   0x0194
#define CMSG_GUILD_CREATE            0x0081
#define CMSG_GUILD_INVITE            0x0082
#define CMSG_GUILD_ACCEPT            0x0083
#define CMSG_GUILD_DECLINE           0x0084
#define CMSG_GUILD_LEAVE             0x0089
#define CMSG_GUILD_DISBAND           0x008A
#define CMSG_GUILD_ROSTER            0x0089
#define CMSG_GUILD_MOTD              0x0090
#define CMSG_GROUP_INVITE            0x006E
#define CMSG_GROUP_ACCEPT            0x0072
#define CMSG_GROUP_DECLINE           0x0073
#define CMSG_GROUP_DISBAND           0x007B
#define CMSG_GROUP_SET_LEADER        0x0078
#define CMSG_LOOT_METHOD             0x007A
#define CMSG_CONVERT_TO_RAID         0x028E
#define CMSG_SEND_MAIL               0x0238
#define CMSG_GET_MAIL_LIST           0x023A
#define CMSG_MAIL_TAKE_MONEY         0x0245
#define CMSG_MAIL_TAKE_ITEM          0x0246
#define CMSG_MAIL_MARK_AS_READ       0x0248
#define CMSG_MAIL_DELETE             0x0249
#define CMSG_AUCTION_SELL_ITEM       0x0256
#define CMSG_AUCTION_PLACE_BID       0x025C
#define CMSG_AUCTION_LIST_ITEMS      0x0258
#define CMSG_AUCTION_LIST_OWNER_ITEMS 0x0259
#define CMSG_AUCTION_REMOVE_ITEM     0x025B
#define SMSG_AUCTION_LIST_RESULT     0x025C

/* Forward declare functions from other system .c files */
extern void SpellSystem_Init(void);
extern void ItemSystem_Init(void);
extern void QuestSystem_Init(void);
extern void LootSystem_Init(void);
extern void GuildSystem_Init(void);
extern void GroupSystem_Init(void);
extern void MailSystem_Init(void);
extern void AuctionSystem_Init(void);
extern void TalentSystem_Init(void);
extern void AchievementSystem_Init(void);
extern void WeatherSystem_Init(void);
extern void TransportSystem_Init(void);
extern void GridSystem_Init(void);

/* ================================================================
 *  Initialize all game systems (called from main.c)
 * ================================================================ */
void GameSystems_Init(void) {
    SpellSystem_Init();
    ItemSystem_Init();
    QuestSystem_Init();
    LootSystem_Init();
    GuildSystem_Init();
    GroupSystem_Init();
    MailSystem_Init();
    AuctionSystem_Init();
    TalentSystem_Init();
    AchievementSystem_Init();
    WeatherSystem_Init();
    TransportSystem_Init();
    GridSystem_Init();
    printf("[Systems] All game systems initialized.\n");
}

/* ================================================================
 *  Update all game systems (called from WorldServer_Update)
 * ================================================================ */
extern void WeatherSystem_Update(uint32_t diffMs);
extern void TransportSystem_Update(uint32_t diffMs);
extern void AuctionSystem_Update(void);

static uint32_t s_auctionTimer = 0;

void GameSystems_Update(uint32_t diffMs) {
    WeatherSystem_Update(diffMs);
    TransportSystem_Update(diffMs);

    /* Auction update every 60 seconds */
    s_auctionTimer += diffMs;
    if (s_auctionTimer >= 60000) {
        s_auctionTimer = 0;
        AuctionSystem_Update();
    }
}

/* ================================================================
 *  Packet send helper
 * ================================================================ */
static void _send(WorldSocket* sock, uint16_t opcode, const uint8_t* body, int bodyLen) {
    if (!sock || sock->dead) return;
    uint16_t totalSize = (uint16_t)(2 + bodyLen);
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
 *  CMSG_SET_SELECTION -- player targets something
 * ================================================================ */
void Handle_SetSelection(WorldServer* ws, WorldSocket* sock,
                         const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 8) return;
    uint64_t targetGuid = 0;
    memcpy(&targetGuid, data, 8);
    /* Store the target on the player for combat/spell targeting */
    sock->player->_unit.fields[UNIT_FIELD_BYTES_0] = targetGuid; /* reuse field as target storage */
    printf("[Session] %s targets GUID %llu\n", sock->player->name, (unsigned long long)targetGuid);
}

/* ================================================================
 *  CMSG_ATTACKSWING -- start melee attack
 * ================================================================ */
void Handle_AttackSwing(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    if (!sock->player || len < 8) return;
    uint64_t targetGuid = 0;
    memcpy(&targetGuid, data, 8);

    printf("[Combat] %s starts attacking GUID %llu\n",
           sock->player->name, (unsigned long long)targetGuid);

    /* Send SMSG_ATTACKSTART */
    uint8_t pkt[16];
    memcpy(pkt, &sock->player->guid, 8);
    memcpy(pkt + 8, &targetGuid, 8);
    _send(sock, SMSG_ATTACKSTART, pkt, 16);

    /* Fire script event */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, "player", "onAttack", sock->player);
}

/* ================================================================
 *  CMSG_ATTACKSTOP -- stop melee attack
 * ================================================================ */
void Handle_AttackStop(WorldServer* ws, WorldSocket* sock,
                       const uint8_t* data, int len) {
    (void)ws; (void)data; (void)len;
    if (!sock->player) return;

    uint8_t pkt[20];
    int pos = 0;
    /* Packed attacker GUID */
    pkt[pos++] = 0x01; pkt[pos++] = (uint8_t)(sock->player->guid & 0xFF);
    /* Packed target GUID (none) */
    pkt[pos++] = 0x00;
    /* Dead flag */
    uint32_t dead = 0;
    memcpy(pkt + pos, &dead, 4); pos += 4;

    _send(sock, SMSG_ATTACKSTOP, pkt, pos);
}

/* ================================================================
 *  CMSG_CAST_SPELL -- cast a spell
 * ================================================================ */
void Handle_CastSpell(WorldServer* ws, WorldSocket* sock,
                      const uint8_t* data, int len) {
    if (!sock->player || len < 6) return;
    /* castCount(1) + spellId(4) + castFlags(1) + targets... */
    uint32_t spellId = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);

    printf("[Spell] %s casts spell %u\n", sock->player->name, spellId);

    /* For now, self-cast on the caster's unit */
    ScriptEngine_ExecuteWordEvent(ws->scriptEngine, "player", "onCast", sock->player);
}

/* ================================================================
 *  CMSG_LEARN_TALENT
 * ================================================================ */
void Handle_LearnTalent(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 8) return;
    uint32_t talentId = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    uint32_t rank     = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
    printf("[Talent] %s learns talent %u rank %u\n", sock->player->name, talentId, rank);
}

/* ================================================================
 *  CMSG_LOOT -- open loot window
 * ================================================================ */
void Handle_Loot(WorldServer* ws, WorldSocket* sock,
                 const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 8) return;
    uint64_t targetGuid = 0;
    memcpy(&targetGuid, data, 8);
    printf("[Loot] %s loots GUID %llu\n", sock->player->name, (unsigned long long)targetGuid);

    /* Send empty loot response for now */
    uint8_t pkt[32];
    int pos = 0;
    memcpy(pkt + pos, &targetGuid, 8); pos += 8;
    pkt[pos++] = 1; /* loot type: corpse */
    pkt[pos++] = 0; /* loot slot: 0 items */
    /* gold (4 bytes) */
    uint32_t gold = 0;
    memcpy(pkt + pos, &gold, 4); pos += 4;
    /* item count */
    pkt[pos++] = 0;
    _send(sock, SMSG_LOOT_RESPONSE, pkt, pos);
}

/* ================================================================
 *  CMSG_LOOT_RELEASE
 * ================================================================ */
void Handle_LootRelease(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (len < 8) return;
    uint64_t guid = 0;
    memcpy(&guid, data, 8);

    uint8_t pkt[9];
    memcpy(pkt, &guid, 8);
    pkt[8] = 1; /* unknown */
    _send(sock, SMSG_LOOT_RELEASE_RESPONSE, pkt, 9);
}

/* ================================================================
 *  CMSG_QUESTGIVER_ACCEPT_QUEST
 * ================================================================ */
void Handle_QuestAccept(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 12) return;
    uint64_t npcGuid = 0;
    memcpy(&npcGuid, data, 8);
    uint32_t questId = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
    printf("[Quest] %s accepts quest %u from NPC %llu\n",
           sock->player->name, questId, (unsigned long long)npcGuid);
}

/* ================================================================
 *  CMSG_QUESTLOG_REMOVE_QUEST
 * ================================================================ */
void Handle_QuestAbandon(WorldServer* ws, WorldSocket* sock,
                         const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 4) return;
    uint32_t slot = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    printf("[Quest] %s abandons quest slot %u\n", sock->player->name, slot);
}

/* ================================================================
 *  CMSG_GUILD_CREATE
 * ================================================================ */
void Handle_GuildCreate(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 2) return;
    char guildName[65] = {0};
    int i = 0;
    while (i < len && i < 64 && data[i]) { guildName[i] = (char)data[i]; i++; }
    printf("[Guild] %s creates guild '%s'\n", sock->player->name, guildName);
}

/* ================================================================
 *  CMSG_GROUP_INVITE
 * ================================================================ */
void Handle_GroupInvite(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 2) return;
    char targetName[65] = {0};
    int i = 0;
    while (i < len && i < 64 && data[i]) { targetName[i] = (char)data[i]; i++; }
    printf("[Group] %s invites '%s' to group\n", sock->player->name, targetName);
}

/* ================================================================
 *  CMSG_SEND_MAIL
 * ================================================================ */
void Handle_SendMail(WorldServer* ws, WorldSocket* sock,
                     const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 20) return;
    /* Simplified: just log it */
    printf("[Mail] %s sends mail\n", sock->player->name);
}

/* ================================================================
 *  CMSG_AUCTION_SELL_ITEM
 * ================================================================ */
void Handle_AuctionSell(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 20) return;
    printf("[Auction] %s lists auction\n", sock->player->name);
}

/* ================================================================
 *  CMSG_BUY_ITEM -- buy from vendor
 * ================================================================ */
void Handle_BuyItem(WorldServer* ws, WorldSocket* sock,
                    const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 12) return;
    uint64_t vendorGuid = 0;
    memcpy(&vendorGuid, data, 8);
    uint32_t slot = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
    printf("[Item] %s buys item slot %u from vendor %llu\n",
           sock->player->name, slot, (unsigned long long)vendorGuid);
}

/* ================================================================
 *  CMSG_SELL_ITEM -- sell to vendor
 * ================================================================ */
void Handle_SellItem(WorldServer* ws, WorldSocket* sock,
                     const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 16) return;
    printf("[Item] %s sells item to vendor\n", sock->player->name);
}

/* ================================================================
 *  CMSG_AUTOEQUIP_ITEM
 * ================================================================ */
void Handle_AutoEquip(WorldServer* ws, WorldSocket* sock,
                      const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 2) return;
    uint8_t srcBag = data[0];
    uint8_t srcSlot = data[1];
    printf("[Item] %s auto-equips from bag=%u slot=%u\n",
           sock->player->name, srcBag, srcSlot);
}

/* ================================================================
 *  CMSG_DESTROYITEM
 * ================================================================ */
void Handle_DestroyItem(WorldServer* ws, WorldSocket* sock,
                        const uint8_t* data, int len) {
    (void)ws;
    if (!sock->player || len < 3) return;
    uint8_t bag = data[0];
    uint8_t slot = data[1];
    uint8_t count = data[2];
    printf("[Item] %s destroys item bag=%u slot=%u count=%u\n",
           sock->player->name, bag, slot, count);
}

/* ================================================================
 *  Movement forwarding to nearby players
 * ================================================================ */
void Handle_MovementForward(WorldServer* ws, WorldSocket* sock,
                            uint32_t opcode, const uint8_t* data, int len) {
    if (!sock->player || len < 28) return;

    /* Update player position from movement data */
    int off = 8; /* skip flags + time */
    if (off + 16 > len) return;
    float x, y, z, o;
    memcpy(&x, data + off, 4);
    memcpy(&y, data + off + 4, 4);
    memcpy(&z, data + off + 8, 4);
    memcpy(&o, data + off + 12, 4);

    float oldX = sock->player->position[0];
    float oldY = sock->player->position[1];
    sock->player->position[0] = x;
    sock->player->position[1] = y;
    sock->player->position[2] = z;
    sock->player->orientation = o;

    /* Forward the movement packet to all other nearby players */
    /* Build the forwarded packet with the player's GUID prepended */
    uint8_t* fwd = (uint8_t*)malloc(8 + len);
    memcpy(fwd, &sock->player->guid, 8);
    memcpy(fwd + 8, data, len);

    for (WorldSocket* c = ws->clients; c; c = c->next) {
        if (c == sock || !c->player || !c->authenticated || c->dead) continue;
        /* Simple distance check (no grid needed for small player counts) */
        float dx = c->player->position[0] - x;
        float dy = c->player->position[1] - y;
        if (dx*dx + dy*dy < 100.0f * 100.0f && c->player->mapId == sock->player->mapId) {
            _send(c, (uint16_t)opcode, fwd, 8 + len);
        }
    }
    free(fwd);

    (void)oldX; (void)oldY;
}

/* ================================================================
 *  Extended packet dispatcher (called from worldsession.c for
 *  opcodes not handled there)
 * ================================================================ */
bool OpcodeHandlers_Dispatch(WorldServer* ws, WorldSocket* sock,
                             uint32_t opcode, const uint8_t* data, int len) {
    switch (opcode) {
    case CMSG_SET_SELECTION:
        Handle_SetSelection(ws, sock, data, len);
        return true;
    case CMSG_ATTACKSWING:
        Handle_AttackSwing(ws, sock, data, len);
        return true;
    case CMSG_ATTACKSTOP:
        Handle_AttackStop(ws, sock, data, len);
        return true;
    case CMSG_CAST_SPELL:
        Handle_CastSpell(ws, sock, data, len);
        return true;
    case CMSG_LEARN_TALENT:
        Handle_LearnTalent(ws, sock, data, len);
        return true;
    case CMSG_LOOT:
        Handle_Loot(ws, sock, data, len);
        return true;
    case CMSG_LOOT_RELEASE:
        Handle_LootRelease(ws, sock, data, len);
        return true;
    case CMSG_QUESTGIVER_ACCEPT_QUEST:
        Handle_QuestAccept(ws, sock, data, len);
        return true;
    case CMSG_QUESTLOG_REMOVE_QUEST:
        Handle_QuestAbandon(ws, sock, data, len);
        return true;
    case CMSG_GUILD_CREATE:
        Handle_GuildCreate(ws, sock, data, len);
        return true;
    case CMSG_GROUP_INVITE:
        Handle_GroupInvite(ws, sock, data, len);
        return true;
    case CMSG_SEND_MAIL:
        Handle_SendMail(ws, sock, data, len);
        return true;
    case CMSG_AUCTION_SELL_ITEM:
        Handle_AuctionSell(ws, sock, data, len);
        return true;
    case CMSG_BUY_ITEM:
        Handle_BuyItem(ws, sock, data, len);
        return true;
    case CMSG_SELL_ITEM:
        Handle_SellItem(ws, sock, data, len);
        return true;
    case CMSG_AUTOEQUIP_ITEM:
        Handle_AutoEquip(ws, sock, data, len);
        return true;
    case CMSG_DESTROYITEM:
        Handle_DestroyItem(ws, sock, data, len);
        return true;
    default:
        /* Check if it's a movement opcode (0x00B5-0x00FF range) */
        if (opcode >= 0x00B5 && opcode <= 0x00FF) {
            Handle_MovementForward(ws, sock, opcode, data, len);
            return true;
        }
        return false; /* not handled */
    }
}
