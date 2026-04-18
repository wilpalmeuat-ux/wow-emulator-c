#pragma once

#include <stdint.h>

#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 512

typedef struct {
    uint16_t worldserverPort;
    char     worldserverName[128];
    uint32_t maxPlayers;
    uint16_t authserverPort;
    char     authserverBind[64];
    char     dbType[32];
    char     dbHost[128];
    uint16_t dbPort;
    char     dbUser[64];
    char     dbPassword[128];
    char     dbDatabase[64];
    int      loadScripts;
    char     scriptPath[MAX_PATH_LEN];
} Config;

extern Config* Config_Load(const char* path);
extern void    Config_Free(Config* cfg);