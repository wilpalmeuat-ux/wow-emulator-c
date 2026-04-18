#include "authserver/AuthServer.h"
#include "authserver/AuthClient.h"
#include "shared/log.h"
#include <stdio.h>
int main(int argc, char** argv) {
    (void)argc; (void)argv;
    printf("WoW AuthServer 3.3.5a - Build %s %s\n", __DATE__, __TIME__);
    authserver_init("0.0.0.0", 3724);
    authserver_run();
    authserver_shutdown();
    return 0;
}
