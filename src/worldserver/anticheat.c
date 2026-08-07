/* anticheat.c -- Anti-cheat detection for WoW 3.3.5a emulator
 *
 * Detects speed hacks, teleport hacks, fly hacks, and packet flooding.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_SPEED_NORMAL      7.0f    /* run speed */
#define MAX_SPEED_MOUNTED    14.0f    /* 100% mount */
#define MAX_SPEED_FLYING     21.0f    /* 310% flying */
#define MAX_TELEPORT_DIST    100.0f   /* max instant move distance */
#define MAX_Z_CHANGE          50.0f   /* max vertical change per update */
#define FLOOD_THRESHOLD       200     /* max packets per second */
#define FLOOD_WINDOW          5000    /* ms window for flood detection */
#define SPEED_VIOLATIONS_MAX  5       /* violations before kick */
#define TELEPORT_VIOLATIONS_MAX 3

typedef enum {
    CHEAT_NONE      = 0,
    CHEAT_SPEED     = 1,
    CHEAT_TELEPORT  = 2,
    CHEAT_FLY       = 3,
    CHEAT_CLIMB     = 4,
    CHEAT_FLOOD     = 5,
    CHEAT_TIME_MANIP= 6
} CheatType;

typedef enum {
    AC_ACTION_LOG    = 0,   /* just log */
    AC_ACTION_WARN   = 1,   /* warn the player */
    AC_ACTION_KICK   = 2,   /* disconnect */
    AC_ACTION_BAN    = 3    /* ban account */
} ACAction;

typedef struct AntiCheatData {
    /* Position tracking */
    float    lastX, lastY, lastZ;
    uint32_t lastMoveTime;       /* client timestamp */
    uint32_t serverMoveTime;     /* server timestamp */

    /* Violation counters */
    uint32_t speedViolations;
    uint32_t teleportViolations;
    uint32_t flyViolations;
    uint32_t climbViolations;

    /* Flood detection */
    uint32_t packetCount;
    uint32_t packetWindowStart;  /* ms */

    /* Movement flags from last packet */
    uint32_t lastMoveFlags;
    bool     isMounted;
    bool     isFlying;
    bool     isSwimming;
    bool     isFalling;

    /* Timestamps */
    time_t   lastViolationTime;
    time_t   firstViolationTime;
    bool     initialized;
} AntiCheatData;

static bool g_anticheatEnabled = true;
static ACAction g_defaultAction = AC_ACTION_KICK;

void AntiCheat_Init(void) {
    g_anticheatEnabled = true;
    g_defaultAction = AC_ACTION_KICK;
    printf("[AntiCheat] System initialized (speed/teleport/fly/flood detection)\n");
}

void AntiCheat_InitPlayer(AntiCheatData* ac) {
    memset(ac, 0, sizeof(AntiCheatData));
}

void AntiCheat_Enable(bool enabled) {
    g_anticheatEnabled = enabled;
    printf("[AntiCheat] %s\n", enabled ? "Enabled" : "Disabled");
}

/* ================================================================
 *  Check a movement update for cheats
 *  Returns the detected cheat type (CHEAT_NONE if clean)
 * ================================================================ */
