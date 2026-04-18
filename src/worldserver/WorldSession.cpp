#include "worldserver/WorldSession.h"
#include "worldserver/WorldServer.h"
#include "worldserver/Player.h"
#include "worldserver/Packets.h"
#include "shared/Logging/Log.h"
#include "shared/Opcodes.h"
#include "shared/Database/Database.h"

extern Database* g_worldDB;

WorldSession::WorldSession(uint32_t accountId, WorldSocket* socket)
    : _accountId(accountId), _socket(socket), _player(nullptr),
      _logoutTime(0), _logoutDelay(0), _muteTime(0), _security(0) {
    _socket->SetSession(this);
}

WorldSession::~WorldSession() {
    if (_player) { _player->SaveToDB(); delete _player; }
    _socket = nullptr;
}

void WorldSession::Update(uint32_t diff) {
    if (_player) _player->Update(diff);
    if (_logoutTime > 0 && GetMSTime() > _logoutTime) Logout(true);
}

void WorldSession::HandlePacket(uint16_t opcode, const uint8_t* data, size_t len) {
    switch ((Opcodes)opcode) {
        case Opcodes::CMSG_CHAR_ENUM:       HandleCharEnum(); break;
        case Opcodes::CMSG_CHAR_CREATE:      HandleCharCreate(data, len); break;
        case Opcodes::CMSG_CHAR_DELETE:      HandleCharDelete(data, len); break;
        case Opcodes::CMSG_PLAYER_LOGIN:     HandlePlayerLogin(data, len); break;
        case Opcodes::CMSG_LOGOUT_REQUEST:   HandleLogoutRequest(); break;
        case Opcodes::CMSG_PLAYER_LOGOUT:    Logout(false); break;
        case Opcodes::CMSG_NAME_QUERY:       HandleNameQuery(data, len); break;
        case Opcodes::CMSG_GOSSIP_HELLO:     HandleGossipHello(); break;
        case Opcodes::CMSG_GOSSIP_SELECT:     HandleGossipSelect(data, len); break;
        case Opcodes::CMSG_QUERY_TIME:        HandleQueryTime(); break;
        case Opcodes::CMSG_CREATURE_QUERY:   HandleCreatureQuery(data, len); break;
        case Opcodes::CMSG_GAMEOBJECT_QUERY:  HandleGameObjectQuery(data, len); break;
        case Opcodes::CMSG_MOVE_WORLDPORT_ACK: HandleMoveWorldPortAck(); break;
        case Opcodes::CMSG_MOVE_HEARTBEAT:
        case Opcodes::CMSG_MOVE_SET_FACING:
        case Opcodes::CMSG_MOVE_SET_PITCH:    HandleMovement(data, len); break;
        case Opcodes::CMSG_TEXT_EMOTE:        HandleTextEmote(data, len); break;
        case Opcodes::CMSG_EMOTE:             HandleEmote(data, len); break;
        case Opcodes::CMSG_CHAT:              HandleChatMessage(data, len); break;
        default:
            LOG_DEBUG("Unhandled CMSG opcode 0x%04X", opcode);
            break;
    }
}

void WorldSession::HandleCharEnum() {
    ByteBuffer pkt;
    pkt.WriteUInt8(0); // success

    QueryResult* res = nullptr;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT guid,name,race,class,gender,level,zone,mapId,positionX,positionY,"
        "positionZ,orientation,guildId FROM characters WHERE accountId=%u", _accountId);
    g_worldDB->Query(sql, res);

    uint32_t count = 0;
    if (res) { count = (uint32_t)res->rowCount; pkt.WriteUInt32(count); }
    else { pkt.WriteUInt32(0); }

    if (res) {
        while (res->NextRow()) {
            pkt.WriteUInt64(res->GetUInt64(0));
            pkt.WriteString(res->GetString(1));
            pkt.WriteUInt8(res->GetUInt8(2));  // race
            pkt.WriteUInt8(res->GetUInt8(3));  // class
            pkt.WriteUInt8(res->GetUInt8(4));  // gender
            pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt8(0); // appearance bytes
            pkt.WriteUInt8(res->GetUInt8(5));  // level
            pkt.WriteUInt32(res->GetUInt32(6)); // zone
            pkt.WriteUInt32(res->GetUInt32(7)); // map
            pkt.WriteFloat(res->GetFloat(8));
            pkt.WriteFloat(res->GetFloat(9));
            pkt.WriteFloat(res->GetFloat(10));
            pkt.WriteFloat(res->GetFloat(11));
            pkt.WriteUInt32(res->GetUInt32(12)); // guildId
            pkt.WriteUInt32(0); // character flags
            pkt.WriteUInt8(0); // customization flags
            pkt.WriteUInt32(0); // at login flags
        }
        delete res;
    }
    SendPacket(Opcodes::SMSG_CHAR_ENUM, pkt);
}

