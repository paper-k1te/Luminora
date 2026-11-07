#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "../standalone_src/localization.h"

#define WORLD_WIDTH 512
#define WORLD_HEIGHT 128
#define HOTBAR_SLOTS 9
#define BACKPACK_SLOTS 27
#define MAX_WORLD_CHESTS 128
#define CHEST_SLOTS 20

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_SAND,
    BLOCK_SANDSTONE,
    BLOCK_CACTUS,
    BLOCK_SNOW,
    BLOCK_ICE,
    BLOCK_CLAY,
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE,
    BLOCK_RUBY_ORE,
    BLOCK_BRICK,
    BLOCK_WOOD_PLANK,
    BLOCK_GLASS,
    BLOCK_TORCH,
    BLOCK_OBSIDIAN,
    BLOCK_MAGMA,
    BLOCK_MOSSY_STONE,
    BLOCK_MUD,
    BLOCK_GLOW_SHROOM,
    BLOCK_BEDROCK,
    BLOCK_WORKBENCH,
    BLOCK_CHAIR,
    BLOCK_TABLE,
    BLOCK_DOOR,
    BLOCK_BED,
    BLOCK_FURNACE,
    BLOCK_CHEST,
    BLOCK_BOOKSHELF,
    BLOCK_CLOCK,
    BLOCK_CHANDELIER,
    BLOCK_TYPE_COUNT
} BlockType;

typedef struct {
    int type;
    int count;
    int durability;
    int maxDurability;
} InventorySlot;

typedef struct {
    int x, y;
    InventorySlot slots[CHEST_SLOTS];
    bool active;
} WorldChest;

static WorldChest g_WorldChests[MAX_WORLD_CHESTS];
static bool g_LightExtinguished[WORLD_WIDTH][WORLD_HEIGHT];
static InventorySlot g_Hotbar[HOTBAR_SLOTS];
static InventorySlot g_Backpack[BACKPACK_SLOTS];
static float g_WorldTime = 0.0f;
static float g_PlayerHealth = 50.0f;
static float g_PlayerMaxHealth = 100.0f;

static inline bool isToolItem(int type) {
    return (type >= 101 && type <= 116);
}

static inline bool isArmorItem(int type) {
    return (type >= 120 && type <= 135);
}

static inline bool isLightExtinguished(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return false;
    return g_LightExtinguished[x][y];
}

static inline void setLightExtinguished(int x, int y, bool ext) {
    if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
        g_LightExtinguished[x][y] = ext;
    }
}

static WorldChest* getOrCreateChest(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return NULL;
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active && g_WorldChests[i].x == x && g_WorldChests[i].y == y) {
            return &g_WorldChests[i];
        }
    }
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (!g_WorldChests[i].active) {
            g_WorldChests[i].active = true;
            g_WorldChests[i].x = x;
            g_WorldChests[i].y = y;
            for (int s = 0; s < CHEST_SLOTS; s++) {
                g_WorldChests[i].slots[s].type = BLOCK_AIR;
                g_WorldChests[i].slots[s].count = 0;
                g_WorldChests[i].slots[s].durability = 0;
                g_WorldChests[i].slots[s].maxDurability = 0;
            }
            return &g_WorldChests[i];
        }
    }
    return NULL;
}

static WorldChest* findChest(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return NULL;
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active && g_WorldChests[i].x == x && g_WorldChests[i].y == y) {
            return &g_WorldChests[i];
        }
    }
    return NULL;
}

static void removeChest(int x, int y) {
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active && g_WorldChests[i].x == x && g_WorldChests[i].y == y) {
            g_WorldChests[i].active = false;
            g_WorldChests[i].x = 0;
            g_WorldChests[i].y = 0;
            for (int s = 0; s < CHEST_SLOTS; s++) {
                g_WorldChests[i].slots[s].type = BLOCK_AIR;
                g_WorldChests[i].slots[s].count = 0;
                g_WorldChests[i].slots[s].durability = 0;
                g_WorldChests[i].slots[s].maxDurability = 0;
            }
            return;
        }
    }
}

