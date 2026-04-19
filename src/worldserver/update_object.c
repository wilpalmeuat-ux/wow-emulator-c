/* update_object.c -- SMSG_UPDATE_OBJECT packet builder for WoW 3.3.5a
 *
 * The most complex packet in WoW. Builds object create/update/destroy
 * blocks that tell the client about players, NPCs, and game objects.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Update types */
#define UPDATETYPE_VALUES       0
#define UPDATETYPE_MOVEMENT     1
#define UPDATETYPE_CREATE_OBJECT 2
#define UPDATETYPE_CREATE_OBJECT2 3 /* for self */
#define UPDATETYPE_OUT_OF_RANGE 4
#define UPDATETYPE_NEAR_OBJECTS 5

/* Object types */
#define TYPEMASK_OBJECT      0x0001
#define TYPEMASK_ITEM        0x0002
#define TYPEMASK_CONTAINER   0x0004
#define TYPEMASK_UNIT        0x0008
#define TYPEMASK_PLAYER      0x0010
#define TYPEMASK_GAMEOBJECT  0x0020
#define TYPEMASK_DYNAMICOBJ  0x0040
#define TYPEMASK_CORPSE      0x0080

/* Movement flags */
#define MOVEFLAG_NONE        0x00000000
#define MOVEFLAG_FORWARD     0x00000001
#define MOVEFLAG_BACKWARD    0x00000002
#define MOVEFLAG_STRAFE_LEFT 0x00000004
#define MOVEFLAG_STRAFE_RIGHT 0x00000008

/* Update fields (WoW 3.3.5 field offsets -- simplified subset) */
#define OBJECT_FIELD_GUID           0x0000
#define OBJECT_FIELD_TYPE           0x0002
#define OBJECT_FIELD_ENTRY          0x0003
#define OBJECT_FIELD_SCALE_X        0x0004
#define UNIT_FIELD_CHARM            0x0006
#define UNIT_FIELD_SUMMON           0x0008
#define UNIT_FIELD_CHARMEDBY        0x000A
#define UNIT_FIELD_SUMMONEDBY       0x000C
#define UNIT_FIELD_CREATEDBY        0x000E
#define UNIT_FIELD_TARGET           0x0010
#define UNIT_FIELD_PERSUADED        0x0012
#define UNIT_FIELD_CHANNEL_OBJECT   0x0014
#define UF_HEALTH                   0x0018
#define UF_MAXHEALTH                0x001A
#define UF_LEVEL                    0x001C
#define UF_FACTIONTEMPLATE          0x001E
#define UF_DISPLAYID                0x0020
#define UF_NATIVEDISPLAYID          0x0022
#define UF_FLAGS                    0x002A
#define UF_BASEATTACKTIME           0x002C
#define UF_BOUNDINGRADIUS           0x002E
#define UF_COMBATREACH              0x002F
#define UF_MINDAMAGE                0x0050
#define UF_MAXDAMAGE                0x0051
#define UF_MOD_CAST_SPEED           0x0092
#define PLAYER_FIELD_BYTES          0x00C4
#define PLAYER_FIELD_BYTES2         0x00C8
#define PLAYER_XP                   0x00CC
#define PLAYER_NEXT_LEVEL_XP        0x00CD

#define MAX_UPDATE_FIELDS 0x0500  /* max field index */
#define UPDATE_MASK_BLOCKS ((MAX_UPDATE_FIELDS + 31) / 32)

/* ================================================================
 *  UpdateMask -- bitmask tracking which fields are set
 * ================================================================ */
typedef struct {
    uint32_t bits[UPDATE_MASK_BLOCKS];
    uint32_t values[MAX_UPDATE_FIELDS];
    int      fieldCount;
} UpdateMask;

static void UM_Init(UpdateMask* um) {
    memset(um, 0, sizeof(UpdateMask));
    um->fieldCount = MAX_UPDATE_FIELDS;
}

static void UM_SetBit(UpdateMask* um, int field) {
    if (field >= 0 && field < MAX_UPDATE_FIELDS)
        um->bits[field / 32] |= (1u << (field % 32));
}

static void UM_SetUInt32(UpdateMask* um, int field, uint32_t val) {
    if (field < 0 || field >= MAX_UPDATE_FIELDS) return;
    um->values[field] = val;
    UM_SetBit(um, field);
}

