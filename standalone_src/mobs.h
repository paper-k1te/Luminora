#ifndef MOBS_H
#define MOBS_H

#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "sound_engine.h"
#include "localization.h"

#ifndef ITEM_GEL
#define ITEM_GEL 118
#endif

// ============================================================================
// Mob System Configuration & Enumerations
// ============================================================================
#define MAX_MOBS                32
#define TARGET_MOB_COUNT        16
#define MAX_HOSTILE_MOBS        9
#define SAFE_ZONE_SPAWN_RADIUS  25.0f
#define MOB_DESPAWN_DIST        48.0f
#define MOB_MIN_SPAWN_DIST      16.0f
#define MOB_MAX_SPAWN_DIST      34.0f

typedef enum {
    MOB_SLIME = 0,
    MOB_BAT,
    MOB_ZOMBIE,
    MOB_SHEEP,
    MOB_COW,
    MOB_PIG,
    MOB_RABBIT,
    MOB_SKELETON,
    MOB_SPIDER,
    MOB_GHOST,
    MOB_LAVA_SLIME,
    MOB_TYPE_COUNT
} MobType;

// Slime States
#define SLIME_STATE_IDLE    0
#define SLIME_STATE_JUMP    1
#define SLIME_STATE_LAND    2

// Bat States
#define BAT_STATE_WANDER    0
#define BAT_STATE_SWOOP     1
#define BAT_STATE_RETREAT   2

// Zombie States
#define ZOMBIE_STATE_WANDER 0
#define ZOMBIE_STATE_CHASE  1

// Peaceful Animal States
#define ANIMAL_STATE_IDLE   0
#define ANIMAL_STATE_WANDER 1
#define ANIMAL_STATE_GRAZE  2
#define ANIMAL_STATE_FLEE   3

// Skeleton States
#define SKELETON_STATE_WANDER 0
#define SKELETON_STATE_CHASE  1
#define SKELETON_STATE_ATTACK 2

// Spider States
#define SPIDER_STATE_WANDER   0
#define SPIDER_STATE_CHASE    1
#define SPIDER_STATE_LEAP     2

// Ghost States
#define GHOST_STATE_DRIFT     0
#define GHOST_STATE_CHASE     1

// Lava Slime States
#define LAVA_SLIME_STATE_IDLE 0
#define LAVA_SLIME_STATE_JUMP 1
#define LAVA_SLIME_STATE_LAND 2

// ============================================================================
// Mob Data Structure
// ============================================================================
typedef struct {
    float x;
    float y;
    float z;            // 2.5D gameplay plane offset (default 0.01f)
    float vx;
    float vy;
    int type;           // MobType enum
    float health;
    float maxHealth;
    float animTimer;    // Drives squash-and-stretch, wing flap, trotting, hopping
    float stateTimer;   // State duration timer (grazing, idling, fleeing)
    float soundTimer;   // Voice & vocalization cadence timer
    float attackTimer;  // Weapon thrust, leap cooldown, special attack cadence
    bool facingRight;
    int state;
    float hurtTimer;    // > 0 indicates red damage-flash and invincibility frames
    bool active;
    
    // Physical dimensions & collision bounding box
    float width;
    float height;
    bool isGrounded;
} Mob;

static Mob g_Mobs[MAX_MOBS];
static float g_MobSpawnTimer = 0.0f;

// Forward declarations of host environment functions in main.c
#ifndef WORLD_WIDTH
#define WORLD_WIDTH   512
#define WORLD_HEIGHT  128
#endif

#ifndef REACH_DISTANCE
#define REACH_DISTANCE 6.5f
#endif

#ifndef CAVERN_LEVEL
#define CAVERN_LEVEL     55
#define SUBSURFACE_LEVEL 75
#define UNDERWORLD_LEVEL 24
#endif

#ifndef ITEM_STICK
#define ITEM_STICK 117
#endif

#ifndef BLOCK_MAGMA
#define BLOCK_MAGMA 22
#endif

// ============================================================================
// Function Declarations
// ============================================================================
void onPlayerHurt(float damage, float sourceX);
void initMobs(void);
void spawnMob(float x, float y, int type);
void spawnMobs(float playerX, float playerY, int activeCount, int activeHostileCount);
void updateMobs(float dt, float playerX, float playerY);
void renderMobs(float daylight, float camX, float camY);
bool checkMobAttack(float clickWorldX, float clickWorldY, float damage, float playerX, float playerY);
bool checkMobAttackEx(float clickWorldX, float clickWorldY, float damage, float knockbackMult, float playerX, float playerY);

static inline bool isHostileMob(int type) {
    return (type == MOB_ZOMBIE || type == MOB_BAT || type == MOB_SKELETON ||
            type == MOB_SPIDER || type == MOB_GHOST || type == MOB_LAVA_SLIME);
}

static inline bool isPeacefulAnimal(int type) {
    return (type == MOB_SHEEP || type == MOB_COW || type == MOB_PIG || type == MOB_RABBIT);
}

// Anti-Wall Spawning Validation:
// (x, y) and (x, y+1) must be completely AIR (!isSolidBlock), and (x, y-1) must be solid floor
static inline bool isValidMobSpawnPoint(int x, int y, bool isFlying) {
    if (x < 4 || x >= WORLD_WIDTH - 4 || y < 2 || y >= WORLD_HEIGHT - 3) {
        return false;
    }
    // (x, y) and (x, y+1) must be completely AIR
    if (isSolidBlock(x, y) || isSolidBlock(x, y + 1)) {
        return false;
    }
    // Solid floor at (x, y-1) required for non-flying mobs
    if (!isFlying && !isSolidBlock(x, y - 1)) {
        return false;
    }
    return true;
}

// ============================================================================
// Implementation
// ============================================================================

void initMobs(void) {
    memset(g_Mobs, 0, sizeof(g_Mobs));
    g_MobSpawnTimer = 0.0f;
}

void spawnMob(float x, float y, int type) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    bool isFlying = (type == MOB_BAT || type == MOB_GHOST);
    if (!isValidMobSpawnPoint(ix, iy, isFlying)) {
        return; // Anti-wall validation: never spawn inside walls or without solid floor
    }

    if (isHostileMob(type)) {
        int initSpawnX = WORLD_WIDTH / 2;
        int initSpawnY = (initSpawnX >= 0 && initSpawnX < WORLD_WIDTH) ? g_SurfaceHeight[initSpawnX] + 2 : 87;
        float dxSp = x - (float)initSpawnX;
        float dySp = y - (float)initSpawnY;
        if (sqrtf(dxSp * dxSp + dySp * dySp) <= SAFE_ZONE_SPAWN_RADIUS) {
            return; // 0 hostile mobs in safe zone
        }
    }

    for (int i = 0; i < MAX_MOBS; i++) {
        if (!g_Mobs[i].active) {
            Mob* m = &g_Mobs[i];
            m->active = true;
            m->type = type;
            m->x = x;
            m->y = y;
            m->z = 0.01f;
            m->vx = 0.0f;
            m->vy = 0.0f;
            m->facingRight = (rand() % 2 == 0);
            m->hurtTimer = 0.0f;
            m->isGrounded = false;
            m->animTimer = ((float)rand() / RAND_MAX) * 3.14159f;
            m->stateTimer = 1.0f + ((float)rand() / RAND_MAX) * 2.5f;
            m->soundTimer = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;
            m->attackTimer = 0.0f;

            switch (type) {
                case MOB_SLIME:
                    m->width = 0.90f;
                    m->height = 0.72f;
                    m->health = m->maxHealth = 25.0f;
                    m->state = SLIME_STATE_IDLE;
                    m->animTimer = 0.6f + ((float)rand() / RAND_MAX) * 0.8f;
                    break;

                case MOB_BAT:
                    m->width = 0.75f;
                    m->height = 0.55f;
                    m->health = m->maxHealth = 16.0f;
                    m->state = BAT_STATE_WANDER;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    break;

                case MOB_ZOMBIE:
                    m->width = 0.78f;
                    m->height = 1.82f;
                    m->health = m->maxHealth = 45.0f;
                    m->state = ZOMBIE_STATE_WANDER;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    break;

                case MOB_SHEEP:
                    m->width = 1.10f;
                    m->height = 0.95f;
                    m->health = m->maxHealth = 18.0f;
                    m->state = (rand() % 3 == 0) ? ANIMAL_STATE_GRAZE : ANIMAL_STATE_WANDER;
                    break;

                case MOB_COW:
                    m->width = 1.25f;
                    m->height = 1.15f;
                    m->health = m->maxHealth = 24.0f;
                    m->state = (rand() % 3 == 0) ? ANIMAL_STATE_GRAZE : ANIMAL_STATE_WANDER;
                    break;

                case MOB_PIG:
                    m->width = 0.95f;
                    m->height = 0.80f;
                    m->health = m->maxHealth = 18.0f;
                    m->state = ANIMAL_STATE_WANDER;
                    break;

                case MOB_RABBIT:
                    m->width = 0.48f;
                    m->height = 0.48f;
                    m->health = m->maxHealth = 8.0f;
                    m->state = ANIMAL_STATE_WANDER;
                    break;

                case MOB_SKELETON:
                    m->width = 0.72f;
                    m->height = 1.85f;
                    m->health = m->maxHealth = 40.0f;
                    m->state = SKELETON_STATE_WANDER;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    m->attackTimer = 0.0f;
                    break;

                case MOB_SPIDER:
                    m->width = 1.15f;
                    m->height = 0.58f;
                    m->health = m->maxHealth = 28.0f;
                    m->state = SPIDER_STATE_WANDER;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    m->attackTimer = 0.4f + ((float)rand() / RAND_MAX) * 0.8f;
                    break;

                case MOB_GHOST:
                    m->width = 0.80f;
                    m->height = 1.10f;
                    m->health = m->maxHealth = 24.0f;
                    m->state = GHOST_STATE_DRIFT;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    m->attackTimer = 0.0f;
                    break;

                case MOB_LAVA_SLIME:
                    m->width = 1.05f;
                    m->height = 0.84f;
                    m->health = m->maxHealth = 38.0f;
                    m->state = LAVA_SLIME_STATE_IDLE;
                    m->animTimer = 0.5f + ((float)rand() / RAND_MAX) * 0.7f;
                    m->attackTimer = 0.0f;
                    break;

                default:
                    break;
            }
            return;
        }
    }
}

static inline float getMobSpawnLightLevel(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) return 0.0f;
    float sunAngle = g_WorldTime * 6.2831853f;
    float rawDaylight = sinf(sunAngle);
    float daylight = (rawDaylight > 0.0f) ? rawDaylight : 0.0f;

    int surfY = (x >= 0 && x < WORLD_WIDTH) ? g_SurfaceHeight[x] : 85;
    float sunLight = 0.0f;
    if (y >= surfY) {
        sunLight = daylight;
    } else {
        float depth = (float)(surfY - y);
        sunLight = daylight * expf(-depth * 0.25f);
    }

    float torchLight = 0.0f;
    for (int dx = -7; dx <= 7; dx++) {
        for (int dy = -7; dy <= 7; dy++) {
            int tx = x + dx;
            int ty = y + dy;
            if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                BlockType bt = g_World[tx][ty];
                if (bt != BLOCK_AIR && g_BlockDefs[bt].emitLight) {
                    if ((bt == BLOCK_TORCH || bt == BLOCK_CHANDELIER) && isLightExtinguished(tx, ty)) {
                        continue;
                    }
                    float dist = sqrtf((float)(dx * dx + dy * dy));
                    if (dist < 7.5f) {
                        float att = (1.0f - dist / 7.5f) * 0.90f;
                        if (att > torchLight) torchLight = att;
                    }
                }
            }
        }
    }

    float totalLight = sunLight + torchLight;
    if (totalLight > 1.0f) totalLight = 1.0f;
    return totalLight;
}

