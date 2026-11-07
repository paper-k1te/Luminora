#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "texture_atlas.h"

#include "sound_engine.h"

#include "font_render.h"
#include "localization.h"
#include "save_system.h"

// Mob System Forward Declarations
void initMobs(void);
void spawnMob(float x, float y, int type);
void updateMobs(float dt, float playerX, float playerY);
void renderMobs(float daylight, float camX, float camY);
bool checkMobAttack(float clickWorldX, float clickWorldY, float damage, float playerX, float playerY);
bool checkMobAttackEx(float clickWorldX, float clickWorldY, float damage, float knockbackMult, float playerX, float playerY);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Constants & World Configuration
// ============================================================================
#define DEFAULT_WINDOW_WIDTH  1280
#define DEFAULT_WINDOW_HEIGHT 720

static int g_WindowWidth = DEFAULT_WINDOW_WIDTH;
static int g_WindowHeight = DEFAULT_WINDOW_HEIGHT;
static bool g_IsFullscreen = false;
static WINDOWPLACEMENT g_PrevPlacement;
static HWND g_MainHwnd = NULL;
static int g_ResPresetIndex = 0; // 0: 1280x720, 1: 1600x900, 2: 1920x1080, 3: Fullscreen

static inline int getWindowWidth(void) { return g_WindowWidth; }
static inline int getWindowHeight(void) { return g_WindowHeight; }
static inline bool isFullscreen(void) { return g_IsFullscreen; }
static inline int getResPresetIndex(void) { return g_ResPresetIndex; }

#define WINDOW_WIDTH  (getWindowWidth())
#define WINDOW_HEIGHT (getWindowHeight())
#define WORLD_WIDTH   512
#define WORLD_HEIGHT  128

#define BEDROCK_LEVEL    2
#define UNDERWORLD_LEVEL 24
#define CAVERN_LEVEL     55
#define SUBSURFACE_LEVEL 75
#define BASE_SURFACE_Y   85

#define BLOCK_DEPTH      0.45f
#define WALL_Z          -0.75f
#define REACH_DISTANCE   6.5f
#define MAX_DROPPED_ITEMS 256
#define MAX_PARTICLES    512
#define MAX_FLOATING_TEXTS 32
#define MAX_MODIFIED_BLOCKS 65536
#define HOTBAR_SLOTS     9
#define BACKPACK_SLOTS   27

// ============================================================================
// Block Types & Definition
// ============================================================================
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
    BLOCK_DOOR_UPPER,
    BLOCK_DOOR_OPEN_LOWER,
    BLOCK_DOOR_OPEN_UPPER,
    BLOCK_TYPE_COUNT
} BlockType;

typedef struct {
    const char* name;
    const char* chineseName;
    const char* dropName;
    float r, g, b;
    float hardness;
    bool isSolid;
    bool isTransparent;
    bool emitLight;
    float lightR, lightG, lightB;
} BlockDef;

static BlockDef g_BlockDefs[BLOCK_TYPE_COUNT] = {
    { "Air",            "空气",       "无",         0.0f, 0.0f, 0.0f,  0.0f, false, true,  false, 0,0,0 },
    { "Grass Block",    "草方块",     "泥土",       0.26f, 0.68f, 0.22f, 0.60f, true,  false, false, 0,0,0 },
    { "Dirt Block",     "泥土",       "泥土",       0.52f, 0.35f, 0.22f, 0.60f, true,  false, false, 0,0,0 },
    { "Stone Block",    "石块",       "原石",       0.50f, 0.52f, 0.54f, 4.5f,  true,  false, false, 0,0,0 },
    { "Wood Log",       "原木",       "原木",       0.58f, 0.38f, 0.20f, 2.0f,  false, true,  false, 0,0,0 },
    { "Leaves",         "树叶",       "树叶",       0.18f, 0.55f, 0.20f, 0.30f, true,  true,  false, 0,0,0 },
    { "Sand Block",     "沙块",       "沙块",       0.88f, 0.78f, 0.52f, 0.60f, true,  false, false, 0,0,0 },
    { "Sandstone",      "砂岩",       "砂岩",       0.75f, 0.64f, 0.42f, 4.5f,  true,  false, false, 0,0,0 },
    { "Cactus",         "仙人掌",     "仙人掌",     0.22f, 0.62f, 0.28f, 0.60f, true,  false, false, 0,0,0 },
    { "Snow Block",     "雪块",       "雪球",       0.92f, 0.95f, 0.98f, 0.60f, true,  false, false, 0,0,0 },
    { "Ice Block",      "坚冰",       "冰块",       0.65f, 0.85f, 0.95f, 0.80f, true,  true,  false, 0,0,0 },
    { "Clay Block",     "粘土",       "粘土块",     0.68f, 0.52f, 0.45f, 0.80f, true,  false, false, 0,0,0 },
    { "Coal Ore",       "煤矿石",     "煤炭",       0.25f, 0.25f, 0.27f, 6.0f,  true,  false, false, 0,0,0 },
    { "Iron Ore",       "铁矿石",     "铁矿",       0.72f, 0.58f, 0.50f, 6.0f,  true,  false, false, 0,0,0 },
    { "Gold Ore",       "金矿石",     "金矿",       0.95f, 0.82f, 0.22f, 6.0f,  true,  false, false, 0,0,0 },
    { "Diamond Ore",    "钻石矿石",   "璀璨钻石",   0.28f, 0.88f, 0.95f, 6.0f,  true,  false, false, 0,0,0 },
    { "Ruby Ore",       "红宝石矿",   "红宝石",     0.92f, 0.18f, 0.28f, 6.0f,  true,  false, false, 0,0,0 },
    { "Stone Brick",    "石砖",       "石砖",       0.72f, 0.38f, 0.30f, 4.5f,  true,  false, false, 0,0,0 },
    { "Wood Plank",     "木板",       "木板",       0.68f, 0.48f, 0.28f, 2.0f,  true,  false, false, 0,0,0 },
    { "Glass Block",    "透明玻璃",   "玻璃",       0.75f, 0.90f, 0.98f, 0.35f, true,  true,  false, 0,0,0 },
    { "Torch",          "火把",       "火把",       1.00f, 0.85f, 0.25f, 0.05f, false, true,  true,  1.0f, 0.85f, 0.3f },
    { "Obsidian",       "黑曜石",     "黑曜石",     0.16f, 0.12f, 0.22f, 25.0f, true,  false, false, 0,0,0 },
    { "Magma Block",    "熔岩块",     "岩浆块",     0.90f, 0.32f, 0.08f, 6.0f,  true,  false, true,  0.9f, 0.35f, 0.1f },
    { "Mossy Stone",    "苔石",       "苔石",       0.42f, 0.52f, 0.40f, 4.5f,  true,  false, false, 0,0,0 },
    { "Mud Block",      "泥浆块",     "泥浆",       0.40f, 0.30f, 0.22f, 0.60f, true,  false, false, 0,0,0 },
    { "Glow Shroom",    "发光蘑菇",   "发光蘑菇",   0.15f, 0.82f, 0.88f, 0.05f, false, true,  true,  0.2f, 0.9f, 0.95f },
    { "Bedrock",        "基岩",       "基岩",       0.12f, 0.12f, 0.14f,-1.0f,  true,  false, false, 0,0,0 },
    { "Work Bench",     "工作台",     "工作台",     0.68f, 0.48f, 0.28f, 2.0f,  true,  false, false, 0,0,0 },
    { "Wooden Chair",   "木椅",       "木椅",       0.72f, 0.45f, 0.22f, 2.0f,  false, true,  false, 0,0,0 },
    { "Wooden Table",   "木桌",       "木桌",       0.65f, 0.42f, 0.22f, 2.0f,  true,  false, false, 0,0,0 },
    { "Wooden Door",    "木门",       "木门",       0.60f, 0.38f, 0.20f, 2.0f,  true,  true,  false, 0,0,0 },
    { "Bed",            "舒适木床",   "舒适木床",   0.85f, 0.25f, 0.25f, 2.0f,  true,  false, false, 0,0,0 },
    { "Furnace",        "石制熔炉",   "石制熔炉",   0.55f, 0.55f, 0.58f, 6.0f,  true,  false, true,  1.0f, 0.65f, 0.25f },
    { "Treasure Chest", "大藏宝箱",   "大藏宝箱",   0.85f, 0.68f, 0.20f, 2.0f,  true,  false, false, 0,0,0 },
    { "Bookshelf",      "精装书架",   "精装书架",   0.55f, 0.35f, 0.18f, 2.0f,  true,  false, false, 0,0,0 },
    { "Clock",          "古典摆钟",   "古典摆钟",   0.50f, 0.32f, 0.16f, 2.0f,  true,  false, false, 0,0,0 },
    { "Chandelier",     "铁艺吊灯",   "铁艺吊灯",   0.95f, 0.85f, 0.45f, 0.50f, false, true,  true,  1.0f, 0.88f, 0.50f },
    { "Wooden Door (Top)",   "木门 (上半)", "木门", 0.60f, 0.38f, 0.20f, 2.0f,  true,  true,  false, 0,0,0 },
    { "Open Door (Bottom)",  "开启木门(下)", "木门", 0.60f, 0.38f, 0.20f, 2.0f,  false, true,  false, 0,0,0 },
    { "Open Door (Top)",     "开启木门(上)", "木门", 0.60f, 0.38f, 0.20f, 2.0f,  false, true,  false, 0,0,0 }
};

static inline bool isDoorBlock(int bt) {
    return (bt == BLOCK_DOOR || bt == BLOCK_DOOR_UPPER ||
            bt == BLOCK_DOOR_OPEN_LOWER || bt == BLOCK_DOOR_OPEN_UPPER);
}
static inline bool isDoorLower(int bt) {
    return (bt == BLOCK_DOOR || bt == BLOCK_DOOR_OPEN_LOWER);
}
static inline bool isDoorUpper(int bt) {
    return (bt == BLOCK_DOOR_UPPER || bt == BLOCK_DOOR_OPEN_UPPER);
}
static inline bool isDoorOpen(int bt) {
    return (bt == BLOCK_DOOR_OPEN_LOWER || bt == BLOCK_DOOR_OPEN_UPPER);
}

// ============================================================================
// Biome System
// ============================================================================
typedef enum {
    BIOME_DESERT = 0,
    BIOME_FOREST,
    BIOME_TUNDRA,
    BIOME_COUNT
} BiomeType;

static const char* g_BiomeNames_ZH[4] = { "干燥沙漠", "苍翠森林", "极地雪原", "地下深渊" };
static const char* g_BiomeNames_EN[4] = { "Sunken Desert", "Verdant Forest", "Glacial Tundra", "Underworld" };
#define g_BiomeNames ((getLanguage() == LANG_ZH) ? g_BiomeNames_ZH : g_BiomeNames_EN)
static inline const char* getBiomeName(BiomeType biome) {
    switch (biome) {
        case BIOME_FOREST: return LOC("苍翠森林", "Verdant Forest");
        case BIOME_DESERT: return LOC("干燥沙漠", "Sunken Desert");
        case BIOME_TUNDRA: return LOC("极地雪原", "Glacial Tundra");
        default: return LOC("地下深渊", "Underworld");
    }
}


// ============================================================================
// World Data & Delta Tracking for Save/Load
// ============================================================================
static unsigned char g_World[WORLD_WIDTH][WORLD_HEIGHT];
static unsigned char g_WallWorld[WORLD_WIDTH][WORLD_HEIGHT];

typedef struct {
    short x, y;
    unsigned char isWall;
    unsigned char blockType;
} BlockDelta;

static BlockDelta g_ModifiedBlocks[MAX_MODIFIED_BLOCKS];
static int g_ModifiedBlockCount = 0;

static void recordBlockDelta(int x, int y, bool isWall, unsigned char type) {
    for (int i = 0; i < g_ModifiedBlockCount; i++) {
        if (g_ModifiedBlocks[i].x == x && g_ModifiedBlocks[i].y == y && g_ModifiedBlocks[i].isWall == (isWall ? 1 : 0)) {
            g_ModifiedBlocks[i].blockType = type;
            return;
        }
    }
    if (g_ModifiedBlockCount < MAX_MODIFIED_BLOCKS) {
        g_ModifiedBlocks[g_ModifiedBlockCount].x = (short)x;
        g_ModifiedBlocks[g_ModifiedBlockCount].y = (short)y;
        g_ModifiedBlocks[g_ModifiedBlockCount].isWall = isWall ? 1 : 0;
        g_ModifiedBlocks[g_ModifiedBlockCount].blockType = type;
        g_ModifiedBlockCount++;
    }
}

// ============================================================================
// Floating Text & Notifications
// ============================================================================
typedef struct {
    bool active;
    char text[64];
    float x, y;
    float r, g, b;
    float life;
    float maxLife;
} FloatingText;

static FloatingText g_FloatingTexts[MAX_FLOATING_TEXTS];
static char g_NotificationText[128] = "";
static float g_NotificationTimer = 0.0f;
static float g_NotificationR = 0.2f, g_NotificationG = 0.9f, g_NotificationB = 0.4f;

static void showNotification(const char* text, float r, float g, float b) {
    strncpy(g_NotificationText, text, sizeof(g_NotificationText) - 1);
    g_NotificationText[sizeof(g_NotificationText) - 1] = '\0';
    g_NotificationTimer = 3.5f;
    g_NotificationR = r;
    g_NotificationG = g;
    g_NotificationB = b;
}

static void spawnFloatingText(const char* text, float x, float y, float r, float g, float b) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!g_FloatingTexts[i].active) {
            g_FloatingTexts[i].active = true;
            strncpy(g_FloatingTexts[i].text, text, sizeof(g_FloatingTexts[i].text) - 1);
            g_FloatingTexts[i].text[sizeof(g_FloatingTexts[i].text) - 1] = '\0';
            g_FloatingTexts[i].x = x;
            g_FloatingTexts[i].y = y;
            g_FloatingTexts[i].r = r;
            g_FloatingTexts[i].g = g;
            g_FloatingTexts[i].b = b;
            g_FloatingTexts[i].life = 0.0f;
            g_FloatingTexts[i].maxLife = 1.4f;
            return;
        }
    }
}

// ============================================================================
// Tutorial Guidance System
// ============================================================================
typedef enum {
    TUTORIAL_MOVE_JUMP = 0,
    TUTORIAL_MINE,
    TUTORIAL_BUILD,
    TUTORIAL_TORCH,
    TUTORIAL_EXPLORE,
    TUTORIAL_COMPLETE
} TutorialStep;

static TutorialStep g_TutorialStep = TUTORIAL_MOVE_JUMP;
static bool g_TutMovedLeft = false;
static bool g_TutMovedRight = false;
static bool g_TutJumped = false;
static int  g_MinedCount = 0;

// ============================================================================
// Dropped Items & Particles
// ============================================================================
typedef struct {
    bool active;
    float x, y;
    float vx, vy;
    float rotation;
    int type;
    int count;
    float life;
} DroppedItem;

static DroppedItem g_DroppedItems[MAX_DROPPED_ITEMS];

typedef struct {
    bool active;
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    float r, g, b;
    float size;
} Particle;

static Particle g_Particles[MAX_PARTICLES];

// ============================================================================
// Player & Inventory
// ============================================================================
typedef struct {
    int type; // BlockType (0..26), ToolItemID (101..116), or ArmorItemID (120..135)
    int count;
    int durability;
    int maxDurability;
} InventorySlot;

typedef struct {
    float x, y;
    float vx, vy;
    float width, height;
    bool isGrounded;
    bool isFacingRight;
    float moveSpeed;
    float jumpSpeed;
    float walkCycle;
    float health;
    float maxHealth;
    float hurtTimer;
    float timeSinceCombat;
    float regenTimer;
    bool isDead;
    float deathTimer;
    float fallVelocity;
    float fallStartY;
} Player;

static Player g_Player;
static int g_SurfaceHeight[WORLD_WIDTH];
static int g_Seed = 1337;
static InventorySlot g_Hotbar[HOTBAR_SLOTS];
static InventorySlot g_Backpack[BACKPACK_SLOTS];
static int g_ActiveSlot = 0;

static int g_EquippedArmor[4] = { 0, 0, 0, 0 };
static int g_ArmorDurability[4] = { 0, 0, 0, 0 };

// ============================================================================
// World Chest Storage Container System
// ============================================================================
#define MAX_WORLD_CHESTS 128
#define CHEST_SLOTS 20

typedef struct {
    int x, y;
    InventorySlot slots[CHEST_SLOTS];
    bool active;
} WorldChest;

static WorldChest g_WorldChests[MAX_WORLD_CHESTS];
static bool g_ChestOpen = false;
static int g_ActiveChestX = -1;
static int g_ActiveChestY = -1;

// Extinguished Lights State (Torches, Chandeliers)
static bool g_LightExtinguished[WORLD_WIDTH][WORLD_HEIGHT];

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

// Camera & World Time
static float g_CamX = 0.0f;
static float g_CamY = 85.0f;
static float g_CamDist = 16.0f;
static float g_WorldTime = 8.0f / 24.0f;
static bool  g_ShowHelp = false;

// Mining State
static int   g_TargetX = -1;
static int   g_TargetY = -1;
static bool  g_TargetValid = false;
static float g_MiningProgress = 0.0f;
static int   g_LastMiningX = -1;
static int   g_LastMiningY = -1;

// Input Keys
static bool g_Keys[256];
static bool g_MouseLeft = false;
static bool g_MouseRight = false;
static int  g_MouseScreenX = 0;
static int  g_MouseScreenY = 0;

// OpenGL Texture Atlas (managed via texture_atlas.h)

static void generateWorld(int seed);
static bool addToInventory(int type, int count);
static int getSurfaceHeight(int x, int seed);

// ============================================================================
// Procedural Math & Noise
// ============================================================================
static float hash1D(int x, int seed) {
    int n = x + seed * 57;
    n = (n << 13) ^ n;
    return (float)(1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

static float smoothNoise1D(float x, int seed) {
    int i = (int)floorf(x);
    float f = x - (float)i;
    float u = f * f * (3.0f - 2.0f * f);
    float a = hash1D(i, seed);
    float b = hash1D(i + 1, seed);
    return a + u * (b - a);
}

static float fbm1D(float x, int seed, int octaves, float freq, float persistence) {
    float total = 0.0f;
    float maxAmp = 0.0f;
    float amp = 1.0f;
    for (int i = 0; i < octaves; i++) {
        total += smoothNoise1D(x * freq, seed + i * 1337) * amp;
        maxAmp += amp;
        amp *= persistence;
        freq *= 2.0f;
    }
    return (total / maxAmp) * 0.5f + 0.5f;
}

static float hash2D(int x, int y, int seed) {
    int n = x + y * 57 + seed * 1337;
    n = (n << 13) ^ n;
    return (float)(1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

static float smoothNoise2D(float x, float y, int seed) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);

    float n00 = hash2D(ix, iy, seed);
    float n10 = hash2D(ix + 1, iy, seed);
    float n01 = hash2D(ix, iy + 1, seed);
    float n11 = hash2D(ix + 1, iy + 1, seed);

    float nx0 = n00 + ux * (n10 - n00);
    float nx1 = n01 + ux * (n11 - n01);
    return nx0 + uy * (nx1 - nx0);
}

static float fbm2D(float x, float y, int seed, int octaves, float freq) {
    float total = 0.0f;
    float maxAmp = 0.0f;
    float amp = 1.0f;
    for (int i = 0; i < octaves; i++) {
        total += smoothNoise2D(x * freq, y * freq, seed + i * 543) * amp;
        maxAmp += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    return (total / maxAmp) * 0.5f + 0.5f;
}

static BiomeType getBiomeAt(int x) {
    if (x < 160) return BIOME_DESERT;
    if (x < 360) return BIOME_FOREST;
    return BIOME_TUNDRA;
}

static int getSurfaceHeight(int x, int seed) {
    BiomeType biome = getBiomeAt(x);
    float baseH = (float)BASE_SURFACE_Y;
    float noise = fbm1D((float)x, seed, 4, 0.02f, 0.5f);

    float h = baseH + (noise - 0.5f) * 22.0f;
    if (biome == BIOME_DESERT) {
        h += sinf((float)x * 0.08f) * 3.5f;
    } else if (biome == BIOME_TUNDRA) {
        float peak = fbm1D((float)x, seed + 888, 3, 0.04f, 0.5f);
        h += (peak - 0.5f) * 14.0f;
    }

    int ih = (int)roundf(h);
    if (ih < CAVERN_LEVEL + 5) ih = CAVERN_LEVEL + 5;
    if (ih > WORLD_HEIGHT - 12) ih = WORLD_HEIGHT - 12;
    return ih;
}

static void spawnTree(int rootX, int rootY, BiomeType biome) {
    if (biome == BIOME_FOREST) {
        int trunkH = 5 + (rand() % 3);
        for (int ty = 0; ty < trunkH; ty++) {
            if (rootY + ty < WORLD_HEIGHT) g_World[rootX][rootY + ty] = BLOCK_WOOD;
        }
        int topY = rootY + trunkH;
        for (int ox = -2; ox <= 2; ox++) {
            for (int oy = -1; oy <= 2; oy++) {
                if (abs(ox) == 2 && oy == 2) continue;
                int lx = rootX + ox;
                int ly = topY + oy;
                if (lx >= 0 && lx < WORLD_WIDTH && ly >= 0 && ly < WORLD_HEIGHT) {
                    if (g_World[lx][ly] == BLOCK_AIR) g_World[lx][ly] = BLOCK_LEAVES;
                }
            }
        }
    } else if (biome == BIOME_DESERT) {
        int cacH = 4 + (rand() % 3);
        for (int cy = 0; cy < cacH; cy++) {
            if (rootY + cy < WORLD_HEIGHT) g_World[rootX][rootY + cy] = BLOCK_CACTUS;
        }
        if (rootX + 1 < WORLD_WIDTH && rootY + 2 < WORLD_HEIGHT) {
            g_World[rootX + 1][rootY + 2] = BLOCK_CACTUS;
            g_World[rootX + 1][rootY + 3] = BLOCK_CACTUS;
        }
    } else if (biome == BIOME_TUNDRA) {
        int trunkH = 7 + (rand() % 3);
        for (int ty = 0; ty < trunkH; ty++) {
            if (rootY + ty < WORLD_HEIGHT) g_World[rootX][rootY + ty] = BLOCK_WOOD;
        }
        int topY = rootY + trunkH;
        for (int layer = 0; layer < 4; layer++) {
            int rad = 3 - layer;
            int ly = topY - layer * 2;
            for (int ox = -rad; ox <= rad; ox++) {
                int lx = rootX + ox;
                if (lx >= 0 && lx < WORLD_WIDTH && ly >= 0 && ly < WORLD_HEIGHT) {
                    if (g_World[lx][ly] == BLOCK_AIR) g_World[lx][ly] = BLOCK_LEAVES;
                }
            }
        }
    }
}

static void spawnOreVein(int startX, int startY, BlockType oreType, int count) {
    int curX = startX;
    int curY = startY;
    for (int b = 0; b < count; b++) {
        if (curX >= 1 && curX < WORLD_WIDTH - 1 && curY > BEDROCK_LEVEL && curY < WORLD_HEIGHT - 2) {
            BlockType bt = g_World[curX][curY];
            if (bt == BLOCK_STONE || bt == BLOCK_DIRT || bt == BLOCK_SANDSTONE ||
                bt == BLOCK_ICE || bt == BLOCK_OBSIDIAN || bt == BLOCK_MAGMA || bt == BLOCK_MOSSY_STONE) {
                g_World[curX][curY] = oreType;
            }
        }
        int dir = rand() % 4;
        if (dir == 0) curX++;
        else if (dir == 1) curX--;
        else if (dir == 2) curY++;
        else curY--;
    }
}

static void generateWorld(int seed) {
    memset(g_World, BLOCK_AIR, sizeof(g_World));
    memset(g_WallWorld, BLOCK_AIR, sizeof(g_WallWorld));
    memset(g_WorldChests, 0, sizeof(g_WorldChests));
    memset(g_LightExtinguished, 0, sizeof(g_LightExtinguished));
    g_ChestOpen = false;
    g_ActiveChestX = -1;
    g_ActiveChestY = -1;
    g_ModifiedBlockCount = 0;

    g_Seed = seed;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        int surfaceY = getSurfaceHeight(x, seed);
        g_SurfaceHeight[x] = surfaceY;
        BiomeType biome = getBiomeAt(x);

        BlockType surfaceBlock = (biome == BIOME_DESERT) ? BLOCK_SAND : ((biome == BIOME_TUNDRA) ? BLOCK_SNOW : BLOCK_GRASS);
        BlockType subBlock     = (biome == BIOME_DESERT) ? BLOCK_SANDSTONE : ((biome == BIOME_TUNDRA) ? BLOCK_ICE : BLOCK_DIRT);
        BlockType wallBlock    = (biome == BIOME_DESERT) ? BLOCK_SANDSTONE : BLOCK_DIRT;

        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (y <= BEDROCK_LEVEL) {
                g_World[x][y] = BLOCK_BEDROCK;
                g_WallWorld[x][y] = BLOCK_STONE;
            } else if (y <= UNDERWORLD_LEVEL) {
                g_World[x][y] = (fbm2D((float)x, (float)y, seed + 10, 2, 0.08f) > 0.65f) ? BLOCK_MAGMA : BLOCK_OBSIDIAN;
                g_WallWorld[x][y] = BLOCK_OBSIDIAN;
            } else if (y < surfaceY - 10) {
                g_World[x][y] = BLOCK_STONE;
                g_WallWorld[x][y] = BLOCK_STONE;
            } else if (y < surfaceY) {
                g_World[x][y] = (biome == BIOME_FOREST) ? BLOCK_DIRT : subBlock;
                g_WallWorld[x][y] = wallBlock;
            } else if (y == surfaceY) {
                g_World[x][y] = surfaceBlock;
                g_WallWorld[x][y] = wallBlock;
            }
        }
    }

    for (int x = 0; x < WORLD_WIDTH; x++) {
        int surfaceY = getSurfaceHeight(x, seed);
        for (int y = BEDROCK_LEVEL + 2; y < surfaceY - 2; y++) {
            float caveNoise = fbm2D((float)x, (float)y, seed + 200, 3, 0.045f);
            float wormNoise = fabsf(smoothNoise2D((float)x * 0.06f, (float)y * 0.06f, seed + 300) - 0.5f);

            if (caveNoise > 0.62f || (wormNoise < 0.05f && y < surfaceY - 6)) {
                g_World[x][y] = BLOCK_AIR;
            }
        }
    }

    // Rich Ore Vein Generation (Minecraft-Style Multi-Block Clusters)
    for (int x = 2; x < WORLD_WIDTH - 2; x += 2) {
        int surfaceY = g_SurfaceHeight[x];
        for (int y = BEDROCK_LEVEL + 3; y < surfaceY - 2; y += 2) {
            // 1. Diamond & Ruby Ores: In caverns and underworld, balanced rare clusters (noise threshold 0.88f)
            if (y < CAVERN_LEVEL) {
                float gemN = fbm2D((float)x, (float)y, seed + 440, 2, 0.16f);
                if (gemN > 0.88f) {
                    BlockType gemType = (gemN > 0.94f) ? BLOCK_RUBY_ORE : BLOCK_DIAMOND_ORE;
                    int clusterSize = 1 + (rand() % 2); // 1-2 blocks
                    spawnOreVein(x, y, gemType, clusterSize);
                    continue;
                }
            }

            // 2. Gold Ore: Starts from surfaceY - 12, veins (noise threshold 0.84f)
            if (y < surfaceY - 12) {
                float goldN = fbm2D((float)x, (float)y, seed + 430, 2, 0.16f);
                if (goldN > 0.84f) {
                    int clusterSize = 1 + (rand() % 2); // 1-2 blocks
                    spawnOreVein(x, y, BLOCK_GOLD_ORE, clusterSize);
                    continue;
                }
            }

            // 3. Iron Ore: Starts from surfaceY - 5, veins (noise threshold 0.80f)
            if (y < surfaceY - 5) {
                float ironN = fbm2D((float)x, (float)y, seed + 420, 2, 0.16f);
                if (ironN > 0.80f) {
                    int clusterSize = 2 + (rand() % 2); // 2-3 blocks
                    spawnOreVein(x, y, BLOCK_IRON_ORE, clusterSize);
                    continue;
                }
            }

            // 4. Coal Ore: Starts from surfaceY - 2, clusters (noise threshold 0.77f)
            if (y < surfaceY - 2) {
                float coalN = fbm2D((float)x, (float)y, seed + 410, 2, 0.16f);
                if (coalN > 0.77f) {
                    int clusterSize = 2 + (rand() % 2); // 2-3 blocks
                    spawnOreVein(x, y, BLOCK_COAL_ORE, clusterSize);
                    continue;
                }
            }
        }
    }

    for (int x = 4; x < WORLD_WIDTH - 4; x++) {
        for (int y = UNDERWORLD_LEVEL + 2; y < CAVERN_LEVEL; y++) {
            if (g_World[x][y] == BLOCK_AIR && g_World[x][y - 1] != BLOCK_AIR) {
                if ((rand() % 30) == 0) {
                    g_World[x][y] = BLOCK_GLOW_SHROOM;
                }
            }
        }
    }

    for (int x = 5; x < WORLD_WIDTH - 5; x += (4 + (rand() % 4))) {
        int sy = getSurfaceHeight(x, seed);
        if (g_World[x][sy] != BLOCK_AIR && g_World[x][sy + 1] == BLOCK_AIR) {
            spawnTree(x, sy + 1, getBiomeAt(x));
        }
    }
}

// ============================================================================
// Save & Load System
// ============================================================================
#define SAVE_MAGIC 0x54455252 // "TERR"



static void spawnParticle(float x, float y, float r, float g, float b, float vx, float vy) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g_Particles[i].active) {
            g_Particles[i].active = true;
            g_Particles[i].x = x;
            g_Particles[i].y = y;
            g_Particles[i].vx = vx;
            g_Particles[i].vy = vy;
            g_Particles[i].life = 0.0f;
            g_Particles[i].maxLife = 0.4f + ((float)rand() / RAND_MAX) * 0.3f;
            g_Particles[i].r = r;
            g_Particles[i].g = g;
            g_Particles[i].b = b;
            g_Particles[i].size = 0.12f;
            return;
        }
    }
}

