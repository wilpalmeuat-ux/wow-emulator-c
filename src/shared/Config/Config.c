/* Config.c — Windows-compatible INI config reader (pure C) */
#include <shared/Config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char* _trim(char* s) {
    while (isspace((unsigned char)s[0])) s++;
    if (!s[0]) return s;
    char* e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    e[1] = '\0';
    return s;
}

static char g_path[512] = "configs/worldserver.conf";

Config* Config_Load(const char* path) {
    if (path) strncpy(g_path, path, sizeof(g_path) - 1);
    Config* cfg = (Config*)calloc(1, sizeof(Config));
    cfg->worldserverPort = 8085;
    strncpy(cfg->worldserverName, "WoW 3.3.5a Emulator", 127);
    cfg->maxPlayers = 5000;
    cfg->authserverPort = 3724;
    strncpy(cfg->authserverBind, "0.0.0.0", 63);
    strncpy(cfg->dbType, "mysql", 31);
    strncpy(cfg->dbHost, "127.0.0.1", 127);
    cfg->dbPort = 3306;
    strncpy(cfg->dbUser, "root", 63);
    strncpy(cfg->dbPassword, "", 127);
    strncpy(cfg->dbDatabase, "wow_emulator", 63);
    cfg->loadScripts = 1;
    strncpy(cfg->scriptPath, "scripts/", MAX_PATH_LEN - 1);

    FILE* f = fopen(g_path, "r");
    if (!f) {
        fprintf(stderr, "[Config] Could not open %s (will use defaults)\n", g_path);
        return cfg;
    }
    char line[MAX_LINE_LEN], section[64] = "";
    while (fgets(line, sizeof(line), f)) {
        char* p = _trim(line);
        if (p[0] == '#' || p[0] == ';' || !p[0]) continue;
        if (p[0] == '[') {
            char* close = strchr(p, ']');
            if (close) {
                *close = '\0';
                strncpy(section, _trim(p + 1), sizeof(section) - 1);
            }
            continue;
        }
        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = _trim(p);
        char* val = _trim(eq + 1);
        if (!section[0] || !strcmp(section, "worldserver")) {
            if (!strcmp(key, "Port")) cfg->worldserverPort = (uint16_t)atoi(val);
            else if (!strcmp(key, "WorldName")) { strncpy(cfg->worldserverName, val, 127); cfg->worldserverName[127] = '\0'; }
            else if (!strcmp(key, "MaxPlayers")) cfg->maxPlayers = atoi(val);
        } else if (!strcmp(section, "authserver")) {
            if (!strcmp(key, "Port")) cfg->authserverPort = (uint16_t)atoi(val);
            else if (!strcmp(key, "BindIP")) { strncpy(cfg->authserverBind, val, 63); cfg->authserverBind[63] = '\0'; }
        } else if (!strcmp(section, "database")) {
            if (!strcmp(key, "Type")) { strncpy(cfg->dbType, val, 31); cfg->dbType[31] = '\0'; }
            else if (!strcmp(key, "Host")) { strncpy(cfg->dbHost, val, 127); cfg->dbHost[127] = '\0'; }
            else if (!strcmp(key, "Port")) cfg->dbPort = atoi(val);
            else if (!strcmp(key, "User")) { strncpy(cfg->dbUser, val, 63); cfg->dbUser[63] = '\0'; }
            else if (!strcmp(key, "Password")) { strncpy(cfg->dbPassword, val, 127); cfg->dbPassword[127] = '\0'; }
            else if (!strcmp(key, "Database")) { strncpy(cfg->dbDatabase, val, 63); cfg->dbDatabase[63] = '\0'; }
        } else if (!strcmp(section, "scripting")) {
            if (!strcmp(key, "LoadScripts")) cfg->loadScripts = atoi(val);
            else if (!strcmp(key, "ScriptPath")) { strncpy(cfg->scriptPath, val, MAX_PATH_LEN - 1); cfg->scriptPath[MAX_PATH_LEN - 1] = '\0'; }
        }
    }
    fclose(f);
    return cfg;
}

void Config_Free(Config* cfg) { free(cfg); }