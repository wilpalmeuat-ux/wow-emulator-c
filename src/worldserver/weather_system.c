/* weather_system.c -- Weather + transport stubs for WoW 3.3.5a emulator */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 *  Weather types
 * ================================================================ */
typedef enum {
    WEATHER_FINE     = 0,
    WEATHER_RAIN     = 1,
    WEATHER_SNOW     = 2,
    WEATHER_SANDSTORM= 3,
    WEATHER_THUNDERSTORM = 4
} WeatherType;

typedef struct ZoneWeather {
    uint32_t    zoneId;
    WeatherType type;
    float       intensity;   /* 0.0 - 1.0 */
    uint32_t    duration;    /* remaining seconds */
    uint32_t    changeTimer; /* seconds until weather changes */
} ZoneWeather;

#define MAX_ZONE_WEATHER 256
static ZoneWeather g_weather[MAX_ZONE_WEATHER];
static int g_weatherCount = 0;

void WeatherSystem_Init(void) {
    g_weatherCount = 0;
    memset(g_weather, 0, sizeof(g_weather));

    /* Default weather for some zones */
    g_weather[g_weatherCount++] = (ZoneWeather){ 1, WEATHER_FINE, 0, 0, 600 };    /* Dun Morogh */
    g_weather[g_weatherCount++] = (ZoneWeather){ 12, WEATHER_FINE, 0, 0, 600 };   /* Elwynn Forest */
    g_weather[g_weatherCount++] = (ZoneWeather){ 14, WEATHER_FINE, 0, 0, 600 };   /* Durotar */
    g_weather[g_weatherCount++] = (ZoneWeather){ 85, WEATHER_FINE, 0, 0, 600 };   /* Tirisfal */
    g_weather[g_weatherCount++] = (ZoneWeather){ 3537, WEATHER_FINE, 0, 0, 600 }; /* Borean Tundra */
    g_weather[g_weatherCount++] = (ZoneWeather){ 440, WEATHER_SANDSTORM, 0.3f, 300, 300 }; /* Tanaris */
    g_weather[g_weatherCount++] = (ZoneWeather){ 1377, WEATHER_SNOW, 0.4f, 600, 600 };     /* Silithus */

    printf("[Weather] Initialized weather for %d zones\n", g_weatherCount);
}

void WeatherSystem_Update(uint32_t diffMs) {
    for (int i = 0; i < g_weatherCount; i++) {
        ZoneWeather* w = &g_weather[i];
        uint32_t diffSec = diffMs / 1000;
        if (diffSec == 0) continue;

        if (w->duration > 0) {
            w->duration = (w->duration > diffSec) ? w->duration - diffSec : 0;
            if (w->duration == 0) {
                w->type = WEATHER_FINE;
                w->intensity = 0;
            }
        }

        if (w->changeTimer > 0) {
            w->changeTimer = (w->changeTimer > diffSec) ? w->changeTimer - diffSec : 0;
            if (w->changeTimer == 0) {
                /* Random weather change */
                int roll = rand() % 100;
                if (roll < 40) {
                    w->type = WEATHER_FINE;
                    w->intensity = 0;
                } else if (roll < 60) {
                    w->type = WEATHER_RAIN;
                    w->intensity = 0.3f + (float)(rand() % 70) / 100.0f;
                } else if (roll < 75) {
                    w->type = WEATHER_SNOW;
                    w->intensity = 0.2f + (float)(rand() % 80) / 100.0f;
                } else if (roll < 85) {
                    w->type = WEATHER_SANDSTORM;
                    w->intensity = 0.4f + (float)(rand() % 60) / 100.0f;
                } else {
                    w->type = WEATHER_THUNDERSTORM;
                    w->intensity = 0.6f + (float)(rand() % 40) / 100.0f;
                }
                w->duration = 120 + (uint32_t)(rand() % 600);
                w->changeTimer = 300 + (uint32_t)(rand() % 600);
            }
        }
    }
}

WeatherType WeatherSystem_GetWeather(uint32_t zoneId, float* intensity) {
    for (int i = 0; i < g_weatherCount; i++) {
        if (g_weather[i].zoneId == zoneId) {
            if (intensity) *intensity = g_weather[i].intensity;
            return g_weather[i].type;
        }
    }
    if (intensity) *intensity = 0;
    return WEATHER_FINE;
}

/* ================================================================
 *  Transport system (ships, zeppelins, elevators)
 * ================================================================ */
typedef enum { TRANSPORT_SHIP=0, TRANSPORT_ZEPPELIN=1, TRANSPORT_ELEVATOR=2 } TransportType;

typedef struct TransportWaypoint {
    float x, y, z, o;
    uint32_t mapId;
    uint32_t delay;  /* ms to wait at this point */
} TransportWaypoint;

#define MAX_TRANSPORT_WAYPOINTS 32

typedef struct Transport {
    uint32_t entry;
    char     name[64];
    TransportType type;
    TransportWaypoint waypoints[MAX_TRANSPORT_WAYPOINTS];
    int      waypointCount;
    int      currentWaypoint;
    float    speed;
    float    currentX, currentY, currentZ;
    uint32_t mapId;
    uint32_t timer;
    bool     active;
} Transport;

#define MAX_TRANSPORTS 32
static Transport g_transports[MAX_TRANSPORTS];
static int g_transportCount = 0;

void TransportSystem_Init(void) {
    g_transportCount = 0;
    memset(g_transports, 0, sizeof(g_transports));

    /* Register demo transports */
    Transport* t;

    /* Stormwind to Menethil Harbor boat */
    t = &g_transports[g_transportCount++];
    t->entry = 20808; strncpy(t->name, "Ship: Stormwind-Menethil", 63);
    t->type = TRANSPORT_SHIP; t->speed = 28.0f; t->active = true;
    t->waypoints[0] = (TransportWaypoint){ -8640.0f, 1330.0f, 5.0f, 0, 0, 60000 };
    t->waypoints[1] = (TransportWaypoint){ -3670.0f, -600.0f, 5.0f, 0, 0, 60000 };
    t->waypointCount = 2;

    /* Orgrimmar to Undercity zeppelin */
    t = &g_transports[g_transportCount++];
    t->entry = 20807; strncpy(t->name, "Zeppelin: Org-UC", 63);
    t->type = TRANSPORT_ZEPPELIN; t->speed = 32.0f; t->active = true;
    t->waypoints[0] = (TransportWaypoint){ 1676.0f, -4313.0f, 61.0f, 0, 1, 60000 };
    t->waypoints[1] = (TransportWaypoint){ 2066.0f, 288.0f, 97.0f, 0, 0, 60000 };
    t->waypointCount = 2;

    printf("[Transport] Initialized %d transports\n", g_transportCount);
}

void TransportSystem_Update(uint32_t diffMs) {
    for (int i = 0; i < g_transportCount; i++) {
        Transport* t = &g_transports[i];
        if (!t->active || t->waypointCount < 2) continue;

        t->timer += diffMs;
        TransportWaypoint* wp = &t->waypoints[t->currentWaypoint];
        if (t->timer >= wp->delay) {
            t->timer = 0;
            t->currentWaypoint = (t->currentWaypoint + 1) % t->waypointCount;
            TransportWaypoint* next = &t->waypoints[t->currentWaypoint];
            t->currentX = next->x;
            t->currentY = next->y;
            t->currentZ = next->z;
            t->mapId = next->mapId;
        }
    }
}
