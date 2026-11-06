#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <windows.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ATLAS_TILE_RES 16
#define ATLAS_TILES_ACROSS 16
#define ATLAS_TILES_DOWN 16
#define ATLAS_RES (ATLAS_TILE_RES * ATLAS_TILES_ACROSS) // 256

#define TILE_GRASS_TOP   27
#define TILE_WOOD_TOP    28
#define TILE_CACTUS_TOP  29

#define TILE_STICK            30
#define TILE_WOOD_PICKAXE     31
#define TILE_WOOD_AXE         32
#define TILE_WOOD_SWORD       33
#define TILE_WOOD_SHOVEL      34
#define TILE_STONE_PICKAXE    35
#define TILE_STONE_AXE        36
#define TILE_STONE_SWORD      37
#define TILE_IRON_PICKAXE     38
#define TILE_IRON_AXE         39
#define TILE_IRON_SWORD       40
#define TILE_DIAMOND_PICKAXE  41
#define TILE_DIAMOND_SWORD    42
#define TILE_GEL              43
#define TILE_STICKY_BOMB      44

static inline unsigned int atlas_hash2d(int x, int y, int seed) {
    unsigned int n = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + (unsigned int)seed * 91234567u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return n ^ (n >> 16);
}

static inline void atlas_set_pixel(unsigned char* buf, int tile_idx, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    if (x < 0 || x >= ATLAS_TILE_RES || y < 0 || y >= ATLAS_TILE_RES) return;
    int tile_x = tile_idx % ATLAS_TILES_ACROSS;
    int tile_y = (ATLAS_TILES_DOWN - 1) - (tile_idx / ATLAS_TILES_ACROSS);
    int px = tile_x * ATLAS_TILE_RES + x;
    int py = tile_y * ATLAS_TILE_RES + y;
    int idx = (py * ATLAS_RES + px) * 4;
    buf[idx + 0] = r;
    buf[idx + 1] = g;
    buf[idx + 2] = b;
    buf[idx + 3] = a;
}

// ---------------------------------------------------------------------------
// Block Painters (C implementation matching Unity C# 100%)
// ---------------------------------------------------------------------------

static void paint_dirt(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 101) % 100;
            if (h < 25)      atlas_set_pixel(diff, tile_idx, x, y, 61, 36, 20, 255);
            else if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 89, 56, 34, 255);
            else if (h < 85) atlas_set_pixel(diff, tile_idx, x, y, 110, 70, 42, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 133, 87, 52, 255);
        }
    }
    // River pebbles
    atlas_set_pixel(diff, tile_idx, 3, 4, 66, 62, 59, 255);  atlas_set_pixel(diff, tile_idx, 4, 4, 122, 117, 111, 255); atlas_set_pixel(diff, tile_idx, 5, 4, 66, 62, 59, 255);
    atlas_set_pixel(diff, tile_idx, 3, 5, 122, 117, 111, 255); atlas_set_pixel(diff, tile_idx, 4, 5, 166, 160, 153, 255); atlas_set_pixel(diff, tile_idx, 5, 5, 122, 117, 111, 255);

    atlas_set_pixel(diff, tile_idx, 10, 10, 66, 62, 59, 255); atlas_set_pixel(diff, tile_idx, 11, 10, 122, 117, 111, 255); atlas_set_pixel(diff, tile_idx, 12, 10, 66, 62, 59, 255);
    atlas_set_pixel(diff, tile_idx, 10, 11, 122, 117, 111, 255); atlas_set_pixel(diff, tile_idx, 11, 11, 166, 160, 153, 255); atlas_set_pixel(diff, tile_idx, 12, 11, 122, 117, 111, 255);

    atlas_set_pixel(diff, tile_idx, 7, 1, 66, 62, 59, 255);   atlas_set_pixel(diff, tile_idx, 8, 1, 66, 62, 59, 255);
    atlas_set_pixel(diff, tile_idx, 7, 2, 122, 117, 111, 255); atlas_set_pixel(diff, tile_idx, 8, 2, 166, 160, 153, 255);
}

static void paint_grass_side(unsigned char* diff, int tile_idx) {
    paint_dirt(diff, tile_idx);
    int bladeDepths[16] = { 12, 10, 12, 11, 9, 11, 12, 10, 12, 11, 9, 11, 10, 12, 11, 12 };
    for (int x = 0; x < 16; x++) {
        int d = bladeDepths[x];
        if (d > 0) atlas_set_pixel(diff, tile_idx, x, d - 1, 46, 28, 16, 255);
        for (int y = d; y < 16; y++) {
            if (y == 15) {
                if (x % 3 == 0) atlas_set_pixel(diff, tile_idx, x, y, 167, 244, 50, 255);
                else atlas_set_pixel(diff, tile_idx, x, y, 110, 204, 38, 255);
            } else if (y >= 13) {
                if ((x + y) % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 110, 204, 38, 255);
                else atlas_set_pixel(diff, tile_idx, x, y, 70, 158, 28, 255);
            } else if (y >= 11) {
                if ((x ^ y) % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 70, 158, 28, 255);
                else atlas_set_pixel(diff, tile_idx, x, y, 38, 97, 16, 255);
            } else {
                atlas_set_pixel(diff, tile_idx, x, y, 38, 97, 16, 255);
            }
        }
    }
}

