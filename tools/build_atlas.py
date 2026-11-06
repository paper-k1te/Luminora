"""
Master Pixel Art Atlas Generator
Generates high-detail 16x16 pixel art for all 27 voxel blocks + multi-face variants,
compiling them into a 256x256 RGBA atlas texture and emission map.
"""
import os
import math
from PIL import Image

TILE_RES = 16
TILES_ACROSS = 16
TILES_DOWN = 16
ATLAS_RES = TILE_RES * TILES_ACROSS # 256

def create_buffers():
    diffuse = [(0, 0, 0, 0)] * (ATLAS_RES * ATLAS_RES)
    emission = [(0, 0, 0, 255)] * (ATLAS_RES * ATLAS_RES)
    return diffuse, emission

def set_atlas_pixel(buf, tile_idx, x, y, color):
    if x < 0 or x >= TILE_RES or y < 0 or y >= TILE_RES:
        return
    tile_x = tile_idx % TILES_ACROSS
    tile_y = (TILES_DOWN - 1) - (tile_idx // TILES_ACROSS)
    px = tile_x * TILE_RES + x
    py = tile_y * TILE_RES + y
    buf[py * ATLAS_RES + px] = color

def hash2d(x, y, seed=0):
    n = (x * 374761393 + y * 668265263 + seed * 91234567) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return (n ^ (n >> 16)) & 0xFFFFFFFF

# ---------------------------------------------------------------------------
# Individual Block Painters
# ---------------------------------------------------------------------------

def paint_dirt_pixels(tile_idx, diff):
    # Palette
    d_dark = (61, 36, 20, 255)
    d_base = (89, 56, 34, 255)
    d_mid  = (110, 70, 42, 255)
    d_light= (133, 87, 52, 255)
    peb_dark = (66, 62, 59, 255)
    peb_mid  = (122, 117, 111, 255)
    peb_high = (166, 160, 153, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 101) % 100
            if h < 25: c = d_dark
            elif h < 60: c = d_base
            elif h < 85: c = d_mid
            else: c = d_light
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Embedded river pebbles
    # Pebble 1 (x: 3..5, y: 4..5)
    pebble1 = [
        (3, 4, peb_dark), (4, 4, peb_mid), (5, 4, peb_dark),
        (3, 5, peb_mid), (4, 5, peb_high), (5, 5, peb_mid)
    ]
    for px, py, pc in pebble1:
        set_atlas_pixel(diff, tile_idx, px, py, pc)

    # Pebble 2 (x: 10..12, y: 10..11)
    pebble2 = [
        (10, 10, peb_dark), (11, 10, peb_mid), (12, 10, peb_dark),
        (10, 11, peb_mid), (11, 11, peb_high), (12, 11, peb_mid)
    ]
    for px, py, pc in pebble2:
        set_atlas_pixel(diff, tile_idx, px, py, pc)

    # Pebble 3 (x: 7..8, y: 1..2)
    pebble3 = [
        (7, 1, peb_dark), (8, 1, peb_dark),
        (7, 2, peb_mid), (8, 2, peb_high)
    ]
    for px, py, pc in pebble3:
        set_atlas_pixel(diff, tile_idx, px, py, pc)

def paint_grass_side(tile_idx, diff):
    # Dirt base
    paint_dirt_pixels(tile_idx, diff)

    # Grass fringe palette
    g_tip   = (167, 244, 50, 255)
    g_light = (110, 204, 38, 255)
    g_mid   = (70, 158, 28, 255)
    g_dark  = (38, 97, 16, 255)
    g_shade = (46, 28, 16, 255) # Shadow cast on dirt

    blade_depths = [12, 10, 12, 11, 9, 11, 12, 10, 12, 11, 9, 11, 10, 12, 11, 12]

    for x in range(16):
        d = blade_depths[x]
        # Cast shadow on dirt under blade tip
        if d > 0:
            set_atlas_pixel(diff, tile_idx, x, d - 1, g_shade)
        for y in range(d, 16):
            if y == 15:
                c = g_tip if (x % 3 == 0) else g_light
            elif y >= 13:
                c = g_light if ((x + y) % 2 == 0) else g_mid
            elif y >= 11:
                c = g_mid if ((x ^ y) % 2 == 0) else g_dark
            else:
                c = g_dark
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_grass_top(tile_idx, diff):
    g1 = (46, 125, 20, 255)
    g2 = (65, 156, 28, 255)
    g3 = (92, 184, 46, 255)
    g4 = (130, 214, 56, 255)
    flower_y = (255, 235, 59, 255)
    flower_w = (255, 255, 255, 255)
    flower_c = (255, 152, 0, 255)
    clover   = (160, 240, 60, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 202) % 100
            if h < 20: c = g1
            elif h < 60: c = g2
            elif h < 85: c = g3
            else: c = g4
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Clover patch (x: 4..5, y: 5..6)
    set_atlas_pixel(diff, tile_idx, 4, 5, clover)
    set_atlas_pixel(diff, tile_idx, 5, 5, clover)
    set_atlas_pixel(diff, tile_idx, 4, 6, clover)

    # Yellow buttercup
    set_atlas_pixel(diff, tile_idx, 10, 11, flower_c)
    set_atlas_pixel(diff, tile_idx, 9, 11, flower_y)
    set_atlas_pixel(diff, tile_idx, 11, 11, flower_y)
    set_atlas_pixel(diff, tile_idx, 10, 12, flower_y)
    set_atlas_pixel(diff, tile_idx, 10, 10, flower_y)

    # White daisy
    set_atlas_pixel(diff, tile_idx, 13, 3, flower_c)
    set_atlas_pixel(diff, tile_idx, 12, 3, flower_w)
    set_atlas_pixel(diff, tile_idx, 14, 3, flower_w)
    set_atlas_pixel(diff, tile_idx, 13, 4, flower_w)
    set_atlas_pixel(diff, tile_idx, 13, 2, flower_w)

def paint_stone_pixels(tile_idx, diff):
    grout = (38, 41, 46, 255)
    base1 = (95, 100, 107, 255)
    base2 = (115, 121, 129, 255)
    base3 = (135, 142, 151, 255)
    bevel_high = (180, 188, 198, 255)
    bevel_dark = (60, 64, 70, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 303) % 100
            c = base1 if h < 35 else (base2 if h < 75 else base3)
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Chiseled cobble / brick lines
    # Horizontal seam at y = 8
    for x in range(16):
        set_atlas_pixel(diff, tile_idx, x, 8, grout)
        set_atlas_pixel(diff, tile_idx, x, 9, bevel_high)
        set_atlas_pixel(diff, tile_idx, x, 7, bevel_dark)

    # Upper vertical seam at x = 8, y = 9..15
    for y in range(9, 16):
        set_atlas_pixel(diff, tile_idx, 8, y, grout)
        set_atlas_pixel(diff, tile_idx, 9, y, bevel_high)
        set_atlas_pixel(diff, tile_idx, 7, y, bevel_dark)

    # Lower vertical seam at x = 12, y = 0..7
    for y in range(0, 8):
        set_atlas_pixel(diff, tile_idx, 12, y, grout)
        set_atlas_pixel(diff, tile_idx, 13, y, bevel_high)
        set_atlas_pixel(diff, tile_idx, 11, y, bevel_dark)

    # Diagonal fracture in stone 1
    cracks = [(2, 2), (3, 3), (4, 4), (5, 4), (6, 5)]
    for cx, cy in cracks:
        set_atlas_pixel(diff, tile_idx, cx, cy, grout)
        set_atlas_pixel(diff, tile_idx, cx, cy + 1, bevel_high)

def paint_wood_side(tile_idx, diff):
    bark_darkest = (32, 19, 11, 255)
    bark_furrow  = (49, 30, 19, 255)
    bark_mid     = (94, 59, 35, 255)
    bark_light   = (138, 87, 52, 255)
    bark_high    = (168, 109, 67, 255)

    for y in range(16):
        for x in range(16):
            # Vertical grain with wander
            fissure1 = 3 + (1 if 4 <= y <= 10 else 0)
            fissure2 = 8 + (1 if y >= 11 or y <= 3 else 0)
            fissure3 = 13 + (1 if 6 <= y <= 12 else 0)

            if x == fissure1 or x == fissure2 or x == fissure3:
                c = bark_darkest if (y % 2 == 0) else bark_furrow
            elif x == fissure1 + 1 or x == fissure2 + 1 or x == fissure3 + 1:
                c = bark_high if (y % 3 == 0) else bark_light
            elif x == fissure1 - 1 or x == fissure2 - 1 or x == fissure3 - 1:
                c = bark_furrow
            else:
                h = hash2d(x, y, 404) % 100
                c = bark_mid if h < 60 else bark_light
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Knothole at x=10, y=6
    knot = [
        (9, 6, bark_furrow), (10, 6, bark_darkest), (11, 6, bark_furrow),
        (9, 7, bark_light),  (10, 7, bark_high),    (11, 7, bark_light)
    ]
    for kx, ky, kc in knot:
        set_atlas_pixel(diff, tile_idx, kx, ky, kc)

def paint_wood_top(tile_idx, diff):
    bark_rim   = (49, 30, 19, 255)
    bark_mid   = (94, 59, 35, 255)
    sap_light  = (215, 181, 135, 255)
    sap_mid    = (194, 158, 112, 255)
    ring_dark  = (153, 115, 72, 255)
    pith_core  = (112, 75, 40, 255)
    check_line = (32, 19, 11, 255)

    cx, cy = 7.5, 7.5
    for y in range(16):
        for x in range(16):
            # Outer bark border (1px)
            if x == 0 or x == 15 or y == 0 or y == 15:
                c = bark_rim if (x + y) % 2 == 0 else bark_mid
            else:
                dist = math.sqrt((x - cx)**2 + (y - cy)**2)
                if dist < 1.8:
                    c = pith_core
                elif 3.0 <= dist <= 3.8:
                    c = ring_dark
                elif 5.2 <= dist <= 6.0:
                    c = ring_dark
                else:
                    c = sap_light if (x + y) % 2 == 0 else sap_mid
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Radial heartwood check crack
    radial = [(8, 8), (9, 9), (10, 10), (11, 11), (12, 12)]
    for rx, ry in radial:
        set_atlas_pixel(diff, tile_idx, rx, ry, check_line)

def paint_leaves_pixels(tile_idx, diff):
    l_high = (141, 227, 68, 255)
    l_mid  = (78, 168, 44, 255)
    l_dark = (39, 97, 24, 255)
    l_deep = (22, 61, 12, 255)
    transparent = (0, 0, 0, 0)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 505) % 100
            if h < 25: c = l_deep
            elif h < 60: c = l_dark
            elif h < 85: c = l_mid
            else: c = l_high
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Cutout transparent holes for pixel-art canopy volume
    cutouts = [
        (0, 0), (15, 0), (0, 15), (15, 15),
        (1, 0), (0, 1), (14, 15), (15, 14),
        (7, 7), (8, 7), (7, 8),
        (3, 11), (4, 11), (12, 4), (13, 4)
    ]
    for cx, cy in cutouts:
        set_atlas_pixel(diff, tile_idx, cx, cy, transparent)

