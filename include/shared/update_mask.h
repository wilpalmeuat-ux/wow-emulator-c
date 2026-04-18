#ifndef UPDATE_MASK_H
#define UPDATE_MASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UPDATE_MASK_MAX_FIELDS 128
#define UPDATE_MASK_MAX_BYTES ((UPDATE_MASK_MAX_FIELDS + 7) / 8)

typedef enum {
    TYPEID_OBJECT        = 0,
    TYPEID_ITEM         = 1,
    TYPEID_CONTAINER    = 2,
    TYPEID_UNIT         = 3,
    TYPEID_PLAYER       = 4,
    TYPEID_GAMEOBJECT   = 5,
    TYPEID_DYNAMICOBJECT = 6,
    TYPEID_CORPSE       = 7
} TypeID;

typedef enum {
    UPDATE_FULL         = 0,
    UPDATE_VALUES       = 1,
    UPDATE_NONE         = 2
} UpdateType;

typedef struct {
    uint8_t bits[UPDATE_MASK_MAX_BYTES];
    uint32_t field_count;
} UpdateMask;

typedef struct {
    uint64_t guid;
    int32_t ival;
    float   fval;
    uint8_t bytes[4];
} UpdateValue;

typedef struct {
    uint64_t object_guid;
    uint8_t  object_type;
    uint8_t  update_type;

    UpdateMask mask;
    UpdateValue values[UPDATE_MASK_MAX_FIELDS];

    uint64_t inventory[20];

    float x, y, z, o;
    uint32_t move_flags;
    uint32_t move_flags2;
    uint32_t move_time;
    float walk_speed;
    float run_speed;
    float swim_speed;
    float fly_speed;
    float turn_rate;

    uint16_t emote_state;
    uint32_t unit_flags;
    uint32_t dynamic_flags;
    uint32_t health;
    uint32_t max_health;
    uint32_t power;
    uint32_t max_power;
    uint32_t level;
    uint32_t faction_template;
    uint32_t race;
    uint32_t class_id;
    float bounding_radius;
    float combat_reach;
} UpdateBlock;

UpdateMask* UpdateMask_New(void);
void UpdateMask_Delete(UpdateMask* m);
void UpdateMask_SetBit(UpdateMask* m, uint32_t index);
void UpdateMask_ClearBit(UpdateMask* m, uint32_t index);
bool UpdateMask_GetBit(UpdateMask* m, uint32_t index);
void UpdateMask_Resize(UpdateMask* m, uint32_t field_count);
void UpdateMask_Clear(UpdateMask* m);
uint32_t UpdateMask_GetBlockCount(UpdateMask* m);

UpdateBlock* UpdateBlock_New(void);
void UpdateBlock_Delete(UpdateBlock* b);
void UpdateBlock_Init(UpdateBlock* b, uint64_t guid, uint8_t type);
void UpdateBlock_SetInt(UpdateBlock* b, uint32_t field, int32_t val);
void UpdateBlock_SetFloat(UpdateBlock* b, uint32_t field, float val);
void UpdateBlock_SetGUID(UpdateBlock* b, uint32_t field, uint64_t guid);
void UpdateBlock_SetBytes(UpdateBlock* b, uint32_t field, uint8_t a, uint8_t b2, uint8_t c, uint8_t d);
void UpdateBlock_SetMovement(UpdateBlock* b,
    float x, float y, float z, float o,
    uint32_t flags, uint32_t flags2, uint32_t time,
    float walk, float run, float swim, float fly, float turn);
void UpdateBlock_SetUnitFields(UpdateBlock* b,
    uint32_t health, uint32_t max_health,
    uint32_t power, uint32_t max_power,
    uint32_t level, uint32_t faction, uint32_t race, uint32_t class_id,
    float bound_radius, float combat_reach,
    uint32_t unit_flags, uint32_t dyn_flags);
void UpdateBlock_SetPlayerFields(UpdateBlock* b,
    uint64_t guid, uint32_t level, uint32_t race, uint32_t class_id,
    uint32_t health, uint32_t max_health,
    uint32_t power, uint32_t max_power,
    float x, float y, float z, float o,
    uint32_t map_id, uint32_t zone_id,
    uint32_t guild_id, const char* name);

int UpdateBlock_Build(UpdateBlock* b, uint8_t* out_buf, int max_len);
int UpdateBlock_Parse(uint8_t* buf, int len, UpdateBlock* out_b);

int BuildPlayerUpdatePacket(void* player, uint8_t* out_buf, int max_len, bool for_self);
int BuildCreatureUpdatePacket(void* creature, uint8_t* out_buf, int max_len);

#endif