static void paint_grass_top(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 202) % 100;
            if (h < 20)      atlas_set_pixel(diff, tile_idx, x, y, 46, 125, 20, 255);
            else if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 65, 156, 28, 255);
            else if (h < 85) atlas_set_pixel(diff, tile_idx, x, y, 92, 184, 46, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 130, 214, 56, 255);
        }
    }
    // Clover & flowers
    atlas_set_pixel(diff, tile_idx, 4, 5, 160, 240, 60, 255);
    atlas_set_pixel(diff, tile_idx, 5, 5, 160, 240, 60, 255);
    atlas_set_pixel(diff, tile_idx, 4, 6, 160, 240, 60, 255);

    atlas_set_pixel(diff, tile_idx, 10, 11, 255, 152, 0, 255);
    atlas_set_pixel(diff, tile_idx, 9, 11, 255, 235, 59, 255);
    atlas_set_pixel(diff, tile_idx, 11, 11, 255, 235, 59, 255);
    atlas_set_pixel(diff, tile_idx, 10, 12, 255, 235, 59, 255);
    atlas_set_pixel(diff, tile_idx, 10, 10, 255, 235, 59, 255);

    atlas_set_pixel(diff, tile_idx, 13, 3, 255, 152, 0, 255);
    atlas_set_pixel(diff, tile_idx, 12, 3, 255, 255, 255, 255);
    atlas_set_pixel(diff, tile_idx, 14, 3, 255, 255, 255, 255);
    atlas_set_pixel(diff, tile_idx, 13, 4, 255, 255, 255, 255);
    atlas_set_pixel(diff, tile_idx, 13, 2, 255, 255, 255, 255);
}

static void paint_stone(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 303) % 100;
            if (h < 35)      atlas_set_pixel(diff, tile_idx, x, y, 95, 100, 107, 255);
            else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 115, 121, 129, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 135, 142, 151, 255);
        }
    }
    // Horizontal seam at y = 8
    for (int x = 0; x < 16; x++) {
        atlas_set_pixel(diff, tile_idx, x, 8, 38, 41, 46, 255);
        atlas_set_pixel(diff, tile_idx, x, 9, 180, 188, 198, 255);
        atlas_set_pixel(diff, tile_idx, x, 7, 60, 64, 70, 255);
    }
    // Upper vertical seam at x = 8
    for (int y = 9; y < 16; y++) {
        atlas_set_pixel(diff, tile_idx, 8, y, 38, 41, 46, 255);
        atlas_set_pixel(diff, tile_idx, 9, y, 180, 188, 198, 255);
        atlas_set_pixel(diff, tile_idx, 7, y, 60, 64, 70, 255);
    }
    // Lower vertical seam at x = 12
    for (int y = 0; y < 8; y++) {
        atlas_set_pixel(diff, tile_idx, 12, y, 38, 41, 46, 255);
        atlas_set_pixel(diff, tile_idx, 13, y, 180, 188, 198, 255);
        atlas_set_pixel(diff, tile_idx, 11, y, 60, 64, 70, 255);
    }
    // Fracture
    atlas_set_pixel(diff, tile_idx, 2, 2, 38, 41, 46, 255);  atlas_set_pixel(diff, tile_idx, 2, 3, 180, 188, 198, 255);
    atlas_set_pixel(diff, tile_idx, 3, 3, 38, 41, 46, 255);  atlas_set_pixel(diff, tile_idx, 3, 4, 180, 188, 198, 255);
    atlas_set_pixel(diff, tile_idx, 4, 4, 38, 41, 46, 255);  atlas_set_pixel(diff, tile_idx, 4, 5, 180, 188, 198, 255);
    atlas_set_pixel(diff, tile_idx, 5, 4, 38, 41, 46, 255);
    atlas_set_pixel(diff, tile_idx, 6, 5, 38, 41, 46, 255);  atlas_set_pixel(diff, tile_idx, 6, 6, 180, 188, 198, 255);
}

static void paint_wood(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        int f1 = 3 + ((y >= 4 && y <= 10) ? 1 : 0);
        int f2 = 8 + ((y >= 11 || y <= 3) ? 1 : 0);
        int f3 = 13 + ((y >= 6 && y <= 12) ? 1 : 0);

        for (int x = 0; x < 16; x++) {
            if (x == f1 || x == f2 || x == f3) {
                if (y % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 32, 19, 11, 255);
                else            atlas_set_pixel(diff, tile_idx, x, y, 49, 30, 19, 255);
            } else if (x == f1 + 1 || x == f2 + 1 || x == f3 + 1) {
                if (y % 3 == 0) atlas_set_pixel(diff, tile_idx, x, y, 168, 109, 67, 255);
                else            atlas_set_pixel(diff, tile_idx, x, y, 138, 87, 52, 255);
            } else if (x == f1 - 1 || x == f2 - 1 || x == f3 - 1) {
                atlas_set_pixel(diff, tile_idx, x, y, 49, 30, 19, 255);
            } else {
                unsigned int h = atlas_hash2d(x, y, 404) % 100;
                if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 94, 59, 35, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 138, 87, 52, 255);
            }
        }
    }
    // Knot
    atlas_set_pixel(diff, tile_idx, 9, 6, 49, 30, 19, 255); atlas_set_pixel(diff, tile_idx, 10, 6, 32, 19, 11, 255); atlas_set_pixel(diff, tile_idx, 11, 6, 49, 30, 19, 255);
    atlas_set_pixel(diff, tile_idx, 9, 7, 138, 87, 52, 255); atlas_set_pixel(diff, tile_idx, 10, 7, 168, 109, 67, 255); atlas_set_pixel(diff, tile_idx, 11, 7, 138, 87, 52, 255);
}

static void paint_wood_top(unsigned char* diff, int tile_idx) {
    float cx = 7.5f, cy = 7.5f;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (x == 0 || x == 15 || y == 0 || y == 15) {
                if ((x + y) % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 49, 30, 19, 255);
                else                  atlas_set_pixel(diff, tile_idx, x, y, 94, 59, 35, 255);
            } else {
                float dist = sqrtf((x - cx)*(x - cx) + (y - cy)*(y - cy));
                if (dist < 1.8f) {
                    atlas_set_pixel(diff, tile_idx, x, y, 112, 75, 40, 255);
                } else if (dist >= 3.0f && dist <= 3.8f) {
                    atlas_set_pixel(diff, tile_idx, x, y, 153, 115, 72, 255);
                } else if (dist >= 5.2f && dist <= 6.0f) {
                    atlas_set_pixel(diff, tile_idx, x, y, 153, 115, 72, 255);
                } else {
                    if ((x + y) % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 215, 181, 135, 255);
                    else                  atlas_set_pixel(diff, tile_idx, x, y, 194, 158, 112, 255);
                }
            }
        }
    }
    // Radial check
    for (int i = 8; i <= 12; i++) {
        atlas_set_pixel(diff, tile_idx, i, i, 32, 19, 11, 255);
    }
}