static void spawnDroppedItem(float x, float y, int type, int count) {
    if (type == BLOCK_AIR) return;
    for (int i = 0; i < MAX_DROPPED_ITEMS; i++) {
        if (!g_DroppedItems[i].active) {
            g_DroppedItems[i].active = true;
            g_DroppedItems[i].x = x;
            g_DroppedItems[i].y = y;
            g_DroppedItems[i].vx = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
            g_DroppedItems[i].vy = 2.5f + ((float)rand() / RAND_MAX) * 2.0f;
            g_DroppedItems[i].rotation = 0.0f;
            g_DroppedItems[i].type = type;
            g_DroppedItems[i].count = count;
            g_DroppedItems[i].life = 0.0f;
            return;
        }
    }
}

static bool addToInventory(int type, int count) {
    if (type == BLOCK_AIR || count <= 0) return true;

    char buf[64];
    const char* itemName = "Item";
    float itemR = 0.9f, itemG = 0.9f, itemB = 0.9f;
    if (type >= 0 && type < BLOCK_TYPE_COUNT) {
        itemName = (getLanguage() == LANG_ZH) ? g_BlockDefs[type].chineseName : g_BlockDefs[type].name;
        itemR = g_BlockDefs[type].r;
        itemG = g_BlockDefs[type].g;
        itemB = g_BlockDefs[type].b;
    } else if (type == 118) { // ITEM_GEL
        itemName = (getLanguage() == LANG_ZH) ? "史莱姆凝胶" : "Slime Gel";
        itemR = 0.25f;
        itemG = 0.88f;
        itemB = 0.45f;
    } else if (type == 117) { // ITEM_STICK
        itemName = (getLanguage() == LANG_ZH) ? "木棍" : "Stick";
        itemR = 0.72f;
        itemG = 0.52f;
        itemB = 0.30f;
    }
    snprintf(buf, sizeof(buf), "+%d %s", count, itemName);
    spawnFloatingText(buf, g_Player.x, g_Player.y + 1.8f, itemR, itemG, itemB);

    int remaining = count;

    // a) Try stacking in g_Hotbar[0..8]
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == (int)type && g_Hotbar[i].count < 999) {
            int space = 999 - g_Hotbar[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Hotbar[i].count += take;
            remaining -= take;
        }
    }
    if (remaining <= 0) return true;

    // b) If not fully placed, try stacking in g_Backpack[0..26]
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == (int)type && g_Backpack[i].count < 999) {
            int space = 999 - g_Backpack[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Backpack[i].count += take;
            remaining -= take;
        }
    }
    if (remaining <= 0) return true;

    // c) If still not placed, try finding an empty slot in g_Hotbar[0..8]
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == BLOCK_AIR || g_Hotbar[i].count <= 0) {
            int take = (remaining < 999) ? remaining : 999;
            g_Hotbar[i].type = (int)type;
            g_Hotbar[i].count = take;
            g_Hotbar[i].durability = 0;
            g_Hotbar[i].maxDurability = 0;
            remaining -= take;
        }
    }
    if (remaining <= 0) return true;

    // d) If still not placed, try finding an empty slot in g_Backpack[0..26]
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == BLOCK_AIR || g_Backpack[i].count <= 0) {
            int take = (remaining < 999) ? remaining : 999;
            g_Backpack[i].type = (int)type;
            g_Backpack[i].count = take;
            g_Backpack[i].durability = 0;
            g_Backpack[i].maxDurability = 0;
            remaining -= take;
        }
    }
    if (remaining <= 0) return true;

    return false;
}


static bool isSolidBlock(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return true;
    return g_BlockDefs[g_World[x][y]].isSolid;
}

static void drawQuad2D(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void drawBorder2D(float x, float y, float w, float h, float r, float g, float b, float width) {
    glColor3f(r, g, b);
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}

static void toggleFullscreen(HWND hwnd) {
    if (!hwnd) hwnd = g_MainHwnd;
    if (!hwnd) return;

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (!g_IsFullscreen) {
        MONITORINFO mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (GetWindowPlacement(hwnd, &g_PrevPlacement) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            g_IsFullscreen = true;
            g_ResPresetIndex = 3;
            playSound(SFX_CLICK);
            showNotification(LOC("已开启无边框全屏模式 [按 F11 还原窗口]", "Switched to: Fullscreen [Press F11 to Restore]"), 0.35f, 0.90f, 1.0f);
        }
    } else {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_PrevPlacement);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_IsFullscreen = false;
        g_ResPresetIndex = 0;
        playSound(SFX_CLICK);
        showNotification(LOC("已还原窗口模式 [按 F11 开启全屏]", "Switched to: Windowed Mode [Press F11 for Fullscreen]"), 0.35f, 0.95f, 0.45f);
    }
}

static void cycleResolutionPreset(HWND hwnd) {
    if (!hwnd) hwnd = g_MainHwnd;
    if (!hwnd) return;

    int nextPreset = (g_ResPresetIndex + 1) % 4;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    if (nextPreset == 3) {
        if (!g_IsFullscreen) {
            toggleFullscreen(hwnd);
        }
        g_ResPresetIndex = 3;
        return;
    }

    if (g_IsFullscreen) {
        toggleFullscreen(hwnd);
    }

    int targetW = 1280;
    int targetH = 720;
    if (nextPreset == 1) {
        targetW = 1600;
        targetH = 900;
    } else if (nextPreset == 2) {
        targetW = 1920;
        targetH = 1080;
    }

    if (targetW > screenW || targetH > screenH) {
        if (!g_IsFullscreen) toggleFullscreen(hwnd);
        g_ResPresetIndex = 3;
        return;
    }

    RECT wr = { 0, 0, targetW, targetH };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    int winW = wr.right - wr.left;
    int winH = wr.bottom - wr.top;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;
    if (winX < 0) winX = 0;
    if (winY < 0) winY = 0;

    SetWindowPos(hwnd, NULL, winX, winY, winW, winH, SWP_NOZORDER | SWP_FRAMECHANGED);
    g_ResPresetIndex = nextPreset;

    char msgBuf[128];
    snprintf(msgBuf, sizeof(msgBuf), "%s: %dx%d [F11 %s]",
        LOC("分辨率已切换为", "Resolution set to"), targetW, targetH,
        LOC("全屏", "Fullscreen"));
    showNotification(msgBuf, 0.4f, 0.85f, 1.0f);
}

#include "menu_creative.h"
#include "creative_inventory.h"
#include "crafting_system.h"
#include "mobs.h"

static bool saveWorldSlot(int slotIdx) {
    char path[128];
    getSlotFilePath(slotIdx, path, sizeof(path));

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    SaveFileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = SAVE_MAGIC_V2;
    hdr.version = 2;
    snprintf(hdr.worldName, sizeof(hdr.worldName), "%s", g_CurrentWorldName);
    hdr.mode = (int)getGameMode();
    hdr.seed = g_Seed;
    hdr.timestamp = (long long)time(NULL);
    hdr.playerHealth = g_Player.health;
    hdr.playerMaxHealth = g_Player.maxHealth;
    hdr.playerX = g_Player.x;
    hdr.playerY = g_Player.y;
    hdr.playerVx = g_Player.vx;
    hdr.playerVy = g_Player.vy;
    hdr.isFacingRight = g_Player.isFacingRight;
    hdr.worldTime = g_WorldTime;
    hdr.tutorialStep = (int)g_TutorialStep;
    hdr.minedCount = g_MinedCount;
    hdr.activeSlot = g_ActiveSlot;

    for (int i = 0; i < 9; i++) {
        hdr.hotbar[i].type = g_Hotbar[i].type;
        hdr.hotbar[i].count = g_Hotbar[i].count;
    }
    for (int i = 0; i < 27; i++) {
        hdr.backpack[i].type = g_Backpack[i].type;
        hdr.backpack[i].count = g_Backpack[i].count;
    }
    hdr.modifiedBlockCount = g_ModifiedBlockCount;

    fwrite(&hdr, sizeof(SaveFileHeader), 1, f);
    if (g_ModifiedBlockCount > 0) {
        fwrite(g_ModifiedBlocks, sizeof(BlockDelta), g_ModifiedBlockCount, f);
    }

    // Persist World Chests
    int activeChests = 0;
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active) activeChests++;
    }
    fwrite(&activeChests, sizeof(int), 1, f);
    for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
        if (g_WorldChests[i].active) {
            fwrite(&g_WorldChests[i], sizeof(WorldChest), 1, f);
        }
    }

    // Persist Extinguished Lights
    int extCount = 0;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (g_LightExtinguished[x][y]) extCount++;
        }
    }
    fwrite(&extCount, sizeof(int), 1, f);
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (g_LightExtinguished[x][y]) {
                short pt[2] = { (short)x, (short)y };
                fwrite(pt, sizeof(short), 2, f);
            }
        }
    }
    fclose(f);

    if (slotIdx == 0) {
        FILE* lf = fopen("savegame.bin", "wb");
        if (lf) {
            unsigned int magic = 0x54455252;
            int version = 1;
            fwrite(&magic, sizeof(unsigned int), 1, lf);
            fwrite(&version, sizeof(int), 1, lf);
            fwrite(&g_Player.x, sizeof(float), 1, lf);
            fwrite(&g_Player.y, sizeof(float), 1, lf);
            fwrite(&g_Player.vx, sizeof(float), 1, lf);
            fwrite(&g_Player.vy, sizeof(float), 1, lf);
            fwrite(&g_Player.isFacingRight, sizeof(bool), 1, lf);
            fwrite(&g_ActiveSlot, sizeof(int), 1, lf);
            fwrite(g_Hotbar, sizeof(InventorySlot), HOTBAR_SLOTS, lf);
            fwrite(&g_WorldTime, sizeof(float), 1, lf);
            int tutStep = (int)g_TutorialStep;
            fwrite(&tutStep, sizeof(int), 1, lf);
            fwrite(&g_MinedCount, sizeof(int), 1, lf);
            fwrite(&g_ModifiedBlockCount, sizeof(int), 1, lf);
            if (g_ModifiedBlockCount > 0) fwrite(g_ModifiedBlocks, sizeof(BlockDelta), g_ModifiedBlockCount, lf);
            int currentLang = (int)getLanguage();
            fwrite(&currentLang, sizeof(int), 1, lf);

            int activeChestsLegacy = 0;
            for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
                if (g_WorldChests[i].active) activeChestsLegacy++;
            }
            fwrite(&activeChestsLegacy, sizeof(int), 1, lf);
            for (int i = 0; i < MAX_WORLD_CHESTS; i++) {
                if (g_WorldChests[i].active) {
                    fwrite(&g_WorldChests[i], sizeof(WorldChest), 1, lf);
                }
            }

            int extCountLegacy = 0;
            for (int x = 0; x < WORLD_WIDTH; x++) {
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    if (g_LightExtinguished[x][y]) extCountLegacy++;
                }
            }
            fwrite(&extCountLegacy, sizeof(int), 1, lf);
            for (int x = 0; x < WORLD_WIDTH; x++) {
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    if (g_LightExtinguished[x][y]) {
                        short pt[2] = { (short)x, (short)y };
                        fwrite(pt, sizeof(short), 2, lf);
                    }
                }
            }

            fclose(lf);
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), LOC("世界已保存至存档位 %d！[F5]", "World Saved to Slot %d! [F5]"), slotIdx + 1);
    showNotification(msg, 0.2f, 0.95f, 0.4f);
    return true;
}

static bool loadWorldSlot(int slotIdx) {
    char path[128];
    getSlotFilePath(slotIdx, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (!f) {
        if (slotIdx == 0) {
            FILE* lf = fopen("savegame.bin", "rb");
            if (lf) {
                fclose(lf);
                FILE* f2 = fopen("savegame.bin", "rb");
                unsigned int magic = 0; int version = 0;
                fread(&magic, sizeof(unsigned int), 1, f2);
                fread(&version, sizeof(int), 1, f2);
                generateWorld(1337);
                fread(&g_Player.x, sizeof(float), 1, f2);
                fread(&g_Player.y, sizeof(float), 1, f2);
                fread(&g_Player.vx, sizeof(float), 1, f2);
                fread(&g_Player.vy, sizeof(float), 1, f2);
                fread(&g_Player.isFacingRight, sizeof(bool), 1, f2);
                fread(&g_ActiveSlot, sizeof(int), 1, f2);
                fread(g_Hotbar, sizeof(InventorySlot), HOTBAR_SLOTS, f2);
                fread(&g_WorldTime, sizeof(float), 1, f2);
                int tutStep = 0; fread(&tutStep, sizeof(int), 1, f2); g_TutorialStep = (TutorialStep)tutStep;
                fread(&g_MinedCount, sizeof(int), 1, f2);
                int count = 0; fread(&count, sizeof(int), 1, f2);
                if (count > 0 && count <= MAX_MODIFIED_BLOCKS) {
                    fread(g_ModifiedBlocks, sizeof(BlockDelta), count, f2);
                    g_ModifiedBlockCount = count;
                    for (int i = 0; i < count; i++) {
                        int x = g_ModifiedBlocks[i].x; int y = g_ModifiedBlocks[i].y;
                        if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                            if (g_ModifiedBlocks[i].isWall) g_WallWorld[x][y] = g_ModifiedBlocks[i].blockType;
                            else g_World[x][y] = g_ModifiedBlocks[i].blockType;
                        }
                    }
                }

                // Load World Chests
                memset(g_WorldChests, 0, sizeof(g_WorldChests));
                g_ChestOpen = false;
                int legacyChestCount = 0;
                if (fread(&legacyChestCount, sizeof(int), 1, f2) == 1 && legacyChestCount >= 0 && legacyChestCount <= MAX_WORLD_CHESTS) {
                    for (int i = 0; i < legacyChestCount; i++) {
                        WorldChest chest;
                        if (fread(&chest, sizeof(WorldChest), 1, f2) == 1) {
                            WorldChest* dst = getOrCreateChest(chest.x, chest.y);
                            if (dst) {
                                memcpy(dst->slots, chest.slots, sizeof(chest.slots));
                                dst->active = true;
                            }
                        }
                    }
                }

                // Load Extinguished Lights
                memset(g_LightExtinguished, 0, sizeof(g_LightExtinguished));
                int legacyExtCount = 0;
                if (fread(&legacyExtCount, sizeof(int), 1, f2) == 1 && legacyExtCount >= 0 && legacyExtCount <= WORLD_WIDTH * WORLD_HEIGHT) {
                    for (int i = 0; i < legacyExtCount; i++) {
                        short pt[2];
                        if (fread(pt, sizeof(short), 2, f2) == 2) {
                            if (pt[0] >= 0 && pt[0] < WORLD_WIDTH && pt[1] >= 0 && pt[1] < WORLD_HEIGHT) {
                                g_LightExtinguished[pt[0]][pt[1]] = true;
                            }
                        }
                    }
                }

                fclose(f2);
                g_Player.health = 100.0f; g_Player.maxHealth = 100.0f; g_Player.isDead = false;
                g_Player.fallStartY = g_Player.y;
                showNotification(LOC("已读取旧版世界存档！", "Loaded legacy save!"), 0.35f, 0.95f, 0.45f);
                return true;
            }
        }
        showNotification(LOC("未找到世界存档文件！", "Save file not found!"), 0.95f, 0.3f, 0.2f);
        return false;
    }

    SaveFileHeader hdr;
    if (fread(&hdr, sizeof(SaveFileHeader), 1, f) != 1 || hdr.magic != SAVE_MAGIC_V2) {
        fclose(f);
        showNotification(LOC("世界存档文件已损坏！", "Save file corrupted!"), 0.95f, 0.2f, 0.2f);
        return false;
    }

    g_ActiveSaveSlot = slotIdx;
    snprintf(g_CurrentWorldName, sizeof(g_CurrentWorldName), "%s", hdr.worldName);
    g_Seed = hdr.seed;
    generateWorld(hdr.seed);
    initMobs();

    setGameMode((hdr.mode == 0) ? MODE_SURVIVAL : MODE_CREATIVE);

    g_Player.x = hdr.playerX;
    g_Player.y = hdr.playerY;
    g_Player.vx = hdr.playerVx;
    g_Player.vy = hdr.playerVy;
    g_Player.isFacingRight = hdr.isFacingRight;
    g_Player.health = (hdr.playerHealth > 0.0f) ? hdr.playerHealth : 100.0f;
    g_Player.maxHealth = (hdr.playerMaxHealth > 0.0f) ? hdr.playerMaxHealth : 100.0f;
    g_Player.hurtTimer = 0.0f;
    g_Player.isDead = false;
    g_Player.deathTimer = 0.0f;
    g_Player.fallStartY = g_Player.y;

    g_WorldTime = hdr.worldTime;
    g_TutorialStep = (TutorialStep)hdr.tutorialStep;
    g_MinedCount = hdr.minedCount;
    g_ActiveSlot = hdr.activeSlot;

    for (int i = 0; i < 9; i++) {
        g_Hotbar[i].type = hdr.hotbar[i].type;
        g_Hotbar[i].count = hdr.hotbar[i].count;
        int maxD = getMaxDurability(g_Hotbar[i].type);
        g_Hotbar[i].durability = maxD;
        g_Hotbar[i].maxDurability = maxD;
    }
    for (int i = 0; i < 27; i++) {
        g_Backpack[i].type = hdr.backpack[i].type;
        g_Backpack[i].count = hdr.backpack[i].count;
        int maxD = getMaxDurability(g_Backpack[i].type);
        g_Backpack[i].durability = maxD;
        g_Backpack[i].maxDurability = maxD;
    }
    for (int i = 0; i < 4; i++) {
        g_EquippedArmor[i] = 0;
        g_ArmorDurability[i] = 0;
    }

    int count = hdr.modifiedBlockCount;
    g_ModifiedBlockCount = 0;
    if (count > 0 && count <= MAX_MODIFIED_BLOCKS) {
        fread(g_ModifiedBlocks, sizeof(BlockDelta), count, f);
        g_ModifiedBlockCount = count;
        for (int i = 0; i < count; i++) {
            int x = g_ModifiedBlocks[i].x;
            int y = g_ModifiedBlocks[i].y;
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                if (g_ModifiedBlocks[i].isWall) {
                    g_WallWorld[x][y] = g_ModifiedBlocks[i].blockType;
                } else {
                    g_World[x][y] = g_ModifiedBlocks[i].blockType;
                }
            }
        }
    }

    // Load World Chests
    memset(g_WorldChests, 0, sizeof(g_WorldChests));
    g_ChestOpen = false;
    int chestCount = 0;
    if (fread(&chestCount, sizeof(int), 1, f) == 1 && chestCount >= 0 && chestCount <= MAX_WORLD_CHESTS) {
        for (int i = 0; i < chestCount; i++) {
            WorldChest chest;
            if (fread(&chest, sizeof(WorldChest), 1, f) == 1) {
                WorldChest* dst = getOrCreateChest(chest.x, chest.y);
                if (dst) {
                    memcpy(dst->slots, chest.slots, sizeof(chest.slots));
                    dst->active = true;
                }
            }
        }
    }

    // Load Extinguished Lights
    memset(g_LightExtinguished, 0, sizeof(g_LightExtinguished));
    int extCount = 0;
    if (fread(&extCount, sizeof(int), 1, f) == 1 && extCount >= 0 && extCount <= WORLD_WIDTH * WORLD_HEIGHT) {
        for (int i = 0; i < extCount; i++) {
            short pt[2];
            if (fread(pt, sizeof(short), 2, f) == 2) {
                if (pt[0] >= 0 && pt[0] < WORLD_WIDTH && pt[1] >= 0 && pt[1] < WORLD_HEIGHT) {
                    g_LightExtinguished[pt[0]][pt[1]] = true;
                }
            }
        }
    }
    fclose(f);

    char msg[128];
    snprintf(msg, sizeof(msg), LOC("世界已成功读取！(%s)", "World loaded! (%s)"), g_CurrentWorldName);
    showNotification(msg, 0.35f, 0.95f, 0.45f);
    return true;
}

static bool saveGame(void) {
    return saveWorldSlot(g_ActiveSaveSlot);
}

static bool loadGame(void) {
    return loadWorldSlot(g_ActiveSaveSlot);
}

static void startNewWorld(int slotIdx, const char* name, int mode, int seed) {
    g_ActiveSaveSlot = slotIdx;
    snprintf(g_CurrentWorldName, sizeof(g_CurrentWorldName), "%s", name);
    g_Seed = seed;
    g_ModifiedBlockCount = 0;
    generateWorld(seed);
    initMobs();

    setGameMode((mode == 0) ? MODE_SURVIVAL : MODE_CREATIVE);

    int spawnX = WORLD_WIDTH / 2;
    int spawnY = getSurfaceHeight(spawnX, seed) + 2;
    g_Player.x = (float)spawnX;
    g_Player.y = (float)spawnY;
    g_Player.vx = 0.0f;
    g_Player.vy = 0.0f;
    g_Player.fallVelocity = 0.0f;
    g_Player.fallStartY = (float)spawnY;
    g_WorldTime = 8.0f / 24.0f;
    g_Player.health = 100.0f;
    g_Player.maxHealth = 100.0f;
    g_Player.hurtTimer = 0.0f;
    g_Player.timeSinceCombat = 25.0f;
    g_Player.regenTimer = 0.0f;
    g_Player.isDead = false;
    g_Player.deathTimer = 0.0f;

    for (int i = 0; i < 4; i++) {
        g_EquippedArmor[i] = 0;
        g_ArmorDurability[i] = 0;
    }

    // Requirement 3: In Survival Mode start from 0 items (empty hotbar & backpack)
    if (mode == 0) { // Survival
        for (int i = 0; i < HOTBAR_SLOTS; i++) {
            g_Hotbar[i].type = BLOCK_AIR;
            g_Hotbar[i].count = 0;
            g_Hotbar[i].durability = 0;
            g_Hotbar[i].maxDurability = 0;
        }
        for (int i = 0; i < BACKPACK_SLOTS; i++) {
            g_Backpack[i].type = BLOCK_AIR;
            g_Backpack[i].count = 0;
            g_Backpack[i].durability = 0;
            g_Backpack[i].maxDurability = 0;
        }
    } else { // Creative
        g_Hotbar[0] = (InventorySlot){ ITEM_TOOL_DIAMOND_PICKAXE, 1, 1560, 1560 };
        g_Hotbar[1] = (InventorySlot){ ITEM_TOOL_DIAMOND_SWORD, 1, 1560, 1560 };
        g_Hotbar[2] = (InventorySlot){ BLOCK_DIRT, 999, 0, 0 };
        g_Hotbar[3] = (InventorySlot){ BLOCK_STONE, 999, 0, 0 };
        g_Hotbar[4] = (InventorySlot){ BLOCK_TORCH, 999, 0, 0 };
        g_Hotbar[5] = (InventorySlot){ BLOCK_GLASS, 999, 0, 0 };
        g_Hotbar[6] = (InventorySlot){ BLOCK_BRICK, 999, 0, 0 };
        g_Hotbar[7] = (InventorySlot){ BLOCK_GLOW_SHROOM, 999, 0, 0 };
        g_Hotbar[8] = (InventorySlot){ BLOCK_WOOD_PLANK, 999, 0, 0 };
        for (int i = 0; i < BACKPACK_SLOTS; i++) {
            g_Backpack[i].type = BLOCK_AIR;
            g_Backpack[i].count = 0;
            g_Backpack[i].durability = 0;
            g_Backpack[i].maxDurability = 0;
        }
    }

    saveWorldSlot(slotIdx);
}

