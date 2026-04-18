/* main_win.c — WoW 3.3.5a Emulator entry point for Windows */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "worldserver/world_server.h"
#include "authserver/auth_server.h"
#include "scripting/script_engine.h"
#include "shared/database.h"
#include "Config.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

static volatile int g_running = 1;

static BOOL WINAPI _sig(DWORD ctrl) {
    (void)ctrl;
    g_running = 0;
    printf("\n[Main] Shutting down...\n");
    return TRUE;
}

/* Load all .wss and .txt scripts from a directory */
static void _load_scripts(ScriptEngine* e, const char* dir) {
    WIN32_FIND_DATA find = {0};
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s\\*.wss", dir);
    HANDLE h = FindFirstFile(pattern, &find);
    int count = 0;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char path[512];
            snprintf(path, sizeof(path), "%s\\%s", dir, find.cFileName);
            printf("[Main] Loading script: %s\n", find.cFileName);
            if (!ScriptEngine_LoadScript(e, path))
                printf("[Main]   Error: %s\n", ScriptEngine_LastError(e));
            else
                printf("[Main]   OK\n"), count++;
        } while (FindNextFile(h, &find));
        FindClose(h);
    }
    snprintf(pattern, sizeof(pattern), "%s\\*.txt", dir);
    h = FindFirstFile(pattern, &find);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char path[512];
            snprintf(path, sizeof(path), "%s\\%s", dir, find.cFileName);
            printf("[Main] Loading script: %s\n", find.cFileName);
            if (!ScriptEngine_LoadScript(e, path))
                printf("[Main]   Error: %s\n", ScriptEngine_LastError(e));
            else
                printf("[Main]   OK\n"), count++;
        } while (FindNextFile(h, &find));
        FindClose(h);
    }
    printf("[Main] Loaded %d script(s)\n", count);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    printf("\n");
    printf("  +=============================================+\n");
    printf("  |   WoW 3.3.5a Emulator  -  C + Word Scripts |\n");
    printf("  +=============================================+\n\n");

    /* WinSock init */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Main] FATAL: WSAStartup failed\n");
        return 1;
    }

    SetConsoleCtrlHandler(_sig, TRUE);

    /* Load config */
    printf("[Main] Loading config...\n");
    Config* cfg = Config_Load(NULL);
    if (!cfg) {
        printf("[Main] FATAL: Could not load config\n");
        return 1;
    }
    printf("[Main] Config: MySQL %s@%s:%d/%s\n",
           cfg->dbUser, cfg->dbHost, cfg->dbPort, cfg->dbDatabase);

    /* Initialize MySQL database */
    printf("[Main] Connecting to MySQL...\n");
    DBConfig dbCfg = {0};
    dbCfg.type = 0; /* MySQL */
    dbCfg.host = cfg->dbHost;
    dbCfg.port = cfg->dbPort;
    dbCfg.user = cfg->dbUser;
    dbCfg.password = cfg->dbPassword;
    dbCfg.database = cfg->dbDatabase;
    Database* db = database_create(&dbCfg);
    if (!db) {
        printf("[Main] FATAL: MySQL connection failed\n");
        printf("[Main] Make sure MySQL Server is running and credentials are correct.\n");
        printf("[Main] Edit configs\\worldserver.conf to set correct password.\n");
        return 1;
    }
    printf("[Main] MySQL connected successfully\n");

    /* Start auth server */
    printf("\n[Main] Starting AuthServer on port %u...\n", cfg->authserverPort);
    AuthServer* auth = AuthServer_Create();
    if (!AuthServer_Start(auth, cfg->authserverPort)) {
        printf("[Main] WARNING: AuthServer bind failed on port %u\n", cfg->authserverPort);
        printf("[Main] May need to run as Administrator or use a different port.\n");
    } else {
        printf("[Main] AuthServer listening on 0.0.0.0:%u\n", cfg->authserverPort);
    }

    /* Start world server */
    printf("\n[Main] Starting WorldServer on port %u...\n", cfg->worldserverPort);
    WorldServer* ws = WorldServer_Create(cfg->worldserverPort);
    if (!ws) {
        printf("[Main] FATAL: WorldServer creation failed\n");
        return 1;
    }
    if (!WorldServer_Start(ws)) {
        printf("[Main] FATAL: WorldServer failed to start on port %u\n", cfg->worldserverPort);
        return 1;
    }
    printf("[Main] WorldServer listening on 0.0.0.0:%u\n", cfg->worldserverPort);

    /* Register built-in script statements */
    printf("\n[Main] Registering built-in WSS statements...\n");
    extern void ScriptEngine_RegisterBuiltins(ScriptEngine* e);
    ScriptEngine_RegisterBuiltins(ws->scriptEngine);
    printf("[Main] %d built-in statements registered\n", ws->scriptEngine->statementCount);

    /* Load WSS scripts */
    printf("\n[Main] Loading WSS scripts...\n");
    _load_scripts(ws->scriptEngine, "scripts");
    _load_scripts(ws->scriptEngine, "scripts\\core");
    _load_scripts(ws->scriptEngine, ".\\scripts");
    _load_scripts(ws->scriptEngine, ".\\scripts\\core");

    /* Spawn creatures from DB (if any) */
    printf("\n[Main] Spawning creatures from database...\n");
    WorldServer_LoadCreatures(ws);
    WorldServer_LoadGameObjects(ws);

    /* Demo creatures */
    printf("[Main] Spawning demo creatures at Orgrimmar (map=0)...\n");
    WorldServer_SpawnCreature(ws, 1,   0, -8949.0f, -132.0f,  83.0f, 0.0f);
    WorldServer_SpawnCreature(ws, 1,   0, -8945.0f, -130.0f,  83.0f, 1.0f);
    WorldServer_SpawnCreature(ws, 15,  0, -8920.0f, -140.0f,  84.0f, 2.0f);
    WorldServer_SpawnCreature(ws, 50,  0, -8900.0f, -145.0f,  85.0f, 0.5f);
    WorldServer_SpawnCreature(ws, 999, 1, -10806.f,  284.0f,  35.0f, 0.0f);
    WorldServer_SpawnCreature(ws, 9999,0, -8940.0f, -120.0f,  83.5f, 0.0f);
    WorldServer_SpawnCreature(ws, 1234,0, -8945.0f, -115.0f,  83.0f, 3.14f);
    WorldServer_SpawnCreature(ws, 5678,0, -8955.0f, -125.0f,  83.0f, 1.5f);
    WorldServer_SpawnCreature(ws, 9001,0, -8925.0f, -135.0f,  83.0f, 0.0f);
    printf("[Main] Demo creatures spawned\n");

    printf("\n");
    printf("  =======================================\n");
    printf("  WoW 3.3.5a Server Online!  (Windows)\n");
    printf("  =======================================\n");
    printf("  Auth Port  : %u\n", cfg->authserverPort);
    printf("  World Port : %u\n", cfg->worldserverPort);
    printf("  Database   : MySQL (%s:%d)\n", cfg->dbHost, cfg->dbPort);
    printf("  Scripts    : active (WSS word/statement)\n");
    printf("  =======================================\n");
    printf("  Default account: test / testpassword\n");
    printf("\n  Realmlist: 127.0.0.1 %u\n", cfg->worldserverPort);
    printf("\n  Press Ctrl+C to stop.\n\n");

    /* Main update loop */
    uint32_t tick = 0;
    time_t startTime = time(NULL);

    while (g_running) {
        uint32_t diffMs = 50;
        WorldServer_Update(ws, diffMs);

        if (tick % 1200 == 0 && tick > 0) {
            time_t now = time(NULL);
            int uptime = (int)(now - startTime);
            int h = uptime / 3600;
            int m = (uptime % 3600) / 60;
            int s = uptime % 60;
            printf("[Status] Tick %u | Uptime %02d:%02d:%02d | Online: 0\n", tick, h, m, s);
        }

        tick++;
        Sleep(50);
    }

    printf("\n[Main] Saving data...\n");
    database_free(db);
    WorldServer_Stop(ws);
    AuthServer_Stop(auth);
    WorldServer_Destroy(ws);
    AuthServer_Destroy(auth);
    Config_Free(cfg);
    WSACleanup();
    printf("[Main] Done.\n");
    return 0;
}