static void paint_leaves(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 505) % 100;
            if (h < 25)      atlas_set_pixel(diff, tile_idx, x, y, 22, 61, 12, 255);
            else if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 39, 97, 24, 255);
            else if (h < 85) atlas_set_pixel(diff, tile_idx, x, y, 78, 168, 44, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 141, 227, 68, 255);
        }
    }
    // Cutouts
    int cutouts[][2] = {
        {0,0}, {15,0}, {0,15}, {15,15}, {1,0}, {0,1}, {14,15}, {15,14},
        {7,7}, {8,7}, {7,8}, {3,11}, {4,11}, {12,4}, {13,4}
    };
    int numCutouts = sizeof(cutouts) / sizeof(cutouts[0]);
    for (int i = 0; i < numCutouts; i++) {
        atlas_set_pixel(diff, tile_idx, cutouts[i][0], cutouts[i][1], 0, 0, 0, 0);
    }
}

static void paint_sand(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            float wave = sinf(x * 0.45f + (y / 4) * 1.5f) * 1.2f;
            float val = fmodf(y + wave, 4.0f);
            if (val < 0.0f) val += 4.0f;

            if (val < 0.8f)      atlas_set_pixel(diff, tile_idx, x, y, 255, 245, 165, 255);
            else if (val < 2.0f) atlas_set_pixel(diff, tile_idx, x, y, 245, 220, 125, 255);
            else if (val < 3.2f) atlas_set_pixel(diff, tile_idx, x, y, 225, 190, 95, 255);
            else                 atlas_set_pixel(diff, tile_idx, x, y, 195, 155, 65, 255);

            if (atlas_hash2d(x, y, 606) % 23 == 0) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 255, 220, 255);
            }
        }
    }
}

static void paint_sandstone(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        if (y == 3 || y == 7 || y == 11 || y == 15) {
            for (int x = 0; x < 16; x++) atlas_set_pixel(diff, tile_idx, x, y, 115, 62, 31, 255);
            continue;
        }
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 707) % 100;
            if (y < 3) {
                if (h < 55) atlas_set_pixel(diff, tile_idx, x, y, 140, 82, 50, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 165, 98, 62, 255);
            } else if (y < 7) {
                if (h < 55) atlas_set_pixel(diff, tile_idx, x, y, 205, 155, 80, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 225, 175, 95, 255);
            } else if (y < 11) {
                if (h < 55) atlas_set_pixel(diff, tile_idx, x, y, 230, 210, 155, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 245, 228, 180, 255);
            } else {
                if (h < 55) atlas_set_pixel(diff, tile_idx, x, y, 186, 117, 76, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 206, 133, 90, 255);
            }
        }
    }
}

static void paint_cactus_side(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int col = x % 4;
            if (col == 0 || col == 3) atlas_set_pixel(diff, tile_idx, x, y, 27, 88, 18, 255);
            else if (col == 1)        atlas_set_pixel(diff, tile_idx, x, y, 82, 192, 58, 255);
            else                      atlas_set_pixel(diff, tile_idx, x, y, 50, 135, 35, 255);

            if (col == 1 && (y % 4 == 0)) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 238, 88, 255);
                if (x > 0) atlas_set_pixel(diff, tile_idx, x - 1, y, 255, 255, 230, 255);
            } else if (col == 3 && (y % 4 == 2)) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 238, 88, 255);
                if (x + 1 < 16) atlas_set_pixel(diff, tile_idx, x + 1, y, 255, 255, 230, 255);
            }
        }
    }
}

static void paint_cactus_top(unsigned char* diff, int tile_idx) {
    float cx = 7.5f, cy = 7.5f;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            float dx = x - cx, dy = y - cy;
            float angle = atan2f(dy, dx);
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < 2.0f) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 238, 88, 255);
            } else {
                float normAngle = (angle + (float)M_PI) / ((float)M_PI / 4.0f);
                int rib = (int)(normAngle + 0.5f) % 8;
                float diffRib = fabsf(normAngle - (float)rib);
                if (diffRib < 0.15f)      atlas_set_pixel(diff, tile_idx, x, y, 82, 192, 58, 255);
                else if (diffRib < 0.35f) atlas_set_pixel(diff, tile_idx, x, y, 50, 135, 35, 255);
                else                      atlas_set_pixel(diff, tile_idx, x, y, 27, 88, 18, 255);
            }
        }
    }
    atlas_set_pixel(diff, tile_idx, 7, 7, 255, 255, 230, 255); atlas_set_pixel(diff, tile_idx, 8, 7, 255, 255, 230, 255);
    atlas_set_pixel(diff, tile_idx, 7, 8, 255, 255, 230, 255); atlas_set_pixel(diff, tile_idx, 8, 8, 255, 255, 230, 255);
}

static void paint_snow(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 808) % 100;
            float wave = sinf(x * 0.4f) * 1.5f + cosf(y * 0.5f) * 1.0f;
            if (wave > 1.2f) {
                if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 255, 255, 255, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 240, 248, 255, 255);
            } else if (wave > -0.5f) {
                if (h < 70) atlas_set_pixel(diff, tile_idx, x, y, 240, 248, 255, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 205, 228, 245, 255);
            } else {
                if (h < 60) atlas_set_pixel(diff, tile_idx, x, y, 205, 228, 245, 255);
                else        atlas_set_pixel(diff, tile_idx, x, y, 168, 202, 226, 255);
            }
        }
    }
}

