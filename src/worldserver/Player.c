#include "worldserver/Player.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

PlayerObject* Player_New(ObjectGuid guid) {
    PlayerObject* p = calloc(1, sizeof(PlayerObject));
    p->guid = guid;
    /* defaults */
    p->fields[0x0014/4] = 0x08; /* OBJECT_TYPE */
    p->fields[0x000C/4] = 0x39; /* OBJECT_ENTRY */
    p->fields[0x0010/4] = 0x48; /* OBJECT_PADDING */
    return p;
}

void Player_Delete(PlayerObject* p) { if (p) free(p); }

void Player_SetUInt32(PlayerObject* p, uint32 field, uint32 value) {
    if (field < PLAYER_UPDATE_FIELDS) p->fields[field] = value;
}

void Player_SetUInt64(PlayerObject* p, uint32 field, uint64 value) {
    if (field < PLAYER_UPDATE_FIELDS) p->fields[field/2] = (uint32)(value & 0xFFFFFFFFULL);
    if (field+1 < PLAYER_UPDATE_FIELDS) p->fields[field/2+1] = (uint32)(value >> 32);
}

uint32 Player_GetUInt32(PlayerObject* p, uint32 field) {
    return (field < PLAYER_UPDATE_FIELDS) ? p->fields[field] : 0;
}

uint64 Player_GetUInt64(PlayerObject* p, uint32 field) {
    uint64 lo = (field < PLAYER_UPDATE_FIELDS) ? p->fields[field] : 0;
    uint64 hi = (field+1 < PLAYER_UPDATE_FIELDS) ? p->fields[field+1] : 0;
    return lo | (hi << 32);
}

void Player_SetPosition(PlayerObject* p, uint32 mapId, float x, float y, float z, float o) {
    Player_SetUInt32(p, 0x03A8, mapId);
    /* UNIT_FIELD_POSITION_* — stored in floats via direct memory */
    *(float*)&p->fields[0x01A4/4] = x;
    *(float*)&p->fields[0x01A8/4] = y;
    *(float*)&p->fields[0x01AC/4] = z;
    *(float*)&p->fields[0x01B0/4] = o;
}

void Player_SetName(PlayerObject* p, const char* name) {
    if (!name) return;
    size_t len = strlen(name);
    if (len > 15) len = 15;
    /* name stored in OBJECT_FIELD_NAME as bytes at offset 0x18 */
    memset((char*)&p->fields[0x18/4], 0, 32);
    memcpy(&p->fields[0x18/4], name, len);
}

void Player_SetCreateData(PlayerObject* p, uint32 race, uint32 class_, uint8 level, uint32 mapId, float x, float y, float z, float o) {
    Player_SetUInt64(p, 0x0038, p->guid);
    Player_SetUInt32(p, 0x003C, 0x19);         /* OBJECT_TYPE = OBJECTTYPE_PLAYER */
    Player_SetUInt32(p, 0x0040, race);          /* UNIT_FIELD_BYTES_0: race */
    Player_SetUInt32(p, 0x0044, class_);        /* class */
    Player_SetUInt32(p, 0x0048, level);        /* level */
    Player_SetUInt32(p, 0x004C, level);        /* level again? */
    Player_SetUInt32(p, 0x0050, 100);           /* health */
    Player_SetUInt32(p, 0x0060, 100);           /* power (mana) */
    Player_SetUInt32(p, 0x0068, 100);           /* max health */
    Player_SetUInt32(p, 0x0078, 100);           /* max mana */
    Player_SetUInt32(p, 0x0098, 20);            /* strength */
    Player_SetUInt32(p, 0x009C, 20);            /* agility */
    Player_SetUInt32(p, 0x00A0, 20);            /* stamina */
    Player_SetUInt32(p, 0x00A4, 20);           /* intellect */
    Player_SetUInt32(p, 0x00A8, 20);            /* spirit */
    Player_SetUInt32(p, 0x00AC, 0);             /* armor */
    Player_SetUInt32(p, 0x00B0, 0);             /* resistance */
    Player_SetUInt32(p, 0x00D0, 0x0B);         /* UNIT_FIELD_BYTES_2: sheathe */
    /* map/position */
    *(float*)&p->fields[0x01A4/4] = x;
    *(float*)&p->fields[0x01A8/4] = y;
    *(float*)&p->fields[0x01AC/4] = z;
    *(float*)&p->fields[0x01B0/4] = o;
    Player_SetUInt32(p, 0x03A8, mapId);
}

void Player_SetStatsForLevel(PlayerObject* p, uint8 level) {
    /* Simplified stat scaling per level */
    uint32 baseHealth = 100 + (level - 1) * 10;
    uint32 baseMana   = 50  + (level - 1) * 5;
    Player_SetUInt32(p, 0x0050, baseHealth);
    Player_SetUInt32(p, 0x0068, baseHealth);
    Player_SetUInt32(p, 0x0054, baseMana);
    Player_SetUInt32(p, 0x0078, baseMana);
    Player_SetUInt32(p, 0x0048, level);
    Player_SetUInt32(p, 0x004C, level);
    Player_SetUInt32(p, 0x0098, 20 + level);
    Player_SetUInt32(p, 0x009C, 20 + level);
    Player_SetUInt32(p, 0x00A0, 20 + level);
    Player_SetUInt32(p, 0x00A4, 20 + level);
    Player_SetUInt32(p, 0x00A8, 20 + level);
}

void Player_WriteCreateBytes(PlayerObject* p, ByteBuffer* bb, ObjectGuid guid) {
    /* Write all update fields for SMSG_UPDATE_OBJECT create */
    for (uint32 i = 0; i < PLAYER_UPDATE_FIELDS; i++) {
        uint32 val = Player_GetUInt32(p, i);
        ByteBuffer_WriteUInt(bb, val);
    }
    /* Write movement data (4 floats + 1 uint32 flags) */
    ByteBuffer_WriteFloat(bb, 0.0f); /* x — set from position */
    ByteBuffer_WriteFloat(bb, 0.0f); /* y */
    ByteBuffer_WriteFloat(bb, 0.0f); /* z */
    ByteBuffer_WriteFloat(bb, 0.0f); /* orientation */
    ByteBuffer_WriteUInt(bb, 0);     /* movement flags */
}