void onPlayerHurt(float damage, float sourceX) {
    if (getGameMode() != MODE_SURVIVAL) return;
    if (g_Player.isDead || g_Player.hurtTimer > 0.0f) return;

    float reduction = getPlayerArmorReduction();
    float actualDamage = damage * (1.0f - reduction);
    if (actualDamage < 1.0f && damage > 0.0f) actualDamage = 1.0f;

    g_Player.health -= actualDamage;
    g_Player.hurtTimer = 0.65f;
    g_Player.timeSinceCombat = 0.0f;
    playSound(SFX_HIT);

    // Each equipped armor piece loses 1 durability
    for (int i = 0; i < 4; i++) {
        if (g_EquippedArmor[i] != 0 && g_ArmorDurability[i] > 0) {
            g_ArmorDurability[i]--;
            if (g_ArmorDurability[i] <= 0) {
                int brokenId = g_EquippedArmor[i];
                g_EquippedArmor[i] = 0;
                g_ArmorDurability[i] = 0;
                playSound(SFX_BREAK);
                char breakNotif[128];
                snprintf(breakNotif, sizeof(breakNotif), "%s%s%s",
                         LOC("[防具已损坏: ", "[Armor Broken: "),
                         getItemLocalizedName(brokenId),
                         "]");
                showNotification(breakNotif, 1.0f, 0.35f, 0.35f);
            }
        }
    }

    float pushDir = (g_Player.x >= sourceX) ? 1.0f : -1.0f;
    g_Player.vx = pushDir * 6.5f;
    g_Player.vy = 5.8f;

    char dmgStr[16];
    snprintf(dmgStr, sizeof(dmgStr), "-%d", (int)(actualDamage + 0.5f));
    spawnFloatingText(dmgStr, g_Player.x, g_Player.y + 1.8f, 1.0f, 0.2f, 0.2f);

    if (g_Player.health <= 0.0f) {
        g_Player.health = 0.0f;
        g_Player.isDead = true;
        g_Player.deathTimer = 0.0f;
        playSound(SFX_KILL);
    }
}

static void respawnPlayer(void) {
    int spawnX = WORLD_WIDTH / 2;
    int spawnY = getSurfaceHeight(spawnX, g_Seed) + 2;
    g_Player.x = (float)spawnX;
    g_Player.y = (float)spawnY;
    g_Player.vx = 0.0f;
    g_Player.vy = 0.0f;
    g_Player.fallVelocity = 0.0f;
    g_Player.fallStartY = (float)spawnY;
    g_Player.health = g_Player.maxHealth;
    g_Player.hurtTimer = 1.0f;
    g_Player.timeSinceCombat = 25.0f;
    g_Player.regenTimer = 0.0f;
    g_Player.isDead = false;
    g_Player.deathTimer = 0.0f;
    playSound(SFX_CRAFT);
    showNotification(LOC("你已重新复活！", "You have respawned!"), 0.35f, 0.95f, 0.45f);
}


static void initGame(void) {
    initSaveSystem();
    srand(1337);
    generateWorld(1337);
    initMobs();

    g_Player.health = 100.0f;
    g_Player.maxHealth = 100.0f;
    g_Player.hurtTimer = 0.0f;
    g_Player.fallVelocity = 0.0f;
    g_Player.fallStartY = 0.0f;
    g_WorldTime = 8.0f / 24.0f;
    g_Player.isDead = false;
    g_Player.deathTimer = 0.0f;

    // Requirement 3: In Survival Mode start from 0 items (empty hotbar & backpack)
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        g_Hotbar[i].type = BLOCK_AIR;
        g_Hotbar[i].count = 0;
    }
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        g_Backpack[i].type = BLOCK_AIR;
        g_Backpack[i].count = 0;
    }

    int spawnX = WORLD_WIDTH / 2;
    int spawnY = getSurfaceHeight(spawnX, 1337) + 2;
    g_Player.x = (float)spawnX;
    g_Player.y = (float)spawnY;
    g_Player.fallStartY = (float)spawnY;
    g_Player.vx = 0.0f;
    g_Player.vy = 0.0f;
    g_Player.width = 0.76f;
    g_Player.height = 1.8f;
    g_Player.isGrounded = false;
    g_Player.isFacingRight = true;
    g_Player.moveSpeed = 8.5f;
    g_Player.jumpSpeed = 13.5f;
    g_Player.walkCycle = 0.0f;

    g_CamX = g_Player.x;
    g_CamY = g_Player.y;

    showNotification(LOC("欢迎来到 Luminora｜流明洛原！按 [H] 查看指南，[F5] 快速保存。", "Welcome to Luminora! Press [H] for Help, [F5] to Save."), 1.0f, 0.85f, 0.3f);
}


static void updatePhysics(float dt) {
    if (g_Player.hurtTimer > 0.0f) {
        g_Player.hurtTimer -= dt;
        if (g_Player.hurtTimer < 0.0f) g_Player.hurtTimer = 0.0f;
    }

    // Requirement 3: Natural Health Regeneration (After leaving combat for 20s, start slowly regenerating health)
    if (getGameMode() == MODE_SURVIVAL && !g_Player.isDead) {
        g_Player.timeSinceCombat += dt;
        if (g_Player.timeSinceCombat >= 20.0f && g_Player.health < g_Player.maxHealth) {
            float regenRate = 3.5f; // 3.5 HP per second
            g_Player.health += regenRate * dt;
            if (g_Player.health > g_Player.maxHealth) g_Player.health = g_Player.maxHealth;

            g_Player.regenTimer += dt;
            if (g_Player.regenTimer >= 2.5f) {
                g_Player.regenTimer = 0.0f;
                char rBuf[32];
                snprintf(rBuf, sizeof(rBuf), "+%d HP", (int)(regenRate * 2.5f));
                spawnFloatingText(rBuf, g_Player.x, g_Player.y + 1.8f, 0.35f, 1.0f, 0.45f);
            }
        } else {
            g_Player.regenTimer = 0.0f;
        }
    }

    if (g_Player.isDead) {
        g_Player.deathTimer += dt;
        g_Player.vx = 0.0f;
        if (!g_Player.isGrounded) {
            g_Player.vy -= 34.0f * dt;
            g_Player.y += g_Player.vy * dt;
        }
        return;
    }

    if (isCreativeFlying()) {
        updateFlightPhysics(dt);
        return;
    }
    float targetVX = 0.0f;
    if (g_Keys['A'] || g_Keys[VK_LEFT]) {
        targetVX -= g_Player.moveSpeed;
        g_Player.isFacingRight = false;
        g_TutMovedLeft = true;
    }
    if (g_Keys['D'] || g_Keys[VK_RIGHT]) {
        targetVX += g_Player.moveSpeed;
        g_Player.isFacingRight = true;
        g_TutMovedRight = true;
    }

    float accel = g_Player.isGrounded ? 45.0f : 25.0f;
    g_Player.vx += (targetVX - g_Player.vx) * accel * dt;

    g_Player.vy -= 34.0f * dt;
    if (g_Player.vy < -28.0f) g_Player.vy = -28.0f;
    if (g_Player.vy < 0.0f) {
        g_Player.fallVelocity = g_Player.vy;
    }

    if ((g_Keys[VK_SPACE] || g_Keys['W'] || g_Keys[VK_UP]) && g_Player.isGrounded) {
        g_Player.vy = g_Player.jumpSpeed;
        g_Player.isGrounded = false;
        g_TutJumped = true;
        playSound(SFX_JUMP);
    }

    if (g_TutorialStep == TUTORIAL_MOVE_JUMP && g_TutMovedLeft && g_TutMovedRight && g_TutJumped) {
        g_TutorialStep = TUTORIAL_MINE;
        playSound(SFX_CRAFT);
        showNotification(LOC("任务 1 已完成！按住鼠标左键挖掘 5 个方块。", "Quest 1 Completed! Now mine 5 blocks with Left-Click."), 0.3f, 0.95f, 0.3f);
    }

    if (fabsf(g_Player.vx) > 0.1f) {
        float dir = g_Player.vx > 0.0f ? 1.0f : -1.0f;
        int checkX = (int)floorf(g_Player.x + dir * (g_Player.width * 0.5f + 0.15f));
        int footY = (int)floorf(g_Player.y);
        int headY = (int)floorf(g_Player.y + g_Player.height);

        if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
            g_Player.y += 18.0f * dt;
            if (g_Player.y > (float)(footY + 1)) g_Player.y = (float)(footY + 1);
        }
    }

    float nextX = g_Player.x + g_Player.vx * dt;
    int minX = (int)floorf(nextX - g_Player.width * 0.5f);
    int maxX = (int)floorf(nextX + g_Player.width * 0.5f);
    int minY = (int)floorf(g_Player.y + 0.1f);
    int maxY = (int)floorf(g_Player.y + g_Player.height - 0.1f);

    for (int y = minY; y <= maxY; y++) {
        if (g_Player.vx > 0.0f && isSolidBlock(maxX, y)) {
            nextX = (float)maxX - g_Player.width * 0.5f - 0.001f;
            g_Player.vx = 0.0f;
            break;
        } else if (g_Player.vx < 0.0f && isSolidBlock(minX, y)) {
            nextX = (float)(minX + 1) + g_Player.width * 0.5f + 0.001f;
            g_Player.vx = 0.0f;
            break;
        }
    }
    g_Player.x = nextX;

    float nextY = g_Player.y + g_Player.vy * dt;
    minX = (int)floorf(g_Player.x - g_Player.width * 0.45f);
    maxX = (int)floorf(g_Player.x + g_Player.width * 0.45f);
    minY = (int)floorf(nextY);
    maxY = (int)floorf(nextY + g_Player.height);

    g_Player.isGrounded = false;
    if (g_Player.vy <= 0.0f) {
        for (int x = minX; x <= maxX; x++) {
            if (isSolidBlock(x, minY)) {
                nextY = (float)(minY + 1);
                g_Player.vy = 0.0f;
                g_Player.isGrounded = true;
                break;
            }
        }
        if (g_Player.isGrounded) {
            // Fall damage precision rebalance (User Request 5)
            float fallDist = g_Player.fallStartY - nextY;
            g_Player.fallStartY = nextY;
            if (fallDist < 8.0f) {
                // Falls under 8 blocks are completely safe (0 damage)
            } else if (getGameMode() != MODE_CREATIVE && !isCreativeFlying()) {
                float dmg = (floorf(fallDist) - 7.0f) * 5.0f;
                if (dmg > 0.0f && !g_Player.isDead) {
                    g_Player.health -= dmg;
                    g_Player.hurtTimer = 0.4f;
                    g_Player.timeSinceCombat = 0.0f;
                    playSound(SFX_HIT);
                    char dmgStr[16];
                    snprintf(dmgStr, sizeof(dmgStr), "-%d", (int)(dmg + 0.5f));
                    spawnFloatingText(dmgStr, g_Player.x, nextY + 1.8f, 1.0f, 0.2f, 0.2f);
                    if (g_Player.health <= 0.0f) {
                        g_Player.health = 0.0f;
                        g_Player.isDead = true;
                        g_Player.deathTimer = 0.0f;
                        playSound(SFX_KILL);
                    }
                }
            }
            g_Player.fallVelocity = 0.0f;
        }
    } else {
        for (int x = minX; x <= maxX; x++) {
            if (isSolidBlock(x, maxY)) {
                nextY = (float)maxY - g_Player.height - 0.001f;
                g_Player.vy = 0.0f;
                break;
            }
        }
    }
    g_Player.y = nextY;

    // Track highest apex while airborne
    if (!g_Player.isGrounded) {
        if (g_Player.y > g_Player.fallStartY) {
            g_Player.fallStartY = g_Player.y;
        }
    }

    static float s_StepSfxTimer = 0.0f;
    if (g_Player.isGrounded && fabsf(g_Player.vx) > 0.4f) {
        g_Player.walkCycle += fabsf(g_Player.vx) * 2.2f * dt;
        s_StepSfxTimer += dt;
        if (s_StepSfxTimer >= 0.35f) {
            playSound(SFX_STEP);
            s_StepSfxTimer = 0.0f;
        }
    } else {
        s_StepSfxTimer = 0.28f;
    }

    // Smooth camera tracking without abrupt lookahead flip (User Request 4)
    g_CamX += (g_Player.x - g_CamX) * 5.0f * dt;
    g_CamY += (g_Player.y + 1.2f - g_CamY) * 5.0f * dt;
    if (g_CamY < 8.0f) g_CamY = 8.0f;

    // 24-minute full day cycle (User Request 3)
    g_WorldTime += dt / 1440.0f;
    if (g_WorldTime >= 1.0f) g_WorldTime -= 1.0f;

    if (g_NotificationTimer > 0.0f) {
        g_NotificationTimer -= dt;
    }

    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (g_FloatingTexts[i].active) {
            g_FloatingTexts[i].life += dt;
            g_FloatingTexts[i].y += 0.8f * dt;
            if (g_FloatingTexts[i].life >= g_FloatingTexts[i].maxLife) {
                g_FloatingTexts[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_Particles[i].active) {
            g_Particles[i].x += g_Particles[i].vx * dt;
            g_Particles[i].y += g_Particles[i].vy * dt;
            g_Particles[i].vy -= 18.0f * dt;
            g_Particles[i].life += dt;
            if (g_Particles[i].life >= g_Particles[i].maxLife) {
                g_Particles[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_DROPPED_ITEMS; i++) {
        if (g_DroppedItems[i].active) {
            g_DroppedItems[i].life += dt;
            g_DroppedItems[i].rotation += 120.0f * dt;

            float dx = g_Player.x - g_DroppedItems[i].x;
            float dy = (g_Player.y + 0.8f) - g_DroppedItems[i].y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < 3.2f) {
                float pullSpeed = 8.0f + (3.2f - dist) * 6.0f;
                g_DroppedItems[i].vx += (dx / dist) * pullSpeed * dt;
                g_DroppedItems[i].vy += (dy / dist) * pullSpeed * dt;

                if (dist < 0.65f) {
                    addToInventory(g_DroppedItems[i].type, g_DroppedItems[i].count);
                    g_DroppedItems[i].active = false;
                    playSound(SFX_PICKUP);
                    continue;
                }
            } else {
                g_DroppedItems[i].vy -= 14.0f * dt;
            }

            g_DroppedItems[i].x += g_DroppedItems[i].vx * dt;
            g_DroppedItems[i].y += g_DroppedItems[i].vy * dt;

            int ix = (int)floorf(g_DroppedItems[i].x);
            int iy = (int)floorf(g_DroppedItems[i].y);
            if (isSolidBlock(ix, iy)) {
                // Requirement 3: Dropped Item Wall Ejection
                // When an item is inside a solid block (isSolidBlock(ix, iy)), detect open adjacent air blocks
                // (iy+1, ix-1, ix+1, iy-1) and push the item out with an impulse into open air so items never get stuck in walls.
                if (!isSolidBlock(ix, iy + 1)) {
                    g_DroppedItems[i].y = (float)(iy + 1);
                    if (g_DroppedItems[i].vy < 0.0f) g_DroppedItems[i].vy = 0.0f;
                    g_DroppedItems[i].vx *= 0.8f;
                } else if (!isSolidBlock(ix - 1, iy)) {
                    g_DroppedItems[i].x = (float)ix - 0.2f;
                    g_DroppedItems[i].vx = -3.0f;
                    g_DroppedItems[i].vy = 2.0f;
                } else if (!isSolidBlock(ix + 1, iy)) {
                    g_DroppedItems[i].x = (float)(ix + 1) + 0.2f;
                    g_DroppedItems[i].vx = 3.0f;
                    g_DroppedItems[i].vy = 2.0f;
                } else if (!isSolidBlock(ix, iy - 1)) {
                    g_DroppedItems[i].y = (float)iy - 0.2f;
                    g_DroppedItems[i].vy = -2.0f;
                } else {
                    g_DroppedItems[i].y = (float)(iy + 1) + 0.1f;
                    g_DroppedItems[i].vy = 3.0f;
                }
            }
        }
    }

    updateMobs(dt, g_Player.x, g_Player.y);
}

static BlockType getMatchingWallType(BlockType bt) {
    switch (bt) {
        case BLOCK_GRASS:
        case BLOCK_DIRT:
        case BLOCK_CLAY:
        case BLOCK_MUD:
            return BLOCK_DIRT;
        case BLOCK_STONE:
        case BLOCK_COAL_ORE:
        case BLOCK_IRON_ORE:
        case BLOCK_GOLD_ORE:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_RUBY_ORE:
        case BLOCK_MOSSY_STONE:
            return BLOCK_STONE;
        case BLOCK_SAND:
        case BLOCK_SANDSTONE:
        case BLOCK_CACTUS:
            return BLOCK_SANDSTONE;
        case BLOCK_SNOW:
        case BLOCK_ICE:
            return BLOCK_ICE;
        case BLOCK_WOOD:
        case BLOCK_LEAVES:
            return BLOCK_WOOD;
        case BLOCK_WOOD_PLANK:
            return BLOCK_WOOD_PLANK;
        case BLOCK_BRICK:
            return BLOCK_BRICK;
        case BLOCK_OBSIDIAN:
        case BLOCK_MAGMA:
        case BLOCK_BEDROCK:
            return BLOCK_OBSIDIAN;
        case BLOCK_GLOW_SHROOM:
            return BLOCK_STONE;
        case BLOCK_GLASS:
            return BLOCK_GLASS;
        case BLOCK_WORKBENCH:
            return BLOCK_WORKBENCH;
        case BLOCK_CHAIR:
            return BLOCK_CHAIR;
        case BLOCK_TABLE:
            return BLOCK_TABLE;
        case BLOCK_DOOR:
        case BLOCK_DOOR_UPPER:
        case BLOCK_DOOR_OPEN_LOWER:
        case BLOCK_DOOR_OPEN_UPPER:
            return BLOCK_DOOR;
        case BLOCK_BED:
            return BLOCK_BED;
        case BLOCK_FURNACE:
            return BLOCK_FURNACE;
        case BLOCK_CHEST:
            return BLOCK_CHEST;
        case BLOCK_BOOKSHELF:
            return BLOCK_BOOKSHELF;
        case BLOCK_CLOCK:
            return BLOCK_CLOCK;
        case BLOCK_CHANDELIER:
            return BLOCK_CHANDELIER;
        default:
            return BLOCK_DIRT;
    }
}

static void screenToWorld(int screenX, int screenY, float* outWorldX, float* outWorldY) {
    float ndcX = ((float)screenX / (float)WINDOW_WIDTH) * 2.0f - 1.0f;
    float ndcY = 1.0f - ((float)screenY / (float)WINDOW_HEIGHT) * 2.0f;

    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float fovY = 55.0f * (float)M_PI / 180.0f;
    float halfH = tanf(fovY * 0.5f) * g_CamDist;
    float halfW = halfH * aspect;

    *outWorldX = g_CamX + ndcX * halfW;
    *outWorldY = g_CamY + ndcY * halfH;
}

static void updateInteraction(float dt) {
    if (g_ChestOpen) {
        float cdx = (float)g_ActiveChestX + 0.5f - g_Player.x;
        float cdy = (float)g_ActiveChestY + 0.5f - g_Player.y;
        if (cdx * cdx + cdy * cdy > 64.0f || g_World[g_ActiveChestX][g_ActiveChestY] != BLOCK_CHEST) {
            g_ChestOpen = false;
        }
    }
    if (g_ChestOpen || isCraftingOpen() || isCreativeInventoryOpen()) {
        g_MiningProgress = 0.0f;
        return;
    }

    updateToolSwing(dt, g_MouseLeft);

    float mouseWX, mouseWY;
    screenToWorld(g_MouseScreenX, g_MouseScreenY, &mouseWX, &mouseWY);

    // Immediately face the target/mouse coordinate when attacking or interacting with mouse
    if (g_MouseLeft || g_MouseRight) {
        g_Player.isFacingRight = (mouseWX >= g_Player.x);
    }

    static float s_AttackCooldown = 0.0f;
    s_AttackCooldown -= dt;
    if (g_MouseLeft && s_AttackCooldown <= 0.0f) {
        int heldItem = g_Hotbar[g_ActiveSlot].type;
        float damage = getToolDamage(heldItem);
        float knockback = getToolKnockback(heldItem);
        if (checkMobAttackEx(mouseWX, mouseWY, damage, knockback, g_Player.x, g_Player.y)) {
            s_AttackCooldown = 0.20f;
            g_MiningProgress = 0.0f;
            g_Player.timeSinceCombat = 0.0f;

            if (getGameMode() == MODE_SURVIVAL) {
                InventorySlot* slot = &g_Hotbar[g_ActiveSlot];
                if (slot->type != BLOCK_AIR && slot->maxDurability > 0 && isToolItem(slot->type)) {
                    slot->durability--;
                    if (slot->durability <= 0) {
                        slot->type = BLOCK_AIR;
                        slot->count = 0;
                        slot->durability = 0;
                        slot->maxDurability = 0;
                        playSound(SFX_BREAK);
                        showNotification(LOC("[工具已损坏！]", "[Tool Broken!]"), 1.0f, 0.35f, 0.35f);
                    }
                }
            }
            return;
        }
    }

    g_TargetX = (int)floorf(mouseWX);
    g_TargetY = (int)floorf(mouseWY);

    float dx = (float)g_TargetX + 0.5f - g_Player.x;
    float dy = (float)g_TargetY + 0.5f - (g_Player.y + 0.9f);
    float dist = sqrtf(dx * dx + dy * dy);

    g_TargetValid = (dist <= REACH_DISTANCE) && (g_TargetX >= 0 && g_TargetX < WORLD_WIDTH && g_TargetY >= 0 && g_TargetY < WORLD_HEIGHT);

    // Line-of-sight occlusion: trace from player eye to target block center
    if (g_TargetValid) {
        int playerBX = (int)floorf(g_Player.x);
        int playerBY = (int)floorf(g_Player.y);

        // Requirement 2: LoS Raycast Relaxation
        // Fix raycast occlusion so it doesn't block mining overhead ceiling blocks or blocks adjacent to the player:
        // (abs(g_TargetX - playerBX) <= 1 && abs(g_TargetY - playerBY) <= 2). Player's own occupied blocks should not occlude the ray.
        if (abs(g_TargetX - playerBX) <= 1 && abs(g_TargetY - playerBY) <= 2) {
            // Adjacent or overhead ceiling blocks: bypass raycast occlusion!
        } else {
            float eyeX = g_Player.x;
            float eyeY = g_Player.y + 1.2f;
            float targetCenterX = (float)g_TargetX + 0.5f;
            float targetCenterY = (float)g_TargetY + 0.5f;
            float rayDx = targetCenterX - eyeX;
            float rayDy = targetCenterY - eyeY;
            float rayDist = sqrtf(rayDx * rayDx + rayDy * rayDy);

            if (rayDist > 0.001f) {
                int minOccX = (int)floorf(g_Player.x - g_Player.width * 0.5f);
                int maxOccX = (int)floorf(g_Player.x + g_Player.width * 0.5f);
                int minOccY = (int)floorf(g_Player.y);
                int maxOccY = (int)floorf(g_Player.y + g_Player.height);

                float step = 0.2f;
                float curDist = step;
                while (curDist < rayDist) {
                    float t = curDist / rayDist;
                    float rx = eyeX + rayDx * t;
                    float ry = eyeY + rayDy * t;
                    int bx = (int)floorf(rx);
                    int by = (int)floorf(ry);
                    if (bx != g_TargetX || by != g_TargetY) {
                        // Player's own occupied blocks should not occlude the ray
                        if (bx >= minOccX && bx <= maxOccX && by >= minOccY && by <= maxOccY) {
                            curDist += step;
                            continue;
                        }
                        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                            if (isSolidBlock(bx, by)) {
                                g_TargetValid = false;
                                break;
                            }
                        }
                    }
                    curDist += step;
                }
            }
        }
    }

    // Turn to face the target/mouse coordinate only when actively swinging/mining or right-clicking
    if (g_MouseLeft || g_MouseRight) {
        g_Player.isFacingRight = (mouseWX >= g_Player.x);
    }

    if (!g_TargetValid) {
        g_MiningProgress = 0.0f;
        return;
    }

    if (g_MouseLeft) {
        BlockType bt = g_World[g_TargetX][g_TargetY];
        if (bt != BLOCK_AIR && (g_BlockDefs[bt].hardness >= 0.0f || isCreativeInstantBreak())) {
            if (g_TargetX != g_LastMiningX || g_TargetY != g_LastMiningY) {
                g_MiningProgress = 0.0f;
                g_LastMiningX = g_TargetX;
                g_LastMiningY = g_TargetY;
            }

            static float s_MineSfxTimer = 0.0f;
            s_MineSfxTimer += dt;
            if (s_MineSfxTimer >= 0.20f) {
                playSound(SFX_MINE);
                s_MineSfxTimer = 0.0f;
            }

            if (isCreativeInstantBreak()) {
                g_MiningProgress = 1.0f;
            } else {
                int heldItem = g_Hotbar[g_ActiveSlot].type;
                float efficiency = getToolEfficiency(heldItem, bt);
                float baseTime = getBlockBaseBreakTime(bt);
                float breakTime = baseTime / efficiency;
                if (breakTime < 0.04f) breakTime = 0.04f;
                g_MiningProgress += dt / breakTime;
            }

            if ((rand() % 4) == 0) {
                spawnParticle(
                    (float)g_TargetX + 0.5f,
                    (float)g_TargetY + 0.5f,
                    g_BlockDefs[bt].r, g_BlockDefs[bt].g, g_BlockDefs[bt].b,
                    ((float)rand() / RAND_MAX - 0.5f) * 3.0f,
                    2.0f + ((float)rand() / RAND_MAX) * 2.0f
                );
            }

            if (g_MiningProgress >= 1.0f) {
                playSound(SFX_BREAK);
                for (int i = 0; i < 12; i++) {
                    spawnParticle(
                        (float)g_TargetX + 0.5f,
                        (float)g_TargetY + 0.5f,
                        g_BlockDefs[bt].r, g_BlockDefs[bt].g, g_BlockDefs[bt].b,
                        ((float)rand() / RAND_MAX - 0.5f) * 5.0f,
                        1.0f + ((float)rand() / RAND_MAX) * 4.0f
                    );
                }

                if (isDoorBlock(bt)) {
                    // Requirement 1: Breaking either half breaks both and drops 1 door item
                    spawnDroppedItem((float)g_TargetX + 0.5f, (float)g_TargetY + 0.5f, BLOCK_DOOR, 1);

                    int otherY = isDoorLower(bt) ? (g_TargetY + 1) : (g_TargetY - 1);
                    if (otherY >= 0 && otherY < WORLD_HEIGHT && isDoorBlock(g_World[g_TargetX][otherY])) {
                        for (int p = 0; p < 8; p++) {
                            spawnParticle(
                                (float)g_TargetX + 0.5f,
                                (float)otherY + 0.5f,
                                g_BlockDefs[BLOCK_DOOR].r, g_BlockDefs[BLOCK_DOOR].g, g_BlockDefs[BLOCK_DOOR].b,
                                ((float)rand() / RAND_MAX - 0.5f) * 4.0f,
                                1.0f + ((float)rand() / RAND_MAX) * 3.0f
                            );
                        }
                        g_World[g_TargetX][otherY] = BLOCK_AIR;
                        recordBlockDelta(g_TargetX, otherY, false, BLOCK_AIR);
                    }
                    g_World[g_TargetX][g_TargetY] = BLOCK_AIR;
                    recordBlockDelta(g_TargetX, g_TargetY, false, BLOCK_AIR);
                } else {
                    if (bt == BLOCK_CHEST) {
                        WorldChest* chest = findChest(g_TargetX, g_TargetY);
                        if (chest) {
                            for (int s = 0; s < CHEST_SLOTS; s++) {
                                if (chest->slots[s].type != BLOCK_AIR && chest->slots[s].count > 0) {
                                    spawnDroppedItem((float)g_TargetX + 0.5f, (float)g_TargetY + 0.5f,
                                                     chest->slots[s].type, chest->slots[s].count);
                                }
                            }
                            removeChest(g_TargetX, g_TargetY);
                        }
                        if (g_ChestOpen && g_ActiveChestX == g_TargetX && g_ActiveChestY == g_TargetY) {
                            g_ChestOpen = false;
                        }
                    } else if (bt == BLOCK_TORCH || bt == BLOCK_CHANDELIER) {
                        setLightExtinguished(g_TargetX, g_TargetY, false);
                    }
                    spawnDroppedItem((float)g_TargetX + 0.5f, (float)g_TargetY + 0.5f, bt, 1);
                    g_World[g_TargetX][g_TargetY] = BLOCK_AIR;
                    recordBlockDelta(g_TargetX, g_TargetY, false, BLOCK_AIR);
                }
                g_MiningProgress = 0.0f;

                if (getGameMode() == MODE_SURVIVAL) {
                    InventorySlot* slot = &g_Hotbar[g_ActiveSlot];
                    if (slot->type != BLOCK_AIR && slot->maxDurability > 0 && isToolItem(slot->type)) {
                        slot->durability--;
                        if (slot->durability <= 0) {
                            slot->type = BLOCK_AIR;
                            slot->count = 0;
                            slot->durability = 0;
                            slot->maxDurability = 0;
                            playSound(SFX_BREAK);
                            showNotification(LOC("[工具已损坏！]", "[Tool Broken!]"), 1.0f, 0.35f, 0.35f);
                        }
                    }
                }

                if (g_TutorialStep == TUTORIAL_MINE) {
                    g_MinedCount++;
                    if (g_MinedCount >= 5) {
                        g_TutorialStep = TUTORIAL_BUILD;
                        playSound(SFX_CRAFT);
                        showNotification(LOC("任务 2 已完成！选中方块 [1-9]，按鼠标右键放置。", "Quest 2 Completed! Select a block [1-9] & Right-Click to place."), 0.3f, 0.95f, 0.3f);
                    }
                }
            }
        }
    } else {
        g_MiningProgress = 0.0f;
    }

    if (g_MouseRight) {
        BlockType clickedBlock = g_World[g_TargetX][g_TargetY];
        if (clickedBlock != BLOCK_AIR) {
            // Requirement 1: Right-click interaction on existing blocks
            if (clickedBlock == BLOCK_WORKBENCH) {
                openCraftingUI(CRAFT_TAB_CRAFTING);
                playSound(SFX_CLICK);
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_FURNACE) {
                openCraftingUI(CRAFT_TAB_CRAFTING);
                playSound(SFX_CLICK);
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_CHEST) {
                if (isCraftingOpen()) closeCraftingUI();
                if (isCreativeInventoryOpen()) toggleCreativeInventory();
                g_ActiveChestX = g_TargetX;
                g_ActiveChestY = g_TargetY;
                getOrCreateChest(g_TargetX, g_TargetY);
                g_ChestOpen = true;
                playSound(SFX_CLICK);
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_BED) {
                float sunAngle = g_WorldTime * (float)M_PI * 2.0f;
                bool isDay = (sinf(sunAngle) > 0.0f);
                if (!isDay) {
                    g_WorldTime = 8.0f / 24.0f;
                    g_Player.health = g_Player.maxHealth;
                    g_Player.hurtTimer = 0.0f;
                    playSound(SFX_CRAFT);
                    showNotification(LOC("[已安稳入睡至清晨 08:00，生命值已恢复！]", "[You slept until 08:00 morning. Health fully restored!]"), 0.3f, 0.95f, 0.4f);
                } else {
                    playSound(SFX_CLICK);
                    showNotification(LOC("[只能在夜间入睡！]", "[You can only sleep at night!]"), 0.95f, 0.7f, 0.2f);
                }
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_CLOCK) {
                float totalHours = g_WorldTime * 24.0f;
                int hour = (int)totalHours % 24;
                int minute = (int)((totalHours - (float)(int)totalHours) * 60.0f) % 60;
                float sunAngle = g_WorldTime * (float)M_PI * 2.0f;
                bool isDay = (sinf(sunAngle) > 0.0f);
                const char* period = isDay ? LOC("白昼", "Day") : LOC("夜晚", "Night");
                char timeMsg[128];
                snprintf(timeMsg, sizeof(timeMsg),
                         LOC("[摆钟报时: 当前时间为 %02d:%02d (%s)]", "[Clock Chime: Current time is %02d:%02d (%s)]"),
                         hour, minute, period);
                showNotification(timeMsg, 1.0f, 0.88f, 0.35f);
                playSound(SFX_CLICK);
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_TORCH) {
                bool ext = !isLightExtinguished(g_TargetX, g_TargetY);
                setLightExtinguished(g_TargetX, g_TargetY, ext);
                playSound(SFX_CLICK);
                if (ext) {
                    showNotification(LOC("[火把已熄灭]", "[Torch Extinguished]"), 0.75f, 0.75f, 0.75f);
                } else {
                    showNotification(LOC("[火把已点燃]", "[Torch Reignited]"), 1.0f, 0.85f, 0.3f);
                }
                g_MouseRight = false;
            } else if (clickedBlock == BLOCK_CHANDELIER) {
                bool ext = !isLightExtinguished(g_TargetX, g_TargetY);
                setLightExtinguished(g_TargetX, g_TargetY, ext);
                playSound(SFX_CLICK);
                if (ext) {
                    showNotification(LOC("[铁艺吊灯已熄灭]", "[Chandelier Extinguished]"), 0.75f, 0.75f, 0.75f);
                } else {
                    showNotification(LOC("[铁艺吊灯已点燃]", "[Chandelier Reignited]"), 1.0f, 0.85f, 0.3f);
                }
                g_MouseRight = false;
            } else if (isDoorBlock(clickedBlock)) {
                // Right-clicking on either half of a door toggles it between CLOSED and OPEN
                int lowX = g_TargetX;
                int lowY = isDoorLower(clickedBlock) ? g_TargetY : (g_TargetY - 1);
                int upX = g_TargetX;
                int upY = lowY + 1;

                bool currentlyOpen = isDoorOpen(clickedBlock);
                if (currentlyOpen) {
                    // Open -> Closed (solid, blocks movement)
                    g_World[lowX][lowY] = BLOCK_DOOR;
                    recordBlockDelta(lowX, lowY, false, BLOCK_DOOR);
                    if (upY >= 0 && upY < WORLD_HEIGHT && isDoorBlock(g_World[upX][upY])) {
                        g_World[upX][upY] = BLOCK_DOOR_UPPER;
                        recordBlockDelta(upX, upY, false, BLOCK_DOOR_UPPER);
                    }
                    showNotification(LOC("[木门已关闭 (实体碰撞)]", "[Door Closed (Solid)]"), 0.85f, 0.70f, 0.40f);
                } else {
                    // Closed -> Open (non-solid, passable)
                    g_World[lowX][lowY] = BLOCK_DOOR_OPEN_LOWER;
                    recordBlockDelta(lowX, lowY, false, BLOCK_DOOR_OPEN_LOWER);
                    if (upY >= 0 && upY < WORLD_HEIGHT && isDoorBlock(g_World[upX][upY])) {
                        g_World[upX][upY] = BLOCK_DOOR_OPEN_UPPER;
                        recordBlockDelta(upX, upY, false, BLOCK_DOOR_OPEN_UPPER);
                    }
                    showNotification(LOC("[木门已开启 (自由通行)]", "[Door Opened (Passable)]"), 0.40f, 0.95f, 0.45f);
                }
                playSound(SFX_CLICK);
                g_MouseRight = false;
            }
        } else {
            // Target is BLOCK_AIR: Place held block/item
            InventorySlot* slot = &g_Hotbar[g_ActiveSlot];
            if (slot->type != BLOCK_AIR && !isToolItem(slot->type) && slot->count > 0) {
                BlockType placedType = slot->type;
                if (placedType == BLOCK_DOOR) {
                    // Requirement 1: 2-Block Door Placement
                    // Placing a door places 2 vertical blocks (lower and upper) if both are air and supported.
                    int lowX = g_TargetX;
                    int lowY = g_TargetY;
                    int upX = g_TargetX;
                    int upY = g_TargetY + 1;

                    // Allow clicking upper air block in a 2-high doorway opening if supported below
                    if (upY >= WORLD_HEIGHT || g_World[lowX][upY] != BLOCK_AIR) {
                        if (lowY - 1 >= 0 && g_World[lowX][lowY - 1] == BLOCK_AIR && (lowY - 2 >= 0 && isSolidBlock(lowX, lowY - 2))) {
                            lowY = g_TargetY - 1;
                            upY = g_TargetY;
                        }
                    }

                    if (lowY >= 0 && upY < WORLD_HEIGHT &&
                        g_World[lowX][lowY] == BLOCK_AIR && g_World[upX][upY] == BLOCK_AIR) {
                        
                        bool hasSupport = (lowY - 1 >= 0 && isSolidBlock(lowX, lowY - 1)) ||
                                          (g_WallWorld[lowX][lowY] != BLOCK_AIR) ||
                                          (upY + 1 < WORLD_HEIGHT && isSolidBlock(lowX, upY + 1)) ||
                                          isSolidBlock(lowX + 1, lowY) || isSolidBlock(lowX - 1, lowY);

                        float pLeft = g_Player.x - g_Player.width * 0.5f;
                        float pRight = g_Player.x + g_Player.width * 0.5f;
                        float pBottom = g_Player.y;
                        float pTop = g_Player.y + g_Player.height;
                        bool intersect = (pRight > (float)lowX && pLeft < (float)(lowX + 1) &&
                                          pTop > (float)lowY && pBottom < (float)(upY + 1));

                        if (hasSupport && !intersect) {
                            g_World[lowX][lowY] = BLOCK_DOOR;
                            g_World[upX][upY] = BLOCK_DOOR_UPPER;
                            recordBlockDelta(lowX, lowY, false, BLOCK_DOOR);
                            recordBlockDelta(upX, upY, false, BLOCK_DOOR_UPPER);
                            playSound(SFX_PLACE);

                            if (g_WallWorld[lowX][lowY] == BLOCK_AIR) {
                                BlockType matchWall = getMatchingWallType(BLOCK_WOOD_PLANK);
                                if (matchWall != BLOCK_AIR) {
                                    g_WallWorld[lowX][lowY] = matchWall;
                                    g_WallWorld[upX][upY] = matchWall;
                                    recordBlockDelta(lowX, lowY, true, matchWall);
                                    recordBlockDelta(upX, upY, true, matchWall);
                                }
                            }

                            if (!isCreativeInfinitePlace()) {
                                slot->count--;
                                if (slot->count <= 0) slot->type = BLOCK_AIR;
                            }
                            g_MouseRight = false;
                        }
                    }
                } else {
                    // Regular 1-block placement
                    bool hasSupport = (g_WallWorld[g_TargetX][g_TargetY] != BLOCK_AIR) ||
                                      isSolidBlock(g_TargetX + 1, g_TargetY) ||
                                      isSolidBlock(g_TargetX - 1, g_TargetY) ||
                                      isSolidBlock(g_TargetX, g_TargetY + 1) ||
                                      isSolidBlock(g_TargetX, g_TargetY - 1);

                    float pLeft = g_Player.x - g_Player.width * 0.5f;
                    float pRight = g_Player.x + g_Player.width * 0.5f;
                    float pBottom = g_Player.y;
                    float pTop = g_Player.y + g_Player.height;
                    bool intersect = (pRight > (float)g_TargetX && pLeft < (float)(g_TargetX + 1) &&
                                      pTop > (float)g_TargetY && pBottom < (float)(g_TargetY + 1));

                    if (hasSupport && (!g_BlockDefs[slot->type].isSolid || !intersect)) {
                        BlockType placedType = slot->type;
                        g_World[g_TargetX][g_TargetY] = placedType;
                        recordBlockDelta(g_TargetX, g_TargetY, false, placedType);
                        playSound(SFX_PLACE);

                        if (placedType == BLOCK_CHEST) {
                            getOrCreateChest(g_TargetX, g_TargetY);
                        } else if (placedType == BLOCK_TORCH || placedType == BLOCK_CHANDELIER) {
                            setLightExtinguished(g_TargetX, g_TargetY, false);
                        }

                        if (g_WallWorld[g_TargetX][g_TargetY] == BLOCK_AIR) {
                            BlockType matchWall = getMatchingWallType(placedType);
                            if (matchWall != BLOCK_AIR) {
                                g_WallWorld[g_TargetX][g_TargetY] = matchWall;
                                recordBlockDelta(g_TargetX, g_TargetY, true, matchWall);
                            }
                        }

                        if (!isCreativeInfinitePlace()) {
                            slot->count--;
                            if (slot->count <= 0) slot->type = BLOCK_AIR;
                        }
                        g_MouseRight = false;

                        if (g_TutorialStep == TUTORIAL_BUILD) {
                            g_TutorialStep = TUTORIAL_TORCH;
                            playSound(SFX_CRAFT);
                            showNotification(LOC("任务 3 已完成！选定火把 [5] 并放置以照亮洞穴。", "Quest 3 Completed! Select Torch [5] and place to light dark caves."), 0.3f, 0.95f, 0.3f);
                        } else if (g_TutorialStep == TUTORIAL_TORCH && (placedType == BLOCK_TORCH || placedType == BLOCK_GLOW_SHROOM)) {
                            g_TutorialStep = TUTORIAL_EXPLORE;
                            playSound(SFX_CRAFT);
                            showNotification(LOC("任务 4 已完成！按 [F5] 快速保存你的世界！", "Quest 4 Completed! Press [F5] to Quick Save your game!"), 0.3f, 0.95f, 0.3f);
                        }
                    }
                }
            }
        }
    }
}

static void setPerspective(float fovy, float aspect, float zNear, float zFar) {
    float f = 1.0f / tanf(fovy * (float)M_PI / 360.0f);
    GLfloat m[16] = {0};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    glMultMatrixf(m);
}

typedef enum {
    TORCH_DIR_FLOOR = 0,
    TORCH_DIR_LEFT_WALL,
    TORCH_DIR_RIGHT_WALL,
    TORCH_DIR_BACK_WALL
} TorchOrientation;

static inline TorchOrientation getTorchOrientation(int x, int y) {
    if (y - 1 >= 0 && isSolidBlock(x, y - 1)) return TORCH_DIR_FLOOR;
    if (x - 1 >= 0 && isSolidBlock(x - 1, y)) return TORCH_DIR_LEFT_WALL;
    if (x + 1 < WORLD_WIDTH && isSolidBlock(x + 1, y)) return TORCH_DIR_RIGHT_WALL;
    return TORCH_DIR_BACK_WALL;
}

static void renderTorch25D(float x, float y, int orientation, float lightFactor) {
    float u0, v0, u1, v1;
    int texIdx = GetBlockFaceTextureIndex(BLOCK_WOOD, 0);
    GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);

    float sx0 = x + 0.43f, sx1 = x + 0.57f;
    float sy0 = y, sy1 = y + 0.58f;
    float sz = 0.01f;

    if (orientation == TORCH_DIR_LEFT_WALL) {
        sx0 = x + 0.04f; sx1 = x + 0.36f;
        sy0 = y + 0.22f; sy1 = y + 0.68f;
    } else if (orientation == TORCH_DIR_RIGHT_WALL) {
        sx0 = x + 0.96f; sx1 = x + 0.64f;
        sy0 = y + 0.22f; sy1 = y + 0.68f;
    } else if (orientation == TORCH_DIR_BACK_WALL) {
        sx0 = x + 0.43f; sx1 = x + 0.57f;
        sy0 = y + 0.15f; sy1 = y + 0.68f;
    }

    // Wooden Stick
    glColor3f(0.55f * lightFactor, 0.35f * lightFactor, 0.20f * lightFactor);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(sx0, sy0, sz);
    glTexCoord2f(u1, v1); glVertex3f(sx1, sy0, sz);
    glTexCoord2f(u1, v0); glVertex3f(sx1, sy1, sz);
    glTexCoord2f(u0, v0); glVertex3f(sx0, sy1, sz);
    glEnd();

    // If extinguished, don't draw flame!
    if (isLightExtinguished((int)x, (int)y)) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.18f * lightFactor, 0.18f * lightFactor, 0.18f * lightFactor);
        glBegin(GL_QUADS);
        glVertex3f(sx0 + 0.01f, sy1 - 0.04f, sz + 0.01f);
        glVertex3f(sx1 - 0.01f, sy1 - 0.04f, sz + 0.01f);
        glVertex3f(sx1 - 0.01f, sy1 + 0.02f, sz + 0.01f);
        glVertex3f(sx0 + 0.01f, sy1 + 0.02f, sz + 0.01f);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        return;
    }

    // Animated Flame
    float fx = (sx0 + sx1) * 0.5f;
    float fy = sy1 + 0.04f;
    float flicker = sinf(g_WorldTime * 80.0f + x * 2.7f + y * 1.9f) * 0.03f;
    float flameW = 0.12f + flicker;
    float flameH = 0.20f + flicker;

    glDisable(GL_TEXTURE_2D);

    // Warm Orange Flame Glow Outer
    glColor4f(1.0f, 0.52f, 0.12f, 0.60f);
    glBegin(GL_QUADS);
    glVertex3f(fx - flameW * 1.4f, fy - flameH * 0.4f, sz + 0.02f);
    glVertex3f(fx + flameW * 1.4f, fy - flameH * 0.4f, sz + 0.02f);
    glVertex3f(fx + flameW * 1.4f, fy + flameH * 1.2f, sz + 0.02f);
    glVertex3f(fx - flameW * 1.4f, fy + flameH * 1.2f, sz + 0.02f);
    glEnd();

    // Bright Golden Inner Flame Core
    glColor4f(1.0f, 0.95f, 0.35f, 0.95f);
    glBegin(GL_QUADS);
    glVertex3f(fx - flameW * 0.65f, fy - flameH * 0.2f, sz + 0.03f);
    glVertex3f(fx + flameW * 0.65f, fy - flameH * 0.2f, sz + 0.03f);
    glVertex3f(fx + flameW * 0.65f, fy + flameH * 0.85f, sz + 0.03f);
    glVertex3f(fx - flameW * 0.65f, fy + flameH * 0.85f, sz + 0.03f);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