static void paint_ice(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 909) % 100;
            if (h < 30)      atlas_set_pixel(diff, tile_idx, x, y, 38, 140, 166, 220);
            else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 68, 176, 204, 230);
            else             atlas_set_pixel(diff, tile_idx, x, y, 120, 215, 235, 240);
        }
    }
    int cracks[][2] = {
        {2,2}, {3,3}, {4,4}, {5,6}, {6,7}, {7,8}, {8,9},
        {9,10}, {10,12}, {11,13}, {12,14}, {13,14},
        {8,9}, {9,8}, {10,7}, {11,7}, {12,6}
    };
    int numCracks = sizeof(cracks) / sizeof(cracks[0]);
    for (int i = 0; i < numCracks; i++) {
        int cx = cracks[i][0], cy = cracks[i][1];
        atlas_set_pixel(diff, tile_idx, cx, cy, 255, 255, 255, 255);
        if (cx + 1 < 16) atlas_set_pixel(diff, tile_idx, cx + 1, cy, 210, 250, 255, 255);
    }
}

static void paint_clay(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int swirl = abs((int)(sinf(x * 0.4f + y * 0.3f) * 2.5f + (x + y * 2) * 0.25f)) % 4;
            if (swirl == 0)      atlas_set_pixel(diff, tile_idx, x, y, 125, 104, 96, 255);
            else if (swirl == 1) atlas_set_pixel(diff, tile_idx, x, y, 145, 124, 116, 255);
            else if (swirl == 2) atlas_set_pixel(diff, tile_idx, x, y, 168, 145, 136, 255);
            else                 atlas_set_pixel(diff, tile_idx, x, y, 188, 166, 156, 255);
        }
    }
}

static void paint_ore(unsigned char* diff, unsigned char* emit, int tile_idx,
                      unsigned char r1, unsigned char g1, unsigned char b1,
                      unsigned char r2, unsigned char g2, unsigned char b2,
                      bool isEmissive) {
    paint_stone(diff, tile_idx);

    int clusters[][3] = {
        {3, 11, 0}, {4, 10, 0}, {5, 10, 0}, {6, 11, 0},
        {4, 11, 1}, {5, 11, 1}, {4, 12, 2}, {5, 12, 1},
        {4, 12, 3}, // Specular glint
        {9, 4, 0}, {10, 3, 0}, {11, 3, 0}, {12, 4, 0}, {13, 5, 0},
        {10, 4, 1}, {11, 4, 1}, {12, 5, 1}, {11, 5, 2},
        {11, 5, 3}, // Specular glint
        {7, 6, 0}, {8, 6, 1}, {7, 7, 2}, {8, 7, 3}
    };
    int numClusters = sizeof(clusters) / sizeof(clusters[0]);

    for (int i = 0; i < numClusters; i++) {
        int gx = clusters[i][0];
        int gy = clusters[i][1];
        int t = clusters[i][2];

        unsigned char r, g, b;
        if (t == 0)      { r = 20; g = 20; b = 20; }
        else if (t == 1) { r = r1; g = g1; b = b1; }
        else if (t == 2) { r = r2; g = g2; b = b2; }
        else             { r = 255; g = 255; b = 255; } // Star glint

        atlas_set_pixel(diff, tile_idx, gx, gy, r, g, b, 255);
        if (isEmissive && t >= 1 && emit) {
            atlas_set_pixel(emit, tile_idx, gx, gy, r, g, b, 255);
        }
    }
}

static void paint_brick(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        bool isHMortar = (y % 4 == 0);
        int row = y / 4;
        int xShift = (row % 2 == 0) ? 0 : 4;

        for (int x = 0; x < 16; x++) {
            bool isVMortar = ((x + xShift) % 8 == 0);
            if (isHMortar || isVMortar) {
                if ((x + y) % 2 == 0) atlas_set_pixel(diff, tile_idx, x, y, 133, 138, 145, 255);
                else                  atlas_set_pixel(diff, tile_idx, x, y, 209, 213, 219, 255);
            } else {
                int localY = y % 4;
                int localX = (x + xShift) % 8;
                if (localY == 3 || localX == 1) {
                    atlas_set_pixel(diff, tile_idx, x, y, 240, 120, 95, 255);
                } else {
                    unsigned int h = atlas_hash2d(x, y, 1111) % 100;
                    if (h < 30)      atlas_set_pixel(diff, tile_idx, x, y, 145, 39, 20, 255);
                    else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 184, 58, 35, 255);
                    else             atlas_set_pixel(diff, tile_idx, x, y, 217, 90, 65, 255);
                }
            }
        }
    }
}

static void paint_wood_plank(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        bool isGroove = (y % 4 == 0);
        for (int x = 0; x < 16; x++) {
            if (isGroove) {
                atlas_set_pixel(diff, tile_idx, x, y, 46, 24, 11, 255);
            } else {
                int localY = y % 4;
                if (localY == 3) {
                    atlas_set_pixel(diff, tile_idx, x, y, 215, 160, 105, 255);
                } else {
                    unsigned int h = atlas_hash2d(x, y, 1212) % 100;
                    if (h < 35)      atlas_set_pixel(diff, tile_idx, x, y, 140, 91, 48, 255);
                    else if (h < 80) atlas_set_pixel(diff, tile_idx, x, y, 168, 114, 62, 255);
                    else             atlas_set_pixel(diff, tile_idx, x, y, 194, 138, 82, 255);
                }
            }
        }
    }
    // Nails
    int nails[][2] = { {1,2}, {14,2}, {2,6}, {13,6}, {1,10}, {14,10}, {2,14}, {13,14} };
    int numNails = sizeof(nails) / sizeof(nails[0]);
    for (int i = 0; i < numNails; i++) {
        atlas_set_pixel(diff, tile_idx, nails[i][0], nails[i][1], 48, 48, 48, 255);
        atlas_set_pixel(diff, tile_idx, nails[i][0], nails[i][1] + 1, 160, 160, 160, 255);
    }
}

