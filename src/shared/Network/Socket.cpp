#include "Network/Socket.h"
#include "Logging/Log.h"
#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <netinet/tcp.h>

// ─── ByteBuffer ───────────────────────────────────────────────────
size_t ByteBuffer::ReadBytes(void* dest, size_t len) {
    if (_rpos + len > _storage.size()) len = _storage.size() - _rpos;
    memcpy(dest, _storage.data() + _rpos, len);
    _rpos += len;
    return len;
}

void ByteBuffer::WriteBytes(const void* data, size_t len) {
    if (_wpos + len > _storage.size()) _storage.resize(_wpos + len);
    memcpy(_storage.data() + _wpos, data, len);
    _wpos += len;
}

uint8_t  ByteBuffer::ReadUInt8()  { uint8_t v=0;  ReadBytes(&v,1); return v; }
uint16_t ByteBuffer::ReadUInt16() { uint16_t v=0; ReadBytes(&v,2); return v; }
uint32_t ByteBuffer::ReadUInt32() { uint32_t v=0; ReadBytes(&v,4); return v; }
uint64_t ByteBuffer::ReadUInt64() { uint64_t v=0; ReadBytes(&v,8); return v; }
float    ByteBuffer::ReadFloat()   { float v=0;   ReadBytes(&v,4); return v; }
double   ByteBuffer::ReadDouble() { double v=0;  ReadBytes(&v,8); return v; }

std::string ByteBuffer::ReadString(uint32_t len) {
    if (len == 0) len = ReadUInt32();
    if (_rpos + len > _storage.size()) len = (uint32_t)(_storage.size() - _rpos);
    std::string s((char*)_storage.data() + _rpos, len);
    _rpos += len;
    return s;
}

std::string ByteBuffer::ReadCString() {
    size_t start = _rpos;
    while (_rpos < _storage.size() && _storage[_rpos] != '\0') ++_rpos;
    std::string s((char*)_storage.data() + start, _rpos - start);
    if (_rpos < _storage.size()) ++_rpos;
    return s;
}

void ByteBuffer::WriteUInt8(uint8_t val)   { WriteBytes(&val,1); }
void ByteBuffer::WriteUInt16(uint16_t val)  { WriteBytes(&val,2); }
void ByteBuffer::WriteUInt32(uint32_t val)  { WriteBytes(&val,4); }
void ByteBuffer::WriteUInt64(uint64_t val)  { WriteBytes(&val,8); }
void ByteBuffer::WriteFloat(float val)       { WriteBytes(&val,4); }
void ByteBuffer::WriteDouble(double val)    { WriteBytes(&val,8); }

void ByteBuffer::WriteString(const std::string& val) {
    WriteUInt32((uint32_t)val.size());
    WriteBytes(val.data(), val.size());
}
void ByteBuffer::WriteCString(const char* val) {
    size_t len = strlen(val) + 1;
    WriteBytes(val, len);
}

// ─── Socket ───────────────────────────────────────────────────────
Socket::Socket(int fd, const std::string& remoteIP, uint16_t remotePort)
    : _fd(fd), _remoteIP(remoteIP), _remotePort(remotePort) { _open.store(true); }
Socket::~Socket() { Close(); }

void Socket::Close() {
    if (_fd >= 0) { ::close(_fd); _fd = -1; }
    _open.store(false);
}
void Socket::SetNoDelay(bool enable) {
    int flag = enable ? 1 : 0;
    setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
}
size_t Socket::Send(const uint8_t* data, size_t len) {
    ssize_t s = ::send(_fd, data, len, 0);
    return s > 0 ? (size_t)s : 0;
}
size_t Socket::Recv(uint8_t* buf, size_t len) {
    ssize_t r = ::recv(_fd, buf, len, 0);
    if (r <= 0) _open.store(false);
    return r > 0 ? (size_t)r : 0;
}

// ─── AuthSocket ────────────────────────────────────────────────────
AuthSocket::AuthSocket(int fd, const std::string& ip, uint16_t port)
    : Socket(fd, ip, port) {}
AuthSocket::~AuthSocket() {
    if (_N) BN_free(_N);
    if (_g) BN_free(_g);
    if (_s) BN_free(_s);
}

void AuthSocket::OnOpen() {
    LOG_INFO("[Auth] Client connected from %s:%u", _remoteIP.c_str(), _remotePort);
}
void AuthSocket::OnClose() {
    LOG_INFO("[Auth] Client disconnected");
}

void AuthSocket::OnMessage(const uint8_t* data, size_t len) {
    if (len < 4) return;
    uint8_t cmd = data[0];
    if (cmd == 0x00) HandleLogonChallenge(data + 4, len - 4);
    else if (cmd == 0x01) HandleLogonProof(data + 4, len - 4);
    else if (cmd == 0x10) HandleRealmlistRequest(data + 4, len - 4);
}

void AuthSocket::SendAuthResponse(uint8_t code) {
    uint8_t pkt[5] = { 0x01, 0x00, code, 0x00, 0x00 };
    Send(pkt, sizeof(pkt));
}