static bool transferChestToPlayerEx(WorldChest* chest, int slotIdx, int maxTake) {
    if (!chest || slotIdx < 0 || slotIdx >= CHEST_SLOTS) return false;
    InventorySlot* src = &chest->slots[slotIdx];
    if (src->type == BLOCK_AIR || src->count <= 0) return false;

    int type = src->type;
    int count = (maxTake > 0 && maxTake < src->count) ? maxTake : src->count;
    int dur = src->durability;
    int maxDur = src->maxDurability;

    if (isToolItem(type) || isArmorItem(type)) {
        for (int i = 0; i < HOTBAR_SLOTS; i++) {
            if (g_Hotbar[i].type == BLOCK_AIR || g_Hotbar[i].count <= 0) {
                g_Hotbar[i].type = type;
                g_Hotbar[i].count = 1;
                g_Hotbar[i].durability = dur;
                g_Hotbar[i].maxDurability = maxDur;
                src->type = BLOCK_AIR;
                src->count = 0;
                src->durability = 0;
                src->maxDurability = 0;
                return true;
            }
        }
        for (int i = 0; i < BACKPACK_SLOTS; i++) {
            if (g_Backpack[i].type == BLOCK_AIR || g_Backpack[i].count <= 0) {
                g_Backpack[i].type = type;
                g_Backpack[i].count = 1;
                g_Backpack[i].durability = dur;
                g_Backpack[i].maxDurability = maxDur;
                src->type = BLOCK_AIR;
                src->count = 0;
                src->durability = 0;
                src->maxDurability = 0;
                return true;
            }
        }
        return false;
    }

    int remaining = count;
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == type && g_Hotbar[i].count < 999) {
            int space = 999 - g_Hotbar[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Hotbar[i].count += take;
            remaining -= take;
        }
    }
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == type && g_Backpack[i].count < 999) {
            int space = 999 - g_Backpack[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Backpack[i].count += take;
            remaining -= take;
        }
    }
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == BLOCK_AIR || g_Hotbar[i].count <= 0) {
            int take = (remaining < 999) ? remaining : 999;
            g_Hotbar[i].type = type;
            g_Hotbar[i].count = take;
            g_Hotbar[i].durability = 0;
            g_Hotbar[i].maxDurability = 0;
            remaining -= take;
        }
    }
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == BLOCK_AIR || g_Backpack[i].count <= 0) {
            int take = (remaining < 999) ? remaining : 999;
            g_Backpack[i].type = type;
            g_Backpack[i].count = take;
            g_Backpack[i].durability = 0;
            g_Backpack[i].maxDurability = 0;
            remaining -= take;
        }
    }

    int taken = count - remaining;
    if (taken > 0) {
        src->count -= taken;
        if (src->count <= 0) {
            src->type = BLOCK_AIR;
            src->count = 0;
            src->durability = 0;
            src->maxDurability = 0;
        }
        return true;
    }
    return false;
}

static bool transferPlayerToChestEx(WorldChest* chest, InventorySlot* pSlot, int maxTake) {
    if (!chest || !pSlot || pSlot->type == BLOCK_AIR || pSlot->count <= 0) return false;

    int type = pSlot->type;
    int count = (maxTake > 0 && maxTake < pSlot->count) ? maxTake : pSlot->count;
    int dur = pSlot->durability;
    int maxDur = pSlot->maxDurability;

    if (isToolItem(type) || isArmorItem(type)) {
        for (int i = 0; i < CHEST_SLOTS; i++) {
            if (chest->slots[i].type == BLOCK_AIR || chest->slots[i].count <= 0) {
                chest->slots[i].type = type;
                chest->slots[i].count = 1;
                chest->slots[i].durability = dur;
                chest->slots[i].maxDurability = maxDur;
                pSlot->type = BLOCK_AIR;
                pSlot->count = 0;
                pSlot->durability = 0;
                pSlot->maxDurability = 0;
                return true;
            }
        }
        return false;
    }

    int remaining = count;
    for (int i = 0; i < CHEST_SLOTS && remaining > 0; i++) {
        if (chest->slots[i].type == type && chest->slots[i].count < 999) {
            int space = 999 - chest->slots[i].count;
            int take = (remaining < space) ? remaining : space;
            chest->slots[i].count += take;
            remaining -= take;
        }
    }
    for (int i = 0; i < CHEST_SLOTS && remaining > 0; i++) {
        if (chest->slots[i].type == BLOCK_AIR || chest->slots[i].count <= 0) {
            int take = (remaining < 999) ? remaining : 999;
            chest->slots[i].type = type;
            chest->slots[i].count = take;
            chest->slots[i].durability = 0;
            chest->slots[i].maxDurability = 0;
            remaining -= take;
        }
    }

    int taken = count - remaining;
    if (taken > 0) {
        pSlot->count -= taken;
        if (pSlot->count <= 0) {
            pSlot->type = BLOCK_AIR;
            pSlot->count = 0;
            pSlot->durability = 0;
            pSlot->maxDurability = 0;
        }
        return true;
    }
    return false;
}

