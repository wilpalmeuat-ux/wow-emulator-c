#include "worldserver/WorldServer.h"
#include "scripting/wss_vm.h"
#include "scripting/wss_bindings.h"
#include "scripting/wss_parser.h"
#include "shared/log.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    printf("WoW WorldServer 3.3.5a - Build %s %s\n", __DATE__, __TIME__);
    srand((unsigned)time(NULL));

    WSSState* S = wss_new();
    wss_bind_wow(S);
    wss_bind_player(S);
    wss_bind_world(S);
    wss_bind_combat(S);
    wss_bind_inventory(S);
    wss_bind_spells(S);
    wss_bind_quests(S);
    wss_bind_chat(S);

    LOG_INFO("Loading scripts from scripts/");
    // Load and execute startup scripts
    const char* startup_scripts[] = {
        "scripts/core/events.wss",
        "scripts/core/commands.wss",
        "scripts/core/hooks.wss",
        NULL
    };
    for(int i=0; startup_scripts[i]; i++) {
        FILE* f = fopen(startup_scripts[i], "r");
        if(f) {
            fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
            char* buf = malloc(len+1); fread(buf,1,len,f); buf[len]=0; fclose(f);
            LOG_INFO("Executing script: %s", startup_scripts[i]);
            parse_script(buf, S);
            free(buf);
        }
    }

    LOG_INFO("Starting world server...");
    worldserver_init("0.0.0.0", 8085);

    // Example: run a scripted event loop from within the VM
    wss_call(S, "onServerStart", 12, NULL, 0);

    worldserver_run();
    worldserver_shutdown();

    wss_dump(S);
    wss_free(S);
    LOG_INFO("WorldServer shut down cleanly");
    return 0;
}
