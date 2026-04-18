#include "update_mask.h"
#include "ByteBuffer.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

UpdateMask* UpdateMask_New(void) {
    UpdateMask* m = calloc(1, sizeof(UpdateMask));
    m->field_count = 32;
    return m;
}

void UpdateMask_Delete(UpdateMask* m) { if (m) free(m); }

void UpdateMask_SetBit(UpdateMask* m, uint32_t index) {
    if (index >= UPDATE_MASK_MAX_FIELDS) return;
    m->bits[index / 8] |= (1 << (index % 8));
}

void UpdateMask_ClearBit(UpdateMask* m, uint32_t index) {
    if (index >= UPDATE_MASK_MAX_FIELDS) return;
    m->bits[index / 8] &= ~(1 << (index % 8));
}

bool UpdateMask_GetBit(UpdateMask* m, uint32_t index) {
    if (index >= UPDATE_MASK_MAX_FIELDS) return false;
    return (m->bits[index / 8] & (1 << (index % 8))) != 0;
}

void UpdateMask_Resize(UpdateMask* m, uint32_t field_count) {
    if (field_count > UPDATE_MASK_MAX_FIELDS) field_count = UPDATE_MASK_MAX_FIELDS;
    m->field_count = field_count;
}

void UpdateMask_Clear(UpdateMask* m) {
    memset(m->bits, 0, UPDATE_MASK_MAX_BYTES);
}

uint32_t UpdateMask_GetBlockCount(UpdateMask* m) {
    return (m->field_count + 31) / 32;
}

UpdateBlock* UpdateBlock_New(void) {
    UpdateBlock* b = calloc(1, sizeof(UpdateBlock));
    b->object_type = TYPEID_OBJECT;
    b->update_type = UPDATE_FULL;
    b->walk_speed = 2.5f;
    b->run_speed = 7.0f;
    b->swim_speed = 4.5f;
    b->fly_speed = 7.0f;
    b->turn_rate = 3.14159f;
    UpdateMask_Resize(&b->mask, 128);
    return b;
}

void UpdateBlock_Delete(UpdateBlock* b) { if (b) free(b); }

void UpdateBlock_Init(UpdateBlock* b, uint64_t guid, uint8_t type) {
    memset(b, 0, sizeof(UpdateBlock));
    b->object_guid = guid;
    b->object_type = type;
    b->update_type = UPDATE_FULL;
    UpdateMask_Resize(&b->mask, 128);
    b->walk_speed = 2.5f;
    b->run_speed = 7.0f;
    b->swim_speed = 4.5f;
    b->fly_speed = 7.0f;
    b->turn_rate = 3.14159f;
}

void UpdateBlock_SetInt(UpdateBlock* b, uint32_t field, int32_t val) {
    if (field >= UPDATE_MASK_MAX_FIELDS) return;
    b->values[field].ival = val;
    UpdateMask_SetBit(&b->mask, field);
}

void UpdateBlock_SetFloat(UpdateBlock* b, uint32_t field, float val) {
    if (field >= UPDATE_MASK_MAX_FIELDS) return;
    b->values[field].fval = val;
    UpdateMask_SetBit(&b->mask, field);
}

void UpdateBlock_SetGUID(UpdateBlock* b, uint32_t field, uint64_t guid) {
    if (field >= UPDATE_MASK_MAX_FIELDS) return;
    b->values[field].guid = guid;
    UpdateMask_SetBit(&b->mask, field);
}

void UpdateBlock_SetBytes(UpdateBlock* b, uint32_t field, uint8_t a, uint8_t b2, uint8_t c, uint8_t d) {
    if (field >= UPDATE_MASK_MAX_FIELDS) return;
    b->values[field].bytes[0] = a;
    b->values[field].bytes[1] = b2;
    b->values[field].bytes[2] = c;
    b->values[field].bytes[3] = d;
    UpdateMask_SetBit(&b->mask, field);
}

void UpdateBlock_SetMovement(UpdateBlock* b,
    float x, float y, float z, float o,
    uint32_t flags, uint32_t flags2, uint32_t time,
    float walk, float run, float swim, float fly, float turn) {
    b->x = x; b->y = y; b->z = z; b->o = o;
    b->move_flags = flags;
    b->move_flags2 = flags2;
    b->move_time = time;
    b->walk_speed = walk;
    b->run_speed = run;
    b->swim_speed = swim;
    b->fly_speed = fly;
    b->turn_rate = turn;
}

