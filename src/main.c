/* main.c — WoW 3.3.5a Emulator entry point (Windows Only)
 * Built for MSVC + WinSock2. No POSIX dependencies.
 */
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

/* ── Global running flag ─────────────────────────────────── */
static volatile int g_running = 1;

/* ── Console ctrl handler (Ctrl+C / Ctrl+Break) ─────────── */
static BOOL WINAPI _sig(DWORD ctrl) {
    (void)ctrl;
    if (g_running) {
        g_running = 0;
        printf("\n[Main] Shutdown requested... Press any key to exit.\n");
    }
    return TRUE;
}

/* ── Script directory loader (Windows FindFirstFile API) ─── */
static void _load_scripts(ScriptEngine* e, const char* dir) {
    WIN32_FIND_DATAA find = {0};
    char pattern[MAX_PATH];
    int count = 0;

    /* Load *.wss */
    snprintf(pattern, sizeof(pattern), "%s\\*.wss", dir);
    HANDLE h = FindFirstFileA(pattern, &find);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s\\%s", dir, find.cFileName);
            printf("[Main] Loading script: %s\n", find.cFileName);
            if (!ScriptEngine_LoadScript(e, path))
                printf("[Main]   Error: %s\n", ScriptEngine_LastError(e));
            else
                printf("[Main]   OK\n"), count++;
        } while (FindNextFileA(h, &find));
        FindClose(h);
    }

    /* Load *.txt */
    snprintf(pattern, sizeof(pattern), "%s\\*.txt", dir);
    h = FindFirstFileA(pattern, &find);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s\\%s", dir, find.cFileName);
            printf("[Main] Loading script: %s\n", find.cFileName);
            if (!ScriptEngine_LoadScript(e, path))
                printf("[Main]   Error: %s\n", ScriptEngine_LastError(e));
            else
                printf("[Main]   OK\n"), count++;
        } while (FindNextFileA(h, &find));
        FindClose(h);
    }

    printf("[Main] Loaded %d script(s) from: %s\n", count, dir);
}

/* ── Resolve config path (support relative + absolute) ───── */
static void _resolve_config_path(char* out, size_t sz, const char* argv0, const char* filename) {
    /* Try exe-relative first */
    const char* sep = strrchr(argv0, '\\');
    if (sep) {
        snprintf(out, sz, "%.*s\\%s", (int)(sep - argv0 + 1), argv0, filename);
        FILE* f = fopen(out, "r");
        if (f) { fclose(f); return; }
    }
    /* Try current dir */
    snprintf(out, sz, "%s", filename);
    FILE* f = fopen(out, "r");
    if (f) { fclose(f); return; }
    /* Fallback to configs subdir */
    snprintf(out, sz, "configs\\%s", filename);
}

