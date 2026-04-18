#ifndef WOW_WORLDSERVER_H
#define WOW_WORLDSERVER_H
#include <stdint.h>
void worldserver_init(const char* host, uint16_t port);
void worldserver_run(void);
void worldserver_shutdown(void);
#endif