static void UM_SetUInt64(UpdateMask* um, int field, uint64_t val) {
    UM_SetUInt32(um, field, (uint32_t)(val & 0xFFFFFFFF));
    UM_SetUInt32(um, field + 1, (uint32_t)(val >> 32));
}

static void UM_SetFloat(UpdateMask* um, int field, float val) {
    union { float f; uint32_t u; } conv;
    conv.f = val;
    UM_SetUInt32(um, field, conv.u);
}

/* Find the highest set block index */
static int UM_BlockCount(UpdateMask* um) {
    int highest = 0;
    for (int i = 0; i < UPDATE_MASK_BLOCKS; i++) {
        if (um->bits[i]) highest = i + 1;
    }
    return highest;
}

/* Write the update mask + values to a buffer. Returns bytes written. */
static int UM_WriteTo(UpdateMask* um, uint8_t* out, int maxLen) {
    int blockCount = UM_BlockCount(um);
    int pos = 0;

    /* Block count (1 byte) */
    if (pos + 1 > maxLen) return 0;
    out[pos++] = (uint8_t)blockCount;

    /* Mask bits */
    for (int i = 0; i < blockCount; i++) {
        if (pos + 4 > maxLen) return 0;
        memcpy(out + pos, &um->bits[i], 4);
        pos += 4;
    }

    /* Values for set bits */
    for (int i = 0; i < blockCount * 32 && i < MAX_UPDATE_FIELDS; i++) {
        if (um->bits[i / 32] & (1u << (i % 32))) {
            if (pos + 4 > maxLen) return 0;
            memcpy(out + pos, &um->values[i], 4);
            pos += 4;
        }
    }
    return pos;
}

/* ================================================================
 *  WritePackedGuid -- compressed GUID format used in update packets
 * ================================================================ */
static int WritePackedGuid(uint8_t* out, uint64_t guid) {
    uint8_t mask = 0;
    uint8_t packed[8];
    int pcount = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t byte = (uint8_t)((guid >> (i * 8)) & 0xFF);
        if (byte) {
            mask |= (1 << i);
            packed[pcount++] = byte;
        }
    }
    out[0] = mask;
    memcpy(out + 1, packed, pcount);
    return 1 + pcount;
}

/* ================================================================
 *  Build a CREATE_OBJECT block for a player (self-view)
 * ================================================================ */
