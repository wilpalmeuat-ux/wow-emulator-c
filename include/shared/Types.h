/* shared/Types.h — Common types for WoW emulator */
#pragma once
#include <stdint.h>

typedef uint64_t ObjectGuid;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint64_t uint64;

#define GUID_LOPART(x) ((uint32)((x) & 0xFFFFFFFFULL))
#define GUID_HIPART(x) ((uint32)(((x) >> 32) & 0xFFFFFFFFULL))
#define GUID_ENPART(x) ((uint32)(((x) >> 36) & 0xFFFFFFFFULL))

/* WoW OPCODES */
#define OPCODE_NULL                      0x0000
#define OPCODE_CMSG_CHAR_ENUM            0x0037
#define OPCODE_CMSG_CHAR_CREATE          0x0038
#define OPCODE_CMSG_CHAR_DELETE          0x0039
#define OPCODE_CMSG_PLAYER_LOGIN         0x003D
#define OPCODE_SMSG_CHAR_ENUM            0x003B
#define OPCODE_SMSG_LOGIN_VERIFY_WORLD    0x003C
#define OPCODE_SMSG_LOGOUT_COMPLETE      0x00DC
#define OPCODE_CMSG_LOGOUT_REQUEST       0x00DB
#define OPCODE_CMSG_LOGOUT_COMPLETE      0x00DD
#define OPCODE_CMSG_NAME_QUERY           0x0050
#define OPCODE_SMSG_NAME_QUERY_RESPONSE  0x0051
#define OPCODE_CMSG_GOSSIP_HELLO         0x00A2
#define OPCODE_CMSG_GOSSIP_SELECT        0x00A3
#define OPCODE_SMSG_GOSSIP_MESSAGE       0x00A1
#define OPCODE_CMSG_QUERY_TIME           0x00BE
#define OPCODE_SMSG_QUERY_TIME_RESPONSE  0x00BF
#define OPCODE_CMSG_CREATURE_QUERY       0x00C8
#define OPCODE_SMSG_CREATURE_QUERY_RESPONSE 0x00C9
#define OPCODE_CMSG_GAMEOBJECT_QUERY     0x0102
#define OPCODE_SMSG_GAMEOBJECT_QUERY_RESPONSE 0x0103
#define OPCODE_CMSG_MOVE_WORLDPORT_ACK   0x00EB
#define OPCODE_CMSG_MOVE_HEARTBEAT       0x00A5
#define OPCODE_CMSG_MOVE_SET_FACING      0x00A9
#define OPCODE_CMSG_MOVE_SET_PITCH       0x00AA
#define OPCODE_CMSG_TEXT_EMOTE           0x00AF
#define OPCODE_CMSG_EMOTE                0x00B0
#define OPCODE_SMSG_TEXT_EMOTE            0x00AE
#define OPCODE_SMSG_EMOTE                0x00B1
#define OPCODE_CMSG_CHAT                 0x00A7
#define OPCODE_SMSG_MESSAGECHAT          0x00A8
#define OPCODE_SMSG_UPDATEOBJECT         0x00A9
#define OPCODE_CMSG_AUTOSTORE_BAG_ITEM    0x00BE
#define OPCODE_CMSG_LOGOUT_COMPLETE       0x00DC
