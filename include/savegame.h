#pragma once
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include "common.h"
#include "player.h"
#include "world.h"

static const uint8_t MRE_MAGIC[4]   = {0x4E,0x5A,0x4D,0x52};
static const uint8_t RETJE_MAGIC[4] = {0x4E,0x5A,0x52,0x54};

static const int  MAX_REGISTRY_ENTRIES = MAX_SAVE_SLOTS + 16;
static const char SAVE_DIR_NAME[]      = "Null Zone";
static const char SAVES_SUBDIR[]       = "saves";

static char g_save_dir[MAX_PATH]      = {};
static char g_saves_dir[MAX_PATH]     = {};
static char g_registry_path[MAX_PATH] = {};

struct MREFile {
    uint8_t  magic[4];
    uint8_t  uuid[16];
    uint32_t headerCRC;
    uint32_t level;
    uint32_t health;
    uint32_t armor;
    uint32_t score;
    uint32_t kills;
    uint32_t timestamp;
    uint8_t  slotType;
    uint8_t  currentWeapon;
    uint8_t  pad[2];
    uint32_t weaponOwned;
    int32_t  weaponAmmo[MAX_WEAPONS];
    float    posX;
    float    posY;
    float    angle;
    uint32_t dataCRC;
};

struct RETJEHeader {
    uint8_t  magic[4];
    uint32_t version;
    uint32_t entryCount;
    uint32_t headerCRC;
};

struct RETJEEntry {
    uint8_t  uuid[16];
    uint8_t  slotType;
    uint8_t  pad[3];
    uint32_t timestamp;
    uint32_t fileCRC;
    uint32_t level;
    uint32_t score;
    char     filename[64];
};

static RETJEEntry g_registry[MAX_REGISTRY_ENTRIES];
static int        g_registryCount = 0;

static inline uint32_t mre_compute_data_crc(const MREFile& f) {
    const uint8_t* ptr = (const uint8_t*)&f;
    size_t sz = sizeof(MREFile) - sizeof(uint32_t);
    return crc32_compute(ptr, sz);
}

static inline uint32_t mre_compute_header_crc(const MREFile& f) {
    return crc32_compute(f.uuid, 16) ^ f.level ^ f.timestamp;
}

inline bool savegame_init_dirs() {
    char pf[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, pf)))
        GetEnvironmentVariableA("ProgramFiles", pf, MAX_PATH);

    SDL_snprintf(g_save_dir,  sizeof(g_save_dir),  "%s\\%s", pf, SAVE_DIR_NAME);
    SDL_snprintf(g_saves_dir, sizeof(g_saves_dir),  "%s\\%s", g_save_dir, SAVES_SUBDIR);
    SDL_snprintf(g_registry_path, sizeof(g_registry_path),
                 "%s\\registry.retje", g_saves_dir);

    CreateDirectoryA(g_save_dir,  nullptr);
    CreateDirectoryA(g_saves_dir, nullptr);
    return true;
}

static bool registry_load() {
    FILE* f = fopen(g_registry_path, "rb");
    if (!f) { g_registryCount=0; return false; }

    RETJEHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) { fclose(f); return false; }
    if (memcmp(hdr.magic, RETJE_MAGIC, 4) != 0) { fclose(f); return false; }

    uint32_t hdrCRC = crc32_compute((const uint8_t*)&hdr, sizeof(hdr)-sizeof(uint32_t));
    if (hdrCRC != hdr.headerCRC) { fclose(f); return false; }

    int cnt = (int)std::min((uint32_t)MAX_REGISTRY_ENTRIES, hdr.entryCount);
    g_registryCount = 0;
    for (int i=0;i<cnt;i++) {
        if (fread(&g_registry[i], sizeof(RETJEEntry), 1, f)==1)
            g_registryCount++;
    }
    fclose(f);
    return true;
}

