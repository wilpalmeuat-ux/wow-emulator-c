/* Config.h — Windows-compatible INI config reader */
#pragma once
#include <stdint.h>

typedef struct Config Config;

struct Config {
    uint16_t worldserverPort;
    char     worldserverName[64];
    int      maxPlayers;
    uint16_t authserverPort;
    char     authserverBind[32];
    char     dbType[16];
    char     dbHost[64];
    int      dbPort;
    char     dbUser[32];
    char     dbPassword[64];
    char     dbDatabase[64];
    int      loadScripts;
    char     scriptPath[128];
};

/* Load config from file. Pass NULL for default path "configs\worldserver.conf" */
Config* Config_Load(const char* path);

/* Free config */
void Config_Free(Config* cfg);

/* Get singleton */
Config* Config_Get(void);
