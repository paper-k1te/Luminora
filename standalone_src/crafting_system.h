#ifndef CRAFTING_SYSTEM_H
#define CRAFTING_SYSTEM_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "localization.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Recipe Database & Structs
// ============================================================================
#define MAX_INGREDIENTS 3
#define RECIPE_COUNT 29
#define BACKPACK_SLOTS 27

typedef struct {
    int type;
    int count;
} RecipeIngredient;

typedef struct {
    int id;
    const char* name;
    const char* chineseName;
    const char* category; // "TOOLS", "WEAPONS", "BLOCKS", "SUPPLIES", "MATERIALS"
    const char* description;
    const char* chineseDescription;
    int outputType;
    int outputCount;
    int ingredientCount;
    RecipeIngredient ingredients[MAX_INGREDIENTS];
} RecipeDef;

// Luminora Comprehensive Recipe Catalog
static RecipeDef g_Recipes[RECIPE_COUNT] = {
    // 1. 1 Wood -> 4 Wood Planks (木板)
    { 1, "Wood Planks", "木板", "BLOCKS",
      "Cut raw logs into sturdy lumber planks for building and carpentry.",
      "将原木切割成坚固的建筑与木工木板。",
      BLOCK_WOOD_PLANK, 4, 1, { { BLOCK_WOOD, 1 } } },

    // 2. 2 Wood Planks -> 4 Sticks (木棍)
    { 2, "Sticks", "木棍", "MATERIALS",
      "Slender shafts of wood, indispensable for tool handles and torches.",
      "细木棍，工具手柄和火把不可或缺的材料。",
      ITEM_STICK, 4, 1, { { BLOCK_WOOD_PLANK, 2 } } },

    // 3. 3 Wood Planks + 2 Sticks -> Wooden Pickaxe (木镐)
    { 3, "Wooden Pickaxe", "木镐", "TOOLS",
      "Basic entry-level mining pickaxe. Can mine stone and coal ores.",
      "基础采矿镐，可开采岩石与煤矿。",
      ITEM_TOOL_WOOD_PICKAXE, 1, 2, { { BLOCK_WOOD_PLANK, 3 }, { ITEM_STICK, 2 } } },

    // 4. 3 Wood Planks + 2 Sticks -> Wooden Axe (木斧)
    { 4, "Wooden Axe", "木斧", "TOOLS",
      "Standard timber axe. Accelerates tree chopping and wood collection.",
      "标准伐木斧，加速伐木与木材采集。",
      ITEM_TOOL_WOOD_AXE, 1, 2, { { BLOCK_WOOD_PLANK, 3 }, { ITEM_STICK, 2 } } },

    // 5. 2 Wood Planks + 1 Stick -> Wooden Sword (木剑)
    { 5, "Wooden Sword", "木剑", "WEAPONS",
      "Light defensive sidearm. Deals moderate knockback to slimes and zombies.",
      "轻型防卫近战剑，提供基础击退效果。",
      ITEM_TOOL_WOOD_SWORD, 1, 2, { { BLOCK_WOOD_PLANK, 2 }, { ITEM_STICK, 1 } } },

    // 6. 1 Wood Plank + 2 Sticks -> Wooden Shovel (木铲)
    { 6, "Wooden Shovel", "木铲", "TOOLS",
      "Excavation spade. Digs dirt, sand, gravel and snow effortlessly.",
      "挖掘铲，轻松挖掘泥土、沙子与雪块。",
      ITEM_TOOL_WOOD_SHOVEL, 1, 2, { { BLOCK_WOOD_PLANK, 1 }, { ITEM_STICK, 2 } } },

    // 7. 3 Stone + 2 Sticks -> Stone Pickaxe (石镐)
    { 7, "Stone Pickaxe", "石镐", "TOOLS",
      "Sturdy stone-headed pickaxe. Mines iron, coal and sandstone swiftly.",
      "坚固的石镐，可快速开采铁矿与砂岩。",
      ITEM_TOOL_STONE_PICKAXE, 1, 2, { { BLOCK_STONE, 3 }, { ITEM_STICK, 2 } } },

    // 8. 3 Stone + 2 Sticks -> Stone Axe (石斧)
    { 8, "Stone Axe", "石斧", "TOOLS",
      "Reinforced flint-edged woodcutter axe.",
      "强化石刃伐木斧，大幅提升砍树效率。",
      ITEM_TOOL_STONE_AXE, 1, 2, { { BLOCK_STONE, 3 }, { ITEM_STICK, 2 } } },

    // 9. 2 Stone + 1 Stick -> Stone Sword (石剑)
    { 9, "Stone Sword", "石剑", "WEAPONS",
      "Sharpened broadsword carved from subterranean granite.",
      "深层花岗岩精制石剑，具备更高伤害与击退。",
      ITEM_TOOL_STONE_SWORD, 1, 2, { { BLOCK_STONE, 2 }, { ITEM_STICK, 1 } } },

    // 10. 3 Iron Ore/Ingot + 2 Sticks -> Iron Pickaxe (铁镐)
    { 10, "Iron Pickaxe", "铁镐", "TOOLS",
      "High-durability forged iron pickaxe. Capable of cleaving gold and diamond.",
      "高耐用度精锻铁镐，可开采金矿与钻石。",
      ITEM_TOOL_IRON_PICKAXE, 1, 2, { { BLOCK_IRON_ORE, 3 }, { ITEM_STICK, 2 } } },

    // 11. 3 Iron Ore/Ingot + 2 Sticks -> Iron Axe (铁斧)
    { 11, "Iron Axe", "铁斧", "TOOLS",
      "Heavy iron battle axe and high-efficiency logging implement.",
      "重型铁斧，兼具伐木与战斗威力。",
      ITEM_TOOL_IRON_AXE, 1, 2, { { BLOCK_IRON_ORE, 3 }, { ITEM_STICK, 2 } } },

    // 12. 2 Iron Ore/Ingot + 1 Stick -> Iron Sword (铁剑)
    { 12, "Iron Sword", "铁剑", "WEAPONS",
      "Knightly iron longsword with lethal cutting edge.",
      "锋利的骑士铁长剑，造成致命挥砍伤害。",
      ITEM_TOOL_IRON_SWORD, 1, 2, { { BLOCK_IRON_ORE, 2 }, { ITEM_STICK, 1 } } },

    // 13. 3 Diamond + 2 Sticks -> Diamond Pickaxe (钻石镐)
    { 13, "Diamond Pickaxe", "钻石镐", "TOOLS",
      "Legendary master pickaxe infused with crystallized diamond hardness.",
      "极品钻石神镐，极致开采速度。",
      ITEM_TOOL_DIAMOND_PICKAXE, 1, 2, { { BLOCK_DIAMOND_ORE, 3 }, { ITEM_STICK, 2 } } },

    // 14. 2 Diamond + 1 Stick -> Diamond Sword (钻石剑)
    { 14, "Diamond Sword", "钻石剑", "WEAPONS",
      "Peerless crystalline blade glowing with azure radiance. Massive damage.",
      "散发天蓝光辉的至尊钻石剑，造成巨大伤害。",
      ITEM_TOOL_DIAMOND_SWORD, 1, 2, { { BLOCK_DIAMOND_ORE, 2 }, { ITEM_STICK, 1 } } },

    // 15. 1 Coal Ore + 1 Stick -> 4 Torches (火把)
    { 15, "Torches", "火把", "SUPPLIES",
      "Traditional coal-lit torches that dispel subterranean darkness.",
      "传统煤炭火把，驱散地底深处的黑暗。",
      BLOCK_TORCH, 4, 2, { { BLOCK_COAL_ORE, 1 }, { ITEM_STICK, 1 } } },

    // 16. 1 Slime Gel + 1 Stick -> 4 Torches (史莱姆火把)
    { 16, "Gel Torches", "史莱姆火把", "SUPPLIES",
      "Luminous torch fueled by flammable slime gelatin.",
      "明亮火把，由可燃史莱姆凝胶制成。",
      BLOCK_TORCH, 4, 2, { { ITEM_GEL, 1 }, { ITEM_STICK, 1 } } },

    // 17. 4 Stone -> 4 Stone Bricks (石砖)
    { 17, "Stone Bricks", "石砖", "BLOCKS",
      "Polished masonry bricks for constructing castles, dungeons and forts.",
      "打磨石砖，用于建造城堡、地牢与要塞。",
      BLOCK_BRICK, 4, 1, { { BLOCK_STONE, 4 } } },

    // 18. 1 Sand -> 1 Glass (玻璃)
    { 18, "Glass", "玻璃", "BLOCKS",
      "Heat-smelted desert sand cooled into crystalline architectural glass.",
      "高温熔炼沙子冷却而成的透明建筑玻璃。",
      BLOCK_GLASS, 1, 1, { { BLOCK_SAND, 1 } } },

    // 19. 1 Slime Gel + 1 Torch -> 1 Sticky Bomb (粘性炸弹)
    { 19, "Sticky Bomb", "粘性炸弹", "SUPPLIES",
      "Adhesive demolition charge that clings steadfastly to cavern walls.",
      "可紧紧粘附在洞穴墙壁上的强力粘性炸弹。",
      ITEM_STICKY_BOMB, 1, 2, { { ITEM_GEL, 1 }, { BLOCK_TORCH, 1 } } },

    // 20. 4 Wood Planks -> Work Bench (工作台)
    { 20, "Work Bench", "工作台", "BLOCKS",
      "Essential carpentry crafting bench for assembling fine tools and furnishings.",
      "基础木工工作台，是搭建工坊与制作精细工具的核心平台。",
      BLOCK_WORKBENCH, 1, 1, { { BLOCK_WOOD_PLANK, 4 } } },

    // 21. 4 Wood Planks -> Wooden Chair (木椅)
    { 21, "Wooden Chair", "木椅", "BLOCKS",
      "Hand-carved wooden chair. Adds home comfort to any shelter.",
      "手工雕刻的舒适木椅，探险家居所的经典家具。",
      BLOCK_CHAIR, 1, 1, { { BLOCK_WOOD_PLANK, 4 } } },

    // 22. 4 Wood Planks + 2 Sticks -> Wooden Table (木桌)
    { 22, "Wooden Table", "木桌", "BLOCKS",
      "Sturdy four-legged dining and workbench table.",
      "厚重结实的四角木桌，适合摆放烛台与战利品。",
      BLOCK_TABLE, 1, 2, { { BLOCK_WOOD_PLANK, 4 }, { ITEM_STICK, 2 } } },

    // 23. 6 Wood Planks -> Wooden Door (木门)
    { 23, "Wooden Door", "木门", "BLOCKS",
      "Hinged entrance door with brass knob to secure your home.",
      "装配黄铜把手的铰链木门，阻挡洞穴外的蝙蝠与僵尸。",
      BLOCK_DOOR, 1, 1, { { BLOCK_WOOD_PLANK, 6 } } },

    // 24. 5 Wood Planks + 3 Slime Gel -> Comfortable Bed (舒适木床)
    { 24, "Bed", "舒适木床", "BLOCKS",
      "Cozy bed with soft stuffing and red quilt. Rest here to recover.",
      "柔软充填棉被的舒适木床，让你在庇护所中安心休息。",
      BLOCK_BED, 1, 2, { { BLOCK_WOOD_PLANK, 5 }, { ITEM_GEL, 3 } } },

    // 25. 8 Stone + 2 Coal Ore -> Furnace (石制熔炉)
    { 25, "Furnace", "石制熔炉", "BLOCKS",
      "Cobblestone smelting furnace with smoldering coal fire.",
      "由坚固原石砌成的炽热冶炼炉，内部炭火熊熊燃烧。",
      BLOCK_FURNACE, 1, 2, { { BLOCK_STONE, 8 }, { BLOCK_COAL_ORE, 2 } } },

    // 26. 8 Wood Planks + 2 Iron Ore -> Treasure Chest (大藏宝箱)
    { 26, "Treasure Chest", "大藏宝箱", "BLOCKS",
      "Brass-banded storage chest to safeguard your subterranean loot.",
      "镶嵌黄铜包角与加固锁扣的木质宝箱，用于珍藏稀世珍宝。",
      BLOCK_CHEST, 1, 2, { { BLOCK_WOOD_PLANK, 8 }, { BLOCK_IRON_ORE, 2 } } },

    // 27. 6 Wood Planks + 2 Sticks -> Bookshelf (精装书架)
    { 27, "Bookshelf", "精装书架", "BLOCKS",
      "Three-tier wooden bookcase holding ancient magical tomes.",
      "三层实木书架，陈列着记载魔法秘典的羊皮卷轴。",
      BLOCK_BOOKSHELF, 1, 2, { { BLOCK_WOOD_PLANK, 6 }, { ITEM_STICK, 2 } } },

    // 28. 6 Wood Planks + 1 Gold Ore -> Clock (古典摆钟)
    { 28, "Clock", "古典摆钟", "BLOCKS",
      "Grandfather clock with swinging brass pendulum indicating time.",
      "高雅的落地摆钟，黄铜钟摆来回摆动，精准指示世界时辰。",
      BLOCK_CLOCK, 1, 2, { { BLOCK_WOOD_PLANK, 6 }, { BLOCK_GOLD_ORE, 1 } } },

    // 29. 4 Torches + 2 Iron Ore -> Chandelier (铁艺吊灯)
    { 29, "Chandelier", "铁艺吊灯", "BLOCKS",
      "Wrought-iron ceiling chandelier illuminating cavern halls.",
      "悬挂式锻铁枝形吊灯，装有四支温润长燃蜡烛，照亮大厅。",
      BLOCK_CHANDELIER, 1, 2, { { BLOCK_TORCH, 4 }, { BLOCK_IRON_ORE, 2 } } }
};