void AuthSocket::HandleLogonChallenge(const uint8_t* pkt, size_t) {
    uint8_t proto = pkt[0];
    if (proto != 3) { SendAuthResponse(0x09); return; }
    uint16_t nameLen = *(uint16_t*)(pkt + 1);
    nameLen = ntohs(nameLen);
    if (nameLen > 64) { SendAuthResponse(0x04); return; }
    _username = std::string((char*)pkt + 3, nameLen);

    // Build challenge response
    uint8_t response[142] = {0};
    response[0] = 0x00;
    response[1] = 0x00;
    response[2] = 0x00;
    memcpy(response + 3, "2.4.3", 5);
    // In production: query DB for account, build real SRP6 challenge
    Send(response, 142);
    (void)_usernameHash;
}

void AuthSocket::HandleLogonProof(const uint8_t* pkt, size_t) {
    // In production: verify SRP6 M1, calculate M2, send AUTH_SUCCESS
    SendAuthResponse(0x00);
    (void)pkt;
}

void AuthSocket::HandleRealmlistRequest(const uint8_t*, size_t) {
    std::vector<std::pair<std::string,std::string>> realms;
    realms.emplace_back("Realm 1 - Lightbringer", "127.0.0.1:8085");
    realms.emplace_back("Realm 2 - Darkmourne",   "127.0.0.1:8086");
    SendRealmListPacket(realms);
}

void AuthSocket::SendRealmListPacket(const std::vector<std::pair<std::string,std::string>>& realms) {
    ByteBuffer buf;
    buf.WriteUInt8(0x10);
    buf.WriteUInt16(0); // placeholder size
    buf.WriteUInt32(0); // padding
    buf.WriteUInt32(0); // realmlist size placeholder
    buf.WriteUInt32(0); // unknown
    uint32_t countPos = buf.GetWritePos();
    buf.WriteUInt32(0); // realm count placeholder
    buf.WriteUInt32(0); // unknown

    for (auto& r : realms) {
        buf.WriteCString(r.first.c_str());   // realm name
        buf.WriteCString(r.second.c_str());  // icon+IP:port
        buf.WriteCString("1");              // color flag
        buf.WriteFloat(1.0f);               // population
        buf.WriteUInt32(0);                 // char count
        buf.WriteUInt8(0);                  // timezone
        buf.WriteUInt32(0);                // unk
    }
    buf.WriteUInt8(0); // terminator

    uint32_t totalSize = (uint32_t)buf.GetStorageSize() - 4;
    *(uint16_t*)(buf.GetData() + 1) = htons((uint16_t)totalSize);
    *(uint32_t*)(buf.GetData() + 5) = htonl((uint32_t)(buf.GetStorageSize() - 21));
    *(uint32_t*)(buf.GetData() + countPos) = htonl((uint32_t)realms.size());

    Send(buf.GetData(), buf.GetStorageSize());
}

// ─── WorldSocket ───────────────────────────────────────────────────
WorldSocket::WorldSocket(int fd, const std::string& ip, uint16_t port)
    : Socket(fd, ip, port), _headerBuf{}, _headerRead(0), _pktBuf{} {}

WorldSocket::~WorldSocket() = default;

void WorldSocket::OnOpen() {
    LOG_INFO("[World] Client connected from %s:%u", _remoteIP.c_str(), _remotePort);
    SetNoDelay(true);
}
void WorldSocket::OnClose() {
    LOG_INFO("[World] Client disconnected");
}

void WorldSocket::OnMessage(const uint8_t* data, size_t len) {
    _pktBuf.insert(_pktBuf.end(), data, data + len);
    ProcessPackets();
}

void WorldSocket::ProcessPackets() {
    while (_pktBuf.size() >= 6) {
        uint16_t size = *(uint16_t*)_pktBuf.data();
        uint16_t opcode = *(uint16_t*)(_pktBuf.data() + 2);
        size = ntohs(size);
        opcode = ntohs(opcode);
        if (_pktBuf.size() < (size_t)(6 + size)) break;

        std::vector<uint8_t> pkt(size);
        memcpy(pkt.data(), _pktBuf.data() + 6, size);
        _pktBuf.erase(_pktBuf.begin(), _pktBuf.begin() + 6 + size);

        HandleIncomingPacket(pkt.data(), pkt.size());
    }
}

void WorldSocket::SendPacket(Opcodes opcode, const ByteBuffer& body) {
    uint8_t hdr[6];
    *(uint16_t*)hdr = htons((uint16_t)body.GetStorageSize());
    *(uint16_t*)(hdr + 2) = htons((uint16_t)opcode);
    std::vector<uint8_t> out;
    out.insert(out.end(), hdr, hdr + 6);
    out.insert(out.end(), body.GetData(), body.GetData() + body.GetStorageSize());
    Send(out.data(), out.size());
}

void WorldSocket::HandleIncomingPacket(const uint8_t* data, size_t len) {
    ByteBuffer buf;
    buf.WriteBytes(data, len);
    // opcode read from header already stripped
    // dispatch to world session handler
    (void)buf;
}