int UpdateObject_BuildPlayerCreate(uint8_t* out, int maxLen, Player* player) {
    if (!player || maxLen < 256) return 0;
    int pos = 0;

    /* Update type */
    out[pos++] = UPDATETYPE_CREATE_OBJECT2; /* self */

    /* Packed GUID */
    pos += WritePackedGuid(out + pos, player->guid);

    /* Object type: 0x19 = TYPEID_PLAYER */
    out[pos++] = 4; /* TYPEID_PLAYER */

    /* Movement block flags */
    /* UpdateFlag: 0x71 = SELF|LIVING|HAS_POSITION|STATIONARY */
    out[pos++] = 0x71;

    /* Movement flags (4 bytes) */
    uint32_t moveFlags = MOVEFLAG_NONE;
    memcpy(out + pos, &moveFlags, 4); pos += 4;

    /* Extra movement flags (2 bytes) */
    uint16_t moveFlags2 = 0;
    memcpy(out + pos, &moveFlags2, 2); pos += 2;

    /* Timestamp (4 bytes) */
    uint32_t timestamp = 0;
    memcpy(out + pos, &timestamp, 4); pos += 4;

    /* Position X, Y, Z, O */
    memcpy(out + pos, &player->position[0], 4); pos += 4;
    memcpy(out + pos, &player->position[1], 4); pos += 4;
    memcpy(out + pos, &player->position[2], 4); pos += 4;
    memcpy(out + pos, &player->orientation, 4); pos += 4;

    /* Fall time */
    uint32_t fallTime = 0;
    memcpy(out + pos, &fallTime, 4); pos += 4;

    /* Speeds (9 floats: walk, run, runBack, swim, swimBack, fly, flyBack, turnRate, pitchRate) */
    float speeds[] = { 2.5f, 7.0f, 4.5f, 4.72f, 2.5f, 7.0f, 4.5f, 3.14f, 3.14f };
    for (int i = 0; i < 9; i++) {
        memcpy(out + pos, &speeds[i], 4); pos += 4;
    }

    /* Update fields */
    UpdateMask um;
    UM_Init(&um);

    UM_SetUInt64(&um, OBJECT_FIELD_GUID, player->guid);
    UM_SetUInt32(&um, OBJECT_FIELD_TYPE, TYPEMASK_OBJECT | TYPEMASK_UNIT | TYPEMASK_PLAYER);
    UM_SetFloat(&um, OBJECT_FIELD_SCALE_X, 1.0f);
    UM_SetUInt32(&um, UF_HEALTH, 100);
    UM_SetUInt32(&um, UF_MAXHEALTH, 100);
    UM_SetUInt32(&um, UF_LEVEL, player->level);
    UM_SetUInt32(&um, UF_FACTIONTEMPLATE, 1); /* Human */
    UM_SetUInt32(&um, UF_DISPLAYID, 49 + player->race); /* Basic display */
    UM_SetUInt32(&um, UF_NATIVEDISPLAYID, 49 + player->race);
    UM_SetFloat(&um, UF_BOUNDINGRADIUS, 0.389f);
    UM_SetFloat(&um, UF_COMBATREACH, 1.5f);
    UM_SetFloat(&um, UF_MINDAMAGE, 5.0f);
    UM_SetFloat(&um, UF_MAXDAMAGE, 10.0f);
    UM_SetFloat(&um, UF_MOD_CAST_SPEED, 1.0f);
    UM_SetUInt32(&um, PLAYER_XP, 0);
    UM_SetUInt32(&um, PLAYER_NEXT_LEVEL_XP, 400);

    /* Race/class/gender packed into PLAYER_FIELD_BYTES */
    uint32_t playerBytes = player->race | (player->class_ << 8) | (0 << 16) | (0 << 24);
    UM_SetUInt32(&um, PLAYER_FIELD_BYTES, playerBytes);

    int maskLen = UM_WriteTo(&um, out + pos, maxLen - pos);
    pos += maskLen;

    return pos;
}

/* ================================================================
 *  Build a CREATE_OBJECT block for a creature
 * ================================================================ */
int UpdateObject_BuildCreatureCreate(uint8_t* out, int maxLen, Unit* unit) {
    if (!unit || maxLen < 256) return 0;
    int pos = 0;

    out[pos++] = UPDATETYPE_CREATE_OBJECT;
    pos += WritePackedGuid(out + pos, unit->guid);
    out[pos++] = 3; /* TYPEID_UNIT */

    /* UpdateFlag: 0x70 = LIVING|HAS_POSITION|STATIONARY (no SELF) */
    out[pos++] = 0x70;

    /* Movement flags */
    uint32_t moveFlags = 0;
    memcpy(out + pos, &moveFlags, 4); pos += 4;
    uint16_t moveFlags2 = 0;
    memcpy(out + pos, &moveFlags2, 2); pos += 2;
    uint32_t timestamp = 0;
    memcpy(out + pos, &timestamp, 4); pos += 4;

    /* Position */
    memcpy(out + pos, &unit->position[0], 4); pos += 4;
    memcpy(out + pos, &unit->position[1], 4); pos += 4;
    memcpy(out + pos, &unit->position[2], 4); pos += 4;
    memcpy(out + pos, &unit->orientation, 4); pos += 4;

    uint32_t fallTime = 0;
    memcpy(out + pos, &fallTime, 4); pos += 4;

    float speeds[] = { 2.5f, unit->speedRun, 4.5f, 4.72f, 2.5f, 7.0f, 4.5f, 3.14f, 3.14f };
    for (int i = 0; i < 9; i++) {
        memcpy(out + pos, &speeds[i], 4); pos += 4;
    }

    UpdateMask um;
    UM_Init(&um);

    UM_SetUInt64(&um, OBJECT_FIELD_GUID, unit->guid);
    UM_SetUInt32(&um, OBJECT_FIELD_TYPE, TYPEMASK_OBJECT | TYPEMASK_UNIT);
    UM_SetUInt32(&um, OBJECT_FIELD_ENTRY, unit->entry);
    UM_SetFloat(&um, OBJECT_FIELD_SCALE_X, 1.0f);
    UM_SetUInt32(&um, UF_HEALTH, (uint32_t)unit->fields[UNIT_FIELD_HEALTH]);
    UM_SetUInt32(&um, UF_MAXHEALTH, (uint32_t)unit->fields[UNIT_FIELD_MAXHEALTH]);
    UM_SetUInt32(&um, UF_LEVEL, unit->level);
    UM_SetUInt32(&um, UF_FACTIONTEMPLATE, unit->faction);
    UM_SetUInt32(&um, UF_DISPLAYID, unit->displayId ? unit->displayId : 1);
    UM_SetUInt32(&um, UF_NATIVEDISPLAYID, unit->displayId ? unit->displayId : 1);
    UM_SetFloat(&um, UF_BOUNDINGRADIUS, unit->boundingRadius);
    UM_SetFloat(&um, UF_COMBATREACH, 1.5f);
    UM_SetFloat(&um, UF_MOD_CAST_SPEED, 1.0f);

    int maskLen = UM_WriteTo(&um, out + pos, maxLen - pos);
    pos += maskLen;

    return pos;
}