static void paint_glass(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (x == 0 || x == 15 || y == 0 || y == 15) {
                atlas_set_pixel(diff, tile_idx, x, y, 75, 85, 99, 255);
            } else if ((x + y == 17) || (x + y == 18 && x >= 4 && x <= 12)) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 255, 255, 195);
            } else if ((x + y == 9 && x >= 2 && x <= 6) || (x + y == 10 && x >= 3 && x <= 5)) {
                atlas_set_pixel(diff, tile_idx, x, y, 255, 255, 255, 135);
            } else {
                atlas_set_pixel(diff, tile_idx, x, y, 195, 235, 247, 55);
            }
        }
    }
}

static void paint_torch(unsigned char* diff, unsigned char* emit, int tile_idx) {
    // Stick
    for (int y = 1; y < 8; y++) {
        atlas_set_pixel(diff, tile_idx, 6, y, 54, 27, 12, 255);
        atlas_set_pixel(diff, tile_idx, 7, y, 117, 68, 33, 255);
        atlas_set_pixel(diff, tile_idx, 8, y, 148, 89, 45, 255);
        atlas_set_pixel(diff, tile_idx, 9, y, 54, 27, 12, 255);
    }
    for (int x = 6; x < 10; x++) {
        atlas_set_pixel(diff, tile_idx, x, 8, 36, 32, 28, 255);
    }
    // Flame
    int flame[][3] = {
        {7, 9, 0}, {8, 9, 0},
        {6, 10, 0}, {7, 10, 1}, {8, 10, 1}, {9, 10, 0},
        {5, 11, 0}, {6, 11, 1}, {7, 11, 2}, {8, 11, 2}, {9, 11, 1}, {10, 11, 0},
        {5, 12, 0}, {6, 12, 2}, {7, 12, 3}, {8, 12, 3}, {9, 12, 2}, {10, 12, 0},
        {6, 13, 0}, {7, 13, 2}, {8, 13, 2}, {9, 13, 0},
        {7, 14, 1}, {8, 14, 1},
        {7, 15, 0}
    };
    int numFlame = sizeof(flame) / sizeof(flame[0]);
    for (int i = 0; i < numFlame; i++) {
        int fx = flame[i][0], fy = flame[i][1], ct = flame[i][2];
        unsigned char r, g, b;
        if (ct == 0)      { r = 211; g = 47; b = 47; }
        else if (ct == 1) { r = 255; g = 109; b = 0; }
        else if (ct == 2) { r = 255; g = 214; b = 0; }
        else              { r = 255; g = 255; b = 255; }

        atlas_set_pixel(diff, tile_idx, fx, fy, r, g, b, 255);
        if (emit) atlas_set_pixel(emit, tile_idx, fx, fy, r, g, b, 255);
    }
}

static void paint_obsidian(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 1313) % 100;
            if (h < 45)      atlas_set_pixel(diff, tile_idx, x, y, 18, 9, 28, 255);
            else if (h < 85) atlas_set_pixel(diff, tile_idx, x, y, 28, 15, 42, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 48, 24, 72, 255);
        }
    }
    int arcs[][2] = {
        {2,4}, {3,5}, {4,6}, {5,8}, {6,9}, {7,10}, {8,12}, {9,13},
        {8,3}, {9,4}, {10,5}, {11,7}, {12,8}, {13,10}
    };
    int numArcs = sizeof(arcs) / sizeof(arcs[0]);
    for (int i = 0; i < numArcs; i++) {
        int ax = arcs[i][0], ay = arcs[i][1];
        atlas_set_pixel(diff, tile_idx, ax, ay, 104, 36, 140, 255);
        if (ax + 1 < 16) atlas_set_pixel(diff, tile_idx, ax + 1, ay, 156, 56, 199, 255);
    }
    atlas_set_pixel(diff, tile_idx, 6, 9, 241, 225, 255, 255);
    atlas_set_pixel(diff, tile_idx, 11, 7, 241, 225, 255, 255);
}

static void paint_magma(unsigned char* diff, unsigned char* emit, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 1414) % 100;
            if (h < 40)      atlas_set_pixel(diff, tile_idx, x, y, 23, 21, 21, 255);
            else if (h < 85) atlas_set_pixel(diff, tile_idx, x, y, 43, 40, 40, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 65, 60, 60, 255);
        }
    }
    int fissures[][3] = {
        {0, 3, 0}, {1, 4, 1}, {2, 4, 2}, {3, 5, 3},
        {4, 5, 2}, {5, 6, 1}, {6, 7, 3}, {7, 7, 2},
        {8, 8, 3}, {9, 8, 2}, {10, 9, 1}, {11, 10, 3},
        {12, 11, 2}, {13, 12, 1}, {14, 13, 0}, {15, 14, 0},
        {8, 7, 1}, {8, 6, 2}, {9, 5, 3}, {10, 4, 2}, {11, 3, 1}, {12, 2, 0},
        {6, 8, 1}, {6, 9, 2}, {5, 10, 3}, {5, 11, 2}, {4, 12, 1}, {3, 13, 0}
    };
    int numFissures = sizeof(fissures) / sizeof(fissures[0]);
    for (int i = 0; i < numFissures; i++) {
        int fx = fissures[i][0], fy = fissures[i][1], ct = fissures[i][2];
        unsigned char r, g, b;
        if (ct == 0)      { r = 191; g = 21; b = 0; }
        else if (ct == 1) { r = 255; g = 109; b = 0; }
        else if (ct == 2) { r = 255; g = 214; b = 0; }
        else              { r = 255; g = 249; b = 196; }

        atlas_set_pixel(diff, tile_idx, fx, fy, r, g, b, 255);
        if (emit) atlas_set_pixel(emit, tile_idx, fx, fy, r, g, b, 255);
    }
}

