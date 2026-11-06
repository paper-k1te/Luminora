#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "localization.h"

#define MAX_SAVE_SLOTS 4
#define SAVE_MAGIC_V2 0x4C554D32 // "LUM2" (Luminora Version 2)
#define SAVE_MAGIC_LEGACY 0x54455232 // "TER2"

typedef struct {
    int slotIndex;
    bool exists;
    char worldName[64];
    int mode; // 0 = Survival, 1 = Creative
    int seed;
    long long lastSavedTimestamp;
    char dateStr[32];
    float playerHealth;
    float playerX, playerY;
    int modifiedCount;
} WorldSlotInfo;

#pragma pack(push, 1)
typedef struct {
    unsigned int magic;
    int version;
    char worldName[64];
    int mode;
    int seed;
    long long timestamp;
    float playerHealth;
    float playerMaxHealth;
    float playerX, playerY;
    float playerVx, playerVy;
    bool isFacingRight;
    float worldTime;
    int tutorialStep;
    int minedCount;
    int activeSlot;
    // 9 Hotbar slots + 27 Backpack slots
    struct { int type; int count; } hotbar[9];
    struct { int type; int count; } backpack[27];
    int modifiedBlockCount;
} SaveFileHeader;
#pragma pack(pop)

static int g_ActiveSaveSlot = 0;
static char g_CurrentWorldName[64] = "世界 1";

static inline void initSaveSystem(void) {
    CreateDirectoryA("saves", NULL);
}

static inline void getSlotFilePath(int slotIdx, char* outPath, size_t maxLen) {
    if (slotIdx < 0 || slotIdx >= MAX_SAVE_SLOTS) slotIdx = 0;
    snprintf(outPath, maxLen, "saves/world_slot_%d.bin", slotIdx);
}

static inline bool getSlotInfo(int slotIdx, WorldSlotInfo* info) {
    if (!info) return false;
    memset(info, 0, sizeof(WorldSlotInfo));
    info->slotIndex = slotIdx;

    char path[128];
    getSlotFilePath(slotIdx, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (!f) {
        // Fallback check legacy savegame.bin for slot 0
        if (slotIdx == 0) {
            FILE* lf = fopen("savegame.bin", "rb");
            if (lf) {
                fclose(lf);
                info->exists = true;
                snprintf(info->worldName, sizeof(info->worldName), "%s", (getLanguage() == LANG_ZH) ? "旧版存档" : "Legacy World");
                info->mode = 0;
                info->seed = 1337;
                info->playerHealth = 100.0f;
                snprintf(info->dateStr, sizeof(info->dateStr), "Legacy");
                return true;
            }
        }
        info->exists = false;
        return false;
    }

    SaveFileHeader hdr;
    if (fread(&hdr, sizeof(SaveFileHeader), 1, f) != 1 || (hdr.magic != SAVE_MAGIC_V2 && hdr.magic != SAVE_MAGIC_LEGACY)) {
        fclose(f);
        info->exists = false;
        return false;
    }
    fclose(f);

    info->exists = true;
    snprintf(info->worldName, sizeof(info->worldName), "%s", hdr.worldName);
    info->mode = hdr.mode;
    info->seed = hdr.seed;
    info->lastSavedTimestamp = hdr.timestamp;
    info->playerHealth = hdr.playerHealth;
    info->playerX = hdr.playerX;
    info->playerY = hdr.playerY;
    info->modifiedCount = hdr.modifiedBlockCount;

    // Format timestamp
    time_t rawtime = (time_t)hdr.timestamp;
    struct tm* timeinfo = localtime(&rawtime);
    if (timeinfo) {
        strftime(info->dateStr, sizeof(info->dateStr), "%Y-%m-%d %H:%M", timeinfo);
    } else {
        snprintf(info->dateStr, sizeof(info->dateStr), "2026-11-06");
    }

    return true;
}

static inline bool deleteWorldSlot(int slotIdx) {
    char path[128];
    getSlotFilePath(slotIdx, path, sizeof(path));
    BOOL res = DeleteFileA(path);
    if (slotIdx == 0) {
        DeleteFileA("savegame.bin");
    }
    return res != FALSE;
}

#endif // SAVE_SYSTEM_H
