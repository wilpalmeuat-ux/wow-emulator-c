#include "shared/Opcodes.h"

const char* GetOpcodeName(Opcodes opcode) {
    switch (opcode) {
        case Opcodes::CMSG_BOOTME: return "CMSG_BOOTME";
        case Opcodes::CMSG_DBLOOKUP: return "CMSG_DBLOOKUP";
        case Opcodes::CMSG_SEARCH_QUEST: return "CMSG_SEARCH_QUEST";
        case Opcodes::CMSG_HAND_SHAKE: return "CMSG_HAND_SHAKE";
        case Opcodes::CMSG_PLAYER_LOGIN: return "CMSG_PLAYER_LOGIN";
        case Opcodes::CMSG_CHAR_ENUM: return "CMSG_CHAR_ENUM";
        case Opcodes::CMSG_CHAR_CREATE: return "CMSG_CHAR_CREATE";
        case Opcodes::CMSG_CHAR_DELETE: return "CMSG_CHAR_DELETE";
        case Opcodes::CMSG_PLAYER_LOGOUT: return "CMSG_PLAYER_LOGOUT";
        case Opcodes::CMSG_LOGOUT_REQUEST: return "CMSG_LOGOUT_REQUEST";
        case Opcodes::CMSG_NAME_QUERY: return "CMSG_NAME_QUERY";
        case Opcodes::CMSG_PET_BATTLE_START_PVP: return "CMSG_PET_BATTLE_START_PVP";
        case Opcodes::CMSG_GOSSIP_HELLO: return "CMSG_GOSSIP_HELLO";
        case Opcodes::CMSG_GOSSIP_SELECT: return "CMSG_GOSSIP_SELECT";
        case Opcodes::CMSG_QUERY_TIME: return "CMSG_QUERY_TIME";
        case Opcodes::CMSG_CREATURE_QUERY: return "CMSG_CREATURE_QUERY";
        case Opcodes::CMSG_GAMEOBJECT_QUERY: return "CMSG_GAMEOBJECT_QUERY";
        case Opcodes::CMSG_MOVE_WORLDPORT_ACK: return "CMSG_MOVE_WORLDPORT_ACK";
        case Opcodes::CMSG_MOVE_HEARTBEAT: return "CMSG_MOVE_HEARTBEAT";
        case Opcodes::CMSG_MOVE_SET_FACING: return "CMSG_MOVE_SET_FACING";
        case Opcodes::CMSG_MOVE_SET_PITCH: return "CMSG_MOVE_SET_PITCH";
        case Opcodes::CMSG_MOVE_FLY_START: return "CMSG_MOVE_FLY_START";
        case Opcodes::CMSG_MOVE_FLY_STOP: return "CMSG_MOVE_FLY_STOP";
        case Opcodes::CMSG_MOVE_FLY_CHANGE_SPEED: return "CMSG_MOVE_FLY_CHANGE_SPEED";
        case Opcodes::CMSG_TEXT_EMOTE: return "CMSG_TEXT_EMOTE";
        case Opcodes::CMSG_CHAT: return "CMSG_CHAT";
        case Opcodes::CMSG_EMOTE: return "CMSG_EMOTE";
        case Opcodes::SMSG_CHAR_ENUM: return "SMSG_CHAR_ENUM";
        case Opcodes::SMSG_CHAR_CREATE: return "SMSG_CHAR_CREATE";
        case Opcodes::SMSG_CHAR_DELETE: return "SMSG_CHAR_DELETE";
        case Opcodes::SMSG_CHAR_LOGIN_FAILED: return "SMSG_CHAR_LOGIN_FAILED";
        case Opcodes::SMSG_LOGIN_VERIFY_WORLD: return "SMSG_LOGIN_VERIFY_WORLD";
        case Opcodes::SMSG_LOGOUT_RESPONSE: return "SMSG_LOGOUT_RESPONSE";
        case Opcodes::SMSG_NAME_QUERY_RESPONSE: return "SMSG_NAME_QUERY_RESPONSE";
        case Opcodes::SMSG_CREATURE_QUERY_RESPONSE: return "SMSG_CREATURE_QUERY_RESPONSE";
        case Opcodes::SMSG_GAMEOBJECT_QUERY_RESPONSE: return "SMSG_GAMEOBJECT_QUERY_RESPONSE";
        case Opcodes::SMSG_MESSAG_CHAT: return "SMSG_MESSAG_CHAT";
        case Opcodes::SMSG_TEXT_EMOTE: return "SMSG_TEXT_EMOTE";
        case Opcodes::SMSG_EMOTE: return "SMSG_EMOTE";
        default: return "UNKNOWN";
    }
}