/* ── Main ─────────────────────────────────────────────────── */
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\n");
    printf("  +==============================================+\n");
    printf("  |   WoW 3.3.5a Emulator  -  C + Word Scripts   |\n");
    printf("  |   Windows Build  (MSVC + WinSock2)            |\n");
    printf("  +==============================================+\n\n");

    /* WinSock2 startup */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Main] FATAL: WSAStartup failed with error %d\n", WSAGetLastError());
        return 1;
    }

    /* Console ctrl handler */
    SetConsoleCtrlHandler(_sig, TRUE);

    /* Load configuration */
    printf("[Main] Loading configuration...\n");
    char configPath[MAX_PATH];
    _resolve_config_path(configPath, sizeof(configPath), argv[0], "worldserver.conf");

    Config* cfg = Config_Load(configPath);
    if (!cfg) {
        printf("[Main] FATAL: Could not load config from: %s\n", configPath);
        printf("[Main] Ensure configs\\worldserver.conf exists and is readable.\n");
        WSACleanup();
        return 1;
    }

    printf("[Main] Config loaded:\n");
    printf("       MySQL    : %s@%s:%d/%s\n",
           cfg->dbUser, cfg->dbHost, cfg->dbPort, cfg->dbDatabase);
    printf("       AuthPort : %u\n", cfg->authserverPort);
    printf("       WorldPort: %u\n", cfg->worldserverPort);

    /* Initialize database */
    printf("\n[Main] Connecting to MySQL...\n");
    DBConfig dbCfg = {0};
    dbCfg.type = 0;          /* 0 = MySQL */
    dbCfg.host = cfg->dbHost;
    dbCfg.port = cfg->dbPort;
    dbCfg.user = cfg->dbUser;
    dbCfg.pass = cfg->dbPassword;
    dbCfg.database = cfg->dbDatabase;

    Database* db = database_create(&dbCfg);
    if (!db) {
        printf("[Main] FATAL: MySQL connection failed.\n");
        printf("[Main] Verify MySQL Server is running and credentials are correct.\n");
        printf("[Main] Edit: configs\\worldserver.conf\n");
        Config_Free(cfg);
        WSACleanup();
        return 1;
    }
    printf("[Main] MySQL connected successfully.\n");

    /* Create auth server */
    printf("\n[Main] Starting AuthServer on port %u...\n", cfg->authserverPort);
    AuthServer* auth = AuthServer_Create();
    if (!AuthServer_Start(auth, cfg->authserverPort)) {
        printf("[Main] WARNING: AuthServer failed to bind port %u.\n", cfg->authserverPort);
        printf("[Main] Try running as Administrator, or change AuthServerPort in config.\n");
    } else {
        printf("[Main] AuthServer listening on 0.0.0.0:%u\n", cfg->authserverPort);
    }

    /* Create world server */
    printf("\n[Main] Starting WorldServer on port %u...\n", cfg->worldserverPort);
    WorldServer* ws = WorldServer_Create(cfg->worldserverPort);
    if (!ws) {
        printf("[Main] FATAL: WorldServer creation failed.\n");
        database_free(db);
        AuthServer_Destroy(auth);
        Config_Free(cfg);
        WSACleanup();
        return 1;
    }
    if (!WorldServer_Start(ws)) {
        printf("[Main] FATAL: WorldServer failed to start on port %u.\n", cfg->worldserverPort);
        WorldServer_Destroy(ws);
        database_free(db);
        AuthServer_Destroy(auth);
        Config_Free(cfg);
        WSACleanup();
        return 1;
    }
    printf("[Main] WorldServer listening on 0.0.0.0:%u\n", cfg->worldserverPort);

    /* Register built-in WSS statements */
    printf("\n[Main] Registering built-in WSS statements...\n");
    extern void ScriptEngine_RegisterBuiltins(ScriptEngine* e);
    ScriptEngine_RegisterBuiltins(ws->scriptEngine);
    printf("[Main] %d built-in statements registered.\n", ws->scriptEngine->statementCount);

    /* Load WSS scripts from multiple locations */
    printf("\n[Main] Loading WSS scripts...\n");
    _load_scripts(ws->scriptEngine, "scripts");
    _load_scripts(ws->scriptEngine, "scripts\\core");
    _load_scripts(ws->scriptEngine, ".\\scripts");
    _load_scripts(ws->scriptEngine, ".\\scripts\\core");

    /* Load creatures and game objects from DB */
    printf("\n[Main] Loading world data from database...\n");
    WorldServer_LoadCreatures(ws);
    WorldServer_LoadGameObjects(ws);

    /* Demo creatures (always spawned as fallback) */
    printf("[Main] Spawning demo creatures at Orgrimmar (map=0)...\n");
    WorldServer_SpawnCreature(ws, 1,    0, -8949.0f, -132.0f,   83.0f, 0.0f);   /* Guard */
    WorldServer_SpawnCreature(ws, 1,    0, -8945.0f, -130.0f,   83.0f, 1.0f);   /* Guard */
    WorldServer_SpawnCreature(ws, 15,   0, -8920.0f, -140.0f,   84.0f, 2.0f);   /* Grunt */
    WorldServer_SpawnCreature(ws, 50,   0, -8900.0f, -145.0f,   85.0f, 0.5f);   /* Combat Trainer */
    WorldServer_SpawnCreature(ws, 999,  1, -10806.f,  284.0f,   35.0f, 0.0f);   /* Stormwind Soldier */
    WorldServer_SpawnCreature(ws, 9999, 0, -8940.0f, -120.0f,   83.5f, 0.0f);   /* World Boss */
    WorldServer_SpawnCreature(ws, 1234, 0, -8945.0f, -115.0f,   83.0f, 3.14f);  /* Kel'Thuzad */
    WorldServer_SpawnCreature(ws, 5678, 0, -8955.0f, -125.0f,   83.0f, 1.5f);   /* Arcane Golem */
    WorldServer_SpawnCreature(ws, 9001, 0, -8925.0f, -135.0f,   83.0f, 0.0f);   /* Portal Guardian */
    printf("[Main] Demo creatures spawned.\n");

    /* ── Banner ───────────────────────────────────── */
    printf("\n");
    printf("  ==========================================\n");
    printf("   WoW 3.3.5a Server Online!   (Windows)\n");
    printf("  ==========================================\n");
    printf("   Auth Port  : %u\n", cfg->authserverPort);
    printf("   World Port : %u\n", cfg->worldserverPort);
    printf("   Database   : MySQL (%s:%d)\n", cfg->dbHost, cfg->dbPort);
    printf("   Scripts    : active (WSS word/statement)\n");
    printf("  ==========================================\n");
    printf("   Default account: test / testpassword\n");
    printf("\n");
    printf("   Client realmlist:\n");
    printf("   set realmlist 127.0.0.1:%u\n", cfg->worldserverPort);
    printf("\n");
    printf("   Press Ctrl+C to stop.\n");
    printf("  ===========================================\n\n");

    /* ── Main update loop (50ms tick = 20/sec) ─── */
    uint32_t tick = 0;
    time_t startTime = time(NULL);

    while (g_running) {
        uint32_t diffMs = 50;
        WorldServer_Update(ws, diffMs);

        /* Status every 60 seconds */
        if (tick % 1200 == 0 && tick > 0) {
            time_t now = time(NULL);
            int uptime = (int)(now - startTime);
            int h = uptime / 3600;
            int m = (uptime % 3600) / 60;
            int s = uptime % 60;
            printf("[Status] Tick %u | Uptime %02d:%02d:%02d | Online: 0\n",
                   tick, h, m, s);
        }

        tick++;
        Sleep(50);  /* Windows sleep in milliseconds */
    }

    /* ── Graceful shutdown ─────────────────────────── */
    printf("\n[Main] Saving data and shutting down...\n");
    WorldServer_Stop(ws);
    AuthServer_Stop(auth);
    WorldServer_Destroy(ws);
    AuthServer_Destroy(auth);
    database_free(db);
    Config_Free(cfg);
    WSACleanup();
    printf("[Main] Done. Goodbye!\n");
    return 0;
}