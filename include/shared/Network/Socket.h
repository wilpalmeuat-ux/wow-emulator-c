#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <vector>
#include <openssl/bn.h>

enum class Opcodes : uint32_t;

// ─── ByteBuffer ───────────────────────────────────────────────────
class ByteBuffer {
public:
    ByteBuffer() : _rpos(0), _wpos(0) {}
    explicit ByteBuffer(size_t res) { _storage.reserve(res); }

    void Clear() { _storage.clear(); _rpos = 0; _wpos = 0; }

    size_t ReadBytes(void* dest, size_t len);
    void WriteBytes(const void* data, size_t len);

    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    int8_t ReadInt8() { return (int8_t)ReadUInt8(); }
    int16_t ReadInt16() { return (int16_t)ReadUInt16(); }
    int32_t ReadInt32() { return (int32_t)ReadUInt32(); }
    int64_t ReadInt64() { return (int64_t)ReadUInt64(); }
    float ReadFloat();
    double ReadDouble();
    std::string ReadString(uint32_t len = 0);
    std::string ReadCString();

    void WriteUInt8(uint8_t val);
    void WriteUInt16(uint16_t val);
    void WriteUInt32(uint32_t val);
    void WriteUInt64(uint64_t val);
    void WriteInt8(int8_t val) { WriteUInt8((uint8_t)val); }
    void WriteInt16(int16_t val) { WriteUInt16((uint16_t)val); }
    void WriteInt32(int32_t val) { WriteUInt32((uint32_t)val); }
    void WriteInt64(int64_t val) { WriteUInt64((uint64_t)val); }
    void WriteFloat(float val);
    void WriteDouble(double val);
    void WriteString(const std::string& val);
    void WriteCString(const char* val);

    size_t GetReadPos() const { return _rpos; }
    void SetReadPos(size_t p) { _rpos = p; }
    size_t GetWritePos() const { return _wpos; }
    void SetWritePos(size_t p) { _wpos = p; }
    size_t GetActiveSize() const { return _storage.size() - _rpos; }
    size_t GetStorageSize() const { return _storage.size(); }
    const uint8_t* GetData() const { return _storage.data(); }
    uint8_t* GetData() { return _storage.data(); }

    void PrintHex() const;

private:
    std::vector<uint8_t> _storage;
    size_t _rpos = 0;
    size_t _wpos = 0;

    void _resize(size_t len);
};

// ─── Socket ───────────────────────────────────────────────────────
class Socket {
public:
    Socket(int fd, const std::string& remoteIP, uint16_t remotePort);
    virtual ~Socket();

    int GetFd() const { return _fd; }
    bool IsOpen() const { return _open.load(); }
    void Close();
    void SetNoDelay(bool enable);

    size_t Send(const uint8_t* data, size_t len);
    size_t Recv(uint8_t* buf, size_t len);

    virtual void OnOpen() {}
    virtual void OnClose() {}
    virtual void OnMessage(const uint8_t* data, size_t len) = 0;

    const std::string& GetRemoteIP() const { return _remoteIP; }
    uint16_t GetRemotePort() const { return _remotePort; }

protected:
    int _fd = -1;
    std::string _remoteIP;
    uint16_t _remotePort = 0;
    std::atomic<bool> _open{false};
};

// ─── AuthSocket ────────────────────────────────────────────────────
class AuthSocket : public Socket {
public:
    AuthSocket(int fd, const std::string& ip, uint16_t port);
    ~AuthSocket() override;

    void OnOpen() override;
    void OnClose() override;
    void OnMessage(const uint8_t* data, size_t len) override;

    void SendAuthResponse(uint8_t code);
    void SendRealmListPacket(const std::vector<std::pair<std::string, std::string>>& realms);

private:
    void HandleLogonChallenge(const uint8_t* pkt, size_t len);
    void HandleLogonProof(const uint8_t* pkt, size_t len);
    void HandleRealmlistRequest(const uint8_t* pkt, size_t len);

    uint32_t _accountId = 0;
    std::string _username;
    uint8_t _usernameHash[32] = {0};

    BIGNUM* _N = nullptr;
    BIGNUM* _g = nullptr;
    BIGNUM* _s = nullptr;
    uint8_t _sessionKey[40] = {0};
};

// ─── WorldSocket ───────────────────────────────────────────────────
class WorldSocket : public Socket {
public:
    WorldSocket(int fd, const std::string& ip, uint16_t port);
    ~WorldSocket() override;

    void OnOpen() override;
    void OnClose() override;
    void OnMessage(const uint8_t* data, size_t len) override;

    void SendPacket(const uint8_t* data, size_t len);
    void SendPacket(Opcodes opcode, const ByteBuffer& body);
    void HandleIncomingPacket(const uint8_t* data, size_t len);

    uint32_t GetAccountId() const { return _accountId; }
    void SetAccountId(uint32_t id) { _accountId = id; }

private:
    uint32_t _accountId = 0;
    uint32_t _build = 0;
    uint8_t _localChallenge[16] = {0};
    bool _authed = false;
};
