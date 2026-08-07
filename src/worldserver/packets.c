/* packets.c -- Opcode name table + packet utilities for WoW 3.3.5a
 *
 * Provides human-readable opcode names for logging/debugging and
 * shared packet building helpers.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 *  Opcode name lookup (for debug logging)
 * ================================================================ */
typedef struct {
    uint32_t opcode;
    const char* name;
} OpcodeEntry;

static const OpcodeEntry _opcodeTable[] = {
    { 0x0037, "CMSG_CHAR_ENUM" },
    { 0x0039, "CMSG_CHAR_CREATE" },
    { 0x003B, "CMSG_CHAR_DELETE" },
    { 0x003D, "CMSG_PLAYER_LOGIN" },
    { 0x004B, "CMSG_LOGOUT_REQUEST" },
    { 0x0050, "CMSG_NAME_QUERY" },
    { 0x0060, "CMSG_CREATURE_QUERY" },
    { 0x005E, "CMSG_GAMEOBJECT_QUERY" },
    { 0x0095, "CMSG_MESSAGECHAT" },
    { 0x00DC, "CMSG_MOVE_WORLDPORT_ACK" },
    { 0x00EE, "CMSG_MOVE_HEARTBEAT" },
    { 0x017B, "CMSG_GOSSIP_HELLO" },
    { 0x017C, "CMSG_GOSSIP_SELECT_OPTION" },
    { 0x01CE, "CMSG_QUERY_TIME" },
    { 0x01DC, "CMSG_PING" },
    { 0x01ED, "CMSG_AUTH_SESSION" },
    { 0x003A, "SMSG_CHAR_CREATE" },
    { 0x003B, "SMSG_CHAR_ENUM" },
    { 0x003C, "SMSG_CHAR_DELETE" },
    { 0x0051, "SMSG_NAME_QUERY_RESPONSE" },
    { 0x0061, "SMSG_CREATURE_QUERY_RESPONSE" },
    { 0x0096, "SMSG_MESSAGECHAT" },
    { 0x00A9, "SMSG_UPDATE_OBJECT" },
    { 0x00AA, "SMSG_DESTROY_OBJECT" },
    { 0x01CF, "SMSG_QUERY_TIME_RESPONSE" },
    { 0x01DD, "SMSG_PONG" },
    { 0x01EC, "SMSG_AUTH_CHALLENGE" },
    { 0x01EE, "SMSG_AUTH_RESPONSE" },
    { 0x017D, "SMSG_GOSSIP_MESSAGE" },
    { 0x0209, "SMSG_ACCOUNT_DATA_TIMES" },
    { 0x0236, "SMSG_LOGIN_VERIFY_WORLD" },
    { 0x02EF, "SMSG_ADDON_INFO" },
    { 0, NULL }
};

const char* Opcode_GetName(uint32_t opcode) {
    for (int i = 0; _opcodeTable[i].name; i++) {
        if (_opcodeTable[i].opcode == opcode)
            return _opcodeTable[i].name;
    }
    return "UNKNOWN";
}

/* ================================================================
 *  Packet builder helpers
 * ================================================================ */
typedef struct {
    uint8_t* data;
    int      pos;
    int      capacity;
} PacketBuilder;

PacketBuilder* PacketBuilder_Create(int initialCapacity) {
    PacketBuilder* pb = (PacketBuilder*)calloc(1, sizeof(PacketBuilder));
    pb->capacity = initialCapacity > 0 ? initialCapacity : 256;
    pb->data = (uint8_t*)calloc(1, pb->capacity);
    return pb;
}

void PacketBuilder_Destroy(PacketBuilder* pb) {
    if (!pb) return;
    free(pb->data);
    free(pb);
}

static void _pb_ensure(PacketBuilder* pb, int needed) {
    if (pb->pos + needed > pb->capacity) {
        int newCap = pb->capacity * 2;
        while (newCap < pb->pos + needed) newCap *= 2;
        uint8_t* newData = (uint8_t*)realloc(pb->data, newCap);
        if (newData) {
            pb->data = newData;
            pb->capacity = newCap;
        }
    }
}

void PacketBuilder_WriteU8(PacketBuilder* pb, uint8_t v) {
    _pb_ensure(pb, 1);
    pb->data[pb->pos++] = v;
}

void PacketBuilder_WriteU16(PacketBuilder* pb, uint16_t v) {
    _pb_ensure(pb, 2);
    pb->data[pb->pos++] = (uint8_t)(v & 0xFF);
    pb->data[pb->pos++] = (uint8_t)((v >> 8) & 0xFF);
}

void PacketBuilder_WriteU32(PacketBuilder* pb, uint32_t v) {
    _pb_ensure(pb, 4);
    pb->data[pb->pos++] = (uint8_t)(v & 0xFF);
    pb->data[pb->pos++] = (uint8_t)((v >> 8) & 0xFF);
    pb->data[pb->pos++] = (uint8_t)((v >> 16) & 0xFF);
    pb->data[pb->pos++] = (uint8_t)((v >> 24) & 0xFF);
}

void PacketBuilder_WriteU64(PacketBuilder* pb, uint64_t v) {
    _pb_ensure(pb, 8);
    for (int i = 0; i < 8; i++)
        pb->data[pb->pos++] = (uint8_t)((v >> (i * 8)) & 0xFF);
}

void PacketBuilder_WriteFloat(PacketBuilder* pb, float v) {
    union { float f; uint32_t i; } u;
    u.f = v;
    PacketBuilder_WriteU32(pb, u.i);
}

void PacketBuilder_WriteString(PacketBuilder* pb, const char* s) {
    if (!s) { PacketBuilder_WriteU8(pb, 0); return; }
    int len = (int)strlen(s);
    _pb_ensure(pb, len + 1);
    memcpy(pb->data + pb->pos, s, len + 1);
    pb->pos += len + 1;
}

void PacketBuilder_WriteBytes(PacketBuilder* pb, const void* data, int len) {
    _pb_ensure(pb, len);
    memcpy(pb->data + pb->pos, data, len);
    pb->pos += len;
}

void PacketBuilder_WriteZeros(PacketBuilder* pb, int count) {
    _pb_ensure(pb, count);
    memset(pb->data + pb->pos, 0, count);
    pb->pos += count;
}
