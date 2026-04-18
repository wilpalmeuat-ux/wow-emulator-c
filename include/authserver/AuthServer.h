#ifndef WOW_AUTHSERVER_H
#define WOW_AUTHSERVER_H
#include <stdint.h>
void authserver_init(const char* host, uint16_t port);
void authserver_run(void);
void authserver_shutdown(void);
#endif