void spawnMobs(float playerX, float playerY, int activeCount, int activeHostileCount) {
    if (activeCount >= TARGET_MOB_COUNT) return;

    float spawnOffset = (MOB_MIN_SPAWN_DIST + ((float)rand() / RAND_MAX) * (MOB_MAX_SPAWN_DIST - MOB_MIN_SPAWN_DIST));
    if (rand() % 2 == 0) spawnOffset = -spawnOffset;

    int targetX = (int)floorf(playerX + spawnOffset);
    if (targetX < 5 || targetX >= WORLD_WIDTH - 5) return;

    int initSpawnX = WORLD_WIDTH / 2;
    int initSpawnY = (initSpawnX >= 0 && initSpawnX < WORLD_WIDTH) ? g_SurfaceHeight[initSpawnX] + 2 : 87;

    float sunAngle = g_WorldTime * 6.2831853f;
    float rawDaylight = sinf(sunAngle);
    bool isDay = (rawDaylight > 0.12f);
    bool isMidnight = (rawDaylight < -0.35f);

    // ------------------------------------------------------------------------
    // Zone 1: UNDERWORLD (playerY < UNDERWORLD_LEVEL) -> Lava Slimes thrive here!
    // ------------------------------------------------------------------------
    if (playerY < UNDERWORLD_LEVEL) {
        if (activeHostileCount >= MAX_HOSTILE_MOBS) return;

        int chosenType = MOB_LAVA_SLIME;
        int r = rand() % 100;
        if (r < 50) {
            chosenType = MOB_LAVA_SLIME; // Signature underworld fiery magma cube
        } else if (r < 75) {
            chosenType = MOB_SKELETON;   // Underworld bone wanderer
        } else if (r < 88) {
            chosenType = MOB_BAT;
        } else {
            chosenType = MOB_GHOST;      // Lost spectral entity
        }

        int searchY = (int)floorf(playerY + ((rand() % 12) - 6));
        if (searchY < 3) searchY = 3;
        if (searchY >= UNDERWORLD_LEVEL + 2) searchY = UNDERWORLD_LEVEL + 1;

        bool isFlying = (chosenType == MOB_BAT || chosenType == MOB_GHOST);

        for (int testY = searchY; testY >= 3; testY--) {
            if (isSolidBlock(targetX, testY) && !isSolidBlock(targetX, testY + 1) && !isSolidBlock(targetX, testY + 2)) {
                int spawnY = testY + 1;
                float dxSp = (float)targetX + 0.5f - (float)initSpawnX;
                float dySp = (float)spawnY - (float)initSpawnY;
                if (sqrtf(dxSp * dxSp + dySp * dySp) > SAFE_ZONE_SPAWN_RADIUS) {
                    spawnMob((float)targetX + 0.5f, (float)spawnY, chosenType);
                }
                break;
            } else if (isFlying && !isSolidBlock(targetX, testY) && !isSolidBlock(targetX, testY + 1)) {
                float dxSp = (float)targetX + 0.5f - (float)initSpawnX;
                float dySp = (float)testY - (float)initSpawnY;
                if (sqrtf(dxSp * dxSp + dySp * dySp) > SAFE_ZONE_SPAWN_RADIUS) {
                    spawnMob((float)targetX + 0.5f, (float)testY, chosenType);
                }
                break;
            }
        }
    }
    // ------------------------------------------------------------------------
    // Zone 2: CAVERNS (UNDERWORLD_LEVEL <= playerY < CAVERN_LEVEL)
    //         -> Spiders & Skeletons in caverns, Ghosts in deep dark caverns
    // ------------------------------------------------------------------------
    else if (playerY < CAVERN_LEVEL) {
        if (activeHostileCount >= MAX_HOSTILE_MOBS) return;

        int chosenType = MOB_SKELETON;
        int r = rand() % 100;
        bool isDeepDark = (playerY < (UNDERWORLD_LEVEL + CAVERN_LEVEL) / 2); // y < 39

        if (isDeepDark && r < 25) {
            chosenType = MOB_GHOST;    // Ghosts in deep dark caverns
        } else if (r < 45) {
            chosenType = MOB_SPIDER;   // Cave spider with 8 scuttling legs
        } else if (r < 75) {
            chosenType = MOB_SKELETON; // Skeleton warrior
        } else if (r < 88) {
            chosenType = MOB_BAT;
        } else {
            chosenType = MOB_ZOMBIE;
        }

        int searchY = (int)floorf(playerY + ((rand() % 14) - 7));
        if (searchY < UNDERWORLD_LEVEL) searchY = UNDERWORLD_LEVEL;
        if (searchY >= CAVERN_LEVEL) searchY = CAVERN_LEVEL - 1;

        bool isFlying = (chosenType == MOB_BAT || chosenType == MOB_GHOST);

        for (int testY = searchY; testY >= UNDERWORLD_LEVEL; testY--) {
            if (isSolidBlock(targetX, testY) && !isSolidBlock(targetX, testY + 1) && !isSolidBlock(targetX, testY + 2)) {
                int spawnY = testY + 1;
                float lightFactor = getMobSpawnLightLevel(targetX, spawnY);
                if (lightFactor < 0.35f) {
                    float dxSp = (float)targetX + 0.5f - (float)initSpawnX;
                    float dySp = (float)spawnY - (float)initSpawnY;
                    if (sqrtf(dxSp * dxSp + dySp * dySp) > SAFE_ZONE_SPAWN_RADIUS) {
                        spawnMob((float)targetX + 0.5f, (float)spawnY, chosenType);
                    }
                }
                break;
            } else if (isFlying && !isSolidBlock(targetX, testY) && !isSolidBlock(targetX, testY + 1)) {
                float lightFactor = getMobSpawnLightLevel(targetX, testY);
                if (lightFactor < 0.28f) {
                    float dxSp = (float)targetX + 0.5f - (float)initSpawnX;
                    float dySp = (float)testY - (float)initSpawnY;
                    if (sqrtf(dxSp * dxSp + dySp * dySp) > SAFE_ZONE_SPAWN_RADIUS) {
                        spawnMob((float)targetX + 0.5f, (float)testY, chosenType);
                    }
                }
                break;
            }
        }
    }
    // ------------------------------------------------------------------------
    // Zone 3: SURFACE & SUBSURFACE (playerY >= CAVERN_LEVEL)
    //         -> Peaceful animals in daylight, Ghosts in midnight surface
    // ------------------------------------------------------------------------
    else {
        int surfY = (targetX >= 0 && targetX < WORLD_WIDTH) ? g_SurfaceHeight[targetX] : getSurfaceHeight(targetX, 1337);

        int groundY = -1;
        for (int ty = surfY + 6; ty >= surfY - 6; ty--) {
            if (ty >= 2 && ty < WORLD_HEIGHT - 3) {
                if (isSolidBlock(targetX, ty) && !isSolidBlock(targetX, ty + 1) && !isSolidBlock(targetX, ty + 2)) {
                    groundY = ty;
                    break;
                }
            }
        }

        if (groundY > 0) {
            int spawnY = groundY + 1;
            float dxSp = (float)targetX + 0.5f - (float)initSpawnX;
            float dySp = (float)spawnY - (float)initSpawnY;
            bool inSafeZone = (sqrtf(dxSp * dxSp + dySp * dySp) <= SAFE_ZONE_SPAWN_RADIUS);

            int chosenType = MOB_SLIME;
            if (isDay) {
                // Daytime Surface: strictly peaceful animals and rare green slime
                int r = rand() % 100;
                if (r < 28) chosenType = MOB_SHEEP;
                else if (r < 54) chosenType = MOB_COW;
                else if (r < 74) chosenType = MOB_PIG;
                else if (r < 90) chosenType = MOB_RABBIT;
                else chosenType = MOB_SLIME;
            } else {
                // Nighttime Surface:
                if (inSafeZone || activeHostileCount >= MAX_HOSTILE_MOBS) {
                    int r = rand() % 100;
                    if (r < 40) chosenType = MOB_SHEEP;
                    else if (r < 75) chosenType = MOB_COW;
                    else chosenType = MOB_SLIME;
                } else {
                    int r = rand() % 100;
                    // Midnight surface: Ghosts drift through midnight darkness!
                    if (isMidnight && r < 30) {
                        chosenType = MOB_GHOST;
                    } else if (r < 55) {
                        chosenType = MOB_ZOMBIE;
                    } else if (r < 75) {
                        chosenType = MOB_SKELETON;
                    } else if (r < 90) {
                        chosenType = MOB_SPIDER;
                    } else {
                        chosenType = MOB_SLIME;
                    }
                }
            }

            if (chosenType >= 0) {
                float actualSpawnY = (chosenType == MOB_GHOST) ? (float)spawnY + 1.2f : (float)spawnY;
                spawnMob((float)targetX + 0.5f, actualSpawnY, chosenType);
            }
        }
    }
}