static inline const char* getRecipeLocalizedName(const RecipeDef* r) {
    if (!r) return "";
    return (getLanguage() == LANG_ZH) ? r->chineseName : r->name;
}

static inline const char* getRecipeDescription(const RecipeDef* r) {
    if (!r) return "";
    return (getLanguage() == LANG_ZH) ? (r->chineseDescription ? r->chineseDescription : r->description) : r->description;
}

// ============================================================================
// State Variables
// ============================================================================
typedef enum {
    CRAFT_TAB_CRAFTING = 0,
    CRAFT_TAB_INVENTORY = 1
} CraftingTab;

static bool        g_CraftingOpen = false;
static CraftingTab g_CraftingCurrentTab = CRAFT_TAB_CRAFTING;
static int         g_CraftingSelectedRecipe = 0;
static int         g_CraftingScrollOffset = 0;
static int         g_CraftingActiveCategory = 0; // 0: ALL, 1: TOOLS, 2: WEAPONS, 3: BLOCKS, 4: SUPPLIES, 5: MATERIALS
static int         g_CraftingMouseX = 0;
static int         g_CraftingMouseY = 0;
static bool        g_CraftBtnHovered = false;
static bool        g_CraftBtnActive = false;

// 27-slot backpack inventory storage complementing the 9 hotbar slots
static InventorySlot g_Backpack[BACKPACK_SLOTS];

typedef struct {
    int type;
    int count;
    int sourceSlot; // 0..8 hotbar, 9..35 backpack, -1 none
} CursorHeldItem;

static CursorHeldItem g_CursorHeld = { 0, 0, -1 };