void WorldSession::HandleCharCreate(const uint8_t* data, size_t len) {
    ByteBuffer buf; buf.WriteBytes(data, len);
    uint8_t race=buf.ReadUInt8(), class_=buf.ReadUInt8(), gender=buf.ReadUInt8();
    uint8_t skin=buf.ReadUInt8(), face=buf.ReadUInt8(), hairStyle=buf.ReadUInt8();
    uint8_t hairColor=buf.ReadUInt8(), facialFeatures=buf.ReadUInt8();
    std::string name = buf.ReadCString();

    uint8_t result = 0;
    if (!IsValidName(name)) result = 0x1E;
    else if (CharacterExistsInDB(name)) result = 0x1F;

    if (result == 0) {
        uint64_t guid = CreateCharacterInDB(name, race, class_, gender,
                                           skin, face, hairStyle, hairColor, facialFeatures);
        ByteBuffer pkt; pkt.WriteUInt8(0); pkt.WriteUInt64(guid);
        SendPacket(Opcodes::SMSG_CHAR_CREATE, pkt);
    } else {
        ByteBuffer pkt; pkt.WriteUInt8(result);
        SendPacket(Opcodes::SMSG_CHAR_CREATE, pkt);
    }
    (void)len;
}

void WorldSession::HandleCharDelete(const uint8_t* data, size_t len) {
    uint64_t guid = *(uint64_t*)data;
    bool ok = DeleteCharacterFromDB(guid);
    ByteBuffer pkt; pkt.WriteUInt8(ok ? 0 : 0x1D);
    SendPacket(Opcodes::SMSG_CHAR_DELETE, pkt);
    (void)len;
}

void WorldSession::HandlePlayerLogin(const uint8_t* data, size_t len) {
    uint64_t guid = *(uint64_t*)data;
    _player = Player::LoadFromDB(guid, g_worldDB);
    if (!_player) { LOG_ERROR("Failed to load player %llu", (unsigned long long)guid); return; }
    _player->SetSession(this);
    WorldServer::Get().AddPlayer(_player);

    ByteBuffer verifyPkt;
    verifyPkt.WriteUInt32(_player->GetMapId());
    verifyPkt.WriteFloat(_player->GetX());
    verifyPkt.WriteFloat(_player->GetY());
    verifyPkt.WriteFloat(_player->GetZ());
    verifyPkt.WriteFloat(_player->GetO());
    verifyPkt.WriteUInt32(_player->GetZoneId());
    SendPacket(Opcodes::SMSG_LOGIN_VERIFY_WORLD, verifyPkt);

    _player->SendInitialSpells();
    _player->SendInitialTalents();
    _player->SendInitialEquipment();
    _player->SendInitialInventory();
    (void)len;
}

void WorldSession::HandleLogoutRequest() {
    uint32_t now = GetMSTime();
    ByteBuffer pkt;
    pkt.WriteUInt32(0); // logout result
    pkt.WriteUInt32(0); // loginProof
    SendPacket(Opcodes::SMSG_LOGOUT_RESPONSE, pkt);
    _logoutTime = now + 15000; // 15 sec grace
}

