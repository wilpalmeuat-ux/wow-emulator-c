/* dbc_loader.c -- DBC (DataBase Client) file loader for WoW 3.3.5a
 *
 * DBC files are the client-side data files that define spells, items,
 * maps, talents, etc. Format: header(20 bytes) + records + string_block.
 *
 * Header: magic(4) + recordCount(4) + fieldCount(4) + recordSize(4) + stringBlockSize(4)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define DBC_MAGIC 0x43424457  /* 'WDBC' */

typedef struct DBCHeader {
    uint32_t magic;
    uint32_t recordCount;
    uint32_t fieldCount;
    uint32_t recordSize;
    uint32_t stringBlockSize;
} DBCHeader;

typedef struct DBCFile {
    DBCHeader header;
    uint8_t*  records;       /* raw record data */
    char*     stringBlock;   /* string table */
    bool      loaded;
    char      filename[256];
} DBCFile;

/* ================================================================
 *  Load a DBC file from disk
 * ================================================================ */
bool DBC_Load(DBCFile* dbc, const char* path) {
    if (!dbc || !path) return false;
    memset(dbc, 0, sizeof(DBCFile));
    strncpy(dbc->filename, path, 255);

    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("[DBC] Cannot open: %s\n", path);
        return false;
    }

    /* Read header */
    if (fread(&dbc->header, sizeof(DBCHeader), 1, f) != 1) {
        printf("[DBC] Failed to read header: %s\n", path);
        fclose(f);
        return false;
    }

    /* Validate magic */
    if (dbc->header.magic != DBC_MAGIC) {
        printf("[DBC] Invalid magic in %s (expected WDBC, got 0x%08X)\n",
               path, dbc->header.magic);
        fclose(f);
        return false;
    }

    /* Read records */
    size_t recordsSize = (size_t)dbc->header.recordCount * dbc->header.recordSize;
    dbc->records = (uint8_t*)malloc(recordsSize);
    if (fread(dbc->records, 1, recordsSize, f) != recordsSize) {
        printf("[DBC] Failed to read records: %s\n", path);
        free(dbc->records);
        fclose(f);
        return false;
    }

    /* Read string block */
    if (dbc->header.stringBlockSize > 0) {
        dbc->stringBlock = (char*)malloc(dbc->header.stringBlockSize);
        if (fread(dbc->stringBlock, 1, dbc->header.stringBlockSize, f) != dbc->header.stringBlockSize) {
            printf("[DBC] Failed to read string block: %s\n", path);
            free(dbc->records);
            free(dbc->stringBlock);
            fclose(f);
            return false;
        }
    }

    fclose(f);
    dbc->loaded = true;
    printf("[DBC] Loaded %s: %u records, %u fields, %u bytes/record\n",
           path, dbc->header.recordCount, dbc->header.fieldCount, dbc->header.recordSize);
    return true;
}

void DBC_Free(DBCFile* dbc) {
    if (!dbc) return;
    free(dbc->records);
    free(dbc->stringBlock);
    memset(dbc, 0, sizeof(DBCFile));
}

/* Get a record pointer by index */
const uint8_t* DBC_GetRecord(const DBCFile* dbc, uint32_t index) {
    if (!dbc || !dbc->loaded || index >= dbc->header.recordCount) return NULL;
    return dbc->records + (size_t)index * dbc->header.recordSize;
}

/* Read a uint32 field from a record */
uint32_t DBC_GetUInt32(const DBCFile* dbc, uint32_t record, uint32_t field) {
    const uint8_t* rec = DBC_GetRecord(dbc, record);
    if (!rec || field >= dbc->header.fieldCount) return 0;
    uint32_t val;
    memcpy(&val, rec + field * 4, 4);
    return val;
}

/* Read a float field */
float DBC_GetFloat(const DBCFile* dbc, uint32_t record, uint32_t field) {
    const uint8_t* rec = DBC_GetRecord(dbc, record);
    if (!rec || field >= dbc->header.fieldCount) return 0.0f;
    float val;
    memcpy(&val, rec + field * 4, 4);
    return val;
}

