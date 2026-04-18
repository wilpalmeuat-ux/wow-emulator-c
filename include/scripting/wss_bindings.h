#ifndef WSS_BINDINGS_H
#define WSS_BINDINGS_H
#include "wss_vm.h"
#include "worldserver/WorldPlayer.h"
void wss_bind_wow(WSSState* S);
void wss_bind_player(WSSState* S);
void wss_bind_world(WSSState* S);
void wss_bind_combat(WSSState* S);
void wss_bind_inventory(WSSState* S);
void wss_bind_spells(WSSState* S);
void wss_bind_quests(WSSState* S);
void wss_bind_chat(WSSState* S);
#endif