def paint_sand_pixels(tile_idx, diff):
    s_crest  = (255, 245, 165, 255)
    s_light  = (245, 220, 125, 255)
    s_mid    = (225, 190, 95, 255)
    s_trough = (195, 155, 65, 255)
    s_glint  = (255, 255, 220, 255)

    for y in range(16):
        for x in range(16):
            # Smooth horizontal wind ripple waves
            wave = math.sin(x * 0.45 + (y // 4) * 1.5) * 1.2
            val = (y + wave) % 4.0
            if val < 0.8: c = s_crest
            elif val < 2.0: c = s_light
            elif val < 3.2: c = s_mid
            else: c = s_trough

            if hash2d(x, y, 606) % 23 == 0:
                c = s_glint
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_sandstone_pixels(tile_idx, diff):
    seam = (115, 62, 31, 255)
    b0_dark  = (140, 82, 50, 255)
    b0_light = (165, 98, 62, 255)
    b1_dark  = (205, 155, 80, 255)
    b1_light = (225, 175, 95, 255)
    b2_dark  = (230, 210, 155, 255)
    b2_light = (245, 228, 180, 255)
    b3_dark  = (186, 117, 76, 255)
    b3_light = (206, 133, 90, 255)

    for y in range(16):
        # 4 distinct geological strata bands
        if y == 3 or y == 7 or y == 11 or y == 15:
            for x in range(16):
                set_atlas_pixel(diff, tile_idx, x, y, seam)
            continue

        if y < 3:
            cd, cl = b0_dark, b0_light
        elif y < 7:
            cd, cl = b1_dark, b1_light
        elif y < 11:
            cd, cl = b2_dark, b2_light
        else:
            cd, cl = b3_dark, b3_light

        for x in range(16):
            h = hash2d(x, y, 707) % 100
            c = cd if h < 55 else cl
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_cactus_side(tile_idx, diff):
    c_furrow = (27, 88, 18, 255)
    c_flank  = (50, 135, 35, 255)
    c_crest  = (82, 192, 58, 255)
    c_spine  = (255, 255, 230, 255)
    c_spbase = (255, 238, 88, 255)

    for y in range(16):
        for x in range(16):
            col = x % 4
            if col == 0:   c = c_furrow
            elif col == 1: c = c_crest
            elif col == 2: c = c_flank
            else:          c = c_furrow
            set_atlas_pixel(diff, tile_idx, x, y, c)

            # Spines at intervals along crest
            if col == 1 and (y % 4 == 0):
                set_atlas_pixel(diff, tile_idx, x, y, c_spbase)
                # Left/Right needle tips
                if x > 0: set_atlas_pixel(diff, tile_idx, x - 1, y, c_spine)
            elif col == 3 and (y % 4 == 2):
                set_atlas_pixel(diff, tile_idx, x, y, c_spbase)
                if x + 1 < 16: set_atlas_pixel(diff, tile_idx, x + 1, y, c_spine)

def paint_cactus_top(tile_idx, diff):
    c_furrow = (27, 88, 18, 255)
    c_flank  = (50, 135, 35, 255)
    c_crest  = (82, 192, 58, 255)
    c_center = (255, 238, 88, 255)
    c_spine  = (255, 255, 230, 255)

    cx, cy = 7.5, 7.5
    for y in range(16):
        for x in range(16):
            dx, dy = x - cx, y - cy
            angle = math.atan2(dy, dx)
            dist = math.sqrt(dx*dx + dy*dy)
            if dist < 2.0:
                c = c_center
            else:
                # 8 radial ribs
                rib = int((angle + math.pi) / (math.pi / 4.0) + 0.5) % 8
                rib_pos = abs(((angle + math.pi) / (math.pi / 4.0)) - rib)
                if rib_pos < 0.15:
                    c = c_crest
                elif rib_pos < 0.35:
                    c = c_flank
                else:
                    c = c_furrow
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Spine dots around crown
    crown_spines = [(7, 7), (8, 7), (7, 8), (8, 8), (4, 4), (11, 4), (4, 11), (11, 11)]
    for sx, sy in crown_spines:
        set_atlas_pixel(diff, tile_idx, sx, sy, c_spine)

def paint_snow_pixels(tile_idx, diff):
    s_pure   = (255, 255, 255, 255)
    s_light  = (240, 248, 255, 255)
    s_shadow = (205, 228, 245, 255)
    s_deep   = (168, 202, 226, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 808) % 100
            # Undulating snow mounds with soft blue hollows
            wave = math.sin(x * 0.4) * 1.5 + math.cos(y * 0.5) * 1.0
            if wave > 1.2:
                c = s_pure if h < 60 else s_light
            elif wave > -0.5:
                c = s_light if h < 70 else s_shadow
            else:
                c = s_shadow if h < 60 else s_deep
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_ice_pixels(tile_idx, diff):
    ice_deep  = (38, 140, 166, 220)
    ice_mid   = (68, 176, 204, 230)
    ice_light = (120, 215, 235, 240)
    ice_crack = (255, 255, 255, 255)
    ice_halo  = (210, 250, 255, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 909) % 100
            c = ice_deep if h < 30 else (ice_mid if h < 75 else ice_light)
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Sharp internal crystal fractures
    cracks = [
        (2, 2), (3, 3), (4, 4), (5, 6), (6, 7), (7, 8), (8, 9),
        (9, 10), (10, 12), (11, 13), (12, 14), (13, 14),
        # Branch
        (8, 9), (9, 8), (10, 7), (11, 7), (12, 6)
    ]
    for cx, cy in cracks:
        set_atlas_pixel(diff, tile_idx, cx, cy, ice_crack)
        if cx + 1 < 16: set_atlas_pixel(diff, tile_idx, cx + 1, cy, ice_halo)

def paint_clay_pixels(tile_idx, diff):
    c1 = (125, 104, 96, 255)
    c2 = (145, 124, 116, 255)
    c3 = (168, 145, 136, 255)
    c4 = (188, 166, 156, 255)

    for y in range(16):
        for x in range(16):
            # Smooth plastic silt swirls
            swirl = int(math.sin(x * 0.4 + y * 0.3) * 2.5 + (x + y * 2) * 0.25) % 4
            c = [c1, c2, c3, c4][swirl]
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_ore_pixels(tile_idx, diff, emit, gem_palette, is_emissive=False):
    # First paint stone foundation
    paint_stone_pixels(tile_idx, diff)

    # Gem clusters: list of (x, y, color_index)
    # palette: [shadow, base, light, star_glint]
    clusters = [
        # Cluster 1 (upper left, x: 3..6, y: 10..13)
        (3, 11, 0), (4, 10, 0), (5, 10, 0), (6, 11, 0),
        (4, 11, 1), (5, 11, 1), (4, 12, 2), (5, 12, 1),
        (4, 12, 3), # Specular star glint!
        # Cluster 2 (bottom right, x: 9..13, y: 3..6)
        (9, 4, 0), (10, 3, 0), (11, 3, 0), (12, 4, 0), (13, 5, 0),
        (10, 4, 1), (11, 4, 1), (12, 5, 1), (11, 5, 2),
        (11, 5, 3), # Specular star glint!
        # Cluster 3 (center small, x: 7..8, y: 6..7)
        (7, 6, 0), (8, 6, 1), (7, 7, 2), (8, 7, 3)
    ]

    for gx, gy, c_idx in clusters:
        c = gem_palette[c_idx]
        set_atlas_pixel(diff, tile_idx, gx, gy, c)
        if is_emissive and c_idx >= 1:
            set_atlas_pixel(emit, tile_idx, gx, gy, c)

def paint_brick_pixels(tile_idx, diff):
    mortar_dark  = (133, 138, 145, 255)
    mortar_light = (209, 213, 219, 255)
    b_dark   = (145, 39, 20, 255)
    b_mid    = (184, 58, 35, 255)
    b_light  = (217, 90, 65, 255)
    b_bevel  = (240, 120, 95, 255)

    for y in range(16):
        is_h_mortar = (y % 4 == 0)
        row = y // 4
        x_shift = 0 if (row % 2 == 0) else 4

        for x in range(16):
            is_v_mortar = ((x + x_shift) % 8 == 0)
            if is_h_mortar or is_v_mortar:
                c = mortar_dark if (x + y) % 2 == 0 else mortar_light
            else:
                # Brick interior with top-left bevel highlight
                local_y = y % 4
                local_x = (x + x_shift) % 8
                if local_y == 3 or local_x == 1:
                    c = b_bevel
                else:
                    h = hash2d(x, y, 1111) % 100
                    c = b_dark if h < 30 else (b_mid if h < 75 else b_light)
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_wood_plank_pixels(tile_idx, diff):
    groove    = (46, 24, 11, 255)
    p_dark    = (140, 91, 48, 255)
    p_mid     = (168, 114, 62, 255)
    p_light   = (194, 138, 82, 255)
    p_bevel   = (215, 160, 105, 255)
    nail_dark = (48, 48, 48, 255)
    nail_high = (160, 160, 160, 255)

    for y in range(16):
        is_groove = (y % 4 == 0)
        plank_row = y // 4
        for x in range(16):
            if is_groove:
                set_atlas_pixel(diff, tile_idx, x, y, groove)
            else:
                local_y = y % 4
                if local_y == 3:
                    c = p_bevel
                else:
                    h = hash2d(x, y, 1212) % 100
                    c = p_dark if h < 35 else (p_mid if h < 80 else p_light)
                set_atlas_pixel(diff, tile_idx, x, y, c)

    # Countersunk nail heads on planks
    nails = [
        (1, 2), (14, 2),
        (2, 6), (13, 6),
        (1, 10), (14, 10),
        (2, 14), (13, 14)
    ]
    for nx, ny in nails:
        set_atlas_pixel(diff, tile_idx, nx, ny, nail_dark)
        set_atlas_pixel(diff, tile_idx, nx, ny + 1, nail_high)

def paint_glass_pixels(tile_idx, diff):
    border = (75, 85, 99, 255)
    tint   = (195, 235, 247, 55)
    streak1= (255, 255, 255, 195)
    streak2= (255, 255, 255, 135)

    for y in range(16):
        for x in range(16):
            # 1px leaded outer border
            if x == 0 or x == 15 or y == 0 or y == 15:
                set_atlas_pixel(diff, tile_idx, x, y, border)
            # Diagonal specular reflection streaks
            elif (x + y == 17) or (x + y == 18 and 4 <= x <= 12):
                set_atlas_pixel(diff, tile_idx, x, y, streak1)
            elif (x + y == 9 and 2 <= x <= 6) or (x + y == 10 and 3 <= x <= 5):
                set_atlas_pixel(diff, tile_idx, x, y, streak2)
            else:
                set_atlas_pixel(diff, tile_idx, x, y, tint)

def paint_torch_pixels(tile_idx, diff, emit):
    transparent = (0, 0, 0, 0)
    w_dark  = (54, 27, 12, 255)
    w_core  = (117, 68, 33, 255)
    w_light = (148, 89, 45, 255)
    charred = (36, 32, 28, 255)

    f_red   = (211, 47, 47, 255)
    f_org   = (255, 109, 0, 255)
    f_yel   = (255, 214, 0, 255)
    f_white = (255, 255, 255, 255)

    for y in range(16):
        for x in range(16):
            set_atlas_pixel(diff, tile_idx, x, y, transparent)

    # Wooden handle (y=1..8)
    for y in range(1, 8):
        set_atlas_pixel(diff, tile_idx, 6, y, w_dark)
        set_atlas_pixel(diff, tile_idx, 7, y, w_core)
        set_atlas_pixel(diff, tile_idx, 8, y, w_light)
        set_atlas_pixel(diff, tile_idx, 9, y, w_dark)

    # Charred head (y=8..9)
    for x in range(6, 10):
        set_atlas_pixel(diff, tile_idx, x, 8, charred)

    # Incandescent flame teardrop (y=9..15)
    flame_pixels = [
        # y=9
        (7, 9, f_red), (8, 9, f_red),
        # y=10
        (6, 10, f_red), (7, 10, f_org), (8, 10, f_org), (9, 10, f_red),
        # y=11
        (5, 11, f_red), (6, 11, f_org), (7, 11, f_yel), (8, 11, f_yel), (9, 11, f_org), (10, 11, f_red),
        # y=12
        (5, 12, f_red), (6, 12, f_yel), (7, 12, f_white), (8, 12, f_white), (9, 12, f_yel), (10, 12, f_red),
        # y=13
        (6, 13, f_red), (7, 13, f_yel), (8, 13, f_yel), (9, 13, f_red),
        # y=14
        (7, 14, f_org), (8, 14, f_org),
        # y=15
        (7, 15, f_red)
    ]
    for fx, fy, fc in flame_pixels:
        set_atlas_pixel(diff, tile_idx, fx, fy, fc)
        set_atlas_pixel(emit, tile_idx, fx, fy, fc)

def paint_obsidian_pixels(tile_idx, diff):
    o_black  = (18, 9, 28, 255)
    o_dark   = (28, 15, 42, 255)
    o_mid    = (48, 24, 72, 255)
    o_purple = (104, 36, 140, 255)
    o_violet = (156, 56, 199, 255)
    o_glint  = (241, 225, 255, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 1313) % 100
            c = o_black if h < 45 else (o_dark if h < 85 else o_mid)
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Conchoidal fracture arcs
    arcs = [
        (2, 4), (3, 5), (4, 6), (5, 8), (6, 9), (7, 10), (8, 12), (9, 13),
        (8, 3), (9, 4), (10, 5), (11, 7), (12, 8), (13, 10)
    ]
    for ax, ay in arcs:
        set_atlas_pixel(diff, tile_idx, ax, ay, o_purple)
        if ax + 1 < 16: set_atlas_pixel(diff, tile_idx, ax + 1, ay, o_violet)

    # Specular razor glints
    set_atlas_pixel(diff, tile_idx, 6, 9, o_glint)
    set_atlas_pixel(diff, tile_idx, 11, 7, o_glint)

def paint_magma_pixels(tile_idx, diff, emit):
    crust_dark = (23, 21, 21, 255)
    crust_mid  = (43, 40, 40, 255)
    crust_high = (65, 60, 60, 255)

    l_red    = (191, 21, 0, 255)
    l_orange = (255, 109, 0, 255)
    l_yellow = (255, 214, 0, 255)
    l_core   = (255, 249, 196, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 1414) % 100
            c = crust_dark if h < 40 else (crust_mid if h < 85 else crust_high)
            set_atlas_pixel(diff, tile_idx, x, y, c)

    # Spidering molten lava fissure network
    fissures = [
        # Main vein
        (0, 3, l_red), (1, 4, l_orange), (2, 4, l_yellow), (3, 5, l_core),
        (4, 5, l_yellow), (5, 6, l_orange), (6, 7, l_core), (7, 7, l_yellow),
        (8, 8, l_core), (9, 8, l_yellow), (10, 9, l_orange), (11, 10, l_core),
        (12, 11, l_yellow), (13, 12, l_orange), (14, 13, l_red), (15, 14, l_red),
        # Branch vein
        (8, 7, l_orange), (8, 6, l_yellow), (9, 5, l_core), (10, 4, l_yellow),
        (11, 3, l_orange), (12, 2, l_red),
        # North branch
        (6, 8, l_orange), (6, 9, l_yellow), (5, 10, l_core), (5, 11, l_yellow),
        (4, 12, l_orange), (3, 13, l_red)
    ]
    for fx, fy, fc in fissures:
        set_atlas_pixel(diff, tile_idx, fx, fy, fc)
        set_atlas_pixel(emit, tile_idx, fx, fy, fc)

def paint_mossy_stone_pixels(tile_idx, diff):
    paint_stone_pixels(tile_idx, diff)
    m_dark  = (46, 82, 16, 255)
    m_mid   = (78, 140, 34, 255)
    m_light = (128, 196, 51, 255)

    for y in range(16):
        for x in range(16):
            # Overgrown moss clusters
            if (x < 9 and y > 6) or (x > 8 and y < 8) or (x % 3 == 0 and y % 2 == 0):
                h = hash2d(x, y, 1515) % 100
                if h < 30: c = m_dark
                elif h < 75: c = m_mid
                elif h < 95: c = m_light
                else: continue
                set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_mud_pixels(tile_idx, diff):
    m_peat   = (44, 26, 14, 255)
    m_muck   = (61, 38, 22, 255)
    m_loam   = (77, 49, 30, 255)
    m_puddle = (99, 67, 43, 255)
    m_sheen  = (140, 104, 74, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 1616) % 100
            if h < 35: c = m_peat
            elif h < 75: c = m_muck
            else: c = m_loam
            # Wet puddles
            if (3 <= x <= 7 and 4 <= y <= 7) or (10 <= x <= 13 and 9 <= y <= 12):
                c = m_sheen if (x == 4 and y == 5) or (x == 11 and y == 10) else m_puddle
            set_atlas_pixel(diff, tile_idx, x, y, c)

def paint_glowshroom_pixels(tile_idx, diff, emit):
    transparent = (0, 0, 0, 0)
    stem_dark  = (40, 120, 140, 255)
    stem_light = (84, 197, 217, 255)
    cap_deep   = (0, 107, 133, 255)
    cap_bright = (0, 229, 255, 255)
    cap_spot   = (224, 255, 255, 255)

    for y in range(16):
        for x in range(16):
            set_atlas_pixel(diff, tile_idx, x, y, transparent)

    # Translucent stalk (y=1..6, x=7..8)
    for y in range(1, 7):
        set_atlas_pixel(diff, tile_idx, 6, y, stem_dark)
        set_atlas_pixel(diff, tile_idx, 7, y, stem_light)
        set_atlas_pixel(diff, tile_idx, 8, y, stem_light)
        set_atlas_pixel(diff, tile_idx, 9, y, stem_dark)

    # Glowing mushroom umbrella dome (y=7..12)
    for y in range(7, 13):
        for x in range(2, 14):
            # Curved mushroom cap geometry
            if y == 7 and (x < 3 or x > 12): continue
            if y == 11 and (x < 3 or x > 12): continue
            if y == 12 and (x < 5 or x > 10): continue

            c = cap_deep if (y == 7 or y == 8) else cap_bright
            set_atlas_pixel(diff, tile_idx, x, y, c)
            set_atlas_pixel(emit, tile_idx, x, y, c)

    # Bioluminescent cap spots
    spots = [(5, 10), (7, 11), (10, 10), (4, 8), (11, 8)]
    for sx, sy in spots:
        set_atlas_pixel(diff, tile_idx, sx, sy, cap_spot)
        set_atlas_pixel(emit, tile_idx, sx, sy, cap_spot)

    # Floating bioluminescent spores drifting above
    spores = [(3, 14), (6, 15), (11, 14), (13, 13)]
    for sx, sy in spores:
        set_atlas_pixel(diff, tile_idx, sx, sy, cap_spot)
        set_atlas_pixel(emit, tile_idx, sx, sy, cap_spot)

def paint_bedrock_pixels(tile_idx, diff):
    b_void    = (8, 9, 10, 255)
    b_dark    = (20, 22, 24, 255)
    b_mineral = (34, 37, 42, 255)
    b_facet   = (56, 60, 68, 255)

    for y in range(16):
        for x in range(16):
            h = hash2d(x, y, 1717) % 100
            c = b_void if h < 35 else (b_dark if h < 75 else (b_mineral if h < 93 else b_facet))
            set_atlas_pixel(diff, tile_idx, x, y, c)

# ---------------------------------------------------------------------------
# Master Atlas Assembler
# ---------------------------------------------------------------------------

def generate_all():
    diff, emit = create_buffers()

    # Tile 0: Air (Transparent)
    # Tile 1: Grass Side
    paint_grass_side(1, diff)
    # Tile 2: Dirt
    paint_dirt_pixels(2, diff)
    # Tile 3: Stone
    paint_stone_pixels(3, diff)
    # Tile 4: Wood Log Side
    paint_wood_side(4, diff)
    # Tile 5: Leaves
    paint_leaves_pixels(5, diff)
    # Tile 6: Sand
    paint_sand_pixels(6, diff)
    # Tile 7: Sandstone
    paint_sandstone_pixels(7, diff)
    # Tile 8: Cactus Side
    paint_cactus_side(8, diff)
    # Tile 9: Snow
    paint_snow_pixels(9, diff)
    # Tile 10: Ice
    paint_ice_pixels(10, diff)
    # Tile 11: Clay
    paint_clay_pixels(11, diff)

    # Tile 12: Coal Ore
    coal_pal = [
        (24, 25, 28, 255), (49, 52, 59, 255), (86, 91, 99, 255), (240, 242, 245, 255)
    ]
    paint_ore_pixels(12, diff, emit, coal_pal, is_emissive=False)

    # Tile 13: Iron Ore
    iron_pal = [
        (110, 61, 29, 255), (170, 104, 56, 255), (212, 149, 99, 255), (255, 255, 255, 255)
    ]
    paint_ore_pixels(13, diff, emit, iron_pal, is_emissive=False)

    # Tile 14: Gold Ore
    gold_pal = [
        (158, 107, 5, 255), (240, 184, 12, 255), (255, 222, 67, 255), (255, 255, 255, 255)
    ]
    paint_ore_pixels(14, diff, emit, gold_pal, is_emissive=False)

    # Tile 15: Diamond Ore
    dia_pal = [
        (0, 119, 138, 255), (0, 212, 235, 255), (122, 233, 247, 255), (255, 255, 255, 255)
    ]
    paint_ore_pixels(15, diff, emit, dia_pal, is_emissive=True)

    # Tile 16: Ruby Ore
    ruby_pal = [
        (89, 2, 18, 255), (196, 8, 40, 255), (255, 43, 79, 255), (255, 255, 255, 255)
    ]
    paint_ore_pixels(16, diff, emit, ruby_pal, is_emissive=True)

    # Tile 17: Brick
    paint_brick_pixels(17, diff)
    # Tile 18: Wood Plank
    paint_wood_plank_pixels(18, diff)
    # Tile 19: Glass
    paint_glass_pixels(19, diff)
    # Tile 20: Torch
    paint_torch_pixels(20, diff, emit)
    # Tile 21: Obsidian
    paint_obsidian_pixels(21, diff)
    # Tile 22: Magma
    paint_magma_pixels(22, diff, emit)
    # Tile 23: Mossy Stone
    paint_mossy_stone_pixels(23, diff)
    # Tile 24: Mud
    paint_mud_pixels(24, diff)
    # Tile 25: GlowShroom
    paint_glowshroom_pixels(25, diff, emit)
    # Tile 26: Bedrock
    paint_bedrock_pixels(26, diff)

    # Special Extra Tiles for 2.5D visual perfection
    # Tile 27: Grass Top
    paint_grass_top(27, diff)
    # Tile 28: Wood Top Rings
    paint_wood_top(28, diff)
    # Tile 29: Cactus Top
    paint_cactus_top(29, diff)

    return diff, emit

if __name__ == "__main__":
    os.makedirs("Assets/Resources", exist_ok=True)
    diff, emit = generate_all()

    # Save PNGs using PIL
    img_diff = Image.new("RGBA", (ATLAS_RES, ATLAS_RES))
    img_diff.putdata(diff)
    img_diff.save("Assets/Resources/VoxelAtlas.png")

    img_emit = Image.new("RGBA", (ATLAS_RES, ATLAS_RES))
    img_emit.putdata(emit)
    img_emit.save("Assets/Resources/VoxelEmission.png")

    print("[OK] Successfully generated Assets/Resources/VoxelAtlas.png and VoxelEmission.png")