void UpdateBlock_SetUnitFields(UpdateBlock* b,
    uint32_t health, uint32_t max_health,
    uint32_t power, uint32_t max_power,
    uint32_t level, uint32_t faction, uint32_t race, uint32_t class_id,
    float bound_radius, float combat_reach,
    uint32_t unit_flags, uint32_t dyn_flags) {
    b->health = health;
    b->max_health = max_health;
    b->power = power;
    b->max_power = max_power;
    b->level = level;
    b->faction_template = faction;
    b->race = race;
    b->class_id = class_id;
    b->bounding_radius = bound_radius;
    b->combat_reach = combat_reach;
    b->unit_flags = unit_flags;
    b->dynamic_flags = dyn_flags;

    UpdateBlock_SetInt(b,  6, health);
    UpdateBlock_SetInt(b,  7, max_health);
    UpdateBlock_SetInt(b,  9, max_power);
    UpdateBlock_SetInt(b, 10, power);
    UpdateBlock_SetInt(b, 11, level);
    UpdateBlock_SetInt(b, 12, faction);
    UpdateBlock_SetInt(b, 14, 0); /* base mana */
    UpdateBlock_SetBytes(b, 15, (uint8_t)race, (uint8_t)class_id, 0, 0);
    UpdateBlock_SetInt(b, 18, race);
    UpdateBlock_SetInt(b, 19, class_id);
    UpdateBlock_SetBytes(b, 22, 0, 0, 0, 0);
    UpdateBlock_SetFloat(b, 24, bound_radius);
}

void UpdateBlock_SetPlayerFields(UpdateBlock* b,
    uint64_t guid, uint32_t level, uint32_t race, uint32_t class_id,
    uint32_t health, uint32_t max_health,
    uint32_t power, uint32_t max_power,
    float x, float y, float z, float o,
    uint32_t map_id, uint32_t zone_id,
    uint32_t guild_id, const char* name) {
    (void)name;
    UpdateBlock_Init(b, guid, TYPEID_PLAYER);

    UpdateBlock_SetGUID(b, 0, guid);
    UpdateBlock_SetInt(b,  1, TYPEID_PLAYER);
    UpdateBlock_SetFloat(b, 3, 1.0f); /* scale */

    UpdateBlock_SetGUID(b,  6, 0); /* duel arbiter */
    UpdateBlock_SetInt(b,   7, 0); /* player flags */
    UpdateBlock_SetInt(b,   8, guild_id);
    UpdateBlock_SetInt(b,   9, 0); /* guild rank */
    UpdateBlock_SetBytes(b, 10, (uint8_t)race, (uint8_t)class_id, 0, 0);
    UpdateBlock_SetBytes(b, 11, 0, 0, 0, 0);
    UpdateBlock_SetBytes(b, 12, 0, 0, 0, 0);
    UpdateBlock_SetInt(b,  13, 0); /* XP */
    UpdateBlock_SetInt(b,  14, 400); /* next level XP */

    UpdateBlock_SetUnitFields(b,
        health, max_health, power, max_power,
        level, 0x1000 | race, race, class_id,
        0.3f, 1.5f,
        0x00000008, 0x00000000);

    UpdateBlock_SetMovement(b, x, y, z, o,
        0x00000000, 0x00000000, 0,
        2.5f, 7.0f, 4.5f, 7.0f, 3.14159f);

    UpdateBlock_SetInt(b, 27, map_id);
    UpdateBlock_SetInt(b, 28, zone_id);
    UpdateBlock_SetInt(b, 29, 0); /* monitored unit guid */
    UpdateBlock_SetInt(b, 30, 0); /* battleground guid */
    UpdateBlock_SetInt(b, 31, 0); /* death restore time */
    UpdateBlock_SetInt(b, 32, 0); /* rename timeout */
    UpdateBlock_SetInt(b, 33, 0); /* group */
    UpdateBlock_SetInt(b, 34, 0); /* first login */
}