static void paint_mossy_stone(unsigned char* diff, int tile_idx) {
    paint_stone(diff, tile_idx);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if ((x < 9 && y > 6) || (x > 8 && y < 8) || (x % 3 == 0 && y % 2 == 0)) {
                unsigned int h = atlas_hash2d(x, y, 1515) % 100;
                if (h < 30)      atlas_set_pixel(diff, tile_idx, x, y, 46, 82, 16, 255);
                else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 78, 140, 34, 255);
                else if (h < 95) atlas_set_pixel(diff, tile_idx, x, y, 128, 196, 51, 255);
            }
        }
    }
}

static void paint_mud(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 1616) % 100;
            if (h < 35)      atlas_set_pixel(diff, tile_idx, x, y, 44, 26, 14, 255);
            else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 61, 38, 22, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 77, 49, 30, 255);

            if ((x >= 3 && x <= 7 && y >= 4 && y <= 7) || (x >= 10 && x <= 13 && y >= 9 && y <= 12)) {
                if ((x == 4 && y == 5) || (x == 11 && y == 10)) {
                    atlas_set_pixel(diff, tile_idx, x, y, 140, 104, 74, 255);
                } else {
                    atlas_set_pixel(diff, tile_idx, x, y, 99, 67, 43, 255);
                }
            }
        }
    }
}

static void paint_glowshroom(unsigned char* diff, unsigned char* emit, int tile_idx) {
    // Stalk
    for (int y = 1; y < 7; y++) {
        atlas_set_pixel(diff, tile_idx, 6, y, 40, 120, 140, 255);
        atlas_set_pixel(diff, tile_idx, 7, y, 84, 197, 217, 255);
        atlas_set_pixel(diff, tile_idx, 8, y, 84, 197, 217, 255);
        atlas_set_pixel(diff, tile_idx, 9, y, 40, 120, 140, 255);
    }
    // Cap
    for (int y = 7; y <= 12; y++) {
        for (int x = 2; x <= 13; x++) {
            if (y == 7 && (x < 3 || x > 12)) continue;
            if (y == 11 && (x < 3 || x > 12)) continue;
            if (y == 12 && (x < 5 || x > 10)) continue;

            if (y == 7 || y == 8) {
                atlas_set_pixel(diff, tile_idx, x, y, 0, 107, 133, 255);
                if (emit) atlas_set_pixel(emit, tile_idx, x, y, 0, 107, 133, 255);
            } else {
                atlas_set_pixel(diff, tile_idx, x, y, 0, 229, 255, 255);
                if (emit) atlas_set_pixel(emit, tile_idx, x, y, 0, 229, 255, 255);
            }
        }
    }
    // Spots & Spores
    int spots[][2] = {
        {5,10}, {7,11}, {10,10}, {4,8}, {11,8},
        {3,14}, {6,15}, {11,14}, {13,13}
    };
    int numSpots = sizeof(spots) / sizeof(spots[0]);
    for (int i = 0; i < numSpots; i++) {
        atlas_set_pixel(diff, tile_idx, spots[i][0], spots[i][1], 224, 255, 255, 255);
        if (emit) atlas_set_pixel(emit, tile_idx, spots[i][0], spots[i][1], 224, 255, 255, 255);
    }
}

static void paint_bedrock(unsigned char* diff, int tile_idx) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            unsigned int h = atlas_hash2d(x, y, 1717) % 100;
            if (h < 35)      atlas_set_pixel(diff, tile_idx, x, y, 8, 9, 10, 255);
            else if (h < 75) atlas_set_pixel(diff, tile_idx, x, y, 20, 22, 24, 255);
            else if (h < 93) atlas_set_pixel(diff, tile_idx, x, y, 34, 37, 42, 255);
            else             atlas_set_pixel(diff, tile_idx, x, y, 56, 60, 68, 255);
        }
    }
}

// ---------------------------------------------------------------------------
// Craftable Items & Tools Painters (16x16 Pixel Art)
// ---------------------------------------------------------------------------

static void paint_item_stick(unsigned char* diff, int tile_idx) {
    for (int i = 3; i <= 12; i++) {
        atlas_set_pixel(diff, tile_idx, i, i, 140, 95, 48, 255);
        atlas_set_pixel(diff, tile_idx, i - 1, i, 185, 135, 75, 255);
        atlas_set_pixel(diff, tile_idx, i, i - 1, 95, 55, 25, 255);
    }
}

static void paint_item_pickaxe(unsigned char* diff, int tile_idx,
    unsigned char hr, unsigned char hg, unsigned char hb,
    unsigned char br, unsigned char bg, unsigned char bb,
    unsigned char dr, unsigned char dg, unsigned char db) {
    paint_item_stick(diff, tile_idx);
    int headCoords[7][2] = { {6,13}, {8,12}, {10,11}, {11,10}, {12,8}, {13,6}, {10,10} };
    for (int i = 0; i < 7; i++) {
        int x = headCoords[i][0];
        int y = headCoords[i][1];
        atlas_set_pixel(diff, tile_idx, x, y, hr, hg, hb, 255);
        atlas_set_pixel(diff, tile_idx, x, y + 1, br, bg, bb, 255);
        atlas_set_pixel(diff, tile_idx, x + 1, y, dr, dg, db, 255);
    }
}

