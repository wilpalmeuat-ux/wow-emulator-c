#include "shared/NetworkCompat.h"
#include "worldserver/WorldServer.h"
#include "worldserver/WorldSession.h"
#include "worldserver/Player.h"
#include "worldserver/Creature.h"
#include "worldserver/GameObject.h"
#include "worldserver/SpellSystem.h"
#include "worldserver/ItemSystem.h"
#include "worldserver/QuestSystem.h"
#include "worldserver/AISystem.h"
#include "worldserver/Chat.h"
#include "shared/Logging/Log.h"
#include "shared/Opcodes.h"
#include "scripting/ScriptEngine.h"

WorldServer& WorldServer::Get() { static WorldServer inst; return inst; }

bool WorldServer::Initialize(const std::string& ip, uint16_t port,
                              const std::string& scriptPath, uint32_t playerLimit) {
    _running = false; _port = port; _playerLimit = playerLimit; _scriptPath = scriptPath;

    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0) { LOG_ERROR("socket() failed"); return false; }
    int opt = 1; setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(_listenFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() failed on port %u", port); return false;
    }
    listen(_listenFd, 100);

    SpellSystem::Get().Initialize();
    ItemSystem::Get().Initialize();
    QuestSystem::Get().Initialize();
    AISystem::Get().Initialize();
    ChatHandler::Get().Initialize();
    WoWEmulator::ScriptEngine::Get().Initialize();
    LoadScripts(_scriptPath);

    LOG_INFO("WorldServer listening on %s:%u", ip.c_str(), port);
    return true;
}

void WorldServer::Shutdown() {
    _running = false;
    closesocket(_listenFd);
    SpellSystem::Get().Shutdown();
    ItemSystem::Get().Shutdown();
    QuestSystem::Get().Shutdown();
    AISystem::Get().Shutdown();
    WoWEmulator::ScriptEngine::Get().Shutdown();
    { std::lock_guard<std::mutex> l(_playerMutex); for (auto& p : _players) delete p.second; _players.clear(); }
    { std::lock_guard<std::mutex> l(_creatureMutex); for (auto& c : _creatures) delete c.second; _creatures.clear(); }
    { std::lock_guard<std::mutex> l(_goMutex); for (auto& g : _gameObjects) delete g.second; _gameObjects.clear(); }
    { std::lock_guard<std::mutex> l(_sessionMutex); for (auto& s : _sessions) delete s.second; _sessions.clear(); }
    LOG_INFO("WorldServer shutdown complete");
}

void WorldServer::Run() {
    _running = true;
    uint32_t lastUpdate = 0, lastSave = 0;
    const uint32_t TICK_MS = 100;

    while (_running) {
        uint32_t now = GetMSTime();
        uint32_t diff = now - lastUpdate; lastUpdate = now;

        struct sockaddr_in caddr{};
        socklen_t clen = sizeof(caddr);
        int csock = accept(_listenFd, (sockaddr*)&caddr, &clen);
        if (csock >= 0) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
            uint16_t cport = ntohs(caddr.sin_port);
            auto* ws = new WorldSocket(csock, ip, cport);
            ws->SetWorldServer(this);
            std::thread([ws] {
                uint8_t buf[16384];
                while (ws->IsOpen()) {
                    auto n = ws->Recv(buf, sizeof(buf));
                    if (n > 0) ws->OnMessage(buf, n);
                    else break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                ws->OnClose(); delete ws;
            }).detach();
        }

        AISystem::Get().Update(diff);
        SpellSystem::Get().Update(diff);
        UpdateSessions(diff);

        if (now - lastSave > 300000) {
            std::lock_guard<std::mutex> l(_playerMutex);
            for (auto& p : _players) p.second->SaveToDB();
            lastSave = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
    }
}

void WorldServer::UpdateSessions(uint32_t diff) {
    std::lock_guard<std::mutex> l(_sessionMutex);
    for (auto it = _sessions.begin(); it != _sessions.end();) {
        if (!it->second->IsOpen()) { delete it->second; it = _sessions.erase(it); }
        else { it->second->Update(diff); ++it; }
    }
}

void WorldServer::AddSession(WorldSession* s) { std::lock_guard<std::mutex> l(_sessionMutex); _sessions[s->GetAccountId()] = s; }
WorldSession* WorldServer::FindSession(uint32_t accountId) { std::lock_guard<std::mutex> l(_sessionMutex); auto it=_sessions.find(accountId); return it!=_sessions.end()?it->second:nullptr; }
Player* WorldServer::GetPlayer(uint64_t guid) { std::lock_guard<std::mutex> l(_playerMutex); auto it=_players.find(guid); return it!=_players.end()?it->second:nullptr; }
void WorldServer::AddPlayer(Player* p) { std::lock_guard<std::mutex> l(_playerMutex); _players[p->GetGUID()] = p; }
void WorldServer::RemovePlayer(uint64_t guid) { std::lock_guard<std::mutex> l(_playerMutex); _players.erase(guid); }
Creature* WorldServer::GetCreature(uint64_t guid) { std::lock_guard<std::mutex> l(_creatureMutex); auto it=_creatures.find(guid); return it!=_creatures.end()?it->second:nullptr; }
void WorldServer::AddCreature(Creature* c) { std::lock_guard<std::mutex> l(_creatureMutex); _creatures[c->GetGUID()] = c; }
void WorldServer::RemoveCreature(uint64_t guid) { std::lock_guard<std::mutex> l(_creatureMutex); _creatures.erase(guid); }
GameObject* WorldServer::GetGameObject(uint64_t guid) { std::lock_guard<std::mutex> l(_goMutex); auto it=_gameObjects.find(guid); return it!=_gameObjects.end()?it->second:nullptr; }
void WorldServer::AddGameObject(GameObject* g) { std::lock_guard<std::mutex> l(_goMutex); _gameObjects[g->GetGUID()] = g; }
void WorldServer::RemoveGameObject(uint64_t guid) { std::lock_guard<std::mutex> l(_goMutex); _gameObjects.erase(guid); }
void WorldServer::BroadcastMessage(const std::string& msg) { std::lock_guard<std::mutex> l(_sessionMutex); for (auto& s : _sessions) s.second->SendAreaMessage(msg); }
void WorldServer::LoadScripts(const std::string& path) { (void)path; WoWEmulator::ScriptEngine::Get().RegisterBuiltInCommands(); LOG_INFO("Scripts loaded from %s", path.c_str()); }