static inline const char* getCraftCategoryName(int cat) {
    switch (cat) {
        case 0: return LOC("全部", "All");
        case 1: return LOC("工具", "Tools");
        case 2: return LOC("武器", "Weapons");
        case 3: return LOC("建筑", "Blocks");
        case 4: return LOC("消耗品", "Supplies");
        case 5: return LOC("基础材料", "Materials");
        default: return "";
    }
}
#define CRAFT_CATEGORY_COUNT 6

// ============================================================================
// Procedural Fanfare Synthesizer (SFX_CRAFT) via Windows WinMM PlaySoundA
// ============================================================================
static unsigned char s_CraftFanfareWav[44 + 32000];
static bool s_CraftWavBuilt = false;

static void buildCraftFanfareWav(void) {
    if (s_CraftWavBuilt) return;
    int sampleRate = 44100;
    int numSamples = 15435; // ~0.35s
    int dataSize = numSamples * 2; // 16-bit mono
    int fileSize = 36 + dataSize;

    // RIFF Header
    memcpy(&s_CraftFanfareWav[0], "RIFF", 4);
    memcpy(&s_CraftFanfareWav[4], &fileSize, 4);
    memcpy(&s_CraftFanfareWav[8], "WAVE", 4);
    memcpy(&s_CraftFanfareWav[12], "fmt ", 4);
    int fmtChunkSize = 16;
    short audioFormat = 1; // PCM
    short numChannels = 1; // Mono
    int byteRate = sampleRate * 2;
    short blockAlign = 2;
    short bitsPerSample = 16;
    memcpy(&s_CraftFanfareWav[16], &fmtChunkSize, 4);
    memcpy(&s_CraftFanfareWav[20], &audioFormat, 2);
    memcpy(&s_CraftFanfareWav[22], &numChannels, 2);
    memcpy(&s_CraftFanfareWav[24], &sampleRate, 4);
    memcpy(&s_CraftFanfareWav[28], &byteRate, 4);
    memcpy(&s_CraftFanfareWav[32], &blockAlign, 2);
    memcpy(&s_CraftFanfareWav[34], &bitsPerSample, 2);
    memcpy(&s_CraftFanfareWav[36], "data", 4);
    memcpy(&s_CraftFanfareWav[40], &dataSize, 4);

    short* samples = (short*)&s_CraftFanfareWav[44];
    float freqs[4] = { 523.25f, 659.25f, 783.99f, 1046.50f }; // C5 -> E5 -> G5 -> C6
    float noteDur = 0.35f / 4.0f;

    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / (float)sampleRate;
        int noteIdx = (int)(t / noteDur);
        if (noteIdx > 3) noteIdx = 3;
        float noteT = t - (float)noteIdx * noteDur;
        float f = freqs[noteIdx];
        float env = expf(-noteT * 12.0f);
        if (noteIdx == 3) env = expf(-noteT * 5.0f); // Lingering final note
        float wave = sinf(2.0f * (float)M_PI * f * noteT) * 0.70f + sinf(4.0f * (float)M_PI * f * noteT) * 0.25f;
        samples[i] = (short)(wave * env * 14000.0f);
    }
    s_CraftWavBuilt = true;
}

static void playCraftFanfareSound(void) {
    buildCraftFanfareWav();
    PlaySoundA((LPCSTR)s_CraftFanfareWav, NULL, SND_MEMORY | SND_ASYNC);
}

// ============================================================================
// Public Accessors & Modals
// ============================================================================
static inline bool isCraftingOpen(void) {
    return g_CraftingOpen;
}

static inline void openCraftingUI(CraftingTab tab) {
    g_CraftingOpen = true;
    g_CraftingCurrentTab = tab;
}

static inline void returnCursorHeldItem(void);
static inline void closeCraftingUI(void) {
    returnCursorHeldItem();
    g_CraftingOpen = false;
}

static inline void toggleCraftingUI(void) {
    if (g_CraftingOpen && g_CraftingCurrentTab == CRAFT_TAB_CRAFTING) {
        g_CraftingOpen = false;
    } else {
        openCraftingUI(CRAFT_TAB_CRAFTING);
    }
}

static inline void toggleCraftingInventoryUI(void) {
    if (g_CraftingOpen) {
        g_CraftingOpen = false;
    } else {
        openCraftingUI(CRAFT_TAB_INVENTORY);
    }
}

// ============================================================================
// Inventory Query & Deduction Helpers
// ============================================================================
static int countPlayerItem(int type) {
    int total = 0;
    // Check hotbar (9 slots)
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (g_Hotbar[i].type == type && g_Hotbar[i].count > 0) {
            total += g_Hotbar[i].count;
        }
    }
    // Check backpack (27 slots)
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        if (g_Backpack[i].type == type && g_Backpack[i].count > 0) {
            total += g_Backpack[i].count;
        }
    }
    return total;
}

static bool deductPlayerItem(int type, int count) {
    if (count <= 0) return true;
    if (countPlayerItem(type) < count) return false;

    int remaining = count;

    // Deduct from backpack first
    for (int i = 0; i < BACKPACK_SLOTS && remaining > 0; i++) {
        if (g_Backpack[i].type == type && g_Backpack[i].count > 0) {
            if (g_Backpack[i].count <= remaining) {
                remaining -= g_Backpack[i].count;
                g_Backpack[i].type = BLOCK_AIR;
                g_Backpack[i].count = 0;
            } else {
                g_Backpack[i].count -= remaining;
                remaining = 0;
            }
        }
    }

    // Then deduct from hotbar
    for (int i = 0; i < HOTBAR_SLOTS && remaining > 0; i++) {
        if (g_Hotbar[i].type == type && g_Hotbar[i].count > 0) {
            if (g_Hotbar[i].count <= remaining) {
                remaining -= g_Hotbar[i].count;
                g_Hotbar[i].type = BLOCK_AIR;
                g_Hotbar[i].count = 0;
            } else {
                g_Hotbar[i].count -= remaining;
                remaining = 0;
            }
        }
    }

    return (remaining == 0);
}

// Delivers item into hotbar first, then backpack
static bool deliverCraftedItem(int type, int count) {
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (g_Hotbar[i].type == type && g_Hotbar[i].count < 999) {
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

    // If hotbar full, place into backpack
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        if (g_Backpack[i].type == type && g_Backpack[i].count < 999) {
            g_Backpack[i].count += count;
            return true;
        }
    }
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        if (g_Backpack[i].type == BLOCK_AIR || g_Backpack[i].count <= 0) {
            g_Backpack[i].type = type;
            g_Backpack[i].count = count;
            return true;
        }
    }
    return false;
}

static inline void returnCursorHeldItem(void) {
    if (g_CursorHeld.type != BLOCK_AIR && g_CursorHeld.count > 0) {
        if (g_CursorHeld.sourceSlot >= 0 && g_CursorHeld.sourceSlot < 36) {
            InventorySlot* src = (g_CursorHeld.sourceSlot < 9) ? &g_Hotbar[g_CursorHeld.sourceSlot] : &g_Backpack[g_CursorHeld.sourceSlot - 9];
            if (src->type == BLOCK_AIR || src->count <= 0) {
                src->type = g_CursorHeld.type;
                src->count = g_CursorHeld.count;
                g_CursorHeld.type = BLOCK_AIR;
                g_CursorHeld.count = 0;
                g_CursorHeld.sourceSlot = -1;
                return;
            } else if (src->type == g_CursorHeld.type && !isToolItem(src->type)) {
                src->count += g_CursorHeld.count;
                g_CursorHeld.type = BLOCK_AIR;
                g_CursorHeld.count = 0;
                g_CursorHeld.sourceSlot = -1;
                return;
            }
        }
        deliverCraftedItem(g_CursorHeld.type, g_CursorHeld.count);
        g_CursorHeld.type = BLOCK_AIR;
        g_CursorHeld.count = 0;
        g_CursorHeld.sourceSlot = -1;
    }
}