static void renderGlowShroom25D(float x, float y, float lightFactor) {
    float u0, v0, u1, v1;
    int texIdx = GetBlockFaceTextureIndex(BLOCK_GLOW_SHROOM, 0);
    GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);

    glColor3f(0.6f * lightFactor, 0.75f * lightFactor, 0.8f * lightFactor);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x + 0.42f, y, 0.01f);
    glTexCoord2f(u1, v1); glVertex3f(x + 0.58f, y, 0.01f);
    glTexCoord2f(u1, v0); glVertex3f(x + 0.58f, y + 0.35f, 0.01f);
    glTexCoord2f(u0, v0); glVertex3f(x + 0.42f, y + 0.35f, 0.01f);
    glEnd();

    float capL = 1.15f;
    glColor3f(0.2f * capL, 0.9f * capL, 0.98f * capL);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x + 0.18f, y + 0.30f, 0.02f);
    glTexCoord2f(u1, v1); glVertex3f(x + 0.82f, y + 0.30f, 0.02f);
    glTexCoord2f(u1, v0); glVertex3f(x + 0.72f, y + 0.65f, 0.02f);
    glTexCoord2f(u0, v0); glVertex3f(x + 0.28f, y + 0.65f, 0.02f);
    glEnd();
}

static void renderCactus25D(float x, float y, float lightFactor) {
    float u0, v0, u1, v1;
    int texIdx = GetBlockFaceTextureIndex(BLOCK_CACTUS, 0);
    GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);

    glColor3f(lightFactor, lightFactor, lightFactor);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x + 0.25f, y, 0.01f);
    glTexCoord2f(u1, v1); glVertex3f(x + 0.75f, y, 0.01f);
    glTexCoord2f(u1, v0); glVertex3f(x + 0.75f, y + 1.0f, 0.01f);
    glTexCoord2f(u0, v0); glVertex3f(x + 0.25f, y + 1.0f, 0.01f);
    glEnd();
}
static void renderCube25D(float x, float y, int blockType, bool topCull, bool btmCull, bool leftCull, bool rightCull, float lightFactor) {
    if (blockType == BLOCK_TORCH) {
        int ori = (int)getTorchOrientation((int)x, (int)y);
        renderTorch25D(x, y, ori, lightFactor);
        return;
    }
    if (blockType == BLOCK_GLOW_SHROOM) {
        renderGlowShroom25D(x, y, lightFactor);
        return;
    }
    if (blockType == BLOCK_CACTUS) {
        renderCactus25D(x, y, lightFactor);
        return;
    }

    if (blockType == BLOCK_DOOR || blockType == BLOCK_DOOR_OPEN_LOWER) {
        topCull = true;
    }
    if (blockType == BLOCK_DOOR_UPPER || blockType == BLOCK_DOOR_OPEN_UPPER) {
        btmCull = true;
    }
    if (blockType == BLOCK_DOOR_OPEN_LOWER || blockType == BLOCK_DOOR_OPEN_UPPER) {
        topCull = true;
        btmCull = true;
    }

    float u0, v0, u1, v1;
    float zFront = (blockType == BLOCK_WOOD || blockType == BLOCK_CHAIR || blockType == BLOCK_CHANDELIER) ? -0.15f : 0.0f;
    float zBack = (blockType == BLOCK_WOOD || blockType == BLOCK_CHAIR || blockType == BLOCK_CHANDELIER) ? -0.55f : -BLOCK_DEPTH;

    // Front Face (Z = 0)
    int frontTex = GetBlockFaceTextureIndex(blockType, 0);
    GetAtlasTileUV(frontTex, &u0, &v0, &u1, &v1);
    glColor3f(lightFactor, lightFactor, lightFactor);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex3f(x, y, zFront);
    glTexCoord2f(u1, v0); glVertex3f(x + 1.0f, y, zFront);
    glTexCoord2f(u1, v1); glVertex3f(x + 1.0f, y + 1.0f, zFront);
    glTexCoord2f(u0, v1); glVertex3f(x, y + 1.0f, zFront);
    glEnd();

    // Top Bevel Face (+Y)
    if (!topCull) {
        int topTex = GetBlockFaceTextureIndex(blockType, 1);
        GetAtlasTileUV(topTex, &u0, &v0, &u1, &v1);
        float topLight = lightFactor * 1.15f;
        if (topLight > 1.0f) topLight = 1.0f;
        glColor3f(topLight, topLight, topLight);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x, y + 1.0f, zFront);
        glTexCoord2f(u1, v0); glVertex3f(x + 1.0f, y + 1.0f, zFront);
        glTexCoord2f(u1, v1); glVertex3f(x + 1.0f, y + 1.0f, zBack);
        glTexCoord2f(u0, v1); glVertex3f(x, y + 1.0f, zBack);
        glEnd();
    }

    // Bottom Bevel Face (-Y)
    if (!btmCull) {
        int btmTex = GetBlockFaceTextureIndex(blockType, 2);
        GetAtlasTileUV(btmTex, &u0, &v0, &u1, &v1);
        float btmLight = lightFactor * 0.65f;
        glColor3f(btmLight, btmLight, btmLight);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x, y, zBack);
        glTexCoord2f(u1, v0); glVertex3f(x + 1.0f, y, zBack);
        glTexCoord2f(u1, v1); glVertex3f(x + 1.0f, y, zFront);
        glTexCoord2f(u0, v1); glVertex3f(x, y, zFront);
        glEnd();
    }

    // Left Bevel Face (-X)
    if (!leftCull) {
        int sideTex = GetBlockFaceTextureIndex(blockType, 3);
        GetAtlasTileUV(sideTex, &u0, &v0, &u1, &v1);
        float sideLight = lightFactor * 0.80f;
        glColor3f(sideLight, sideLight, sideLight);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x, y, zBack);
        glTexCoord2f(u1, v0); glVertex3f(x, y, zFront);
        glTexCoord2f(u1, v1); glVertex3f(x, y + 1.0f, zFront);
        glTexCoord2f(u0, v1); glVertex3f(x, y + 1.0f, zBack);
        glEnd();
    }

    // Right Bevel Face (+X)
    if (!rightCull) {
        int sideTex = GetBlockFaceTextureIndex(blockType, 4);
        GetAtlasTileUV(sideTex, &u0, &v0, &u1, &v1);
        float sideLight = lightFactor * 0.90f;
        glColor3f(sideLight, sideLight, sideLight);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(x + 1.0f, y, zFront);
        glTexCoord2f(u1, v0); glVertex3f(x + 1.0f, y, zBack);
        glTexCoord2f(u1, v1); glVertex3f(x + 1.0f, y + 1.0f, zBack);
        glTexCoord2f(u0, v1); glVertex3f(x + 1.0f, y + 1.0f, zFront);
        glEnd();
    }
}

