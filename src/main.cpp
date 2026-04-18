#include <iostream>
#include <string>
#include <cstring>
#include "authserver/AuthServer.h"
#include "worldserver/WorldServer.h"
#include "shared/Opcodes.h"
#include "scripting/ScriptEngine.h"

void PrintBanner() {
    std::cout << "\n";
    std::cout << "  ██╗  ██╗███████╗███╗   ██╗ ██████╗ ███╗   ██╗\n";
    std::cout << "  ██║ ██╔╝██╔════╝████╗  ██║██╔═══██╗████╗  ██║\n";
    std::cout << "  █████╔╝ █████╗  ██╔██╗ ██║██║   ██║██╔██╗ ██║\n";
    std::cout << "  ██╔═██╗ ██╔══╝  ██║╚██╗██║██║   ██║██║╚██╗██║\n";
    std::cout << "  ██║  ██╗███████╗██║ ╚████║╚██████╔╝██║ ╚████║\n";
    std::cout << "  ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═══╝\n";
    std::cout << "          EMULATOR v3.3.5a\n";
    std::cout << "        Custom Word-Based Scripting Engine\n";
    std::cout << "\n";
}

void PrintUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --authserver       Run auth server only\n";
    std::cout << "  --worldserver      Run world server only\n";
    std::cout << "  --both             Run both servers (default)\n";
    std::cout << "  --config <file>    Use alternate config file\n";
    std::cout << "  --script <path>    Set script directory\n";
    std::cout << "  --help             Show this help\n";
}

int main(int argc, char** argv) {
    PrintBanner();
    
    bool runAuth = true;
    bool runWorld = true;
    std::string configFile = "emulator.conf";
    std::string scriptPath = "./scripts";
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--authserver") == 0) {
            runAuth = true;
            runWorld = false;
        }
        else if (strcmp(argv[i], "--worldserver") == 0) {
            runAuth = false;
            runWorld = true;
        }
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configFile = argv[++i];
        }
        else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
            scriptPath = argv[++i];
        }
    }
    
    if (runAuth) {
        AuthServerConfig authConfig;
        authConfig.bindIP = "0.0.0.0";
        authConfig.port = 3724;
        authConfig.logFile = "auth.log";
        authConfig.threadCount = 1;
        
        if (!AUTH_SERVER.Initialize(authConfig)) {
            std::cerr << "[ERROR] Failed to initialize Auth Server\n";
            return 1;
        }
        std::cout << "[OK] Auth Server initialized on port " << authConfig.port << "\n";
    }
    
    if (runWorld) {
        WorldServerConfig worldConfig;
        worldConfig.bindIP = "0.0.0.0";
        worldConfig.port = 8085;
        worldConfig.logFile = "world.log";
        worldConfig.threadCount = 4;
        worldConfig.playerLimit = 1000;
        worldConfig.saveInterval = 300;
        worldConfig.scriptPath = scriptPath;
        worldConfig.databaseHost = "localhost";
        worldConfig.databasePort = 3306;
        worldConfig.databaseUser = "root";
        worldConfig.databasePass = "";
        worldConfig.databaseName = "world";
        
        if (!WORLD_SERVER.Initialize(worldConfig)) {
            std::cerr << "[ERROR] Failed to initialize World Server\n";
            return 1;
        }
        std::cout << "[OK] World Server initialized on port " << worldConfig.port << "\n";
        std::cout << "[OK] Script path: " << worldConfig.scriptPath << "\n";
    }
    
    std::cout << "\n[INFO] Servers running. Press Ctrl+C to stop.\n";
    
    while (true) {
    }
    
    return 0;
}