static inline void handleSlotClick(int slotIdx, bool isRightClick) {
    if (slotIdx < 0 || slotIdx >= 36) return;
    InventorySlot* slot = (slotIdx < 9) ? &g_Hotbar[slotIdx] : &g_Backpack[slotIdx - 9];

    if (!isRightClick) {
        // Left-Click: Pick up, Drop, Swap, or Merge
        if (g_CursorHeld.type == BLOCK_AIR || g_CursorHeld.count <= 0) {
            if (slot->type != BLOCK_AIR && slot->count > 0) {
                g_CursorHeld.type = slot->type;
                g_CursorHeld.count = slot->count;
                g_CursorHeld.sourceSlot = slotIdx;
                slot->type = BLOCK_AIR;
                slot->count = 0;
                playSound(SFX_CLICK);
            }
        } else {
            // Drop or Swap
            if (slot->type == BLOCK_AIR || slot->count <= 0) {
                slot->type = g_CursorHeld.type;
                slot->count = g_CursorHeld.count;
                g_CursorHeld.type = BLOCK_AIR;
                g_CursorHeld.count = 0;
                g_CursorHeld.sourceSlot = -1;
                playSound(SFX_PLACE);
            } else if (slot->type == g_CursorHeld.type && !isToolItem(slot->type)) {
                int space = 999 - slot->count;
                if (space >= g_CursorHeld.count) {
                    slot->count += g_CursorHeld.count;
                    g_CursorHeld.type = BLOCK_AIR;
                    g_CursorHeld.count = 0;
                    g_CursorHeld.sourceSlot = -1;
                } else {
                    slot->count = 999;
                    g_CursorHeld.count -= space;
                }
                playSound(SFX_PLACE);
            } else {
                // Swap items
                int oldType = slot->type;
                int oldCount = slot->count;
                slot->type = g_CursorHeld.type;
                slot->count = g_CursorHeld.count;
                g_CursorHeld.type = oldType;
                g_CursorHeld.count = oldCount;
                g_CursorHeld.sourceSlot = slotIdx;
                playSound(SFX_CLICK);
            }
        }
    } else {
        // Right-Click: Split stack or place single item
        if (g_CursorHeld.type == BLOCK_AIR || g_CursorHeld.count <= 0) {
            if (slot->type != BLOCK_AIR && slot->count > 0) {
                int half = (slot->count + 1) / 2;
                g_CursorHeld.type = slot->type;
                g_CursorHeld.count = half;
                g_CursorHeld.sourceSlot = slotIdx;
                slot->count -= half;
                if (slot->count <= 0) {
                    slot->type = BLOCK_AIR;
                    slot->count = 0;
                }
                playSound(SFX_CLICK);
            }
        } else {
            if (slot->type == BLOCK_AIR || slot->count <= 0) {
                slot->type = g_CursorHeld.type;
                slot->count = 1;
                g_CursorHeld.count--;
                if (g_CursorHeld.count <= 0) {
                    g_CursorHeld.type = BLOCK_AIR;
                    g_CursorHeld.sourceSlot = -1;
                }
                playSound(SFX_PLACE);
            } else if (slot->type == g_CursorHeld.type && !isToolItem(slot->type) && slot->count < 999) {
                slot->count++;
                g_CursorHeld.count--;
                if (g_CursorHeld.count <= 0) {
                    g_CursorHeld.type = BLOCK_AIR;
                    g_CursorHeld.sourceSlot = -1;
                }
                playSound(SFX_PLACE);
            }
        }
    }
}

// ============================================================================
// Recipe Evaluation & Crafting Action
// ============================================================================
static bool canCraftRecipe(int recipeId, bool isCreative) {
    if (isCreative) return true;
    if (recipeId < 1 || recipeId > RECIPE_COUNT) return false;

    RecipeDef* r = &g_Recipes[recipeId - 1];
    for (int i = 0; i < r->ingredientCount; i++) {
        if (countPlayerItem(r->ingredients[i].type) < r->ingredients[i].count) {
            return false;
        }
    }
    return true;
}

static bool craftRecipe(int recipeId) {
    if (recipeId < 1 || recipeId > RECIPE_COUNT) return false;
    RecipeDef* r = &g_Recipes[recipeId - 1];
    bool isCreative = (getGameMode() == MODE_CREATIVE);

    if (!isCreative) {
        if (!canCraftRecipe(recipeId, false)) {
            showNotification(LOC("缺少必要材料，无法合成！", "Missing ingredients to craft this recipe!"), 1.0f, 0.35f, 0.35f);
            return false;
        }
        // Deduct ingredients in survival
        for (int i = 0; i < r->ingredientCount; i++) {
            deductPlayerItem(r->ingredients[i].type, r->ingredients[i].count);
        }
    }

    // Deliver product
    deliverCraftedItem(r->outputType, r->outputCount);

    // Play fanfare SFX
    playCraftFanfareSound();

    // Spawns floating success notification
    char buf[128];
    const char* itemName = getRecipeLocalizedName(r);
    snprintf(buf, sizeof(buf), "%s%s x%d",
             LOC("已合成并存入背包: ", "Crafted & Added to Inventory: "),
             itemName, r->outputCount);

    spawnFloatingText(buf, g_Player.x, g_Player.y + 1.8f, 0.35f, 1.0f, 0.45f);
    showNotification(buf, 0.35f, 0.95f, 0.45f);

    return true;
}

// Filter helper
static int getFilteredRecipeIndices(int* outIndices, int maxCount) {
    int count = 0;
    const char* catFilter = NULL;
    if (g_CraftingActiveCategory == 1) catFilter = "TOOLS";
    else if (g_CraftingActiveCategory == 2) catFilter = "WEAPONS";
    else if (g_CraftingActiveCategory == 3) catFilter = "BLOCKS";
    else if (g_CraftingActiveCategory == 4) catFilter = "SUPPLIES";
    else if (g_CraftingActiveCategory == 5) catFilter = "MATERIALS";

    for (int i = 0; i < RECIPE_COUNT; i++) {
        if (catFilter == NULL || strcmp(g_Recipes[i].category, catFilter) == 0) {
            if (count < maxCount) {
                outIndices[count++] = i;
            }
        }
    }
    return count;
}

// ============================================================================
// Input Handling
// ============================================================================

static inline void getCraftButtonLayout(float modalX, float modalY,
                                        float* outBtnX, float* outBtnY, float* outBtnW, float* outBtnH,
                                        float* outSmallY, float* outSmallW, float* outSmallH) {
    float cardW = 330.0f;
    float prevX = modalX + cardW + 36.0f;
    float prevY = modalY + 92.0f;
    float prevW = 840.0f - cardW - 52.0f;
    float prevH = 560.0f - 108.0f;

    *outBtnX = prevX + 20.0f;
    *outBtnY = prevY + prevH - 100.0f;
    *outBtnW = prevW - 40.0f;
    *outBtnH = 46.0f;

    *outSmallW = (*outBtnW - 10.0f) * 0.5f;
    *outSmallH = 28.0f;
    *outSmallY = *outBtnY + *outBtnH + 8.0f;
}

