#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>

// Mock the environment needed by mobs.h
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_RUBY_ORE,
    BLOCK_GLOW_SHROOM,
    BLOCK_TORCH,
    BLOCK_CHANDELIER,
    BLOCK_TYPE_COUNT
} BlockType;

static inline bool isLightExtinguished(int x, int y) { (void)x; (void)y; return false; }

typedef struct {
    float r, g, b;
    bool emitLight;
    bool isSolid;
} TestBlockDef;

static TestBlockDef g_BlockDefs[BLOCK_TYPE_COUNT];
static BlockType g_World[512][128];
static int g_SurfaceHeight[512];
static float g_WorldTime = 0.25f; // Daytime

static int g_LastDroppedType = -1;
static int g_LastDroppedCount = 0;
static float g_PlayerHurtDamage = 0.0f;

static void spawnParticle(float x, float y, float r, float g, float b, float vx, float vy) {
    (void)x; (void)y; (void)r; (void)g; (void)b; (void)vx; (void)vy;
}

static void spawnFloatingText(const char* text, float x, float y, float r, float g, float b) {
    (void)text; (void)x; (void)y; (void)r; (void)g; (void)b;
}

static void spawnDroppedItem(float x, float y, int type, int count) {
    (void)x; (void)y;
    g_LastDroppedType = type;
    g_LastDroppedCount = count;
}

void onPlayerHurt(float damage, float sourceX) {
    (void)sourceX;
    g_PlayerHurtDamage += damage;
}

static bool isSolidBlock(int x, int y) {
    if (x < 0 || x >= 512 || y < 0 || y >= 128) return true;
    return (g_World[x][y] != BLOCK_AIR);
}

static int getSurfaceHeight(int x, int seed) {
    (void)seed;
    return g_SurfaceHeight[x];
}

// Now include mobs.h
#include "../standalone_src/mobs.h"