static void paint_item_axe(unsigned char* diff, int tile_idx,
    unsigned char hr, unsigned char hg, unsigned char hb,
    unsigned char br, unsigned char bg, unsigned char bb,
    unsigned char dr, unsigned char dg, unsigned char db) {
    paint_item_stick(diff, tile_idx);
    for (int y = 9; y <= 13; y++) {
        for (int x = 8; x <= 13; x++) {
            if ((x - 8) + (y - 9) >= 1 && (x - 8) + (y - 9) <= 7) {
                atlas_set_pixel(diff, tile_idx, x, y, hr, hg, hb, 255);
            }
        }
    }
    atlas_set_pixel(diff, tile_idx, 12, 13, br, bg, bb, 255);
    atlas_set_pixel(diff, tile_idx, 13, 12, br, bg, bb, 255);
    atlas_set_pixel(diff, tile_idx, 13, 11, br, bg, bb, 255);
    atlas_set_pixel(diff, tile_idx, 8, 9, dr, dg, db, 255);
}

static void paint_item_sword(unsigned char* diff, int tile_idx,
    unsigned char br, unsigned char bg, unsigned char bb,
    unsigned char hr, unsigned char hg, unsigned char hb,
    unsigned char dr, unsigned char dg, unsigned char db) {
    atlas_set_pixel(diff, tile_idx, 2, 2, 95, 55, 25, 255);
    atlas_set_pixel(diff, tile_idx, 3, 3, 135, 90, 45, 255);
    atlas_set_pixel(diff, tile_idx, 4, 4, 135, 90, 45, 255);
    atlas_set_pixel(diff, tile_idx, 3, 5, 80, 80, 85, 255);
    atlas_set_pixel(diff, tile_idx, 4, 5, 120, 120, 125, 255);
    atlas_set_pixel(diff, tile_idx, 5, 4, 120, 120, 125, 255);
    atlas_set_pixel(diff, tile_idx, 5, 3, 80, 80, 85, 255);
    for (int i = 5; i <= 13; i++) {
        atlas_set_pixel(diff, tile_idx, i, i, br, bg, bb, 255);
        atlas_set_pixel(diff, tile_idx, i - 1, i, hr, hg, hb, 255);
        atlas_set_pixel(diff, tile_idx, i, i - 1, dr, dg, db, 255);
    }
    atlas_set_pixel(diff, tile_idx, 14, 14, hr, hg, hb, 255);
}

static void paint_item_shovel(unsigned char* diff, int tile_idx,
    unsigned char hr, unsigned char hg, unsigned char hb,
    unsigned char br, unsigned char bg, unsigned char bb,
    unsigned char dr, unsigned char dg, unsigned char db) {
    paint_item_stick(diff, tile_idx);
    for (int x = 9; x <= 13; x++) {
        for (int y = 9; y <= 13; y++) {
            if ((x - 8) + (y - 8) >= 4) {
                atlas_set_pixel(diff, tile_idx, x, y, hr, hg, hb, 255);
            }
        }
    }
    atlas_set_pixel(diff, tile_idx, 13, 13, br, bg, bb, 255);
    atlas_set_pixel(diff, tile_idx, 12, 13, br, bg, bb, 255);
    atlas_set_pixel(diff, tile_idx, 9, 9, dr, dg, db, 255);
}

static void paint_item_gel(unsigned char* diff, int tile_idx) {
    for (int y = 3; y <= 12; y++) {
        for (int x = 3; x <= 12; x++) {
            float dx = (float)(x - 7.5f);
            float dy = (float)(y - 7.0f);
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= 4.2f) {
                atlas_set_pixel(diff, tile_idx, x, y, 46, 204, 113, 230);
            } else if (dist <= 4.8f) {
                atlas_set_pixel(diff, tile_idx, x, y, 30, 150, 75, 200);
            }
        }
    }
    atlas_set_pixel(diff, tile_idx, 6, 9, 210, 255, 220, 255);
    atlas_set_pixel(diff, tile_idx, 7, 9, 240, 255, 245, 255);
    atlas_set_pixel(diff, tile_idx, 7, 8, 210, 255, 220, 255);
}

static void paint_item_sticky_bomb(unsigned char* diff, int tile_idx) {
    for (int y = 2; y <= 11; y++) {
        for (int x = 3; x <= 12; x++) {
            float dx = (float)(x - 7.5f);
            float dy = (float)(y - 6.5f);
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= 4.0f) {
                atlas_set_pixel(diff, tile_idx, x, y, 45, 52, 54, 255);
            } else if (dist <= 4.6f) {
                atlas_set_pixel(diff, tile_idx, x, y, 25, 30, 32, 255);
            }
        }
    }
    atlas_set_pixel(diff, tile_idx, 7, 12, 120, 100, 70, 255);
    atlas_set_pixel(diff, tile_idx, 8, 13, 255, 165, 2, 255);
    atlas_set_pixel(diff, tile_idx, 8, 14, 255, 71, 87, 255);
    atlas_set_pixel(diff, tile_idx, 9, 13, 255, 230, 80, 255);
    atlas_set_pixel(diff, tile_idx, 4, 3, 46, 213, 115, 255);
    atlas_set_pixel(diff, tile_idx, 4, 2, 46, 213, 115, 255);
    atlas_set_pixel(diff, tile_idx, 10, 3, 46, 213, 115, 255);
    atlas_set_pixel(diff, tile_idx, 11, 4, 46, 213, 115, 255);
}

// ---------------------------------------------------------------------------
// Master Atlas Generator
// ---------------------------------------------------------------------------

