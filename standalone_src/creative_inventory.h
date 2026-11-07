#ifndef CREATIVE_INVENTORY_H
#define CREATIVE_INVENTORY_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "localization.h"

// ============================================================================
// Tool & Weapon System Architecture
// ============================================================================

typedef enum {
    TOOL_NONE = 0,
    TOOL_PICKAXE,
    TOOL_AXE,
    TOOL_SWORD,
    TOOL_SHOVEL,
    TOOL_TYPE_COUNT
} ToolType;

typedef enum {
    TIER_NONE = 0,
    TIER_WOOD,
    TIER_STONE,
    TIER_IRON,
    TIER_DIAMOND,
    TIER_COUNT
} ToolTier;

// Distinct Tool Item IDs (starting at 101 to never collide with BlockType 0..26)
typedef enum {
    ITEM_ID_NONE = 0,

    // Pickaxes (101 - 104)
    ITEM_TOOL_WOOD_PICKAXE    = 101,
    ITEM_TOOL_STONE_PICKAXE   = 102,
    ITEM_TOOL_IRON_PICKAXE    = 103,
    ITEM_TOOL_DIAMOND_PICKAXE = 104,

    // Axes (105 - 108)
    ITEM_TOOL_WOOD_AXE        = 105,
    ITEM_TOOL_STONE_AXE       = 106,
    ITEM_TOOL_IRON_AXE        = 107,
    ITEM_TOOL_DIAMOND_AXE     = 108,

    // Swords (109 - 112)
    ITEM_TOOL_WOOD_SWORD      = 109,
    ITEM_TOOL_STONE_SWORD     = 110,
    ITEM_TOOL_IRON_SWORD      = 111,
    ITEM_TOOL_DIAMOND_SWORD   = 112,

    // Shovels (113 - 116)
    ITEM_TOOL_WOOD_SHOVEL     = 113,
    ITEM_TOOL_STONE_SHOVEL    = 114,
    ITEM_TOOL_IRON_SHOVEL     = 115,
    ITEM_TOOL_DIAMOND_SHOVEL  = 116,

    // Craftable Materials & Consumables (117 - 119)
    ITEM_STICK                = 117,
    ITEM_GEL                  = 118,
    ITEM_STICKY_BOMB          = 119,

    // Armor Pieces (120 - 135)
    ITEM_ARMOR_WOOD_HELMET    = 120,
    ITEM_ARMOR_WOOD_CHEST     = 121,
    ITEM_ARMOR_WOOD_LEGS      = 122,
    ITEM_ARMOR_WOOD_BOOTS     = 123,

    ITEM_ARMOR_STONE_HELMET   = 124,
    ITEM_ARMOR_STONE_CHEST    = 125,
    ITEM_ARMOR_STONE_LEGS     = 126,
    ITEM_ARMOR_STONE_BOOTS    = 127,

    ITEM_ARMOR_IRON_HELMET    = 128,
    ITEM_ARMOR_IRON_CHEST     = 129,
    ITEM_ARMOR_IRON_LEGS      = 130,
    ITEM_ARMOR_IRON_BOOTS     = 131,

    ITEM_ARMOR_DIAMOND_HELMET = 132,
    ITEM_ARMOR_DIAMOND_CHEST  = 133,
    ITEM_ARMOR_DIAMOND_LEGS   = 134,
    ITEM_ARMOR_DIAMOND_BOOTS  = 135,

    ITEM_TOOL_COUNT           = 16,
    ITEM_ARMOR_COUNT          = 16
} ToolItemID;

typedef struct {
    int id;
    ToolType type;
    ToolTier tier;
    const char* name;
    const char* chineseName;
    const char* description;
    const char* englishDescription;
    float speedMultiplier;
    float damage;
    float knockback;
    float r, g, b; // Visual Tier Color
} ToolDef;

// 16 Full Tools Definitions across Wood, Stone, Iron, Diamond
static const ToolDef g_ToolDefs[ITEM_TOOL_COUNT] = {
    // Pickaxes: Speed multiplier on stone, ores, bricks
    { ITEM_TOOL_WOOD_PICKAXE,    TOOL_PICKAXE, TIER_WOOD,    "Wooden Pickaxe",  "木镐",   "采矿工具: 开采岩石与矿脉速度 2.0x",   "Mining tool: Mines stone and ores at 2.0x speed",    2.0f,  6.0f, 1.0f, 0.68f, 0.46f, 0.24f },
    { ITEM_TOOL_STONE_PICKAXE,   TOOL_PICKAXE, TIER_STONE,   "Stone Pickaxe",   "石镐",   "采矿工具: 开采岩石与矿脉速度 4.5x",   "Mining tool: Mines stone and ores at 4.5x speed",    4.5f,  7.0f, 1.1f, 0.62f, 0.64f, 0.66f },
    { ITEM_TOOL_IRON_PICKAXE,    TOOL_PICKAXE, TIER_IRON,    "Iron Pickaxe",    "铁镐",   "采矿工具: 开采岩石与矿脉速度 10.0x",  "Mining tool: Mines stone and ores at 10.0x speed",   10.0f,  8.0f, 1.2f, 0.88f, 0.90f, 0.94f },
    { ITEM_TOOL_DIAMOND_PICKAXE, TOOL_PICKAXE, TIER_DIAMOND, "Diamond Pickaxe", "钻石镐", "神级采矿: 开采岩石与矿脉速度 22.0x",  "Supreme tool: Mines stone and ores at 22.0x speed", 22.0f,  9.0f, 1.4f, 0.25f, 0.90f, 0.98f },

    // Axes: Rapid chop on wood, planks, leaves, cactus
    { ITEM_TOOL_WOOD_AXE,        TOOL_AXE,     TIER_WOOD,    "Wooden Axe",      "木斧",   "伐木工具: 砍伐树木与木板速度 2.0x",   "Logging tool: Chops trees and wood at 2.0x speed",   2.0f,  7.0f, 1.1f, 0.68f, 0.46f, 0.24f },
    { ITEM_TOOL_STONE_AXE,       TOOL_AXE,     TIER_STONE,   "Stone Axe",       "石斧",   "伐木工具: 砍伐树木与木板速度 4.5x",   "Logging tool: Chops trees and wood at 4.5x speed",   4.5f,  8.0f, 1.2f, 0.62f, 0.64f, 0.66f },
    { ITEM_TOOL_IRON_AXE,        TOOL_AXE,     TIER_IRON,    "Iron Axe",        "铁斧",   "伐木工具: 砍伐树木与木板速度 10.0x",  "Logging tool: Chops trees and wood at 10.0x speed",  10.0f, 10.0f, 1.4f, 0.88f, 0.90f, 0.94f },
    { ITEM_TOOL_DIAMOND_AXE,     TOOL_AXE,     TIER_DIAMOND, "Diamond Axe",     "钻石斧", "神级伐木: 秒速砍伐木材速度 22.0x",  "Supreme logging: Chops wood instantly at 22.0x speed", 22.0f, 12.0f, 1.8f, 0.25f, 0.90f, 0.98f },

    // Swords: Heavy melee damage and massive knockback against Slimes, Bats, Zombies!
    { ITEM_TOOL_WOOD_SWORD,      TOOL_SWORD,   TIER_WOOD,    "Wooden Sword",    "木剑",   "近战武器: 挥砍伤害 +3 | 击退 1.5x",    "Melee weapon: Slash damage +3 | Knockback 1.5x",    1.0f,  8.0f, 1.5f, 0.68f, 0.46f, 0.24f },
    { ITEM_TOOL_STONE_SWORD,     TOOL_SWORD,   TIER_STONE,   "Stone Sword",     "石剑",   "近战武器: 挥砍伤害 +6 | 击退 1.8x",    "Melee weapon: Slash damage +6 | Knockback 1.8x",    1.0f, 11.0f, 1.8f, 0.62f, 0.64f, 0.66f },
    { ITEM_TOOL_IRON_SWORD,      TOOL_SWORD,   TIER_IRON,    "Iron Sword",      "铁剑",   "近战武器: 挥砍伤害 +10 | 击退 2.3x",   "Melee weapon: Slash damage +10 | Knockback 2.3x",   1.0f, 15.0f, 2.3f, 0.88f, 0.90f, 0.94f },
    { ITEM_TOOL_DIAMOND_SWORD,   TOOL_SWORD,   TIER_DIAMOND, "Diamond Sword",   "钻石剑", "至尊神剑: 挥砍伤害 +15 | 巨幅击退 3.2x", "Supreme sword: Slash damage +15 | Massive knockback 3.2x", 1.0f, 20.0f, 3.2f, 0.25f, 0.90f, 0.98f },

    // Shovels: Rapid dig on dirt, sand, clay, snow, mud
    { ITEM_TOOL_WOOD_SHOVEL,     TOOL_SHOVEL,  TIER_WOOD,    "Wooden Shovel",   "木铲",   "挖掘工具: 挖掘泥土与沙地速度 2.0x",   "Excavation tool: Digs dirt and sand at 2.0x speed",  2.0f,  6.0f, 1.0f, 0.68f, 0.46f, 0.24f },
    { ITEM_TOOL_STONE_SHOVEL,    TOOL_SHOVEL,  TIER_STONE,   "Stone Shovel",    "石铲",   "挖掘工具: 挖掘泥土与沙地速度 4.5x",   "Excavation tool: Digs dirt and sand at 4.5x speed",  4.5f,  7.0f, 1.1f, 0.62f, 0.64f, 0.66f },
    { ITEM_TOOL_IRON_SHOVEL,     TOOL_SHOVEL,  TIER_IRON,    "Iron Shovel",     "铁铲",   "挖掘工具: 挖掘泥土与沙地速度 10.0x",  "Excavation tool: Digs dirt and sand at 10.0x speed",  10.0f,  8.0f, 1.2f, 0.88f, 0.90f, 0.94f },
    { ITEM_TOOL_DIAMOND_SHOVEL,  TOOL_SHOVEL,  TIER_DIAMOND, "Diamond Shovel",  "钻石铲", "神级挖掘: 挖掘泥土与沙地速度 22.0x",  "Supreme excavation: Digs dirt and sand at 22.0x speed", 22.0f,  9.0f, 1.3f, 0.25f, 0.90f, 0.98f },
};

// ============================================================================
// Tool Helpers & Queries
// ============================================================================

static inline bool isToolItem(int itemId) {
    return (itemId >= ITEM_TOOL_WOOD_PICKAXE && itemId <= ITEM_TOOL_DIAMOND_SHOVEL);
}

static inline const ToolDef* getToolDef(int itemId) {
    if (isToolItem(itemId)) {
        return &g_ToolDefs[itemId - ITEM_TOOL_WOOD_PICKAXE];
    }
    return NULL;
}