void WorldSession::Logout(bool force) {
    if (_player) {
        _player->SaveToDB();
        WorldServer::Get().RemovePlayer(_player->GetGUID());
        delete _player;
        _player = nullptr;
    }
    _socket->Close();
    (void)force;
}

void WorldSession::HandleNameQuery(const uint8_t* data, size_t len) {
    uint64_t guid = *(uint64_t*)data;
    ByteBuffer pkt; pkt.WriteUInt64(guid);
    pkt.WriteString("LoggedUser");
    pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt32(0);
    SendPacket(Opcodes::SMSG_NAME_QUERY_RESPONSE, pkt);
    (void)len;
}

void WorldSession::HandleGossipHello() {
    if (!_player) return;
    ByteBuffer pkt;
    pkt.WriteUInt64(0); // guid
    pkt.WriteUInt32(0); // gossip menuid
    SendPacket(Opcodes::SMSG_GOSSIP_MESSAGE, pkt);
}

void WorldSession::HandleGossipSelect(const uint8_t* data, size_t len) {
    ByteBuffer buf; buf.WriteBytes(data, len);
    uint64_t guid = buf.ReadUInt64();
    uint32_t menuId = buf.ReadUInt32();
    uint32_t optionIdx = buf.ReadUInt32();
    LOG_DEBUG("GossipSelect: guid=%llu menu=%u option=%u", (unsigned long long)guid, menuId, optionIdx);
    (void)len;
}

void WorldSession::HandleQueryTime() {
    ByteBuffer pkt; pkt.WriteUInt32((uint32_t)time(nullptr));
    SendPacket(Opcodes::SMSG_QUERY_TIME_RESPONSE, pkt);
}

void WorldSession::HandleCreatureQuery(const uint8_t* data, size_t len) {
    uint32_t entry = *(uint32_t*)data;
    ByteBuffer pkt;
    pkt.WriteUInt32(entry);
    pkt.WriteString("Test Creature");
    pkt.WriteUInt32(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0);
    pkt.WriteUInt8(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0);
    pkt.WriteFloat(1.0f); pkt.WriteFloat(1.5f);
    pkt.WriteUInt32(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0);
    pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt8(0); pkt.WriteUInt32(0);
    pkt.WriteUInt32(0x19); pkt.WriteUInt32(0);
    SendPacket(Opcodes::SMSG_CREATURE_QUERY_RESPONSE, pkt);
    (void)len;
}

void WorldSession::HandleGameObjectQuery(const uint8_t* data, size_t len) {
    uint32_t entry = *(uint32_t*)data;
    ByteBuffer pkt;
    pkt.WriteUInt32(entry);
    pkt.WriteString("Test Object");
    pkt.WriteUInt32(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0); pkt.WriteUInt32(0);
    pkt.WriteUInt32(0); pkt.WriteFloat(1.0f);
    pkt.WriteUInt8(0);
    for (int i = 0; i < 24; ++i) pkt.WriteUInt32(0);
    SendPacket(Opcodes::SMSG_GAMEOBJECT_QUERY_RESPONSE, pkt);
    (void)len;
}

void WorldSession::HandleMoveWorldPortAck() {
    if (_player) _player->SetCanMove(true);
}

void WorldSession::HandleMovement(const uint8_t* data, size_t len) {
    if (!_player || len < 40) return;
    float x = *(float*)(data + 4);
    float y = *(float*)(data + 8);
    float z = *(float*)(data + 12);
    float o = *(float*)(data + 16);
    _player->Relocate(x, y, z, o);
}

void WorldSession::HandleTextEmote(const uint8_t* data, size_t len) {
    if (!_player || len < 16) return;
    uint64_t guid = *(uint64_t*)data;
    uint32_t emoteId = *(uint32_t*)(data + 8);
    LOG_DEBUG("TextEmote: guid=%llu emoteId=%u", (unsigned long long)guid, emoteId);
    ByteBuffer pkt; pkt.WriteUInt64(_player->GetGUID());
    pkt.WriteUInt32(emoteId); pkt.WriteUInt32(0);
    pkt.WriteString(_player->GetName());
    SendPacket(Opcodes::SMSG_TEXT_EMOTE, pkt);
    (void)len;
}

