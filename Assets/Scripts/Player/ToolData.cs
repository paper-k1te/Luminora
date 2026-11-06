using System;
using System.Collections.Generic;
using Core;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Configuration data and gameplay statistics for a tool or weapon.
    /// Encapsulates speed multipliers, melee damage values, knockback force,
    /// target block affinities, and bilingual display names.
    /// </summary>
    [Serializable]
    public class ToolData
    {
        public ToolID id;
        public ToolType type;
        public ToolTier tier;
        public string englishName;
        public string chineseName;
        public string description;
        public string englishDescription;
        public float efficiencyMultiplier;
        public float damage;
        public float knockbackMultiplier;
        public Color tierColor;

        public string displayName => LocalizationManager.Get(chineseName, englishName);
        public string localizedDescription => LocalizationManager.Get(description, englishDescription ?? description);

        public ToolData(ToolID id, ToolType type, ToolTier tier, string englishName, string chineseName,
                        string description, float efficiencyMultiplier, float damage, float knockbackMultiplier, Color tierColor,
                        string englishDescription = null)
        {
            this.id = id;
            this.type = type;
            this.tier = tier;
            this.englishName = englishName;
            this.chineseName = chineseName;
            this.description = description;
            this.englishDescription = englishDescription ?? description;
            this.efficiencyMultiplier = efficiencyMultiplier;
            this.damage = damage;
            this.knockbackMultiplier = knockbackMultiplier;
            this.tierColor = tierColor;
        }

        /// <summary>
        /// Checks if this tool has an efficiency affinity for the specified block type.
        /// </summary>
        public bool IsEffectiveAgainst(BlockType blockType)
        {
            switch (type)
            {
                case ToolType.Pickaxe:
                    return blockType == BlockType.Stone ||
                           blockType == BlockType.CoalOre ||
                           blockType == BlockType.IronOre ||
                           blockType == BlockType.GoldOre ||
                           blockType == BlockType.DiamondOre ||
                           blockType == BlockType.RubyOre ||
                           blockType == BlockType.Brick ||
                           blockType == BlockType.Obsidian ||
                           blockType == BlockType.Magma ||
                           blockType == BlockType.MossyStone ||
                           blockType == BlockType.Sandstone;

                case ToolType.Axe:
                    return blockType == BlockType.Wood ||
                           blockType == BlockType.WoodPlank ||
                           blockType == BlockType.Leaves ||
                           blockType == BlockType.Cactus;

                case ToolType.Shovel:
                    return blockType == BlockType.Grass ||
                           blockType == BlockType.Dirt ||
                           blockType == BlockType.Sand ||
                           blockType == BlockType.Snow ||
                           blockType == BlockType.Ice ||
                           blockType == BlockType.Clay ||
                           blockType == BlockType.Mud;

                default:
                    return false;
            }
        }

        /// <summary>
        /// Returns the breaking speed multiplier for this tool on the target block.
        /// </summary>
        public float GetEfficiency(BlockType blockType)
        {
            return IsEffectiveAgainst(blockType) ? efficiencyMultiplier : 1.0f;
        }
    }

    /// <summary>
    /// Central registry containing definitions and statistics for all tools and weapons in the game.
    /// </summary>
    public static class ToolRegistry
    {
        private static readonly Dictionary<ToolID, ToolData> Tools = new Dictionary<ToolID, ToolData>();
        private static readonly List<ToolData> AllToolsList = new List<ToolData>();

        static ToolRegistry()
        {
            Color woodColor    = new Color(0.68f, 0.46f, 0.24f);
            Color stoneColor   = new Color(0.62f, 0.64f, 0.66f);
            Color ironColor    = new Color(0.88f, 0.90f, 0.94f);
            Color diamondColor = new Color(0.25f, 0.90f, 0.98f);

            // ================================================================
            // 1. Pickaxes (Mines stone, ores, bricks fast)
            // ================================================================
            Register(new ToolData(ToolID.WoodPickaxe, ToolType.Pickaxe, ToolTier.Wood, "Wood Pickaxe", "木镐",
                "采矿工具: 开采岩石与矿脉速度 1.5x", 1.5f, 6.0f, 1.0f, woodColor));
            Register(new ToolData(ToolID.StonePickaxe, ToolType.Pickaxe, ToolTier.Stone, "Stone Pickaxe", "石镐",
                "采矿工具: 开采岩石与矿脉速度 2.5x", 2.5f, 7.0f, 1.1f, stoneColor));
            Register(new ToolData(ToolID.IronPickaxe, ToolType.Pickaxe, ToolTier.Iron, "Iron Pickaxe", "铁镐",
                "采矿工具: 开采岩石与矿脉速度 4.0x", 4.0f, 8.0f, 1.2f, ironColor));
            Register(new ToolData(ToolID.DiamondPickaxe, ToolType.Pickaxe, ToolTier.Diamond, "Diamond Pickaxe", "钻石镐",
                "神级采矿: 开采岩石与矿脉速度 8.0x", 8.0f, 9.0f, 1.4f, diamondColor));

            // ================================================================
            // 2. Axes (Chops wood, planks, leaves rapidly)
            // ================================================================
            Register(new ToolData(ToolID.WoodAxe, ToolType.Axe, ToolTier.Wood, "Wood Axe", "木斧",
                "伐木工具: 砍伐树木与木板速度 2.0x", 2.0f, 7.0f, 1.1f, woodColor));
            Register(new ToolData(ToolID.StoneAxe, ToolType.Axe, ToolTier.Stone, "Stone Axe", "石斧",
                "伐木工具: 砍伐树木与木板速度 3.0x", 3.0f, 8.0f, 1.2f, stoneColor));
            Register(new ToolData(ToolID.IronAxe, ToolType.Axe, ToolTier.Iron, "Iron Axe", "铁斧",
                "伐木工具: 砍伐树木与木板速度 5.0x", 5.0f, 10.0f, 1.4f, ironColor));
            Register(new ToolData(ToolID.DiamondAxe, ToolType.Axe, ToolTier.Diamond, "Diamond Axe", "钻石斧",
                "神级伐木: 秒速砍伐木材速度 10.0x", 10.0f, 12.0f, 1.8f, diamondColor));

            // ================================================================
            // 3. Swords (Heavy damage & massive knockback vs Slimes, Bats, Zombies)
            // ================================================================
            Register(new ToolData(ToolID.WoodSword, ToolType.Sword, ToolTier.Wood, "Wood Sword", "木剑",
                "近战武器: 挥砍伤害 +4 | 击退 1.5x", 1.0f, 8.0f, 1.5f, woodColor));
            Register(new ToolData(ToolID.StoneSword, ToolType.Sword, ToolTier.Stone, "Stone Sword", "石剑",
                "近战武器: 挥砍伤害 +6 | 击退 1.8x", 1.0f, 10.0f, 1.8f, stoneColor));
            Register(new ToolData(ToolID.IronSword, ToolType.Sword, ToolTier.Iron, "Iron Sword", "铁剑",
                "近战武器: 挥砍伤害 +9 | 击退 2.3x", 1.0f, 13.0f, 2.3f, ironColor));
            Register(new ToolData(ToolID.DiamondSword, ToolType.Sword, ToolTier.Diamond, "Diamond Sword", "钻石剑",
                "至尊神剑: 挥砍伤害 +15 | 巨幅击退 3.2x", 1.0f, 19.0f, 3.2f, diamondColor));

            // ================================================================
            // 4. Shovels (Digs soil, sand, clay, snow fast)
            // ================================================================
            Register(new ToolData(ToolID.WoodShovel, ToolType.Shovel, ToolTier.Wood, "Wood Shovel", "木铲",
                "挖掘工具: 挖掘泥土与沙地速度 2.0x", 2.0f, 6.0f, 1.0f, woodColor));
            Register(new ToolData(ToolID.StoneShovel, ToolType.Shovel, ToolTier.Stone, "Stone Shovel", "石铲",
                "挖掘工具: 挖掘泥土与沙地速度 3.0x", 3.0f, 7.0f, 1.1f, stoneColor));
            Register(new ToolData(ToolID.IronShovel, ToolType.Shovel, ToolTier.Iron, "Iron Shovel", "铁铲",
                "挖掘工具: 挖掘泥土与沙地速度 5.0x", 5.0f, 8.0f, 1.2f, ironColor));
            Register(new ToolData(ToolID.DiamondShovel, ToolType.Shovel, ToolTier.Diamond, "Diamond Shovel", "钻石铲",
                "神级挖掘: 挖掘泥土与沙地速度 9.0x", 9.0f, 9.0f, 1.3f, diamondColor));
        }

        private static void Register(ToolData data)
        {
            Tools[data.id] = data;
            AllToolsList.Add(data);
        }

        public static ToolData Get(ToolID id)
        {
            Tools.TryGetValue(id, out var data);
            return data;
        }

        public static bool IsTool(int id)
        {
            return id >= (int)ToolID.WoodPickaxe && id <= (int)ToolID.DiamondShovel;
        }

        public static IReadOnlyList<ToolData> GetAllTools() => AllToolsList;
    }
}