static bool registry_save() {
    FILE* f = fopen(g_registry_path, "wb");
    if (!f) return false;

    RETJEHeader hdr;
    memcpy(hdr.magic, RETJE_MAGIC, 4);
    hdr.version    = 1;
    hdr.entryCount = (uint32_t)g_registryCount;
    hdr.headerCRC  = crc32_compute((const uint8_t*)&hdr, sizeof(hdr)-sizeof(uint32_t));

    fwrite(&hdr, sizeof(hdr), 1, f);
    for (int i=0;i<g_registryCount;i++)
        fwrite(&g_registry[i], sizeof(RETJEEntry), 1, f);
    fclose(f);
    return true;
}

static RETJEEntry* registry_find_uuid(const uint8_t uuid[16]) {
    for (int i=0;i<g_registryCount;i++)
        if (uuid_equal(g_registry[i].uuid, uuid))
            return &g_registry[i];
    return nullptr;
}

static RETJEEntry* registry_find_slot(uint8_t slotType) {
    for (int i=0;i<g_registryCount;i++)
        if (g_registry[i].slotType == slotType)
            return &g_registry[i];
    return nullptr;
}

inline bool savegame_load_registry() {
    registry_load();
    return true;
}

inline bool savegame_write(const Player& p, uint8_t slotType) {
    MREFile mre = {};
    memcpy(mre.magic, MRE_MAGIC, 4);

    uint32_t ts = (uint32_t)time(nullptr);
    uuid_generate(mre.uuid, ts ^ (uint32_t)slotType ^ (uint32_t)p.score);

    mre.level         = (uint32_t)p.level;
    mre.health        = (uint32_t)p.health;
    mre.armor         = (uint32_t)p.armor;
    mre.score         = (uint32_t)p.score;
    mre.kills         = (uint32_t)p.kills;
    mre.timestamp     = ts;
    mre.slotType      = slotType;
    mre.currentWeapon = (uint8_t)p.currentWeapon;
    mre.posX          = (float)p.pos.x;
    mre.posY          = (float)p.pos.y;
    mre.angle         = (float)p.angle;

    mre.weaponOwned = 0;
    for (int i=0;i<MAX_WEAPONS;i++) {
        if (p.weapons[i].owned) mre.weaponOwned |= (1u<<i);
        mre.weaponAmmo[i] = p.weapons[i].ammo;
    }

    mre.headerCRC = mre_compute_header_crc(mre);
    mre.dataCRC   = mre_compute_data_crc(mre);

    char filename[MAX_PATH];
    if (slotType == 0)
        SDL_snprintf(filename, sizeof(filename), "%s\\autosave.mre", g_saves_dir);
    else
        SDL_snprintf(filename, sizeof(filename), "%s\\slot_%d.mre", g_saves_dir, (int)slotType);

    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    fwrite(&mre, sizeof(mre), 1, f);
    fclose(f);

    uint32_t fileCRC = crc32_compute((const uint8_t*)&mre, sizeof(mre));

    RETJEEntry* existing = registry_find_slot(slotType);
    RETJEEntry* entry;
    if (existing) {
        entry = existing;
    } else {
        if (g_registryCount >= MAX_REGISTRY_ENTRIES) return false;
        entry = &g_registry[g_registryCount++];
    }

    memcpy(entry->uuid, mre.uuid, 16);
    entry->slotType  = slotType;
    entry->timestamp = ts;
    entry->fileCRC   = fileCRC;
    entry->level     = mre.level;
    entry->score     = mre.score;
    char shortname[64];
    if (slotType==0) SDL_snprintf(shortname,sizeof(shortname),"autosave.mre");
    else             SDL_snprintf(shortname,sizeof(shortname),"slot_%d.mre",(int)slotType);
    SDL_strlcpy(entry->filename, shortname, sizeof(entry->filename));

    return registry_save();
}

enum SaveLoadResult {
    SAVE_OK,
    SAVE_ERR_NOT_FOUND,
    SAVE_ERR_MAGIC,
    SAVE_ERR_UUID_MISMATCH,
    SAVE_ERR_CRC_HEADER,
    SAVE_ERR_CRC_DATA,
    SAVE_ERR_NO_REGISTRY
};