static inline float getToolEfficiency(int heldItemId, int blockType) {
    const ToolDef* def = isToolItem(heldItemId) ? getToolDef(heldItemId) : NULL;

    // 1. Stone / Ore / Brick / Rock types
    if (blockType == BLOCK_STONE ||
        blockType == BLOCK_COAL_ORE ||
        blockType == BLOCK_IRON_ORE ||
        blockType == BLOCK_GOLD_ORE ||
        blockType == BLOCK_DIAMOND_ORE ||
        blockType == BLOCK_RUBY_ORE ||
        blockType == BLOCK_BRICK ||
        blockType == BLOCK_OBSIDIAN ||
        blockType == BLOCK_MAGMA ||
        blockType == BLOCK_MOSSY_STONE ||
        blockType == BLOCK_SANDSTONE) {
        if (def && def->type == TOOL_PICKAXE) {
            return def->speedMultiplier;
        }
        return 0.04f; // Heavy penalty: bare hands or wrong tool takes 25-40s to break stone/ores!
    }

    // 2. Wood / Plank / Furniture / Tree
    if (blockType == BLOCK_WOOD ||
        blockType == BLOCK_WOOD_PLANK ||
        blockType == BLOCK_LEAVES ||
        blockType == BLOCK_CACTUS ||
        blockType == BLOCK_WORKBENCH ||
        blockType == BLOCK_CHAIR ||
        blockType == BLOCK_TABLE ||
        blockType == BLOCK_DOOR ||
        blockType == BLOCK_BED ||
        blockType == BLOCK_CHEST ||
        blockType == BLOCK_BOOKSHELF ||
        blockType == BLOCK_CLOCK) {
        if (def && def->type == TOOL_AXE) {
            return def->speedMultiplier;
        }
        return 0.4f; // Bare hands or wrong tool on wood (~3s break time)
    }

    // 3. Dirt / Sand / Clay / Snow / Mud
    if (blockType == BLOCK_GRASS ||
        blockType == BLOCK_DIRT ||
        blockType == BLOCK_SAND ||
        blockType == BLOCK_SNOW ||
        blockType == BLOCK_ICE ||
        blockType == BLOCK_CLAY ||
        blockType == BLOCK_MUD) {
        if (def && def->type == TOOL_SHOVEL) {
            return def->speedMultiplier;
        }
        return 1.0f; // Bare hands on soil/sand (~0.4s break time)
    }

    // Default for remaining blocks (torches, glass, etc.)
    if (def) {
        return def->speedMultiplier;
    }
    return 1.0f;
}

static inline float getToolDamage(int heldItemId) {
    if (isToolItem(heldItemId)) {
        const ToolDef* def = getToolDef(heldItemId);
        if (def) return def->damage;
    }
    return 5.0f; // Base fist damage = 5.0
}

static inline float getToolKnockback(int heldItemId) {
    if (isToolItem(heldItemId)) {
        const ToolDef* def = getToolDef(heldItemId);
        if (def) return def->knockback;
    }
    return 1.0f;
}

// ============================================================================
// Armor System Architecture & Equipment Slots (4-Piece Armor: Wood, Stone, Iron, Diamond)
// ============================================================================

typedef enum {
    ARMOR_SLOT_HELMET = 0, // 头盔
    ARMOR_SLOT_CHEST  = 1, // 胸甲
    ARMOR_SLOT_LEGS   = 2, // 护腿
    ARMOR_SLOT_BOOTS  = 3, // 靴子
    ARMOR_SLOT_COUNT  = 4
} ArmorSlotType;

typedef struct {
    int id;
    int slot;  // ARMOR_SLOT_HELMET .. BOOTS
    int tier;  // TIER_WOOD .. TIER_DIAMOND
    const char* name;
    const char* chineseName;
    const char* description;
    const char* englishDescription;
    float reduction;
    int maxDurability;
    float r, g, b;
} ArmorDef;

static const ArmorDef g_ArmorDefs[ITEM_ARMOR_COUNT] = {
    // Wood Tier (Total 15% Reduction, Max Durability 60)
    { ITEM_ARMOR_WOOD_HELMET,    ARMOR_SLOT_HELMET, TIER_WOOD,    "Wooden Helmet",     "木头盔",   "基础防具: 伤害减免 +3% | 耐久 60",  "Basic armor: Damage reduction +3% | Durability 60",  0.03f, 60,   0.68f, 0.46f, 0.24f },
    { ITEM_ARMOR_WOOD_CHEST,     ARMOR_SLOT_CHEST,  TIER_WOOD,    "Wooden Chestplate", "木胸甲",   "基础防具: 伤害减免 +6% | 耐久 60",  "Basic armor: Damage reduction +6% | Durability 60",  0.06f, 60,   0.68f, 0.46f, 0.24f },
    { ITEM_ARMOR_WOOD_LEGS,      ARMOR_SLOT_LEGS,   TIER_WOOD,    "Wooden Leggings",   "木护腿",   "基础防具: 伤害减免 +4% | 耐久 60",  "Basic armor: Damage reduction +4% | Durability 60",  0.04f, 60,   0.68f, 0.46f, 0.24f },
    { ITEM_ARMOR_WOOD_BOOTS,     ARMOR_SLOT_BOOTS,  TIER_WOOD,    "Wooden Boots",      "木靴子",   "基础防具: 伤害减免 +2% | 耐久 60",  "Basic armor: Damage reduction +2% | Durability 60",  0.02f, 60,   0.68f, 0.46f, 0.24f },

    // Stone Tier (Total 30% Reduction, Max Durability 130)
    { ITEM_ARMOR_STONE_HELMET,   ARMOR_SLOT_HELMET, TIER_STONE,   "Stone Helmet",      "石头盔",   "坚固防具: 伤害减免 +6% | 耐久 130", "Sturdy armor: Damage reduction +6% | Durability 130", 0.06f, 130,  0.62f, 0.64f, 0.66f },
    { ITEM_ARMOR_STONE_CHEST,    ARMOR_SLOT_CHEST,  TIER_STONE,   "Stone Chestplate",  "石胸甲",   "坚固防具: 伤害减免 +12% | 耐久 130","Sturdy armor: Damage reduction +12% | Durability 130",0.12f, 130,  0.62f, 0.64f, 0.66f },
    { ITEM_ARMOR_STONE_LEGS,     ARMOR_SLOT_LEGS,   TIER_STONE,   "Stone Leggings",    "石护腿",   "坚固防具: 伤害减免 +8% | 耐久 130", "Sturdy armor: Damage reduction +8% | Durability 130", 0.08f, 130,  0.62f, 0.64f, 0.66f },
    { ITEM_ARMOR_STONE_BOOTS,    ARMOR_SLOT_BOOTS,  TIER_STONE,   "Stone Boots",       "石靴子",   "坚固防具: 伤害减免 +4% | 耐久 130", "Sturdy armor: Damage reduction +4% | Durability 130", 0.04f, 130,  0.62f, 0.64f, 0.66f },

    // Iron Tier (Total 50% Reduction, Max Durability 250)
    { ITEM_ARMOR_IRON_HELMET,    ARMOR_SLOT_HELMET, TIER_IRON,    "Iron Helmet",       "铁头盔",   "重型防具: 伤害减免 +10% | 耐久 250","Heavy armor: Damage reduction +10% | Durability 250",0.10f, 250,  0.88f, 0.90f, 0.94f },
    { ITEM_ARMOR_IRON_CHEST,     ARMOR_SLOT_CHEST,  TIER_IRON,    "Iron Chestplate",   "铁胸甲",   "重型防具: 伤害减免 +20% | 耐久 250","Heavy armor: Damage reduction +20% | Durability 250",0.20f, 250,  0.88f, 0.90f, 0.94f },
    { ITEM_ARMOR_IRON_LEGS,      ARMOR_SLOT_LEGS,   TIER_IRON,    "Iron Leggings",     "铁护腿",   "重型防具: 伤害减免 +13% | 耐久 250","Heavy armor: Damage reduction +13% | Durability 250",0.13f, 250,  0.88f, 0.90f, 0.94f },
    { ITEM_ARMOR_IRON_BOOTS,     ARMOR_SLOT_BOOTS,  TIER_IRON,    "Iron Boots",        "铁靴子",   "重型防具: 伤害减免 +7% | 耐久 250", "Heavy armor: Damage reduction +7% | Durability 250", 0.07f, 250,  0.88f, 0.90f, 0.94f },

    // Diamond Tier (Total 75% Reduction, Max Durability 1560)
    { ITEM_ARMOR_DIAMOND_HELMET, ARMOR_SLOT_HELMET, TIER_DIAMOND, "Diamond Helmet",    "钻石头盔", "至尊防具: 伤害减免 +15% | 耐久 1560","Supreme armor: Damage reduction +15% | Durability 1560", 0.15f, 1560, 0.25f, 0.90f, 0.98f },
    { ITEM_ARMOR_DIAMOND_CHEST,  ARMOR_SLOT_CHEST,  TIER_DIAMOND, "Diamond Chestplate","钻石胸甲", "至尊防具: 伤害减免 +30% | 耐久 1560","Supreme armor: Damage reduction +30% | Durability 1560", 0.30f, 1560, 0.25f, 0.90f, 0.98f },
    { ITEM_ARMOR_DIAMOND_LEGS,   ARMOR_SLOT_LEGS,   TIER_DIAMOND, "Diamond Leggings",  "钻石护腿", "至尊防具: 伤害减免 +20% | 耐久 1560","Supreme armor: Damage reduction +20% | Durability 1560", 0.20f, 1560, 0.25f, 0.90f, 0.98f },
    { ITEM_ARMOR_DIAMOND_BOOTS,  ARMOR_SLOT_BOOTS,  TIER_DIAMOND, "Diamond Boots",     "钻石靴子", "至尊防具: 伤害减免 +10% | 耐久 1560","Supreme armor: Damage reduction +10% | Durability 1560", 0.10f, 1560, 0.25f, 0.90f, 0.98f },
};

static inline bool isArmorItem(int itemId) {
    return (itemId >= ITEM_ARMOR_WOOD_HELMET && itemId <= ITEM_ARMOR_DIAMOND_BOOTS);
}

static inline int getArmorSlot(int itemId) {
    if (!isArmorItem(itemId)) return -1;
    return (itemId - ITEM_ARMOR_WOOD_HELMET) % 4;
}

static inline int getArmorTier(int itemId) {
    if (!isArmorItem(itemId)) return TIER_NONE;
    return TIER_WOOD + (itemId - ITEM_ARMOR_WOOD_HELMET) / 4;
}

static inline const ArmorDef* getArmorDef(int itemId) {
    if (isArmorItem(itemId)) {
        return &g_ArmorDefs[itemId - ITEM_ARMOR_WOOD_HELMET];
    }
    return NULL;
}