/* Build update block into binary wire format for SMSG_UPDATE_OBJECT */
int UpdateBlock_Build(UpdateBlock* b, uint8_t* out_buf, int max_len) {
    if (!b || !out_buf || max_len < 64) return 0;

    int offset = 0;
    ByteBuffer pkt;
    ByteBuffer_Init(&pkt, 8192);

    /* Update block header */
    ByteBuffer_WriteUInt8(&pkt, b->update_type);
    ByteBuffer_WriteUInt64(&pkt, b->object_guid);

    if (b->update_type == UPDATE_FULL) {
        /* Full update: write movement + values */

        /* Movement block */
        uint32_t move_size_offset = pkt.wpos;
        ByteBuffer_WriteUInt32(&pkt, 0); /* movement block size placeholder */

        /* Movement flags */
        uint8_t move_flags = 0;
        if (b->move_flags & 0x00000100) move_flags |= 0x01; /* walk */
        if (b->move_flags & 0x00200000) move_flags |= 0x02; /* jump */
        ByteBuffer_WriteUInt8(&pkt, move_flags);

        ByteBuffer_WriteUInt32(&pkt, b->move_time);
        ByteBuffer_WriteFloat(&pkt, b->x);
        ByteBuffer_WriteFloat(&pkt, b->y);
        ByteBuffer_WriteFloat(&pkt, b->z);
        ByteBuffer_WriteFloat(&pkt, b->o);

        /* Speed fields */
        ByteBuffer_WriteFloat(&pkt, b->walk_speed);
        ByteBuffer_WriteFloat(&pkt, b->run_speed);
        ByteBuffer_WriteFloat(&pkt, b->swim_speed);
        ByteBuffer_WriteFloat(&pkt, b->fly_speed);
        ByteBuffer_WriteFloat(&pkt, b->turn_rate);

        if (b->object_type >= TYPEID_UNIT) {
            ByteBuffer_WriteFloat(&pkt, b->walk_speed); /* swim back speed */
            ByteBuffer_WriteFloat(&pkt, b->run_speed);  /* fly back speed */
            ByteBuffer_WriteFloat(&pkt, b->turn_rate);  /* pitch rate */
        }

        /* Transport data (optional - skip for normal movement) */
        /* For simplicity, write 0 transport flags */
        ByteBuffer_WriteUInt8(&pkt, 0); /* no transport */

        /* Write movement block size at saved offset */
        uint32_t size = pkt.wpos - move_size_offset - 4;
        *(uint32_t*)(pkt.data + move_size_offset) = size;

        /* Build update mask from set bits */
        uint8_t mask_buf[32];
        memset(mask_buf, 0, sizeof(mask_buf));

        for (uint32_t i = 0; i < b->mask.field_count; i++) {
            if (UpdateMask_GetBit(&b->mask, i)) {
                mask_buf[i / 8] |= (1 << (i % 8));
            }
        }

        /* Write update mask */
        uint32_t mask_bytes = (b->mask.field_count + 7) / 8;
        ByteBuffer_WriteBytes(&pkt, mask_buf, mask_bytes);

        /* Write all set field values */
        for (uint32_t i = 0; i < b->mask.field_count; i++) {
            if (!UpdateMask_GetBit(&b->mask, i)) continue;

            switch (i) {
                case 0: case 6: case 8: /* guid or int fields */
                case 1: case 2: case 7: case 9: case 10: case 11: case 12:
                case 14: case 18: case 19: case 23: case 25: case 26:
                case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34:
                    ByteBuffer_WriteUInt32(&pkt, (uint32_t)b->values[i].ival);
                    break;
                case 3: /* scale_x - float */
                case 24: /* padding */
                    ByteBuffer_WriteFloat(&pkt, b->values[i].fval);
                    break;
                default: {
                    /* Check if any bytes set for this field */
                    if (b->values[i].bytes[0] || b->values[i].bytes[1] ||
                        b->values[i].bytes[2] || b->values[i].bytes[3]) {
                        ByteBuffer_WriteBytes(&pkt, b->values[i].bytes, 4);
                    } else {
                        ByteBuffer_WriteUInt32(&pkt, 0);
                    }
                    break;
                }
            }
        }

        /* For player objects, write inventory block (20 slots) */
        if (b->object_type == TYPEID_PLAYER) {
            ByteBuffer_WriteUInt32(&pkt, 20); /* inventory visible count */
            for (int i = 0; i < 20; i++) {
                ByteBuffer_WriteUInt64(&pkt, b->inventory[i]);
            }
        }

    } else {
        /* Partial update: only mask + changed values */
        uint8_t mask_buf[32];
        memset(mask_buf, 0, sizeof(mask_buf));
        for (uint32_t i = 0; i < b->mask.field_count; i++) {
            if (UpdateMask_GetBit(&b->mask, i))
                mask_buf[i / 8] |= (1 << (i % 8));
        }
        uint32_t mask_bytes = (b->mask.field_count + 7) / 8;
        ByteBuffer_WriteBytes(&pkt, mask_buf, mask_bytes);

        for (uint32_t i = 0; i < b->mask.field_count; i++) {
            if (!UpdateMask_GetBit(&b->mask, i)) continue;
            ByteBuffer_WriteUInt32(&pkt, (uint32_t)b->values[i].ival);
        }
    }

    if (pkt.wpos > max_len) return 0;
    memcpy(out_buf, pkt.data, pkt.wpos);
    int written = pkt.wpos;
    ByteBuffer_Delete(&pkt);
    return written;
}

int UpdateBlock_Parse(uint8_t* buf, int len, UpdateBlock* out_b) {
    (void)buf; (void)len; (void)out_b;
    return 0;
}

int BuildPlayerUpdatePacket(void* player, uint8_t* out_buf, int max_len, bool for_self) {
    (void)player; (void)for_self;
    UpdateBlock* b = UpdateBlock_New();
    int result = UpdateBlock_Build(b, out_buf, max_len);
    UpdateBlock_Delete(b);
    return result;
}

int BuildCreatureUpdatePacket(void* creature, uint8_t* out_buf, int max_len) {
    (void)creature; (void)out_buf; (void)max_len;
    return 0;
}