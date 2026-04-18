#ifndef SHARED_NETWORK_H
#define SHARED_NETWORK_H

#include <stdint.h>

int net_init(uint16_t port);
void net_poll(void);
void net_close(void);
void net_shutdown(void);

#endif
