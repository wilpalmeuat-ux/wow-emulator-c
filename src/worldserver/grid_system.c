/* grid_system.c -- Grid/visibility system for WoW 3.3.5a emulator
 * Divides the world into cells so clients only receive updates for nearby objects.
 */
#include "worldserver/world_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GRID_SIZE       533.33333f  /* One grid cell = 533.33 yards */
#define GRID_CELLS_X    64
#define GRID_CELLS_Y    64
#define VISIBILITY_RANGE 100.0f     /* yards -- max update distance */
#define MAX_OBJECTS_PER_CELL 256

typedef struct GridCell {
    uint64_t objectGuids[MAX_OBJECTS_PER_CELL];
    int      count;
} GridCell;

typedef struct GridMap {
    MapId    mapId;
    GridCell cells[GRID_CELLS_X][GRID_CELLS_Y];
    bool     active;
} GridMap;

#define MAX_GRID_MAPS 4
static GridMap g_gridMaps[MAX_GRID_MAPS];
static bool g_gridInit = false;

void GridSystem_Init(void) {
    memset(g_gridMaps, 0, sizeof(g_gridMaps));
    for (int i = 0; i < MAX_GRID_MAPS; i++) {
        g_gridMaps[i].mapId = (MapId)i;
        g_gridMaps[i].active = true;
    }
    g_gridInit = true;
    printf("[Grid] Initialized %d grid maps (%dx%d cells each)\n",
           MAX_GRID_MAPS, GRID_CELLS_X, GRID_CELLS_Y);
}

/* Convert world coordinates to grid cell */
static void _worldToCell(float x, float y, int* cx, int* cy) {
    /* WoW map center is at (0,0), coords range roughly -17066 to 17066 */
    float offset = 32.0f * GRID_SIZE; /* center offset */
    *cx = (int)((x + offset) / GRID_SIZE);
    *cy = (int)((y + offset) / GRID_SIZE);
    if (*cx < 0) *cx = 0; if (*cx >= GRID_CELLS_X) *cx = GRID_CELLS_X - 1;
    if (*cy < 0) *cy = 0; if (*cy >= GRID_CELLS_Y) *cy = GRID_CELLS_Y - 1;
}

GridMap* GridSystem_GetMap(MapId mapId) {
    if (mapId < MAX_GRID_MAPS) return &g_gridMaps[mapId];
    return NULL;
}

/* Add an object to the grid */
void GridSystem_AddObject(MapId mapId, uint64_t guid, float x, float y) {
    GridMap* gm = GridSystem_GetMap(mapId);
    if (!gm) return;
    int cx, cy;
    _worldToCell(x, y, &cx, &cy);
    GridCell* cell = &gm->cells[cx][cy];
    if (cell->count < MAX_OBJECTS_PER_CELL) {
        cell->objectGuids[cell->count++] = guid;
    }
}

/* Remove an object from the grid */
void GridSystem_RemoveObject(MapId mapId, uint64_t guid, float x, float y) {
    GridMap* gm = GridSystem_GetMap(mapId);
    if (!gm) return;
    int cx, cy;
    _worldToCell(x, y, &cx, &cy);
    GridCell* cell = &gm->cells[cx][cy];
    for (int i = 0; i < cell->count; i++) {
        if (cell->objectGuids[i] == guid) {
            cell->objectGuids[i] = cell->objectGuids[cell->count - 1];
            cell->count--;
            return;
        }
    }
}

/* Move an object between cells */
void GridSystem_MoveObject(MapId mapId, uint64_t guid,
                           float oldX, float oldY, float newX, float newY) {
    int ocx, ocy, ncx, ncy;
    _worldToCell(oldX, oldY, &ocx, &ocy);
    _worldToCell(newX, newY, &ncx, &ncy);
    if (ocx == ncx && ocy == ncy) return; /* same cell */
    GridSystem_RemoveObject(mapId, guid, oldX, oldY);
    GridSystem_AddObject(mapId, guid, newX, newY);
}

/* Get all objects visible from a position */
int GridSystem_GetNearbyObjects(MapId mapId, float x, float y,
                                uint64_t* outGuids, int maxOut) {
    GridMap* gm = GridSystem_GetMap(mapId);
    if (!gm) return 0;
    int cx, cy;
    _worldToCell(x, y, &cx, &cy);
    int count = 0;

    /* Check surrounding cells (3x3 area) */
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || nx >= GRID_CELLS_X || ny < 0 || ny >= GRID_CELLS_Y) continue;
            GridCell* cell = &gm->cells[nx][ny];
            for (int i = 0; i < cell->count && count < maxOut; i++) {
                outGuids[count++] = cell->objectGuids[i];
            }
        }
    }
    return count;
}

/* Check if two positions are within visibility range */
bool GridSystem_IsVisible(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2-x1, dy = y2-y1, dz = z2-z1;
    return (dx*dx + dy*dy + dz*dz) <= (VISIBILITY_RANGE * VISIBILITY_RANGE);
}