/* Read a string field (offset into string block) */
const char* DBC_GetString(const DBCFile* dbc, uint32_t record, uint32_t field) {
    uint32_t offset = DBC_GetUInt32(dbc, record, field);
    if (!dbc->stringBlock || offset >= dbc->header.stringBlockSize) return "";
    return dbc->stringBlock + offset;
}

/* Find a record by its first field (ID) */
int DBC_FindById(const DBCFile* dbc, uint32_t id) {
    for (uint32_t i = 0; i < dbc->header.recordCount; i++) {
        if (DBC_GetUInt32(dbc, i, 0) == id) return (int)i;
    }
    return -1;
}

/* ================================================================
 *  Load all standard WoW DBC files from a directory
 * ================================================================ */
typedef struct DBCStore {
    DBCFile spellDbc;
    DBCFile talentDbc;
    DBCFile talentTabDbc;
    DBCFile chrRacesDbc;
    DBCFile chrClassesDbc;
    DBCFile mapDbc;
    DBCFile areaTriggerDbc;
    DBCFile areaTableDbc;
    DBCFile emotesDbc;
    DBCFile factionDbc;
    DBCFile factionTemplateDbc;
    DBCFile itemDisplayInfoDbc;
    DBCFile skillLineDbc;
    DBCFile skillLineAbilityDbc;
    DBCFile achievementDbc;
    DBCFile achievementCriteriaDbc;
    bool loaded;
} DBCStore;

static DBCStore g_dbcStore;

bool DBCStore_Load(const char* dataPath) {
    memset(&g_dbcStore, 0, sizeof(DBCStore));
    char path[512];
    int loaded = 0;

    /* Try loading each DBC file -- it's OK if some are missing */
    #define TRY_LOAD(field, name) do { \
        snprintf(path, sizeof(path), "%s/dbc/%s", dataPath, name); \
        if (DBC_Load(&g_dbcStore.field, path)) loaded++; \
    } while(0)

    TRY_LOAD(spellDbc, "Spell.dbc");
    TRY_LOAD(talentDbc, "Talent.dbc");
    TRY_LOAD(talentTabDbc, "TalentTab.dbc");
    TRY_LOAD(chrRacesDbc, "ChrRaces.dbc");
    TRY_LOAD(chrClassesDbc, "ChrClasses.dbc");
    TRY_LOAD(mapDbc, "Map.dbc");
    TRY_LOAD(areaTableDbc, "AreaTable.dbc");
    TRY_LOAD(factionDbc, "Faction.dbc");
    TRY_LOAD(factionTemplateDbc, "FactionTemplate.dbc");
    TRY_LOAD(skillLineDbc, "SkillLine.dbc");
    TRY_LOAD(achievementDbc, "Achievement.dbc");

    #undef TRY_LOAD

    g_dbcStore.loaded = (loaded > 0);
    printf("[DBC] Loaded %d DBC files from %s/dbc/\n", loaded, dataPath);
    return g_dbcStore.loaded;
}

void DBCStore_Free(void) {
    DBC_Free(&g_dbcStore.spellDbc);
    DBC_Free(&g_dbcStore.talentDbc);
    DBC_Free(&g_dbcStore.talentTabDbc);
    DBC_Free(&g_dbcStore.chrRacesDbc);
    DBC_Free(&g_dbcStore.chrClassesDbc);
    DBC_Free(&g_dbcStore.mapDbc);
    DBC_Free(&g_dbcStore.areaTableDbc);
    DBC_Free(&g_dbcStore.factionDbc);
    DBC_Free(&g_dbcStore.factionTemplateDbc);
    DBC_Free(&g_dbcStore.skillLineDbc);
    DBC_Free(&g_dbcStore.achievementDbc);
    g_dbcStore.loaded = false;
}

DBCStore* DBCStore_Get(void) { return &g_dbcStore; }