static void renderBackgroundParallax(void) {
    glDisable(GL_TEXTURE_2D);

    float skyR, skyG, skyB;
    if (g_WorldTime < 0.25f) {
        float t = g_WorldTime / 0.25f;
        skyR = 0.85f * (1.0f - t) + 0.45f * t;
        skyG = 0.55f * (1.0f - t) + 0.72f * t;
        skyB = 0.35f * (1.0f - t) + 0.95f * t;
    } else if (g_WorldTime < 0.50f) {
        float t = (g_WorldTime - 0.25f) / 0.25f;
        skyR = 0.45f * (1.0f - t) + 0.90f * t;
        skyG = 0.72f * (1.0f - t) + 0.45f * t;
        skyB = 0.95f * (1.0f - t) + 0.30f * t;
    } else if (g_WorldTime < 0.75f) {
        float t = (g_WorldTime - 0.50f) / 0.25f;
        skyR = 0.90f * (1.0f - t) + 0.05f * t;
        skyG = 0.45f * (1.0f - t) + 0.08f * t;
        skyB = 0.30f * (1.0f - t) + 0.18f * t;
    } else {
        float t = (g_WorldTime - 0.75f) / 0.25f;
        skyR = 0.05f * (1.0f - t) + 0.85f * t;
        skyG = 0.08f * (1.0f - t) + 0.55f * t;
        skyB = 0.18f * (1.0f - t) + 0.35f * t;
    }
    glClearColor(skyR, skyG, skyB, 1.0f);

    glPushMatrix();
    glTranslatef(g_CamX * 0.88f, 75.0f, -40.0f);
    glColor3f(skyR * 0.7f, skyG * 0.7f, skyB * 0.8f);
    glBegin(GL_TRIANGLES);
    for (int i = -10; i <= 10; i++) {
        float mx = (float)i * 45.0f;
        glVertex3f(mx - 30.0f, -20.0f, 0.0f);
        glVertex3f(mx, 28.0f, 0.0f);
        glVertex3f(mx + 30.0f, -20.0f, 0.0f);
    }
    glEnd();
    glPopMatrix();

    glPushMatrix();
    float cloudShift = g_WorldTime * 200.0f;
    glTranslatef(g_CamX * 0.3f + cloudShift, 100.0f, -20.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
    glBegin(GL_QUADS);
    for (int i = -8; i <= 8; i++) {
        float cx = (float)i * 55.0f;
        glVertex3f(cx, 0.0f, 0.0f);
        glVertex3f(cx + 22.0f, 0.0f, 0.0f);
        glVertex3f(cx + 22.0f, 4.5f, 0.0f);
        glVertex3f(cx, 4.5f, 0.0f);
    }
    glEnd();
    glPopMatrix();
}

static void renderWorld25D(void) {
    int startX = (int)floorf(g_CamX - 22.0f);
    int endX   = (int)ceilf(g_CamX + 22.0f);
    int startY = (int)floorf(g_CamY - 14.0f);
    int endY   = (int)ceilf(g_CamY + 14.0f);

    if (startX < 0) startX = 0;
    if (endX >= WORLD_WIDTH) endX = WORLD_WIDTH - 1;
    if (startY < 0) startY = 0;
    if (endY >= WORLD_HEIGHT) endY = WORLD_HEIGHT - 1;

    float sunAngle = g_WorldTime * (float)M_PI * 2.0f;
    float daylight = sinf(sunAngle);
    if (daylight < 0.005f) daylight = 0.005f;

    typedef struct {
        float x, y;
        float radius;
        float r, g, b;
        float intensity;
    } PointLight;

    PointLight localLights[64];
    int numLocalLights = 0;

    // Player personal sight:
    // Only if actively holding a light source (BLOCK_TORCH, BLOCK_GLOW_SHROOM, BLOCK_CHANDELIER),
    // add a player dynamic light source (radius 8.5f).
    // If empty-handed or holding non-torch, do not add a bright player light bubble
    // (only radius 0.6f intensity 0.04f so you can barely see your hands).
    int heldItem = g_Hotbar[g_ActiveSlot].type;
    if (heldItem == BLOCK_TORCH || heldItem == BLOCK_CHANDELIER) {
        localLights[numLocalLights++] = (PointLight){
            g_Player.x, g_Player.y + 1.0f,
            8.5f,
            1.0f, 0.90f, 0.60f,
            1.20f
        };
    } else if (heldItem == BLOCK_GLOW_SHROOM) {
        localLights[numLocalLights++] = (PointLight){
            g_Player.x, g_Player.y + 1.0f,
            8.5f,
            0.30f, 0.88f, 1.0f,
            1.20f
        };
    } else {
        localLights[numLocalLights++] = (PointLight){
            g_Player.x, g_Player.y + 1.0f,
            0.6f,
            0.80f, 0.80f, 0.85f,
            0.04f
        };
    }

    int lStartX = (startX - 9 < 0) ? 0 : startX - 9;
    int lEndX   = (endX + 9 >= WORLD_WIDTH) ? WORLD_WIDTH - 1 : endX + 9;
    int lStartY = (startY - 9 < 0) ? 0 : startY - 9;
    int lEndY   = (endY + 9 >= WORLD_HEIGHT) ? WORLD_HEIGHT - 1 : endY + 9;

    for (int lx = lStartX; lx <= lEndX && numLocalLights < 60; lx++) {
        for (int ly = lStartY; ly <= lEndY && numLocalLights < 60; ly++) {
            BlockType bt = g_World[lx][ly];
            if (bt != BLOCK_AIR && g_BlockDefs[bt].emitLight) {
                if ((bt == BLOCK_TORCH || bt == BLOCK_CHANDELIER) && isLightExtinguished(lx, ly)) {
                    continue;
                }
                float lr = 1.0f, lg = 0.90f, lb = 0.60f;
                float rad = 8.5f;
                if (bt == BLOCK_MAGMA) { lr = 1.0f; lg = 0.35f; lb = 0.10f; rad = 6.0f; }
                else if (bt == BLOCK_GLOW_SHROOM) { lr = 0.30f; lg = 0.88f; lb = 1.0f; rad = 8.5f; }
                else if (bt == BLOCK_TORCH || bt == BLOCK_CHANDELIER) { lr = 1.0f, lg = 0.90f, lb = 0.60f; rad = 8.5f; }
                localLights[numLocalLights++] = (PointLight){
                    (float)lx + 0.5f, (float)ly + 0.5f,
                    rad,
                    lr, lg, lb,
                    1.20f
                };
            }
        }
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_AtlasTex);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.15f);

    // Pass 1: Background Walls (Behind player at Z = WALL_Z = -0.75f)
    for (int x = startX; x <= endX; x++) {
        int sY = (x >= 0 && x < WORLD_WIDTH) ? g_SurfaceHeight[x] : 85;
        for (int y = startY; y <= endY; y++) {
            BlockType wt = g_WallWorld[x][y];
            if (wt != BLOCK_AIR && (g_World[x][y] == BLOCK_AIR || g_BlockDefs[g_World[x][y]].isTransparent)) {
                float baseL = daylight;
                if (y < sY) {
                    float depth = (float)(sY - y);
                    float sunPen = daylight * expf(-depth * 0.25f);
                    float caveAmbient = 0.0f;
                    baseL = (sunPen > caveAmbient) ? sunPen : caveAmbient;
                }
                float rAcc = baseL, gAcc = baseL, bAcc = baseL;
                for (int li = 0; li < numLocalLights; li++) {
                    float ldx = ((float)x + 0.5f) - localLights[li].x;
                    float ldy = ((float)y + 0.5f) - localLights[li].y;
                    float dSq = ldx * ldx + ldy * ldy;
                    float rSq = localLights[li].radius * localLights[li].radius;
                    if (dSq < rSq) {
                        float dist = sqrtf(dSq);
                        float att = (1.0f - dist / localLights[li].radius) * localLights[li].intensity;
                        rAcc += localLights[li].r * att;
                        gAcc += localLights[li].g * att;
                        bAcc += localLights[li].b * att;
                    }
                }
                if (rAcc > 1.0f) rAcc = 1.0f;
                if (gAcc > 1.0f) gAcc = 1.0f;
                if (bAcc > 1.0f) bAcc = 1.0f;

                float wallR = rAcc * 0.60f;
                float wallG = gAcc * 0.58f;
                float wallB = bAcc * 0.62f;

                float u0, v0, u1, v1;
                int texIdx = GetBlockFaceTextureIndex(wt, 0);
                GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);
                glColor3f(wallR, wallG, wallB);
                glBegin(GL_QUADS);
                glTexCoord2f(u0, v1); glVertex3f((float)x, (float)y, WALL_Z);
                glTexCoord2f(u1, v1); glVertex3f((float)(x + 1), (float)y, WALL_Z);
                glTexCoord2f(u1, v0); glVertex3f((float)(x + 1), (float)(y + 1), WALL_Z);
                glTexCoord2f(u0, v0); glVertex3f((float)x, (float)(y + 1), WALL_Z);
                glEnd();
            }
        }
    }

    // Pass 2: Foreground 2.5D Blocks (Front face Z = 0.0f, bevels into Z = -0.45f)
    for (int x = startX; x <= endX; x++) {
        int sY = (x >= 0 && x < WORLD_WIDTH) ? g_SurfaceHeight[x] : 85;
        for (int y = startY; y <= endY; y++) {
            BlockType bt = g_World[x][y];
            if (bt == BLOCK_AIR) continue;

            BlockDef* def = &g_BlockDefs[bt];

            bool topCull   = (y + 1 < WORLD_HEIGHT) && isSolidBlock(x, y + 1) && !g_BlockDefs[g_World[x][y + 1]].isTransparent;
            bool btmCull   = (y - 1 >= 0)           && isSolidBlock(x, y - 1) && !g_BlockDefs[g_World[x][y - 1]].isTransparent;
            bool leftCull  = (x - 1 >= 0)           && isSolidBlock(x - 1, y) && !g_BlockDefs[g_World[x - 1][y]].isTransparent;
            bool rightCull = (x + 1 < WORLD_WIDTH)  && isSolidBlock(x + 1, y) && !g_BlockDefs[g_World[x + 1][y]].isTransparent;

            float lightFactor;
            if (def->emitLight && !((bt == BLOCK_TORCH || bt == BLOCK_CHANDELIER) && isLightExtinguished(x, y))) {
                lightFactor = 1.15f;
            } else {
                float baseL = daylight;
                if (y < sY) {
                    float depth = (float)(sY - y);
                    float sunPen = daylight * expf(-depth * 0.25f);
                    float caveAmbient = 0.0f;
                    baseL = (sunPen > caveAmbient) ? sunPen : caveAmbient;
                }
                float rAcc = baseL, gAcc = baseL, bAcc = baseL;
                for (int li = 0; li < numLocalLights; li++) {
                    float ldx = ((float)x + 0.5f) - localLights[li].x;
                    float ldy = ((float)y + 0.5f) - localLights[li].y;
                    float dSq = ldx * ldx + ldy * ldy;
                    float rSq = localLights[li].radius * localLights[li].radius;
                    if (dSq < rSq) {
                        float dist = sqrtf(dSq);
                        float att = (1.0f - dist / localLights[li].radius) * localLights[li].intensity;
                        rAcc += localLights[li].r * att;
                        gAcc += localLights[li].g * att;
                        bAcc += localLights[li].b * att;
                    }
                }
                lightFactor = (rAcc + gAcc + bAcc) * 0.3333f;
                if (lightFactor > 1.0f) lightFactor = 1.0f;
                if (lightFactor < 0.005f) lightFactor = 0.005f;
            }

            renderCube25D((float)x, (float)y, bt, topCull, btmCull, leftCull, rightCull, lightFactor);
        }
    }

    // Pass 3: Dropped Items (Z = 0.0f)
    for (int i = 0; i < MAX_DROPPED_ITEMS; i++) {
        if (g_DroppedItems[i].active) {
            glPushMatrix();
            glTranslatef(g_DroppedItems[i].x, g_DroppedItems[i].y, 0.0f);
            glRotatef(g_DroppedItems[i].rotation, 0.0f, 1.0f, 0.0f);
            glScalef(0.38f, 0.38f, 0.38f);
            renderCube25D(-0.5f, -0.5f, g_DroppedItems[i].type, false, false, false, false, 1.15f);
            glPopMatrix();
        }
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);

    // Pass 4: Particles (Z = 0.02f)
    glBegin(GL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_Particles[i].active) {
            glColor3f(g_Particles[i].r, g_Particles[i].g, g_Particles[i].b);
            float s = g_Particles[i].size;
            glVertex3f(g_Particles[i].x - s, g_Particles[i].y - s, 0.02f);
            glVertex3f(g_Particles[i].x + s, g_Particles[i].y - s, 0.02f);
            glVertex3f(g_Particles[i].x + s, g_Particles[i].y + s, 0.02f);
            glVertex3f(g_Particles[i].x - s, g_Particles[i].y + s, 0.02f);
        }
    }
    glEnd();

    // Pass 5: 2.5D Animated Player Character (Z = 0.0f ~ 0.03f, in front of background wall)
    // Pass 5: 2.5D Animated Player Character with Rich Detail (Z = -0.04f ~ 0.04f)
    glPushMatrix();
    glTranslatef(g_Player.x, g_Player.y, 0.0f);
    if (!g_Player.isFacingRight) glScalef(-1.0f, 1.0f, 1.0f);

    float sinWalk = sinf(g_Player.walkCycle);

    // 0. Visual Armor Overlay Palettes
    ArmorMaterialPalette helmetPal, chestPal, legsPal, bootsPal;
    bool hasHelmet = (g_EquippedArmor[ARMOR_SLOT_HELMET] != 0) &&
                     getArmorMaterialPalette(g_EquippedArmor[ARMOR_SLOT_HELMET], &helmetPal);
    bool hasChestplate = (g_EquippedArmor[ARMOR_SLOT_CHEST] != 0) &&
                         getArmorMaterialPalette(g_EquippedArmor[ARMOR_SLOT_CHEST], &chestPal);
    bool hasLeggings = (g_EquippedArmor[ARMOR_SLOT_LEGS] != 0) &&
                       getArmorMaterialPalette(g_EquippedArmor[ARMOR_SLOT_LEGS], &legsPal);
    bool hasBoots = (g_EquippedArmor[ARMOR_SLOT_BOOTS] != 0) &&
                    getArmorMaterialPalette(g_EquippedArmor[ARMOR_SLOT_BOOTS], &bootsPal);

    // 1. Back Arm / Off-hand (Z = -0.04f, behind torso)
    float backArmAngle = g_ToolIsSwinging ? (-g_ToolSwingTimer * 25.0f) : (-sinWalk * 18.0f);
    glPushMatrix();
    glTranslatef(-0.14f, 1.05f, -0.04f);
    glRotatef(backArmAngle, 0.0f, 0.0f, 1.0f);
    if (hasChestplate) {
        glColor3f(chestPal.r * 0.75f, chestPal.g * 0.75f, chestPal.b * 0.75f); // Armored rear sleeve
    } else {
        glColor3f(0.14f, 0.30f, 0.60f); // Shaded rear sleeve
    }
    glBegin(GL_QUADS);
    glVertex3f(-0.07f, -0.28f, 0.0f);
    glVertex3f( 0.07f, -0.28f, 0.0f);
    glVertex3f( 0.07f,  0.02f, 0.0f);
    glVertex3f(-0.07f,  0.02f, 0.0f);
    glEnd();
    if (hasChestplate) {
        glColor3f(chestPal.hr * 0.75f, chestPal.hg * 0.75f, chestPal.hb * 0.75f);
        glBegin(GL_QUADS);
        glVertex3f(-0.08f, -0.28f, 0.005f);
        glVertex3f( 0.08f, -0.28f, 0.005f);
        glVertex3f( 0.08f, -0.24f, 0.005f);
        glVertex3f(-0.08f, -0.24f, 0.005f);
        glEnd();
    }
    glColor3f(0.82f, 0.68f, 0.56f); // Shaded rear hand
    glBegin(GL_QUADS);
    glVertex3f(-0.06f, -0.38f, 0.0f);
    glVertex3f( 0.06f, -0.38f, 0.0f);
    glVertex3f( 0.06f, -0.28f, 0.0f);
    glVertex3f(-0.06f, -0.28f, 0.0f);
    glEnd();
    glPopMatrix();

    // 2. Legs & Exploration Boots
    float legAngleL = g_Player.isGrounded ? sinWalk * 0.18f : 0.15f;
    float legAngleR = g_Player.isGrounded ? -sinWalk * 0.18f : -0.15f;

    // Left Leg (Trousers + Leather Boot)
    glColor3f(0.16f, 0.20f, 0.38f);
    glBegin(GL_QUADS);
    glVertex3f(-0.22f + legAngleL, 0.12f, 0.0f);
    glVertex3f(-0.05f + legAngleL, 0.12f, 0.0f);
    glVertex3f(-0.05f, 0.45f, 0.0f);
    glVertex3f(-0.22f, 0.45f, 0.0f);
    glEnd();
    glColor3f(0.35f, 0.20f, 0.10f); // Boot Leather
    glBegin(GL_QUADS);
    glVertex3f(-0.23f + legAngleL, 0.03f, 0.01f);
    glVertex3f(-0.04f + legAngleL, 0.03f, 0.01f);
    glVertex3f(-0.04f + legAngleL, 0.14f, 0.01f);
    glVertex3f(-0.23f + legAngleL, 0.14f, 0.01f);
    glEnd();
    glColor3f(0.12f, 0.08f, 0.05f); // Boot Sole
    glBegin(GL_QUADS);
    glVertex3f(-0.24f + legAngleL, 0.0f, 0.015f);
    glVertex3f(-0.03f + legAngleL, 0.0f, 0.015f);
    glVertex3f(-0.03f + legAngleL, 0.03f, 0.015f);
    glVertex3f(-0.24f + legAngleL, 0.03f, 0.015f);
    glEnd();

    // Right Leg (Trousers + Leather Boot)
    glColor3f(0.20f, 0.25f, 0.46f);
    glBegin(GL_QUADS);
    glVertex3f( 0.05f + legAngleR, 0.12f, 0.0f);
    glVertex3f( 0.22f + legAngleR, 0.12f, 0.0f);
    glVertex3f( 0.22f, 0.45f, 0.0f);
    glVertex3f( 0.05f, 0.45f, 0.0f);
    glEnd();
    glColor3f(0.40f, 0.24f, 0.12f); // Boot Leather
    glBegin(GL_QUADS);
    glVertex3f( 0.04f + legAngleR, 0.03f, 0.01f);
    glVertex3f( 0.23f + legAngleR, 0.03f, 0.01f);
    glVertex3f( 0.23f + legAngleR, 0.14f, 0.01f);
    glVertex3f( 0.04f + legAngleR, 0.14f, 0.01f);
    glEnd();
    glColor3f(0.12f, 0.08f, 0.05f); // Boot Sole
    glBegin(GL_QUADS);
    glVertex3f( 0.03f + legAngleR, 0.0f, 0.015f);
    glVertex3f( 0.24f + legAngleR, 0.0f, 0.015f);
    glVertex3f( 0.24f + legAngleR, 0.03f, 0.015f);
    glVertex3f( 0.03f + legAngleR, 0.03f, 0.015f);
    glEnd();

    // Visual Armor Overlays: Greaves (Leggings) & Sabatons (Boots)
    if (hasLeggings) {
        // Armored Tassets / Hip Plates (Z = 0.025f)
        glColor3f(legsPal.r, legsPal.g, legsPal.b);
        glBegin(GL_QUADS);
        glVertex3f(-0.31f, 0.38f, 0.025f);
        glVertex3f( 0.31f, 0.38f, 0.025f);
        glVertex3f( 0.31f, 0.48f, 0.025f);
        glVertex3f(-0.31f, 0.48f, 0.025f);
        glEnd();
        // Tasset bottom trim
        glColor3f(legsPal.sr, legsPal.sg, legsPal.sb);
        glBegin(GL_QUADS);
        glVertex3f(-0.31f, 0.36f, 0.026f);
        glVertex3f( 0.31f, 0.36f, 0.026f);
        glVertex3f( 0.31f, 0.38f, 0.026f);
        glVertex3f(-0.31f, 0.38f, 0.026f);
        glEnd();

        // Left Leg Armored Greave (Back Leg, Z = 0.02f)
        glColor3f(legsPal.r * 0.85f, legsPal.g * 0.85f, legsPal.b * 0.85f);
        glBegin(GL_QUADS);
        // Thigh cuisse
        glVertex3f(-0.23f + legAngleL * 0.4f, 0.28f, 0.02f);
        glVertex3f(-0.04f + legAngleL * 0.4f, 0.28f, 0.02f);
        glVertex3f(-0.04f,                    0.44f, 0.02f);
        glVertex3f(-0.23f,                    0.44f, 0.02f);
        // Shin greave
        glVertex3f(-0.23f + legAngleL * 0.9f, 0.13f, 0.02f);
        glVertex3f(-0.04f + legAngleL * 0.9f, 0.13f, 0.02f);
        glVertex3f(-0.04f + legAngleL * 0.4f, 0.28f, 0.02f);
        glVertex3f(-0.23f + legAngleL * 0.4f, 0.28f, 0.02f);
        glEnd();
        // Left Knee Poleyn (Kneecap guard, Z = 0.025f)
        glColor3f(legsPal.hr * 0.85f, legsPal.hg * 0.85f, legsPal.hb * 0.85f);
        glBegin(GL_QUADS);
        glVertex3f(-0.24f + legAngleL * 0.65f, 0.25f, 0.025f);
        glVertex3f(-0.03f + legAngleL * 0.65f, 0.25f, 0.025f);
        glVertex3f(-0.03f + legAngleL * 0.65f, 0.31f, 0.025f);
        glVertex3f(-0.24f + legAngleL * 0.65f, 0.31f, 0.025f);
        glEnd();

        // Right Leg Armored Greave (Front Leg, Z = 0.02f)
        glColor3f(legsPal.r, legsPal.g, legsPal.b);
        glBegin(GL_QUADS);
        // Thigh cuisse
        glVertex3f(0.04f + legAngleR * 0.4f, 0.28f, 0.02f);
        glVertex3f(0.23f + legAngleR * 0.4f, 0.28f, 0.02f);
        glVertex3f(0.23f,                    0.44f, 0.02f);
        glVertex3f(0.04f,                    0.44f, 0.02f);
        // Shin greave
        glVertex3f(0.04f + legAngleR * 0.9f, 0.13f, 0.02f);
        glVertex3f(0.23f + legAngleR * 0.9f, 0.13f, 0.02f);
        glVertex3f(0.23f + legAngleR * 0.4f, 0.28f, 0.02f);
        glVertex3f(0.04f + legAngleR * 0.4f, 0.28f, 0.02f);
        glEnd();
        // Right Knee Poleyn (Kneecap guard, Z = 0.025f)
        glColor3f(legsPal.hr, legsPal.hg, legsPal.hb);
        glBegin(GL_QUADS);
        glVertex3f(0.03f + legAngleR * 0.65f, 0.25f, 0.025f);
        glVertex3f(0.24f + legAngleR * 0.65f, 0.25f, 0.025f);
        glVertex3f(0.24f + legAngleR * 0.65f, 0.31f, 0.025f);
        glVertex3f(0.03f + legAngleR * 0.65f, 0.31f, 0.025f);
        glEnd();
        // Specular highlight line along front shin
        glColor3f(legsPal.hr, legsPal.hg, legsPal.hb);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3f(0.14f + legAngleR * 0.9f, 0.14f, 0.028f);
        glVertex3f(0.14f + legAngleR * 0.4f, 0.27f, 0.028f);
        glEnd();
    }

    if (hasBoots) {
        // Left Armored Sabaton (Back Foot, Z = 0.025f)
        glColor3f(bootsPal.r * 0.85f, bootsPal.g * 0.85f, bootsPal.b * 0.85f);
        glBegin(GL_QUADS);
        glVertex3f(-0.25f + legAngleL, 0.03f, 0.025f);
        glVertex3f(-0.02f + legAngleL, 0.03f, 0.025f);
        glVertex3f(-0.02f + legAngleL, 0.15f, 0.025f);
        glVertex3f(-0.25f + legAngleL, 0.15f, 0.025f);
        glEnd();
        // Sabaton ankle cuff trim
        glColor3f(bootsPal.hr * 0.85f, bootsPal.hg * 0.85f, bootsPal.hb * 0.85f);
        glBegin(GL_QUADS);
        glVertex3f(-0.26f + legAngleL, 0.12f, 0.028f);
        glVertex3f(-0.01f + legAngleL, 0.12f, 0.028f);
        glVertex3f(-0.01f + legAngleL, 0.16f, 0.028f);
        glVertex3f(-0.26f + legAngleL, 0.16f, 0.028f);
        glEnd();
        // Heavy armored sole
        glColor3f(bootsPal.sr, bootsPal.sg, bootsPal.sb);
        glBegin(GL_QUADS);
        glVertex3f(-0.26f + legAngleL, 0.0f, 0.028f);
        glVertex3f(-0.01f + legAngleL, 0.0f, 0.028f);
        glVertex3f(-0.01f + legAngleL, 0.035f, 0.028f);
        glVertex3f(-0.26f + legAngleL, 0.035f, 0.028f);
        glEnd();

        // Right Armored Sabaton (Front Foot, Z = 0.025f)
        glColor3f(bootsPal.r, bootsPal.g, bootsPal.b);
        glBegin(GL_QUADS);
        glVertex3f(0.02f + legAngleR, 0.03f, 0.025f);
        glVertex3f(0.25f + legAngleR, 0.03f, 0.025f);
        glVertex3f(0.25f + legAngleR, 0.15f, 0.025f);
        glVertex3f(0.02f + legAngleR, 0.15f, 0.025f);
        // Pointed armored toe cap
        glVertex3f(0.22f + legAngleR, 0.02f, 0.026f);
        glVertex3f(0.28f + legAngleR, 0.02f, 0.026f);
        glVertex3f(0.28f + legAngleR, 0.08f, 0.026f);
        glVertex3f(0.22f + legAngleR, 0.08f, 0.026f);
        glEnd();
        // Sabaton ankle cuff trim
        glColor3f(bootsPal.hr, bootsPal.hg, bootsPal.hb);
        glBegin(GL_QUADS);
        glVertex3f(0.01f + legAngleR, 0.12f, 0.028f);
        glVertex3f(0.26f + legAngleR, 0.12f, 0.028f);
        glVertex3f(0.26f + legAngleR, 0.16f, 0.028f);
        glVertex3f(0.01f + legAngleR, 0.16f, 0.028f);
        glEnd();
        // Armored tread / sole
        glColor3f(bootsPal.sr, bootsPal.sg, bootsPal.sb);
        glBegin(GL_QUADS);
        glVertex3f(0.01f + legAngleR, 0.0f, 0.028f);
        glVertex3f(0.29f + legAngleR, 0.0f, 0.028f);
        glVertex3f(0.29f + legAngleR, 0.035f, 0.028f);
        glVertex3f(0.01f + legAngleR, 0.035f, 0.028f);
        glEnd();
        // Toe cap highlight
        glColor3f(bootsPal.hr, bootsPal.hg, bootsPal.hb);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3f(0.23f + legAngleR, 0.08f, 0.03f);
        glVertex3f(0.28f + legAngleR, 0.03f, 0.03f);
        glEnd();
    }

    // 3. Torso & Adventurer Tunic
    // Main Royal Blue Tunic
    glColor3f(0.22f, 0.46f, 0.90f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 0.45f, 0.0f);
    glVertex3f( 0.30f, 0.45f, 0.0f);
    glVertex3f( 0.30f, 1.25f, 0.0f);
    glVertex3f(-0.30f, 1.25f, 0.0f);
    glEnd();
    // Shaded side contour
    glColor3f(0.16f, 0.34f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 0.45f, 0.01f);
    glVertex3f(-0.20f, 0.45f, 0.01f);
    glVertex3f(-0.20f, 1.25f, 0.01f);
    glVertex3f(-0.30f, 1.25f, 0.01f);
    glEnd();
    // Cream V-Neck Collar & Undershirt
    glColor3f(0.92f, 0.90f, 0.85f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.09f, 1.25f, 0.02f);
    glVertex3f( 0.09f, 1.25f, 0.02f);
    glVertex3f( 0.0f,  1.05f, 0.02f);
    glEnd();

    // Leather Utility Belt
    glColor3f(0.32f, 0.18f, 0.08f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 0.45f, 0.02f);
    glVertex3f( 0.30f, 0.45f, 0.02f);
    glVertex3f( 0.30f, 0.60f, 0.02f);
    glVertex3f(-0.30f, 0.60f, 0.02f);
    glEnd();
    // Gleaming Golden Buckle
    glColor3f(0.98f, 0.86f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(-0.08f, 0.46f, 0.03f);
    glVertex3f( 0.08f, 0.46f, 0.03f);
    glVertex3f( 0.08f, 0.59f, 0.03f);
    glVertex3f(-0.08f, 0.59f, 0.03f);
    glEnd();
    // Buckle Prong Center
    glColor3f(0.40f, 0.25f, 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(-0.02f, 0.49f, 0.035f);
    glVertex3f( 0.02f, 0.49f, 0.035f);
    glVertex3f( 0.02f, 0.56f, 0.035f);
    glVertex3f(-0.02f, 0.56f, 0.035f);
    glEnd();
    // Side Leather Pouch
    glColor3f(0.42f, 0.26f, 0.14f);
    glBegin(GL_QUADS);
    glVertex3f( 0.18f, 0.47f, 0.03f);
    glVertex3f( 0.28f, 0.47f, 0.03f);
    glVertex3f( 0.28f, 0.62f, 0.03f);
    glVertex3f( 0.18f, 0.62f, 0.03f);
    glEnd();

    // Visual Armor Overlay: Cuirass & Pauldrons (Chestplate)
    if (hasChestplate) {
        // Cuirass Breastplate Base (Z = 0.04f)
        glColor3f(chestPal.r, chestPal.g, chestPal.b);
        glBegin(GL_QUADS);
        glVertex3f(-0.31f, 0.54f, 0.04f);
        glVertex3f( 0.31f, 0.54f, 0.04f);
        glVertex3f( 0.31f, 1.26f, 0.04f);
        glVertex3f(-0.31f, 1.26f, 0.04f);
        glEnd();

        // Shaded rear torso edge
        glColor3f(chestPal.sr, chestPal.sg, chestPal.sb);
        glBegin(GL_QUADS);
        glVertex3f(-0.31f, 0.54f, 0.042f);
        glVertex3f(-0.22f, 0.54f, 0.042f);
        glVertex3f(-0.22f, 1.26f, 0.042f);
        glVertex3f(-0.31f, 1.26f, 0.042f);
        glEnd();

        // Segmented Lower Abdominal Faulds
        glColor3f(chestPal.r * 0.9f, chestPal.g * 0.9f, chestPal.b * 0.9f);
        glBegin(GL_QUADS);
        glVertex3f(-0.29f, 0.54f, 0.043f);
        glVertex3f( 0.29f, 0.54f, 0.043f);
        glVertex3f( 0.29f, 0.70f, 0.043f);
        glVertex3f(-0.29f, 0.70f, 0.043f);
        glEnd();
        // Fauld articulation line
        glColor3f(chestPal.sr, chestPal.sg, chestPal.sb);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3f(-0.29f, 0.62f, 0.044f);
        glVertex3f( 0.29f, 0.62f, 0.044f);
        glEnd();

        // Pectoral Plate Center Ridge & Gorget Collar (Specular Highlight)
        glColor3f(chestPal.hr, chestPal.hg, chestPal.hb);
        glBegin(GL_QUADS);
        // Pectoral plate crest
        glVertex3f(-0.06f, 0.78f, 0.045f);
        glVertex3f( 0.18f, 0.78f, 0.045f);
        glVertex3f( 0.18f, 1.18f, 0.045f);
        glVertex3f(-0.06f, 1.18f, 0.045f);
        // Armored Neck Gorget
        glVertex3f(-0.16f, 1.18f, 0.046f);
        glVertex3f( 0.16f, 1.18f, 0.046f);
        glVertex3f( 0.16f, 1.27f, 0.046f);
        glVertex3f(-0.16f, 1.27f, 0.046f);
        glEnd();

        // Center Sternum Specular Chiseled Line
        glColor3f(fminf(chestPal.hr + 0.2f, 1.0f), fminf(chestPal.hg + 0.2f, 1.0f), fminf(chestPal.hb + 0.2f, 1.0f));
        glLineWidth(2.2f);
        glBegin(GL_LINES);
        glVertex3f(0.06f, 0.74f, 0.048f);
        glVertex3f(0.06f, 1.22f, 0.048f);
        glEnd();

        // Front Pauldron (Shoulder Guard, Z = 0.05f)
        glColor3f(chestPal.r, chestPal.g, chestPal.b);
        glBegin(GL_QUADS);
        glVertex3f(0.08f, 1.04f, 0.05f);
        glVertex3f(0.33f, 1.04f, 0.05f);
        glVertex3f(0.31f, 1.28f, 0.05f);
        glVertex3f(0.08f, 1.28f, 0.05f);
        glEnd();
        // Front Pauldron trim
        glColor3f(chestPal.hr, chestPal.hg, chestPal.hb);
        glLineWidth(2.2f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(0.08f, 1.04f, 0.052f);
        glVertex3f(0.33f, 1.04f, 0.052f);
        glVertex3f(0.31f, 1.28f, 0.052f);
        glVertex3f(0.08f, 1.28f, 0.052f);
        glEnd();

        // Rear Pauldron (Shoulder Guard, Z = 0.05f)
        glColor3f(chestPal.r * 0.85f, chestPal.g * 0.85f, chestPal.b * 0.85f);
        glBegin(GL_QUADS);
        glVertex3f(-0.33f, 1.04f, 0.05f);
        glVertex3f(-0.08f, 1.04f, 0.05f);
        glVertex3f(-0.08f, 1.28f, 0.05f);
        glVertex3f(-0.31f, 1.28f, 0.05f);
        glEnd();
        // Rear Pauldron border
        glColor3f(chestPal.sr, chestPal.sg, chestPal.sb);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-0.33f, 1.04f, 0.052f);
        glVertex3f(-0.08f, 1.04f, 0.052f);
        glVertex3f(-0.08f, 1.28f, 0.052f);
        glVertex3f(-0.31f, 1.28f, 0.052f);
        glEnd();
    }

    // 4. Head, Hair & Expressive Face
    // Base Face Skin
    glColor3f(0.98f, 0.82f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(-0.25f, 1.25f, 0.0f);
    glVertex3f( 0.25f, 1.25f, 0.0f);
    glVertex3f( 0.25f, 1.70f, 0.0f);
    glVertex3f(-0.25f, 1.70f, 0.0f);
    glEnd();
    // Chin Shadow
    glColor3f(0.90f, 0.74f, 0.62f);
    glBegin(GL_QUADS);
    glVertex3f(-0.20f, 1.25f, 0.01f);
    glVertex3f( 0.20f, 1.25f, 0.01f);
    glVertex3f( 0.20f, 1.30f, 0.01f);
    glVertex3f(-0.20f, 1.30f, 0.01f);
    glEnd();

    // Back & Top Hair (Rich Auburn/Brown Locks)
    glColor3f(0.36f, 0.20f, 0.10f);
    glBegin(GL_QUADS);
    glVertex3f(-0.28f, 1.55f, 0.015f);
    glVertex3f( 0.28f, 1.55f, 0.015f);
    glVertex3f( 0.28f, 1.80f, 0.015f);
    glVertex3f(-0.28f, 1.80f, 0.015f);
    // Rear wind-swept hair tuft
    glVertex3f(-0.32f, 1.48f, 0.015f);
    glVertex3f(-0.25f, 1.48f, 0.015f);
    glVertex3f(-0.25f, 1.68f, 0.015f);
    glVertex3f(-0.32f, 1.68f, 0.015f);
    glEnd();

    // Adventurer Crimson Bandana / Headband
    glColor3f(0.85f, 0.22f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(-0.28f, 1.56f, 0.025f);
    glVertex3f( 0.28f, 1.56f, 0.025f);
    glVertex3f( 0.28f, 1.66f, 0.025f);
    glVertex3f(-0.28f, 1.66f, 0.025f);
    // Trailing bandana ribbon at back
    glVertex3f(-0.34f, 1.48f, 0.025f);
    glVertex3f(-0.28f, 1.48f, 0.025f);
    glVertex3f(-0.28f, 1.62f, 0.025f);
    glVertex3f(-0.34f, 1.62f, 0.025f);
    glEnd();
    // Bandana golden knot accent
    glColor3f(0.98f, 0.85f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 1.55f, 0.03f);
    glVertex3f(-0.26f, 1.55f, 0.03f);
    glVertex3f(-0.26f, 1.63f, 0.03f);
    glVertex3f(-0.30f, 1.63f, 0.03f);
    glEnd();

    // Front Hair Bangs overlapping forehead
    glColor3f(0.48f, 0.28f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(-0.06f, 1.52f, 0.028f);
    glVertex3f( 0.12f, 1.52f, 0.028f);
    glVertex3f( 0.12f, 1.62f, 0.028f);
    glVertex3f(-0.06f, 1.62f, 0.028f);
    glEnd();

    // Expressive Eyes with Pupil & Twinkle Glint
    // Sclera (White)
    glColor3f(0.96f, 0.98f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(0.06f, 1.38f, 0.03f);
    glVertex3f(0.19f, 1.38f, 0.03f);
    glVertex3f(0.19f, 1.50f, 0.03f);
    glVertex3f(0.06f, 1.50f, 0.03f);
    glEnd();
    // Dark Pupil / Iris
    glColor3f(0.10f, 0.12f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(0.09f, 1.38f, 0.035f);
    glVertex3f(0.18f, 1.38f, 0.035f);
    glVertex3f(0.18f, 1.49f, 0.035f);
    glVertex3f(0.09f, 1.49f, 0.035f);
    glEnd();
    // Specular Twinkle Sparkle
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(0.13f, 1.44f, 0.04f);
    glVertex3f(0.17f, 1.44f, 0.04f);
    glVertex3f(0.17f, 1.48f, 0.04f);
    glVertex3f(0.13f, 1.48f, 0.04f);
    glEnd();
    // Eyebrow
    glColor3f(0.30f, 0.16f, 0.08f);
    glBegin(GL_QUADS);
    glVertex3f(0.05f, 1.51f, 0.035f);
    glVertex3f(0.20f, 1.51f, 0.035f);
    glVertex3f(0.20f, 1.54f, 0.035f);
    glVertex3f(0.05f, 1.54f, 0.035f);
    glEnd();
    // Cheek Blush
    glColor3f(0.96f, 0.65f, 0.58f);
    glBegin(GL_QUADS);
    glVertex3f(0.08f, 1.31f, 0.02f);
    glVertex3f(0.18f, 1.31f, 0.02f);
    glVertex3f(0.18f, 1.36f, 0.02f);
    glVertex3f(0.08f, 1.36f, 0.02f);
    glEnd();

    // Visual Armor Overlay: Helmet with Visor & Cheekplates
    if (hasHelmet) {
        // Helmet Dome / Skullcap (covers top and rear hair, Z = 0.045f)
        glColor3f(helmetPal.r, helmetPal.g, helmetPal.b);
        glBegin(GL_QUADS);
        glVertex3f(-0.29f, 1.54f, 0.045f);
        glVertex3f( 0.28f, 1.54f, 0.045f);
        glVertex3f( 0.26f, 1.83f, 0.045f);
        glVertex3f(-0.29f, 1.83f, 0.045f);
        glEnd();

        // Rear Neckguard / Nape Guard (Z = 0.045f)
        glColor3f(helmetPal.sr, helmetPal.sg, helmetPal.sb);
        glBegin(GL_QUADS);
        glVertex3f(-0.32f, 1.25f, 0.045f);
        glVertex3f(-0.22f, 1.25f, 0.045f);
        glVertex3f(-0.22f, 1.58f, 0.045f);
        glVertex3f(-0.32f, 1.58f, 0.045f);
        glEnd();

        // Helmet Crown Crest Ridge (Z = 0.048f)
        glColor3f(helmetPal.hr, helmetPal.hg, helmetPal.hb);
        glBegin(GL_QUADS);
        glVertex3f(-0.16f, 1.80f, 0.048f);
        glVertex3f( 0.16f, 1.80f, 0.048f);
        glVertex3f( 0.14f, 1.87f, 0.048f);
        glVertex3f(-0.14f, 1.87f, 0.048f);
        glEnd();

        // Cheekplate / Temporal Guard (Z = 0.046f)
        glColor3f(helmetPal.r, helmetPal.g, helmetPal.b);
        glBegin(GL_QUADS);
        glVertex3f(-0.26f, 1.26f, 0.046f);
        glVertex3f( 0.02f, 1.26f, 0.046f);
        glVertex3f( 0.02f, 1.56f, 0.046f);
        glVertex3f(-0.26f, 1.56f, 0.046f);
        glEnd();
        // Cheekplate bevel border
        glColor3f(helmetPal.sr, helmetPal.sg, helmetPal.sb);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex3f(0.02f, 1.26f, 0.047f);
        glVertex3f(0.02f, 1.56f, 0.047f);
        glEnd();

        // Armored Chin / Lower Jaw Guard (Z = 0.046f)
        glColor3f(helmetPal.r * 0.95f, helmetPal.g * 0.95f, helmetPal.b * 0.95f);
        glBegin(GL_QUADS);
        glVertex3f(0.02f, 1.25f, 0.046f);
        glVertex3f(0.24f, 1.25f, 0.046f);
        glVertex3f(0.24f, 1.34f, 0.046f);
        glVertex3f(0.02f, 1.34f, 0.046f);
        glEnd();

        // Visor Brow / Forehead Guard (Projecting visor, Z = 0.05f)
        glColor3f(helmetPal.hr, helmetPal.hg, helmetPal.hb);
        glBegin(GL_QUADS);
        glVertex3f(-0.02f, 1.51f, 0.05f);
        glVertex3f( 0.30f, 1.51f, 0.05f);
        glVertex3f( 0.28f, 1.62f, 0.05f);
        glVertex3f(-0.02f, 1.62f, 0.05f);
        glEnd();

        // Nasal Guard / Front Lip (Z = 0.048f)
        glColor3f(helmetPal.r, helmetPal.g, helmetPal.b);
        glBegin(GL_QUADS);
        glVertex3f(0.21f, 1.32f, 0.048f);
        glVertex3f(0.27f, 1.32f, 0.048f);
        glVertex3f(0.27f, 1.52f, 0.048f);
        glVertex3f(0.21f, 1.52f, 0.048f);
        glEnd();

        // Visor Eye Slit Aperture Dark Framing (Z = 0.049f)
        glColor3f(0.10f, 0.12f, 0.15f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(0.03f, 1.35f, 0.049f);
        glVertex3f(0.21f, 1.35f, 0.049f);
        glVertex3f(0.21f, 1.51f, 0.049f);
        glVertex3f(0.03f, 1.51f, 0.049f);
        glEnd();

        // Diamond radiant crest glint (Special diamond helmet visual)
        if (getArmorTier(g_EquippedArmor[ARMOR_SLOT_HELMET]) == TIER_DIAMOND) {
            glColor3f(0.70f, 1.0f, 1.0f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.12f, 1.54f, 0.052f);
            glVertex3f(0.18f, 1.54f, 0.052f);
            glVertex3f(0.15f, 1.60f, 0.052f);
            glEnd();
        }
    }

    // 5. In-Hand Tool Rendering with Dynamic Arm Swing Arc & Motion Trail
    renderInHandTool(g_Hotbar[g_ActiveSlot].type, g_ToolSwingTimer, g_ToolIsSwinging, g_Player.isFacingRight);

    glPopMatrix();

    // Pass 5.5: 2.5D Animated Procedural Mobs (Slimes, Bats, Zombies)
    renderMobs(daylight, g_CamX, g_CamY);

    // Pass 6: 3D Target Selection Box (Z = 0.04f, in front of blocks)
    if (g_TargetValid) {
        float tx = (float)g_TargetX;
        float ty = (float)g_TargetY;

        if (g_MiningProgress > 0.0f) {
            float shake = sinf(g_MiningProgress * 40.0f) * 0.04f;
            glColor3f(1.0f, 0.75f - g_MiningProgress * 0.4f, 0.1f);
            tx += shake;
        } else {
            glColor3f(0.2f, 0.95f, 0.35f);
        }

        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(tx - 0.02f, ty - 0.02f, 0.04f);
        glVertex3f(tx + 1.02f, ty - 0.02f, 0.04f);
        glVertex3f(tx + 1.02f, ty + 1.02f, 0.04f);
        glVertex3f(tx - 0.02f, ty + 1.02f, 0.04f);
        glEnd();
        glLineWidth(1.0f);
    }
}

static void renderTutorialQuestBox(void) {
    float qBoxW = 350.0f;
    float qBoxH = 105.0f;
    float qBoxX = (float)WINDOW_WIDTH - qBoxW - 15.0f;
    float qBoxY = 15.0f;
    drawQuad2D(qBoxX, qBoxY, qBoxW, qBoxH, 0.08f, 0.10f, 0.15f, 0.90f);
    drawBorder2D(qBoxX, qBoxY, qBoxW, qBoxH, 1.0f, 0.82f, 0.20f, 2.0f);

    drawText(qBoxX + 15, qBoxY + 22, LOC("新手探险指南", "Quest Guide"), 1.0f, 0.85f, 0.2f, true);

    if (g_TutorialStep == TUTORIAL_MOVE_JUMP) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 1: 基础移动与跳跃", "Step 1: Movement & Jump"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("使用 [A / D] 移动，[Space] 跳跃", "Use [A / D] to move, [Space] to Jump"), 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "[%s] %s  [%s] %s  [%s] %s",
            g_TutMovedLeft ? "V" : " ", LOC("左移", "Left"),
            g_TutMovedRight ? "V" : " ", LOC("右移", "Right"),
            g_TutJumped ? "V" : " ", LOC("跳跃", "Jump"));
        drawText(qBoxX + 15, qBoxY + 82, prog, 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_MINE) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 2: 资源采集", "Step 2: Resource Gathering"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("按住 [鼠标左键] 挖掘 5 个方块", "Hold [Left Click] to mine 5 blocks"), 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "%s: %d / 5", LOC("采集进度", "Progress"), g_MinedCount);
        drawText(qBoxX + 15, qBoxY + 82, prog, 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_BUILD) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 3: 方块搭建", "Step 3: Block Construction"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("选择方块 [1-9]，[鼠标右键] 放置", "Select block [1-9], [Right Click] to place"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("目标位置须与已有方块相邻", "Target must be adjacent to solid block"), 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_TORCH) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 4: 地下照明", "Step 4: Underground Illumination"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("选择火把 [5] 并放置以照亮黑暗", "Select Torch [5] & place to light dark"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("火把可散发暖色动态照明", "Torches emit warm dynamic light"), 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_EXPLORE) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 5: 快速存档与探索", "Step 5: Quick Save & Explore"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("按 [F5] 快速保存当前世界进度！", "Press [F5] to Quick Save your game!"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("随时按 [H] 可查看全键位说明", "Press [H] anytime for Keybinds"), 0.3f, 0.95f, 0.4f, false);
    } else {
        drawText(qBoxX + 15, qBoxY + 44, LOC("新手引导完成！探索大师", "Tutorial Complete! Master Explorer"), 0.3f, 0.95f, 0.3f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("自由探索沙漠、雪原与地下熔岩！", "Explore Desert, Tundra & Underworld!"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("[F5] 保存  [F9] 读取  [H] 帮助", "[F5] Quick Save  [F9] Quick Load  [H] Help"), 1.0f, 0.85f, 0.2f, false);
    }
}

// ============================================================================
// Chest Storage Container UI & Interaction
// ============================================================================
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
                playSound(SFX_CLICK);
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
                playSound(SFX_CLICK);
                return true;
            }
        }
        showNotification(LOC("[背包已满！]", "[Inventory Full!]"), 0.95f, 0.35f, 0.35f);
        return false;
    }

    int remaining = count;
    // 1. Stacking in hotbar
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == type && g_Hotbar[i].count < 999) {
            int space = 999 - g_Hotbar[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Hotbar[i].count += take;
            remaining -= take;
        }
    }
    // 2. Stacking in backpack
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == type && g_Backpack[i].count < 999) {
            int space = 999 - g_Backpack[i].count;
            int take = (remaining < space) ? remaining : space;
            g_Backpack[i].count += take;
            remaining -= take;
        }
    }
    // 3. Empty hotbar slot
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
    // 4. Empty backpack slot
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
        playSound(SFX_CLICK);
        src->count -= taken;
        if (src->count <= 0) {
            src->type = BLOCK_AIR;
            src->count = 0;
            src->durability = 0;
            src->maxDurability = 0;
        }
        return true;
    } else {
        showNotification(LOC("[背包已满！]", "[Inventory Full!]"), 0.95f, 0.35f, 0.35f);
        return false;
    }
}