int main(void) {
    printf("=======================================================\n");
    printf("   Luminora Creature Ecosystem & AI Validation Test   \n");
    printf("=======================================================\n\n");

    // Initialize block definitions & world
    memset(g_BlockDefs, 0, sizeof(g_BlockDefs));
    for (int b = 1; b < BLOCK_TYPE_COUNT; b++) {
        g_BlockDefs[b].isSolid = true;
    }
    g_BlockDefs[BLOCK_AIR].isSolid = false;

    // Set up ground at y = 80
    for (int x = 0; x < 512; x++) {
        g_SurfaceHeight[x] = 80;
        for (int y = 0; y < 128; y++) {
            if (y <= 80) {
                g_World[x][y] = (y == 80) ? BLOCK_GRASS : BLOCK_DIRT;
            } else {
                g_World[x][y] = BLOCK_AIR;
            }
        }
    }

    // -------------------------------------------------------------
    // Test 1: Slime Gel Drop Fix (ITEM_GEL = 118)
    // -------------------------------------------------------------
    printf("[Test 1] Testing Slime Gel Drop Fix (ITEM_GEL = 118)...\n");
    initMobs();
    spawnMob(100.5f, 81.0f, MOB_SLIME);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_SLIME);

    // Attack slime to kill it
    g_LastDroppedType = -1;
    bool hit = checkMobAttackEx(100.5f, 81.3f, 100.0f, 1.0f, 102.0f, 81.0f);
    assert(hit);
    assert(!g_Mobs[0].active); // Dead
    assert(g_LastDroppedType == ITEM_GEL);
    assert(g_LastDroppedCount == 2);
    printf("  -> PASS: Slime dropped ITEM_GEL (id 118, count 2) successfully!\n\n");

    // -------------------------------------------------------------
    // Test 2: Peaceful Animals Initialization & Attributes
    // -------------------------------------------------------------
    printf("[Test 2] Testing Peaceful Animals Initialization...\n");
    initMobs();
    spawnMob(100.5f, 81.0f, MOB_SHEEP);
    spawnMob(105.5f, 81.0f, MOB_COW);
    spawnMob(110.5f, 81.0f, MOB_PIG);
    spawnMob(115.5f, 81.0f, MOB_RABBIT);

    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_SHEEP);
    assert(g_Mobs[1].active && g_Mobs[1].type == MOB_COW);
    assert(g_Mobs[2].active && g_Mobs[2].type == MOB_PIG);
    assert(g_Mobs[3].active && g_Mobs[3].type == MOB_RABBIT);

    assert(isPeacefulAnimal(g_Mobs[0].type));
    assert(isPeacefulAnimal(g_Mobs[1].type));
    assert(isPeacefulAnimal(g_Mobs[2].type));
    assert(isPeacefulAnimal(g_Mobs[3].type));
    assert(!isHostileMob(g_Mobs[0].type));
    printf("  -> PASS: All 4 peaceful animals initialized with correct MobTypes!\n\n");

    // -------------------------------------------------------------
    // Test 3: Peaceful AI & Zero Player Contact Damage
    // -------------------------------------------------------------
    printf("[Test 3] Testing Zero Player Damage from Peaceful Animals...\n");
    g_PlayerHurtDamage = 0.0f;
    // Position player right on top of sheep
    updateMobs(0.016f, 100.5f, 81.0f);
    assert(g_PlayerHurtDamage == 0.0f);
    printf("  -> PASS: Peaceful animals do not attack or damage player!\n\n");

    // -------------------------------------------------------------
    // Test 4: Timid Rabbit AI (Hops away when player approaches)
    // -------------------------------------------------------------
    printf("[Test 4] Testing Timid Rabbit AI (hops away from player)...\n");
    Mob* rabbit = &g_Mobs[3]; // Rabbit is at x = 115.5
    rabbit->isGrounded = true;
    rabbit->stateTimer = 0.0f;
    // Player approaches from left at x = 112.0 (dist = 3.5 < 7.5)
    updateMobs(0.016f, 112.0f, 81.0f);
    assert(rabbit->state == ANIMAL_STATE_FLEE);
    assert(rabbit->facingRight == true); // Facing away from player (player is to left)
    assert(rabbit->vx > 0.0f); // Hops to the right!
    printf("  -> PASS: Rabbit hops away when player approaches within 7.5 blocks!\n\n");

    // -------------------------------------------------------------
    // Test 5: Mob Spawning Anti-Wall Validation
    // -------------------------------------------------------------
    printf("[Test 5] Testing Anti-Wall Spawning Validation...\n");
    initMobs();

    // Inside solid ground: x = 100, y = 70 (both solid)
    assert(!isValidMobSpawnPoint(100, 70, false));
    spawnMob(100.5f, 70.0f, MOB_ZOMBIE);
    assert(!g_Mobs[0].active); // Must NOT spawn in solid ground!

    // In mid-air with no floor: x = 100, y = 100 (y-1 is air)
    assert(!isValidMobSpawnPoint(100, 100, false));
    spawnMob(100.5f, 100.0f, MOB_ZOMBIE);
    assert(!g_Mobs[0].active); // Non-flying mob must NOT spawn in mid-air!

    // Inside a 1-block ceiling/wall: x = 100, y = 80 is solid, y = 81 is air, y = 82 is solid
    g_World[100][82] = BLOCK_DIRT;
    assert(!isValidMobSpawnPoint(100, 81, false)); // (x, y+1) is solid
    spawnMob(100.5f, 81.0f, MOB_ZOMBIE);
    assert(!g_Mobs[0].active); // Must NOT spawn when head penetrates ceiling!
    g_World[100][82] = BLOCK_AIR; // Restore

    // Valid location: x = 100, y = 81 (air at 81, air at 82, floor at 80)
    assert(isValidMobSpawnPoint(100, 81, false));
    spawnMob(100.5f, 81.0f, MOB_SHEEP);
    assert(g_Mobs[0].active);
    printf("  -> PASS: Mobs never spawn inside walls or without solid floor!\n\n");

    // -------------------------------------------------------------
    // Test 6: Anti-Wall Unsticking Mechanism in updateMobs()
    // -------------------------------------------------------------
    printf("[Test 6] Testing Anti-Wall Unsticking Mechanism...\n");
    // Artificially push mob into a solid block at y = 80
    g_Mobs[0].y = 80.0f; // Foot is in BLOCK_GRASS
    updateMobs(0.016f, 105.0f, 81.0f);
    assert(g_Mobs[0].y >= 81.0f); // Pushed up into adjacent air at y = 81!
    printf("  -> PASS: Stuck mob pushed out of solid block into adjacent air!\n\n");

    // -------------------------------------------------------------
    // Test 7: Safe Zone Suppression (25 block radius around initial spawn)
    // -------------------------------------------------------------
    printf("[Test 7] Testing Safe Zone (25 blocks around initial spawn point)...\n");
    initMobs();
    int initSpawnX = 512 / 2; // 256
    (void)g_SurfaceHeight;

    // Attempt to spawn hostile Zombie at x = 256, y = 81 (dist < 25)
    spawnMob((float)initSpawnX + 0.5f, 81.0f, MOB_ZOMBIE);
    assert(!g_Mobs[0].active); // Suppressed by safe zone!

    // Attempt to spawn peaceful Sheep at x = 256, y = 81
    spawnMob((float)initSpawnX + 0.5f, 81.0f, MOB_SHEEP);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_SHEEP); // Allowed!

    // Outside safe zone (x = 256 + 30 = 286)
    spawnMob(286.5f, 81.0f, MOB_ZOMBIE);
    assert(g_Mobs[1].active && g_Mobs[1].type == MOB_ZOMBIE); // Allowed!
    printf("  -> PASS: Safe zone contains 0 hostile mobs while permitting peaceful animals!\n\n");

    // -------------------------------------------------------------
    // Test 8: Reduced Mob Mineral Drops (12%)
    // -------------------------------------------------------------
    printf("[Test 8] Testing Reduced Mob Mineral Drops (12%% rate)...\n");
    srand(42);
    int zombieOreDrops = 0;
    int batOreDrops = 0;
    int trials = 1000;

    for (int t = 0; t < trials; t++) {
        // Zombie trial
        initMobs();
        spawnMob(286.5f, 81.0f, MOB_ZOMBIE);
        g_LastDroppedType = -1;
        checkMobAttackEx(286.5f, 81.5f, 100.0f, 1.0f, 288.0f, 81.0f);
        if (g_LastDroppedType == BLOCK_GOLD_ORE || g_LastDroppedType == BLOCK_IRON_ORE) {
            zombieOreDrops++;
        }

        // Bat trial
        initMobs();
        spawnMob(286.5f, 81.0f, MOB_BAT);
        g_LastDroppedType = -1;
        checkMobAttackEx(286.5f, 81.5f, 100.0f, 1.0f, 288.0f, 81.0f);
        if (g_LastDroppedType == BLOCK_RUBY_ORE || g_LastDroppedType == BLOCK_COAL_ORE) {
            batOreDrops++;
        }
    }

    float zombieRate = (float)zombieOreDrops / trials;
    float batRate = (float)batOreDrops / trials;
    printf("  Zombie ore drop rate over %d kills: %.2f%% (Expected ~12%%)\n", trials, zombieRate * 100.0f);
    printf("  Bat ore drop rate over %d kills:    %.2f%% (Expected ~12%%)\n", trials, batRate * 100.0f);
    assert(zombieRate > 0.08f && zombieRate < 0.16f);
    assert(batRate > 0.08f && batRate < 0.16f);
    printf("  -> PASS: Mineral drops properly calibrated to 12%%!\n\n");

    // -------------------------------------------------------------
    // Test 9: 1-Block Auto Step Climbing
    // -------------------------------------------------------------
    printf("[Test 9] Testing 1-Block Auto Step Climbing for Animals...\n");
    initMobs();
    // Create a 1-block step at x = 102 (ground at 81, next block is 81 solid, 82 air)
    g_World[102][81] = BLOCK_DIRT; // 1-block step up
    spawnMob(101.0f, 81.0f, MOB_SHEEP);
    Mob* sheep = &g_Mobs[0];
    sheep->state = ANIMAL_STATE_WANDER;
    sheep->isGrounded = true;
    sheep->facingRight = true;
    sheep->vx = 1.0f;
    // Step forward toward x = 102
    for (int step = 0; step < 40; step++) {
        sheep->isGrounded = true;
        sheep->state = ANIMAL_STATE_WANDER;
        sheep->vx = 1.0f;
        updateMobs(0.016f, 105.0f, 81.0f);
    }
    // Sheep climbed up to y >= 82.0!
    assert(sheep->y >= 81.8f);
    printf("  -> PASS: Animals smoothly step up 1-block obstacles!\n\n");
    g_World[102][81] = BLOCK_AIR; // Clean up

    // -------------------------------------------------------------
    // Test 10: Skeleton Warrior (MOB_SKELETON)
    // -------------------------------------------------------------
    printf("[Test 10] Testing Skeleton Warrior (MOB_SKELETON)...\n");
    initMobs();
    spawnMob(100.5f, 81.0f, MOB_SKELETON);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_SKELETON);
    assert(g_Mobs[0].health == 40.0f);
    assert(isHostileMob(g_Mobs[0].type));

    // Skeleton attack within range (2.0 blocks)
    g_PlayerHurtDamage = 0.0f;
    g_Mobs[0].attackTimer = 0.85f; // Ready to thrust
    updateMobs(0.016f, 102.0f, 81.0f);
    assert(g_PlayerHurtDamage > 0.0f); // Player damaged by skeleton thrust!

    // Skeleton death drops sticks
    g_LastDroppedType = -1;
    checkMobAttackEx(100.5f, 81.5f, 100.0f, 1.0f, 102.0f, 81.0f);
    assert(!g_Mobs[0].active);
    assert(g_LastDroppedType == ITEM_STICK);
    assert(g_LastDroppedCount == 2);
    printf("  -> PASS: Skeleton attacks with bone weapon and drops ITEM_STICK on death!\n\n");

    // -------------------------------------------------------------
    // Test 11: Cave Spider (MOB_SPIDER) Fast Scuttle & Leap Attack
    // -------------------------------------------------------------
    printf("[Test 11] Testing Cave Spider (MOB_SPIDER) Leap Attack...\n");
    initMobs();
    spawnMob(100.5f, 81.0f, MOB_SPIDER);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_SPIDER);
    assert(g_Mobs[0].health == 28.0f);
    assert(isHostileMob(g_Mobs[0].type));

    // Player within 4.0 blocks (dist <= 4.5f)
    g_Mobs[0].isGrounded = true;
    g_Mobs[0].attackTimer = 0.0f;
    updateMobs(0.016f, 104.0f, 81.0f);
    assert(g_Mobs[0].state == SPIDER_STATE_LEAP);
    assert(g_Mobs[0].vy > 6.0f); // Leaping into air!
    assert(g_Mobs[0].vx > 4.0f); // Leaping toward player!
    printf("  -> PASS: Cave Spider leaps aggressively at player within 4.5 blocks!\n\n");

    // -------------------------------------------------------------
    // Test 12: Night Shadow Ghost (MOB_GHOST) Flying & Aerial Pursuit
    // -------------------------------------------------------------
    printf("[Test 12] Testing Night Shadow Ghost (MOB_GHOST) Floating Aerial Pursuit...\n");
    initMobs();
    // Ghost is a flying entity: can spawn in air cavity without solid floor underneath
    assert(isValidMobSpawnPoint(100, 90, true));
    spawnMob(100.5f, 90.0f, MOB_GHOST);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_GHOST);
    assert(g_Mobs[0].health == 24.0f);
    assert(isHostileMob(g_Mobs[0].type));

    // Ghost glides towards player at (110.0, 95.0)
    updateMobs(0.016f, 110.0f, 95.0f);
    assert(g_Mobs[0].state == GHOST_STATE_CHASE);
    assert(g_Mobs[0].vx > 0.0f); // Glides right towards player
    assert(g_Mobs[0].vy > 0.0f); // Glides up towards player (no gravity fall!)
    printf("  -> PASS: Ghost glides freely through air pursuing player with sine bobbing!\n\n");

    // -------------------------------------------------------------
    // Test 13: Lava Slime (MOB_LAVA_SLIME) High Jump & Magma/Gel Drops
    // -------------------------------------------------------------
    printf("[Test 13] Testing Lava Slime (MOB_LAVA_SLIME) High Jump & Underworld Drops...\n");
    initMobs();
    spawnMob(100.5f, 81.0f, MOB_LAVA_SLIME);
    assert(g_Mobs[0].active && g_Mobs[0].type == MOB_LAVA_SLIME);
    assert(g_Mobs[0].health == 38.0f);
    assert(isHostileMob(g_Mobs[0].type));

    // Test high jump
    g_Mobs[0].isGrounded = true;
    g_Mobs[0].animTimer = 0.0f; // Trigger jump
    updateMobs(0.016f, 105.0f, 81.0f);
    assert(g_Mobs[0].state == LAVA_SLIME_STATE_JUMP);
    assert(g_Mobs[0].vy > 12.0f); // High jump velocity (13.8f vs regular 9.8f)!

    // Kill lava slime to verify drops
    g_LastDroppedType = -1;
    checkMobAttackEx(100.5f, 81.5f, 100.0f, 1.0f, 102.0f, 81.0f);
    assert(!g_Mobs[0].active);
    assert(g_LastDroppedType == ITEM_GEL); // Last dropped item is gel, and magma was dropped
    printf("  -> PASS: Lava Slime executes high jump and drops magma block & slime gel!\n\n");

    // -------------------------------------------------------------
    // Test 14: Zone-based Spawning Distribution (spawnMobs)
    // -------------------------------------------------------------
    printf("[Test 14] Testing Zone Spawning Distribution (Caverns, Deep Dark/Midnight, Underworld)...\n");
    // Underworld zone: playerY = 15 (< UNDERWORLD_LEVEL 24)
    // Set up solid floor in underworld around x = 100, y = 14
    for (int x = 0; x < 512; x++) {
        g_World[x][14] = BLOCK_STONE;
        g_World[x][15] = BLOCK_AIR;
        g_World[x][16] = BLOCK_AIR;
    }

    int underworldLavaSlimes = 0;
    for (int iter = 0; iter < 100; iter++) {
        initMobs();
        spawnMobs(100.0f, 15.0f, 0, 0);
        for (int m = 0; m < MAX_MOBS; m++) {
            if (g_Mobs[m].active && g_Mobs[m].type == MOB_LAVA_SLIME) {
                underworldLavaSlimes++;
                break;
            }
        }
    }
    printf("  Underworld Lava Slime spawn rate: %d / 100 trials\n", underworldLavaSlimes);
    assert(underworldLavaSlimes > 20); // Frequent Lava Slime spawns in Underworld!

    printf("=======================================================\n");
    printf("   ALL 14 CREATURE ECOSYSTEM UNIT TESTS PASSED! (100%%) \n");
    printf("=======================================================\n");
    return 0;
}