CheatType AntiCheat_CheckMovement(AntiCheatData* ac, float newX, float newY, float newZ,
                                   uint32_t moveFlags, uint32_t clientTime, uint32_t serverTime) {
    if (!g_anticheatEnabled || !ac) return CHEAT_NONE;

    if (!ac->initialized) {
        ac->lastX = newX; ac->lastY = newY; ac->lastZ = newZ;
        ac->lastMoveTime = clientTime;
        ac->serverMoveTime = serverTime;
        ac->initialized = true;
        return CHEAT_NONE;
    }

    /* Calculate distance moved */
    float dx = newX - ac->lastX;
    float dy = newY - ac->lastY;
    float dz = newZ - ac->lastZ;
    float dist2d = sqrtf(dx*dx + dy*dy);
    float dist3d = sqrtf(dx*dx + dy*dy + dz*dz);

    /* Time since last update (in seconds) */
    uint32_t timeDiff = serverTime - ac->serverMoveTime;
    if (timeDiff == 0) timeDiff = 1;
    float timeSec = (float)timeDiff / 1000.0f;
    if (timeSec <= 0.0f) timeSec = 0.05f;

    /* Calculated speed */
    float speed = dist2d / timeSec;

    /* Determine max allowed speed */
    float maxSpeed = MAX_SPEED_NORMAL;
    if (ac->isFlying) maxSpeed = MAX_SPEED_FLYING;
    else if (ac->isMounted) maxSpeed = MAX_SPEED_MOUNTED;
    maxSpeed *= 1.5f; /* tolerance margin */

    CheatType detected = CHEAT_NONE;

    /* Speed hack detection */
    if (speed > maxSpeed && dist2d > 5.0f) {
        ac->speedViolations++;
        if (ac->speedViolations >= SPEED_VIOLATIONS_MAX) {
            detected = CHEAT_SPEED;
            printf("[AntiCheat] SPEED HACK: speed=%.1f max=%.1f dist=%.1f time=%.2fs\n",
                   speed, maxSpeed, dist2d, timeSec);
        }
    } else {
        if (ac->speedViolations > 0) ac->speedViolations--;
    }

    /* Teleport hack detection */
    if (dist3d > MAX_TELEPORT_DIST && timeSec < 1.0f) {
        ac->teleportViolations++;
        if (ac->teleportViolations >= TELEPORT_VIOLATIONS_MAX) {
            detected = CHEAT_TELEPORT;
            printf("[AntiCheat] TELEPORT HACK: moved %.1f in %.2fs\n", dist3d, timeSec);
        }
    }

    /* Fly hack detection (moving upward without flying flag) */
    bool hasFlightFlag = (moveFlags & 0x02000000) != 0; /* MOVEFLAG_FLYING */
    if (dz > 5.0f && !hasFlightFlag && !ac->isFalling && !ac->isSwimming) {
        ac->flyViolations++;
        if (ac->flyViolations >= 10) {
            detected = CHEAT_FLY;
            printf("[AntiCheat] FLY HACK: dz=%.1f without flight flag\n", dz);
        }
    } else {
        if (ac->flyViolations > 0) ac->flyViolations--;
    }

    /* Wall climbing detection */
    if (fabsf(dz) > MAX_Z_CHANGE && dist2d < 2.0f && !hasFlightFlag) {
        ac->climbViolations++;
        if (ac->climbViolations >= 5) {
            detected = CHEAT_CLIMB;
        }
    }

    /* Update state */
    ac->lastX = newX; ac->lastY = newY; ac->lastZ = newZ;
    ac->lastMoveTime = clientTime;
    ac->serverMoveTime = serverTime;
    ac->lastMoveFlags = moveFlags;
    ac->isFlying = hasFlightFlag;

    if (detected != CHEAT_NONE) {
        ac->lastViolationTime = time(NULL);
        if (ac->firstViolationTime == 0) ac->firstViolationTime = ac->lastViolationTime;
    }

    return detected;
}

/* ================================================================
 *  Packet flood detection
 * ================================================================ */
CheatType AntiCheat_CheckFlood(AntiCheatData* ac, uint32_t currentTimeMs) {
    if (!g_anticheatEnabled || !ac) return CHEAT_NONE;

    if (currentTimeMs - ac->packetWindowStart > FLOOD_WINDOW) {
        ac->packetWindowStart = currentTimeMs;
        ac->packetCount = 0;
    }

    ac->packetCount++;

    if (ac->packetCount > FLOOD_THRESHOLD) {
        printf("[AntiCheat] PACKET FLOOD: %u packets in %ums\n",
               ac->packetCount, currentTimeMs - ac->packetWindowStart);
        return CHEAT_FLOOD;
    }

    return CHEAT_NONE;
}

/* ================================================================
 *  Get recommended action for a cheat detection
 * ================================================================ */
ACAction AntiCheat_GetAction(CheatType cheat) {
    switch (cheat) {
    case CHEAT_SPEED:     return AC_ACTION_KICK;
    case CHEAT_TELEPORT:  return AC_ACTION_KICK;
    case CHEAT_FLY:       return AC_ACTION_KICK;
    case CHEAT_CLIMB:     return AC_ACTION_WARN;
    case CHEAT_FLOOD:     return AC_ACTION_KICK;
    case CHEAT_TIME_MANIP:return AC_ACTION_BAN;
    default:              return AC_ACTION_LOG;
    }
}

const char* AntiCheat_CheatName(CheatType cheat) {
    switch (cheat) {
    case CHEAT_SPEED:      return "Speed Hack";
    case CHEAT_TELEPORT:   return "Teleport Hack";
    case CHEAT_FLY:        return "Fly Hack";
    case CHEAT_CLIMB:      return "Wall Climb";
    case CHEAT_FLOOD:      return "Packet Flood";
    case CHEAT_TIME_MANIP: return "Time Manipulation";
    default:               return "None";
    }
}