void updateMobs(float dt, float playerX, float playerY) {
    // ------------------------------------------------------------------------
    // 1. Mob Spawning & Despawning Management
    // ------------------------------------------------------------------------
    int activeCount = 0;
    int activeHostileCount = 0;
    int initSpawnX = WORLD_WIDTH / 2;
    int initSpawnY = (initSpawnX >= 0 && initSpawnX < WORLD_WIDTH) ? g_SurfaceHeight[initSpawnX] + 2 : 87;

    for (int i = 0; i < MAX_MOBS; i++) {
        if (g_Mobs[i].active) {
            float dx = g_Mobs[i].x - playerX;
            float dy = g_Mobs[i].y - playerY;
            float dist = sqrtf(dx * dx + dy * dy);

            // Despawn mobs that wander too far away or fall out of bounds
            if (dist > MOB_DESPAWN_DIST || g_Mobs[i].y < 1.0f || g_Mobs[i].y >= WORLD_HEIGHT - 2) {
                g_Mobs[i].active = false;
                continue;
            }
            activeCount++;
            if (isHostileMob(g_Mobs[i].type)) {
                activeHostileCount++;
            }
        }
    }

    g_MobSpawnTimer += dt;
    if (g_MobSpawnTimer >= 1.0f) {
        g_MobSpawnTimer = 0.0f;
        spawnMobs(playerX, playerY, activeCount, activeHostileCount);
    }

    // ------------------------------------------------------------------------
    // 2. Individual Mob Behaviors, State Machines, Physics & Anti-Wall Unstick
    // ------------------------------------------------------------------------
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!g_Mobs[i].active) continue;
        Mob* m = &g_Mobs[i];

        if (m->hurtTimer > 0.0f) {
            m->hurtTimer -= dt;
            if (m->hurtTimer < 0.0f) m->hurtTimer = 0.0f;
        }

        // Safe zone enforcement: 0 hostile mobs allowed within 25 blocks of initial spawn point
        if (isHostileMob(m->type)) {
            float dxSp = m->x - (float)initSpawnX;
            float dySp = m->y - (float)initSpawnY;
            float distSp = sqrtf(dxSp * dxSp + dySp * dySp);
            if (distSp < SAFE_ZONE_SPAWN_RADIUS) {
                if (distSp < 18.0f) {
                    m->active = false;
                    continue;
                }
                m->vx = (dxSp > 0.0f ? 1.0f : -1.0f) * 2.5f;
                m->facingRight = (m->vx > 0.0f);
            }
        }

        // Anti-wall penetration check: if mob penetrates a solid block, unstick by pushing into adjacent air
        int curFootX = (int)floorf(m->x);
        int curFootY = (int)floorf(m->y + 0.05f);
        int curHeadY = (int)floorf(m->y + m->height * 0.85f);

        bool footStuck = isSolidBlock(curFootX, curFootY);
        bool headStuck = (m->height > 0.75f) && isSolidBlock(curFootX, curHeadY);

        if (footStuck || headStuck) {
            // Unstick by pushing into nearest adjacent air block
            if (!isSolidBlock(curFootX, curFootY + 1) && !isSolidBlock(curFootX, curFootY + 2)) {
                m->y = (float)(curFootY + 1);
                m->vy = 0.0f;
            } else if (!isSolidBlock(curFootX + 1, curFootY) && !isSolidBlock(curFootX + 1, curFootY + 1)) {
                m->x = (float)(curFootX + 1) + m->width * 0.5f + 0.05f;
                m->vx = 1.0f;
            } else if (!isSolidBlock(curFootX - 1, curFootY) && !isSolidBlock(curFootX - 1, curFootY + 1)) {
                m->x = (float)curFootX - m->width * 0.5f - 0.05f;
                m->vx = -1.0f;
            } else if (!isSolidBlock(curFootX, curFootY + 2) && !isSolidBlock(curFootX, curFootY + 3)) {
                m->y = (float)(curFootY + 2);
                m->vy = 0.0f;
            } else if (!isSolidBlock(curFootX, curFootY - 1)) {
                m->y = (float)(curFootY - 1);
                m->vy = 0.0f;
            }
        }

        float dxToPlayer = playerX - m->x;
        float dyToPlayer = (playerY + 0.9f) - m->y;
        float distToPlayer = sqrtf(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);

        // --------------------------------------------------------------------
        // GREEN SLIME: Hopping state machine with squash & stretch
        // --------------------------------------------------------------------
        if (m->type == MOB_SLIME) {
            m->vy -= 28.0f * dt; // Gravity

            if (m->state == SLIME_STATE_IDLE) {
                m->animTimer -= dt;
                m->vx *= 0.80f;

                if (m->animTimer <= 0.0f && m->isGrounded) {
                    m->state = SLIME_STATE_JUMP;
                    m->animTimer = 0.0f;
                    m->isGrounded = false;

                    float leapDir = (distToPlayer < 18.0f) ? ((dxToPlayer > 0.0f) ? 1.0f : -1.0f)
                                                           : ((rand() % 2 == 0) ? 1.0f : -1.0f);
                    m->facingRight = (leapDir > 0.0f);
                    m->vx = leapDir * (3.8f + ((float)rand() / RAND_MAX) * 1.6f);
                    m->vy = 9.8f + ((float)rand() / RAND_MAX) * 2.2f;

                    if (distToPlayer < 24.0f) {
                        playSound(SFX_SLIME_JUMP);
                    }
                }
            } else if (m->state == SLIME_STATE_JUMP) {
                m->animTimer += dt;
            } else if (m->state == SLIME_STATE_LAND) {
                m->animTimer -= dt;
                m->vx *= 0.72f;
                if (m->animTimer <= 0.0f) {
                    m->state = SLIME_STATE_IDLE;
                    m->animTimer = 0.7f + ((float)rand() / RAND_MAX) * 0.9f;
                }
            }
        }
        // --------------------------------------------------------------------
        // CAVE BAT: Flying state machine with sine-wave glides and swoop pursuit
        // --------------------------------------------------------------------
        else if (m->type == MOB_BAT) {
            m->animTimer += dt;
            m->soundTimer -= dt;

            if (distToPlayer < 16.0f) {
                bool wasSwoop = (m->state == BAT_STATE_SWOOP);
                m->state = BAT_STATE_SWOOP;
                if (!wasSwoop && distToPlayer < 20.0f) {
                    playSound(SFX_BAT_CHIRP);
                    m->soundTimer = 2.5f + ((float)rand() / RAND_MAX) * 2.5f;
                } else if (m->soundTimer <= 0.0f && distToPlayer < 18.0f) {
                    playSound(SFX_BAT_CHIRP);
                    m->soundTimer = 3.0f + ((float)rand() / RAND_MAX) * 3.0f;
                }
                float targetVX = (dxToPlayer > 0.0f ? 1.0f : -1.0f) * 4.5f;
                float targetVY = (dyToPlayer / (distToPlayer + 0.001f)) * 3.8f + sinf(m->animTimer * 5.0f) * 1.5f;
                m->vx += (targetVX - m->vx) * 3.5f * dt;
                m->vy += (targetVY - m->vy) * 3.5f * dt;
                m->facingRight = (dxToPlayer > 0.0f);
            } else {
                m->state = BAT_STATE_WANDER;
                if (m->soundTimer <= 0.0f && distToPlayer < 20.0f) {
                    playSound(SFX_BAT_CHIRP);
                    m->soundTimer = 4.5f + ((float)rand() / RAND_MAX) * 4.5f;
                }
                float targetVX = (m->facingRight ? 2.6f : -2.6f);
                float targetVY = sinf(m->animTimer * 2.8f) * 2.0f;
                m->vx += (targetVX - m->vx) * 2.2f * dt;
                m->vy += (targetVY - m->vy) * 2.2f * dt;

                int checkX = (int)floorf(m->x + (m->facingRight ? 0.8f : -0.8f));
                int checkY = (int)floorf(m->y);
                if (isSolidBlock(checkX, checkY)) {
                    m->facingRight = !m->facingRight;
                }
            }

            float nextX = m->x + m->vx * dt;
            float nextY = m->y + m->vy * dt;
            int testX = (int)floorf(nextX);
            int testY = (int)floorf(nextY);
            if (isSolidBlock(testX, testY)) {
                m->vx = -m->vx * 0.6f;
                m->vy = -m->vy * 0.6f;
            } else {
                m->x = nextX;
                m->y = nextY;
            }

            // Player contact combat for Bat
            float pLeft = playerX - 0.38f;
            float pRight = playerX + 0.38f;
            float pBottom = playerY;
            float pTop = playerY + 1.8f;
            float mLeft = m->x - m->width * 0.45f;
            float mRight = m->x + m->width * 0.45f;
            float mBottom = m->y;
            float mTop = m->y + m->height;

            if (pRight > mLeft && pLeft < mRight && pTop > mBottom && pBottom < mTop) {
                onPlayerHurt(8.0f, m->x);
            }
            continue; // Bats skip grounded block collision
        }
        // --------------------------------------------------------------------
        // ZOMBIE: Bipedal walker, 1-block auto step climbing, player pursuit
        // --------------------------------------------------------------------
        else if (m->type == MOB_ZOMBIE) {
            m->vy -= 30.0f * dt; // Gravity
            m->animTimer += dt;
            m->soundTimer -= dt;

            float targetVX = 0.0f;
            if (distToPlayer < 22.0f) {
                bool wasChase = (m->state == ZOMBIE_STATE_CHASE);
                m->state = ZOMBIE_STATE_CHASE;
                if (!wasChase && distToPlayer < 22.0f) {
                    playSound(SFX_ZOMBIE_GROWL);
                    m->soundTimer = 3.5f + ((float)rand() / RAND_MAX) * 3.5f;
                } else if (m->soundTimer <= 0.0f && distToPlayer < 20.0f) {
                    playSound(SFX_ZOMBIE_GROWL);
                    m->soundTimer = 4.5f + ((float)rand() / RAND_MAX) * 4.5f;
                }
                float dir = (dxToPlayer > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (dir > 0.0f);
                targetVX = dir * 2.4f;
            } else {
                m->state = ZOMBIE_STATE_WANDER;
                if (m->soundTimer <= 0.0f && distToPlayer < 22.0f) {
                    playSound(SFX_ZOMBIE_GROWL);
                    m->soundTimer = 6.0f + ((float)rand() / RAND_MAX) * 6.0f;
                }
                float wanderDir = (sinf(m->animTimer * 0.4f) > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (wanderDir > 0.0f);
                targetVX = wanderDir * 1.3f;
            }

            float accel = m->isGrounded ? 32.0f : 16.0f;
            m->vx += (targetVX - m->vx) * accel * dt;

            // 1-Block Auto Step Climbing for Zombie
            if (m->isGrounded && fabsf(m->vx) > 0.2f) {
                int checkX = (m->vx > 0.0f) ? (int)floorf(m->x + m->width * 0.5f + 0.15f)
                                            : (int)floorf(m->x - m->width * 0.5f - 0.15f);
                int footY = (int)floorf(m->y);
                int headY = (int)floorf(m->y + m->height);

                if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
                    m->y += 16.0f * dt;
                    if (m->y > (float)(footY + 1.0f)) m->y = (float)(footY + 1.0f);
                }
            }
        }
        // --------------------------------------------------------------------
        // PEACEFUL ANIMALS: SHEEP, COW, PIG (Graze, wander, step climbing, do not attack)
        // --------------------------------------------------------------------
        else if (m->type == MOB_SHEEP || m->type == MOB_COW || m->type == MOB_PIG) {
            m->vy -= 28.0f * dt; // Gravity
            m->animTimer += dt;
            m->stateTimer -= dt;
            m->soundTimer -= dt;

            // Periodic characteristic vocalization (Baa, Moo, Oink)
            if (m->soundTimer <= 0.0f && distToPlayer < 22.0f) {
                if (m->type == MOB_SHEEP) {
                    playSound(SFX_SHEEP_BAA);
                    m->soundTimer = 6.5f + ((float)rand() / RAND_MAX) * 7.5f;
                } else if (m->type == MOB_COW) {
                    playSound(SFX_COW_MOO);
                    m->soundTimer = 8.0f + ((float)rand() / RAND_MAX) * 8.5f;
                } else if (m->type == MOB_PIG) {
                    playSound(SFX_PIG_OINK);
                    m->soundTimer = 5.5f + ((float)rand() / RAND_MAX) * 6.5f;
                }
            }

            float walkSpeed = 0.95f;
            if (m->type == MOB_COW) walkSpeed = 0.85f;
            else if (m->type == MOB_PIG) walkSpeed = 1.15f;

            if (m->state == ANIMAL_STATE_FLEE) {
                // Panicked running when attacked
                float fleeDir = (dxToPlayer > 0.0f) ? -1.0f : 1.0f;
                m->facingRight = (fleeDir > 0.0f);
                float targetVX = fleeDir * (walkSpeed * 2.8f);
                m->vx += (targetVX - m->vx) * 24.0f * dt;

                if (m->stateTimer <= 0.0f && m->hurtTimer <= 0.0f) {
                    m->state = ANIMAL_STATE_WANDER;
                    m->stateTimer = 4.0f + ((float)rand() / RAND_MAX) * 4.0f;
                }
            } else if (m->state == ANIMAL_STATE_GRAZE) {
                // Peaceful grazing on grass
                m->vx *= 0.70f;
                if (m->stateTimer <= 0.0f) {
                    m->state = ANIMAL_STATE_WANDER;
                    m->stateTimer = 4.0f + ((float)rand() / RAND_MAX) * 5.0f;
                    if (rand() % 3 == 0) m->facingRight = !m->facingRight;
                }
            } else if (m->state == ANIMAL_STATE_IDLE) {
                // Peaceful idle looking around
                m->vx *= 0.70f;
                if (m->stateTimer <= 0.0f) {
                    m->state = ANIMAL_STATE_WANDER;
                    m->stateTimer = 3.5f + ((float)rand() / RAND_MAX) * 4.5f;
                    if (rand() % 2 == 0) m->facingRight = !m->facingRight;
                }
            } else {
                // ANIMAL_STATE_WANDER
                float targetVX = (m->facingRight ? 1.0f : -1.0f) * walkSpeed;
                float accel = m->isGrounded ? 18.0f : 8.0f;
                m->vx += (targetVX - m->vx) * accel * dt;

                if (m->stateTimer <= 0.0f) {
                    if (rand() % 2 == 0 && m->type != MOB_PIG) {
                        m->state = ANIMAL_STATE_GRAZE;
                        m->stateTimer = 2.5f + ((float)rand() / RAND_MAX) * 2.0f;
                    } else {
                        m->state = ANIMAL_STATE_IDLE;
                        m->stateTimer = 1.8f + ((float)rand() / RAND_MAX) * 1.5f;
                    }
                }
            }

            // 1-Block Auto Step Climbing for Sheep, Cow, Pig
            if (m->isGrounded && fabsf(m->vx) > 0.15f) {
                int checkX = (m->vx > 0.0f) ? (int)floorf(m->x + m->width * 0.5f + 0.15f)
                                            : (int)floorf(m->x - m->width * 0.5f - 0.15f);
                int footY = (int)floorf(m->y);
                int headY = (int)floorf(m->y + m->height);

                if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
                    m->y += 15.0f * dt;
                    if (m->y > (float)(footY + 1.0f)) m->y = (float)(footY + 1.0f);
                } else if (isSolidBlock(checkX, footY) && isSolidBlock(checkX, footY + 1)) {
                    // Turn around when encountering a wall
                    m->facingRight = !m->facingRight;
                    m->vx = -m->vx * 0.4f;
                }
            }
        }
        // --------------------------------------------------------------------
        // TIMID RABBIT: Timid, hops away from player, periodic quick hops
        // --------------------------------------------------------------------
        else if (m->type == MOB_RABBIT) {
            m->vy -= 28.0f * dt; // Gravity
            m->animTimer += dt;
            m->stateTimer -= dt;

            if (distToPlayer < 7.5f || m->hurtTimer > 0.0f) {
                // Timid behavior: hops rapidly away when player approaches!
                m->state = ANIMAL_STATE_FLEE;
                float fleeDir = (dxToPlayer > 0.0f) ? -1.0f : 1.0f;
                m->facingRight = (fleeDir > 0.0f);

                if (m->isGrounded && m->stateTimer <= 0.0f) {
                    m->vy = 6.2f;
                    m->vx = fleeDir * 4.2f;
                    m->isGrounded = false;
                    m->stateTimer = 0.28f; // Rapid hopping cadence
                }
            } else {
                // Calm wandering: cute small hops and pauses
                if (m->stateTimer <= 0.0f) {
                    if (rand() % 3 == 0) {
                        m->state = ANIMAL_STATE_IDLE;
                        m->stateTimer = 1.2f + ((float)rand() / RAND_MAX) * 1.5f;
                        if (rand() % 2 == 0) m->facingRight = !m->facingRight;
                    } else {
                        m->state = ANIMAL_STATE_WANDER;
                        if (m->isGrounded) {
                            m->vy = 4.2f;
                            m->vx = (m->facingRight ? 1.0f : -1.0f) * 2.2f;
                            m->isGrounded = false;
                        }
                        m->stateTimer = 0.7f + ((float)rand() / RAND_MAX) * 0.8f;
                    }
                }
            }

            if (m->isGrounded && m->state != ANIMAL_STATE_FLEE) {
                m->vx *= 0.82f;
            }

            // 1-Block Auto Step Climbing for Rabbit
            if (m->isGrounded && fabsf(m->vx) > 0.15f) {
                int checkX = (m->vx > 0.0f) ? (int)floorf(m->x + m->width * 0.5f + 0.15f)
                                            : (int)floorf(m->x - m->width * 0.5f - 0.15f);
                int footY = (int)floorf(m->y);
                int headY = (int)floorf(m->y + m->height);

                if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
                    m->y += 16.0f * dt;
                    if (m->y > (float)(footY + 1.0f)) m->y = (float)(footY + 1.0f);
                } else if (isSolidBlock(checkX, footY) && isSolidBlock(checkX, footY + 1)) {
                    m->facingRight = !m->facingRight;
                    m->vx = -m->vx;
                }
            }
        }
        // --------------------------------------------------------------------
        // SKELETON: Cavern wanderer, bipedal step climber, bone weapon thrust with knockback
        // --------------------------------------------------------------------
        else if (m->type == MOB_SKELETON) {
            m->vy -= 30.0f * dt; // Gravity
            m->animTimer += dt;
            m->soundTimer -= dt;
            m->attackTimer += dt;

            float targetVX = 0.0f;
            if (distToPlayer < 20.0f) {
                bool wasChase = (m->state == SKELETON_STATE_CHASE || m->state == SKELETON_STATE_ATTACK);
                float dir = (dxToPlayer > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (dir > 0.0f);

                if (!wasChase && distToPlayer < 20.0f) {
                    playSound(SFX_SKELETON_RATTLE);
                    m->soundTimer = 3.5f + ((float)rand() / RAND_MAX) * 3.0f;
                } else if (m->soundTimer <= 0.0f && distToPlayer < 18.0f) {
                    playSound(SFX_SKELETON_RATTLE);
                    m->soundTimer = 4.0f + ((float)rand() / RAND_MAX) * 3.5f;
                }

                if (distToPlayer <= 2.2f) {
                    // Attack range: thrust bone weapon!
                    m->state = SKELETON_STATE_ATTACK;
                    targetVX = dir * 0.9f;
                    if (m->attackTimer >= 0.85f) {
                        m->attackTimer = 0.0f;
                        playSound(SFX_SKELETON_RATTLE);
                        if (distToPlayer <= 2.4f) {
                            onPlayerHurt(16.0f, m->x);
                        }
                    }
                } else {
                    m->state = SKELETON_STATE_CHASE;
                    targetVX = dir * 2.3f;
                }
            } else {
                m->state = SKELETON_STATE_WANDER;
                float wanderDir = (sinf(m->animTimer * 0.35f) > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (wanderDir > 0.0f);
                targetVX = wanderDir * 1.2f;

                if (m->soundTimer <= 0.0f && distToPlayer < 22.0f) {
                    playSound(SFX_SKELETON_RATTLE);
                    m->soundTimer = 5.0f + ((float)rand() / RAND_MAX) * 4.0f;
                }
            }

            float accel = m->isGrounded ? 30.0f : 15.0f;
            m->vx += (targetVX - m->vx) * accel * dt;

            // 1-Block Auto Step Climbing for Skeleton
            if (m->isGrounded && fabsf(m->vx) > 0.15f) {
                int checkX = (m->vx > 0.0f) ? (int)floorf(m->x + m->width * 0.5f + 0.15f)
                                            : (int)floorf(m->x - m->width * 0.5f - 0.15f);
                int footY = (int)floorf(m->y);
                int headY = (int)floorf(m->y + m->height);

                if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
                    m->y += 16.0f * dt;
                    if (m->y > (float)(footY + 1.0f)) m->y = (float)(footY + 1.0f);
                }
            }
        }
        // --------------------------------------------------------------------
        // SPIDER: 8 scuttling legs, fast movement, leaps at player within 4.5 blocks
        // --------------------------------------------------------------------
        else if (m->type == MOB_SPIDER) {
            m->vy -= 28.0f * dt; // Gravity
            m->animTimer += dt;
            m->soundTimer -= dt;
            if (m->attackTimer > 0.0f) m->attackTimer -= dt;

            float targetVX = 0.0f;
            if (distToPlayer < 20.0f) {
                float dir = (dxToPlayer > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (dir > 0.0f);

                // Leap attack at player within 4.5 blocks!
                if (distToPlayer <= 4.5f && m->isGrounded && m->attackTimer <= 0.0f) {
                    m->state = SPIDER_STATE_LEAP;
                    m->attackTimer = 1.8f + ((float)rand() / RAND_MAX) * 1.0f; // Leap cooldown
                    m->isGrounded = false;
                    m->vx = dir * 6.6f;
                    m->vy = 7.6f;
                    playSound(SFX_SPIDER_HISS);
                } else if (m->state != SPIDER_STATE_LEAP || m->isGrounded) {
                    m->state = SPIDER_STATE_CHASE;
                    targetVX = dir * 3.8f; // Fast scuttling movement
                    if (m->soundTimer <= 0.0f && distToPlayer < 16.0f) {
                        playSound(SFX_SPIDER_HISS);
                        m->soundTimer = 3.5f + ((float)rand() / RAND_MAX) * 3.0f;
                    }
                }
            } else {
                m->state = SPIDER_STATE_WANDER;
                float wanderDir = (sinf(m->animTimer * 0.45f) > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (wanderDir > 0.0f);
                targetVX = wanderDir * 2.2f;
            }

            if (m->state != SPIDER_STATE_LEAP || m->isGrounded) {
                float accel = m->isGrounded ? 36.0f : 16.0f;
                m->vx += (targetVX - m->vx) * accel * dt;
            }

            // 1-Block Auto Step Climbing for Spider
            if (m->isGrounded && fabsf(m->vx) > 0.15f) {
                int checkX = (m->vx > 0.0f) ? (int)floorf(m->x + m->width * 0.5f + 0.15f)
                                            : (int)floorf(m->x - m->width * 0.5f - 0.15f);
                int footY = (int)floorf(m->y);
                int headY = (int)floorf(m->y + m->height);

                if (isSolidBlock(checkX, footY) && !isSolidBlock(checkX, footY + 1) && !isSolidBlock(checkX, headY + 1)) {
                    m->y += 18.0f * dt;
                    if (m->y > (float)(footY + 1.0f)) m->y = (float)(footY + 1.0f);
                }
            }
        }
        // --------------------------------------------------------------------
        // GHOST: Translucent cyan-blue spectral entity, floating sine bobbing, aerial pursuit
        // --------------------------------------------------------------------
        else if (m->type == MOB_GHOST) {
            m->animTimer += dt;
            float sineBob = sinf(m->animTimer * 3.0f) * 1.4f;

            if (distToPlayer < 24.0f) {
                m->state = GHOST_STATE_CHASE;
                float targetVX = (dxToPlayer > 0.0f ? 1.0f : -1.0f) * 2.8f;
                float targetVY = (dyToPlayer / (distToPlayer + 0.001f)) * 2.4f + sineBob * 0.5f;
                m->vx += (targetVX - m->vx) * 2.2f * dt;
                m->vy += (targetVY - m->vy) * 2.2f * dt;
                m->facingRight = (dxToPlayer > 0.0f);
            } else {
                m->state = GHOST_STATE_DRIFT;
                float targetVX = (m->facingRight ? 1.4f : -1.4f);
                float targetVY = sineBob;
                m->vx += (targetVX - m->vx) * 1.6f * dt;
                m->vy += (targetVY - m->vy) * 1.6f * dt;

                int checkX = (int)floorf(m->x + (m->facingRight ? 0.9f : -0.9f));
                int checkY = (int)floorf(m->y);
                if (isSolidBlock(checkX, checkY)) {
                    m->facingRight = !m->facingRight;
                }
            }

            float nextX = m->x + m->vx * dt;
            float nextY = m->y + m->vy * dt;
            int testX = (int)floorf(nextX);
            int testY = (int)floorf(nextY);
            if (isSolidBlock(testX, testY)) {
                m->vx = -m->vx * 0.5f;
                m->vy += 1.2f;
            } else {
                m->x = nextX;
                m->y = nextY;
            }

            // Ethereal glowing cyan trail particle
            if ((rand() % 5) == 0) {
                spawnParticle(
                    m->x + ((float)rand() / RAND_MAX - 0.5f) * 0.4f,
                    m->y + ((float)rand() / RAND_MAX) * m->height,
                    0.30f, 0.85f, 1.0f,
                    ((float)rand() / RAND_MAX - 0.5f) * 0.4f,
                    0.4f + ((float)rand() / RAND_MAX) * 0.6f
                );
            }

            // Player Contact Combat Collision for Ghost
            float pLeft = playerX - 0.38f;
            float pRight = playerX + 0.38f;
            float pBottom = playerY;
            float pTop = playerY + 1.8f;

            float mLeft = m->x - m->width * 0.45f;
            float mRight = m->x + m->width * 0.45f;
            float mBottom = m->y;
            float mTop = m->y + m->height;

            if (pRight > mLeft && pLeft < mRight && pTop > mBottom && pBottom < mTop) {
                onPlayerHurt(15.0f, m->x);
            }
            continue; // Ghost floats freely through air; skips grounded block physics
        }
        // --------------------------------------------------------------------
        // LAVA SLIME: Fiery magma cube in underworld, fiery orange-red glow, high jump & ember particles
        // --------------------------------------------------------------------
        else if (m->type == MOB_LAVA_SLIME) {
            m->vy -= 28.0f * dt; // Gravity

            if (m->state == LAVA_SLIME_STATE_IDLE) {
                m->animTimer -= dt;
                m->vx *= 0.80f;

                if (m->animTimer <= 0.0f && m->isGrounded) {
                    m->state = LAVA_SLIME_STATE_JUMP;
                    m->animTimer = 0.0f;
                    m->isGrounded = false;

                    float leapDir = (distToPlayer < 20.0f) ? ((dxToPlayer > 0.0f) ? 1.0f : -1.0f)
                                                           : ((rand() % 2 == 0) ? 1.0f : -1.0f);
                    m->facingRight = (leapDir > 0.0f);
                    m->vx = leapDir * (4.5f + ((float)rand() / RAND_MAX) * 2.0f);
                    // High jump! (13.8f vs 9.8f for regular slime)
                    m->vy = 13.8f + ((float)rand() / RAND_MAX) * 2.4f;

                    if (distToPlayer < 24.0f) {
                        playSound(SFX_SLIME_JUMP);
                    }

                    // Fiery ember particles burst on jump
                    for (int p = 0; p < 8; p++) {
                        spawnParticle(
                            m->x + ((float)rand() / RAND_MAX - 0.5f) * m->width,
                            m->y + 0.1f,
                            1.0f, 0.45f + ((float)rand() / RAND_MAX) * 0.35f, 0.08f,
                            ((float)rand() / RAND_MAX - 0.5f) * 4.0f,
                            2.0f + ((float)rand() / RAND_MAX) * 3.5f
                        );
                    }
                }
            } else if (m->state == LAVA_SLIME_STATE_JUMP) {
                m->animTimer += dt;
                // Trailing fiery ember particles in air
                if ((rand() % 2) == 0) {
                    spawnParticle(
                        m->x + ((float)rand() / RAND_MAX - 0.5f) * m->width * 0.6f,
                        m->y + ((float)rand() / RAND_MAX) * m->height * 0.6f,
                        1.0f, 0.55f, 0.10f,
                        ((float)rand() / RAND_MAX - 0.5f) * 1.2f,
                        -0.4f + ((float)rand() / RAND_MAX) * 0.8f
                    );
                }
            } else if (m->state == LAVA_SLIME_STATE_LAND) {
                m->animTimer -= dt;
                m->vx *= 0.72f;
                if (m->animTimer <= 0.0f) {
                    m->state = LAVA_SLIME_STATE_IDLE;
                    m->animTimer = 0.6f + ((float)rand() / RAND_MAX) * 0.8f;
                }
            }
        }

        // --------------------------------------------------------------------
        // Grounded Collision against g_World (Slimes, Zombies, Sheep, Cow, Pig, Rabbit, Skeletons, Spiders, Lava Slimes)
        // --------------------------------------------------------------------
        float nextX = m->x + m->vx * dt;
        int minX = (int)floorf(nextX - m->width * 0.5f);
        int maxX = (int)floorf(nextX + m->width * 0.5f);
        int minY = (int)floorf(m->y);
        int maxY = (int)floorf(m->y + m->height - 0.08f);

        // Horizontal collision resolution with 1-block auto step climbing
        for (int y = minY; y <= maxY; y++) {
            if (m->vx > 0.0f && isSolidBlock(maxX, y)) {
                // If grounded and the obstacle is only 1-block high with clear air above, step up!
                if (y == minY && m->isGrounded && !isSolidBlock(maxX, minY + 1) && !isSolidBlock(maxX, maxY + 1)) {
                    m->y = (float)(minY + 1.0f);
                    minY = (int)floorf(m->y);
                    continue;
                }
                nextX = (float)maxX - m->width * 0.5f - 0.001f;
                m->vx = 0.0f;
                if (isPeacefulAnimal(m->type)) {
                    m->facingRight = false;
                }
                break;
            } else if (m->vx < 0.0f && isSolidBlock(minX, y)) {
                // If grounded and the obstacle is only 1-block high with clear air above, step up!
                if (y == minY && m->isGrounded && !isSolidBlock(minX, minY + 1) && !isSolidBlock(minX, maxY + 1)) {
                    m->y = (float)(minY + 1.0f);
                    minY = (int)floorf(m->y);
                    continue;
                }
                nextX = (float)(minX + 1) + m->width * 0.5f + 0.001f;
                m->vx = 0.0f;
                if (isPeacefulAnimal(m->type)) {
                    m->facingRight = true;
                }
                break;
            }
        }
        m->x = nextX;

        // Vertical collision resolution
        float nextY = m->y + m->vy * dt;
        minX = (int)floorf(m->x - m->width * 0.5f + 0.05f);
        maxX = (int)floorf(m->x + m->width * 0.5f - 0.05f);
        minY = (int)floorf(nextY);
        maxY = (int)floorf(nextY + m->height);

        bool wasGrounded = m->isGrounded;
        m->isGrounded = false;

        if (m->vy <= 0.0f) {
            for (int x = minX; x <= maxX; x++) {
                if (isSolidBlock(x, minY)) {
                    nextY = (float)(minY + 1);
                    m->vy = 0.0f;
                    m->isGrounded = true;

                    // Slime & Lava Slime impact squash transition
                    if ((m->type == MOB_SLIME || m->type == MOB_LAVA_SLIME) && !wasGrounded) {
                        m->state = (m->type == MOB_LAVA_SLIME) ? LAVA_SLIME_STATE_LAND : SLIME_STATE_LAND;
                        m->animTimer = 0.20f;
                    }
                    break;
                }
            }
        } else {
            for (int x = minX; x <= maxX; x++) {
                if (isSolidBlock(x, maxY)) {
                    nextY = (float)maxY - m->height - 0.001f;
                    m->vy = 0.0f;
                    break;
                }
            }
        }
        m->y = nextY;

        // Player Contact Combat Collision (Hostile mobs and Slimes only!)
        float pLeft = playerX - 0.38f;
        float pRight = playerX + 0.38f;
        float pBottom = playerY;
        float pTop = playerY + 1.8f;

        float mLeft = m->x - m->width * 0.45f;
        float mRight = m->x + m->width * 0.45f;
        float mBottom = m->y;
        float mTop = m->y + m->height;

        if (pRight > mLeft && pLeft < mRight && pTop > mBottom && pBottom < mTop) {
            // Peaceful animals NEVER attack player
            if (isHostileMob(m->type) || m->type == MOB_SLIME) {
                float mobDmg = 12.0f;
                if (m->type == MOB_SLIME) mobDmg = 10.0f;
                else if (m->type == MOB_BAT) mobDmg = 8.0f;
                else if (m->type == MOB_ZOMBIE) {
                    mobDmg = 18.0f;
                    playSound(SFX_ZOMBIE_GROWL);
                } else if (m->type == MOB_SKELETON) {
                    mobDmg = 16.0f;
                    playSound(SFX_SKELETON_RATTLE);
                } else if (m->type == MOB_SPIDER) {
                    mobDmg = 14.0f;
                    playSound(SFX_SPIDER_HISS);
                } else if (m->type == MOB_GHOST) {
                    mobDmg = 15.0f;
                } else if (m->type == MOB_LAVA_SLIME) {
                    mobDmg = 18.0f;
                }
                onPlayerHurt(mobDmg, m->x);
            }
        }
    }

    // Deep subterranean ambient horror cues (bone rattle / spider hiss in pitch dark)
    static float s_CavernAtmosphereTimer = 15.0f;
    if (playerY < CAVERN_LEVEL) {
        s_CavernAtmosphereTimer -= dt;
        if (s_CavernAtmosphereTimer <= 0.0f) {
            if (rand() % 2 == 0) {
                playSound(SFX_SKELETON_RATTLE);
            } else {
                playSound(SFX_SPIDER_HISS);
            }
            s_CavernAtmosphereTimer = 16.0f + ((float)rand() / RAND_MAX) * 20.0f;
        }
    }
}

// Color lighting and hurt-flash macro
#define MOB_RGB(r0, g0, b0) \
    glColor3f((1.0f - hurtFlash) * ((r0) * depthLight) + hurtFlash * 1.0f, \
              (1.0f - hurtFlash) * ((g0) * depthLight) + hurtFlash * 0.25f, \
              (1.0f - hurtFlash) * ((b0) * depthLight) + hurtFlash * 0.25f)

#define MOB_RGBA(r0, g0, b0, a0) \
    glColor4f((1.0f - hurtFlash) * ((r0) * depthLight) + hurtFlash * 1.0f, \
              (1.0f - hurtFlash) * ((g0) * depthLight) + hurtFlash * 0.25f, \
              (1.0f - hurtFlash) * ((b0) * depthLight) + hurtFlash * 0.25f, (a0))

void renderMobs(float daylight, float camX, float camY) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < MAX_MOBS; i++) {
        if (!g_Mobs[i].active) continue;
        Mob* m = &g_Mobs[i];

        // Frustum cull mobs outside camera view bounds
        if (m->x < camX - 25.0f || m->x > camX + 25.0f ||
            m->y < camY - 16.0f || m->y > camY + 16.0f) {
            continue;
        }

        // Lighting calculation based on depth and daylight
        float depthLight = (m->y < CAVERN_LEVEL) ? 0.40f : daylight;
        if (depthLight < 0.28f) depthLight = 0.28f;

        // Hurt flash intensity
        float hurtFlash = (m->hurtTimer > 0.0f) ? (m->hurtTimer / 0.32f) : 0.0f;

        glPushMatrix();
        glTranslatef(m->x, m->y, m->z);
        if (!m->facingRight) {
            glScalef(-1.0f, 1.0f, 1.0f);
        }

        // ====================================================================
        // 1. GREEN SLIME: Translucent Gelatinous Body + Inner Core + Eyes
        // ====================================================================
        if (m->type == MOB_SLIME) {
            float sx = 1.0f, sy = 1.0f;
            if (m->state == SLIME_STATE_IDLE) {
                if (m->animTimer < 0.22f) {
                    float t = 1.0f - (m->animTimer / 0.22f);
                    sx = 1.0f + 0.32f * t;
                    sy = 1.0f - 0.26f * t;
                } else {
                    float b = sinf(m->animTimer * 5.0f);
                    sx = 1.0f + b * 0.05f;
                    sy = 1.0f - b * 0.05f;
                }
            } else if (m->state == SLIME_STATE_JUMP) {
                sx = 0.82f;
                sy = 1.25f;
            } else if (m->state == SLIME_STATE_LAND) {
                float t = m->animTimer / 0.20f;
                sx = 1.0f + 0.38f * t;
                sy = 1.0f - 0.30f * t;
            }

            glScalef(sx, sy, 1.0f);

            // Outer Gelatinous Translucent Body
            MOB_RGBA(0.24f, 0.92f, 0.35f, 0.78f);
            float w2 = m->width * 0.5f;
            float h = m->height;

            glBegin(GL_POLYGON);
            glVertex3f(-w2 * 0.85f, 0.0f,     0.0f);
            glVertex3f( w2 * 0.85f, 0.0f,     0.0f);
            glVertex3f( w2,         h * 0.3f, 0.0f);
            glVertex3f( w2 * 0.90f, h * 0.8f, 0.0f);
            glVertex3f( w2 * 0.50f, h,        0.0f);
            glVertex3f(-w2 * 0.50f, h,        0.0f);
            glVertex3f(-w2 * 0.90f, h * 0.8f, 0.0f);
            glVertex3f(-w2,         h * 0.3f, 0.0f);
            glEnd();

            // Inner Floating Dark Emerald Core Nucleus
            MOB_RGBA(0.12f, 0.55f, 0.20f, 0.92f);
            float coreBob = sinf(m->animTimer * 6.0f) * 0.03f;
            glBegin(GL_QUADS);
            glVertex3f(-0.16f, h * 0.28f + coreBob, 0.01f);
            glVertex3f( 0.16f, h * 0.28f + coreBob, 0.01f);
            glVertex3f( 0.16f, h * 0.60f + coreBob, 0.01f);
            glVertex3f(-0.16f, h * 0.60f + coreBob, 0.01f);
            glEnd();

            // Eyes
            glColor3f(0.08f, 0.12f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(0.05f, h * 0.45f, 0.02f);
            glVertex3f(0.14f, h * 0.45f, 0.02f);
            glVertex3f(0.14f, h * 0.62f, 0.02f);
            glVertex3f(0.05f, h * 0.62f, 0.02f);

            glVertex3f(0.20f, h * 0.45f, 0.02f);
            glVertex3f(0.29f, h * 0.45f, 0.02f);
            glVertex3f(0.29f, h * 0.62f, 0.02f);
            glVertex3f(0.20f, h * 0.62f, 0.02f);
            glEnd();

            // Specular dots
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(0.07f, h * 0.54f, 0.03f);
            glVertex3f(0.11f, h * 0.54f, 0.03f);
            glVertex3f(0.11f, h * 0.60f, 0.03f);
            glVertex3f(0.07f, h * 0.60f, 0.03f);

            glVertex3f(0.22f, h * 0.54f, 0.03f);
            glVertex3f(0.26f, h * 0.54f, 0.03f);
            glVertex3f(0.26f, h * 0.60f, 0.03f);
            glVertex3f(0.22f, h * 0.60f, 0.03f);
            glEnd();
        }
        // ====================================================================
        // 2. CAVE BAT: Articulated Flapping Wings + Dark Fur + Glowing Eyes
        // ====================================================================
        else if (m->type == MOB_BAT) {
            float flapAngle = sinf(m->animTimer * 18.0f) * 44.0f;

            // Bat Central Body & Head
            MOB_RGB(0.28f, 0.24f, 0.34f);
            glBegin(GL_QUADS);
            glVertex3f(-0.16f, 0.08f, 0.0f);
            glVertex3f( 0.16f, 0.08f, 0.0f);
            glVertex3f( 0.16f, 0.44f, 0.0f);
            glVertex3f(-0.16f, 0.44f, 0.0f);
            glEnd();

            // Pointed Bat Ears
            glBegin(GL_TRIANGLES);
            glVertex3f(-0.16f, 0.42f, 0.01f);
            glVertex3f(-0.06f, 0.42f, 0.01f);
            glVertex3f(-0.13f, 0.58f, 0.01f);

            glVertex3f( 0.06f, 0.42f, 0.01f);
            glVertex3f( 0.16f, 0.42f, 0.01f);
            glVertex3f( 0.13f, 0.58f, 0.01f);
            glEnd();

            // Glowing Crimson Eyes
            glColor3f(1.0f, 0.15f, 0.15f);
            glBegin(GL_QUADS);
            glVertex3f(0.04f, 0.28f, 0.02f);
            glVertex3f(0.12f, 0.28f, 0.02f);
            glVertex3f(0.12f, 0.35f, 0.02f);
            glVertex3f(0.04f, 0.35f, 0.02f);
            glEnd();

            // Articulated Wings
            MOB_RGBA(0.38f, 0.28f, 0.44f, 0.88f);

            glPushMatrix();
            glTranslatef(-0.14f, 0.30f, 0.01f);
            glRotatef(flapAngle, 0.0f, 0.0f, 1.0f);
            glBegin(GL_TRIANGLES);
            glVertex3f( 0.00f,  0.00f, 0.0f);
            glVertex3f(-0.35f,  0.15f, 0.0f);
            glVertex3f(-0.18f, -0.22f, 0.0f);

            glVertex3f(-0.35f,  0.15f, 0.0f);
            glVertex3f(-0.62f,  0.08f, 0.0f);
            glVertex3f(-0.38f, -0.15f, 0.0f);
            glEnd();
            glPopMatrix();

            glPushMatrix();
            glTranslatef(0.14f, 0.30f, 0.01f);
            glRotatef(-flapAngle, 0.0f, 0.0f, 1.0f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.00f,  0.00f, 0.0f);
            glVertex3f(0.35f,  0.15f, 0.0f);
            glVertex3f(0.18f, -0.22f, 0.0f);

            glVertex3f(0.35f,  0.15f, 0.0f);
            glVertex3f(0.62f,  0.08f, 0.0f);
            glVertex3f(0.38f, -0.15f, 0.0f);
            glEnd();
            glPopMatrix();
        }
        // ====================================================================
        // 3. ZOMBIE: Green Skin + Ragged Blue Shirt + Outstretched Arms
        // ====================================================================
        else if (m->type == MOB_ZOMBIE) {
            float sinWalk = sinf(m->animTimer * 6.5f);
            float legAngleL = m->isGrounded ? sinWalk * 0.18f : 0.12f;
            float legAngleR = m->isGrounded ? -sinWalk * 0.18f : -0.12f;

            // Ragged Dark Cyan/Blue Shirt Torso
            MOB_RGB(0.22f, 0.46f, 0.62f);
            glBegin(GL_QUADS);
            glVertex3f(-0.30f, 0.48f, 0.0f);
            glVertex3f( 0.30f, 0.48f, 0.0f);
            glVertex3f( 0.30f, 1.28f, 0.0f);
            glVertex3f(-0.30f, 1.28f, 0.0f);
            glEnd();

            // Ragged Tattered Edge exposing green skin
            MOB_RGB(0.34f, 0.64f, 0.36f);
            glBegin(GL_QUADS);
            glVertex3f(-0.24f, 0.45f, 0.01f);
            glVertex3f( 0.12f, 0.45f, 0.01f);
            glVertex3f( 0.12f, 0.54f, 0.01f);
            glVertex3f(-0.24f, 0.54f, 0.01f);
            glEnd();

            // Zombie Head
            MOB_RGB(0.34f, 0.64f, 0.36f);
            glBegin(GL_QUADS);
            glVertex3f(-0.25f, 1.28f, 0.0f);
            glVertex3f( 0.25f, 1.28f, 0.0f);
            glVertex3f( 0.25f, 1.74f, 0.0f);
            glVertex3f(-0.25f, 1.74f, 0.0f);
            glEnd();

            // Menacing Dark Red Eyes
            glColor3f(0.85f, 0.18f, 0.18f);
            glBegin(GL_QUADS);
            glVertex3f(0.08f, 1.44f, 0.02f);
            glVertex3f(0.20f, 1.44f, 0.02f);
            glVertex3f(0.20f, 1.54f, 0.02f);
            glVertex3f(0.08f, 1.54f, 0.02f);
            glEnd();

            // Dark Blue Tattered Pants Legs
            MOB_RGB(0.15f, 0.32f, 0.45f);
            glBegin(GL_QUADS);
            glVertex3f(-0.22f + legAngleL, 0.0f,  0.0f);
            glVertex3f(-0.06f + legAngleL, 0.0f,  0.0f);
            glVertex3f(-0.06f,             0.48f, 0.0f);
            glVertex3f(-0.22f,             0.48f, 0.0f);

            glVertex3f( 0.06f + legAngleR, 0.0f,  0.0f);
            glVertex3f( 0.22f + legAngleR, 0.0f,  0.0f);
            glVertex3f( 0.22f,             0.48f, 0.0f);
            glVertex3f( 0.06f,             0.48f, 0.0f);
            glEnd();

            // Horizontal Outstretched Arms
            float armLurch = sinf(m->animTimer * 4.0f) * 0.04f;
            MOB_RGB(0.22f, 0.46f, 0.62f);
            glBegin(GL_QUADS);
            glVertex3f(0.15f, 0.95f + armLurch, 0.02f);
            glVertex3f(0.35f, 0.95f + armLurch, 0.02f);
            glVertex3f(0.35f, 1.15f + armLurch, 0.02f);
            glVertex3f(0.15f, 1.15f + armLurch, 0.02f);
            glEnd();

            MOB_RGB(0.34f, 0.64f, 0.36f);
            glBegin(GL_QUADS);
            glVertex3f(0.35f, 0.98f + armLurch, 0.02f);
            glVertex3f(0.70f, 0.98f + armLurch, 0.02f);
            glVertex3f(0.70f, 1.12f + armLurch, 0.02f);
            glVertex3f(0.35f, 1.12f + armLurch, 0.02f);
            glEnd();
        }
        // ====================================================================
        // 4. SHEEP: White Fluffy Fleece Body, Dark Head, 4 Legs
        // ====================================================================
        else if (m->type == MOB_SHEEP) {
            float sinWalk = sinf(m->animTimer * 6.5f);
            bool isMoving = (m->state == ANIMAL_STATE_WANDER || m->state == ANIMAL_STATE_FLEE) && fabsf(m->vx) > 0.1f;
            float legSwingF = isMoving ? sinWalk * 0.10f : 0.0f;
            float legSwingB = isMoving ? -sinWalk * 0.10f : 0.0f;
            bool isGrazing = (m->state == ANIMAL_STATE_GRAZE);
            float headYOff = isGrazing ? -0.22f : sinf(m->animTimer * 2.0f) * 0.02f;
            float headXOff = isGrazing ? 0.06f : 0.0f;

            // 4 Legs (Dark charcoal hooves)
            MOB_RGB(0.24f, 0.22f, 0.21f);
            glBegin(GL_QUADS);
            // Left back leg (background)
            glVertex3f(-0.34f - legSwingB, 0.0f,  -0.01f);
            glVertex3f(-0.24f - legSwingB, 0.0f,  -0.01f);
            glVertex3f(-0.24f,             0.35f, -0.01f);
            glVertex3f(-0.34f,             0.35f, -0.01f);
            // Right back leg (background)
            glVertex3f( 0.18f - legSwingF, 0.0f,  -0.01f);
            glVertex3f( 0.28f - legSwingF, 0.0f,  -0.01f);
            glVertex3f( 0.28f,             0.35f, -0.01f);
            glVertex3f( 0.18f,             0.35f, -0.01f);

            // Left front leg (foreground)
            glVertex3f(-0.30f + legSwingB, 0.0f,  0.02f);
            glVertex3f(-0.20f + legSwingB, 0.0f,  0.02f);
            glVertex3f(-0.20f,             0.35f, 0.02f);
            glVertex3f(-0.30f,             0.35f, 0.02f);
            // Right front leg (foreground)
            glVertex3f( 0.22f + legSwingF, 0.0f,  0.02f);
            glVertex3f( 0.32f + legSwingF, 0.0f,  0.02f);
            glVertex3f( 0.32f,             0.35f, 0.02f);
            glVertex3f( 0.22f,             0.35f, 0.02f);
            glEnd();

            // Fluffy Fleece Main Body (White fluffy cloud)
            MOB_RGB(0.96f, 0.95f, 0.93f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.46f, 0.32f, 0.0f);
            glVertex3f( 0.38f, 0.32f, 0.0f);
            glVertex3f( 0.44f, 0.45f, 0.0f);
            glVertex3f( 0.44f, 0.78f, 0.0f);
            glVertex3f( 0.36f, 0.88f, 0.0f);
            glVertex3f(-0.40f, 0.88f, 0.0f);
            glVertex3f(-0.48f, 0.78f, 0.0f);
            glVertex3f(-0.48f, 0.45f, 0.0f);
            glEnd();

            // Fluffy Wool Puffs & Cloud bumps
            MOB_RGB(0.92f, 0.91f, 0.89f);
            glBegin(GL_QUADS);
            glVertex3f( 0.22f, 0.78f, 0.01f);
            glVertex3f( 0.42f, 0.78f, 0.01f);
            glVertex3f( 0.38f, 0.92f, 0.01f);
            glVertex3f( 0.20f, 0.92f, 0.01f);

            glVertex3f(-0.42f, 0.76f, 0.01f);
            glVertex3f(-0.20f, 0.76f, 0.01f);
            glVertex3f(-0.22f, 0.91f, 0.01f);
            glVertex3f(-0.44f, 0.91f, 0.01f);

            // Little puffy tail
            glVertex3f(-0.54f, 0.60f, 0.01f);
            glVertex3f(-0.46f, 0.60f, 0.01f);
            glVertex3f(-0.46f, 0.72f, 0.01f);
            glVertex3f(-0.54f, 0.72f, 0.01f);
            glEnd();

            // Dark Head
            MOB_RGB(0.20f, 0.19f, 0.18f);
            glBegin(GL_QUADS);
            glVertex3f(0.30f + headXOff, 0.48f + headYOff, 0.02f);
            glVertex3f(0.62f + headXOff, 0.48f + headYOff, 0.02f);
            glVertex3f(0.62f + headXOff, 0.78f + headYOff, 0.02f);
            glVertex3f(0.30f + headXOff, 0.78f + headYOff, 0.02f);
            glEnd();

            // Dark Drooping Ear
            MOB_RGB(0.18f, 0.17f, 0.16f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.34f + headXOff, 0.72f + headYOff, 0.03f);
            glVertex3f(0.42f + headXOff, 0.72f + headYOff, 0.03f);
            glVertex3f(0.32f + headXOff, 0.54f + headYOff, 0.03f);
            glEnd();

            // Eye & Highlight
            glColor3f(0.08f, 0.08f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(0.50f + headXOff, 0.64f + headYOff, 0.03f);
            glVertex3f(0.56f + headXOff, 0.64f + headYOff, 0.03f);
            glVertex3f(0.56f + headXOff, 0.72f + headYOff, 0.03f);
            glVertex3f(0.50f + headXOff, 0.72f + headYOff, 0.03f);
            glEnd();

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(0.53f + headXOff, 0.68f + headYOff, 0.04f);
            glVertex3f(0.55f + headXOff, 0.68f + headYOff, 0.04f);
            glVertex3f(0.55f + headXOff, 0.71f + headYOff, 0.04f);
            glVertex3f(0.53f + headXOff, 0.71f + headYOff, 0.04f);
            glEnd();
        }
        // ====================================================================
        // 5. COW: Black/White Spotted Hide, Horns, Sturdy Legs
        // ====================================================================
        else if (m->type == MOB_COW) {
            float sinWalk = sinf(m->animTimer * 5.5f);
            bool isMoving = (m->state == ANIMAL_STATE_WANDER || m->state == ANIMAL_STATE_FLEE) && fabsf(m->vx) > 0.1f;
            float legSwingF = isMoving ? sinWalk * 0.12f : 0.0f;
            float legSwingB = isMoving ? -sinWalk * 0.12f : 0.0f;
            bool isGrazing = (m->state == ANIMAL_STATE_GRAZE);
            float headYOff = isGrazing ? -0.25f : sinf(m->animTimer * 1.8f) * 0.02f;
            float headXOff = isGrazing ? 0.06f : 0.0f;

            // 4 Sturdy White Legs
            MOB_RGB(0.95f, 0.94f, 0.92f);
            glBegin(GL_QUADS);
            glVertex3f(-0.38f - legSwingB, 0.10f, -0.01f);
            glVertex3f(-0.24f - legSwingB, 0.10f, -0.01f);
            glVertex3f(-0.24f,             0.42f, -0.01f);
            glVertex3f(-0.38f,             0.42f, -0.01f);

            glVertex3f( 0.22f - legSwingF, 0.10f, -0.01f);
            glVertex3f( 0.36f - legSwingF, 0.10f, -0.01f);
            glVertex3f( 0.36f,             0.42f, -0.01f);
            glVertex3f( 0.22f,             0.42f, -0.01f);

            glVertex3f(-0.34f + legSwingB, 0.10f, 0.02f);
            glVertex3f(-0.20f + legSwingB, 0.10f, 0.02f);
            glVertex3f(-0.20f,             0.42f, 0.02f);
            glVertex3f(-0.34f,             0.42f, 0.02f);

            glVertex3f( 0.26f + legSwingF, 0.10f, 0.02f);
            glVertex3f( 0.40f + legSwingF, 0.10f, 0.02f);
            glVertex3f( 0.40f,             0.42f, 0.02f);
            glVertex3f( 0.26f,             0.42f, 0.02f);
            glEnd();

            // Dark Hooves
            MOB_RGB(0.20f, 0.18f, 0.18f);
            glBegin(GL_QUADS);
            glVertex3f(-0.38f - legSwingB, 0.0f, -0.01f);
            glVertex3f(-0.24f - legSwingB, 0.0f, -0.01f);
            glVertex3f(-0.24f - legSwingB, 0.10f, -0.01f);
            glVertex3f(-0.38f - legSwingB, 0.10f, -0.01f);

            glVertex3f( 0.22f - legSwingF, 0.0f, -0.01f);
            glVertex3f( 0.36f - legSwingF, 0.0f, -0.01f);
            glVertex3f( 0.36f - legSwingF, 0.10f, -0.01f);
            glVertex3f( 0.22f - legSwingF, 0.10f, -0.01f);

            glVertex3f(-0.34f + legSwingB, 0.0f, 0.02f);
            glVertex3f(-0.20f + legSwingB, 0.0f, 0.02f);
            glVertex3f(-0.20f + legSwingB, 0.10f, 0.02f);
            glVertex3f(-0.34f + legSwingB, 0.10f, 0.02f);

            glVertex3f( 0.26f + legSwingF, 0.0f, 0.02f);
            glVertex3f( 0.40f + legSwingF, 0.0f, 0.02f);
            glVertex3f( 0.40f + legSwingF, 0.10f, 0.02f);
            glVertex3f( 0.26f + legSwingF, 0.10f, 0.02f);
            glEnd();

            // White Hide Body Torso
            MOB_RGB(0.95f, 0.94f, 0.92f);
            glBegin(GL_QUADS);
            glVertex3f(-0.54f, 0.38f, 0.0f);
            glVertex3f( 0.48f, 0.38f, 0.0f);
            glVertex3f( 0.48f, 0.98f, 0.0f);
            glVertex3f(-0.54f, 0.98f, 0.0f);
            glEnd();

            // Black Spotted Patches
            MOB_RGB(0.18f, 0.18f, 0.19f);
            glBegin(GL_QUADS);
            glVertex3f(-0.40f, 0.58f, 0.01f);
            glVertex3f(-0.14f, 0.58f, 0.01f);
            glVertex3f(-0.14f, 0.92f, 0.01f);
            glVertex3f(-0.40f, 0.92f, 0.01f);

            glVertex3f( 0.06f, 0.42f, 0.01f);
            glVertex3f( 0.32f, 0.42f, 0.01f);
            glVertex3f( 0.32f, 0.74f, 0.01f);
            glVertex3f( 0.06f, 0.74f, 0.01f);
            glEnd();

            // Pink Udder
            MOB_RGB(0.96f, 0.72f, 0.75f);
            glBegin(GL_QUADS);
            glVertex3f(-0.35f, 0.30f, 0.01f);
            glVertex3f(-0.18f, 0.30f, 0.01f);
            glVertex3f(-0.18f, 0.40f, 0.01f);
            glVertex3f(-0.35f, 0.40f, 0.01f);
            glEnd();

            // Cow Head (White base)
            MOB_RGB(0.95f, 0.94f, 0.92f);
            glBegin(GL_QUADS);
            glVertex3f(0.38f + headXOff, 0.56f + headYOff, 0.02f);
            glVertex3f(0.76f + headXOff, 0.56f + headYOff, 0.02f);
            glVertex3f(0.76f + headXOff, 1.02f + headYOff, 0.02f);
            glVertex3f(0.38f + headXOff, 1.02f + headYOff, 0.02f);
            glEnd();

            // Black Eye Patch
            MOB_RGB(0.18f, 0.18f, 0.19f);
            glBegin(GL_QUADS);
            glVertex3f(0.46f + headXOff, 0.72f + headYOff, 0.03f);
            glVertex3f(0.68f + headXOff, 0.72f + headYOff, 0.03f);
            glVertex3f(0.68f + headXOff, 0.98f + headYOff, 0.03f);
            glVertex3f(0.46f + headXOff, 0.98f + headYOff, 0.03f);
            glEnd();

            // Dark Muzzle / Snout
            MOB_RGB(0.45f, 0.36f, 0.35f);
            glBegin(GL_QUADS);
            glVertex3f(0.64f + headXOff, 0.56f + headYOff, 0.03f);
            glVertex3f(0.86f + headXOff, 0.56f + headYOff, 0.03f);
            glVertex3f(0.86f + headXOff, 0.76f + headYOff, 0.03f);
            glVertex3f(0.64f + headXOff, 0.76f + headYOff, 0.03f);
            glEnd();

            // Nostrils
            MOB_RGB(0.20f, 0.15f, 0.15f);
            glBegin(GL_QUADS);
            glVertex3f(0.78f + headXOff, 0.60f + headYOff, 0.04f);
            glVertex3f(0.83f + headXOff, 0.60f + headYOff, 0.04f);
            glVertex3f(0.83f + headXOff, 0.67f + headYOff, 0.04f);
            glVertex3f(0.78f + headXOff, 0.67f + headYOff, 0.04f);
            glEnd();

            // Ivory Pointed Horns
            MOB_RGB(0.92f, 0.88f, 0.74f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.45f + headXOff, 1.00f + headYOff, 0.03f);
            glVertex3f(0.53f + headXOff, 1.00f + headYOff, 0.03f);
            glVertex3f(0.42f + headXOff, 1.18f + headYOff, 0.03f);

            glVertex3f(0.58f + headXOff, 1.00f + headYOff, 0.03f);
            glVertex3f(0.66f + headXOff, 1.00f + headYOff, 0.03f);
            glVertex3f(0.57f + headXOff, 1.18f + headYOff, 0.03f);
            glEnd();

            // Eye & Highlight
            glColor3f(0.08f, 0.08f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(0.54f + headXOff, 0.80f + headYOff, 0.04f);
            glVertex3f(0.60f + headXOff, 0.80f + headYOff, 0.04f);
            glVertex3f(0.60f + headXOff, 0.88f + headYOff, 0.04f);
            glVertex3f(0.54f + headXOff, 0.88f + headYOff, 0.04f);
            glEnd();

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(0.57f + headXOff, 0.84f + headYOff, 0.05f);
            glVertex3f(0.59f + headXOff, 0.84f + headYOff, 0.05f);
            glVertex3f(0.59f + headXOff, 0.87f + headYOff, 0.05f);
            glVertex3f(0.57f + headXOff, 0.87f + headYOff, 0.05f);
            glEnd();
        }
        // ====================================================================
        // 6. PIG: Pink Body, Snout, Cute Trotting Legs
        // ====================================================================
        else if (m->type == MOB_PIG) {
            float sinWalk = sinf(m->animTimer * 8.5f);
            bool isMoving = (m->state == ANIMAL_STATE_WANDER || m->state == ANIMAL_STATE_FLEE) && fabsf(m->vx) > 0.1f;
            float legSwingF = isMoving ? sinWalk * 0.09f : 0.0f;
            float legSwingB = isMoving ? -sinWalk * 0.09f : 0.0f;

            // 4 Short Trotting Pink Legs
            MOB_RGB(0.96f, 0.62f, 0.66f);
            glBegin(GL_QUADS);
            glVertex3f(-0.32f - legSwingB, 0.0f,  -0.01f);
            glVertex3f(-0.20f - legSwingB, 0.0f,  -0.01f);
            glVertex3f(-0.20f,             0.28f, -0.01f);
            glVertex3f(-0.32f,             0.28f, -0.01f);

            glVertex3f( 0.16f - legSwingF, 0.0f,  -0.01f);
            glVertex3f( 0.28f - legSwingF, 0.0f,  -0.01f);
            glVertex3f( 0.28f,             0.28f, -0.01f);
            glVertex3f( 0.16f,             0.28f, -0.01f);

            glVertex3f(-0.28f + legSwingB, 0.0f,  0.02f);
            glVertex3f(-0.16f + legSwingB, 0.0f,  0.02f);
            glVertex3f(-0.16f,             0.28f, 0.02f);
            glVertex3f(-0.28f,             0.28f, 0.02f);

            glVertex3f( 0.20f + legSwingF, 0.0f,  0.02f);
            glVertex3f( 0.32f + legSwingF, 0.0f,  0.02f);
            glVertex3f( 0.32f,             0.28f, 0.02f);
            glVertex3f( 0.20f,             0.28f, 0.02f);
            glEnd();

            // Pink Body Torso
            MOB_RGB(0.98f, 0.68f, 0.72f);
            glBegin(GL_QUADS);
            glVertex3f(-0.44f, 0.25f, 0.0f);
            glVertex3f( 0.36f, 0.25f, 0.0f);
            glVertex3f( 0.36f, 0.72f, 0.0f);
            glVertex3f(-0.44f, 0.72f, 0.0f);
            glEnd();

            // Belly Shading
            MOB_RGB(0.90f, 0.58f, 0.62f);
            glBegin(GL_QUADS);
            glVertex3f(-0.40f, 0.25f, 0.01f);
            glVertex3f( 0.32f, 0.25f, 0.01f);
            glVertex3f( 0.32f, 0.35f, 0.01f);
            glVertex3f(-0.40f, 0.35f, 0.01f);
            glEnd();

            // Curly Tail
            float tailWiggle = sinf(m->animTimer * 10.0f) * 0.04f;
            MOB_RGB(0.96f, 0.58f, 0.64f);
            glBegin(GL_LINES);
            glVertex3f(-0.44f, 0.52f, 0.01f);
            glVertex3f(-0.54f, 0.58f + tailWiggle, 0.01f);
            glVertex3f(-0.54f, 0.58f + tailWiggle, 0.01f);
            glVertex3f(-0.50f, 0.66f + tailWiggle, 0.01f);
            glEnd();

            // Pink Head
            MOB_RGB(0.98f, 0.68f, 0.72f);
            glBegin(GL_QUADS);
            glVertex3f(0.26f, 0.36f, 0.02f);
            glVertex3f(0.62f, 0.36f, 0.02f);
            glVertex3f(0.62f, 0.72f, 0.02f);
            glVertex3f(0.26f, 0.72f, 0.02f);
            glEnd();

            // Floppy Ear
            MOB_RGB(0.92f, 0.55f, 0.60f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.32f, 0.70f, 0.03f);
            glVertex3f(0.44f, 0.70f, 0.03f);
            glVertex3f(0.38f, 0.82f, 0.03f);
            glEnd();

            // Snout (Forward protruding rectangle)
            MOB_RGB(0.96f, 0.54f, 0.58f);
            glBegin(GL_QUADS);
            glVertex3f(0.58f, 0.40f, 0.03f);
            glVertex3f(0.74f, 0.40f, 0.03f);
            glVertex3f(0.74f, 0.56f, 0.03f);
            glVertex3f(0.58f, 0.56f, 0.03f);
            glEnd();

            // Nostrils
            MOB_RGB(0.65f, 0.30f, 0.35f);
            glBegin(GL_QUADS);
            glVertex3f(0.64f, 0.45f, 0.04f);
            glVertex3f(0.68f, 0.45f, 0.04f);
            glVertex3f(0.68f, 0.51f, 0.04f);
            glVertex3f(0.64f, 0.51f, 0.04f);

            glVertex3f(0.70f, 0.45f, 0.04f);
            glVertex3f(0.73f, 0.45f, 0.04f);
            glVertex3f(0.73f, 0.51f, 0.04f);
            glVertex3f(0.70f, 0.51f, 0.04f);
            glEnd();

            // Eye with specular dot
            glColor3f(0.08f, 0.08f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(0.48f, 0.55f, 0.03f);
            glVertex3f(0.54f, 0.55f, 0.03f);
            glVertex3f(0.54f, 0.62f, 0.03f);
            glVertex3f(0.48f, 0.62f, 0.03f);
            glEnd();

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(0.51f, 0.59f, 0.04f);
            glVertex3f(0.53f, 0.59f, 0.04f);
            glVertex3f(0.53f, 0.61f, 0.04f);
            glVertex3f(0.51f, 0.61f, 0.04f);
            glEnd();
        }
        // ====================================================================
        // 7. RABBIT: Small Fluffy Body, Long Ears, Quick Hopping Animation
        // ====================================================================
        else if (m->type == MOB_RABBIT) {
            bool isMoving = (m->state == ANIMAL_STATE_FLEE || !m->isGrounded || fabsf(m->vx) > 0.1f);
            float hopBob = isMoving ? fabsf(sinf(m->animTimer * 14.0f)) * 0.10f : 0.0f;

            glPushMatrix();
            glTranslatef(0.0f, hopBob, 0.0f);

            // Tiny paws
            MOB_RGB(0.78f, 0.68f, 0.56f);
            glBegin(GL_QUADS);
            glVertex3f(-0.16f, 0.0f,  0.01f);
            glVertex3f(-0.06f, 0.0f,  0.01f);
            glVertex3f(-0.06f, 0.10f, 0.01f);
            glVertex3f(-0.16f, 0.10f, 0.01f);

            glVertex3f( 0.08f, 0.0f,  0.01f);
            glVertex3f( 0.18f, 0.0f,  0.01f);
            glVertex3f( 0.18f, 0.10f, 0.01f);
            glVertex3f( 0.08f, 0.10f, 0.01f);
            glEnd();

            // Fluffy Sandy Body
            MOB_RGB(0.85f, 0.75f, 0.64f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.20f, 0.08f, 0.0f);
            glVertex3f( 0.16f, 0.08f, 0.0f);
            glVertex3f( 0.20f, 0.20f, 0.0f);
            glVertex3f( 0.14f, 0.34f, 0.0f);
            glVertex3f(-0.16f, 0.34f, 0.0f);
            glVertex3f(-0.22f, 0.22f, 0.0f);
            glEnd();

            // White Fluffy Chest
            MOB_RGB(0.96f, 0.95f, 0.92f);
            glBegin(GL_QUADS);
            glVertex3f(0.04f, 0.10f, 0.01f);
            glVertex3f(0.18f, 0.10f, 0.01f);
            glVertex3f(0.16f, 0.26f, 0.01f);
            glVertex3f(0.04f, 0.26f, 0.01f);
            glEnd();

            // Fluffy White Cotton Ball Tail
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(-0.26f, 0.16f, 0.01f);
            glVertex3f(-0.18f, 0.16f, 0.01f);
            glVertex3f(-0.18f, 0.26f, 0.01f);
            glVertex3f(-0.26f, 0.26f, 0.01f);
            glEnd();

            // Rabbit Head
            MOB_RGB(0.85f, 0.75f, 0.64f);
            glBegin(GL_QUADS);
            glVertex3f(0.08f, 0.20f, 0.02f);
            glVertex3f(0.25f, 0.20f, 0.02f);
            glVertex3f(0.25f, 0.38f, 0.02f);
            glVertex3f(0.08f, 0.38f, 0.02f);
            glEnd();

            // Twitching Pink Nose
            float noseTwitch = sinf(m->animTimer * 16.0f) * 0.01f;
            MOB_RGB(0.96f, 0.65f, 0.70f);
            glBegin(GL_QUADS);
            glVertex3f(0.24f, 0.23f + noseTwitch, 0.03f);
            glVertex3f(0.28f, 0.23f + noseTwitch, 0.03f);
            glVertex3f(0.28f, 0.28f + noseTwitch, 0.03f);
            glVertex3f(0.24f, 0.28f + noseTwitch, 0.03f);
            glEnd();

            // Long Distinctive Upright Ears
            MOB_RGB(0.82f, 0.72f, 0.60f);
            glBegin(GL_QUADS);
            glVertex3f(0.08f, 0.36f, 0.02f);
            glVertex3f(0.14f, 0.36f, 0.02f);
            glVertex3f(0.12f, 0.58f, 0.02f);
            glVertex3f(0.06f, 0.58f, 0.02f);

            glVertex3f(0.15f, 0.36f, 0.02f);
            glVertex3f(0.21f, 0.36f, 0.02f);
            glVertex3f(0.19f, 0.58f, 0.02f);
            glVertex3f(0.13f, 0.58f, 0.02f);
            glEnd();

            // Pink Inner Ear Linings
            MOB_RGB(0.96f, 0.68f, 0.72f);
            glBegin(GL_QUADS);
            glVertex3f(0.08f, 0.38f, 0.03f);
            glVertex3f(0.12f, 0.38f, 0.03f);
            glVertex3f(0.11f, 0.54f, 0.03f);
            glVertex3f(0.07f, 0.54f, 0.03f);

            glVertex3f(0.15f, 0.38f, 0.03f);
            glVertex3f(0.19f, 0.38f, 0.03f);
            glVertex3f(0.18f, 0.54f, 0.03f);
            glVertex3f(0.14f, 0.54f, 0.03f);
            glEnd();

            // Eye & Highlight
            glColor3f(0.08f, 0.08f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(0.16f, 0.28f, 0.03f);
            glVertex3f(0.20f, 0.28f, 0.03f);
            glVertex3f(0.20f, 0.34f, 0.03f);
            glVertex3f(0.16f, 0.34f, 0.03f);
            glEnd();

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex3f(0.18f, 0.31f, 0.04f);
            glVertex3f(0.20f, 0.31f, 0.04f);
            glVertex3f(0.20f, 0.33f, 0.04f);
            glVertex3f(0.18f, 0.33f, 0.04f);
            glEnd();

            glPopMatrix();
        }
        // ====================================================================
        // 8. MOB_SKELETON (骷髅战士): Pale Ivory Model (Skull, Ribcage, Limbs, Bone Weapon)
        // ====================================================================
        else if (m->type == MOB_SKELETON) {
            float sinWalk = sinf(m->animTimer * 6.5f);
            float legSwingL = m->isGrounded ? sinWalk * 0.16f : 0.10f;
            float legSwingR = m->isGrounded ? -sinWalk * 0.16f : -0.10f;

            // Attack bone weapon thrust
            float thrust = 0.0f;
            if (m->state == SKELETON_STATE_ATTACK) {
                thrust = sinf(m->attackTimer * 3.14159f / 0.85f) * 0.35f;
                if (thrust < 0.0f) thrust = 0.0f;
            }

            // Central Ivory Spine Column
            MOB_RGB(0.85f, 0.83f, 0.76f);
            glBegin(GL_QUADS);
            glVertex3f(-0.04f, 0.58f, 0.0f);
            glVertex3f( 0.04f, 0.58f, 0.0f);
            glVertex3f( 0.04f, 1.30f, 0.0f);
            glVertex3f(-0.04f, 1.30f, 0.0f);
            glEnd();

            // Pelvic Girdle
            MOB_RGB(0.88f, 0.86f, 0.80f);
            glBegin(GL_QUADS);
            glVertex3f(-0.18f, 0.48f, 0.01f);
            glVertex3f( 0.18f, 0.48f, 0.01f);
            glVertex3f( 0.15f, 0.60f, 0.01f);
            glVertex3f(-0.15f, 0.60f, 0.01f);
            glEnd();

            // Ribcage: 4 distinct pairs of curved ivory ribs with open hollow interior
            MOB_RGB(0.92f, 0.90f, 0.84f);
            glBegin(GL_QUADS);
            for (int r = 0; r < 4; r++) {
                float ribY = 0.72f + (float)r * 0.14f;
                // Left rib
                glVertex3f(-0.25f, ribY,         0.01f);
                glVertex3f(-0.02f, ribY + 0.03f, 0.01f);
                glVertex3f(-0.02f, ribY + 0.08f, 0.01f);
                glVertex3f(-0.25f, ribY + 0.06f, 0.01f);
                // Right rib
                glVertex3f( 0.02f, ribY + 0.03f, 0.01f);
                glVertex3f( 0.25f, ribY,         0.01f);
                glVertex3f( 0.25f, ribY + 0.06f, 0.01f);
                glVertex3f( 0.02f, ribY + 0.08f, 0.01f);
            }
            glEnd();

            // Ivory Cranium / Skull
            MOB_RGB(0.93f, 0.91f, 0.85f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.20f, 1.30f, 0.0f);
            glVertex3f( 0.20f, 1.30f, 0.0f);
            glVertex3f( 0.24f, 1.44f, 0.0f);
            glVertex3f( 0.22f, 1.74f, 0.0f);
            glVertex3f(-0.22f, 1.74f, 0.0f);
            glVertex3f(-0.24f, 1.44f, 0.0f);
            glEnd();

            // Hollow Dark Eye Sockets
            glColor3f(0.08f, 0.07f, 0.07f);
            glBegin(GL_QUADS);
            glVertex3f( 0.04f, 1.46f, 0.02f);
            glVertex3f( 0.15f, 1.46f, 0.02f);
            glVertex3f( 0.15f, 1.58f, 0.02f);
            glVertex3f( 0.04f, 1.58f, 0.02f);

            glVertex3f(-0.15f, 1.46f, 0.02f);
            glVertex3f(-0.04f, 1.46f, 0.02f);
            glVertex3f(-0.04f, 1.58f, 0.02f);
            glVertex3f(-0.15f, 1.58f, 0.02f);
            glEnd();

            // Faint glowing red pinpoint in eye sockets
            glColor3f(0.95f, 0.20f, 0.20f);
            glBegin(GL_QUADS);
            glVertex3f( 0.08f, 1.50f, 0.03f);
            glVertex3f( 0.12f, 1.50f, 0.03f);
            glVertex3f( 0.12f, 1.54f, 0.03f);
            glVertex3f( 0.08f, 1.54f, 0.03f);

            glVertex3f(-0.11f, 1.50f, 0.03f);
            glVertex3f(-0.07f, 1.50f, 0.03f);
            glVertex3f(-0.07f, 1.54f, 0.03f);
            glVertex3f(-0.11f, 1.54f, 0.03f);
            glEnd();

            // Nasal Cavity
            glColor3f(0.10f, 0.08f, 0.08f);
            glBegin(GL_TRIANGLES);
            glVertex3f(-0.03f, 1.40f, 0.02f);
            glVertex3f( 0.03f, 1.40f, 0.02f);
            glVertex3f( 0.00f, 1.45f, 0.02f);
            glEnd();

            // Skeleton Teeth / Jaw
            glColor3f(0.12f, 0.10f, 0.10f);
            glBegin(GL_LINES);
            glVertex3f(-0.12f, 1.34f, 0.02f);
            glVertex3f( 0.12f, 1.34f, 0.02f);
            for (float tx = -0.10f; tx <= 0.10f; tx += 0.04f) {
                glVertex3f(tx, 1.30f, 0.02f);
                glVertex3f(tx, 1.37f, 0.02f);
            }
            glEnd();

            // Thin Ivory Bone Legs
            MOB_RGB(0.88f, 0.86f, 0.80f);
            glBegin(GL_QUADS);
            // Left leg
            glVertex3f(-0.15f + legSwingL, 0.0f,  0.0f);
            glVertex3f(-0.08f + legSwingL, 0.0f,  0.0f);
            glVertex3f(-0.08f,             0.48f, 0.0f);
            glVertex3f(-0.15f,             0.48f, 0.0f);
            // Right leg
            glVertex3f( 0.08f + legSwingR, 0.0f,  0.0f);
            glVertex3f( 0.15f + legSwingR, 0.0f,  0.0f);
            glVertex3f( 0.15f,             0.48f, 0.0f);
            glVertex3f( 0.08f,             0.48f, 0.0f);
            glEnd();

            // Left Arm (background)
            MOB_RGB(0.82f, 0.80f, 0.74f);
            glBegin(GL_QUADS);
            glVertex3f(-0.25f - legSwingL * 0.8f, 0.65f, -0.01f);
            glVertex3f(-0.18f - legSwingL * 0.8f, 0.65f, -0.01f);
            glVertex3f(-0.20f,                    1.22f, -0.01f);
            glVertex3f(-0.27f,                    1.22f, -0.01f);
            glEnd();

            // Right Arm (foreground) with Bone Weapon
            MOB_RGB(0.90f, 0.88f, 0.82f);
            float armAngle = (m->state == SKELETON_STATE_ATTACK) ? 0.35f : 0.08f;
            glBegin(GL_QUADS);
            glVertex3f(0.18f + thrust, 1.15f, 0.02f);
            glVertex3f(0.24f + thrust, 1.15f, 0.02f);
            glVertex3f(0.42f + thrust, 0.95f + armAngle, 0.02f);
            glVertex3f(0.36f + thrust, 0.95f + armAngle, 0.02f);
            glEnd();

            // Bone Weapon (Carved sharp ivory bone blade / spear)
            MOB_RGB(0.96f, 0.95f, 0.90f);
            glBegin(GL_POLYGON);
            glVertex3f(0.36f + thrust, 0.92f + armAngle, 0.03f);
            glVertex3f(0.78f + thrust, 1.12f + armAngle, 0.03f);
            glVertex3f(0.92f + thrust, 1.16f + armAngle, 0.03f); // sharp point
            glVertex3f(0.76f + thrust, 1.24f + armAngle, 0.03f);
            glVertex3f(0.40f + thrust, 1.04f + armAngle, 0.03f);
            glEnd();
        }
        // ====================================================================
        // 9. MOB_SPIDER (洞穴蜘蛛): 8 Animated Scuttling Legs, Dark Body, Glowing Red Eyes
        // ====================================================================
        else if (m->type == MOB_SPIDER) {
            float scuttleSpeed = (m->state == SPIDER_STATE_CHASE) ? 24.0f : 14.0f;
            float legPhases[4] = { 0.0f, 1.57f, 3.14f, 4.71f };

            // 8 Animated Scuttling Legs (4 background legs, 4 foreground legs)
            MOB_RGB(0.12f, 0.10f, 0.10f);
            for (int k = 0; k < 4; k++) {
                float legCycle = sinf(m->animTimer * scuttleSpeed + legPhases[k]);
                float hipX = -0.18f + (float)k * 0.14f;
                float kneeX = hipX - 0.22f + legCycle * 0.08f;
                float kneeY = 0.38f + fabsf(legCycle) * 0.10f;
                float footX = hipX - 0.44f + legCycle * 0.14f;
                float footY = 0.0f;

                glBegin(GL_LINES);
                glVertex3f(hipX,  0.22f, -0.01f);
                glVertex3f(kneeX, kneeY, -0.01f);
                glVertex3f(kneeX, kneeY, -0.01f);
                glVertex3f(footX, footY, -0.01f);
                glEnd();
            }

            // Bulbous Dark Abdomen (Rear)
            MOB_RGB(0.10f, 0.08f, 0.09f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.54f, 0.14f, 0.0f);
            glVertex3f(-0.12f, 0.14f, 0.0f);
            glVertex3f( 0.02f, 0.28f, 0.0f);
            glVertex3f(-0.06f, 0.52f, 0.0f);
            glVertex3f(-0.38f, 0.56f, 0.0f);
            glVertex3f(-0.58f, 0.36f, 0.0f);
            glEnd();

            // Dark Chitin Abdomen Markings
            MOB_RGB(0.24f, 0.12f, 0.14f);
            glBegin(GL_QUADS);
            glVertex3f(-0.42f, 0.28f, 0.01f);
            glVertex3f(-0.20f, 0.28f, 0.01f);
            glVertex3f(-0.24f, 0.44f, 0.01f);
            glVertex3f(-0.38f, 0.44f, 0.01f);
            glEnd();

            // Cephalothorax (Head & Thorax)
            MOB_RGB(0.14f, 0.11f, 0.12f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.05f, 0.12f, 0.0f);
            glVertex3f( 0.32f, 0.12f, 0.0f);
            glVertex3f( 0.42f, 0.26f, 0.0f);
            glVertex3f( 0.32f, 0.42f, 0.0f);
            glVertex3f(-0.02f, 0.38f, 0.0f);
            glEnd();

            // Spider Chelicerae / Fangs
            MOB_RGB(0.20f, 0.06f, 0.08f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.36f, 0.16f, 0.02f);
            glVertex3f(0.44f, 0.16f, 0.02f);
            glVertex3f(0.42f, 0.04f, 0.02f);

            glVertex3f(0.28f, 0.16f, 0.02f);
            glVertex3f(0.36f, 0.16f, 0.02f);
            glVertex3f(0.34f, 0.04f, 0.02f);
            glEnd();

            // Glowing Ruby Red Eyes (Multiple arachnid eye cluster!)
            glColor3f(1.0f, 0.10f, 0.10f);
            glBegin(GL_QUADS);
            glVertex3f(0.32f, 0.28f, 0.03f);
            glVertex3f(0.40f, 0.28f, 0.03f);
            glVertex3f(0.40f, 0.36f, 0.03f);
            glVertex3f(0.32f, 0.36f, 0.03f);

            glVertex3f(0.22f, 0.32f, 0.03f);
            glVertex3f(0.28f, 0.32f, 0.03f);
            glVertex3f(0.28f, 0.37f, 0.03f);
            glVertex3f(0.22f, 0.37f, 0.03f);

            glVertex3f(0.34f, 0.21f, 0.03f);
            glVertex3f(0.39f, 0.21f, 0.03f);
            glVertex3f(0.39f, 0.25f, 0.03f);
            glVertex3f(0.34f, 0.25f, 0.03f);
            glEnd();

            // Eye specular glint
            glColor3f(1.0f, 0.85f, 0.85f);
            glBegin(GL_QUADS);
            glVertex3f(0.34f, 0.31f, 0.04f);
            glVertex3f(0.37f, 0.31f, 0.04f);
            glVertex3f(0.37f, 0.34f, 0.04f);
            glVertex3f(0.34f, 0.34f, 0.04f);
            glEnd();

            // 4 Foreground Legs (z = +0.02f)
            MOB_RGB(0.16f, 0.12f, 0.14f);
            for (int k = 0; k < 4; k++) {
                float legCycle = sinf(m->animTimer * scuttleSpeed + legPhases[k] + 3.14f);
                float hipX = -0.12f + (float)k * 0.14f;
                float kneeX = hipX + 0.22f + legCycle * 0.08f;
                float kneeY = 0.40f + fabsf(legCycle) * 0.12f;
                float footX = hipX + 0.46f + legCycle * 0.15f;
                float footY = 0.0f;

                glBegin(GL_LINES);
                glVertex3f(hipX,  0.22f, 0.02f);
                glVertex3f(kneeX, kneeY, 0.02f);
                glVertex3f(kneeX, kneeY, 0.02f);
                glVertex3f(footX, footY, 0.02f);
                glEnd();
            }
        }
        // ====================================================================
        // 10. MOB_GHOST (夜影幽灵): Translucent Cyan-Blue Spectral Entity (Sine Bobbing)
        // ====================================================================
        else if (m->type == MOB_GHOST) {
            float bobY = sinf(m->animTimer * 3.2f) * 0.09f;
            glPushMatrix();
            glTranslatef(0.0f, bobY, 0.0f);

            // Wavy trailing spectral tail points
            float w0 = sinf(m->animTimer * 5.0f) * 0.08f;
            float w1 = sinf(m->animTimer * 5.0f + 1.6f) * 0.08f;
            float w2 = sinf(m->animTimer * 5.0f + 3.2f) * 0.08f;

            // Translucent Outer Cyan Spectral Shroud
            MOB_RGBA(0.24f, 0.78f, 0.98f, 0.65f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.35f, 0.25f + w0, 0.0f);
            glVertex3f(-0.15f, 0.10f + w1, 0.0f);
            glVertex3f( 0.08f, 0.22f + w2, 0.0f);
            glVertex3f( 0.32f, 0.12f + w0, 0.0f);
            glVertex3f( 0.40f, 0.45f,      0.0f);
            glVertex3f( 0.36f, 0.88f,      0.0f);
            glVertex3f( 0.18f, 1.14f,      0.0f);
            glVertex3f(-0.18f, 1.14f,      0.0f);
            glVertex3f(-0.36f, 0.88f,      0.0f);
            glVertex3f(-0.40f, 0.45f,      0.0f);
            glEnd();

            // Inner Floating Ethereal Bright Core
            MOB_RGBA(0.68f, 0.94f, 1.0f, 0.78f);
            glBegin(GL_POLYGON);
            glVertex3f(-0.20f, 0.38f, 0.01f);
            glVertex3f( 0.18f, 0.38f, 0.01f);
            glVertex3f( 0.24f, 0.75f, 0.01f);
            glVertex3f( 0.12f, 0.98f, 0.01f);
            glVertex3f(-0.12f, 0.98f, 0.01f);
            glVertex3f(-0.24f, 0.75f, 0.01f);
            glEnd();

            // Wispy Ethereal Claws / Tendrils reaching forward
            MOB_RGBA(0.40f, 0.88f, 1.0f, 0.70f);
            glBegin(GL_TRIANGLES);
            glVertex3f(0.25f, 0.58f, 0.02f);
            glVertex3f(0.55f, 0.52f, 0.02f);
            glVertex3f(0.35f, 0.42f, 0.02f);

            glVertex3f(-0.10f, 0.52f, 0.02f);
            glVertex3f( 0.15f, 0.46f, 0.02f);
            glVertex3f( 0.02f, 0.36f, 0.02f);
            glEnd();

            // Piercing Pale Spectral Eyes
            glColor4f(0.92f, 1.0f, 1.0f, 0.95f);
            glBegin(GL_QUADS);
            glVertex3f(0.06f, 0.74f, 0.03f);
            glVertex3f(0.18f, 0.74f, 0.03f);
            glVertex3f(0.18f, 0.85f, 0.03f);
            glVertex3f(0.06f, 0.85f, 0.03f);

            glVertex3f(-0.15f, 0.74f, 0.03f);
            glVertex3f(-0.03f, 0.74f, 0.03f);
            glVertex3f(-0.03f, 0.85f, 0.03f);
            glVertex3f(-0.15f, 0.85f, 0.03f);
            glEnd();

            // Deep Spectral Pupils
            glColor4f(0.12f, 0.32f, 0.55f, 0.95f);
            glBegin(GL_QUADS);
            glVertex3f(0.10f, 0.76f, 0.04f);
            glVertex3f(0.15f, 0.76f, 0.04f);
            glVertex3f(0.15f, 0.82f, 0.04f);
            glVertex3f(0.10f, 0.82f, 0.04f);
            glEnd();

            glPopMatrix();
        }
        // ====================================================================
        // 11. MOB_LAVA_SLIME (熔岩史莱姆): Fiery Magma Cube in Underworld (Orange-Red Glow)
        // ====================================================================
        else if (m->type == MOB_LAVA_SLIME) {
            float sx = 1.0f, sy = 1.0f;
            if (m->state == LAVA_SLIME_STATE_IDLE) {
                if (m->animTimer < 0.22f) {
                    float t = 1.0f - (m->animTimer / 0.22f);
                    sx = 1.0f + 0.32f * t;
                    sy = 1.0f - 0.26f * t;
                } else {
                    float b = sinf(m->animTimer * 5.0f);
                    sx = 1.0f + b * 0.05f;
                    sy = 1.0f - b * 0.05f;
                }
            } else if (m->state == LAVA_SLIME_STATE_JUMP) {
                sx = 0.80f;
                sy = 1.30f;
            } else if (m->state == LAVA_SLIME_STATE_LAND) {
                float t = m->animTimer / 0.20f;
                sx = 1.0f + 0.40f * t;
                sy = 1.0f - 0.32f * t;
            }

            glScalef(sx, sy, 1.0f);

            float w2 = m->width * 0.5f;
            float h = m->height;

            // Outer Dark Obsidian/Basalt Magma Crust Cube
            MOB_RGB(0.18f, 0.11f, 0.10f);
            glBegin(GL_POLYGON);
            glVertex3f(-w2 * 0.90f, 0.0f,      0.0f);
            glVertex3f( w2 * 0.90f, 0.0f,      0.0f);
            glVertex3f( w2,         h * 0.25f, 0.0f);
            glVertex3f( w2,         h * 0.85f, 0.0f);
            glVertex3f( w2 * 0.85f, h,         0.0f);
            glVertex3f(-w2 * 0.85f, h,         0.0f);
            glVertex3f(-w2,         h * 0.85f, 0.0f);
            glVertex3f(-w2,         h * 0.25f, 0.0f);
            glEnd();

            // Pulsating Fiery Magma Veins & Glowing Fissures
            float pulse = 0.80f + 0.20f * sinf(m->animTimer * 7.0f);
            glColor3f(1.0f * pulse, 0.42f * pulse, 0.06f * pulse);
            glBegin(GL_QUADS);
            // Horizontal fiery fissure
            glVertex3f(-w2 * 0.85f, h * 0.45f, 0.01f);
            glVertex3f( w2 * 0.85f, h * 0.45f, 0.01f);
            glVertex3f( w2 * 0.85f, h * 0.55f, 0.01f);
            glVertex3f(-w2 * 0.85f, h * 0.55f, 0.01f);

            // Diagonal glowing cracks
            glVertex3f(-w2 * 0.40f, h * 0.15f, 0.01f);
            glVertex3f(-w2 * 0.30f, h * 0.15f, 0.01f);
            glVertex3f(-w2 * 0.10f, h * 0.45f, 0.01f);
            glVertex3f(-w2 * 0.20f, h * 0.45f, 0.01f);

            glVertex3f( w2 * 0.15f, h * 0.55f, 0.01f);
            glVertex3f( w2 * 0.25f, h * 0.55f, 0.01f);
            glVertex3f( w2 * 0.45f, h * 0.85f, 0.01f);
            glVertex3f( w2 * 0.35f, h * 0.85f, 0.01f);
            glEnd();

            // Inner Molten Incandescent Magma Core
            glColor4f(1.0f, 0.78f, 0.18f, 0.95f);
            float coreBob = sinf(m->animTimer * 6.0f) * 0.03f;
            glBegin(GL_QUADS);
            glVertex3f(-0.20f, h * 0.28f + coreBob, 0.02f);
            glVertex3f( 0.20f, h * 0.28f + coreBob, 0.02f);
            glVertex3f( 0.20f, h * 0.65f + coreBob, 0.02f);
            glVertex3f(-0.20f, h * 0.65f + coreBob, 0.02f);
            glEnd();

            // Fiery Glowing Eyes (Bright molten yellow with burning red centers)
            glColor3f(1.0f, 0.92f, 0.25f);
            glBegin(GL_QUADS);
            glVertex3f(0.08f, h * 0.52f, 0.03f);
            glVertex3f(0.20f, h * 0.52f, 0.03f);
            glVertex3f(0.20f, h * 0.70f, 0.03f);
            glVertex3f(0.08f, h * 0.70f, 0.03f);

            glVertex3f(0.26f, h * 0.52f, 0.03f);
            glVertex3f(0.38f, h * 0.52f, 0.03f);
            glVertex3f(0.38f, h * 0.70f, 0.03f);
            glVertex3f(0.26f, h * 0.70f, 0.03f);
            glEnd();

            // Molten Burning Pupils
            glColor3f(0.95f, 0.15f, 0.05f);
            glBegin(GL_QUADS);
            glVertex3f(0.12f, h * 0.56f, 0.04f);
            glVertex3f(0.18f, h * 0.56f, 0.04f);
            glVertex3f(0.18f, h * 0.66f, 0.04f);
            glVertex3f(0.12f, h * 0.66f, 0.04f);

            glVertex3f(0.30f, h * 0.56f, 0.04f);
            glVertex3f(0.36f, h * 0.56f, 0.04f);
            glVertex3f(0.36f, h * 0.66f, 0.04f);
            glVertex3f(0.30f, h * 0.66f, 0.04f);
            glEnd();
        }

        // ====================================================================
        // Floating Health Bar (Rendered when mob is injured)
        // ====================================================================
        if (m->health < m->maxHealth && m->health > 0.0f) {
            float barY = m->height + 0.25f;
            float barW = 0.85f;
            float barH = 0.10f;
            float pct = m->health / m->maxHealth;
            if (pct < 0.0f) pct = 0.0f;
            if (pct > 1.0f) pct = 1.0f;

            // Background dark bar
            glColor4f(0.12f, 0.12f, 0.12f, 0.85f);
            glBegin(GL_QUADS);
            glVertex3f(-barW * 0.5f, barY,        0.04f);
            glVertex3f( barW * 0.5f, barY,        0.04f);
            glVertex3f( barW * 0.5f, barY + barH, 0.04f);
            glVertex3f(-barW * 0.5f, barY + barH, 0.04f);
            glEnd();

            // Foreground Health Fill (Red to Green gradient)
            float fillR = (pct < 0.5f) ? 1.0f : 2.0f * (1.0f - pct);
            float fillG = (pct > 0.5f) ? 1.0f : 2.0f * pct;
            glColor4f(fillR, fillG, 0.15f, 0.95f);
            glBegin(GL_QUADS);
            glVertex3f(-barW * 0.5f,                 barY + 0.015f,        0.05f);
            glVertex3f(-barW * 0.5f + barW * pct,    barY + 0.015f,        0.05f);
            glVertex3f(-barW * 0.5f + barW * pct,    barY + barH - 0.015f, 0.05f);
            glVertex3f(-barW * 0.5f,                 barY + barH - 0.015f, 0.05f);
            glEnd();
        }

        glPopMatrix();
    }

    glDisable(GL_BLEND);
}

bool checkMobAttackEx(float clickWorldX, float clickWorldY, float damage, float knockbackMult, float playerX, float playerY) {
    float dxP = clickWorldX - playerX;
    float dyP = clickWorldY - (playerY + 0.9f);
    if (sqrtf(dxP * dxP + dyP * dyP) > REACH_DISTANCE + 1.2f) {
        return false;
    }

    Mob* hitMob = NULL;
    float closestDist = 999.0f;

    for (int i = 0; i < MAX_MOBS; i++) {
        if (!g_Mobs[i].active) continue;
        Mob* m = &g_Mobs[i];

        float cx = m->x;
        float cy = m->y + m->height * 0.5f;
        float hx = fabsf(clickWorldX - cx);
        float hy = fabsf(clickWorldY - cy);

        // Click hit tolerance box
        if (hx <= (m->width * 0.5f + 0.40f) && hy <= (m->height * 0.5f + 0.40f)) {
            float d = hx * hx + hy * hy;
            if (d < closestDist) {
                closestDist = d;
                hitMob = m;
            }
        }
    }

    if (hitMob != NULL) {
        // Deal damage & apply red hurt flash
        hitMob->health -= damage;
        hitMob->hurtTimer = 0.35f;

        // Peaceful animals flee when attacked
        if (isPeacefulAnimal(hitMob->type)) {
            hitMob->state = ANIMAL_STATE_FLEE;
            hitMob->stateTimer = 4.0f;
            hitMob->facingRight = (hitMob->x > playerX);
        }

        // Knockback impulse vector away from player scaled by tool knockback multiplier
        float kx = hitMob->x - playerX;
        float ky = hitMob->y - playerY + 0.25f;
        float klen = sqrtf(kx * kx + ky * ky);
        if (klen > 0.001f) {
            kx /= klen;
            ky /= klen;
        } else {
            kx = 1.0f;
            ky = 0.5f;
        }

        if (knockbackMult < 0.2f) knockbackMult = 1.0f;
        hitMob->vx = kx * 7.5f * knockbackMult;
        hitMob->vy = 5.8f * (0.8f + knockbackMult * 0.2f);
        hitMob->isGrounded = false;

        // Hit splatter particles
        float pr = 1.0f, pg = 0.2f, pb = 0.2f;
        if (hitMob->type == MOB_SLIME) {
            pr = 0.25f; pg = 0.95f; pb = 0.35f;
        } else if (hitMob->type == MOB_BAT) {
            pr = 0.65f; pg = 0.25f; pb = 0.75f;
        } else if (hitMob->type == MOB_SHEEP) {
            pr = 0.95f; pg = 0.95f; pb = 0.92f;
        } else if (hitMob->type == MOB_COW) {
            pr = 0.85f; pg = 0.82f; pb = 0.78f;
        } else if (hitMob->type == MOB_PIG) {
            pr = 0.98f; pg = 0.68f; pb = 0.72f;
        } else if (hitMob->type == MOB_RABBIT) {
            pr = 0.85f; pg = 0.75f; pb = 0.64f;
        } else if (hitMob->type == MOB_SKELETON) {
            pr = 0.92f; pg = 0.90f; pb = 0.82f;
        } else if (hitMob->type == MOB_SPIDER) {
            pr = 0.22f; pg = 0.12f; pb = 0.16f;
        } else if (hitMob->type == MOB_GHOST) {
            pr = 0.35f; pg = 0.88f; pb = 1.0f;
        } else if (hitMob->type == MOB_LAVA_SLIME) {
            pr = 1.0f; pg = 0.50f; pb = 0.10f;
        }

        for (int p = 0; p < 8; p++) {
            spawnParticle(
                hitMob->x + ((float)rand() / RAND_MAX - 0.5f) * 0.4f,
                hitMob->y + hitMob->height * 0.5f + ((float)rand() / RAND_MAX - 0.5f) * 0.4f,
                pr, pg, pb,
                kx * 3.0f + ((float)rand() / RAND_MAX - 0.5f) * 4.0f,
                ky * 2.0f + ((float)rand() / RAND_MAX) * 3.5f
            );
        }

        // Damage floating text
        char dmgText[16];
        snprintf(dmgText, sizeof(dmgText), "-%d", (int)damage);
        spawnFloatingText(dmgText, hitMob->x, hitMob->y + hitMob->height + 0.3f, 1.0f, 0.25f, 0.25f);

        // Check Mob Death & Loot Drops
        if (hitMob->health <= 0.0f) {
            playSound(SFX_KILL);
            hitMob->active = false;

            // Death burst particles
            for (int p = 0; p < 16; p++) {
                spawnParticle(
                    hitMob->x,
                    hitMob->y + hitMob->height * 0.5f,
                    pr, pg, pb,
                    ((float)rand() / RAND_MAX - 0.5f) * 6.0f,
                    1.5f + ((float)rand() / RAND_MAX) * 5.0f
                );
            }

            // Authentic Luminora Loot Drops:
            if (hitMob->type == MOB_SLIME) {
                // Slimes drop authentic ITEM_GEL (id 118)
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, ITEM_GEL, 2);
                spawnFloatingText("+2 Slime Gel!", hitMob->x, hitMob->y + 1.2f, 0.25f, 0.95f, 0.45f);
            } else if (hitMob->type == MOB_BAT) {
                // Cave Bat has reduced 12% mineral drop rate
                if ((rand() % 100) < 12) {
                    BlockType dropOre = (rand() % 4 == 0) ? BLOCK_RUBY_ORE : BLOCK_COAL_ORE;
                    spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, dropOre, 1);
                    spawnFloatingText((dropOre == BLOCK_RUBY_ORE) ? "+1 Ruby Gem!" : "+1 Coal Ore",
                                      hitMob->x, hitMob->y + 1.2f, 0.95f, 0.75f, 0.25f);
                }
            } else if (hitMob->type == MOB_ZOMBIE) {
                // Zombie has reduced 12% mineral drop rate
                if ((rand() % 100) < 12) {
                    BlockType dropOre = (rand() % 3 == 0) ? BLOCK_GOLD_ORE : BLOCK_IRON_ORE;
                    spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, dropOre, 1);
                    spawnFloatingText((dropOre == BLOCK_GOLD_ORE) ? "+1 Gold Ore!" : "+1 Iron Ore",
                                      hitMob->x, hitMob->y + 1.2f, 1.0f, 0.85f, 0.2f);
                }
            } else if (hitMob->type == MOB_SHEEP) {
                spawnFloatingText("+1 White Wool", hitMob->x, hitMob->y + 1.2f, 0.95f, 0.95f, 0.95f);
            } else if (hitMob->type == MOB_COW) {
                spawnFloatingText("+1 Raw Beef", hitMob->x, hitMob->y + 1.2f, 0.95f, 0.65f, 0.55f);
            } else if (hitMob->type == MOB_PIG) {
                spawnFloatingText("+1 Raw Porkchop", hitMob->x, hitMob->y + 1.2f, 0.98f, 0.68f, 0.72f);
            } else if (hitMob->type == MOB_RABBIT) {
                spawnFloatingText("+1 Raw Rabbit", hitMob->x, hitMob->y + 1.2f, 0.92f, 0.80f, 0.70f);
            } else if (hitMob->type == MOB_SKELETON) {
                // Skeleton drops bones and sticks
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, ITEM_STICK, 2);
                spawnFloatingText((getLanguage() == LANG_ZH) ? "+骨头与木棍" : "+Bones & Sticks!",
                                  hitMob->x, hitMob->y + 1.2f, 0.92f, 0.90f, 0.82f);
            } else if (hitMob->type == MOB_SPIDER) {
                // Spider drops string/silk
                spawnFloatingText((getLanguage() == LANG_ZH) ? "+蜘蛛丝" : "+Spider Silk!",
                                  hitMob->x, hitMob->y + 1.2f, 0.95f, 0.95f, 0.95f);
            } else if (hitMob->type == MOB_GHOST) {
                // Ghost drops ghostly ectoplasm essence
                spawnFloatingText((getLanguage() == LANG_ZH) ? "+幽灵精魄" : "+Spectral Essence!",
                                  hitMob->x, hitMob->y + 1.2f, 0.35f, 0.88f, 1.0f);
            } else if (hitMob->type == MOB_LAVA_SLIME) {
                // Lava Slime drops magma block and slime gel
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, BLOCK_MAGMA, 1);
                spawnDroppedItem(hitMob->x + 0.2f, hitMob->y + 0.4f, ITEM_GEL, 2);
                spawnFloatingText((getLanguage() == LANG_ZH) ? "+熔岩块与凝胶" : "+Magma Block & Gel!",
                                  hitMob->x, hitMob->y + 1.2f, 1.0f, 0.55f, 0.12f);
            }
        } else {
            playSound(SFX_HIT);
            if (hitMob->type == MOB_SHEEP) {
                playSound(SFX_SHEEP_BAA);
            } else if (hitMob->type == MOB_COW) {
                playSound(SFX_COW_MOO);
            } else if (hitMob->type == MOB_PIG) {
                playSound(SFX_PIG_OINK);
            } else if (hitMob->type == MOB_ZOMBIE) {
                playSound(SFX_ZOMBIE_GROWL);
            } else if (hitMob->type == MOB_SLIME) {
                playSound(SFX_SLIME_JUMP);
            } else if (hitMob->type == MOB_BAT) {
                playSound(SFX_BAT_CHIRP);
            } else if (hitMob->type == MOB_SKELETON) {
                playSound(SFX_SKELETON_RATTLE);
            } else if (hitMob->type == MOB_SPIDER) {
                playSound(SFX_SPIDER_HISS);
            } else if (hitMob->type == MOB_LAVA_SLIME) {
                playSound(SFX_SLIME_JUMP);
            }
        }

        return true;
    }

    return false;
}

bool checkMobAttack(float clickWorldX, float clickWorldY, float damage, float playerX, float playerY) {
    return checkMobAttackEx(clickWorldX, clickWorldY, damage, 1.0f, playerX, playerY);
}

#endif // MOBS_H