static inline bool transferChestToPlayer(WorldChest* chest, int slotIdx) {
    return transferChestToPlayerEx(chest, slotIdx, 0);
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
                playSound(SFX_CLICK);
                return true;
            }
        }
        showNotification(LOC("[宝箱已满！]", "[Chest Full!]"), 0.95f, 0.35f, 0.35f);
        return false;
    }

    int remaining = count;
    // 1. Stack in existing chest slots
    for (int i = 0; i < CHEST_SLOTS && remaining > 0; i++) {
        if (chest->slots[i].type == type && chest->slots[i].count < 999) {
            int space = 999 - chest->slots[i].count;
            int take = (remaining < space) ? remaining : space;
            chest->slots[i].count += take;
            remaining -= take;
        }
    }
    // 2. Empty slot in chest
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
        playSound(SFX_CLICK);
        pSlot->count -= taken;
        if (pSlot->count <= 0) {
            pSlot->type = BLOCK_AIR;
            pSlot->count = 0;
            pSlot->durability = 0;
            pSlot->maxDurability = 0;
        }
        return true;
    } else {
        showNotification(LOC("[宝箱已满！]", "[Chest Full!]"), 0.95f, 0.35f, 0.35f);
        return false;
    }
}

static inline bool transferPlayerToChest(WorldChest* chest, InventorySlot* pSlot) {
    return transferPlayerToChestEx(chest, pSlot, 0);
}

