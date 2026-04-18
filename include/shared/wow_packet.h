#ifndef WOW_PACKET_H
#define WOW_PACKET_H

#include <stdint.h>
#include <stdbool.h>

typedef struct WowBuffer {
    uint8_t* data;
    int capacity;
    int pos;
} WowBuffer;

void WowBuffer_Init(WowBuffer* wb, uint8_t* data, int capacity);
void WowBuffer_WriteU8(WowBuffer* wb, uint8_t val);
void WowBuffer_WriteU16(WowBuffer* wb, uint16_t val);
void WowBuffer_WriteU32(WowBuffer* wb, uint32_t val);
void WowBuffer_WriteU64(WowBuffer* wb, uint64_t val);
void WowBuffer_WriteF32(WowBuffer* wb, float val);
void WowBuffer_WriteStr(WowBuffer* wb, const char* str);
void WowBuffer_WriteBytes(WowBuffer* wb, const uint8_t* data, int len);
uint8_t  WowBuffer_ReadU8(WowBuffer* wb);
uint16_t WowBuffer_ReadU16(WowBuffer* wb);
uint32_t WowBuffer_ReadU32(WowBuffer* wb);
uint64_t WowBuffer_ReadU64(WowBuffer* wb);
float    WowBuffer_ReadF32(WowBuffer* wb);
void     WowBuffer_ReadStr(WowBuffer* wb, char* out, int max_len);

#define CMSG_PING                0x01
#define CMSG_LOGOUT              0x04
#define CMSG_PLAYER_LOGIN        0x0D
#define CMSG_CHAR_ENUM           0x8B
#define CMSG_CHAR_CREATE         0x8A
#define CMSG_CHAR_DELETE         0x8C
#define CMSG_GOSSIP_HELLO        0x2D
#define CMSG_GOSSIP_SELECT_OPTION 0x2E
#define CMSG_QUESTGIVER_HELLO    0x30
#define CMSG_QUESTGIVER_ACCEPT_QUEST 0x31
#define CMSG_QUESTGIVER_COMPLETE_QUEST 0x32
#define CMSG_MESSAGECHAT_SAY     0x39
#define CMSG_MESSAGECHAT_YELL    0x3A
#define CMSG_MESSAGECHAT_WHISPER 0x3C
#define CMSG_EMOTE               0x16
#define CMSG_ATTACKSWING         0x19
#define CMSG_ATTACKSTOP          0x1A
#define CMSG_CAST_SPELL          0x55
#define CMSG_LEARN_TALENT        0x5A
#define CMSG_USE_ITEM            0x52
#define CMSG_OPEN_ITEM           0x53
#define CMSG_REPOP_REQUEST       0x7B
#define CMSG_FORCE_MOVE_ACK      0x6A
#define CMSG_MOVE_SET_RAW_POSITION_ACK 0x66
#define CMSG_MOVE_WALK_SPEED_ACK 0x6B
#define CMSG_MOVE_RUN_SPEED_ACK  0x6C
#define CMSG_SET_FACING          0x65
#define CMSG_MOVE_HEARTBEAT      0xB9

#define SMSG_AUTH_CHALLENGE      0x00
#define SMSG_AUTH_RESPONSE       0x01
#define SMSG_CHAR_ENUM           0x8B
#define SMSG_CHAR_CREATE         0x8C
#define SMSG_CHAR_DELETE         0x8D
#define SMSG_SURVEY_RESULT       0x08
#define SMSG_PING                0x01
#define SMSG_NAME_QUERY          0x0E
#define SMSG_QUERY_TIME_RESPONSE 0x10
#define SMSG_LOGOUT_COMPLETE     0x05
#define SMSG_LOGOUT_RESPONSE     0x06
#define SMSG_GOSSIP_MESSAGE      0x2F
#define SMSG_QUESTGIVER_QUEST_DETAILS 0x32
#define SMSG_QUESTGIVER_QUEST_COMPLETE 0x31
#define SMSG_QUESTGIVER_QUEST_LIST    0x30
#define SMSG_MESSAG_CHAT        0x09
#define SMSG_EMOTE              0x15
#define SMSG_TEXT_EMOTE          0x16
#define SMSG_ATTACKER_STATE_VISIBLE 0x2C
#define SMSG_CAST_FAILED         0x2E
#define SMSG_SPELL_START         0x2F
#define SMSG_SPELL_GO            0x30
#define SMSG_SPELLLOGEXECUTE     0x33
#define SMSG_INITIAL_SPELLS      0x2A
#define SMSG_LEARNED_SPELL       0x3D
#define SMSG_UNLEARNED_SPELL     0x3E
#define SMSG_LEVEL_UP_INFO       0x3F
#define SMSG_MOVE_UPDATE         0x0B
#define SMSG_MOVE_TELEPORT       0x0C
#define SMSG_TRANSFER_PENDING    0x07
#define SMSG_NEW_WORLD           0x08
#define SMSG_SPAWN_GROUP_UPDATE  0x0F

#endif