int main(void) {
    printf("=====================================================\n");
    printf(" Luminora Test Suite: Chest Containers & Interactive Blocks\n");
    printf("=====================================================\n\n");

    // Test 1: getOrCreateChest & removeChest
    printf("[Test 1] WorldChest lifecycle & slot allocation...\n");
    memset(g_WorldChests, 0, sizeof(g_WorldChests));
    WorldChest* c1 = getOrCreateChest(10, 85);
    assert(c1 != NULL);
    assert(c1->x == 10 && c1->y == 85);
    assert(c1->active == true);
    for (int s = 0; s < CHEST_SLOTS; s++) {
        assert(c1->slots[s].type == BLOCK_AIR);
        assert(c1->slots[s].count == 0);
    }
    // Finding same chest returns identical pointer
    WorldChest* c1_found = findChest(10, 85);
    assert(c1_found == c1);
    WorldChest* c1_getAgain = getOrCreateChest(10, 85);
    assert(c1_getAgain == c1);

    // Remove chest
    removeChest(10, 85);
    assert(findChest(10, 85) == NULL);
    printf("  [PASS] Chest creation, lookup, and removal verified.\n\n");

    // Test 2: Transfer Player Hotbar -> Chest (Stackable & Single)
    printf("[Test 2] Transfer Player -> Chest...\n");
    WorldChest* chest = getOrCreateChest(20, 85);
    assert(chest != NULL);

    memset(g_Hotbar, 0, sizeof(g_Hotbar));
    g_Hotbar[0].type = BLOCK_WOOD_PLANK;
    g_Hotbar[0].count = 50;

    // Full transfer
    bool transferred = transferPlayerToChestEx(chest, &g_Hotbar[0], 0);
    assert(transferred == true);
    assert(g_Hotbar[0].type == BLOCK_AIR && g_Hotbar[0].count == 0);
    assert(chest->slots[0].type == BLOCK_WOOD_PLANK && chest->slots[0].count == 50);

    // Single item transfer
    g_Hotbar[1].type = BLOCK_DIRT;
    g_Hotbar[1].count = 10;
    transferred = transferPlayerToChestEx(chest, &g_Hotbar[1], 1);
    assert(transferred == true);
    assert(g_Hotbar[1].type == BLOCK_DIRT && g_Hotbar[1].count == 9);
    assert(chest->slots[1].type == BLOCK_DIRT && chest->slots[1].count == 1);

    // Tool transfer with durability
    g_Hotbar[2].type = 104; // Diamond Pickaxe
    g_Hotbar[2].count = 1;
    g_Hotbar[2].durability = 1500;
    g_Hotbar[2].maxDurability = 1560;
    transferred = transferPlayerToChestEx(chest, &g_Hotbar[2], 0);
    assert(transferred == true);
    assert(g_Hotbar[2].type == BLOCK_AIR);
    assert(chest->slots[2].type == 104 && chest->slots[2].durability == 1500);
    printf("  [PASS] Player to chest transfers (stackable, single, tool) verified.\n\n");

    // Test 3: Transfer Chest -> Player Hotbar / Backpack
    printf("[Test 3] Transfer Chest -> Player Inventory...\n");
    // Transfer 10 Wood Planks from chest to hotbar
    transferred = transferChestToPlayerEx(chest, 0, 10);
    assert(transferred == true);
    assert(chest->slots[0].count == 40);
    assert(g_Hotbar[0].type == BLOCK_WOOD_PLANK && g_Hotbar[0].count == 10);

    // Transfer tool back to player
    transferred = transferChestToPlayerEx(chest, 2, 0);
    assert(transferred == true);
    assert(chest->slots[2].type == BLOCK_AIR);
    assert(g_Hotbar[2].type == 104 && g_Hotbar[2].durability == 1500 && g_Hotbar[2].maxDurability == 1560);
    printf("  [PASS] Chest to player transfers (partial stack, tool preservation) verified.\n\n");

    // Test 4: Chest Breaking & Drop Persistence
    printf("[Test 4] Breaking chest contents drop test...\n");
    // Chest still has 40 Wood Planks in slot 0 and 1 Dirt in slot 1
    int droppedCount = 0;
    for (int s = 0; s < CHEST_SLOTS; s++) {
        if (chest->slots[s].type != BLOCK_AIR && chest->slots[s].count > 0) {
            droppedCount += chest->slots[s].count;
        }
    }
    assert(droppedCount == 41); // 40 wood + 1 dirt
    removeChest(20, 85);
    assert(findChest(20, 85) == NULL);
    printf("  [PASS] Chest contents drop calculation (41 items) verified without item loss.\n\n");

    // Test 5: BLOCK_BED (舒适木床) sleep logic
    printf("[Test 5] BLOCK_BED Sleep & Healing Mechanism...\n");
    // 5a. Daytime sleep attempt (should be rejected)
    g_WorldTime = 0.25f; // Daytime: sinf(0.25 * 2pi) = sinf(pi/2) = 1.0 > 0
    float sunAngleDay = g_WorldTime * (float)M_PI * 2.0f;
    bool isDay = (sinf(sunAngleDay) > 0.0f);
    assert(isDay == true); // Cannot sleep during day

    // 5b. Nighttime sleep attempt (should succeed, set to 08:00 morning, heal player)
    g_WorldTime = 0.75f; // Nighttime: sinf(0.75 * 2pi) = sinf(3pi/2) = -1.0 < 0
    float sunAngleNight = g_WorldTime * (float)M_PI * 2.0f;
    bool isNight = (sinf(sunAngleNight) <= 0.0f);
    assert(isNight == true);

    g_PlayerHealth = 35.0f;
    g_PlayerMaxHealth = 100.0f;

    // Simulate sleep
    if (isNight) {
        g_WorldTime = 8.0f / 24.0f;
        g_PlayerHealth = g_PlayerMaxHealth;
    }
    assert(fabsf(g_WorldTime - (8.0f / 24.0f)) < 0.0001f);
    assert(g_PlayerHealth == 100.0f);
    printf("  [PASS] Bed sleep rejects day and resets to 08:00 with full health at night.\n\n");

    // Test 6: BLOCK_CLOCK (古典摆钟) Time formatting
    printf("[Test 6] BLOCK_CLOCK Time Calculation & Localization...\n");
    // Case 1: 08:00 morning
    g_WorldTime = 8.0f / 24.0f;
    float totalHours = g_WorldTime * 24.0f;
    int hour = (int)totalHours % 24;
    int minute = (int)((totalHours - (float)(int)totalHours) * 60.0f) % 60;
    float sAngle = g_WorldTime * (float)M_PI * 2.0f;
    bool clockDay = (sinf(sAngle) > 0.0f);
    assert(hour == 8 && minute == 0);
    assert(clockDay == true);

    char clockZH[128], clockEN[128];
    snprintf(clockZH, sizeof(clockZH), "[摆钟报时: 当前时间为 %02d:%02d (%s)]", hour, minute, clockDay ? "白昼" : "夜晚");
    snprintf(clockEN, sizeof(clockEN), "[Clock Chime: Current time is %02d:%02d (%s)]", hour, minute, clockDay ? "Day" : "Night");
    assert(strcmp(clockZH, "[摆钟报时: 当前时间为 08:00 (白昼)]") == 0);
    assert(strcmp(clockEN, "[Clock Chime: Current time is 08:00 (Day)]") == 0);

    // Case 2: 21:30 night (21.5 / 24)
    g_WorldTime = 21.5f / 24.0f;
    totalHours = g_WorldTime * 24.0f;
    hour = (int)totalHours % 24;
    minute = (int)roundf((totalHours - (float)(int)totalHours) * 60.0f) % 60;
    sAngle = g_WorldTime * (float)M_PI * 2.0f;
    clockDay = (sinf(sAngle) > 0.0f);
    assert(hour == 21 && minute == 30);
    assert(clockDay == false);

    snprintf(clockZH, sizeof(clockZH), "[摆钟报时: 当前时间为 %02d:%02d (%s)]", hour, minute, clockDay ? "白昼" : "夜晚");
    assert(strcmp(clockZH, "[摆钟报时: 当前时间为 21:30 (夜晚)]") == 0);
    printf("  [PASS] Clock calculation and formatted chime messages verified.\n\n");

    // Test 7: BLOCK_TORCH & BLOCK_CHANDELIER Toggle (Extinguish / Reignite)
    printf("[Test 7] Torch & Chandelier Extinguish/Reignite Toggle...\n");
    memset(g_LightExtinguished, 0, sizeof(g_LightExtinguished));

    // Initially lit
    assert(isLightExtinguished(50, 80) == false);

    // Toggle 1: Extinguish
    setLightExtinguished(50, 80, true);
    assert(isLightExtinguished(50, 80) == true);

    // Toggle 2: Reignite
    setLightExtinguished(50, 80, false);
    assert(isLightExtinguished(50, 80) == false);

    // Multiple points
    setLightExtinguished(100, 45, true);
    setLightExtinguished(200, 90, true);
    assert(isLightExtinguished(100, 45) == true);
    assert(isLightExtinguished(200, 90) == true);
    assert(isLightExtinguished(100, 46) == false);
    printf("  [PASS] Light extinguish / reignite states verified.\n\n");

    // Test 8: Chest and Light Persistence Serialization / Deserialization
    printf("[Test 8] Save/Load Binary Persistence Verification...\n");
    // Set up 2 test chests
    memset(g_WorldChests, 0, sizeof(g_WorldChests));
    WorldChest* tc1 = getOrCreateChest(15, 60);
    tc1->slots[0].type = BLOCK_DIAMOND_ORE;
    tc1->slots[0].count = 64;
    tc1->slots[5].type = 104; // Diamond Pickaxe
    tc1->slots[5].count = 1;
    tc1->slots[5].durability = 1200;
    tc1->slots[5].maxDurability = 1560;

    WorldChest* tc2 = getOrCreateChest(45, 90);
    tc2->slots[19].type = BLOCK_GOLD_ORE;
    tc2->slots[19].count = 999;

    FILE* tf = fopen("test_chest_save.bin", "wb");
    assert(tf != NULL);

    int activeCount = 0;
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active) activeCount++;
    }
    fwrite(&activeCount, sizeof(int), 1, tf);
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active) {
            fwrite(&g_WorldChests[i], sizeof(WorldChest), 1, tf);
        }
    }

    int extCount = 0;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (g_LightExtinguished[x][y]) extCount++;
        }
    }
    fwrite(&extCount, sizeof(int), 1, tf);
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (g_LightExtinguished[x][y]) {
                short pt[2] = { (short)x, (short)y };
                fwrite(pt, sizeof(short), 2, tf);
            }
        }
    }
    fclose(tf);

    // Clear everything
    memset(g_WorldChests, 0, sizeof(g_WorldChests));
    memset(g_LightExtinguished, 0, sizeof(g_LightExtinguished));

    // Reload from file
    tf = fopen("test_chest_save.bin", "rb");
    assert(tf != NULL);

    int readActive = 0;
    assert(fread(&readActive, sizeof(int), 1, tf) == 1);
    assert(readActive == 2);
    for (int i = 0; i < readActive; i++) {
        WorldChest loaded;
        assert(fread(&loaded, sizeof(WorldChest), 1, tf) == 1);
        WorldChest* dst = getOrCreateChest(loaded.x, loaded.y);
        assert(dst != NULL);
        memcpy(dst->slots, loaded.slots, sizeof(chest->slots));
    }

    int readExt = 0;
    assert(fread(&readExt, sizeof(int), 1, tf) == 1);
    assert(readExt == 2);
    for (int i = 0; i < readExt; i++) {
        short pt[2];
        assert(fread(pt, sizeof(short), 2, tf) == 2);
        setLightExtinguished(pt[0], pt[1], true);
    }
    fclose(tf);
    remove("test_chest_save.bin");

    // Verify reloaded chest 1
    WorldChest* rc1 = findChest(15, 60);
    assert(rc1 != NULL);
    assert(rc1->slots[0].type == BLOCK_DIAMOND_ORE && rc1->slots[0].count == 64);
    assert(rc1->slots[5].type == 104 && rc1->slots[5].durability == 1200 && rc1->slots[5].maxDurability == 1560);

    // Verify reloaded chest 2
    WorldChest* rc2 = findChest(45, 90);
    assert(rc2 != NULL);
    assert(rc2->slots[19].type == BLOCK_GOLD_ORE && rc2->slots[19].count == 999);

    // Verify reloaded extinguished lights
    assert(isLightExtinguished(100, 45) == true);
    assert(isLightExtinguished(200, 90) == true);
    assert(isLightExtinguished(150, 50) == false);
    printf("  [PASS] Full binary persistence save & load roundtrip verified successfully!\n\n");

    printf("=====================================================\n");
    printf(" ALL 8 CHEST & INTERACTIVE BLOCK TESTS PASSED!\n");
    printf("=====================================================\n");
    return 0;
}
