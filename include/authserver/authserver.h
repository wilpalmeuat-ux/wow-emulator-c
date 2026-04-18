#ifndef AUTHSERVER_H
#define AUTHSERVER_H

#include <stdint.h>
#include <stdbool.h>

#define AUTH_SERVER_PORT 3724

typedef struct AuthServer {
    int server_fd;
    int running;
    void* db;
} AuthServer;

extern AuthServer g_authserver;

void authserver_init(const char* config_file);
void authserver_shutdown(void);
void authserver_run(void);
void authserver_handle_client(int fd);

#endif
