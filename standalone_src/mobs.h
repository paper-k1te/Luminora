#ifndef MOBS_H
#define MOBS_H

#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "sound_engine.h"

// ============================================================================
// Mob System Configuration & Enumerations
// ============================================================================
#define MAX_MOBS            32
#define TARGET_MOB_COUNT    14
#define MOB_DESPAWN_DIST    48.0f
#define MOB_MIN_SPAWN_DIST  16.0f
#define MOB_MAX_SPAWN_DIST  34.0f

typedef enum {
    MOB_SLIME = 0,
    MOB_BAT,
    MOB_ZOMBIE,
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
    float animTimer;    // Drives squash-and-stretch, wing flap, and walking swagger
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

// ============================================================================
// Function Declarations
// ============================================================================
void onPlayerHurt(float damage, float sourceX);
void initMobs(void);
void spawnMob(float x, float y, int type);
void updateMobs(float dt, float playerX, float playerY);
void renderMobs(float daylight, float camX, float camY);
bool checkMobAttack(float clickWorldX, float clickWorldY, float damage, float playerX, float playerY);
bool checkMobAttackEx(float clickWorldX, float clickWorldY, float damage, float knockbackMult, float playerX, float playerY);

// ============================================================================
// Implementation
// ============================================================================

void initMobs(void) {
    memset(g_Mobs, 0, sizeof(g_Mobs));
    g_MobSpawnTimer = 0.0f;
}

void spawnMob(float x, float y, int type) {
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
                default:
                    m->width = 0.78f;
                    m->height = 1.82f;
                    m->health = m->maxHealth = 45.0f;
                    m->state = ZOMBIE_STATE_WANDER;
                    m->animTimer = ((float)rand() / RAND_MAX) * 6.28f;
                    break;
            }
            return;
        }
    }
}

