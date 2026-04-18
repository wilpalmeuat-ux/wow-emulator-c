#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "worldserver/WorldServer.h"
#include "shared/Logging/Log.h"
#include "shared/Config/Config.h"
#include "shared/Database/Database.h"

static bool g_running = true;
static void SignalHandler(int) { g_running = false; }

Database* g_worldDB = nullptr;
Database* g_authDB = nullptr;

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    Log_Init("world.log");
    LOG_INFO("=== WorldServer starting ===");

    Config::Get().Load("worldserver.conf");

    g_authDB = Database::Create("mysql",
        Config::Get().GetString("WorldServer.AuthDBHost", "127.0.0.1"),
        Config::Get().GetInt("WorldServer.AuthDBPort", 3306),
        Config::Get().GetString("WorldServer.AuthDBUser", "root"),
        Config::Get().GetString("WorldServer.AuthDBPass", ""), "auth");

    g_worldDB = Database::Create("mysql",
        Config::Get().GetString("WorldServer.WorldDBHost", "127.0.0.1"),
        Config::Get().GetInt("WorldServer.WorldDBPort", 3306),
        Config::Get().GetString("WorldServer.WorldDBUser", "root"),
        Config::Get().GetString("WorldServer.WorldDBPass", ""), "world");

    if (!g_authDB->Initialize()) LOG_ERROR("Failed to connect to auth DB");
    if (!g_worldDB->Initialize()) LOG_ERROR("Failed to connect to world DB");

    if (!WorldServer::Get().Initialize(
        Config::Get().GetString("WorldServer.BindIP", "0.0.0.0"),
        Config::Get().GetInt("WorldServer.Port", 8085),
        Config::Get().GetString("WorldServer.ScriptPath", "./scripts"),
        Config::Get().GetInt("WorldServer.PlayerLimit", 1000))) {
        LOG_ERROR("WorldServer initialization failed"); return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    WorldServer::Get().Run();

    while (g_running) std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG_INFO("=== WorldServer shutting down ===");
    WorldServer::Get().Shutdown();
    if (g_authDB) { g_authDB->Close(); delete g_authDB; }
    if (g_worldDB) { g_worldDB->Close(); delete g_worldDB; }
    Log_Close();
    return 0;
}
