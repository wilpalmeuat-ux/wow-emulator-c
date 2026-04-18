/* auth_server.h — WoW 3.3.5 AuthServer minimal implementation */
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct AuthServer AuthServer;

AuthServer*  AuthServer_Create(void);
void         AuthServer_Destroy(AuthServer* a);
bool         AuthServer_Start(AuthServer* a, uint16_t port);
void         AuthServer_Stop(AuthServer* a);
void         AuthServer_Update(AuthServer* a);
const char*  AuthServer_LastError(AuthServer* a);