void updateMobs(float dt, float playerX, float playerY) {
    // ------------------------------------------------------------------------
    // 1. Mob Spawning & Despawning Management (Maintains ~10-16 mobs)
    // ------------------------------------------------------------------------
    int activeCount = 0;
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
        }
    }

    g_MobSpawnTimer += dt;
    if (g_MobSpawnTimer >= 1.2f) {
        g_MobSpawnTimer = 0.0f;

        if (activeCount < TARGET_MOB_COUNT) {
            float spawnOffset = (MOB_MIN_SPAWN_DIST + ((float)rand() / RAND_MAX) * (MOB_MAX_SPAWN_DIST - MOB_MIN_SPAWN_DIST));
            if (rand() % 2 == 0) spawnOffset = -spawnOffset;

            int targetX = (int)floorf(playerX + spawnOffset);
            if (targetX >= 5 && targetX < WORLD_WIDTH - 5) {
                // Determine spawning type based on biome, depth, and daylight
                int chosenType = MOB_SLIME;

                // Check depth zone
                if (playerY < CAVERN_LEVEL) {
                    // Deep Caverns & Underworld
                    if (playerY < UNDERWORLD_LEVEL) {
                        // Deep Underworld: Tough Zombies and Underworld Bats
                        chosenType = (rand() % 10 < 6) ? MOB_ZOMBIE : MOB_BAT;
                    } else {
                        // Caverns: Cave Bats thrive in the dark, along with Zombies
                        chosenType = (rand() % 10 < 6) ? MOB_BAT : MOB_ZOMBIE;
                    }

                    // Find open cave cavity around player height
                    int searchY = (int)floorf(playerY + ((rand() % 14) - 7));
                    if (searchY < 3) searchY = 3;
                    if (searchY >= CAVERN_LEVEL) searchY = CAVERN_LEVEL - 1;

                    if (chosenType == MOB_BAT) {
                        // Bats spawn in open air pockets
                        if (!isSolidBlock(targetX, searchY) && !isSolidBlock(targetX, searchY + 1)) {
                            spawnMob((float)targetX + 0.5f, (float)searchY + 0.5f, MOB_BAT);
                        }
                    } else {
                        // Zombies need solid footing
                        for (int testY = searchY; testY >= 3; testY--) {
                            if (isSolidBlock(targetX, testY) && !isSolidBlock(targetX, testY + 1) && !isSolidBlock(targetX, testY + 2)) {
                                spawnMob((float)targetX + 0.5f, (float)(testY + 1), MOB_ZOMBIE);
                                break;
                            }
                        }
                    }
                } else {
                    // Surface Ground Zone
                    int surfY = getSurfaceHeight(targetX, 1337);
                    float sunAngle = g_WorldTime * 6.2831853f;
                    float daylight = sinf(sunAngle);

                    if (daylight < 0.35f) {
                        // Night surface: Zombies crawl out, accompanied by nighttime slimes
                        chosenType = (rand() % 10 < 7) ? MOB_ZOMBIE : MOB_SLIME;
                    } else {
                        // Verdant Forest / Daytime: Lively Green Slimes
                        chosenType = MOB_SLIME;
                    }

                    if (surfY > 0 && surfY < WORLD_HEIGHT - 3) {
                        spawnMob((float)targetX + 0.5f, (float)(surfY + 1), chosenType);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. Individual Mob Behaviors, State Machines, & Physics
    // ------------------------------------------------------------------------
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!g_Mobs[i].active) continue;
        Mob* m = &g_Mobs[i];

        if (m->hurtTimer > 0.0f) {
            m->hurtTimer -= dt;
            if (m->hurtTimer < 0.0f) m->hurtTimer = 0.0f;
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

                // Prepare and leap into the air
                if (m->animTimer <= 0.0f && m->isGrounded) {
                    m->state = SLIME_STATE_JUMP;
                    m->animTimer = 0.0f;
                    m->isGrounded = false;

                    float leapDir = (distToPlayer < 18.0f) ? ((dxToPlayer > 0.0f) ? 1.0f : -1.0f)
                                                           : ((rand() % 2 == 0) ? 1.0f : -1.0f);
                    m->facingRight = (leapDir > 0.0f);
                    m->vx = leapDir * (3.8f + ((float)rand() / RAND_MAX) * 1.6f);
                    m->vy = 9.8f + ((float)rand() / RAND_MAX) * 2.2f;
                }
            } else if (m->state == SLIME_STATE_JUMP) {
                m->animTimer += dt;
                // Landing detected by ground collision resolution below
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

            if (distToPlayer < 16.0f) {
                // Swoop pursuit toward player
                m->state = BAT_STATE_SWOOP;
                float targetVX = (dxToPlayer > 0.0f ? 1.0f : -1.0f) * 4.5f;
                float targetVY = (dyToPlayer / (distToPlayer + 0.001f)) * 3.8f + sinf(m->animTimer * 5.0f) * 1.5f;
                m->vx += (targetVX - m->vx) * 3.5f * dt;
                m->vy += (targetVY - m->vy) * 3.5f * dt;
                m->facingRight = (dxToPlayer > 0.0f);
            } else {
                // Smooth sine-wave cruising / glide
                m->state = BAT_STATE_WANDER;
                float targetVX = (m->facingRight ? 2.6f : -2.6f);
                float targetVY = sinf(m->animTimer * 2.8f) * 2.0f;
                m->vx += (targetVX - m->vx) * 2.2f * dt;
                m->vy += (targetVY - m->vy) * 2.2f * dt;

                // Turn around if bumping into cavern walls
                int checkX = (int)floorf(m->x + (m->facingRight ? 0.8f : -0.8f));
                int checkY = (int)floorf(m->y);
                if (isSolidBlock(checkX, checkY)) {
                    m->facingRight = !m->facingRight;
                }
            }

            // Bat soft collision with solid walls
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
            continue; // Bats skip grounded block collision
        }
        // --------------------------------------------------------------------
        // ZOMBIE: Bipedal walker, 1-block auto step climbing, player pursuit
        // --------------------------------------------------------------------
        else if (m->type == MOB_ZOMBIE) {
            m->vy -= 30.0f * dt; // Gravity
            m->animTimer += dt;

            float targetVX = 0.0f;
            if (distToPlayer < 22.0f) {
                // Relentless pursuit toward player
                m->state = ZOMBIE_STATE_CHASE;
                float dir = (dxToPlayer > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (dir > 0.0f);
                targetVX = dir * 2.4f;
            } else {
                // Idle wander
                m->state = ZOMBIE_STATE_WANDER;
                float wanderDir = (sinf(m->animTimer * 0.4f) > 0.0f) ? 1.0f : -1.0f;
                m->facingRight = (wanderDir > 0.0f);
                targetVX = wanderDir * 1.3f;
            }

            float accel = m->isGrounded ? 32.0f : 16.0f;
            m->vx += (targetVX - m->vx) * accel * dt;

            // Terraria 1-Block Auto Step Climbing for Zombie
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
        // Grounded Collision against g_World (Slimes & Zombies)
        // --------------------------------------------------------------------
        float nextX = m->x + m->vx * dt;
        int minX = (int)floorf(nextX - m->width * 0.5f);
        int maxX = (int)floorf(nextX + m->width * 0.5f);
        int minY = (int)floorf(m->y);
        int maxY = (int)floorf(m->y + m->height - 0.08f);

        // Horizontal collision resolution
        for (int y = minY; y <= maxY; y++) {
            if (m->vx > 0.0f && isSolidBlock(maxX, y)) {
                nextX = (float)maxX - m->width * 0.5f - 0.001f;
                m->vx = 0.0f;
                break;
            } else if (m->vx < 0.0f && isSolidBlock(minX, y)) {
                nextX = (float)(minX + 1) + m->width * 0.5f + 0.001f;
                m->vx = 0.0f;
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

                    // Slime impact squash transition
                    if (m->type == MOB_SLIME && !wasGrounded) {
                        m->state = SLIME_STATE_LAND;
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

        // Player Contact Combat Collision (Survival Mode Damage & Knockback)
        float pLeft = playerX - 0.38f;
        float pRight = playerX + 0.38f;
        float pBottom = playerY;
        float pTop = playerY + 1.8f;

        float mLeft = m->x - m->width * 0.45f;
        float mRight = m->x + m->width * 0.45f;
        float mBottom = m->y;
        float mTop = m->y + m->height;

        if (pRight > mLeft && pLeft < mRight && pTop > mBottom && pBottom < mTop) {
            float mobDmg = 12.0f;
            if (m->type == MOB_SLIME) mobDmg = 10.0f;
            else if (m->type == MOB_BAT) mobDmg = 8.0f;
            else if (m->type == MOB_ZOMBIE) mobDmg = 18.0f;
            onPlayerHurt(mobDmg, m->x);
        }
    }
}

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
                    // Pre-jump anticipation squash
                    float t = 1.0f - (m->animTimer / 0.22f);
                    sx = 1.0f + 0.32f * t;
                    sy = 1.0f - 0.26f * t;
                } else {
                    // Gentle breathing idle
                    float b = sinf(m->animTimer * 5.0f);
                    sx = 1.0f + b * 0.05f;
                    sy = 1.0f - b * 0.05f;
                }
            } else if (m->state == SLIME_STATE_JUMP) {
                // Airborne vertical elongation stretch
                sx = 0.82f;
                sy = 1.25f;
            } else if (m->state == SLIME_STATE_LAND) {
                // Ground impact horizontal squash
                float t = m->animTimer / 0.20f;
                sx = 1.0f + 0.38f * t;
                sy = 1.0f - 0.30f * t;
            }

            glScalef(sx, sy, 1.0f);

            // Outer Gelatinous Translucent Body
            float r = (1.0f - hurtFlash) * (0.24f * depthLight) + hurtFlash * 1.0f;
            float g = (1.0f - hurtFlash) * (0.92f * depthLight) + hurtFlash * 0.25f;
            float b = (1.0f - hurtFlash) * (0.35f * depthLight) + hurtFlash * 0.25f;
            glColor4f(r, g, b, 0.78f);

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
            float cr = (1.0f - hurtFlash) * (0.12f * depthLight) + hurtFlash * 0.9f;
            float cg = (1.0f - hurtFlash) * (0.55f * depthLight) + hurtFlash * 0.2f;
            float cb = (1.0f - hurtFlash) * (0.20f * depthLight) + hurtFlash * 0.2f;
            glColor4f(cr, cg, cb, 0.92f);

            float coreBob = sinf(m->animTimer * 6.0f) * 0.03f;
            glBegin(GL_QUADS);
            glVertex3f(-0.16f, h * 0.28f + coreBob, 0.01f);
            glVertex3f( 0.16f, h * 0.28f + coreBob, 0.01f);
            glVertex3f( 0.16f, h * 0.60f + coreBob, 0.01f);
            glVertex3f(-0.16f, h * 0.60f + coreBob, 0.01f);
            glEnd();

            // Cute Eyes (dark oval pupils with specular highlight)
            glColor3f(0.08f, 0.12f, 0.08f);
            glBegin(GL_QUADS);
            // Left eye
            glVertex3f(0.05f, h * 0.45f, 0.02f);
            glVertex3f(0.14f, h * 0.45f, 0.02f);
            glVertex3f(0.14f, h * 0.62f, 0.02f);
            glVertex3f(0.05f, h * 0.62f, 0.02f);
            // Right eye
            glVertex3f(0.20f, h * 0.45f, 0.02f);
            glVertex3f(0.29f, h * 0.45f, 0.02f);
            glVertex3f(0.29f, h * 0.62f, 0.02f);
            glVertex3f(0.20f, h * 0.62f, 0.02f);
            glEnd();

            // White Specular Dots
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
            float flapAngle = sinf(m->animTimer * 18.0f) * 44.0f; // Rapid wing flap

            float br = (1.0f - hurtFlash) * (0.28f * depthLight) + hurtFlash * 1.0f;
            float bg = (1.0f - hurtFlash) * (0.24f * depthLight) + hurtFlash * 0.25f;
            float bb = (1.0f - hurtFlash) * (0.34f * depthLight) + hurtFlash * 0.25f;

            // Bat Central Body & Head
            glColor3f(br, bg, bb);
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

            // Articulated Left Flapping Wing
            float wr = (1.0f - hurtFlash) * (0.38f * depthLight) + hurtFlash * 0.9f;
            float wg = (1.0f - hurtFlash) * (0.28f * depthLight) + hurtFlash * 0.2f;
            float wb = (1.0f - hurtFlash) * (0.44f * depthLight) + hurtFlash * 0.2f;
            glColor4f(wr, wg, wb, 0.88f);

            glPushMatrix();
            glTranslatef(-0.14f, 0.30f, 0.01f);
            glRotatef(flapAngle, 0.0f, 0.0f, 1.0f);

            glBegin(GL_TRIANGLES);
            // Wing root membrane
            glVertex3f( 0.00f,  0.00f, 0.0f);
            glVertex3f(-0.35f,  0.15f, 0.0f);
            glVertex3f(-0.18f, -0.22f, 0.0f);

            // Wing tip second articulation segment
            glVertex3f(-0.35f,  0.15f, 0.0f);
            glVertex3f(-0.62f,  0.08f, 0.0f);
            glVertex3f(-0.38f, -0.15f, 0.0f);
            glEnd();
            glPopMatrix();

            // Articulated Right Flapping Wing
            glPushMatrix();
            glTranslatef(0.14f, 0.30f, 0.01f);
            glRotatef(-flapAngle, 0.0f, 0.0f, 1.0f);

            glBegin(GL_TRIANGLES);
            // Wing root membrane
            glVertex3f(0.00f,  0.00f, 0.0f);
            glVertex3f(0.35f,  0.15f, 0.0f);
            glVertex3f(0.18f, -0.22f, 0.0f);

            // Wing tip second articulation segment
            glVertex3f(0.35f,  0.15f, 0.0f);
            glVertex3f(0.62f,  0.08f, 0.0f);
            glVertex3f(0.38f, -0.15f, 0.0f);
            glEnd();
            glPopMatrix();
        }
        // ====================================================================
        // 3. ZOMBIE: Green Skin + Ragged Blue Shirt + Outstretched Horizontal Arms
        // ====================================================================
        else if (m->type == MOB_ZOMBIE) {
            float sinWalk = sinf(m->animTimer * 6.5f);
            float legAngleL = m->isGrounded ? sinWalk * 0.18f : 0.12f;
            float legAngleR = m->isGrounded ? -sinWalk * 0.18f : -0.12f;

            // Green Rotten Skin & Ragged Cloth Colors
            float sr = (1.0f - hurtFlash) * (0.34f * depthLight) + hurtFlash * 1.0f;
            float sg = (1.0f - hurtFlash) * (0.64f * depthLight) + hurtFlash * 0.25f;
            float sb = (1.0f - hurtFlash) * (0.36f * depthLight) + hurtFlash * 0.25f;

            float cr = (1.0f - hurtFlash) * (0.22f * depthLight) + hurtFlash * 0.85f;
            float cg = (1.0f - hurtFlash) * (0.46f * depthLight) + hurtFlash * 0.25f;
            float cb = (1.0f - hurtFlash) * (0.62f * depthLight) + hurtFlash * 0.25f;

            // Ragged Dark Cyan/Blue Shirt Torso
            glColor3f(cr, cg, cb);
            glBegin(GL_QUADS);
            glVertex3f(-0.30f, 0.48f, 0.0f);
            glVertex3f( 0.30f, 0.48f, 0.0f);
            glVertex3f( 0.30f, 1.28f, 0.0f);
            glVertex3f(-0.30f, 1.28f, 0.0f);
            glEnd();

            // Ragged Tattered Edge exposing green belly
            glColor3f(sr, sg, sb);
            glBegin(GL_QUADS);
            glVertex3f(-0.24f, 0.45f, 0.01f);
            glVertex3f( 0.12f, 0.45f, 0.01f);
            glVertex3f( 0.12f, 0.54f, 0.01f);
            glVertex3f(-0.24f, 0.54f, 0.01f);
            glEnd();

            // Zombie Head
            glColor3f(sr, sg, sb);
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
            glColor3f(cr * 0.7f, cg * 0.7f, cb * 0.7f);
            glBegin(GL_QUADS);
            // Left leg
            glVertex3f(-0.22f + legAngleL, 0.0f,  0.0f);
            glVertex3f(-0.06f + legAngleL, 0.0f,  0.0f);
            glVertex3f(-0.06f,             0.48f, 0.0f);
            glVertex3f(-0.22f,             0.48f, 0.0f);
            // Right leg
            glVertex3f( 0.06f + legAngleR, 0.0f,  0.0f);
            glVertex3f( 0.22f + legAngleR, 0.0f,  0.0f);
            glVertex3f( 0.22f,             0.48f, 0.0f);
            glVertex3f( 0.06f,             0.48f, 0.0f);
            glEnd();

            // Horizontal Outstretched Arms (Terraria / Minecraft Zombie classic pose!)
            float armLurch = sinf(m->animTimer * 4.0f) * 0.04f;
            // Sleeve
            glColor3f(cr, cg, cb);
            glBegin(GL_QUADS);
            glVertex3f(0.15f, 0.95f + armLurch, 0.02f);
            glVertex3f(0.35f, 0.95f + armLurch, 0.02f);
            glVertex3f(0.35f, 1.15f + armLurch, 0.02f);
            glVertex3f(0.15f, 1.15f + armLurch, 0.02f);
            glEnd();

            // Green Rotting Forearms reaching straight forward
            glColor3f(sr, sg, sb);
            glBegin(GL_QUADS);
            glVertex3f(0.35f, 0.98f + armLurch, 0.02f);
            glVertex3f(0.70f, 0.98f + armLurch, 0.02f);
            glVertex3f(0.70f, 1.12f + armLurch, 0.02f);
            glVertex3f(0.35f, 1.12f + armLurch, 0.02f);
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

        // Spawn vibrant hit splatter particles
        float pr = 1.0f, pg = 0.2f, pb = 0.2f;
        if (hitMob->type == MOB_SLIME) {
            pr = 0.25f; pg = 0.95f; pb = 0.35f;
        } else if (hitMob->type == MOB_BAT) {
            pr = 0.65f; pg = 0.25f; pb = 0.75f;
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

            // Authentic Terraria Loot Drops:
            if (hitMob->type == MOB_SLIME) {
                // Slime drops authentic Gel (represented by Glow Shroom)
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, BLOCK_GLOW_SHROOM, 2);
                spawnFloatingText("+2 Slime Gel!", hitMob->x, hitMob->y + 1.2f, 0.3f, 0.95f, 0.4f);
            } else if (hitMob->type == MOB_BAT) {
                // Cave Bat drops Coal Ore or Ruby
                BlockType dropOre = (rand() % 4 == 0) ? BLOCK_RUBY_ORE : BLOCK_COAL_ORE;
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, dropOre, 1);
                spawnFloatingText((dropOre == BLOCK_RUBY_ORE) ? "+1 Ruby Gem!" : "+1 Coal Ore",
                                  hitMob->x, hitMob->y + 1.2f, 0.95f, 0.75f, 0.25f);
            } else if (hitMob->type == MOB_ZOMBIE) {
                // Zombie drops Iron Ore or Gold Nugget
                BlockType dropOre = (rand() % 3 == 0) ? BLOCK_GOLD_ORE : BLOCK_IRON_ORE;
                spawnDroppedItem(hitMob->x, hitMob->y + 0.4f, dropOre, 1);
                spawnFloatingText((dropOre == BLOCK_GOLD_ORE) ? "+1 Gold Ore!" : "+1 Iron Ore",
                                  hitMob->x, hitMob->y + 1.2f, 1.0f, 0.85f, 0.2f);
            }
        } else {
            playSound(SFX_HIT);
        }

        return true;
    }

    return false;
}

bool checkMobAttack(float clickWorldX, float clickWorldY, float damage, float playerX, float playerY) {
    return checkMobAttackEx(clickWorldX, clickWorldY, damage, 1.0f, playerX, playerY);
}

#endif // MOBS_H