static void renderChestUI(void) {
    if (!g_ChestOpen) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)WINDOW_WIDTH, (double)WINDOW_HEIGHT, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Fullscreen dark backdrop
    menuDrawQuad(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, 0.0f, 0.0f, 0.65f);

    float modalW = 580.0f;
    float modalH = 480.0f;
    float modalX = ((float)WINDOW_WIDTH - modalW) * 0.5f;
    float modalY = ((float)WINDOW_HEIGHT - modalH) * 0.5f;

    // 2. Main ornate modal window
    menuDrawQuad(modalX, modalY, modalW, modalH, 0.08f, 0.10f, 0.14f, 0.96f);
    menuDrawBorder(modalX, modalY, modalW, modalH, 0.28f, 0.36f, 0.48f, 2.0f);
    menuDrawBorder(modalX - 2.0f, modalY - 2.0f, modalW + 4.0f, modalH + 4.0f, 0.85f, 0.70f, 0.25f, 1.0f);

    // 3. Header title & subtitle
    const char* title = LOC("大藏宝箱 / Storage Chest", "Storage Chest / 大藏宝箱");
    drawText(modalX + 22.0f, modalY + 28.0f, title, 1.0f, 0.88f, 0.35f, true);
    drawText(modalX + 22.0f, modalY + 46.0f,
             LOC("安全存储世界道具 | 点击物品自由存取", "Secure Item Storage | Click slots to deposit/withdraw"),
             0.55f, 0.75f, 0.90f, false);

    // Close button [X]
    float closeX = modalX + modalW - 36.0f;
    float closeY = modalY + 16.0f;
    bool closeHov = (g_MouseScreenX >= closeX && g_MouseScreenX <= closeX + 22.0f &&
                     g_MouseScreenY >= closeY && g_MouseScreenY <= closeY + 22.0f);
    menuDrawQuad(closeX, closeY, 22.0f, 22.0f, closeHov ? 0.90f : 0.75f, 0.20f, 0.20f, 1.0f);
    menuDrawBorder(closeX, closeY, 22.0f, 22.0f, 1.0f, 0.8f, 0.8f, 1.0f);
    drawText(closeX + 6.0f, closeY + 16.0f, "X", 1.0f, 1.0f, 1.0f, true);

    // Divider line
    menuDrawQuad(modalX + 16.0f, modalY + 54.0f, modalW - 32.0f, 2.0f, 0.25f, 0.32f, 0.42f, 0.8f);

    // Quick Action buttons on top right:
    // [全部取出 / Take All] and [全部存入 / Quick Stash]
    float actionBtnW = 100.0f;
    float actionBtnH = 22.0f;
    float takeAllX = modalX + modalW - 22.0f - actionBtnW * 2.0f - 8.0f;
    float depositX = modalX + modalW - 22.0f - actionBtnW;
    float btnY = modalY + 62.0f;

    bool takeAllHov = (g_MouseScreenX >= takeAllX && g_MouseScreenX <= takeAllX + actionBtnW &&
                       g_MouseScreenY >= btnY && g_MouseScreenY <= btnY + actionBtnH);
    menuDrawQuad(takeAllX, btnY, actionBtnW, actionBtnH,
                 takeAllHov ? 0.28f : 0.18f, takeAllHov ? 0.32f : 0.22f, takeAllHov ? 0.42f : 0.30f, 1.0f);
    menuDrawBorder(takeAllX, btnY, actionBtnW, actionBtnH,
                   takeAllHov ? 1.0f : 0.40f, takeAllHov ? 0.85f : 0.48f, takeAllHov ? 0.35f : 0.60f, 1.0f);
    drawText(takeAllX + 8.0f, btnY + 16.0f, LOC("全部取出", "Take All"), 0.90f, 0.90f, 0.95f, false);

    bool depHov = (g_MouseScreenX >= depositX && g_MouseScreenX <= depositX + actionBtnW &&
                   g_MouseScreenY >= btnY && g_MouseScreenY <= btnY + actionBtnH);
    menuDrawQuad(depositX, btnY, actionBtnW, actionBtnH,
                 depHov ? 0.28f : 0.18f, depHov ? 0.32f : 0.22f, depHov ? 0.42f : 0.30f, 1.0f);
    menuDrawBorder(depositX, btnY, actionBtnW, actionBtnH,
                   depHov ? 1.0f : 0.40f, depHov ? 0.85f : 0.48f, depHov ? 0.35f : 0.60f, 1.0f);
    drawText(depositX + 8.0f, btnY + 16.0f, LOC("全部存入", "Quick Stash"), 0.90f, 0.90f, 0.95f, false);

    // Section 1: Chest Storage (20 slots, 4 rows x 5 columns)
    drawText(modalX + 22.0f, modalY + 76.0f, LOC("宝箱存储空间 (20格):", "Chest Storage (20 Slots):"), 0.95f, 0.85f, 0.40f, true);

    WorldChest* chest = getOrCreateChest(g_ActiveChestX, g_ActiveChestY);

    float cSlotSize = 56.0f;
    float cSlotGap = 10.0f;
    float chestStartX = modalX + (modalW - (5.0f * cSlotSize + 4.0f * cSlotGap)) * 0.5f;
    float chestStartY = modalY + 88.0f;

    int hoveredItem = -1;
    int hoveredCount = 0;
    int hoveredDur = 0, hoveredMaxDur = 0;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 5; c++) {
            int slotIdx = r * 5 + c;
            float sx = chestStartX + (float)c * (cSlotSize + cSlotGap);
            float sy = chestStartY + (float)r * (cSlotSize + cSlotGap);

            bool hovered = (g_MouseScreenX >= sx && g_MouseScreenX <= sx + cSlotSize &&
                            g_MouseScreenY >= sy && g_MouseScreenY <= sy + cSlotSize);

            // Slot Background
            menuDrawQuad(sx, sy, cSlotSize, cSlotSize, 0.12f, 0.15f, 0.22f, 1.0f);

            if (hovered) {
                menuDrawBorder(sx - 1.0f, sy - 1.0f, cSlotSize + 2.0f, cSlotSize + 2.0f, 1.0f, 0.85f, 0.25f, 2.0f);
            } else {
                menuDrawBorder(sx, sy, cSlotSize, cSlotSize, 0.32f, 0.40f, 0.52f, 1.0f);
            }

            if (chest && chest->slots[slotIdx].type != BLOCK_AIR && chest->slots[slotIdx].count > 0) {
                InventorySlot* s = &chest->slots[slotIdx];
                drawInventoryItemIcon(sx + 8.0f, sy + 6.0f, cSlotSize - 16.0f, s->type);

                if (!isToolItem(s->type) && !isArmorItem(s->type)) {
                    char countBuf[16];
                    snprintf(countBuf, sizeof(countBuf), "%d", s->count);
                    drawText(sx + cSlotSize - 22.0f, sy + cSlotSize - 4.0f, countBuf, 1.0f, 1.0f, 1.0f, false);
                } else if (s->durability > 0 && s->durability < s->maxDurability && s->maxDurability > 0) {
                    float pct = (float)s->durability / (float)s->maxDurability;
                    if (pct > 1.0f) pct = 1.0f;
                    float barW = cSlotSize - 12.0f;
                    float barH = 3.0f;
                    float barX = sx + 6.0f;
                    float barY = sy + cSlotSize - 6.0f;
                    menuDrawQuad(barX, barY - 1.0f, barW, barH + 2.0f, 0.0f, 0.0f, 0.0f, 0.8f);
                    float cr = (pct <= 0.20f) ? 0.95f : ((pct <= 0.50f) ? 0.95f : 0.20f);
                    float cg = (pct <= 0.20f) ? 0.20f : ((pct <= 0.50f) ? 0.85f : 0.90f);
                    float cb = (pct <= 0.20f) ? 0.20f : ((pct <= 0.50f) ? 0.15f : 0.20f);
                    menuDrawQuad(barX, barY, barW * pct, barH, cr, cg, cb, 1.0f);
                }

                if (hovered) {
                    hoveredItem = s->type;
                    hoveredCount = s->count;
                    hoveredDur = s->durability;
                    hoveredMaxDur = s->maxDurability;
                }
            }
        }
    }

    // Divider line between Chest and Hotbar
    float divY = chestStartY + 4.0f * (cSlotSize + cSlotGap) + 4.0f;
    menuDrawQuad(modalX + 16.0f, divY, modalW - 32.0f, 2.0f, 0.25f, 0.32f, 0.42f, 0.8f);

    // Section 2: Player Hotbar (9 slots)
    float hotbarTitleY = divY + 16.0f;
    drawText(modalX + 22.0f, hotbarTitleY, LOC("玩家快捷栏 (点击存入宝箱):", "Player Hotbar (Click to Deposit):"), 1.0f, 0.85f, 0.35f, true);

    float hSlotSize = 52.0f;
    float hSlotGap = 8.0f;
    float hotbarStartX = modalX + (modalW - ((float)HOTBAR_SLOTS * hSlotSize + (float)(HOTBAR_SLOTS - 1) * hSlotGap)) * 0.5f;
    float hotbarStartY = hotbarTitleY + 10.0f;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        float sx = hotbarStartX + (float)i * (hSlotSize + hSlotGap);
        float sy = hotbarStartY;

        bool hovered = (g_MouseScreenX >= sx && g_MouseScreenX <= sx + hSlotSize &&
                        g_MouseScreenY >= sy && g_MouseScreenY <= sy + hSlotSize);

        menuDrawQuad(sx, sy, hSlotSize, hSlotSize, 0.14f, 0.17f, 0.24f, 1.0f);

        if (i == g_ActiveSlot) {
            menuDrawBorder(sx - 2.0f, sy - 2.0f, hSlotSize + 4.0f, hSlotSize + 4.0f, 1.0f, 0.85f, 0.20f, 2.0f);
        } else if (hovered) {
            menuDrawBorder(sx - 1.0f, sy - 1.0f, hSlotSize + 2.0f, hSlotSize + 2.0f, 0.40f, 0.85f, 1.0f, 1.5f);
        } else {
            menuDrawBorder(sx, sy, hSlotSize, hSlotSize, 0.30f, 0.38f, 0.48f, 1.0f);
        }

        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        drawText(sx + 4.0f, sy + 14.0f, numBuf, 0.65f, 0.65f, 0.65f, false);

        if (g_Hotbar[i].type != BLOCK_AIR && g_Hotbar[i].count > 0) {
            InventorySlot* s = &g_Hotbar[i];
            drawInventoryItemIcon(sx + 8.0f, sy + 6.0f, hSlotSize - 16.0f, s->type);

            if (!isToolItem(s->type) && !isArmorItem(s->type)) {
                char countBuf[16];
                snprintf(countBuf, sizeof(countBuf), "%d", s->count);
                drawText(sx + hSlotSize - 22.0f, sy + hSlotSize - 4.0f, countBuf, 1.0f, 1.0f, 1.0f, false);
            } else if (s->durability > 0 && s->durability < s->maxDurability && s->maxDurability > 0) {
                float pct = (float)s->durability / (float)s->maxDurability;
                if (pct > 1.0f) pct = 1.0f;
                float barW = hSlotSize - 12.0f;
                float barH = 3.0f;
                float barX = sx + 6.0f;
                float barY = sy + hSlotSize - 6.0f;
                menuDrawQuad(barX, barY - 1.0f, barW, barH + 2.0f, 0.0f, 0.0f, 0.0f, 0.8f);
                float cr = (pct <= 0.20f) ? 0.95f : ((pct <= 0.50f) ? 0.95f : 0.20f);
                float cg = (pct <= 0.20f) ? 0.20f : ((pct <= 0.50f) ? 0.85f : 0.90f);
                float cb = (pct <= 0.20f) ? 0.20f : ((pct <= 0.50f) ? 0.15f : 0.20f);
                menuDrawQuad(barX, barY, barW * pct, barH, cr, cg, cb, 1.0f);
            }

            if (hovered) {
                hoveredItem = s->type;
                hoveredCount = s->count;
                hoveredDur = s->durability;
                hoveredMaxDur = s->maxDurability;
            }
        }
    }

    // Footer helper text
    drawText(modalX + 22.0f, modalY + modalH - 12.0f,
             LOC("[左键] 全部转移 | [右键] 单个转移 | [E / ESC] 关闭宝箱",
                 "[L-Click] Transfer All | [R-Click] Transfer 1 | [E / ESC] Close Chest"),
             0.60f, 0.70f, 0.85f, false);

    // Floating Tooltip on Hovered Item
    if (hoveredItem > 0) {
        float tipW = 280.0f;
        float tipH = 68.0f;
        float tipX = (float)g_MouseScreenX + 16.0f;
        float tipY = (float)g_MouseScreenY + 16.0f;
        if (tipX + tipW > (float)WINDOW_WIDTH - 20.0f) tipX = (float)WINDOW_WIDTH - tipW - 20.0f;
        if (tipY + tipH > (float)WINDOW_HEIGHT - 20.0f) tipY = (float)WINDOW_HEIGHT - tipH - 20.0f;

        menuDrawQuad(tipX, tipY, tipW, tipH, 0.06f, 0.08f, 0.12f, 0.96f);
        menuDrawBorder(tipX, tipY, tipW, tipH, 0.95f, 0.82f, 0.22f, 1.5f);

        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s x%d", getItemLocalizedName(hoveredItem), hoveredCount);
        drawText(tipX + 10.0f, tipY + 20.0f, nameBuf, 1.0f, 0.90f, 0.30f, true);

        if (isToolItem(hoveredItem)) {
            const ToolDef* td = getToolDef(hoveredItem);
            if (td) {
                char statBuf[128];
                snprintf(statBuf, sizeof(statBuf), "%s%.0f | %s%d/%d",
                         LOC("攻击: ", "ATK: "), td->damage,
                         LOC("耐久: ", "DUR: "), hoveredDur, hoveredMaxDur);
                drawText(tipX + 10.0f, tipY + 38.0f, statBuf, 0.85f, 0.90f, 0.95f, false);
            }
        } else if (isArmorItem(hoveredItem)) {
            const ArmorDef* ad = getArmorDef(hoveredItem);
            if (ad) {
                char statBuf[128];
                snprintf(statBuf, sizeof(statBuf), "%s+%.0f%% | %s%d/%d",
                         LOC("减伤: ", "DEF: "), ad->reduction * 100.0f,
                         LOC("耐久: ", "DUR: "), hoveredDur, hoveredMaxDur);
                drawText(tipX + 10.0f, tipY + 38.0f, statBuf, 0.85f, 0.90f, 0.95f, false);
            }
        } else if (hoveredItem < BLOCK_TYPE_COUNT) {
            char statBuf[128];
            snprintf(statBuf, sizeof(statBuf), "%s%.1f | %s",
                     LOC("硬度: ", "Hardness: "), g_BlockDefs[hoveredItem].hardness,
                     g_BlockDefs[hoveredItem].isSolid ? LOC("实体方块", "Solid Block") : LOC("装饰物", "Decor"));
            drawText(tipX + 10.0f, tipY + 38.0f, statBuf, 0.75f, 0.85f, 0.95f, false);
        }

        drawText(tipX + 10.0f, tipY + 54.0f,
                 LOC("点击在宝箱与背包快捷栏间存取", "Click to transfer between chest and hotbar"),
                 0.40f, 0.90f, 0.50f, false);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static bool handleChestInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!g_ChestOpen) return false;

    float modalW = 580.0f;
    float modalH = 480.0f;
    float modalX = ((float)WINDOW_WIDTH - modalW) * 0.5f;
    float modalY = ((float)WINDOW_HEIGHT - modalH) * 0.5f;

    switch (uMsg) {
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE || wParam == 'E') {
                g_ChestOpen = false;
                playSound(SFX_CLICK);
                return true;
            }
            if (wParam >= '1' && wParam <= '9') {
                g_ActiveSlot = (int)(wParam - '1');
                playSound(SFX_CLICK);
                return true;
            }
            return true; // Consume other keyboard keys while chest is open
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            bool isRight = (uMsg == WM_RBUTTONDOWN);
            int mx = (int)(short)LOWORD(lParam);
            int my = (int)(short)HIWORD(lParam);

            // 1. Close button [X]
            float closeX = modalX + modalW - 36.0f;
            float closeY = modalY + 16.0f;
            if (mx >= closeX && mx <= closeX + 22.0f &&
                my >= closeY && my <= closeY + 22.0f) {
                g_ChestOpen = false;
                playSound(SFX_CLICK);
                return true;
            }

            // Quick Action buttons
            float actionBtnW = 100.0f;
            float actionBtnH = 22.0f;
            float takeAllX = modalX + modalW - 22.0f - actionBtnW * 2.0f - 8.0f;
            float depositX = modalX + modalW - 22.0f - actionBtnW;
            float btnY = modalY + 62.0f;

            WorldChest* chest = getOrCreateChest(g_ActiveChestX, g_ActiveChestY);

            // Take All
            if (mx >= takeAllX && mx <= takeAllX + actionBtnW &&
                my >= btnY && my <= btnY + actionBtnH) {
                if (chest) {
                    bool movedAny = false;
                    for (int s = 0; s < CHEST_SLOTS; s++) {
                        if (chest->slots[s].type != BLOCK_AIR && chest->slots[s].count > 0) {
                            if (transferChestToPlayer(chest, s)) movedAny = true;
                        }
                    }
                    if (movedAny) playSound(SFX_CLICK);
                }
                return true;
            }

            // Quick Stash (Hotbar to Chest)
            if (mx >= depositX && mx <= depositX + actionBtnW &&
                my >= btnY && my <= btnY + actionBtnH) {
                if (chest) {
                    bool movedAny = false;
                    for (int h = 0; h < HOTBAR_SLOTS; h++) {
                        if (g_Hotbar[h].type != BLOCK_AIR && g_Hotbar[h].count > 0) {
                            if (transferPlayerToChest(chest, &g_Hotbar[h])) movedAny = true;
                        }
                    }
                    if (movedAny) playSound(SFX_CLICK);
                }
                return true;
            }

            // 2. Chest slots (4 rows x 5 columns)
            float cSlotSize = 56.0f;
            float cSlotGap = 10.0f;
            float chestStartX = modalX + (modalW - (5.0f * cSlotSize + 4.0f * cSlotGap)) * 0.5f;
            float chestStartY = modalY + 88.0f;

            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 5; c++) {
                    int slotIdx = r * 5 + c;
                    float sx = chestStartX + (float)c * (cSlotSize + cSlotGap);
                    float sy = chestStartY + (float)r * (cSlotSize + cSlotGap);

                    if (mx >= sx && mx <= sx + cSlotSize &&
                        my >= sy && my <= sy + cSlotSize) {
                        if (chest) {
                            if (isRight) {
                                transferChestToPlayerEx(chest, slotIdx, 1);
                            } else {
                                transferChestToPlayer(chest, slotIdx);
                            }
                        }
                        return true;
                    }
                }
            }

            // 3. Player Hotbar slots (9 slots)
            float divY = chestStartY + 4.0f * (cSlotSize + cSlotGap) + 4.0f;
            float hotbarTitleY = divY + 16.0f;
            float hSlotSize = 52.0f;
            float hSlotGap = 8.0f;
            float hotbarStartX = modalX + (modalW - ((float)HOTBAR_SLOTS * hSlotSize + (float)(HOTBAR_SLOTS - 1) * hSlotGap)) * 0.5f;
            float hotbarStartY = hotbarTitleY + 10.0f;

            for (int i = 0; i < HOTBAR_SLOTS; i++) {
                float sx = hotbarStartX + (float)i * (hSlotSize + hSlotGap);
                float sy = hotbarStartY;

                if (mx >= sx && mx <= sx + hSlotSize &&
                    my >= sy && my <= sy + hSlotSize) {
                    if (chest) {
                        if (isRight) {
                            transferPlayerToChestEx(chest, &g_Hotbar[i], 1);
                        } else {
                            transferPlayerToChest(chest, &g_Hotbar[i]);
                        }
                    }
                    return true;
                }
            }

            // Clicked inside modal or outside modal: consume event
            return true;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
            return true;
    }

    return false;
}