static bool handleCraftingInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!g_CraftingOpen) {
        if (uMsg == WM_KEYDOWN) {
            if (wParam == 'C') {
                toggleCraftingUI();
                return true;
            }
            if (wParam == 'E') {
                toggleCraftingInventoryUI();
                return true;
            }
        }
        return false;
    }

    float modalW = 840.0f;
    float modalH = 560.0f;
    float modalX = ((float)WINDOW_WIDTH - modalW) * 0.5f;
    float modalY = ((float)WINDOW_HEIGHT - modalH) * 0.5f;

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            g_CraftingMouseX = (int)(short)LOWORD(lParam);
            g_CraftingMouseY = (int)(short)HIWORD(lParam);

            // Craft button hover check (using unified layout helper)
            float btnX, btnY, btnW, btnH, smallY, smallW, smallH;
            getCraftButtonLayout(modalX, modalY, &btnX, &btnY, &btnW, &btnH, &smallY, &smallW, &smallH);
            g_CraftBtnHovered = (g_CraftingMouseX >= btnX && g_CraftingMouseX <= btnX + btnW &&
                                 g_CraftingMouseY >= btnY && g_CraftingMouseY <= btnY + btnH);
            return true;
        }

        case WM_MOUSEWHEEL: {
            short delta = (short)HIWORD(wParam);
            int filtered[RECIPE_COUNT];
            int count = getFilteredRecipeIndices(filtered, RECIPE_COUNT);

            if (delta > 0) {
                if (g_CraftingScrollOffset > 0) g_CraftingScrollOffset--;
                if (g_CraftingSelectedRecipe > 0) g_CraftingSelectedRecipe--;
            } else if (delta < 0) {
                if (g_CraftingScrollOffset + 7 < count) g_CraftingScrollOffset++;
                if (g_CraftingSelectedRecipe < count - 1) g_CraftingSelectedRecipe++;
            }
            return true;
        }

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN: {
            g_CraftingMouseX = (int)(short)LOWORD(lParam);
            g_CraftingMouseY = (int)(short)HIWORD(lParam);

            // 1. Close button [X]
            float closeX = modalX + modalW - 38.0f;
            float closeY = modalY + 14.0f;
            if (g_CraftingMouseX >= closeX && g_CraftingMouseX <= closeX + 24.0f &&
                g_CraftingMouseY >= closeY && g_CraftingMouseY <= closeY + 24.0f) {
                closeCraftingUI();
                return true;
            }

            // 2. Top Tabs: [CRAFTING] and [INVENTORY]
            float tabW = 145.0f;
            float tabH = 30.0f;
            float tabX = modalX + 380.0f;
            float tabY = modalY + 16.0f;
            if (g_CraftingMouseX >= tabX && g_CraftingMouseX <= tabX + tabW &&
                g_CraftingMouseY >= tabY && g_CraftingMouseY <= tabY + tabH) {
                g_CraftingCurrentTab = CRAFT_TAB_CRAFTING;
                return true;
            }
            if (g_CraftingMouseX >= tabX + tabW + 8.0f && g_CraftingMouseX <= tabX + tabW * 2.0f + 8.0f &&
                g_CraftingMouseY >= tabY && g_CraftingMouseY <= tabY + tabH) {
                g_CraftingCurrentTab = CRAFT_TAB_INVENTORY;
                return true;
            }

            if (g_CraftingCurrentTab == CRAFT_TAB_CRAFTING) {
                // 3. Category Toolbar buttons
                float catW = 95.0f;
                float catH = 24.0f;
                float catX = modalX + 16.0f;
                float catY = modalY + 58.0f;
                for (int i = 0; i < CRAFT_CATEGORY_COUNT; i++) {
                    float bx = catX + i * (catW + 4.0f);
                    if (g_CraftingMouseX >= bx && g_CraftingMouseX <= bx + catW &&
                        g_CraftingMouseY >= catY && g_CraftingMouseY <= catY + catH) {
                        g_CraftingActiveCategory = i;
                        g_CraftingSelectedRecipe = 0;
                        g_CraftingScrollOffset = 0;
                        return true;
                    }
                }

                // 4. Click Recipe List card
                int filtered[RECIPE_COUNT];
                int count = getFilteredRecipeIndices(filtered, RECIPE_COUNT);
                float listX = modalX + 16.0f;
                float listY = modalY + 92.0f;
                float cardW = 320.0f;
                float cardH = 54.0f;
                for (int i = 0; i < 7 && (i + g_CraftingScrollOffset) < count; i++) {
                    int rIdx = i + g_CraftingScrollOffset;
                    float cy = listY + i * (cardH + 6.0f);
                    if (g_CraftingMouseX >= listX && g_CraftingMouseX <= listX + cardW &&
                        g_CraftingMouseY >= cy && g_CraftingMouseY <= cy + cardH) {
                        g_CraftingSelectedRecipe = rIdx;
                        return true;
                    }
                }

                // 5. Click "CRAFT [合成]" button (using unified layout helper)
                float btnX, btnY, btnW, btnH, smallY, smallW, smallH;
                getCraftButtonLayout(modalX, modalY, &btnX, &btnY, &btnW, &btnH, &smallY, &smallW, &smallH);

                if (g_CraftingMouseX >= btnX && g_CraftingMouseX <= btnX + btnW &&
                    g_CraftingMouseY >= btnY && g_CraftingMouseY <= btnY + btnH) {
                    g_CraftBtnActive = true;
                    // Strict 1x craft: consumes only 1 recipe's ingredients
                    if (g_CraftingSelectedRecipe >= 0 && g_CraftingSelectedRecipe < count) {
                        craftRecipe(filtered[g_CraftingSelectedRecipe] + 1);
                    }
                    return true;
                }

                // 6. Bulk craft buttons
                // Craft x5
                if (g_CraftingMouseX >= btnX && g_CraftingMouseX <= btnX + smallW &&
                    g_CraftingMouseY >= smallY && g_CraftingMouseY <= smallY + smallH) {
                    if (g_CraftingSelectedRecipe >= 0 && g_CraftingSelectedRecipe < count) {
                        for (int k = 0; k < 5; k++) {
                            if (!craftRecipe(filtered[g_CraftingSelectedRecipe] + 1)) break;
                        }
                    }
                    return true;
                }
                // Craft Max
                if (g_CraftingMouseX >= btnX + smallW + 10.0f && g_CraftingMouseX <= btnX + btnW &&
                    g_CraftingMouseY >= smallY && g_CraftingMouseY <= smallY + smallH) {
                    if (g_CraftingSelectedRecipe >= 0 && g_CraftingSelectedRecipe < count) {
                        for (int k = 0; k < 64; k++) {
                            if (!craftRecipe(filtered[g_CraftingSelectedRecipe] + 1)) break;
                        }
                    }
                    return true;
                }
            } else {
                // 1. Hotbar Slots (0..8)
                float startX = modalX + 24.0f;
                float startY = modalY + 70.0f;
                float slotSize = 64.0f;
                float slotGap = 10.0f;
                float hy = startY + 28.0f;
                for (int i = 0; i < HOTBAR_SLOTS; i++) {
                    float sx = startX + i * (slotSize + slotGap);
                    if (g_CraftingMouseX >= sx && g_CraftingMouseX <= sx + slotSize &&
                        g_CraftingMouseY >= hy && g_CraftingMouseY <= hy + slotSize) {
                        handleSlotClick(i, uMsg == WM_RBUTTONDOWN);
                        return true;
                    }
                }

                // 2. Backpack Slots (9..35)
                float by = hy + slotSize + 26.0f + 28.0f;
                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 9; c++) {
                        int sIdx = r * 9 + c;
                        float sx = startX + c * (slotSize + slotGap);
                        float sy = by + r * (slotSize + slotGap);
                        if (g_CraftingMouseX >= sx && g_CraftingMouseX <= sx + slotSize &&
                            g_CraftingMouseY >= sy && g_CraftingMouseY <= sy + slotSize) {
                            handleSlotClick(sIdx + 9, uMsg == WM_RBUTTONDOWN);
                            return true;
                        }
                    }
                }
            }
            return true;
        }

        case WM_LBUTTONUP: {
            g_CraftBtnActive = false;
            return true;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE || wParam == 'C') {
                closeCraftingUI();
                return true;
            }
            if (wParam == 'E') {
                if (g_CraftingCurrentTab == CRAFT_TAB_INVENTORY) {
                    closeCraftingUI();
                } else {
                    g_CraftingCurrentTab = CRAFT_TAB_INVENTORY;
                }
                return true;
            }

            int filtered[RECIPE_COUNT];
            int count = getFilteredRecipeIndices(filtered, RECIPE_COUNT);

            if (wParam == VK_UP || wParam == 'W') {
                if (g_CraftingSelectedRecipe > 0) {
                    g_CraftingSelectedRecipe--;
                    if (g_CraftingSelectedRecipe < g_CraftingScrollOffset) {
                        g_CraftingScrollOffset = g_CraftingSelectedRecipe;
                    }
                }
                return true;
            }
            if (wParam == VK_DOWN || wParam == 'S') {
                if (g_CraftingSelectedRecipe < count - 1) {
                    g_CraftingSelectedRecipe++;
                    if (g_CraftingSelectedRecipe >= g_CraftingScrollOffset + 7) {
                        g_CraftingScrollOffset = g_CraftingSelectedRecipe - 6;
                    }
                }
                return true;
            }
            if (wParam == VK_RETURN || wParam == VK_SPACE) {
                if (g_CraftingSelectedRecipe >= 0 && g_CraftingSelectedRecipe < count) {
                    craftRecipe(filtered[g_CraftingSelectedRecipe] + 1);
                }
                return true;
            }
            return true;
        }
    }

    return false;
}

