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
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
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
    { "Grass Block",    "草方块",     "泥土",       0.26f, 0.68f, 0.22f, 0.6f, true,  false, false, 0,0,0 },
    { "Dirt Block",     "泥土",       "泥土",       0.52f, 0.35f, 0.22f, 0.5f, true,  false, false, 0,0,0 },
    { "Stone Block",    "石块",       "原石",       0.50f, 0.52f, 0.54f, 1.5f, true,  false, false, 0,0,0 },
    { "Wood Log",       "原木",       "原木",       0.58f, 0.38f, 0.20f, 1.0f, false, true,  false, 0,0,0 },
    { "Leaves",         "树叶",       "树叶",       0.18f, 0.55f, 0.20f, 0.3f, true,  true,  false, 0,0,0 },
    { "Sand Block",     "沙块",       "沙块",       0.88f, 0.78f, 0.52f, 0.4f, true,  false, false, 0,0,0 },
    { "Sandstone",      "砂岩",       "砂岩",       0.75f, 0.64f, 0.42f, 1.2f, true,  false, false, 0,0,0 },
    { "Cactus",         "仙人掌",     "仙人掌",     0.22f, 0.62f, 0.28f, 0.5f, true,  false, false, 0,0,0 },
    { "Snow Block",     "雪块",       "雪球",       0.92f, 0.95f, 0.98f, 0.3f, true,  false, false, 0,0,0 },
    { "Ice Block",      "坚冰",       "冰块",       0.65f, 0.85f, 0.95f, 0.8f, true,  true,  false, 0,0,0 },
    { "Clay Block",     "粘土",       "粘土块",     0.68f, 0.52f, 0.45f, 0.7f, true,  false, false, 0,0,0 },
    { "Coal Ore",       "煤矿石",     "煤炭",       0.25f, 0.25f, 0.27f, 1.8f, true,  false, false, 0,0,0 },
    { "Iron Ore",       "铁矿石",     "铁矿",       0.72f, 0.58f, 0.50f, 2.2f, true,  false, false, 0,0,0 },
    { "Gold Ore",       "金矿石",     "金矿",       0.95f, 0.82f, 0.22f, 2.8f, true,  false, false, 0,0,0 },
    { "Diamond Ore",    "钻石矿石",   "璀璨钻石",   0.28f, 0.88f, 0.95f, 3.5f, true,  false, true,  0.2f, 0.6f, 0.8f },
    { "Ruby Ore",       "红宝石矿",   "红宝石",     0.92f, 0.18f, 0.28f, 3.0f, true,  false, true,  0.8f, 0.1f, 0.2f },
    { "Stone Brick",    "石砖",       "石砖",       0.72f, 0.38f, 0.30f, 2.0f, true,  false, false, 0,0,0 },
    { "Wood Plank",     "木板",       "木板",       0.68f, 0.48f, 0.28f, 0.8f, true,  false, false, 0,0,0 },
    { "Glass Block",    "透明玻璃",   "玻璃",       0.75f, 0.90f, 0.98f, 0.3f, true,  true,  false, 0,0,0 },
    { "Torch",          "火把",       "火把",       1.00f, 0.85f, 0.25f, 0.1f, false, true,  true,  1.0f, 0.85f, 0.3f },
    { "Obsidian",       "黑曜石",     "黑曜石",     0.16f, 0.12f, 0.22f, 5.0f, true,  false, false, 0,0,0 },
    { "Magma Block",    "熔岩块",     "岩浆块",     0.90f, 0.32f, 0.08f, 2.5f, true,  false, true,  0.9f, 0.35f, 0.1f },
    { "Mossy Stone",    "苔石",       "苔石",       0.42f, 0.52f, 0.40f, 1.6f, true,  false, false, 0,0,0 },
    { "Mud Block",      "泥浆块",     "泥浆",       0.40f, 0.30f, 0.22f, 0.5f, true,  false, false, 0,0,0 },
    { "Glow Shroom",    "发光蘑菇",   "发光蘑菇",   0.15f, 0.82f, 0.88f, 0.2f, false, true,  true,  0.2f, 0.9f, 0.95f },
    { "Bedrock",        "基岩",       "基岩",       0.12f, 0.12f, 0.14f,-1.0f, true,  false, false, 0,0,0 }
};

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
    BlockType type;
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
    int type; // BlockType (0..26) or ToolItemID (101..116)
    int count;
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
    bool isDead;
    float deathTimer;
} Player;