static void renderHUD(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Top-Left Status Panel
    drawQuad2D(15, 15, 320, 116, 0.08f, 0.10f, 0.14f, 0.88f);
    drawBorder2D(15, 15, 320, 116, 0.35f, 0.45f, 0.58f, 1.5f);

    BiomeType curBiome = getBiomeAt((int)floorf(g_Player.x));
    drawText(25, 34, g_BiomeNames[curBiome], 1.0f, 0.95f, 0.7f, true);

    char coordBuf[64];
    snprintf(coordBuf, sizeof(coordBuf), "Pos: X: %.1f, Y: %.1f", g_Player.x, g_Player.y);
    drawText(25, 53, coordBuf, 0.8f, 0.9f, 1.0f, false);

    const char* depthName = (g_Player.y <= BEDROCK_LEVEL) ? LOC("基岩地层", "Bedrock") :
                            ((g_Player.y < UNDERWORLD_LEVEL) ? LOC("熔岩地狱", "Underworld") :
                            ((g_Player.y < CAVERN_LEVEL) ? LOC("地下矿洞", "Cavern") :
                            ((g_Player.y < SUBSURFACE_LEVEL) ? LOC("地下土层", "Subsurface") :
                            LOC("地表世界", "Surface"))));
    char depthBuf[64];
    snprintf(depthBuf, sizeof(depthBuf), "%s: %s", LOC("区域", "Zone"), depthName);
    drawText(25, 71, depthBuf, 0.7f, 0.8f, 0.85f, false);

    char modeBadge[128];
    if (getGameMode() == MODE_CREATIVE) {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s %s",
            LOC("当前模式", "Mode"), LOC("创造模式", "Creative"),
            isCreativeFlying() ? LOC("[飞行开启]", "[FLYING]") : LOC("[双击空格飞行]", "[2x SPACE]"));
        drawText(25, 89, modeBadge, isCreativeFlying() ? 0.35f : 1.0f, 0.90f, isCreativeFlying() ? 1.0f : 0.3f, true);
    } else {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s", LOC("当前模式", "Mode"), LOC("生存模式", "Survival"));
        drawText(25, 89, modeBadge, 0.4f, 0.95f, 0.5f, false);
    }
    drawText(25, 107, LOC("[ESC] 暂停/设置 | [F5] 保存 | [F9] 读取 | [H] 帮助", "[ESC] Pause/Settings | [F5] Save | [F9] Load | [H] Help"), 0.65f, 0.75f, 0.85f, false);

    // 2. Novice Tutorial Quest Box (Top-Right)
    if (getGameMode() != MODE_SURVIVAL) {
        renderTutorialQuestBox();
    }

    // 3. Hovered Block Inspector Tooltip
    if (g_TargetValid && g_World[g_TargetX][g_TargetY] != BLOCK_AIR) {
        BlockType bt = g_World[g_TargetX][g_TargetY];
        BlockDef* def = &g_BlockDefs[bt];

        float tipW = 270.0f;
        float tipH = 58.0f;
        float tipX = (float)WINDOW_WIDTH * 0.5f - tipW * 0.5f;
        float tipY = (float)WINDOW_HEIGHT - 170.0f;

        drawQuad2D(tipX, tipY, tipW, tipH, 0.06f, 0.08f, 0.12f, 0.88f);
        drawBorder2D(tipX, tipY, tipW, tipH, 0.25f, 0.75f, 0.95f, 1.5f);

        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s",
            (getLanguage() == LANG_ZH) ? def->chineseName : def->name);
        drawText(tipX + 12, tipY + 22, nameBuf, 1.0f, 1.0f, 1.0f, true);

        char statBuf[128];
        const char* dropStr = (getLanguage() == LANG_ZH) ? def->chineseName : def->dropName;
        snprintf(statBuf, sizeof(statBuf), "%s: %.1f  %s: %s  [%s: %s]",
            LOC("硬度", "Hardness"), def->hardness,
            LOC("掉落", "Drop"), dropStr,
            LOC("触及范围", "In Reach"), LOC("是", "YES"));
        drawText(tipX + 12, tipY + 42, statBuf, 0.3f, 0.95f, 0.4f, false);
    }

    // 4. Hotbar at Bottom-Center
    float slotSize = 52.0f;
    float slotGap = 6.0f;
    float totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * slotGap;
    float startX = ((float)WINDOW_WIDTH - totalW) * 0.5f;
    float startY = (float)WINDOW_HEIGHT - slotSize - 20.0f;

    int actType = g_Hotbar[g_ActiveSlot].type;
    int actCount = g_Hotbar[g_ActiveSlot].count;

    if (getGameMode() == MODE_CREATIVE) {
        float flyW = 450.0f;
        float flyH = 24.0f;
        float flyX = ((float)WINDOW_WIDTH - flyW) * 0.5f;
        float flyY = startY - 58.0f;
        drawQuad2D(flyX, flyY, flyW, flyH, 0.08f, 0.10f, 0.15f, 0.88f);
        drawBorder2D(flyX, flyY, flyW, flyH, 0.35f, 0.85f, 1.0f, 1.5f);
        if (isCreativeFlying()) {
            drawText(flyX + 15, flyY + 16, LOC("[创造飞行中] 空格: 上升 | Shift/S: 下降 | 双击空格: 着陆", "[FLYING ACTIVE] Space: Up | Shift/S: Down | 2x Space: Land"), 0.35f, 0.95f, 1.0f, true);
        } else {
            drawText(flyX + 18, flyY + 16, LOC("[创造模式] 双击空格开启飞行 | 无限方块放置", "[CREATIVE MODE] Double-tap Space to Fly | Infinite Blocks"), 1.0f, 0.85f, 0.35f, false);
        }
    }

    // Requirement 4: Selected Item HUD Tooltip / Info Card
    // Above the active hotbar slot (or neatly in HUD), render an info card showing: Item Name, Item Category, Attack Damage / Defense % / Durability (X / Y).
    {
        float actSlotCenterX = startX + g_ActiveSlot * (slotSize + slotGap) + slotSize * 0.5f;
        float cardW = 320.0f;
        float cardH = 62.0f;
        float cardX = actSlotCenterX - cardW * 0.5f;
        if (cardX < 15.0f) cardX = 15.0f;
        if (cardX + cardW > (float)WINDOW_WIDTH - 15.0f) cardX = (float)WINDOW_WIDTH - 15.0f - cardW;

        float cardY = startY - cardH - 10.0f;
        if (getGameMode() == MODE_CREATIVE) {
            cardY = startY - 58.0f - cardH - 6.0f;
        }

        // Info card container & golden border
        drawQuad2D(cardX, cardY, cardW, cardH, 0.07f, 0.09f, 0.14f, 0.94f);
        drawBorder2D(cardX, cardY, cardW, cardH, 0.95f, 0.80f, 0.22f, 1.5f);

        // Indicator notch pointing toward active hotbar slot
        drawQuad2D(actSlotCenterX - 5.0f, cardY + cardH, 10.0f, 3.0f, 0.95f, 0.80f, 0.22f, 1.0f);

        char nameLine[128];
        char catLine[128];
        char statLine[128];

        int curDur = g_Hotbar[g_ActiveSlot].durability;
        int maxDur = g_Hotbar[g_ActiveSlot].maxDurability;

        if (actType != BLOCK_AIR && actType != 0 && actCount > 0) {
            if (isToolItem(actType)) {
                const ToolDef* td = getToolDef(actType);
                const char* tName = (getLanguage() == LANG_ZH) ? (td ? td->chineseName : "工具") : (td ? td->name : "Tool");
                snprintf(nameLine, sizeof(nameLine), "[%d] %s", g_ActiveSlot + 1, tName);

                if (td && td->type == TOOL_SWORD) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【近战武器】 锋利战剑", "[Weapon] Melee Sword"));
                } else if (td && td->type == TOOL_PICKAXE) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【开采工具】 矿工镐 (开采岩石/矿脉)", "[Tool] Mining Pickaxe"));
                } else if (td && td->type == TOOL_AXE) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【伐木工具】 伐木斧 (砍伐树木/木材)", "[Tool] Logging Axe"));
                } else {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【挖掘工具】 工兵铲 (挖掘泥土/沙石)", "[Tool] Excavation Shovel"));
                }

                int mDur = (maxDur > 0) ? maxDur : getMaxDurability(actType);
                int cDur = (curDur > 0) ? curDur : mDur;
                float dmg = td ? td->damage : 5.0f;
                snprintf(statLine, sizeof(statLine), "%s: %.1f  |  %s: %d / %d",
                         LOC("攻击伤害", "Attack"), dmg,
                         LOC("耐久度", "Durability"), cDur, mDur);

            } else if (isArmorItem(actType)) {
                const ArmorDef* ad = getArmorDef(actType);
                const char* aName = (getLanguage() == LANG_ZH) ? (ad ? ad->chineseName : "防具") : (ad ? ad->name : "Armor");
                snprintf(nameLine, sizeof(nameLine), "[%d] %s", g_ActiveSlot + 1, aName);

                int slotIdx = ad ? ad->slot : -1;
                if (slotIdx == ARMOR_SLOT_HELMET) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【防具装备】 头部盔甲 (按E装备)", "[Armor] Helmet (Equip in E)"));
                } else if (slotIdx == ARMOR_SLOT_CHEST) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【防具装备】 护胸板甲 (按E装备)", "[Armor] Chestplate (Equip in E)"));
                } else if (slotIdx == ARMOR_SLOT_LEGS) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【防具装备】 腿部护甲 (按E装备)", "[Armor] Leggings (Equip in E)"));
                } else {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【防具装备】 战靴胫甲 (按E装备)", "[Armor] Boots (Equip in E)"));
                }

                int mDur = (maxDur > 0) ? maxDur : (ad ? ad->maxDurability : 100);
                int cDur = (curDur > 0) ? curDur : mDur;
                float defPct = ad ? (ad->reduction * 100.0f) : 0.0f;
                snprintf(statLine, sizeof(statLine), "%s: +%.0f%%  |  %s: %d / %d",
                         LOC("防御减免", "Defense"), defPct,
                         LOC("耐久度", "Durability"), cDur, mDur);

            } else if (actType == ITEM_STICKY_BOMB) {
                snprintf(nameLine, sizeof(nameLine), "[%d] %s  x%d",
                         g_ActiveSlot + 1, (getLanguage() == LANG_ZH) ? "粘性炸弹" : "Sticky Bomb", actCount);
                snprintf(catLine, sizeof(catLine), "%s", LOC("【投掷消耗品】 炸药武器", "[Consumable] Thrown Explosive"));
                snprintf(statLine, sizeof(statLine), "%s: 50.0  |  %s: %d",
                         LOC("爆炸伤害", "Explosion Dmg"), LOC("携带数量", "Count"), actCount);

            } else if (actType == ITEM_STICK || actType == ITEM_GEL) {
                const char* mName = (getLanguage() == LANG_ZH) ? getItemChineseName(actType) : getItemName(actType);
                snprintf(nameLine, sizeof(nameLine), "[%d] %s  x%d", g_ActiveSlot + 1, mName, actCount);
                snprintf(catLine, sizeof(catLine), "%s", LOC("【基础资源】 合成原料", "[Resource] Crafting Material"));
                snprintf(statLine, sizeof(statLine), "%s: %d  |  %s",
                         LOC("堆叠数量", "Stack Count"), actCount,
                         LOC("用于制作工具、武器与火把", "For Tools, Weapons & Torches"));

            } else if (actType < BLOCK_TYPE_COUNT) {
                const char* bName = (getLanguage() == LANG_ZH) ? g_BlockDefs[actType].chineseName : g_BlockDefs[actType].name;
                snprintf(nameLine, sizeof(nameLine), "[%d] %s  x%d", g_ActiveSlot + 1, bName, actCount);

                if (isDoorBlock(actType)) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【功能设施】 双格木门 (右键开/关门)", "[Furniture] 2-Block Door (R-Click Toggle)"));
                    snprintf(statLine, sizeof(statLine), "%s: 2.0x1.0  |  %s: %s",
                             LOC("方块尺寸", "Block Size"),
                             LOC("通行状态", "Passage"),
                             g_BlockDefs[actType].isSolid ? LOC("关闭 (阻挡)", "Closed (Solid)") : LOC("开启 (通行)", "Open (Passable)"));
                } else if (actType == BLOCK_WORKBENCH) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【合成站】 工作台 (右键直接进入合成)", "[Station] Workbench (R-Click Craft)"));
                    snprintf(statLine, sizeof(statLine), "%s: 2.0  |  %s",
                             LOC("方块硬度", "Hardness"),
                             LOC("解锁高级工具与装备配方", "Unlocks Advanced Recipes"));
                } else if (actType == BLOCK_FURNACE) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【合成站】 石制熔炉 (右键直接进入合成)", "[Station] Furnace (R-Click Craft)"));
                    snprintf(statLine, sizeof(statLine), "%s: 6.0  |  %s",
                             LOC("方块硬度", "Hardness"),
                             LOC("用于冶炼矿物与高级加工", "Smelting & Metalworking"));
                } else if (actType >= BLOCK_CHAIR && actType <= BLOCK_CHANDELIER) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【功能家具】 庇护所设施", "[Furniture] Shelter Decor"));
                    snprintf(statLine, sizeof(statLine), "%s: %.1f  |  %s: %s",
                             LOC("方块硬度", "Hardness"), g_BlockDefs[actType].hardness,
                             LOC("阻挡通行", "Solid"), g_BlockDefs[actType].isSolid ? LOC("是", "Yes") : LOC("否", "No"));
                } else if (actType == BLOCK_TORCH || actType == BLOCK_GLOW_SHROOM) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【照明设施】 动态光源", "[Lighting] Dynamic Light"));
                    snprintf(statLine, sizeof(statLine), "%s: %.1f  |  %s",
                             LOC("发光强度", "Light Intensity"), 1.0f,
                             LOC("提供大范围温暖照明", "Provides Wide Illumination"));
                } else if (actType >= BLOCK_COAL_ORE && actType <= BLOCK_RUBY_ORE) {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【矿脉资源】 贵重矿石", "[Mineral] Ore Deposit"));
                    snprintf(statLine, sizeof(statLine), "%s: %.1f (需镐开采)  |  %s: %d",
                             LOC("方块硬度", "Hardness"), g_BlockDefs[actType].hardness,
                             LOC("数量", "Count"), actCount);
                } else {
                    snprintf(catLine, sizeof(catLine), "%s", LOC("【建筑方块】 基础建材", "[Block] Building Material"));
                    snprintf(statLine, sizeof(statLine), "%s: %.1f  |  %s: %s  |  %s: %d",
                             LOC("硬度", "Hardness"), g_BlockDefs[actType].hardness,
                             LOC("实体碰撞", "Solid"), g_BlockDefs[actType].isSolid ? LOC("是", "Yes") : LOC("否", "No"),
                             LOC("数量", "Count"), actCount);
                }
            } else {
                snprintf(nameLine, sizeof(nameLine), "[%d] %s  x%d", g_ActiveSlot + 1, getItemLocalizedName(actType), actCount);
                snprintf(catLine, sizeof(catLine), "%s", LOC("【通用物品】", "[Item] General"));
                snprintf(statLine, sizeof(statLine), "%s: %d", LOC("堆叠数量", "Count"), actCount);
            }
        } else {
            snprintf(nameLine, sizeof(nameLine), "[%d] %s", g_ActiveSlot + 1, LOC("空手 / 未选择物品", "Bare Hands / Empty Slot"));
            snprintf(catLine, sizeof(catLine), "%s", LOC("【当前未装备任何道具】", "[No Item Selected]"));
            snprintf(statLine, sizeof(statLine), "%s: 5.0  |  %s: 1.0x",
                     LOC("基础拳击伤害", "Fist Damage"),
                     LOC("击退力", "Knockback"));
        }

        // Render 3 lines of crisp text
        drawText(cardX + 14.0f, cardY + 16.0f, nameLine, 1.0f, 0.90f, 0.30f, true);
        drawText(cardX + 14.0f, cardY + 34.0f, catLine, 0.38f, 0.85f, 1.0f, false);
        drawText(cardX + 14.0f, cardY + 52.0f, statLine, 0.45f, 0.95f, 0.50f, false);
    }

    drawQuad2D(startX - 10, startY - 4, totalW + 20, slotSize + 10, 0.08f, 0.10f, 0.14f, 0.92f);
    drawBorder2D(startX - 10, startY - 4, totalW + 20, slotSize + 10, 0.30f, 0.38f, 0.48f, 1.5f);

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        float sx = startX + i * (slotSize + slotGap);
        float sy = startY;

        drawQuad2D(sx, sy, slotSize, slotSize, 0.15f, 0.18f, 0.24f, 0.95f);

        if (i == g_ActiveSlot) {
            drawBorder2D(sx - 2, sy - 2, slotSize + 4, slotSize + 4, 1.0f, 0.85f, 0.20f, 3.0f);
        } else {
            drawBorder2D(sx, sy, slotSize, slotSize, 0.35f, 0.42f, 0.52f, 1.0f);
        }

        int bt = g_Hotbar[i].type;
        if (bt != BLOCK_AIR && bt != 0 && g_Hotbar[i].count > 0) {
            if (isToolItem(bt) || isArmorItem(bt)) {
                drawInventoryItemIcon(sx + 6.0f, sy + 6.0f, slotSize - 12.0f, bt);
            } else if (bt < BLOCK_TYPE_COUNT) {
                float u0, v0, u1, v1;
                int texIdx = GetBlockFaceTextureIndex(bt, 0);
                GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);

                float qx = sx + 8.0f, qy = sy + 8.0f, qw = slotSize - 16.0f, qh = slotSize - 16.0f;
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, g_AtlasTex);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glTexCoord2f(u0, v1); glVertex2f(qx, qy);
                glTexCoord2f(u1, v1); glVertex2f(qx + qw, qy);
                glTexCoord2f(u1, v0); glVertex2f(qx + qw, qy + qh);
                glTexCoord2f(u0, v0); glVertex2f(qx, qy + qh);
                glEnd();
                glDisable(GL_TEXTURE_2D);

                drawBorder2D(qx, qy, qw, qh, 0.0f, 0.0f, 0.0f, 1.0f);

                char countBuf[16];
                snprintf(countBuf, sizeof(countBuf), "%d", g_Hotbar[i].count);
                drawText(sx + slotSize - 22, sy + slotSize - 6, countBuf, 1.0f, 1.0f, 1.0f, false);
            } else {
                drawInventoryItemIcon(sx + 6.0f, sy + 6.0f, slotSize - 12.0f, bt);
                char countBuf[16];
                snprintf(countBuf, sizeof(countBuf), "%d", g_Hotbar[i].count);
                drawText(sx + slotSize - 22, sy + slotSize - 6, countBuf, 1.0f, 1.0f, 1.0f, false);
            }

            // Durability Bar (Green >50%, Yellow >20%, Red <=20%)
            if (g_Hotbar[i].durability > 0 && g_Hotbar[i].durability < g_Hotbar[i].maxDurability && g_Hotbar[i].maxDurability > 0) {
                float pct = (float)g_Hotbar[i].durability / (float)g_Hotbar[i].maxDurability;
                if (pct > 1.0f) pct = 1.0f;
                float barPad = 6.0f;
                float barW = slotSize - barPad * 2.0f;
                float barH = 2.0f;
                float barX = sx + barPad;
                float barY = sy + slotSize - 6.0f;
                drawQuad2D(barX, barY - 1.0f, barW, barH + 2.0f, 0.0f, 0.0f, 0.0f, 0.80f);
                float r = 0.20f, g = 0.90f, b = 0.20f;
                if (pct <= 0.20f) {
                    r = 0.95f; g = 0.20f; b = 0.20f;
                } else if (pct <= 0.50f) {
                    r = 0.95f; g = 0.85f; b = 0.15f;
                }
                drawQuad2D(barX, barY, barW * pct, barH, r, g, b, 1.0f);
            }
        }

        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        drawText(sx + 4, sy + 14, numBuf, 0.7f, 0.7f, 0.7f, false);
    }

    // 0. Player Health Bar (Survival Mode - cleanly docked beneath top-left status panel)
    if (getGameMode() == MODE_SURVIVAL) {
        float barX = 15.0f;
        float barY = 138.0f;
        float barW = 320.0f;
        float barH = 28.0f;

        // Background
        drawQuad2D(barX, barY, barW, barH, 0.08f, 0.10f, 0.14f, 0.90f);
        drawBorder2D(barX, barY, barW, barH, 0.35f, 0.45f, 0.58f, 1.5f);

        // Fill
        float hpPct = g_Player.health / (g_Player.maxHealth > 0.0f ? g_Player.maxHealth : 100.0f);
        if (hpPct < 0.0f) hpPct = 0.0f;
        if (hpPct > 1.0f) hpPct = 1.0f;
        float fillW = (barW - 4.0f) * hpPct;
        if (fillW > 0.0f) {
            float fR = (g_Player.hurtTimer > 0.0f) ? 1.0f : 0.88f;
            float fG = (g_Player.hurtTimer > 0.0f) ? 0.75f : 0.18f;
            float fB = (g_Player.hurtTimer > 0.0f) ? 0.75f : 0.22f;
            drawQuad2D(barX + 2.0f, barY + 2.0f, fillW, barH - 4.0f, fR, fG, fB, 0.95f);
        }

        // Text with centered baseline & natural regen feedback
        char hpBuf[128];
        if (g_Player.health >= g_Player.maxHealth) {
            snprintf(hpBuf, sizeof(hpBuf), "❤️ %s: %d / %d", LOC("生命值", "HP"), (int)ceilf(g_Player.health), (int)g_Player.maxHealth);
        } else if (g_Player.timeSinceCombat >= 20.0f) {
            snprintf(hpBuf, sizeof(hpBuf), "❤️ %s: %d / %d  [%s]", LOC("生命值", "HP"), (int)ceilf(g_Player.health), (int)g_Player.maxHealth, LOC("自然回血中 +3.5/s", "Regen +3.5/s"));
        } else {
            snprintf(hpBuf, sizeof(hpBuf), "❤️ %s: %d / %d", LOC("生命值", "HP"), (int)ceilf(g_Player.health), (int)g_Player.maxHealth);
        }
        drawText(barX + 10.0f, barY + 19.0f, hpBuf, 1.0f, 1.0f, 1.0f, true);
    }

    // HUD Time of Day Clock Widget (User Request 3)
    {
        float normTime = g_WorldTime - floorf(g_WorldTime);
        if (normTime < 0.0f) normTime += 1.0f;
        int curHour = ((int)(normTime * 24.0f)) % 24;
        int curMin = ((int)(fmodf(normTime * 1440.0f, 60.0f))) % 60;
        bool isDay = (curHour >= 6 && curHour < 18);

        char clockBuf[32];
        snprintf(clockBuf, sizeof(clockBuf), "%s %02d:%02d", isDay ? "☀️" : "🌙", curHour, curMin);

        float clockX = 15.0f;
        float clockY = (getGameMode() == MODE_SURVIVAL) ? 172.0f : 138.0f;
        float clockW = 126.0f;
        float clockH = 30.0f;

        // Clean semi-transparent background and gold border
        drawQuad2D(clockX, clockY, clockW, clockH, 0.08f, 0.10f, 0.14f, 0.88f);
        drawBorder2D(clockX, clockY, clockW, clockH, 0.95f, 0.80f, 0.22f, 1.5f);
        drawText(clockX + 14.0f, clockY + 20.0f, clockBuf, isDay ? 1.0f : 0.82f, isDay ? 0.92f : 0.88f, isDay ? 0.35f : 1.0f, true);
    }

    // Game Over Overlay
    if (g_Player.isDead) {
        drawQuad2D(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.45f, 0.05f, 0.05f, 0.65f);

        const char* dTitle = LOC("💀 你阵亡了！", "💀 YOU DIED!");
        float dtw = getTextWidthUTF8(dTitle, FONT_SIZE_DEATH, true);
        drawTextUTF8(((float)WINDOW_WIDTH - dtw) * 0.5f, (float)WINDOW_HEIGHT * 0.38f, dTitle, 1.0f, 0.25f, 0.25f, FONT_SIZE_DEATH, true);

        const char* dPrompt = LOC("[ 点击此处 或 按 R 键 立即重生 ]", "[ Click Here or Press R to Respawn ]");
        float dpw = getTextWidth(dPrompt, true);
        float dbtnX = ((float)WINDOW_WIDTH - dpw - 40.0f) * 0.5f;
        float dbtnY = (float)WINDOW_HEIGHT * 0.48f;
        float dbtnW = dpw + 40.0f;
        float dbtnH = 46.0f;

        drawQuad2D(dbtnX, dbtnY, dbtnW, dbtnH, 0.22f, 0.10f, 0.10f, 0.92f);
        drawBorder2D(dbtnX, dbtnY, dbtnW, dbtnH, 1.0f, 0.40f, 0.30f, 2.0f);
        drawText(dbtnX + 20.0f, dbtnY + 28.0f, dPrompt, 1.0f, 0.95f, 0.40f, true);
    }

    // 5. Mining Progress Bar
    if (g_MiningProgress > 0.0f) {
        float bw = 200.0f;
        float bh = 14.0f;
        float bx = ((float)WINDOW_WIDTH - bw) * 0.5f;
        float by = startY - 55.0f;
        drawQuad2D(bx - 2, by - 2, bw + 4, bh + 4, 0.1f, 0.1f, 0.1f, 0.85f);
        drawQuad2D(bx, by, bw * g_MiningProgress, bh, 1.0f, 0.65f, 0.15f, 1.0f);
        drawBorder2D(bx, by, bw, bh, 1.0f, 1.0f, 1.0f, 1.0f);
        drawText(bx + 60, by + 11, LOC("挖掘中...", "MINING..."), 0.1f, 0.1f, 0.1f, false);
    }

    // 6. Notification Toast Banner (Top Center)
    if (g_NotificationTimer > 0.0f) {
        float notW = 460.0f;
        float notH = 34.0f;
        float notX = ((float)WINDOW_WIDTH - notW) * 0.5f;
        float notY = 15.0f;
        drawQuad2D(notX, notY, notW, notH, 0.08f, 0.10f, 0.14f, 0.94f);
        drawBorder2D(notX, notY, notW, notH, g_NotificationR, g_NotificationG, g_NotificationB, 2.0f);
        drawText(notX + 20, notY + 22, g_NotificationText, g_NotificationR, g_NotificationG, g_NotificationB, true);
    }

    // 7. Floating Texts (e.g. "+1 Diamond")
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (g_FloatingTexts[i].active) {
            float ndcX = (g_FloatingTexts[i].x - g_CamX) / (tanf(55.0f * (float)M_PI / 360.0f) * g_CamDist * ((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT));
            float ndcY = (g_FloatingTexts[i].y - g_CamY) / (tanf(55.0f * (float)M_PI / 360.0f) * g_CamDist);
            float sx = (ndcX * 0.5f + 0.5f) * (float)WINDOW_WIDTH;
            float sy = (1.0f - (ndcY * 0.5f + 0.5f)) * (float)WINDOW_HEIGHT;
            drawText(sx, sy, g_FloatingTexts[i].text, g_FloatingTexts[i].r, g_FloatingTexts[i].g, g_FloatingTexts[i].b, true);
        }
    }

    // 8. Help Overlay
    if (g_ShowHelp) {
        float hw = 520.0f;
        float hh = 370.0f;
        float hx = ((float)WINDOW_WIDTH - hw) * 0.5f;
        float hy = ((float)WINDOW_HEIGHT - hh) * 0.5f;
        drawQuad2D(hx, hy, hw, hh, 0.06f, 0.08f, 0.12f, 0.96f);
        drawBorder2D(hx, hy, hw, hh, 0.35f, 0.75f, 0.95f, 2.0f);

        drawText(hx + 20, hy + 28, LOC("操作指南与快捷键", "KEYBINDS & CONTROLS"), 0.35f, 0.85f, 1.0f, true);
        drawText(hx + 20, hy + 58, LOC("[A / D] 或 [左右方向键]  : 左右移动 (支持1格高度自动攀爬)", "[A / D] or [Left/Right] : Move (1-block auto climb)"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 82, LOC("[空格] 或 [W]           : 跳跃 (长按可跳跃更高)", "[Space] or [W]          : Variable Jump"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 106, LOC("[鼠标左键]              : 挖掘 / 破坏方块 / 攻击生物", "[Left Click]            : Mine / Destroy Block / Attack"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 130, LOC("[鼠标右键]              : 放置当前选中的方块", "[Right Click]           : Place Selected Block"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 154, LOC("[1 - 9] 或 鼠标滚轮     : 切换快捷栏栏位", "[1 - 9] or Mouse Wheel  : Switch Active Hotbar Slot"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 178, LOC("[F5]                    : 快速保存世界存档", "[F5]                    : Quick Save Game"), 0.3f, 0.95f, 0.4f, false);
        drawText(hx + 20, hy + 202, LOC("[F9]                    : 快速读取世界存档", "[F9]                    : Quick Load Game"), 0.3f, 0.85f, 1.0f, false);
        drawText(hx + 20, hy + 226, LOC("[C]                     : 打开 / 关闭合成工作台", "[C]                     : Toggle Crafting Station"), 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 250, LOC("[E]                     : 打开 / 关闭创造模式物品库", "[E]                     : Toggle Creative Inventory"), 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 274, LOC("[ESC]                   : 暂停菜单 / 切换模式 / 游戏设置", "[ESC]                   : Pause Menu / Mode / Settings"), 1.0f, 0.85f, 0.30f, false);
        drawText(hx + 20, hy + 298, LOC("[R]                     : 立即重生并返回地表安全点", "[R]                     : Quick Respawn to Surface"), 1.0f, 0.85f, 0.2f, false);
        drawText(hx + 20, hy + 322, LOC("[H] 或 [F1]             : 打开 / 关闭本帮助菜单", "[H] or [F1]             : Toggle This Help Menu"), 0.8f, 0.8f, 0.8f, false);
        drawText(hx + 20, hy + 348, LOC("游戏退出时自动保存。独立 2.5D OpenGL 引擎。", "Auto-saves on exit. Standalone 2.5D OpenGL Engine."), 0.6f, 0.6f, 0.6f, false);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// Window Procedure & Main Entrypoint
// ============================================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && (wParam == VK_F11 || (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000)))) {
        toggleFullscreen(hwnd);
        return 0;
    }

    if (uMsg == WM_SIZE) {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            g_WindowWidth = w;
            g_WindowHeight = h;
            glViewport(0, 0, g_WindowWidth, g_WindowHeight);
        }
        return 0;
    }

    if (uMsg == WM_GETMINMAXINFO) {
        LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
        mmi->ptMinTrackSize.x = 960;
        mmi->ptMinTrackSize.y = 540;
        return 0;
    }

    if (handleChestInput(uMsg, wParam, lParam)) {
        return 0;
    }
    if (handleCraftingInput(uMsg, wParam, lParam)) {
        return 0;
    }
    if (handleCreativeInventoryMsg(uMsg, wParam, lParam)) {
        return 0;
    }
    if (handleMenuInput(uMsg, wParam, lParam)) {
        return 0;
    }
    switch (uMsg) {
        case WM_CLOSE:
            saveGame();
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam < 256) g_Keys[wParam] = true;
            if (wParam >= '1' && wParam <= '9') {
                g_ActiveSlot = (int)(wParam - '1');
                playSound(SFX_CLICK);
            }
            if (wParam == 'H' || wParam == VK_F1) {
                g_ShowHelp = !g_ShowHelp;
            }
            if (wParam == VK_F5) {
                saveGame();
                if (g_TutorialStep == TUTORIAL_EXPLORE) {
                    g_TutorialStep = TUTORIAL_COMPLETE;
                    playSound(SFX_CRAFT);
                }
            }
            if (wParam == VK_F9) {
                loadGame();
            }
            if (wParam == 'R') {
                if (g_Player.isDead) {
                    respawnPlayer();
                } else if (getGameMode() == MODE_CREATIVE) {
                    int sx = (int)floorf(g_Player.x);
                    g_Player.y = (float)(getSurfaceHeight(sx, g_Seed) + 3);
                    g_Player.vx = 0.0f;
                    g_Player.vy = 0.0f;
                    showNotification(LOC("已在地面安全区域重生！", "Respawned on surface safe ground!"), 0.4f, 0.8f, 1.0f);
                } else {
                    showNotification(LOC("仅创造模式可使用地表传送！", "Surface teleport is only available in Creative Mode!"), 1.0f, 0.4f, 0.4f);
                }
            }
            return 0;

        case WM_KEYUP:
            if (wParam < 256) g_Keys[wParam] = false;
            return 0;

        case WM_MOUSEMOVE:
            g_MouseScreenX = (int)(short)LOWORD(lParam);
            g_MouseScreenY = (int)(short)HIWORD(lParam);
            return 0;

        case WM_LBUTTONDOWN:
            if (g_Player.isDead) {
                respawnPlayer();
                return 0;
            }
            g_MouseLeft = true;
            return 0;

        case WM_LBUTTONUP:
            g_MouseLeft = false;
            return 0;

        case WM_RBUTTONDOWN:
            g_MouseRight = true;
            return 0;

        case WM_RBUTTONUP:
            g_MouseRight = false;
            return 0;

        case WM_MOUSEWHEEL: {
            short delta = (short)HIWORD(wParam);
            if (delta > 0) {
                g_ActiveSlot = (g_ActiveSlot + 1) % HOTBAR_SLOTS;
                playSound(SFX_CLICK);
            } else if (delta < 0) {
                g_ActiveSlot = (g_ActiveSlot - 1 + HOTBAR_SLOTS) % HOTBAR_SLOTS;
                playSound(SFX_CLICK);
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    memset(&g_PrevPlacement, 0, sizeof(g_PrevPlacement));
    g_PrevPlacement.length = sizeof(g_PrevPlacement);

    // Enable Per-Monitor V2 DPI awareness for razor-sharp rendering on high-DPI displays
    HMODULE u32 = GetModuleHandleA("user32.dll");
    if (u32) {
        typedef BOOL (WINAPI *SetDpiContextProc)(HANDLE);
        FARPROC fpCtx = GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (fpCtx) {
            SetDpiContextProc pSetDpiCtx = (SetDpiContextProc)(void*)fpCtx;
            pSetDpiCtx((HANDLE)-4);
        } else {
            typedef BOOL (WINAPI *SetDpiAwareProc)(VOID);
            FARPROC fpDpi = GetProcAddress(u32, "SetProcessDPIAware");
            if (fpDpi) {
                SetDpiAwareProc pSetDpi = (SetDpiAwareProc)(void*)fpDpi;
                pSetDpi();
            }
        }
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "LuminoraSandboxClass";
    RegisterClassEx(&wc);

    RECT wr = { 0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    int winW = wr.right - wr.left;
    int winH = wr.bottom - wr.top;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;
    if (winX < 0) winX = 0;
    if (winY < 0) winY = 0;

    HWND hwnd = CreateWindowEx(
        0,
        "LuminoraSandboxClass",
        "Luminora | 流明洛原",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        winX, winY,
        winW, winH,
        NULL, NULL, hInstance, NULL
    );
    g_MainHwnd = hwnd;

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);
    glViewport(0, 0, g_WindowWidth, g_WindowHeight);

    initFontRenderer();

    initSoundEngine();
    g_AtlasTex = CreateTextureAtlasGL();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);

    loadSettings();
    initGame();
    initMenuAndCreative();

    LARGE_INTEGER freq, lastTime, curTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    MSG msg;
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        QueryPerformanceCounter(&curTime);
        float dt = (float)(curTime.QuadPart - lastTime.QuadPart) / (float)freq.QuadPart;
        lastTime = curTime;
        if (dt > 0.05f) dt = 0.05f;

        if (getGameState() == STATE_PLAYING) {
            updatePhysics(dt);
            updateInteraction(dt);
        } else if (getGameState() == STATE_MENU) {
            g_CamX += dt * 3.5f;
            if (g_CamX > (float)WORLD_WIDTH - 25.0f) g_CamX = 25.0f;
            g_WorldTime += dt / 300.0f;
            if (g_WorldTime > 1.0f) g_WorldTime -= 1.0f;
        }

        // Update Procedural Background Music (BGM) context
        {
            bool inMenu = (getGameState() == STATE_MENU ||
                           getGameState() == STATE_WORLDS_LIST ||
                           getGameState() == STATE_CREATE_WORLD ||
                           (getGameState() == STATE_SETTINGS && getPreviousGameState() != STATE_PAUSED));
            int px = (int)g_Player.x;
            if (px < 0) px = 0;
            if (px >= WORLD_WIDTH) px = WORLD_WIDTH - 1;
            float surfaceY = (float)g_SurfaceHeight[px];
            float sunAngle = g_WorldTime * (float)M_PI * 2.0f;
            bool isDay = (sinf(sunAngle) > 0.0f);
            int curBiome = (int)getBiomeAt(px);
            updateBgmContext(inMenu, g_Player.y, surfaceY, isDay, curBiome);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        setPerspective(55.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 500.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(-g_CamX, -g_CamY, -g_CamDist);

        glDepthMask(GL_FALSE);
        renderBackgroundParallax();
        glDepthMask(GL_TRUE);

        renderWorld25D();

        if (getGameState() == STATE_PLAYING) {
            renderHUD();
            if (g_ChestOpen) {
                renderChestUI();
            } else if (isCraftingOpen()) {
                renderCraftingUI();
            } else if (isCreativeInventoryOpen()) {
                renderCreativeInventory(WINDOW_WIDTH, WINDOW_HEIGHT, g_MouseScreenX, g_MouseScreenY);
            }
        } else if (getGameState() == STATE_MENU) {
            renderMainMenu();
        } else if (getGameState() == STATE_WORLDS_LIST) {
            renderWorldsListDialog();
        } else if (getGameState() == STATE_CREATE_WORLD) {
            renderCreateWorldDialog();
        } else if (getGameState() == STATE_PAUSED) {
            renderHUD();
            renderPauseMenu();
        } else if (getGameState() == STATE_SETTINGS) {
            if (getPreviousGameState() == STATE_PAUSED) {
                renderHUD();
            }
            renderSettingsDialog();
        }

        SwapBuffers(hdc);
        Sleep(1);
    }

    cleanupFontRenderer();
    cleanupSoundEngine();

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    return 0;
}
