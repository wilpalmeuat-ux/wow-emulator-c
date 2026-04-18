#ifndef PLAYER_H
#define PLAYER_H

#include "shared/Types.h"
#include "shared/ByteBuffer.h"

/* WoW 3.3.5a has 116 UNIT_FIELD_* + 0x70 PLAYER_FIELD_* */
#define PLAYER_UPDATE_FIELDS 0x00D8

typedef struct {
    ObjectGuid guid;
    uint32     fields[PLAYER_UPDATE_FIELDS];
} PlayerObject;

PlayerObject* Player_New(ObjectGuid guid);
void          Player_Delete(PlayerObject* p);
void          Player_SetUInt32(PlayerObject* p, uint32 field, uint32 value);
void          Player_SetUInt64(PlayerObject* p, uint32 field, uint64 value);
uint32        Player_GetUInt32(PlayerObject* p, uint32 field);
uint64        Player_GetUInt64(PlayerObject* p, uint32 field);
void          Player_SetPosition(PlayerObject* p, uint32 mapId, float x, float y, float z, float o);
void          Player_SetName(PlayerObject* p, const char* name);
void          Player_WriteCreateBytes(PlayerObject* p, ByteBuffer* bb, ObjectGuid guid);
void          Player_SetStatsForLevel(PlayerObject* p, uint8 level);
void          Player_SetCreateData(PlayerObject* p, uint32 race, uint32 class_, uint8 level, uint32 mapId, float x, float y, float z, float o);

#endif