// ============================================================================
// UI Rendering
// ============================================================================
static void renderCraftingUI(void) {
    if (!g_CraftingOpen) return;

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
    menuDrawQuad(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, 0.0f, 0.0f, 0.65f);

    float modalW = 840.0f;
    float modalH = 560.0f;
    float modalX = ((float)WINDOW_WIDTH - modalW) * 0.5f;
    float modalY = ((float)WINDOW_HEIGHT - modalH) * 0.5f;

    // 2. Main ornate modal window
    menuDrawQuad(modalX, modalY, modalW, modalH, 0.07f, 0.09f, 0.13f, 0.96f);
    menuDrawBorder(modalX, modalY, modalW, modalH, 0.28f, 0.36f, 0.48f, 2.0f);
    // Outer golden filigree accent
    menuDrawBorder(modalX - 2, modalY - 2, modalW + 4, modalH + 4, 0.85f, 0.70f, 0.25f, 1.0f);

    // 3. Header title & mode banner
    drawText(modalX + 20, modalY + 28, LOC("物品合成工作台", "Crafting Station"), 1.0f, 0.88f, 0.35f, true);

    bool isCreative = (getGameMode() == MODE_CREATIVE);
    if (isCreative) {
        drawText(modalX + 20, modalY + 46, LOC("创造模式：开启免费即时合成", "[Creative Mode] Free Instant Crafting Enabled"), 0.35f, 0.90f, 1.0f, false);
    } else {
        drawText(modalX + 20, modalY + 46, LOC("生存模式：消耗背包原料", "[Survival Mode] Requires Ingredients from Inventory"), 0.35f, 0.95f, 0.45f, false);
    }

    // Top Tabs: [CRAFTING (合成)] and [INVENTORY (物品栏)]
    float tabW = 145.0f;
    float tabH = 30.0f;
    float tabX = modalX + 460.0f;
    float tabY = modalY + 16.0f;

    // Tab 1: Crafting
    if (g_CraftingCurrentTab == CRAFT_TAB_CRAFTING) {
        menuDrawQuad(tabX, tabY, tabW, tabH, 0.42f, 0.35f, 0.15f, 1.0f);
        menuDrawBorder(tabX, tabY, tabW, tabH, 1.0f, 0.85f, 0.25f, 2.0f);
        drawText(tabX + (getLanguage() == LANG_ZH ? 38 : 42), tabY + 20, LOC("配方合成", "Crafting"), 1.0f, 0.92f, 0.40f, true);
    } else {
        menuDrawQuad(tabX, tabY, tabW, tabH, 0.18f, 0.20f, 0.26f, 1.0f);
        menuDrawBorder(tabX, tabY, tabW, tabH, 0.35f, 0.40f, 0.50f, 1.0f);
        drawText(tabX + (getLanguage() == LANG_ZH ? 38 : 42), tabY + 20, LOC("配方合成", "Crafting"), 0.75f, 0.75f, 0.75f, false);
    }

    // Tab 2: Inventory
    float tab2X = tabX + tabW + 8.0f;
    if (g_CraftingCurrentTab == CRAFT_TAB_INVENTORY) {
        menuDrawQuad(tab2X, tabY, tabW, tabH, 0.42f, 0.35f, 0.15f, 1.0f);
        menuDrawBorder(tab2X, tabY, tabW, tabH, 1.0f, 0.85f, 0.25f, 2.0f);
        drawText(tab2X + (getLanguage() == LANG_ZH ? 38 : 38), tabY + 20, LOC("背包预览", "Inventory"), 1.0f, 0.92f, 0.40f, true);
    } else {
        menuDrawQuad(tab2X, tabY, tabW, tabH, 0.18f, 0.20f, 0.26f, 1.0f);
        menuDrawBorder(tab2X, tabY, tabW, tabH, 0.35f, 0.40f, 0.50f, 1.0f);
        drawText(tab2X + (getLanguage() == LANG_ZH ? 38 : 38), tabY + 20, LOC("背包预览", "Inventory"), 0.75f, 0.75f, 0.75f, false);
    }

    // Close button [X]
    float closeX = modalX + modalW - 36.0f;
    float closeY = modalY + 16.0f;
    menuDrawQuad(closeX, closeY, 22, 22, 0.75f, 0.20f, 0.20f, 1.0f);
    menuDrawBorder(closeX, closeY, 22, 22, 1.0f, 0.8f, 0.8f, 1.0f);
    drawText(closeX + 6, closeY + 16, "X", 1.0f, 1.0f, 1.0f, true);

    // Divider line
    menuDrawQuad(modalX + 16, modalY + 54, modalW - 32, 2, 0.25f, 0.32f, 0.42f, 0.8f);

    if (g_CraftingCurrentTab == CRAFT_TAB_CRAFTING) {
        // Category Filter buttons
        float catW = 95.0f;
        float catH = 24.0f;
        float catX = modalX + 16.0f;
        float catY = modalY + 62.0f;
        for (int i = 0; i < CRAFT_CATEGORY_COUNT; i++) {
            float bx = catX + i * (catW + 4.0f);
            bool isActive = (g_CraftingActiveCategory == i);
            const char* cname = getCraftCategoryName(i);
            if (isActive) {
                menuDrawQuad(bx, catY, catW, catH, 0.40f, 0.32f, 0.12f, 1.0f);
                menuDrawBorder(bx, catY, catW, catH, 1.0f, 0.85f, 0.25f, 1.5f);
                drawText(bx + 10, catY + 17, cname, 1.0f, 0.95f, 0.45f, false);
            } else {
                menuDrawQuad(bx, catY, catW, catH, 0.15f, 0.18f, 0.24f, 1.0f);
                menuDrawBorder(bx, catY, catW, catH, 0.30f, 0.35f, 0.45f, 1.0f);
                drawText(bx + 10, catY + 17, cname, 0.75f, 0.75f, 0.75f, false);
            }
        }

        // Left Column: Recipe List
        int filtered[RECIPE_COUNT];
        int count = getFilteredRecipeIndices(filtered, RECIPE_COUNT);

        float listX = modalX + 16.0f;
        float listY = modalY + 92.0f;
        float cardW = 330.0f;
        float cardH = 56.0f;

        menuDrawQuad(listX - 4, listY - 4, cardW + 8, 7 * (cardH + 6.0f) + 4, 0.05f, 0.07f, 0.10f, 0.92f);
        menuDrawBorder(listX - 4, listY - 4, cardW + 8, 7 * (cardH + 6.0f) + 4, 0.25f, 0.32f, 0.42f, 1.0f);

        for (int i = 0; i < 7 && (i + g_CraftingScrollOffset) < count; i++) {
            int rIdx = i + g_CraftingScrollOffset;
            RecipeDef* r = &g_Recipes[filtered[rIdx]];
            bool isSelected = (rIdx == g_CraftingSelectedRecipe);
            bool canCraft = canCraftRecipe(filtered[rIdx] + 1, isCreative);

            float cy = listY + i * (cardH + 6.0f);

            if (isSelected) {
                menuDrawQuad(listX, cy, cardW, cardH, 0.22f, 0.26f, 0.36f, 0.98f);
                menuDrawBorder(listX, cy, cardW, cardH, 1.0f, 0.85f, 0.25f, 2.0f);
            } else {
                menuDrawQuad(listX, cy, cardW, cardH, 0.12f, 0.14f, 0.20f, 0.92f);
                menuDrawBorder(listX, cy, cardW, cardH, 0.25f, 0.30f, 0.40f, 1.0f);
            }

            // Recipe output icon (using standalone item icon drawer)
            drawInventoryItemIcon(listX + 8, cy + 8, 40, r->outputType);
            menuDrawBorder(listX + 8, cy + 8, 40, 40, 0.35f, 0.40f, 0.50f, 1.0f);

            // Title & quantity
            char titleBuf[64];
            snprintf(titleBuf, sizeof(titleBuf), "%s x%d", getRecipeLocalizedName(r), r->outputCount);
            if (isSelected) {
                drawText(listX + 56, cy + 22, titleBuf, 1.0f, 0.92f, 0.35f, true);
            } else {
                drawText(listX + 56, cy + 22, titleBuf, 0.95f, 0.95f, 0.95f, false);
            }

            // Category Subtitle & Status Badge
            const char* catLocalized = "";
            if (strcmp(r->category, "TOOLS") == 0) catLocalized = LOC("工具", "Tools");
            else if (strcmp(r->category, "WEAPONS") == 0) catLocalized = LOC("武器", "Weapons");
            else if (strcmp(r->category, "BLOCKS") == 0) catLocalized = LOC("建筑", "Blocks");
            else if (strcmp(r->category, "SUPPLIES") == 0) catLocalized = LOC("消耗品", "Supplies");
            else if (strcmp(r->category, "MATERIALS") == 0) catLocalized = LOC("材料", "Materials");
            drawText(listX + 56, cy + 42, catLocalized, 0.70f, 0.75f, 0.82f, false);

            if (canCraft) {
                drawText(listX + cardW - 74, cy + 42, LOC("[可合成]", "[Ready]"), 0.35f, 0.95f, 0.45f, false);
            } else {
                drawText(listX + cardW - 80, cy + 42, LOC("[缺材料]", "[Need]"), 0.65f, 0.65f, 0.70f, false);
            }
        }

        // Right Column: Preview & Craft Pane
        float prevX = modalX + cardW + 36.0f;
        float prevY = modalY + 92.0f;
        float prevW = modalW - cardW - 52.0f;
        float prevH = modalH - 108.0f;

        menuDrawQuad(prevX, prevY, prevW, prevH, 0.08f, 0.10f, 0.15f, 0.95f);
        menuDrawBorder(prevX, prevY, prevW, prevH, 0.30f, 0.38f, 0.48f, 1.5f);

        if (g_CraftingSelectedRecipe >= 0 && g_CraftingSelectedRecipe < count) {
            RecipeDef* sel = &g_Recipes[filtered[g_CraftingSelectedRecipe]];
            bool canCraft = canCraftRecipe(filtered[g_CraftingSelectedRecipe] + 1, isCreative);

            // Big Result Icon Box
            menuDrawQuad(prevX + 20, prevY + 20, 56, 56, 0.14f, 0.16f, 0.22f, 1.0f);
            menuDrawBorder(prevX + 20, prevY + 20, 56, 56, 0.85f, 0.70f, 0.25f, 2.0f);
            drawInventoryItemIcon(prevX + 24, prevY + 24, 48, sel->outputType);

            // Product Name & Amount
            char resTitle[128];
            snprintf(resTitle, sizeof(resTitle), "%s  x%d", getRecipeLocalizedName(sel), sel->outputCount);
            drawText(prevX + 88, prevY + 38, resTitle, 1.0f, 0.90f, 0.35f, true);

            const char* selCatLocalized = "";
            if (strcmp(sel->category, "TOOLS") == 0) selCatLocalized = LOC("工具", "Tools");
            else if (strcmp(sel->category, "WEAPONS") == 0) selCatLocalized = LOC("武器", "Weapons");
            else if (strcmp(sel->category, "BLOCKS") == 0) selCatLocalized = LOC("建筑", "Blocks");
            else if (strcmp(sel->category, "SUPPLIES") == 0) selCatLocalized = LOC("消耗品", "Supplies");
            else if (strcmp(sel->category, "MATERIALS") == 0) selCatLocalized = LOC("材料", "Materials");

            char catText[64];
            snprintf(catText, sizeof(catText), "%s: %s  |  %s: %d",
                     LOC("类别", "Category"), selCatLocalized,
                     LOC("产出", "Output"), sel->outputCount);
            drawText(prevX + 88, prevY + 60, catText, 0.40f, 0.80f, 0.95f, false);

            // Description Box
            drawText(prevX + 20, prevY + 102, getRecipeDescription(sel), 0.85f, 0.85f, 0.90f, false);

            menuDrawQuad(prevX + 20, prevY + 120, prevW - 40, 2, 0.25f, 0.30f, 0.40f, 0.8f);

            // Required Materials Section
            drawText(prevX + 20, prevY + 146, LOC("所需合成材料:", "Required Ingredients:"), 1.0f, 0.85f, 0.35f, true);

            for (int i = 0; i < sel->ingredientCount; i++) {
                int itype = sel->ingredients[i].type;
                int req = sel->ingredients[i].count;
                int owned = countPlayerItem(itype);
                bool satisfied = (owned >= req) || isCreative;

                float iy = prevY + 172.0f + i * 36.0f;

                // Mini slot
                menuDrawQuad(prevX + 24, iy, 28, 28, 0.12f, 0.14f, 0.18f, 1.0f);
                menuDrawBorder(prevX + 24, iy, 28, 28, satisfied ? 0.35f : 0.85f, satisfied ? 0.85f : 0.35f, 0.35f, 1.5f);
                drawInventoryItemIcon(prevX + 26, iy + 2, 24, itype);

                char ingBuf[128];
                const char* localizedItemName = getItemLocalizedName(itype);
                if (satisfied) {
                    snprintf(ingBuf, sizeof(ingBuf), "%s: %d / %d  [%s]",
                             localizedItemName, owned, req, LOC("已齐备 ✔", "Ready ✔"));
                    drawText(prevX + 62, iy + 20, ingBuf, 0.35f, 0.95f, 0.45f, true);
                } else {
                    snprintf(ingBuf, sizeof(ingBuf), "%s: %d / %d  [%s]",
                             localizedItemName, owned, req, LOC("缺少 ✖", "Missing ✖"));
                    drawText(prevX + 62, iy + 20, ingBuf, 1.0f, 0.35f, 0.35f, true);
                }
            }

            // Status Badge indicator
            float badgeY = prevY + 172.0f + sel->ingredientCount * 36.0f + 6.0f;
            if (canCraft || isCreative) {
                drawText(prevX + 24, badgeY, LOC("状态: 材料已齐备 (可合成)", "Status: Ready to Craft"), 0.35f, 0.95f, 0.45f, true);
            } else {
                drawText(prevX + 24, badgeY, LOC("状态: 缺少必要材料", "Status: Missing Materials"), 1.0f, 0.40f, 0.40f, true);
            }

            // Big Golden "CRAFT [合成]" Button (using unified layout helper)
            float btnX, btnY, btnW, btnH, smallY, smallW, smallH;
            getCraftButtonLayout(modalX, modalY, &btnX, &btnY, &btnW, &btnH, &smallY, &smallW, &smallH);

            if (g_CraftBtnActive) {
                menuDrawQuad(btnX, btnY, btnW, btnH, 0.45f, 0.35f, 0.15f, 1.0f);
                menuDrawBorder(btnX, btnY, btnW, btnH, 1.0f, 0.95f, 0.45f, 2.5f);
            } else if (g_CraftBtnHovered && (canCraft || isCreative)) {
                menuDrawQuad(btnX, btnY, btnW, btnH, 0.55f, 0.45f, 0.18f, 1.0f);
                menuDrawBorder(btnX, btnY, btnW, btnH, 1.0f, 0.95f, 0.50f, 2.5f);
            } else if (canCraft || isCreative) {
                menuDrawQuad(btnX, btnY, btnW, btnH, 0.38f, 0.30f, 0.12f, 1.0f);
                menuDrawBorder(btnX, btnY, btnW, btnH, 0.90f, 0.75f, 0.25f, 2.0f);
            } else {
                menuDrawQuad(btnX, btnY, btnW, btnH, 0.16f, 0.18f, 0.22f, 1.0f);
                menuDrawBorder(btnX, btnY, btnW, btnH, 0.35f, 0.35f, 0.40f, 1.0f);
            }

            char craftBtnText[64];
            if (isCreative) {
                snprintf(craftBtnText, sizeof(craftBtnText), "%s (x%d) [%s]",
                         LOC("开始合成", "Craft Item"), sel->outputCount, LOC("免费", "Free"));
                float cbw = getTextWidth(craftBtnText, true);
                drawText(btnX + (btnW - cbw) * 0.5f, btnY + 28, craftBtnText, 0.4f, 0.95f, 1.0f, true);
            } else if (canCraft) {
                snprintf(craftBtnText, sizeof(craftBtnText), "%s (x%d)",
                         LOC("开始合成", "Craft Item"), sel->outputCount);
                float cbw = getTextWidth(craftBtnText, true);
                drawText(btnX + (btnW - cbw) * 0.5f, btnY + 28, craftBtnText, 1.0f, 0.95f, 0.45f, true);
            } else {
                const char* missText = LOC("缺少必要材料", "Missing Materials");
                float cbw = getTextWidth(missText, false);
                drawText(btnX + (btnW - cbw) * 0.5f, btnY + 28, missText, 0.65f, 0.65f, 0.65f, false);
            }

            // Bulk craft buttons (coordinates from getCraftButtonLayout)
            menuDrawQuad(btnX, smallY, smallW, smallH, 0.16f, 0.19f, 0.26f, 1.0f);
            menuDrawBorder(btnX, smallY, smallW, smallH, 0.35f, 0.42f, 0.55f, 1.0f);
            const char* b5Text = LOC("批量合成 x5", "Craft x5");
            float b5w = getTextWidth(b5Text, false);
            drawText(btnX + (smallW - b5w) * 0.5f, smallY + 18, b5Text, 0.85f, 0.85f, 0.90f, false);

            menuDrawQuad(btnX + smallW + 10.0f, smallY, smallW, smallH, 0.16f, 0.19f, 0.26f, 1.0f);
            menuDrawBorder(btnX + smallW + 10.0f, smallY, smallW, smallH, 0.35f, 0.42f, 0.55f, 1.0f);
            const char* allText = LOC("全部合成", "Craft All");
            float allw = getTextWidth(allText, false);
            drawText(btnX + smallW + 10.0f + (smallW - allw) * 0.5f, smallY + 18, allText, 0.85f, 0.85f, 0.90f, false);
        }

        // Footer helper
        drawText(modalX + 20, modalY + modalH - 12,
            LOC("[点击 / 回车] 合成 | [C / E / ESC] 关闭 | [W/S / 滚轮] 选择配方",
                "[Click / Enter] Craft | [C / E / ESC] Close | [W/S / Scroll] Select Recipe"),
            0.60f, 0.65f, 0.75f, false);

    } else {
        // ====================================================================
        // INVENTORY TAB VIEW (36 Slots: 9 Hotbar + 27 Backpack Grid)
        // ====================================================================
        float startX = modalX + 24.0f;
        float startY = modalY + 70.0f;

        drawText(startX, startY + 16, LOC("快捷栏道具 (1-9):", "Active Hotbar (1-9):"), 1.0f, 0.85f, 0.35f, true);

        float slotSize = 64.0f;
        float slotGap = 10.0f;
        float hy = startY + 28.0f;

        for (int i = 0; i < HOTBAR_SLOTS; i++) {
            float sx = startX + i * (slotSize + slotGap);
            menuDrawQuad(sx, hy, slotSize, slotSize, 0.14f, 0.17f, 0.24f, 1.0f);

            if (i == g_ActiveSlot) {
                menuDrawBorder(sx - 2, hy - 2, slotSize + 4, slotSize + 4, 1.0f, 0.85f, 0.20f, 2.5f);
            } else {
                menuDrawBorder(sx, hy, slotSize, slotSize, 0.30f, 0.38f, 0.48f, 1.0f);
            }

            char numBuf[8];
            snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
            drawText(sx + 4, hy + 14, numBuf, 0.7f, 0.7f, 0.7f, false);

            if (g_Hotbar[i].type != BLOCK_AIR && g_Hotbar[i].count > 0) {
                drawInventoryItemIcon(sx + 10, hy + 10, 44, g_Hotbar[i].type);
                char countBuf[16];
                snprintf(countBuf, sizeof(countBuf), "%d", g_Hotbar[i].count);
                drawText(sx + slotSize - 22, hy + slotSize - 6, countBuf, 1.0f, 1.0f, 1.0f, false);
            }
        }

        float by = hy + slotSize + 26.0f;
        drawText(startX, by + 16, LOC("背包主存储区 (27格):", "Backpack Main Storage (27 Slots):"), 1.0f, 0.85f, 0.35f, true);

        by += 28.0f;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 9; c++) {
                int sIdx = r * 9 + c;
                float sx = startX + c * (slotSize + slotGap);
                float sy = by + r * (slotSize + slotGap);

                menuDrawQuad(sx, sy, slotSize, slotSize, 0.12f, 0.14f, 0.20f, 1.0f);
                menuDrawBorder(sx, sy, slotSize, slotSize, 0.25f, 0.32f, 0.42f, 1.0f);

                if (g_Backpack[sIdx].type != BLOCK_AIR && g_Backpack[sIdx].count > 0) {
                    drawInventoryItemIcon(sx + 10, sy + 10, 44, g_Backpack[sIdx].type);
                    char countBuf[16];
                    snprintf(countBuf, sizeof(countBuf), "%d", g_Backpack[sIdx].count);
                    drawText(sx + slotSize - 22, sy + slotSize - 6, countBuf, 1.0f, 1.0f, 1.0f, false);
                }
            }
        }
    }

    // Draw floating cursor item held during slot drag/swap
    if (g_CursorHeld.type != BLOCK_AIR && g_CursorHeld.count > 0) {
        float cx = (float)g_CraftingMouseX;
        float cy = (float)g_CraftingMouseY;
        menuDrawQuad(cx - 24.0f, cy - 24.0f, 48.0f, 48.0f, 0.16f, 0.20f, 0.28f, 0.88f);
        menuDrawBorder(cx - 24.0f, cy - 24.0f, 48.0f, 48.0f, 1.0f, 0.85f, 0.25f, 2.0f);
        drawInventoryItemIcon(cx - 20.0f, cy - 20.0f, 40.0f, g_CursorHeld.type);
        if (!isToolItem(g_CursorHeld.type)) {
            char cntBuf[16];
            snprintf(cntBuf, sizeof(cntBuf), "%d", g_CursorHeld.count);
            drawText(cx + 6.0f, cy + 18.0f, cntBuf, 1.0f, 1.0f, 0.3f, true);
        }
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

#endif // CRAFTING_SYSTEM_H