static inline void GenerateTextureAtlasRGBA(unsigned char* outRgba, unsigned char* outEmission) {
    // Clear out buffers
    for (int i = 0; i < ATLAS_RES * ATLAS_RES * 4; i += 4) {
        outRgba[i + 0] = 0;
        outRgba[i + 1] = 0;
        outRgba[i + 2] = 0;
        outRgba[i + 3] = 0;

        if (outEmission) {
            outEmission[i + 0] = 0;
            outEmission[i + 1] = 0;
            outEmission[i + 2] = 0;
            outEmission[i + 3] = 255;
        }
    }

    paint_grass_side(outRgba, 1);
    paint_dirt(outRgba, 2);
    paint_stone(outRgba, 3);
    paint_wood(outRgba, 4);
    paint_leaves(outRgba, 5);
    paint_sand(outRgba, 6);
    paint_sandstone(outRgba, 7);
    paint_cactus_side(outRgba, 8);
    paint_snow(outRgba, 9);
    paint_ice(outRgba, 10);
    paint_clay(outRgba, 11);

    // Ores
    paint_ore(outRgba, outEmission, 12, 49, 52, 59, 86, 91, 99, false);   // Coal Ore
    paint_ore(outRgba, outEmission, 13, 170, 104, 56, 212, 149, 99, false); // Iron Ore
    paint_ore(outRgba, outEmission, 14, 240, 184, 12, 255, 222, 67, false); // Gold Ore
    paint_ore(outRgba, outEmission, 15, 0, 212, 235, 122, 233, 247, true);  // Diamond Ore
    paint_ore(outRgba, outEmission, 16, 196, 8, 40, 255, 43, 79, true);    // Ruby Ore

    paint_brick(outRgba, 17);
    paint_wood_plank(outRgba, 18);
    paint_glass(outRgba, 19);
    paint_torch(outRgba, outEmission, 20);
    paint_obsidian(outRgba, 21);
    paint_magma(outRgba, outEmission, 22);
    paint_mossy_stone(outRgba, 23);
    paint_mud(outRgba, 24);
    paint_glowshroom(outRgba, outEmission, 25);
    paint_bedrock(outRgba, 26);

    // Multi-face variants
    paint_grass_top(outRgba, TILE_GRASS_TOP);
    paint_wood_top(outRgba, TILE_WOOD_TOP);
    paint_cactus_top(outRgba, TILE_CACTUS_TOP);

    // Craftable Tools, Weapons & Items (Tiles 30 - 44)
    paint_item_stick(outRgba, TILE_STICK);
    paint_item_pickaxe(outRgba, TILE_WOOD_PICKAXE, 160, 110, 60, 200, 150, 95, 110, 75, 40);
    paint_item_axe(outRgba, TILE_WOOD_AXE, 160, 110, 60, 200, 150, 95, 110, 75, 40);
    paint_item_sword(outRgba, TILE_WOOD_SWORD, 160, 110, 60, 200, 150, 95, 110, 75, 40);
    paint_item_shovel(outRgba, TILE_WOOD_SHOVEL, 160, 110, 60, 200, 150, 95, 110, 75, 40);

    paint_item_pickaxe(outRgba, TILE_STONE_PICKAXE, 140, 145, 152, 185, 190, 198, 90, 95, 100);
    paint_item_axe(outRgba, TILE_STONE_AXE, 140, 145, 152, 185, 190, 198, 90, 95, 100);
    paint_item_sword(outRgba, TILE_STONE_SWORD, 140, 145, 152, 185, 190, 198, 90, 95, 100);

    paint_item_pickaxe(outRgba, TILE_IRON_PICKAXE, 210, 215, 225, 250, 252, 255, 140, 145, 155);
    paint_item_axe(outRgba, TILE_IRON_AXE, 210, 215, 225, 250, 252, 255, 140, 145, 155);
    paint_item_sword(outRgba, TILE_IRON_SWORD, 210, 215, 225, 250, 252, 255, 140, 145, 155);

    paint_item_pickaxe(outRgba, TILE_DIAMOND_PICKAXE, 70, 220, 240, 170, 250, 255, 30, 150, 175);
    paint_item_sword(outRgba, TILE_DIAMOND_SWORD, 70, 220, 240, 170, 250, 255, 30, 150, 175);

    paint_item_gel(outRgba, TILE_GEL);
    paint_item_sticky_bomb(outRgba, TILE_STICKY_BOMB);
}

// ---------------------------------------------------------------------------
// OpenGL Texture Helpers
// ---------------------------------------------------------------------------

static inline void GetAtlasTileUV(int tileIndex, float* uMin, float* vMin, float* uMax, float* vMax) {
    if (tileIndex < 0) tileIndex = 0;
    int tileX = tileIndex % ATLAS_TILES_ACROSS;
    int tileY = (ATLAS_TILES_DOWN - 1) - (tileIndex / ATLAS_TILES_ACROSS);

    const float inv = 1.0f / (float)ATLAS_TILES_ACROSS;
    const float pad = 0.0005f;

    *uMin = (float)tileX * inv + pad;
    *uMax = (float)(tileX + 1) * inv - pad;
    *vMin = (float)tileY * inv + pad;
    *vMax = (float)(tileY + 1) * inv - pad;
}

static inline int GetBlockFaceTextureIndex(int blockType, int face) {
    // face: 0 = Front, 1 = Top (+Y), 2 = Bottom (-Y), 3 = Left (-X), 4 = Right (+X)
    if (blockType <= 0) return 0;

    if (blockType == 1) { // Grass
        if (face == 1) return TILE_GRASS_TOP;
        if (face == 2) return 2; // Dirt bottom
        return 1; // Side grass fringe
    }
    if (blockType == 4) { // Wood
        if (face == 1 || face == 2) return TILE_WOOD_TOP;
        return 4; // Bark side
    }
    if (blockType == 8) { // Cactus
        if (face == 1 || face == 2) return TILE_CACTUS_TOP;
        return 8; // Ribbed side
    }
    // Craftable Tools, Weapons & Items (mapped to tiles 30-44)
    if (blockType >= 27 && blockType <= 41) {
        return (blockType - 27) + TILE_STICK;
    }
    return blockType;
}

static inline GLuint CreateTextureAtlasGL(void) {
    unsigned char* rgba = (unsigned char*)malloc(ATLAS_RES * ATLAS_RES * 4);
    if (!rgba) return 0;

    GenerateTextureAtlasRGBA(rgba, NULL);

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_RES, ATLAS_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    free(rgba);
    return texId;
}

#endif // TEXTURE_ATLAS_H
