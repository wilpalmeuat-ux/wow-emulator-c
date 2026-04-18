#include "worldserver/WorldSocket.h"
#include "worldserver/WorldServer.h"
#include "worldserver/WorldSession.h"
#include "shared/Logging/Log.h"

WorldSocket::WorldSocket(int fd, const std::string& ip, uint16_t port)
    : Socket(fd, ip, port), _session(nullptr), _worldServer(nullptr) {
    _headerBuf.fill(0);
    _headerRead = 0;
}

WorldSocket::~WorldSocket() = default;

void WorldSocket::OnOpen() {
    LOG_INFO("[WorldSocket] %s:%u connected", GetRemoteIP().c_str(), GetRemotePort());
    SetNoDelay(true);
}

void WorldSocket::OnClose() {
    if (_session) { _session->Logout(true); delete _session; _session = nullptr; }
    LOG_INFO("[WorldSocket] disconnected");
}

void WorldSocket::OnMessage(const uint8_t* data, size_t len) {
    _recvBuf.insert(_recvBuf.end(), data, data + len);
    ProcessPackets();
}

void WorldSocket::ProcessPackets() {
    while (_recvBuf.size() >= 6) {
        if (_headerRead < 6) {
            size_t need = 6 - _headerRead;
            if (_recvBuf.size() < need) return;
            memcpy(_headerBuf.data() + _headerRead, _recvBuf.data(), need);
            _recvBuf.erase(_recvBuf.begin(), _recvBuf.begin() + need);
            _headerRead = 6;
        }
        uint16_t size = ntohs(*(uint16_t*)_headerBuf);
        uint16_t opcode = ntohs(*(uint16_t*)(_headerBuf + 2));
        if (size > 32768) { Close(); return; }
        if (_recvBuf.size() < size) { _headerRead = 0; return; }

        std::vector<uint8_t> pkt(_recvBuf.begin(), _recvBuf.begin() + size);
        _recvBuf.erase(_recvBuf.begin(), _recvBuf.begin() + size);
        _headerRead = 0;

        if (_session) {
            _session->HandlePacket(opcode, pkt.data(), pkt.size());
        } else {
            LOG_DEBUG("Pre-auth packet opcode 0x%04X", opcode);
        }
    }
}

void WorldSocket::SendPacket(Opcodes opcode, const ByteBuffer& body) {
    uint8_t hdr[6];
    *(uint16_t*)hdr = htons((uint16_t)body.GetStorageSize());
    *(uint16_t*)(hdr+2) = htons((uint16_t)opcode);
    std::vector<uint8_t> out;
    out.insert(out.end(), hdr, hdr+6);
    out.insert(out.end(), body.GetData(), body.GetData()+body.GetStorageSize());
    Send(out.data(), out.size());
}

void WorldSocket::SetWorldServer(WorldServer* ws) { _worldServer = ws; }
void WorldSocket::SetSession(WorldSession* s) { _session = s; }