/* ================================================================
 *  Send SMSG_UPDATE_OBJECT with a create block to a client
 * ================================================================ */
void UpdateObject_SendCreatePlayer(WorldSocket* sock, Player* player) {
    if (!sock || !player) return;

    uint8_t block[8192];
    int blockLen = UpdateObject_BuildPlayerCreate(block, sizeof(block), player);
    if (blockLen <= 0) return;

    /* SMSG_UPDATE_OBJECT header: count(4) + hasTransport(1) + blocks */
    uint8_t pkt[8200];
    int pos = 0;
    uint32_t count = 1;
    memcpy(pkt + pos, &count, 4); pos += 4;
    pkt[pos++] = 0; /* no transport */
    memcpy(pkt + pos, block, blockLen); pos += blockLen;

    /* Send via world socket */
    uint16_t opcode = (uint16_t)SMSG_UPDATE_OBJECT;
    uint16_t pktSize = (uint16_t)(2 + pos);
    uint8_t hdr[4];
    hdr[0] = (uint8_t)((pktSize >> 8) & 0xFF);
    hdr[1] = (uint8_t)(pktSize & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);

    send(sock->fd, (const char*)hdr, 4, 0);
    send(sock->fd, (const char*)pkt, pos, 0);
}

void UpdateObject_SendCreateCreature(WorldSocket* sock, Unit* unit) {
    if (!sock || !unit) return;

    uint8_t block[8192];
    int blockLen = UpdateObject_BuildCreatureCreate(block, sizeof(block), unit);
    if (blockLen <= 0) return;

    uint8_t pkt[8200];
    int pos = 0;
    uint32_t count = 1;
    memcpy(pkt + pos, &count, 4); pos += 4;
    pkt[pos++] = 0;
    memcpy(pkt + pos, block, blockLen); pos += blockLen;

    uint16_t opcode = (uint16_t)SMSG_UPDATE_OBJECT;
    uint16_t pktSize = (uint16_t)(2 + pos);
    uint8_t hdr[4];
    hdr[0] = (uint8_t)((pktSize >> 8) & 0xFF);
    hdr[1] = (uint8_t)(pktSize & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);

    send(sock->fd, (const char*)hdr, 4, 0);
    send(sock->fd, (const char*)pkt, pos, 0);
}

/* ================================================================
 *  Send SMSG_DESTROY_OBJECT
 * ================================================================ */
void UpdateObject_SendDestroy(WorldSocket* sock, uint64_t guid) {
    if (!sock) return;
    uint8_t body[9];
    memcpy(body, &guid, 8);
    body[8] = 0; /* animation */

    uint16_t opcode = (uint16_t)SMSG_DESTROY_OBJECT;
    uint16_t pktSize = (uint16_t)(2 + 9);
    uint8_t hdr[4];
    hdr[0] = (uint8_t)((pktSize >> 8) & 0xFF);
    hdr[1] = (uint8_t)(pktSize & 0xFF);
    hdr[2] = (uint8_t)(opcode & 0xFF);
    hdr[3] = (uint8_t)((opcode >> 8) & 0xFF);

    send(sock->fd, (const char*)hdr, 4, 0);
    send(sock->fd, (const char*)body, 9, 0);
}