static inline int getMaxDurability(int itemId) {
    if (isToolItem(itemId)) {
        const ToolDef* def = getToolDef(itemId);
        if (def) {
            switch (def->tier) {
                case TIER_WOOD:    return 60;
                case TIER_STONE:   return 130;
                case TIER_IRON:    return 250;
                case TIER_DIAMOND: return 1560;
                default: break;
            }
        }
    } else if (isArmorItem(itemId)) {
        const ArmorDef* def = getArmorDef(itemId);
        if (def) return def->maxDurability;
    }
    return 0;
}

static inline float getArmorPieceReduction(int itemId) {
    if (!isArmorItem(itemId)) return 0.0f;
    const ArmorDef* def = getArmorDef(itemId);
    return def ? def->reduction : 0.0f;
}

static inline float getPlayerArmorReduction(void) {
    float total = 0.0f;
    for (int i = 0; i < ARMOR_SLOT_COUNT; i++) {
        if (g_EquippedArmor[i] != 0 && g_ArmorDurability[i] > 0) {
            total += getArmorPieceReduction(g_EquippedArmor[i]);
        }
    }
    if (total > 0.75f) total = 0.75f;
    return total;
}

static inline const char* getArmorDescription(const ArmorDef* def) {
    if (!def) return "";
    return (getLanguage() == LANG_ZH) ? def->description : (def->englishDescription ? def->englishDescription : def->description);
}

static inline const char* getToolDescription(const ToolDef* def) {
    if (!def) return "";
    return (getLanguage() == LANG_ZH) ? def->description : (def->englishDescription ? def->englishDescription : def->description);
}

static inline const char* getItemName(int itemId) {
    if (isToolItem(itemId)) {
        const ToolDef* def = getToolDef(itemId);
        if (def) return def->name;
    } else if (isArmorItem(itemId)) {
        const ArmorDef* def = getArmorDef(itemId);
        if (def) return def->name;
    } else if (itemId == ITEM_STICK) {
        return "Stick";
    } else if (itemId == ITEM_GEL) {
        return "Slime Gel";
    } else if (itemId == ITEM_STICKY_BOMB) {
        return "Sticky Bomb";
    } else if (itemId >= 0 && itemId < BLOCK_TYPE_COUNT) {
        return g_BlockDefs[itemId].name;
    }
    return "Air";
}

static inline const char* getItemChineseName(int itemId) {
    if (isToolItem(itemId)) {
        const ToolDef* def = getToolDef(itemId);
        if (def) return def->chineseName;
    } else if (isArmorItem(itemId)) {
        const ArmorDef* def = getArmorDef(itemId);
        if (def) return def->chineseName;
    } else if (itemId == ITEM_STICK) {
        return "木棍";
    } else if (itemId == ITEM_GEL) {
        return "史莱姆凝胶";
    } else if (itemId == ITEM_STICKY_BOMB) {
        return "粘性炸弹";
    } else if (itemId >= 0 && itemId < BLOCK_TYPE_COUNT) {
        return g_BlockDefs[itemId].chineseName;
    }
    return "空气";
}

static inline const char* getItemLocalizedName(int itemId) {
    if (getLanguage() == LANG_ZH) {
        return getItemChineseName(itemId);
    } else {
        return getItemName(itemId);
    }
}

static inline void getItemColor(int itemId, float* r, float* g, float* b) {
    if (isToolItem(itemId)) {
        const ToolDef* def = getToolDef(itemId);
        if (def) {
            *r = def->r; *g = def->g; *b = def->b;
            return;
        }
    } else if (isArmorItem(itemId)) {
        const ArmorDef* def = getArmorDef(itemId);
        if (def) {
            *r = def->r; *g = def->g; *b = def->b;
            return;
        }
    } else if (itemId == ITEM_STICK) {
        *r = 0.60f; *g = 0.42f; *b = 0.22f;
        return;
    } else if (itemId == ITEM_GEL) {
        *r = 0.25f; *g = 0.85f; *b = 0.45f;
        return;
    } else if (itemId == ITEM_STICKY_BOMB) {
        *r = 0.35f; *g = 0.70f; *b = 0.30f;
        return;
    } else if (itemId >= 0 && itemId < BLOCK_TYPE_COUNT) {
        *r = g_BlockDefs[itemId].r;
        *g = g_BlockDefs[itemId].g;
        *b = g_BlockDefs[itemId].b;
        return;
    }
    *r = 0.5f; *g = 0.5f; *b = 0.5f;
}

// ============================================================================
// Creative Mode Inventory Data & Tabs
// ============================================================================

typedef enum {
    CREATIVE_TAB_BLOCKS = 0,    // 🧱 建筑方块 (Building Blocks)
    CREATIVE_TAB_TOOLS,         // ⚔️ 工具武器 (Tools & Weapons)
    CREATIVE_TAB_MATERIALS,     // 💎 矿物材料 (Ores & Materials)
    CREATIVE_TAB_DECOR,         // 💡 装饰光源 (Decor & Lighting)
    CREATIVE_TAB_COUNT
} CreativeTabType;

static inline const char* getCreativeTabName(int tab) {
    switch (tab) {
        case CREATIVE_TAB_BLOCKS:    return LOC("建筑方块", "Building Blocks");
        case CREATIVE_TAB_TOOLS:     return LOC("工具武器", "Tools & Weapons");
        case CREATIVE_TAB_MATERIALS: return LOC("矿物材料", "Ores & Materials");
        case CREATIVE_TAB_DECOR:     return LOC("装饰光源", "Decor & Lighting");
        default: return "";
    }
}

// Item ID lists for each tab:
static const int g_TabBlocksItems[] = {
    BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_WOOD, BLOCK_LEAVES,
    BLOCK_SAND, BLOCK_SANDSTONE, BLOCK_SNOW, BLOCK_ICE, BLOCK_CLAY,
    BLOCK_BRICK, BLOCK_WOOD_PLANK, BLOCK_GLASS, BLOCK_MOSSY_STONE,
    BLOCK_MUD, BLOCK_OBSIDIAN, BLOCK_BEDROCK
};
static const int g_TabBlocksCount = sizeof(g_TabBlocksItems) / sizeof(g_TabBlocksItems[0]);

static const int g_TabToolsItems[] = {
    // Pickaxes
    ITEM_TOOL_WOOD_PICKAXE, ITEM_TOOL_STONE_PICKAXE, ITEM_TOOL_IRON_PICKAXE, ITEM_TOOL_DIAMOND_PICKAXE,
    // Axes
    ITEM_TOOL_WOOD_AXE, ITEM_TOOL_STONE_AXE, ITEM_TOOL_IRON_AXE, ITEM_TOOL_DIAMOND_AXE,
    // Swords
    ITEM_TOOL_WOOD_SWORD, ITEM_TOOL_STONE_SWORD, ITEM_TOOL_IRON_SWORD, ITEM_TOOL_DIAMOND_SWORD,
    // Shovels
    ITEM_TOOL_WOOD_SHOVEL, ITEM_TOOL_STONE_SHOVEL, ITEM_TOOL_IRON_SHOVEL, ITEM_TOOL_DIAMOND_SHOVEL,
    // Armor Pieces
    ITEM_ARMOR_WOOD_HELMET, ITEM_ARMOR_WOOD_CHEST, ITEM_ARMOR_WOOD_LEGS, ITEM_ARMOR_WOOD_BOOTS,
    ITEM_ARMOR_STONE_HELMET, ITEM_ARMOR_STONE_CHEST, ITEM_ARMOR_STONE_LEGS, ITEM_ARMOR_STONE_BOOTS,
    ITEM_ARMOR_IRON_HELMET, ITEM_ARMOR_IRON_CHEST, ITEM_ARMOR_IRON_LEGS, ITEM_ARMOR_IRON_BOOTS,
    ITEM_ARMOR_DIAMOND_HELMET, ITEM_ARMOR_DIAMOND_CHEST, ITEM_ARMOR_DIAMOND_LEGS, ITEM_ARMOR_DIAMOND_BOOTS
};
static const int g_TabToolsCount = sizeof(g_TabToolsItems) / sizeof(g_TabToolsItems[0]);

static const int g_TabMaterialsItems[] = {
    BLOCK_COAL_ORE, BLOCK_IRON_ORE, BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE, BLOCK_RUBY_ORE, BLOCK_MAGMA,
    ITEM_STICK, ITEM_GEL, ITEM_STICKY_BOMB
};
static const int g_TabMaterialsCount = sizeof(g_TabMaterialsItems) / sizeof(g_TabMaterialsItems[0]);

static const int g_TabDecorItems[] = {
    BLOCK_TORCH, BLOCK_GLOW_SHROOM, BLOCK_CACTUS,
    BLOCK_WORKBENCH, BLOCK_CHAIR, BLOCK_TABLE, BLOCK_DOOR, BLOCK_BED,
    BLOCK_FURNACE, BLOCK_CHEST, BLOCK_BOOKSHELF, BLOCK_CLOCK, BLOCK_CHANDELIER
};
static const int g_TabDecorCount = sizeof(g_TabDecorItems) / sizeof(g_TabDecorItems[0]);

// ============================================================================
// Creative Inventory UI State
// ============================================================================

static bool g_CreativeInventoryOpen = false;
static int  g_CreativeActiveTab = CREATIVE_TAB_BLOCKS;
static int  g_CreativeHoveredItem = -1;
static int  g_CreativeHoveredHotbar = -1;

static inline void toggleCreativeInventory(void) {
    g_CreativeInventoryOpen = !g_CreativeInventoryOpen;
    if (g_CreativeInventoryOpen) {
        showNotification(LOC("创造模式大背包已打开 [按 E 或 ESC 关闭]", "Creative Inventory Opened [E / ESC to Close]"), 0.35f, 0.90f, 1.0f);
    }
}

static inline void openCreativeInventory(void) {
    g_CreativeInventoryOpen = true;
}

static inline void closeCreativeInventory(void) {
    g_CreativeInventoryOpen = false;
}

static inline bool isCreativeInventoryOpen(void) {
    return g_CreativeInventoryOpen;
}

// ============================================================================
// Tool Swing & Attack Animation State
// ============================================================================

static float g_ToolSwingTimer = 0.0f;
static bool  g_ToolIsSwinging = false;
static const float TOOL_SWING_DURATION = 0.22f; // Snappy, punchy 0.22s swing duration

