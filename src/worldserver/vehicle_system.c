/* vehicle_system.c -- Vehicle mechanics for WoW 3.3.5a (WotLK)
 * Handles multi-seat vehicles, turrets, and siege engines.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VEHICLES 500
#define MAX_VEHICLE_SEATS 8

typedef enum { SEAT_DRIVER=0, SEAT_PASSENGER=1, SEAT_TURRET=2, SEAT_CONTROL=3 } SeatType;

typedef struct VehicleSeat {
    SeatType type;
    uint64_t passengerGuid;
    float    attachOffsetX, attachOffsetY, attachOffsetZ;
    float    attachOrientation;
    bool     occupied;
    bool     usable;
    uint32_t flags;
} VehicleSeat;

typedef struct VehicleTemplate {
    uint32_t entry;
    char     name[64];
    uint32_t displayId;
    uint32_t health;
    float    speed;
    uint32_t spellId[4];    /* vehicle ability spells */
    int      seatCount;
    SeatType seatTypes[MAX_VEHICLE_SEATS];
} VehicleTemplate;

typedef struct Vehicle {
    uint64_t         guid;
    uint32_t         entry;
    VehicleSeat      seats[MAX_VEHICLE_SEATS];
    int              seatCount;
    float            x, y, z, o;
    MapId            mapId;
    uint32_t         health;
    uint32_t         maxHealth;
    bool             active;
    bool             destroyed;
} Vehicle;

static VehicleTemplate g_vehicleTemplates[64];
static int g_vehicleTemplateCount = 0;
static Vehicle g_vehicles[MAX_VEHICLES];
static int g_vehicleCount = 0;
static uint64_t g_nextVehicleGuid = 0x6000000000000000ULL;

void VehicleSystem_Init(void) {
    memset(g_vehicleTemplates, 0, sizeof(g_vehicleTemplates));
    memset(g_vehicles, 0, sizeof(g_vehicles));
    g_vehicleTemplateCount = 0;
    g_vehicleCount = 0;

    /* Demo vehicles */
    VehicleTemplate* v;

    v = &g_vehicleTemplates[g_vehicleTemplateCount++];
    v->entry = 28312; strncpy(v->name, "Wintergrasp Siege Engine", 63);
    v->health = 50000; v->speed = 5.0f; v->seatCount = 3;
    v->seatTypes[0] = SEAT_DRIVER; v->seatTypes[1] = SEAT_TURRET; v->seatTypes[2] = SEAT_PASSENGER;

    v = &g_vehicleTemplates[g_vehicleTemplateCount++];
    v->entry = 33060; strncpy(v->name, "Salvaged Demolisher", 63);
    v->health = 60000; v->speed = 4.0f; v->seatCount = 3;
    v->seatTypes[0] = SEAT_DRIVER; v->seatTypes[1] = SEAT_TURRET; v->seatTypes[2] = SEAT_PASSENGER;

    v = &g_vehicleTemplates[g_vehicleTemplateCount++];
    v->entry = 33109; strncpy(v->name, "Salvaged Siege Engine", 63);
    v->health = 100000; v->speed = 3.0f; v->seatCount = 4;
    v->seatTypes[0] = SEAT_DRIVER; v->seatTypes[1] = SEAT_TURRET;
    v->seatTypes[2] = SEAT_PASSENGER; v->seatTypes[3] = SEAT_PASSENGER;

    printf("[Vehicle] Registered %d vehicle templates\n", g_vehicleTemplateCount);
}

VehicleTemplate* VehicleSystem_GetTemplate(uint32_t entry) {
    for (int i = 0; i < g_vehicleTemplateCount; i++)
        if (g_vehicleTemplates[i].entry == entry) return &g_vehicleTemplates[i];
    return NULL;
}

Vehicle* VehicleSystem_Spawn(uint32_t entry, MapId mapId, float x, float y, float z, float o) {
    VehicleTemplate* tmpl = VehicleSystem_GetTemplate(entry);
    if (!tmpl || g_vehicleCount >= MAX_VEHICLES) return NULL;

    Vehicle* veh = &g_vehicles[g_vehicleCount++];
    memset(veh, 0, sizeof(Vehicle));
    veh->guid = g_nextVehicleGuid++;
    veh->entry = entry;
    veh->x = x; veh->y = y; veh->z = z; veh->o = o;
    veh->mapId = mapId;
    veh->health = tmpl->health;
    veh->maxHealth = tmpl->health;
    veh->seatCount = tmpl->seatCount;
    veh->active = true;

    for (int i = 0; i < tmpl->seatCount; i++) {
        veh->seats[i].type = tmpl->seatTypes[i];
        veh->seats[i].usable = true;
    }

    printf("[Vehicle] Spawned '%s' at (%.1f, %.1f, %.1f)\n", tmpl->name, x, y, z);
    return veh;
}

bool VehicleSystem_Enter(Vehicle* veh, uint64_t playerGuid, int seatIndex) {
    if (!veh || !veh->active || veh->destroyed) return false;
    if (seatIndex < 0 || seatIndex >= veh->seatCount) return false;
    if (veh->seats[seatIndex].occupied || !veh->seats[seatIndex].usable) return false;

    veh->seats[seatIndex].passengerGuid = playerGuid;
    veh->seats[seatIndex].occupied = true;
    printf("[Vehicle] Player %llu entered seat %d of vehicle %llu\n",
           (unsigned long long)playerGuid, seatIndex, (unsigned long long)veh->guid);
    return true;
}

void VehicleSystem_Exit(Vehicle* veh, uint64_t playerGuid) {
    if (!veh) return;
    for (int i = 0; i < veh->seatCount; i++) {
        if (veh->seats[i].passengerGuid == playerGuid) {
            veh->seats[i].passengerGuid = 0;
            veh->seats[i].occupied = false;
            printf("[Vehicle] Player %llu exited vehicle\n", (unsigned long long)playerGuid);
            return;
        }
    }
}

void VehicleSystem_Destroy(Vehicle* veh) {
    if (!veh) return;
    veh->destroyed = true;
    veh->health = 0;
    /* Eject all passengers */
    for (int i = 0; i < veh->seatCount; i++) {
        veh->seats[i].occupied = false;
        veh->seats[i].passengerGuid = 0;
    }
    printf("[Vehicle] Vehicle %llu destroyed\n", (unsigned long long)veh->guid);
}
