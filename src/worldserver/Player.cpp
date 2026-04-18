#include "worldserver/Player.h"
#include "worldserver/WorldServer.h"
#include "worldserver/SpellSystem.h"
#include "worldserver/ItemSystem.h"
#include "shared/Logging/Log.h"
#include "shared/Database/Database.h"

extern Database* g_worldDB;

Player::Player(uint64_t guid) : _guid(guid), _session(nullptr) {
    _name[0] = '\0';
    _level = 1; _exp = 0;
    _health = 100; _maxHealth = 100;
    _power = 30; _maxPower = 30;
    _race = 0; _class = 0; _gender = 0;
    _x = -8941.0f; _y = -132.0f; _z = 83.0f; _o = 0.0f;
    _mapId = 0; _zoneId = 1519;
    _gold = 0;
    _standState = 0; _sheathState = 0;
    _canMove = true;
    _critChance = 5.0f; _dodgeChance = 5.0f; _parryChance = 5.0f;
    _respawnX = _x; _respawnY = _y; _respawnZ = _z; _respawnO = _o; _respawnMapId = _mapId;
    _regenTimer = 0;
}

Player::~Player() = default;

void Player::Update(uint32_t diff) {
    _regenTimer += diff;
    if (_regenTimer >= 5000) {
        if (_health < _maxHealth) { _health += (_level * 2); if (_health > _maxHealth) _health = _maxHealth; }
        if (_power < _maxPower)   { _power  += (_level);     if (_power  > _maxPower)  _power  = _maxPower; }
        _regenTimer = 0;
    }
}

void Player::Relocate(float x, float y, float z, float o) { _x=x; _y=y; _z=z; _o=o; }
void Player::SetPosition(float x, float y, float z, float o, uint32_t map, uint32_t zone) {
    Relocate(x,y,z,o); _mapId=map; _zoneId=zone;
}
void Player::SetLevel(uint32_t level) {
    _level = level;
    _maxHealth = 100 + (level-1)*10 + (_class==1 ? level*12 : level*8);
    _maxPower  = 30  + (level-1)*5;
    _health = _maxHealth; _power = _maxPower;
    CalcStats();
}
void Player::GiveExperience(uint32_t xp) {
    if (_level >= 80) return;
    _exp += xp;
    uint32_t xpTable[81] = {0,400,900,1400,2100,2800,3600,4500,5400,6500,
        7500,8800,10000,11500,13000,15000,17000,19000,21000,24000,
        27000,30000,33000,36000,40000,44000,48000,52000,57000,62000,
        67000,73000,79000,86000,93000,100000,108000,116000,124000,133000,
        142000,151000,161000,171000,182000,193000,205000,217000,230000,243000,
        257000,271000,286000,301000,317000,334000,351000,369000,387000,406000,
        426000,446000,467000,489000,512000,535000,559000,584000,610000,637000,
        665000,693000,722000,752000,783000,815000,848000,882000,917000,953000,990000};
    while (_exp >= xpTable[_level] && _level < 80) {
        _exp -= xpTable[_level];
        SetLevel(_level + 1);
        if (_session) _session->SendNotification("You leveled up!");
    }
}
void Player::Resurrect() {
    _health = _maxHealth / 2;
    _standState = 0;
    SetPosition(_respawnX, _respawnY, _respawnZ, _respawnO, _respawnMapId, _zoneId);
}
void Player::Kill() {
    _health = 0; _standState = 4;
    _respawnX=_x; _respawnY=_y; _respawnZ=_z; _respawnO=_o; _respawnMapId=_mapId;
}
Team Player::GetTeam() const {
    if (_team != TEAM_NONE) return _team;
    if (_race==1||_race==3||_race==4||_race==7||_race==11) return TEAM_ALLIANCE;
    if (_race==2||_race==5||_race==6||_race==8||_race==10) return TEAM_HORDE;
    return TEAM_ALLIANCE;
}
void Player::CalcStats() {
    if (_class == 1) { // Warrior
        _strength = 20+_level*3; _agility = 20+_level*2; _stamina = 20+_level*3;
        _intellect = 5+_level; _spirit = 5+_level;
    } else if (_class == 5) { // Priest
        _strength = 15+_level; _agility = 15+_level; _stamina = 15+_level*2;
        _intellect = 20+_level*3; _spirit = 20+_level*3;
    } else {
        _strength = 15+_level*2; _agility = 15+_level*2; _stamina = 15+_level*2;
        _intellect = 15+_level*2; _spirit = 15+_level*2;
    }
    _armor = (uint32_t)(_agility * 2);
    _maxHealth = 100 + _stamina * 10;
    _maxPower  = 30  + _intellect * 5;
}
void Player::SaveToDB() {
    if (!g_worldDB) return;
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "UPDATE characters SET level=%u,experience=%u,health=%u,mana=%u,"
        "positionX=%.2f,positionY=%.2f,positionZ=%.2f,orientation=%.4f,"
        "mapId=%u,zone=%u,gold=%u WHERE guid=%llu",
        _level,_exp,_health,_power,_x,_y,_z,_o,_mapId,_zoneId,_gold,(unsigned long long)_guid);
    g_worldDB->Execute(sql);
}
Player* Player::LoadFromDB(uint64_t guid, Database* db) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT name,race,class,gender,level,experience,health,mana,gold,"
        "positionX,positionY,positionZ,orientation,mapId,zone FROM characters WHERE guid=%llu",
        (unsigned long long)guid);
    QueryResult* res = nullptr;
    if (!db->Query(sql, res) || !res || res->rowCount==0) { delete res; return nullptr; }
    res->NextRow();
    Player* p = new Player(guid);
    strncpy(p->_name, res->GetString(0), sizeof(p->_name)-1);
    p->_race = res->GetUInt8(1); p->_class = res->GetUInt8(2); p->_gender = res->GetUInt8(3);
    p->_level = res->GetUInt8(4); p->_exp = res->GetUInt32(5);
    p->_health = res->GetUInt32(6); p->_power = res->GetUInt32(7); p->_gold = res->GetUInt32(8);
    p->_x = res->GetFloat(9); p->_y = res->GetFloat(10); p->_z = res->GetFloat(11); p->_o = res->GetFloat(12);
    p->_mapId = res->GetUInt32(13); p->_zoneId = res->GetUInt32(14);
    p->CalcStats();
    if (p->_health == 0) p->_health = p->_maxHealth;
    if (p->_power == 0)  p->_power = p->_maxPower;
    delete res;
    return p;
}
void Player::SendNotification(const std::string& msg) {
    if (!_session) return;
    ByteBuffer b; b.WriteUInt32(0); b.WriteUInt32(0); b.WriteString(msg); b.WriteUInt8(0);
    _session->SendPacket((Opcodes)0x36, b);
}
void Player::SendInitialSpells() {
    ByteBuffer b; b.WriteUInt16(0);
    _session->SendPacket((Opcodes)0x1D, b);
}
void Player::SendInitialTalents() {
    ByteBuffer b; b.WriteUInt16(0);
    _session->SendPacket((Opcodes)0x1F, b);
}
void Player::SendInitialEquipment() {
    ByteBuffer b; b.WriteUInt8(0);
    _session->SendPacket((Opcodes)0x2E, b);
}
void Player::SendInitialInventory() {
    ByteBuffer b; b.WriteUInt8(0);
    _session->SendPacket((Opcodes)0x38, b);
}