static inline void updateToolSwing(float dt, bool isActionActive) {
    if (isActionActive) {
        g_ToolIsSwinging = true;
        g_ToolSwingTimer += dt / TOOL_SWING_DURATION;
        if (g_ToolSwingTimer >= 1.0f) {
            g_ToolSwingTimer = fmodf(g_ToolSwingTimer, 1.0f);
        }
    } else {
        if (g_ToolIsSwinging) {
            g_ToolSwingTimer += dt / TOOL_SWING_DURATION;
            if (g_ToolSwingTimer >= 1.0f) {
                g_ToolIsSwinging = false;
                g_ToolSwingTimer = 0.0f;
            }
        }
    }
}

// Calculates dynamic swinging arm angle:
// Rest angle is -15 deg.
// 0.00 to 0.20: Wind-up pulls UP and BACK (+40 deg to +75 deg).
// 0.20 to 0.65: Slashes FORWARD and DOWN (+75 deg to -65 deg) with powerful cubic acceleration.
// 0.65 to 1.00: Clean recovery back to -15 deg resting position.
static inline float getArmSwingAngle(float t, bool isSwinging, float walkCycle) {
    if (!isSwinging) {
        // Natural resting angle with walking swing
        return -15.0f + sinf(walkCycle) * 12.0f;
    }
    // Dynamic swing arc:
    // 0.00 to 0.20: Wind-up (pull back & up from -15 deg to +75 deg)
    if (t < 0.20f) {
        float p = t / 0.20f;
        return -15.0f + 90.0f * (p * p); // Peaks at +75 deg
    }
    // 0.20 to 0.65: Powerful forward-down slash (+75 deg to -65 deg with cubic acceleration)
    else if (t < 0.65f) {
        float p = (t - 0.20f) / 0.45f;
        float ease = 1.0f - powf(1.0f - p, 3.0f); // Ease out cubic: explosive initial acceleration
        return 75.0f - 140.0f * ease; // Slashes down to -65 deg!
    }
    // 0.65 to 1.00: Clean recovery back to resting -15 deg
    else {
        float p = (t - 0.65f) / 0.35f;
        float ease = p * (2.0f - p); // Smooth ease out recovery
        return -65.0f + 50.0f * ease; // Recovers to -15 deg
    }
}

// ============================================================================
// 2D Item & Tool Icon Rendering (Standalone Immediate-Mode)
// ============================================================================