inline SaveLoadResult savegame_read(uint8_t slotType, Player& p) {
    RETJEEntry* entry = registry_find_slot(slotType);
    if (!entry) return SAVE_ERR_NO_REGISTRY;

    char filename[MAX_PATH];
    SDL_snprintf(filename, sizeof(filename), "%s\\%s", g_saves_dir, entry->filename);

    FILE* f = fopen(filename, "rb");
    if (!f) return SAVE_ERR_NOT_FOUND;

    MREFile mre = {};
    if (fread(&mre, sizeof(mre), 1, f) != 1) { fclose(f); return SAVE_ERR_NOT_FOUND; }
    fclose(f);

    if (memcmp(mre.magic, MRE_MAGIC, 4) != 0) return SAVE_ERR_MAGIC;

    uint32_t fileCRC = crc32_compute((const uint8_t*)&mre, sizeof(mre));
    if (fileCRC != entry->fileCRC) return SAVE_ERR_UUID_MISMATCH;

    if (!uuid_equal(mre.uuid, entry->uuid)) return SAVE_ERR_UUID_MISMATCH;

    uint32_t expectedHeaderCRC = mre_compute_header_crc(mre);
    if (mre.headerCRC != expectedHeaderCRC) return SAVE_ERR_CRC_HEADER;

    uint32_t expectedDataCRC = mre_compute_data_crc(mre);
    if (mre.dataCRC != expectedDataCRC) return SAVE_ERR_CRC_DATA;

    p.level         = (int)mre.level;
    p.health        = (int)mre.health;
    p.armor         = (int)mre.armor;
    p.score         = (int)mre.score;
    p.kills         = (int)mre.kills;
    p.currentWeapon = (int)mre.currentWeapon;
    p.pos.x         = (double)mre.posX;
    p.pos.y         = (double)mre.posY;
    p.angle         = (double)mre.angle;

    player_init_weapons(p);
    for (int i=0;i<MAX_WEAPONS;i++) {
        p.weapons[i].owned = (mre.weaponOwned & (1u<<i)) != 0;
        p.weapons[i].ammo  = mre.weaponAmmo[i];
    }

    return SAVE_OK;
}

inline bool savegame_delete(uint8_t slotType) {
    RETJEEntry* entry = registry_find_slot(slotType);
    if (!entry) return false;

    char filename[MAX_PATH];
    SDL_snprintf(filename, sizeof(filename), "%s\\%s", g_saves_dir, entry->filename);
    DeleteFileA(filename);

    int idx = (int)(entry - g_registry);
    for (int i=idx;i<g_registryCount-1;i++)
        g_registry[i] = g_registry[i+1];
    g_registryCount--;

    return registry_save();
}

inline bool savegame_slot_exists(uint8_t slotType) {
    return registry_find_slot(slotType) != nullptr;
}

inline const char* savegame_error_str(SaveLoadResult r) {
    switch(r){
    case SAVE_OK:               return "OK";
    case SAVE_ERR_NOT_FOUND:    return "Save file not found";
    case SAVE_ERR_MAGIC:        return "Invalid save file format";
    case SAVE_ERR_UUID_MISMATCH:return "Save file is corrupted or has been tampered with";
    case SAVE_ERR_CRC_HEADER:   return "Save file header is corrupted";
    case SAVE_ERR_CRC_DATA:     return "Save file data is corrupted";
    case SAVE_ERR_NO_REGISTRY:  return "Save not found in registry";
    default:                    return "Unknown error";
    }
}

inline void savegame_get_slot_info(uint8_t slotType, char* buf, int bufSize) {
    RETJEEntry* e = registry_find_slot(slotType);
    if (!e) { SDL_snprintf(buf, bufSize, "[ EMPTY ]"); return; }
    time_t ts = (time_t)e->timestamp;
    struct tm* tm2 = localtime(&ts);
    char timebuf[32];
    if (tm2) strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %H:%M", tm2);
    else     SDL_strlcpy(timebuf, "unknown", sizeof(timebuf));
    SDL_snprintf(buf, bufSize, "LVL %d  Score:%d  %s",
                 (int)e->level, (int)e->score, timebuf);
}

inline void savegame_autosave(const Player& p) {
    savegame_write(p, 0);
}