static Player g_Player;
static int g_SurfaceHeight[WORLD_WIDTH];
static int g_Seed = 1337;
static InventorySlot g_Hotbar[HOTBAR_SLOTS];
static int g_ActiveSlot = 0;

// Camera & World Time
static float g_CamX = 0.0f;
static float g_CamY = 85.0f;
static float g_CamDist = 16.0f;
static float g_WorldTime = 0.22f;
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

// OpenGL Texture Atlas
static GLuint g_AtlasTex = 0;

static void generateWorld(int seed);
static bool addToInventory(BlockType type, int count);
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

static void generateWorld(int seed) {
    memset(g_World, BLOCK_AIR, sizeof(g_World));
    memset(g_WallWorld, BLOCK_AIR, sizeof(g_WallWorld));
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
            } else if (y < CAVERN_LEVEL) {
                g_World[x][y] = BLOCK_STONE;
                g_WallWorld[x][y] = BLOCK_STONE;
            } else if (y < surfaceY - 4) {
                g_World[x][y] = subBlock;
                g_WallWorld[x][y] = wallBlock;
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

    for (int x = 1; x < WORLD_WIDTH - 1; x++) {
        for (int y = BEDROCK_LEVEL + 3; y < CAVERN_LEVEL + 15; y++) {
            if (g_World[x][y] == BLOCK_STONE || g_World[x][y] == BLOCK_DIRT || g_World[x][y] == BLOCK_SANDSTONE) {
                float oreN = fbm2D((float)x, (float)y, seed + 400, 2, 0.14f);
                if (oreN > 0.88f) {
                    if (y < UNDERWORLD_LEVEL + 10) {
                        g_World[x][y] = (oreN > 0.93f) ? BLOCK_RUBY_ORE : BLOCK_DIAMOND_ORE;
                    } else if (y < CAVERN_LEVEL) {
                        g_World[x][y] = (oreN > 0.92f) ? BLOCK_GOLD_ORE : BLOCK_IRON_ORE;
                    } else {
                        g_World[x][y] = BLOCK_COAL_ORE;
                    }
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

static void spawnDroppedItem(float x, float y, BlockType type, int count) {
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

static bool addToInventory(BlockType type, int count) {
    char buf[64];
    snprintf(buf, sizeof(buf), "+%d %s", count, (getLanguage() == LANG_ZH) ? g_BlockDefs[type].chineseName : g_BlockDefs[type].name);
    spawnFloatingText(buf, g_Player.x, g_Player.y + 1.8f, g_BlockDefs[type].r, g_BlockDefs[type].g, g_BlockDefs[type].b);

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (g_Hotbar[i].type == (int)type && g_Hotbar[i].count < 999) {
            g_Hotbar[i].count += count;
            return true;
        }
    }
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (g_Hotbar[i].type == BLOCK_AIR || g_Hotbar[i].count <= 0) {
            g_Hotbar[i].type = type;
            g_Hotbar[i].count = count;
            return true;
        }
    }
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
                fclose(f2);
                g_Player.health = 100.0f; g_Player.maxHealth = 100.0f; g_Player.isDead = false;
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

    g_WorldTime = hdr.worldTime;
    g_TutorialStep = (TutorialStep)hdr.tutorialStep;
    g_MinedCount = hdr.minedCount;
    g_ActiveSlot = hdr.activeSlot;

    for (int i = 0; i < 9; i++) {
        g_Hotbar[i].type = hdr.hotbar[i].type;
        g_Hotbar[i].count = hdr.hotbar[i].count;
    }
    for (int i = 0; i < 27; i++) {
        g_Backpack[i].type = hdr.backpack[i].type;
        g_Backpack[i].count = hdr.backpack[i].count;
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
    g_Player.health = 100.0f;
    g_Player.maxHealth = 100.0f;
    g_Player.hurtTimer = 0.0f;
    g_Player.isDead = false;
    g_Player.deathTimer = 0.0f;

    // Requirement 3: In Survival Mode start from 0 items (empty hotbar & backpack)
    if (mode == 0) { // Survival
        for (int i = 0; i < HOTBAR_SLOTS; i++) {
            g_Hotbar[i].type = BLOCK_AIR;
            g_Hotbar[i].count = 0;
        }
        for (int i = 0; i < BACKPACK_SLOTS; i++) {
            g_Backpack[i].type = BLOCK_AIR;
            g_Backpack[i].count = 0;
        }
    } else { // Creative
        g_Hotbar[0] = (InventorySlot){ ITEM_TOOL_DIAMOND_PICKAXE, 1 };
        g_Hotbar[1] = (InventorySlot){ ITEM_TOOL_DIAMOND_SWORD, 1 };
        g_Hotbar[2] = (InventorySlot){ BLOCK_DIRT, 999 };
        g_Hotbar[3] = (InventorySlot){ BLOCK_STONE, 999 };
        g_Hotbar[4] = (InventorySlot){ BLOCK_TORCH, 999 };
        g_Hotbar[5] = (InventorySlot){ BLOCK_GLASS, 999 };
        g_Hotbar[6] = (InventorySlot){ BLOCK_BRICK, 999 };
        g_Hotbar[7] = (InventorySlot){ BLOCK_GLOW_SHROOM, 999 };
        g_Hotbar[8] = (InventorySlot){ BLOCK_WOOD_PLANK, 999 };
        for (int i = 0; i < BACKPACK_SLOTS; i++) {
            g_Backpack[i].type = BLOCK_AIR;
            g_Backpack[i].count = 0;
        }
    }

    saveWorldSlot(slotIdx);
}

void onPlayerHurt(float damage, float sourceX) {
    if (getGameMode() != MODE_SURVIVAL) return;
    if (g_Player.isDead || g_Player.hurtTimer > 0.0f) return;

    g_Player.health -= damage;
    g_Player.hurtTimer = 0.65f;
    playSound(SFX_HIT);

    float pushDir = (g_Player.x >= sourceX) ? 1.0f : -1.0f;
    g_Player.vx = pushDir * 6.5f;
    g_Player.vy = 5.8f;

    char dmgStr[16];
    snprintf(dmgStr, sizeof(dmgStr), "-%d", (int)damage);
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
    g_Player.health = g_Player.maxHealth;
    g_Player.hurtTimer = 1.0f;
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

    showNotification(LOC("欢迎来到 Terraria 2.5D！按 [H] 查看指南，[F5] 快速保存。", "Welcome to Terraria 2.5D! Press [H] for Help, [F5] to Save."), 1.0f, 0.85f, 0.3f);
}


static void updatePhysics(float dt) {
    if (g_Player.hurtTimer > 0.0f) {
        g_Player.hurtTimer -= dt;
        if (g_Player.hurtTimer < 0.0f) g_Player.hurtTimer = 0.0f;
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

    float lookahead = g_Player.isFacingRight ? 2.5f : -2.5f;
    g_CamX += (g_Player.x + lookahead - g_CamX) * 5.0f * dt;
    g_CamY += (g_Player.y + 1.2f - g_CamY) * 5.0f * dt;
    if (g_CamY < 8.0f) g_CamY = 8.0f;

    g_WorldTime += dt / 240.0f;
    if (g_WorldTime > 1.0f) g_WorldTime -= 1.0f;

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
                g_DroppedItems[i].y = (float)(iy + 1);
                g_DroppedItems[i].vy = 0.0f;
                g_DroppedItems[i].vx *= 0.8f;
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
    if (isCraftingOpen() || isCreativeInventoryOpen()) {
        g_MiningProgress = 0.0f;
        return;
    }

    updateToolSwing(dt, g_MouseLeft);

    float mouseWX, mouseWY;
    screenToWorld(g_MouseScreenX, g_MouseScreenY, &mouseWX, &mouseWY);

    static float s_AttackCooldown = 0.0f;
    s_AttackCooldown -= dt;
    if (g_MouseLeft && s_AttackCooldown <= 0.0f) {
        int heldItem = g_Hotbar[g_ActiveSlot].type;
        float damage = getToolDamage(heldItem);
        float knockback = getToolKnockback(heldItem);
        if (checkMobAttackEx(mouseWX, mouseWY, damage, knockback, g_Player.x, g_Player.y)) {
            s_AttackCooldown = 0.20f;
            g_MiningProgress = 0.0f;
            return;
        }
    }

    g_TargetX = (int)floorf(mouseWX);
    g_TargetY = (int)floorf(mouseWY);

    float dx = (float)g_TargetX + 0.5f - g_Player.x;
    float dy = (float)g_TargetY + 0.5f - (g_Player.y + 0.9f);
    float dist = sqrtf(dx * dx + dy * dy);

    g_TargetValid = (dist <= REACH_DISTANCE) && (g_TargetX >= 0 && g_TargetX < WORLD_WIDTH && g_TargetY >= 0 && g_TargetY < WORLD_HEIGHT);

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
                float breakTime = (g_BlockDefs[bt].hardness * 0.35f) / efficiency;
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
                spawnDroppedItem((float)g_TargetX + 0.5f, (float)g_TargetY + 0.5f, bt, 1);
                g_World[g_TargetX][g_TargetY] = BLOCK_AIR;
                recordBlockDelta(g_TargetX, g_TargetY, false, BLOCK_AIR);
                g_MiningProgress = 0.0f;

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
        if (g_World[g_TargetX][g_TargetY] == BLOCK_AIR) {
            InventorySlot* slot = &g_Hotbar[g_ActiveSlot];
            if (slot->type != BLOCK_AIR && !isToolItem(slot->type) && slot->count > 0) {
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

    float u0, v0, u1, v1;
    float zFront = (blockType == BLOCK_WOOD) ? -0.15f : 0.0f;
    float zBack = (blockType == BLOCK_WOOD) ? -0.55f : -BLOCK_DEPTH;

    // Front Face (Z = 0)
    int frontTex = GetBlockFaceTextureIndex(blockType, 0);
    GetAtlasTileUV(frontTex, &u0, &v0, &u1, &v1);
    glColor3f(lightFactor, lightFactor, lightFactor);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x, y, zFront);
    glTexCoord2f(u1, v1); glVertex3f(x + 1.0f, y, zFront);
    glTexCoord2f(u1, v0); glVertex3f(x + 1.0f, y + 1.0f, zFront);
    glTexCoord2f(u0, v0); glVertex3f(x, y + 1.0f, zFront);
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
    if (daylight < 0.22f) daylight = 0.22f;

    typedef struct {
        float x, y;
        float radius;
        float r, g, b;
        float intensity;
    } PointLight;

    PointLight localLights[48];
    int numLocalLights = 0;

    localLights[numLocalLights++] = (PointLight){
        g_Player.x, g_Player.y + 1.0f,
        6.5f,
        1.0f, 0.96f, 0.88f,
        0.95f
    };

    int lStartX = (startX - 8 < 0) ? 0 : startX - 8;
    int lEndX   = (endX + 8 >= WORLD_WIDTH) ? WORLD_WIDTH - 1 : endX + 8;
    int lStartY = (startY - 8 < 0) ? 0 : startY - 8;
    int lEndY   = (endY + 8 >= WORLD_HEIGHT) ? WORLD_HEIGHT - 1 : endY + 8;

    for (int lx = lStartX; lx <= lEndX && numLocalLights < 45; lx++) {
        for (int ly = lStartY; ly <= lEndY && numLocalLights < 45; ly++) {
            BlockType bt = g_World[lx][ly];
            if (bt != BLOCK_AIR && g_BlockDefs[bt].emitLight) {
                float lr = 1.0f, lg = 0.90f, lb = 0.60f;
                float rad = 8.0f;
                if (bt == BLOCK_MAGMA) { lr = 1.0f; lg = 0.35f; lb = 0.10f; rad = 6.0f; }
                else if (bt == BLOCK_GLOW_SHROOM) { lr = 0.20f; lg = 0.85f; lb = 1.0f; rad = 7.0f; }
                localLights[numLocalLights++] = (PointLight){
                    (float)lx + 0.5f, (float)ly + 0.5f,
                    rad,
                    lr, lg, lb,
                    1.15f
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
                    float caveAmbient = 0.45f;
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
            if (def->emitLight) {
                lightFactor = 1.15f;
            } else {
                float baseL = daylight;
                if (y < sY) {
                    float depth = (float)(sY - y);
                    float sunPen = daylight * expf(-depth * 0.25f);
                    float caveAmbient = 0.45f;
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
                if (lightFactor < 0.35f) lightFactor = 0.35f;
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
    glPushMatrix();
    glTranslatef(g_Player.x, g_Player.y, 0.0f);
    if (!g_Player.isFacingRight) glScalef(-1.0f, 1.0f, 1.0f);

    float sinWalk = sinf(g_Player.walkCycle);

    // Torso
    glColor3f(0.24f, 0.48f, 0.92f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 0.45f, 0.0f);
    glVertex3f( 0.30f, 0.45f, 0.0f);
    glVertex3f( 0.30f, 1.25f, 0.0f);
    glVertex3f(-0.30f, 1.25f, 0.0f);
    glEnd();

    // Belt & Buckle
    glColor3f(0.42f, 0.26f, 0.14f);
    glBegin(GL_QUADS);
    glVertex3f(-0.30f, 0.45f, 0.02f);
    glVertex3f( 0.30f, 0.45f, 0.02f);
    glVertex3f( 0.30f, 0.60f, 0.02f);
    glVertex3f(-0.30f, 0.60f, 0.02f);
    glEnd();
    glColor3f(0.98f, 0.85f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(-0.08f, 0.46f, 0.03f);
    glVertex3f( 0.08f, 0.46f, 0.03f);
    glVertex3f( 0.08f, 0.59f, 0.03f);
    glVertex3f(-0.08f, 0.59f, 0.03f);
    glEnd();

    // Head
    glColor3f(0.98f, 0.82f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(-0.25f, 1.25f, 0.0f);
    glVertex3f( 0.25f, 1.25f, 0.0f);
    glVertex3f( 0.25f, 1.70f, 0.0f);
    glVertex3f(-0.25f, 1.70f, 0.0f);
    glEnd();

    // Helmet
    glColor3f(0.88f, 0.58f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(-0.28f, 1.55f, 0.02f);
    glVertex3f( 0.28f, 1.55f, 0.02f);
    glVertex3f( 0.28f, 1.82f, 0.02f);
    glVertex3f(-0.28f, 1.82f, 0.02f);
    glEnd();

    // Eyes
    glColor3f(0.10f, 0.10f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(0.08f, 1.40f, 0.03f);
    glVertex3f(0.18f, 1.40f, 0.03f);
    glVertex3f(0.18f, 1.50f, 0.03f);
    glVertex3f(0.08f, 1.50f, 0.03f);
    glEnd();

    // Legs
    float legAngleL = g_Player.isGrounded ? sinWalk * 0.18f : 0.15f;
    float legAngleR = g_Player.isGrounded ? -sinWalk * 0.18f : -0.15f;

    glColor3f(0.20f, 0.24f, 0.45f);
    glBegin(GL_QUADS);
    glVertex3f(-0.22f + legAngleL, 0.0f, 0.0f);
    glVertex3f(-0.05f + legAngleL, 0.0f, 0.0f);
    glVertex3f(-0.05f, 0.45f, 0.0f);
    glVertex3f(-0.22f, 0.45f, 0.0f);

    glVertex3f( 0.05f + legAngleR, 0.0f, 0.0f);
    glVertex3f( 0.22f + legAngleR, 0.0f, 0.0f);
    glVertex3f( 0.22f, 0.45f, 0.0f);
    glVertex3f( 0.05f, 0.45f, 0.0f);
    glEnd();

    // In-Hand Tool Rendering with Dynamic Arm Swing Arc
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
    drawQuad2D(15, 15, 310, 110, 0.08f, 0.10f, 0.14f, 0.88f);
    drawBorder2D(15, 15, 310, 110, 0.35f, 0.45f, 0.58f, 1.5f);

    BiomeType curBiome = getBiomeAt((int)floorf(g_Player.x));
    drawText(25, 34, g_BiomeNames[curBiome], 1.0f, 0.95f, 0.7f, true);

    char coordBuf[64];
    snprintf(coordBuf, sizeof(coordBuf), "Pos: X: %.1f, Y: %.1f", g_Player.x, g_Player.y);
    drawText(25, 52, coordBuf, 0.8f, 0.9f, 1.0f, false);

    const char* depthName = (g_Player.y <= BEDROCK_LEVEL) ? LOC("基岩地层", "Bedrock") :
                            ((g_Player.y < UNDERWORLD_LEVEL) ? LOC("熔岩地狱", "Underworld") :
                            ((g_Player.y < CAVERN_LEVEL) ? LOC("地下矿洞", "Cavern") :
                            ((g_Player.y < SUBSURFACE_LEVEL) ? LOC("地下土层", "Subsurface") :
                            LOC("地表世界", "Surface"))));
    char depthBuf[64];
    snprintf(depthBuf, sizeof(depthBuf), "%s: %s", LOC("区域", "Zone"), depthName);
    drawText(25, 68, depthBuf, 0.7f, 0.8f, 0.85f, false);

    char modeBadge[128];
    if (getGameMode() == MODE_CREATIVE) {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s %s",
            LOC("当前模式", "Mode"), LOC("创造模式", "Creative"),
            isCreativeFlying() ? LOC("[飞行开启]", "[FLYING]") : LOC("[双击空格飞行]", "[2x SPACE]"));
        drawText(25, 86, modeBadge, isCreativeFlying() ? 0.35f : 1.0f, 0.90f, isCreativeFlying() ? 1.0f : 0.3f, true);
    } else {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s", LOC("当前模式", "Mode"), LOC("生存模式", "Survival"));
        drawText(25, 86, modeBadge, 0.4f, 0.95f, 0.5f, false);
    }
    drawText(25, 102, LOC("[ESC] 暂停/设置 | [F5] 保存 | [F9] 读取 | [H] 帮助", "[ESC] Pause/Settings | [F5] Save | [F9] Load | [H] Help"), 0.65f, 0.75f, 0.85f, false);

    // 2. Novice Tutorial Quest Box (Top-Right)
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
    char actBanner[128];
    if (actType != BLOCK_AIR && actType != 0 && actCount > 0) {
        if (isToolItem(actType)) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s",
                g_ActiveSlot + 1, (getLanguage() == LANG_ZH) ? getItemChineseName(actType) : getItemName(actType));
        } else if (actType < BLOCK_TYPE_COUNT) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s  x%d",
                g_ActiveSlot + 1, (getLanguage() == LANG_ZH) ? g_BlockDefs[actType].chineseName : g_BlockDefs[actType].name, actCount);
        }
    } else {
        snprintf(actBanner, sizeof(actBanner), "[%d] (%s)", g_ActiveSlot + 1, LOC("空", "Empty"));
    }
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

    float banW = 280.0f;
    float banX = ((float)WINDOW_WIDTH - banW) * 0.5f;
    drawQuad2D(banX, startY - 30, banW, 24, 0.06f, 0.08f, 0.12f, 0.85f);
    drawBorder2D(banX, startY - 30, banW, 24, 0.95f, 0.80f, 0.20f, 1.5f);
    drawText(banX + 15, startY - 14, actBanner, 1.0f, 0.90f, 0.30f, false);

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
            if (isToolItem(bt)) {
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
            }
        }

        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        drawText(sx + 4, sy + 14, numBuf, 0.7f, 0.7f, 0.7f, false);
    }

    // 0. Player Health Bar (Survival Mode)
    if (getGameMode() == MODE_SURVIVAL) {
        float barX = 20.0f;
        float barY = 20.0f;
        float barW = 200.0f;
        float barH = 22.0f;

        // Background
        drawQuad2D(barX, barY, barW, barH, 0.12f, 0.14f, 0.18f, 0.92f);
        drawBorder2D(barX, barY, barW, barH, 0.40f, 0.45f, 0.55f, 1.5f);

        // Fill
        float hpPct = g_Player.health / (g_Player.maxHealth > 0.0f ? g_Player.maxHealth : 100.0f);
        if (hpPct < 0.0f) hpPct = 0.0f;
        if (hpPct > 1.0f) hpPct = 1.0f;
        float fillW = (barW - 4.0f) * hpPct;
        if (fillW > 0.0f) {
            float fR = (g_Player.hurtTimer > 0.0f) ? 1.0f : 0.90f;
            float fG = (g_Player.hurtTimer > 0.0f) ? 0.75f : 0.20f;
            float fB = (g_Player.hurtTimer > 0.0f) ? 0.75f : 0.20f;
            drawQuad2D(barX + 2.0f, barY + 2.0f, fillW, barH - 4.0f, fR, fG, fB, 0.95f);
        }

        // Text
        char hpBuf[64];
        snprintf(hpBuf, sizeof(hpBuf), "❤️ %s: %d / %d", LOC("生命值", "HP"), (int)ceilf(g_Player.health), (int)g_Player.maxHealth);
        drawText(barX + 12.0f, barY + 16.0f, hpBuf, 1.0f, 1.0f, 1.0f, true);
    }

    // Game Over Overlay
    if (g_Player.isDead) {
        drawQuad2D(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.45f, 0.05f, 0.05f, 0.65f);

        const char* dTitle = LOC("💀 你阵亡了！", "💀 YOU DIED!");
        float dtw = getTextWidthUTF8(dTitle, 24, true);
        drawText(((float)WINDOW_WIDTH - dtw) * 0.5f, (float)WINDOW_HEIGHT * 0.38f, dTitle, 1.0f, 0.25f, 0.25f, true);

        const char* dPrompt = LOC("[ 点击此处 或 按 R 键 立即重生 ]", "[ Click Here or Press R to Respawn ]");
        float dpw = getTextWidthUTF8(dPrompt, 16, true);
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
                } else {
                    int sx = (int)floorf(g_Player.x);
                    g_Player.y = (float)(getSurfaceHeight(sx, g_Seed) + 3);
                    g_Player.vx = 0.0f;
                    g_Player.vy = 0.0f;
                    showNotification(LOC("已在地面安全区域重生！", "Respawned on surface safe ground!"), 0.4f, 0.8f, 1.0f);
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
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "Terraria25DSandboxClass";
    RegisterClassEx(&wc);

    RECT wr = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,
        "Terraria25DSandboxClass",
        "Terraria 2.5D Open Sandbox World",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL
    );

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
            if (isCraftingOpen()) {
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