static void drawInventoryItemIcon(float x, float y, float size, int itemId) {
    if (itemId == BLOCK_AIR || itemId == 0) return;

    GLuint atlas = getAtlasTextureGL();
    int texIdx = -1;

    if (itemId < BLOCK_TYPE_COUNT) {
        texIdx = GetBlockFaceTextureIndex(itemId, 0);
    } else {
        switch (itemId) {
            case ITEM_STICK:                texIdx = TILE_STICK; break;
            case ITEM_TOOL_WOOD_PICKAXE:    texIdx = TILE_WOOD_PICKAXE; break;
            case ITEM_TOOL_WOOD_AXE:        texIdx = TILE_WOOD_AXE; break;
            case ITEM_TOOL_WOOD_SWORD:      texIdx = TILE_WOOD_SWORD; break;
            case ITEM_TOOL_WOOD_SHOVEL:     texIdx = TILE_WOOD_SHOVEL; break;
            case ITEM_TOOL_STONE_PICKAXE:   texIdx = TILE_STONE_PICKAXE; break;
            case ITEM_TOOL_STONE_AXE:       texIdx = TILE_STONE_AXE; break;
            case ITEM_TOOL_STONE_SWORD:     texIdx = TILE_STONE_SWORD; break;
            case ITEM_TOOL_IRON_PICKAXE:    texIdx = TILE_IRON_PICKAXE; break;
            case ITEM_TOOL_IRON_AXE:        texIdx = TILE_IRON_AXE; break;
            case ITEM_TOOL_IRON_SWORD:      texIdx = TILE_IRON_SWORD; break;
            case ITEM_TOOL_DIAMOND_PICKAXE: texIdx = TILE_DIAMOND_PICKAXE; break;
            case ITEM_TOOL_DIAMOND_SWORD:   texIdx = TILE_DIAMOND_SWORD; break;
            case ITEM_GEL:                  texIdx = TILE_GEL; break;
            case ITEM_STICKY_BOMB:          texIdx = TILE_STICKY_BOMB; break;
            default: break;
        }
    }

    if (atlas != 0 && texIdx >= 0) {
        float u0, v0, u1, v1;
        GetAtlasTileUV(texIdx, &u0, &v0, &u1, &v1);

        float pad = size * 0.08f;
        float qx = x + pad;
        float qy = y + pad;
        float qw = size - pad * 2.0f;
        float qh = size - pad * 2.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, atlas);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(u0, v1); glVertex2f(qx, qy);
        glTexCoord2f(u1, v1); glVertex2f(qx + qw, qy);
        glTexCoord2f(u1, v0); glVertex2f(qx + qw, qy + qh);
        glTexCoord2f(u0, v0); glVertex2f(qx, qy + qh);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        if (itemId < BLOCK_TYPE_COUNT && g_BlockDefs[itemId].isSolid) {
            menuDrawBorder(qx, qy, qw, qh, 0.12f, 0.14f, 0.18f, 1.0f);
        }
        return;
    }

    if (isToolItem(itemId)) {
        // High-Quality Stylized Tool Icon
        const ToolDef* def = getToolDef(itemId);
        if (!def) return;

        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        float tr = def->r;
        float tg = def->g;
        float tb = def->b;

        // Wooden handle (from bottom-left to top-right)
        float hx0 = cx - size * 0.32f, hy0 = cy + size * 0.32f;
        float hx1 = cx + size * 0.18f, hy1 = cy - size * 0.18f;

        glLineWidth(3.5f);
        glColor3f(0.48f, 0.32f, 0.16f); // Handle dark wood
        glBegin(GL_LINES);
        glVertex2f(hx0, hy0); glVertex2f(hx1, hy1);
        glEnd();

        glLineWidth(2.0f);
        glColor3f(0.68f, 0.48f, 0.26f); // Handle light wood
        glBegin(GL_LINES);
        glVertex2f(hx0 + 0.8f, hy0 - 0.8f); glVertex2f(hx1 + 0.8f, hy1 - 0.8f);
        glEnd();

        // Tool Head Geometry based on type
        if (def->type == TOOL_PICKAXE) {
            // Pickaxe Curved Head
            float px = hx1, py = hy1;
            glColor3f(tr, tg, tb);
            glLineWidth(4.0f);
            glBegin(GL_LINES);
            glVertex2f(px - size * 0.22f, py - size * 0.08f);
            glVertex2f(px + size * 0.12f, py - size * 0.24f);
            glEnd();

            // Sharp beak points
            glPointSize(4.0f);
            glBegin(GL_POINTS);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(px - size * 0.22f, py - size * 0.08f);
            glVertex2f(px + size * 0.12f, py - size * 0.24f);
            glEnd();
        } else if (def->type == TOOL_AXE) {
            // Axe Broad Wedge Blade
            float ax = hx1, ay = hy1;
            glColor3f(tr, tg, tb);
            glBegin(GL_TRIANGLES);
            glVertex2f(ax, ay);
            glVertex2f(ax - size * 0.05f, ay - size * 0.25f);
            glVertex2f(ax + size * 0.22f, ay - size * 0.10f);
            glEnd();

            // Razor edge highlight
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(1.8f);
            glBegin(GL_LINES);
            glVertex2f(ax - size * 0.05f, ay - size * 0.25f);
            glVertex2f(ax + size * 0.22f, ay - size * 0.10f);
            glEnd();
        } else if (def->type == TOOL_SWORD) {
            // Sword Long Straight Gleaming Blade
            float sx0 = cx - size * 0.08f, sy0 = cy + size * 0.08f;
            float sx1 = cx + size * 0.32f, sy1 = cy - size * 0.32f;

            // Brass crossguard
            glColor3f(0.95f, 0.82f, 0.22f);
            glLineWidth(4.0f);
            glBegin(GL_LINES);
            glVertex2f(sx0 - size * 0.12f, sy0 - size * 0.12f);
            glVertex2f(sx0 + size * 0.12f, sy0 + size * 0.12f);
            glEnd();

            // Blade body
            glColor3f(tr, tg, tb);
            glLineWidth(4.5f);
            glBegin(GL_LINES);
            glVertex2f(sx0, sy0); glVertex2f(sx1, sy1);
            glEnd();

            // Diamond tip specular gleam
            glColor3f(1.0f, 1.0f, 1.0f);
            glPointSize(3.5f);
            glBegin(GL_POINTS);
            glVertex2f(sx1, sy1);
            glEnd();
        } else if (def->type == TOOL_SHOVEL) {
            // Shovel Spade Scoop
            float vx = hx1, vy = hy1;
            glColor3f(tr, tg, tb);
            glBegin(GL_TRIANGLES);
            glVertex2f(vx - size * 0.10f, vy - size * 0.10f);
            glVertex2f(vx + size * 0.18f, vy - size * 0.18f);
            glVertex2f(vx + size * 0.14f, vy - size * 0.28f);
            glEnd();
        }
    } else if (itemId == ITEM_STICK) {
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        glLineWidth(3.5f);
        glColor3f(0.58f, 0.38f, 0.20f);
        glBegin(GL_LINES);
        glVertex2f(cx - size * 0.30f, cy + size * 0.30f);
        glVertex2f(cx + size * 0.30f, cy - size * 0.30f);
        glEnd();
        glLineWidth(1.0f);
    } else if (itemId == ITEM_GEL) {
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        glColor4f(0.25f, 0.88f, 0.45f, 0.90f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int a = 0; a <= 12; a++) {
            float rad = (float)a * (float)M_PI * 2.0f / 12.0f;
            glVertex2f(cx + cosf(rad) * (size * 0.30f), cy + sinf(rad) * (size * 0.28f));
        }
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        glVertex2f(cx - size * 0.08f, cy - size * 0.08f);
        glEnd();
    } else if (itemId == ITEM_STICKY_BOMB) {
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;
        glColor3f(0.22f, 0.24f, 0.28f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int a = 0; a <= 12; a++) {
            float rad = (float)a * (float)M_PI * 2.0f / 12.0f;
            glVertex2f(cx + cosf(rad) * (size * 0.30f), cy + sinf(rad) * (size * 0.30f));
        }
        glEnd();
        glColor3f(0.25f, 0.88f, 0.45f);
        glPointSize(3.5f);
        glBegin(GL_POINTS);
        glVertex2f(cx - size * 0.18f, cy + size * 0.12f);
        glVertex2f(cx + size * 0.18f, cy + size * 0.12f);
        glEnd();
        glColor3f(1.0f, 0.65f, 0.1f);
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        glVertex2f(cx, cy - size * 0.35f);
        glEnd();
    } else if (isArmorItem(itemId)) {
        const ArmorDef* adef = getArmorDef(itemId);
        if (!adef) return;
        float ar = adef->r, ag = adef->g, ab = adef->b;
        int aslot = getArmorSlot(itemId);
        float cx = x + size * 0.5f;
        float cy = y + size * 0.5f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (aslot == ARMOR_SLOT_HELMET) {
            // 1. Helmet: Skull cap + cheek guards + visor
            menuDrawQuad(cx - size * 0.26f, cy - size * 0.32f, size * 0.52f, size * 0.38f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx - size * 0.26f, cy - size * 0.32f, size * 0.52f, size * 0.38f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            menuDrawQuad(cx - size * 0.26f, cy + size * 0.06f, size * 0.14f, size * 0.22f, ar, ag, ab, 1.0f);
            menuDrawQuad(cx + size * 0.12f, cy + size * 0.06f, size * 0.14f, size * 0.22f, ar, ag, ab, 1.0f);
            menuDrawQuad(cx - size * 0.20f, cy - size * 0.04f, size * 0.40f, size * 0.08f, 0.10f, 0.12f, 0.16f, 1.0f);
            glColor3f(1.0f, 0.85f, 0.3f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex2f(cx, cy - size * 0.36f);
            glVertex2f(cx, cy - size * 0.16f);
            glEnd();
        } else if (aslot == ARMOR_SLOT_CHEST) {
            // 2. Chestplate: Shoulder pauldrons + breastplate
            menuDrawQuad(cx - size * 0.28f, cy - size * 0.18f, size * 0.56f, size * 0.48f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx - size * 0.28f, cy - size * 0.18f, size * 0.56f, size * 0.48f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            menuDrawQuad(cx - size * 0.36f, cy - size * 0.32f, size * 0.20f, size * 0.18f, ar * 0.85f, ag * 0.85f, ab * 0.85f, 1.0f);
            menuDrawQuad(cx + size * 0.16f, cy - size * 0.32f, size * 0.20f, size * 0.18f, ar * 0.85f, ag * 0.85f, ab * 0.85f, 1.0f);
            menuDrawQuad(cx - size * 0.12f, cy - size * 0.32f, size * 0.24f, size * 0.14f, 0.08f, 0.10f, 0.14f, 1.0f);
            glColor3f(1.0f, 0.85f, 0.3f);
            glLineWidth(1.8f);
            glBegin(GL_LINES);
            glVertex2f(cx, cy - size * 0.14f);
            glVertex2f(cx, cy + size * 0.24f);
            glEnd();
        } else if (aslot == ARMOR_SLOT_LEGS) {
            // 3. Leggings: Waist belt + two leg greaves
            menuDrawQuad(cx - size * 0.28f, cy - size * 0.30f, size * 0.56f, size * 0.14f, ar * 0.75f, ag * 0.75f, ab * 0.75f, 1.0f);
            menuDrawQuad(cx - size * 0.06f, cy - size * 0.30f, size * 0.12f, size * 0.14f, 1.0f, 0.85f, 0.3f, 1.0f);
            menuDrawQuad(cx - size * 0.26f, cy - size * 0.14f, size * 0.22f, size * 0.44f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx - size * 0.26f, cy - size * 0.14f, size * 0.22f, size * 0.44f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            menuDrawQuad(cx + size * 0.04f, cy - size * 0.14f, size * 0.22f, size * 0.44f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx + size * 0.04f, cy - size * 0.14f, size * 0.22f, size * 0.44f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            glColor3f(1.0f, 0.85f, 0.3f);
            glLineWidth(1.8f);
            glBegin(GL_LINES);
            glVertex2f(cx - size * 0.24f, cy + size * 0.08f);
            glVertex2f(cx - size * 0.06f, cy + size * 0.08f);
            glVertex2f(cx + size * 0.06f, cy + size * 0.08f);
            glVertex2f(cx + size * 0.24f, cy + size * 0.08f);
            glEnd();
        } else if (aslot == ARMOR_SLOT_BOOTS) {
            // 4. Boots: Pair of armored boots / sabatons
            menuDrawQuad(cx - size * 0.32f, cy - size * 0.16f, size * 0.18f, size * 0.26f, ar, ag, ab, 1.0f);
            menuDrawQuad(cx - size * 0.36f, cy + size * 0.10f, size * 0.26f, size * 0.18f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx - size * 0.36f, cy + size * 0.10f, size * 0.26f, size * 0.18f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            menuDrawQuad(cx + size * 0.14f, cy - size * 0.16f, size * 0.18f, size * 0.26f, ar, ag, ab, 1.0f);
            menuDrawQuad(cx + size * 0.10f, cy + size * 0.10f, size * 0.26f, size * 0.18f, ar, ag, ab, 1.0f);
            menuDrawBorder(cx + size * 0.10f, cy + size * 0.10f, size * 0.26f, size * 0.18f, ar * 0.6f, ag * 0.6f, ab * 0.6f, 1.0f);
            menuDrawQuad(cx - size * 0.36f, cy + size * 0.24f, size * 0.26f, size * 0.06f, 0.12f, 0.14f, 0.18f, 1.0f);
            menuDrawQuad(cx + size * 0.10f, cy + size * 0.24f, size * 0.26f, size * 0.06f, 0.12f, 0.14f, 0.18f, 1.0f);
        }
    }
}

// ============================================================================
// In-Hand Tool 2.5D Model Rendering with Dynamic Swinging Animation & Motion Trail
// ============================================================================

void renderInHandTool(int heldItemId, float swingProgress, bool isSwinging, bool isFacingRight) {
    (void)isFacingRight;
    float armAngle = getArmSwingAngle(swingProgress, isSwinging, g_Player.walkCycle);

    // Front shoulder pivot is at (0.18f, 1.05f, 0.04f) in player local space
    // 0. Dynamic Motion Arc / Slashing Swoosh Trail (rendered in shoulder space during downward slash)
    if (isSwinging && swingProgress >= 0.18f && swingProgress <= 0.70f) {
        float trailProgress = (swingProgress - 0.18f) / 0.52f;
        float trailAlpha = sinf(trailProgress * (float)M_PI) * 0.60f;
        float tr = 0.9f, tg = 0.9f, tb = 0.95f;
        if (isToolItem(heldItemId)) {
            const ToolDef* tdef = getToolDef(heldItemId);
            if (tdef) { tr = tdef->r; tg = tdef->g; tb = tdef->b; }
        }

        glPushMatrix();
        glTranslatef(0.18f, 1.05f, 0.01f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous slash trail

        float curA = armAngle;
        float lagA = curA + 42.0f; // Trail sweeps 42 degrees behind the slash direction
        int segments = 10;
        float rInner = 0.55f;
        float rOuter = 1.02f;

        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segments; s++) {
            float frac = (float)s / (float)segments;
            float angDeg = curA + (lagA - curA) * frac;
            float rad = angDeg * (float)M_PI / 180.0f;
            float cosA = cosf(rad);
            float sinA = sinf(rad);
            float fade = trailAlpha * (1.0f - frac);

            glColor4f(tr, tg, tb, fade);
            glVertex3f(sinA * rOuter, -cosA * rOuter, 0.0f);
            glColor4f(tr * 0.7f, tg * 0.7f, tb * 0.7f, 0.0f);
            glVertex3f(sinA * rInner, -cosA * rInner, 0.0f);
        }
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(0.18f, 1.05f, 0.04f);
    glRotatef(armAngle, 0.0f, 0.0f, 1.0f);

    // 1. Adventurer Arm (Sleeve + Cuffed Forearm + Glove Hand)
    // Blue adventurer sleeve (shoulder / bicep)
    glColor3f(0.20f, 0.42f, 0.78f);
    glBegin(GL_QUADS);
    glVertex3f(-0.085f, -0.22f, 0.0f);
    glVertex3f( 0.085f, -0.22f, 0.0f);
    glVertex3f( 0.085f,  0.04f, 0.0f);
    glVertex3f(-0.085f,  0.04f, 0.0f);
    glEnd();

    // Sleeve cuff trim
    glColor3f(0.14f, 0.30f, 0.58f);
    glBegin(GL_QUADS);
    glVertex3f(-0.09f, -0.25f, 0.01f);
    glVertex3f( 0.09f, -0.25f, 0.01f);
    glVertex3f( 0.09f, -0.22f, 0.01f);
    glVertex3f(-0.09f, -0.22f, 0.01f);
    glEnd();

    // Forearm skin
    glColor3f(0.98f, 0.82f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(-0.07f, -0.34f, 0.0f);
    glVertex3f( 0.07f, -0.34f, 0.0f);
    glVertex3f( 0.07f, -0.25f, 0.0f);
    glVertex3f(-0.07f, -0.25f, 0.0f);
    glEnd();

    // Adventurer fingerless leather glove / hand
    glColor3f(0.38f, 0.22f, 0.12f);
    glBegin(GL_QUADS);
    glVertex3f(-0.075f, -0.42f, 0.01f);
    glVertex3f( 0.075f, -0.42f, 0.01f);
    glVertex3f( 0.075f, -0.34f, 0.01f);
    glVertex3f(-0.075f, -0.34f, 0.01f);
    glEnd();
    // Bare fingertips
    glColor3f(0.98f, 0.82f, 0.70f);
    glBegin(GL_QUADS);
    glVertex3f(-0.065f, -0.45f, 0.015f);
    glVertex3f( 0.065f, -0.45f, 0.015f);
    glVertex3f( 0.065f, -0.42f, 0.015f);
    glVertex3f(-0.065f, -0.42f, 0.015f);
    glEnd();

    // 2. Render In-Hand Tool or Block
    if (heldItemId != BLOCK_AIR && heldItemId != 0) {
        glPushMatrix();
        glTranslatef(0.0f, -0.38f, 0.02f);
        glRotatef(45.0f, 0.0f, 0.0f, 1.0f); // Default forward-upward grip angle

        if (isToolItem(heldItemId)) {
            const ToolDef* def = getToolDef(heldItemId);
            if (def) {
                float tr = def->r, tg = def->g, tb = def->b;

                // Wooden Handle (Shaft)
                glColor3f(0.55f, 0.36f, 0.18f);
                glBegin(GL_QUADS);
                glVertex3f(-0.03f, -0.12f, 0.0f);
                glVertex3f( 0.03f, -0.12f, 0.0f);
                glVertex3f( 0.03f,  0.46f, 0.0f);
                glVertex3f(-0.03f,  0.46f, 0.0f);
                glEnd();

                // Specific Tool Head 2.5D Geometry
                if (def->type == TOOL_PICKAXE) {
                    // Pickaxe Curved Head with Bevels
                    glColor3f(tr, tg, tb);
                    glBegin(GL_QUADS);
                    glVertex3f(-0.24f, 0.38f, 0.01f);
                    glVertex3f( 0.24f, 0.38f, 0.01f);
                    glVertex3f( 0.20f, 0.48f, 0.01f);
                    glVertex3f(-0.20f, 0.48f, 0.01f);
                    glEnd();

                    // Specular Highlight Ridge
                    glColor3f(fminf(tr + 0.3f, 1.0f), fminf(tg + 0.3f, 1.0f), fminf(tb + 0.3f, 1.0f));
                    glLineWidth(2.2f);
                    glBegin(GL_LINES);
                    glVertex3f(-0.24f, 0.38f, 0.02f);
                    glVertex3f( 0.24f, 0.38f, 0.02f);
                    glEnd();
                } else if (def->type == TOOL_AXE) {
                    // Axe Heavy Wedge Blade
                    glColor3f(tr, tg, tb);
                    glBegin(GL_QUADS);
                    glVertex3f( 0.02f, 0.30f, 0.01f);
                    glVertex3f( 0.26f, 0.26f, 0.01f);
                    glVertex3f( 0.28f, 0.48f, 0.01f);
                    glVertex3f( 0.02f, 0.46f, 0.01f);
                    glEnd();

                    // Axe Rear Hammer Poll
                    glColor3f(tr * 0.75f, tg * 0.75f, tb * 0.75f);
                    glBegin(GL_QUADS);
                    glVertex3f(-0.12f, 0.36f, 0.01f);
                    glVertex3f( 0.02f, 0.36f, 0.01f);
                    glVertex3f( 0.02f, 0.44f, 0.01f);
                    glVertex3f(-0.12f, 0.44f, 0.01f);
                    glEnd();

                    // Razor Blade Edge
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glLineWidth(2.0f);
                    glBegin(GL_LINES);
                    glVertex3f(0.26f, 0.26f, 0.02f);
                    glVertex3f(0.28f, 0.48f, 0.02f);
                    glEnd();
                } else if (def->type == TOOL_SWORD) {
                    // Brass Crossguard & Pommel
                    glColor3f(0.95f, 0.82f, 0.22f);
                    glBegin(GL_QUADS);
                    glVertex3f(-0.14f, 0.02f, 0.01f);
                    glVertex3f( 0.14f, 0.02f, 0.01f);
                    glVertex3f( 0.14f, 0.08f, 0.01f);
                    glVertex3f(-0.14f, 0.08f, 0.01f);
                    glEnd();

                    // Long Gleaming Straight Blade with Diamond Point
                    glColor3f(tr, tg, tb);
                    glBegin(GL_QUADS);
                    glVertex3f(-0.05f, 0.08f, 0.01f);
                    glVertex3f( 0.05f, 0.08f, 0.01f);
                    glVertex3f( 0.04f, 0.62f, 0.01f);
                    glVertex3f(-0.04f, 0.62f, 0.01f);
                    glEnd();

                    // Blade Diamond Tip
                    glBegin(GL_TRIANGLES);
                    glVertex3f(-0.04f, 0.62f, 0.01f);
                    glVertex3f( 0.04f, 0.62f, 0.01f);
                    glVertex3f( 0.0f,  0.72f, 0.01f);
                    glEnd();

                    // Center Fuller Reflection Line
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glLineWidth(1.8f);
                    glBegin(GL_LINES);
                    glVertex3f(0.0f, 0.10f, 0.02f);
                    glVertex3f(0.0f, 0.68f, 0.02f);
                    glEnd();
                } else if (def->type == TOOL_SHOVEL) {
                    // Shovel Spade Scoop Blade
                    glColor3f(tr, tg, tb);
                    glBegin(GL_QUADS);
                    glVertex3f(-0.10f, 0.38f, 0.01f);
                    glVertex3f( 0.10f, 0.38f, 0.01f);
                    glVertex3f( 0.08f, 0.58f, 0.01f);
                    glVertex3f(-0.08f, 0.58f, 0.01f);
                    glEnd();

                    glBegin(GL_TRIANGLES);
                    glVertex3f(-0.08f, 0.58f, 0.01f);
                    glVertex3f( 0.08f, 0.58f, 0.01f);
                    glVertex3f( 0.0f,  0.66f, 0.01f);
                    glEnd();
                }
            }
        } else if (heldItemId < BLOCK_TYPE_COUNT) {
            // In-Hand 2.5D Mini Block
            float br = g_BlockDefs[heldItemId].r;
            float bg = g_BlockDefs[heldItemId].g;
            float bb = g_BlockDefs[heldItemId].b;

            float sz = 0.22f;
            glColor3f(br, bg, bb);
            glBegin(GL_QUADS);
            glVertex3f(-sz * 0.5f,  0.0f, 0.0f);
            glVertex3f( sz * 0.5f,  0.0f, 0.0f);
            glVertex3f( sz * 0.5f,  sz,   0.0f);
            glVertex3f(-sz * 0.5f,  sz,   0.0f);
            glEnd();

            // Specular top edge
            glColor3f(fminf(br * 1.3f, 1.0f), fminf(bg * 1.3f, 1.0f), fminf(bb * 1.3f, 1.0f));
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex3f(-sz * 0.5f, sz, 0.01f);
            glVertex3f( sz * 0.5f, sz, 0.01f);
            glEnd();
        }

        glPopMatrix();
    }

    // 3. Dynamic Swinging Slash Arc Effect (Trail in front of blade during forward stroke)
    if (isSwinging && swingProgress >= 0.20f && swingProgress <= 0.72f) {
        float trailAlpha = (1.0f - fabsf(swingProgress - 0.45f) / 0.28f) * 0.65f;
        if (trailAlpha > 0.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            float tr = 0.95f, tg = 0.95f, tb = 1.0f;
            if (isToolItem(heldItemId)) {
                const ToolDef* def = getToolDef(heldItemId);
                if (def && def->tier == TIER_DIAMOND) {
                    tr = 0.35f; tg = 0.95f; tb = 1.0f; // Glowing cyan slash
                }
            }

            glColor4f(tr, tg, tb, trailAlpha);
            glLineWidth(3.0f);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 8; i++) {
                float a = (float)i / 8.0f;
                float arcAngle = 50.0f - 110.0f * a;
                float rad = arcAngle * (float)M_PI / 180.0f;
                float dist = 0.85f + 0.15f * sinf(a * (float)M_PI);
                glVertex3f(sinf(rad) * dist, -cosf(rad) * dist, 0.03f);
            }
            glEnd();
        }
    }

    glPopMatrix();
}

// ============================================================================
// Creative Mode Big Inventory UI (Opened/Closed with [E] Key)
// ============================================================================

void renderCreativeInventory(int screenW, int screenH, int mouseX, int mouseY) {
    if (!g_CreativeInventoryOpen) return;

    // Switch to 2D Orthographic Mode for Pixel-Perfect UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenW, screenH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Fullscreen Darkened Backdrop Overlay
    menuDrawQuad(0, 0, (float)screenW, (float)screenH, 0.0f, 0.0f, 0.0f, 0.72f);

    // 2. Centered Modal Window Frame
    float winW = 860.0f;
    float winH = 580.0f;
    float winX = ((float)screenW - winW) * 0.5f;
    float winY = ((float)screenH - winH) * 0.5f;

    // Window Stone Panel
    menuDrawQuad(winX, winY, winW, winH, 0.10f, 0.12f, 0.15f, 0.98f);
    menuDrawBorder(winX, winY, winW, winH, 0.35f, 0.42f, 0.52f, 2.5f);
    // Golden Filigree Inner Accent
    menuDrawBorder(winX + 4.0f, winY + 4.0f, winW - 8.0f, winH - 8.0f, 0.95f, 0.82f, 0.22f, 1.5f);

    // Corner Rivet Studs
    float cornerOffsets[4][2] = { { winX + 8.0f, winY + 8.0f }, { winX + winW - 16.0f, winY + 8.0f },
                                  { winX + 8.0f, winY + winH - 16.0f }, { winX + winW - 16.0f, winY + winH - 16.0f } };
    for (int c = 0; c < 4; c++) {
        menuDrawQuad(cornerOffsets[c][0], cornerOffsets[c][1], 8.0f, 8.0f, 1.0f, 0.85f, 0.30f, 1.0f);
    }

    // 3. Header Bar
    float headerH = 46.0f;
    menuDrawQuad(winX + 6.0f, winY + 6.0f, winW - 12.0f, headerH, 0.07f, 0.08f, 0.11f, 0.95f);
    menuDrawBorder(winX + 6.0f, winY + 6.0f, winW - 12.0f, headerH, 0.25f, 0.30f, 0.38f, 1.0f);

    // Header Title with Drop Shadow
    drawText(winX + 22.0f + 2.0f, winY + 28.0f + 2.0f, LOC("创造模式大背包", "Creative Inventory"), 0.0f, 0.0f, 0.0f, true);
    drawText(winX + 22.0f, winY + 28.0f, LOC("创造模式大背包", "Creative Inventory"), 1.0f, 0.86f, 0.25f, true);

    drawText(winX + 280.0f, winY + 30.0f, LOC("左键点击物品装备到快捷栏 [1-9]，按 [E] 或 [ESC] 关闭", "Click item to equip to Hotbar [1-9], press [E] or [ESC] to close"), 0.65f, 0.72f, 0.82f, false);

    // Close Button [X] at Top-Right
    float closeX = winX + winW - 38.0f;
    float closeY = winY + 10.0f;
    float closeS = 26.0f;
    bool closeHovered = (mouseX >= closeX && mouseX <= closeX + closeS && mouseY >= closeY && mouseY <= closeY + closeS);
    menuDrawQuad(closeX, closeY, closeS, closeS, closeHovered ? 0.90f : 0.65f, 0.20f, 0.20f, 1.0f);
    menuDrawBorder(closeX, closeY, closeS, closeS, 1.0f, 0.8f, 0.8f, 1.2f);
    drawText(closeX + 8.0f, closeY + 18.0f, "X", 1.0f, 1.0f, 1.0f, true);

    // 4. Categorized Tabs Bar
    float tabStartX = winX + 16.0f;
    float tabY = winY + headerH + 12.0f;
    float tabW = 196.0f;
    float tabH = 34.0f;
    float tabGap = 8.0f;

    for (int t = 0; t < CREATIVE_TAB_COUNT; t++) {
        float tx = tabStartX + t * (tabW + tabGap);
        bool isAct = (t == g_CreativeActiveTab);
        bool isHov = (mouseX >= tx && mouseX <= tx + tabW && mouseY >= tabY && mouseY <= tabY + tabH);

        const char* tabLabel = getCreativeTabName(t);
        if (isAct) {
            menuDrawQuad(tx, tabY, tabW, tabH, 0.20f, 0.24f, 0.32f, 1.0f);
            menuDrawBorder(tx, tabY, tabW, tabH, 1.0f, 0.85f, 0.25f, 2.0f);
            drawText(tx + 24.0f, tabY + 22.0f, tabLabel, 1.0f, 0.90f, 0.35f, true);
        } else {
            menuDrawQuad(tx, tabY, tabW, tabH, isHov ? 0.16f : 0.12f, isHov ? 0.19f : 0.14f, isHov ? 0.24f : 0.18f, 0.95f);
            menuDrawBorder(tx, tabY, tabW, tabH, isHov ? 0.45f : 0.25f, isHov ? 0.65f : 0.30f, isHov ? 0.85f : 0.38f, 1.0f);
            drawText(tx + 24.0f, tabY + 22.0f, tabLabel, isHov ? 0.90f : 0.65f, isHov ? 0.92f : 0.70f, isHov ? 0.95f : 0.76f, false);
        }
    }

    // 5. Item Grid Background Panel
    float gridX = winX + 16.0f;
    float gridY = tabY + tabH + 8.0f;
    float gridW = winW - 32.0f;
    float gridH = 310.0f;

    menuDrawQuad(gridX, gridY, gridW, gridH, 0.07f, 0.08f, 0.10f, 0.95f);
    menuDrawBorder(gridX, gridY, gridW, gridH, 0.28f, 0.32f, 0.40f, 1.5f);

    // Determine items list for current active tab
    const int* itemsList = NULL;
    int itemsCount = 0;
    if (g_CreativeActiveTab == CREATIVE_TAB_BLOCKS) {
        itemsList = g_TabBlocksItems;
        itemsCount = g_TabBlocksCount;
    } else if (g_CreativeActiveTab == CREATIVE_TAB_TOOLS) {
        itemsList = g_TabToolsItems;
        itemsCount = g_TabToolsCount;
    } else if (g_CreativeActiveTab == CREATIVE_TAB_MATERIALS) {
        itemsList = g_TabMaterialsItems;
        itemsCount = g_TabMaterialsCount;
    } else if (g_CreativeActiveTab == CREATIVE_TAB_DECOR) {
        itemsList = g_TabDecorItems;
        itemsCount = g_TabDecorCount;
    }

    // Render Grid Items
    int cols = 8;
    float slotSize = 68.0f;
    float slotGapX = 14.0f;
    float slotGapY = 14.0f;
    float startSlotX = gridX + 22.0f;
    float startSlotY = gridY + 16.0f;

    g_CreativeHoveredItem = -1;
    int hoveredItemId = -1;
    float tooltipX = (float)mouseX + 16.0f;
    float tooltipY = (float)mouseY + 16.0f;

    for (int i = 0; i < itemsCount; i++) {
        int r = i / cols;
        int c = i % cols;
        float sx = startSlotX + c * (slotSize + slotGapX);
        float sy = startSlotY + r * (slotSize + slotGapY);

        int curItemId = itemsList[i];
        bool isHovered = (mouseX >= sx && mouseX <= sx + slotSize && mouseY >= sy && mouseY <= sy + slotSize);

        // Slot Box
        if (isHovered) {
            menuDrawQuad(sx, sy, slotSize, slotSize, 0.22f, 0.28f, 0.36f, 1.0f);
            menuDrawBorder(sx, sy, slotSize, slotSize, 1.0f, 0.85f, 0.22f, 2.5f);
            g_CreativeHoveredItem = curItemId;
            hoveredItemId = curItemId;
        } else {
            menuDrawQuad(sx, sy, slotSize, slotSize, 0.13f, 0.15f, 0.20f, 0.95f);
            menuDrawBorder(sx, sy, slotSize, slotSize, 0.25f, 0.30f, 0.38f, 1.0f);
        }

        // Draw Item Icon
        drawInventoryItemIcon(sx + 8.0f, sy + 6.0f, slotSize - 16.0f, curItemId);

        // Item Subtitle / Count indicator
        if (isToolItem(curItemId)) {
            const ToolDef* def = getToolDef(curItemId);
            if (def) {
                const char* tierLabel = (getLanguage() == LANG_ZH) ?
                    (def->tier == TIER_DIAMOND ? "钻石" : (def->tier == TIER_IRON ? "铁质" : (def->tier == TIER_STONE ? "石质" : "木质"))) :
                    (def->tier == TIER_DIAMOND ? "DIA" : (def->tier == TIER_IRON ? "IRON" : (def->tier == TIER_STONE ? "STN" : "WOOD")));
                drawText(sx + 6.0f, sy + slotSize - 6.0f, tierLabel, def->r, def->g, def->b, false);
            }
        } else if (isArmorItem(curItemId)) {
            const ArmorDef* def = getArmorDef(curItemId);
            if (def) {
                const char* tierLabel = (getLanguage() == LANG_ZH) ?
                    (def->tier == TIER_DIAMOND ? "钻石" : (def->tier == TIER_IRON ? "铁质" : (def->tier == TIER_STONE ? "石质" : "木质"))) :
                    (def->tier == TIER_DIAMOND ? "DIA" : (def->tier == TIER_IRON ? "IRON" : (def->tier == TIER_STONE ? "STN" : "WOOD")));
                drawText(sx + 6.0f, sy + slotSize - 6.0f, tierLabel, def->r, def->g, def->b, false);
            }
        } else {
            // Infinite stack symbol for creative blocks
            drawText(sx + slotSize - 20.0f, sy + slotSize - 6.0f, "oo", 0.35f, 0.95f, 1.0f, false);
        }
    }

    // 6. Bottom Hotbar Equipment Bar
    float hotbarSectionY = gridY + gridH + 12.0f;
    drawText(winX + 20.0f, hotbarSectionY + 12.0f,
             LOC("当前手持快捷栏", "Hotbar Slots"),
             0.95f, 0.85f, 0.30f, false);

    float hbSlotSize = 48.0f;
    float hbGap = 8.0f;
    float hbStartX = winX + 22.0f;
    float hbY = hotbarSectionY + 22.0f;

    g_CreativeHoveredHotbar = -1;
    for (int h = 0; h < HOTBAR_SLOTS; h++) {
        float hx = hbStartX + h * (hbSlotSize + hbGap);
        bool isActSlot = (h == g_ActiveSlot);
        bool isHbHov = (mouseX >= hx && mouseX <= hx + hbSlotSize && mouseY >= hbY && mouseY <= hbY + hbSlotSize);

        if (isHbHov) g_CreativeHoveredHotbar = h;

        if (isActSlot) {
            menuDrawQuad(hx, hbY, hbSlotSize, hbSlotSize, 0.22f, 0.28f, 0.38f, 1.0f);
            menuDrawBorder(hx - 2.0f, hbY - 2.0f, hbSlotSize + 4.0f, hbSlotSize + 4.0f, 1.0f, 0.85f, 0.20f, 2.5f);
        } else {
            menuDrawQuad(hx, hbY, hbSlotSize, hbSlotSize, 0.12f, 0.15f, 0.20f, 0.95f);
            menuDrawBorder(hx, hbY, hbSlotSize, hbSlotSize, isHbHov ? 0.65f : 0.30f, isHbHov ? 0.75f : 0.35f, isHbHov ? 0.95f : 0.45f, 1.2f);
        }

        // Draw item in this hotbar slot
        int slotItem = g_Hotbar[h].type;
        if (slotItem != BLOCK_AIR && slotItem != 0 && g_Hotbar[h].count > 0) {
            drawInventoryItemIcon(hx + 6.0f, hbY + 6.0f, hbSlotSize - 12.0f, slotItem);

            char cBuf[16];
            if (isToolItem(slotItem)) {
                snprintf(cBuf, sizeof(cBuf), "x1");
            } else {
                snprintf(cBuf, sizeof(cBuf), "%d", g_Hotbar[h].count);
            }
            drawText(hx + hbSlotSize - 22.0f, hbY + hbSlotSize - 4.0f, cBuf, 1.0f, 1.0f, 1.0f, false);
        }

        // Slot number badge
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", h + 1);
        drawText(hx + 4.0f, hbY + 12.0f, numBuf, isActSlot ? 1.0f : 0.6f, isActSlot ? 0.85f : 0.6f, isActSlot ? 0.25f : 0.6f, false);
    }

    // Active Slot indicator banner
    char actInfoBuf[96];
    int curActItem = g_Hotbar[g_ActiveSlot].type;
    snprintf(actInfoBuf, sizeof(actInfoBuf), "%s [%d]: %s",
             LOC("当前手持快捷栏", "Hotbar Slot"),
             g_ActiveSlot + 1, getItemLocalizedName(curActItem));
    drawText(winX + 540.0f, hotbarSectionY + 36.0f, actInfoBuf, 0.35f, 0.90f, 1.0f, true);

    // Trash / Clear Button
    float trashX = winX + winW - 145.0f;
    float trashY = hbY + 8.0f;
    float trashW = 125.0f;
    float trashH = 34.0f;
    bool trashHovered = (mouseX >= trashX && mouseX <= trashX + trashW && mouseY >= trashY && mouseY <= trashY + trashH);
    menuDrawQuad(trashX, trashY, trashW, trashH, trashHovered ? 0.70f : 0.50f, 0.20f, 0.20f, 0.95f);
    menuDrawBorder(trashX, trashY, trashW, trashH, 0.95f, 0.40f, 0.40f, 1.5f);
    drawText(trashX + (getLanguage() == LANG_ZH ? 30.0f : 16.0f), trashY + 22.0f, LOC("清空槽位", "Clear Slot"), 1.0f, 0.95f, 0.95f, false);

    // 7. Floating Hover Tooltip
    if (hoveredItemId > 0) {
        float tipW = 360.0f;
        float tipH = 96.0f;
        if (tooltipX + tipW > (float)screenW - 20.0f) tooltipX = (float)screenW - tipW - 20.0f;
        if (tooltipY + tipH > (float)screenH - 20.0f) tooltipY = (float)screenH - tipH - 20.0f;

        menuDrawQuad(tooltipX, tooltipY, tipW, tipH, 0.06f, 0.08f, 0.11f, 0.98f);
        menuDrawBorder(tooltipX, tooltipY, tipW, tipH, 0.95f, 0.82f, 0.22f, 1.5f);

        // Name
        drawText(tooltipX + 12.0f, tooltipY + 22.0f, getItemLocalizedName(hoveredItemId), 1.0f, 0.90f, 0.30f, true);

        // Stats & Description
        if (isToolItem(hoveredItemId)) {
            const ToolDef* def = getToolDef(hoveredItemId);
            if (def) {
                drawText(tooltipX + 12.0f, tooltipY + 44.0f, getToolDescription(def), 0.85f, 0.90f, 0.95f, false);

                char statBuf[128];
                snprintf(statBuf, sizeof(statBuf), "%s%.0f | %s%.1fx | %s%.1fx",
                         LOC("攻击力: ", "Attack Damage: "), def->damage,
                         LOC("击退强度: ", "Knockback: "), def->knockback,
                         LOC("挖掘倍率: ", "Mining Speed: "), def->speedMultiplier);
                drawText(tooltipX + 12.0f, tooltipY + 64.0f, statBuf, def->r, def->g, def->b, false);
                drawText(tooltipX + 12.0f, tooltipY + 84.0f, LOC("左键点击装备到当前快捷栏", "Click item to equip to Hotbar"), 0.35f, 0.90f, 1.0f, false);
            }
        } else if (isArmorItem(hoveredItemId)) {
            const ArmorDef* def = getArmorDef(hoveredItemId);
            if (def) {
                drawText(tooltipX + 12.0f, tooltipY + 44.0f, getArmorDescription(def), 0.85f, 0.90f, 0.95f, false);

                char statBuf[128];
                snprintf(statBuf, sizeof(statBuf), "%s+%.0f%% | %s%d",
                         LOC("减伤加成: ", "Damage Reduction: "), def->reduction * 100.0f,
                         LOC("最大耐久: ", "Max Durability: "), def->maxDurability);
                drawText(tooltipX + 12.0f, tooltipY + 64.0f, statBuf, def->r, def->g, def->b, false);
                drawText(tooltipX + 12.0f, tooltipY + 84.0f, LOC("左键点击装备到当前快捷栏", "Click item to equip to Hotbar"), 0.35f, 0.90f, 1.0f, false);
            }
        } else if (hoveredItemId < BLOCK_TYPE_COUNT) {
            char blkStatBuf[128];
            snprintf(blkStatBuf, sizeof(blkStatBuf), "%s%.1f | %s%s | %s",
                     LOC("硬度: ", "Hardness: "),
                     g_BlockDefs[hoveredItemId].hardness,
                     LOC("固体: ", "Solid: "),
                     g_BlockDefs[hoveredItemId].isSolid ? LOC("是", "Yes") : LOC("否", "No"),
                     LOC("创造模式无限", "Infinite In Creative"));
            drawText(tooltipX + 12.0f, tooltipY + 46.0f, blkStatBuf, 0.75f, 0.85f, 0.95f, false);
            drawText(tooltipX + 12.0f, tooltipY + 68.0f, LOC("左键点击装备 999 堆叠到快捷栏", "Left-click to assign 999 into active hotbar slot"), 0.35f, 0.95f, 0.45f, false);
            drawText(tooltipX + 12.0f, tooltipY + 86.0f, LOC("右键快速清空当前手持槽位", "Right-click to clear current slot"), 0.50f, 0.75f, 0.90f, false);
        }
    }

    // Restore Projection & Modelview
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// Creative Inventory Input Handling
// ============================================================================

bool handleCreativeInventoryMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!g_CreativeInventoryOpen) {
        // [E] key opens Creative Inventory
        if (uMsg == WM_KEYDOWN && (wParam == 'E' || wParam == 'e')) {
            toggleCreativeInventory();
            return true;
        }
        return false;
    }

    // While Creative Inventory is open, process all modal interactions:
    switch (uMsg) {
        case WM_KEYDOWN: {
            if (wParam == 'E' || wParam == 'e' || wParam == VK_ESCAPE) {
                closeCreativeInventory();
                return true;
            }
            // Number keys 1-9 switch active hotbar slot
            if (wParam >= '1' && wParam <= '9') {
                g_ActiveSlot = (int)(wParam - '1');
                return true;
            }
            return true; // Consume other keyboard events
        }

        case WM_MOUSEMOVE: {
            // Handled during render pass with mouseX, mouseY
            return true;
        }

        case WM_LBUTTONDOWN: {
            int mx = (int)(short)LOWORD(lParam);
            int my = (int)(short)HIWORD(lParam);

            float winW = 860.0f;
            float winH = 580.0f;
            float winX = ((float)WINDOW_WIDTH - winW) * 0.5f;
            float winY = ((float)WINDOW_HEIGHT - winH) * 0.5f;

            // 1. Check Close Button [X]
            float closeX = winX + winW - 38.0f;
            float closeY = winY + 10.0f;
            float closeS = 26.0f;
            if (mx >= closeX && mx <= closeX + closeS && my >= closeY && my <= closeY + closeS) {
                closeCreativeInventory();
                return true;
            }

            // 2. Check Tabs
            float tabStartX = winX + 16.0f;
            float tabY = winY + 46.0f + 12.0f;
            float tabW = 196.0f;
            float tabH = 34.0f;
            float tabGap = 8.0f;

            for (int t = 0; t < CREATIVE_TAB_COUNT; t++) {
                float tx = tabStartX + t * (tabW + tabGap);
                if (mx >= tx && mx <= tx + tabW && my >= tabY && my <= tabY + tabH) {
                    g_CreativeActiveTab = t;
                    return true;
                }
            }

            // 3. Check Item Grid Clicks (Assign item to active hotbar slot)
            float gridX = winX + 16.0f;
            float gridY = tabY + tabH + 8.0f;
            int cols = 8;
            float slotSize = 68.0f;
            float slotGapX = 14.0f;
            float slotGapY = 14.0f;
            float startSlotX = gridX + 22.0f;
            float startSlotY = gridY + 16.0f;

            const int* itemsList = NULL;
            int itemsCount = 0;
            if (g_CreativeActiveTab == CREATIVE_TAB_BLOCKS) {
                itemsList = g_TabBlocksItems; itemsCount = g_TabBlocksCount;
            } else if (g_CreativeActiveTab == CREATIVE_TAB_TOOLS) {
                itemsList = g_TabToolsItems; itemsCount = g_TabToolsCount;
            } else if (g_CreativeActiveTab == CREATIVE_TAB_MATERIALS) {
                itemsList = g_TabMaterialsItems; itemsCount = g_TabMaterialsCount;
            } else if (g_CreativeActiveTab == CREATIVE_TAB_DECOR) {
                itemsList = g_TabDecorItems; itemsCount = g_TabDecorCount;
            }

            for (int i = 0; i < itemsCount; i++) {
                int r = i / cols;
                int c = i % cols;
                float sx = startSlotX + c * (slotSize + slotGapX);
                float sy = startSlotY + r * (slotSize + slotGapY);

                if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                    int clickedItemId = itemsList[i];
                    // Assign item directly into active hotbar slot
                    g_Hotbar[g_ActiveSlot].type = clickedItemId;
                    if (isToolItem(clickedItemId) || isArmorItem(clickedItemId)) {
                        g_Hotbar[g_ActiveSlot].count = 1;
                        g_Hotbar[g_ActiveSlot].durability = getMaxDurability(clickedItemId);
                        g_Hotbar[g_ActiveSlot].maxDurability = getMaxDurability(clickedItemId);
                    } else {
                        // Infinite Creative Stack
                        g_Hotbar[g_ActiveSlot].count = 999;
                        g_Hotbar[g_ActiveSlot].durability = 0;
                        g_Hotbar[g_ActiveSlot].maxDurability = 0;
                    }

                    char eqNotif[128];
                    snprintf(eqNotif, sizeof(eqNotif), LOC("已装备 %s 到快捷栏 [%d]！", "Equipped %s into Hotbar Slot [%d]!"),
                             getItemLocalizedName(clickedItemId), g_ActiveSlot + 1);
                    showNotification(eqNotif, 0.35f, 0.95f, 0.45f);

                    char floatBuf[64];
                    snprintf(floatBuf, sizeof(floatBuf), "+1 %s", getItemLocalizedName(clickedItemId));
                    spawnFloatingText(floatBuf, g_Player.x, g_Player.y + 1.8f, 0.35f, 0.90f, 1.0f);
                    return true;
                }
            }

            // 4. Check Hotbar Slots Click (Switch active slot or reassign)
            float hotbarSectionY = gridY + 310.0f + 12.0f;
            float hbSlotSize = 48.0f;
            float hbGap = 8.0f;
            float hbStartX = winX + 22.0f;
            float hbY = hotbarSectionY + 22.0f;

            for (int h = 0; h < HOTBAR_SLOTS; h++) {
                float hx = hbStartX + h * (hbSlotSize + hbGap);
                if (mx >= hx && mx <= hx + hbSlotSize && my >= hbY && my <= hbY + hbSlotSize) {
                    g_ActiveSlot = h;
                    return true;
                }
            }

            // 5. Check Trash / Clear Button
            float trashX = winX + winW - 145.0f;
            float trashY = hbY + 8.0f;
            float trashW = 125.0f;
            float trashH = 34.0f;
            if (mx >= trashX && mx <= trashX + trashW && my >= trashY && my <= trashY + trashH) {
                g_Hotbar[g_ActiveSlot].type = BLOCK_AIR;
                g_Hotbar[g_ActiveSlot].count = 0;
                g_Hotbar[g_ActiveSlot].durability = 0;
                g_Hotbar[g_ActiveSlot].maxDurability = 0;
                showNotification(LOC("当前快捷栏槽位已清空！", "Active Hotbar Slot Cleared!"), 1.0f, 0.6f, 0.4f);
                return true;
            }

            return true;
        }

        case WM_RBUTTONDOWN: {
            // Right-click on hotbar slot clears that slot
            int mx = (int)(short)LOWORD(lParam);
            int my = (int)(short)HIWORD(lParam);

            float winW = 860.0f;
            float winH = 580.0f;
            float winX = ((float)WINDOW_WIDTH - winW) * 0.5f;
            float winY = ((float)WINDOW_HEIGHT - winH) * 0.5f;
            float gridY = winY + 46.0f + 12.0f + 34.0f + 8.0f;
            float hotbarSectionY = gridY + 310.0f + 12.0f;
            float hbSlotSize = 48.0f;
            float hbGap = 8.0f;
            float hbStartX = winX + 22.0f;
            float hbY = hotbarSectionY + 22.0f;

            for (int h = 0; h < HOTBAR_SLOTS; h++) {
                float hx = hbStartX + h * (hbSlotSize + hbGap);
                if (mx >= hx && mx <= hx + hbSlotSize && my >= hbY && my <= hbY + hbSlotSize) {
                    g_Hotbar[h].type = BLOCK_AIR;
                    g_Hotbar[h].count = 0;
                    g_Hotbar[h].durability = 0;
                    g_Hotbar[h].maxDurability = 0;
                    showNotification(LOC("槽位已清空", "Slot Cleared"), 0.9f, 0.6f, 0.3f);
                    return true;
                }
            }
            return true;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
            return true; // Consume mouse events while inventory is open
    }

    return true;
}

#endif // CREATIVE_INVENTORY_H