void WorldSession::HandleEmote(const uint8_t* data, size_t len) {
    if (!_player) return;
    uint32_t emoteId = *(uint32_t*)data;
    ByteBuffer pkt; pkt.WriteUInt64(_player->GetGUID());
    pkt.WriteUInt32(emoteId); pkt.WriteUInt32(0);
    SendPacket(Opcodes::SMSG_EMOTE, pkt);
    (void)len;
}

void WorldSession::HandleChatMessage(const uint8_t* data, size_t len) {
    if (!_player || len < 14) return;
    ByteBuffer buf; buf.WriteBytes(data, len);
    uint8_t type = buf.ReadUInt8();
    uint32_t lang = buf.ReadUInt32();
    uint64_t targetGuid = buf.ReadUInt64();
    uint32_t msgLen = buf.ReadUInt32();
    std::string msg = buf.ReadString(msgLen);

    if (type == 0x01) { // SAY
        ChatHandler::Get().HandleSay(_player, msg);
    } else if (type == 0x07) { // YELL
        ChatHandler::Get().HandleYell(_player, msg);
    } else if (type == 0x02) { // WHISPER
        ChatHandler::Get().HandleWhisper(_player, targetGuid, msg);
    }
    (void)lang;
}

void WorldSession::SendPacket(Opcodes opcode, const ByteBuffer& body) {
    if (!_socket) return;
    uint8_t hdr[6];
    *(uint16_t*)hdr = htons((uint16_t)body.GetStorageSize());
    *(uint16_t*)(hdr+2) = htons((uint16_t)opcode);
    std::vector<uint8_t> out;
    out.insert(out.end(), hdr, hdr+6);
    out.insert(out.end(), body.GetData(), body.GetData()+body.GetStorageSize());
    _socket->Send(out.data(), out.size());
}

bool WorldSession::IsValidName(const std::string& name) {
    if (name.empty() || name.size() > 12) return false;
    for (char c : name) if (!isalpha(c)) return false;
    return true;
}

bool WorldSession::CharacterExistsInDB(const std::string& name) {
    char sql[256]; snprintf(sql, sizeof(sql),
        "SELECT COUNT(*) FROM characters WHERE name='%s'",
        g_worldDB->EscapeString(name).c_str());
    QueryResult* res = nullptr;
    bool exists = false;
    if (g_worldDB->Query(sql, res) && res) {
        res->NextRow();
        exists = res->GetUInt32(0) > 0;
        delete res;
    }
    return exists;
}

uint64_t WorldSession::CreateCharacterInDB(const std::string& name, uint8_t race, uint8_t class_,
                                           uint8_t gender, uint8_t skin, uint8_t face,
                                           uint8_t hairStyle, uint8_t hairColor, uint8_t facialFeatures) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO characters (name,race,class,gender,skin,face,hairStyle,hairColor,"
        "facialFeatures,level,experience,health,mana,strength,agility,stamina,intellect,spirit,"
        "positionX,positionY,positionZ,orientation,mapId,zone) "
        "VALUES ('%s',%u,%u,%u,%u,%u,%u,%u,%u,1,0,100,30,20,20,20,20,20,"
        "-8941.0, -132.0, 83.0, 0.0, 0, 1519)",
        g_worldDB->EscapeString(name).c_str(), race, class_, gender, skin, face,
        hairStyle, hairColor, facialFeatures);
    g_worldDB->Execute(sql);
    return g_worldDB->GetLastInsertId();
}

bool WorldSession::DeleteCharacterFromDB(uint64_t guid) {
    char sql[256]; snprintf(sql, sizeof(sql), "DELETE FROM characters WHERE guid=%llu", (unsigned long long)guid);
    return g_worldDB->Execute(sql);
}

void WorldSession::LoadCharactersFromDB(uint32_t accountId, void* chars, size_t max, int* outCount) {
    *outCount = 0;
    (void)chars; (void)max;
}

bool WorldSession::IsOpen() const { return _socket && _socket->IsOpen(); }
