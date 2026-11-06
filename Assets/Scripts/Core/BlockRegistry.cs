using System;
using System.Collections.Generic;
using UnityEngine;

namespace Core
{
    /// <summary>
    /// Central catalog and fast lookup registry for all voxel block definitions.
    /// Provides O(1) array-indexed lookups and sensible game mechanics defaults.
    /// Supports dynamic bilingual localization via LocalizationManager.
    /// </summary>
    public static class BlockRegistry
    {
        private const int MaxBlockTypes = 256;
        private static readonly BlockData[] Blocks = new BlockData[MaxBlockTypes];
        private static readonly bool[] SolidLookup = new bool[MaxBlockTypes];
        private static readonly bool[] TransparentLookup = new bool[MaxBlockTypes];
        private static readonly bool[] LightEmissionLookup = new bool[MaxBlockTypes];

        private static bool _initialized = false;

        static BlockRegistry()
        {
            Initialize();
        }

        /// <summary>
        /// Explicit initialization entry point, safe to call multiple times.
        /// </summary>
        public static void Initialize()
        {
            if (_initialized) return;

            // Fill entire array with Air as fallback to avoid null references
            BlockData airFallback = new BlockData(
                type: BlockType.Air,
                chineseName: "空气",
                englishName: "Air",
                hardness: 0f,
                isSolid: false,
                isPassable: true,
                isTransparent: true,
                textureIndex: 0,
                defaultColor: Color.clear
            );

            for (int i = 0; i < MaxBlockTypes; i++)
            {
                Blocks[i] = airFallback;
            }

            // Register all defined blocks with bilingual metadata
            Register(new BlockData(
                type: BlockType.Air,
                chineseName: "空气",
                englishName: "Air",
                hardness: 0f,
                isSolid: false,
                isPassable: true,
                isTransparent: true,
                textureIndex: 0,
                defaultColor: Color.clear
            ));

            Register(new BlockData(
                type: BlockType.Grass,
                chineseName: "草方块",
                englishName: "Grass Block",
                hardness: 0.6f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 1,
                defaultColor: new Color(0.35f, 0.72f, 0.23f),
                dropItemId: (int)BlockType.Dirt
            ));

            Register(new BlockData(
                type: BlockType.Dirt,
                chineseName: "泥土",
                englishName: "Dirt",
                hardness: 0.5f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 2,
                defaultColor: new Color(0.52f, 0.35f, 0.21f),
                dropItemId: (int)BlockType.Dirt
            ));

            Register(new BlockData(
                type: BlockType.Stone,
                chineseName: "原石",
                englishName: "Stone",
                hardness: 1.5f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 3,
                defaultColor: new Color(0.55f, 0.55f, 0.55f),
                dropItemId: (int)BlockType.Stone
            ));

            Register(new BlockData(
                type: BlockType.Wood,
                chineseName: "原木",
                englishName: "Wood Log",
                hardness: 1.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 4,
                defaultColor: new Color(0.44f, 0.29f, 0.16f),
                dropItemId: (int)BlockType.Wood
            ));

            Register(new BlockData(
                type: BlockType.Leaves,
                chineseName: "树叶",
                englishName: "Leaves",
                hardness: 0.2f,
                isSolid: true,
                isPassable: false,
                isTransparent: true,
                textureIndex: 5,
                defaultColor: new Color(0.24f, 0.58f, 0.18f),
                dropItemId: (int)BlockType.Leaves
            ));

            Register(new BlockData(
                type: BlockType.Sand,
                chineseName: "沙子",
                englishName: "Sand",
                hardness: 0.5f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 6,
                defaultColor: new Color(0.88f, 0.81f, 0.54f),
                dropItemId: (int)BlockType.Sand
            ));

            Register(new BlockData(
                type: BlockType.Sandstone,
                chineseName: "砂岩",
                englishName: "Sandstone",
                hardness: 1.2f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 7,
                defaultColor: new Color(0.79f, 0.69f, 0.45f),
                dropItemId: (int)BlockType.Sandstone
            ));

            Register(new BlockData(
                type: BlockType.Cactus,
                chineseName: "仙人掌",
                englishName: "Cactus",
                hardness: 0.4f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 8,
                defaultColor: new Color(0.25f, 0.55f, 0.28f),
                dropItemId: (int)BlockType.Cactus
            ));

            Register(new BlockData(
                type: BlockType.Snow,
                chineseName: "雪块",
                englishName: "Snow Block",
                hardness: 0.3f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 9,
                defaultColor: new Color(0.92f, 0.95f, 0.98f),
                dropItemId: (int)BlockType.Snow
            ));

            Register(new BlockData(
                type: BlockType.Ice,
                chineseName: "冰块",
                englishName: "Ice",
                hardness: 0.8f,
                isSolid: true,
                isPassable: false,
                isTransparent: true,
                textureIndex: 10,
                defaultColor: new Color(0.65f, 0.85f, 0.95f, 0.8f),
                dropItemId: (int)BlockType.Ice
            ));

            Register(new BlockData(
                type: BlockType.Clay,
                chineseName: "粘土",
                englishName: "Clay",
                hardness: 0.7f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 11,
                defaultColor: new Color(0.68f, 0.64f, 0.68f),
                dropItemId: (int)BlockType.Clay
            ));

            Register(new BlockData(
                type: BlockType.CoalOre,
                chineseName: "煤矿石",
                englishName: "Coal Ore",
                hardness: 2.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 12,
                defaultColor: new Color(0.22f, 0.22f, 0.22f),
                dropItemId: (int)BlockType.CoalOre
            ));

            Register(new BlockData(
                type: BlockType.IronOre,
                chineseName: "铁矿石",
                englishName: "Iron Ore",
                hardness: 2.5f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 13,
                defaultColor: new Color(0.72f, 0.58f, 0.46f),
                dropItemId: (int)BlockType.IronOre
            ));

            Register(new BlockData(
                type: BlockType.GoldOre,
                chineseName: "金矿石",
                englishName: "Gold Ore",
                hardness: 3.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 14,
                defaultColor: new Color(0.96f, 0.82f, 0.24f),
                dropItemId: (int)BlockType.GoldOre
            ));

            Register(new BlockData(
                type: BlockType.DiamondOre,
                chineseName: "钻石矿石",
                englishName: "Diamond Ore",
                hardness: 4.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 15,
                defaultColor: new Color(0.33f, 0.88f, 0.92f),
                dropItemId: (int)BlockType.DiamondOre
            ));

            Register(new BlockData(
                type: BlockType.RubyOre,
                chineseName: "红宝石矿石",
                englishName: "Ruby Ore",
                hardness: 4.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 16,
                defaultColor: new Color(0.88f, 0.16f, 0.26f),
                dropItemId: (int)BlockType.RubyOre
            ));

            Register(new BlockData(
                type: BlockType.Brick,
                chineseName: "石砖",
                englishName: "Stone Brick",
                hardness: 2.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 17,
                defaultColor: new Color(0.62f, 0.42f, 0.36f),
                dropItemId: (int)BlockType.Brick
            ));

            Register(new BlockData(
                type: BlockType.WoodPlank,
                chineseName: "木板",
                englishName: "Wood Plank",
                hardness: 1.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 18,
                defaultColor: new Color(0.68f, 0.49f, 0.30f),
                dropItemId: (int)BlockType.WoodPlank
            ));

            Register(new BlockData(
                type: BlockType.Glass,
                chineseName: "玻璃",
                englishName: "Glass",
                hardness: 0.3f,
                isSolid: true,
                isPassable: false,
                isTransparent: true,
                textureIndex: 19,
                defaultColor: new Color(0.85f, 0.92f, 0.98f, 0.5f),
                dropItemId: (int)BlockType.Glass
            ));

            Register(new BlockData(
                type: BlockType.Torch,
                chineseName: "火把",
                englishName: "Torch",
                hardness: 0.1f,
                isSolid: false,
                isPassable: true,
                isTransparent: true,
                textureIndex: 20,
                defaultColor: new Color(1.0f, 0.88f, 0.35f),
                emitLight: true,
                lightColor: new Color(1.0f, 0.82f, 0.45f),
                dropItemId: (int)BlockType.Torch
            ));

            Register(new BlockData(
                type: BlockType.Obsidian,
                chineseName: "黑曜石",
                englishName: "Obsidian",
                hardness: 6.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 21,
                defaultColor: new Color(0.18f, 0.14f, 0.24f),
                dropItemId: (int)BlockType.Obsidian
            ));

            Register(new BlockData(
                type: BlockType.Magma,
                chineseName: "岩浆块",
                englishName: "Magma Block",
                hardness: 3.0f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 22,
                defaultColor: new Color(0.85f, 0.32f, 0.08f),
                emitLight: true,
                lightColor: new Color(1.0f, 0.40f, 0.12f),
                dropItemId: (int)BlockType.Magma
            ));

            Register(new BlockData(
                type: BlockType.MossyStone,
                chineseName: "苔石",
                englishName: "Mossy Stone",
                hardness: 1.6f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 23,
                defaultColor: new Color(0.42f, 0.56f, 0.38f),
                dropItemId: (int)BlockType.MossyStone
            ));

            Register(new BlockData(
                type: BlockType.Mud,
                chineseName: "泥泞",
                englishName: "Mud",
                hardness: 0.5f,
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 24,
                defaultColor: new Color(0.36f, 0.28f, 0.22f),
                dropItemId: (int)BlockType.Mud
            ));

            Register(new BlockData(
                type: BlockType.GlowShroom,
                chineseName: "发光蘑菇",
                englishName: "Glowing Mushroom",
                hardness: 0.2f,
                isSolid: false,
                isPassable: true,
                isTransparent: true,
                textureIndex: 25,
                defaultColor: new Color(0.20f, 0.85f, 0.95f),
                emitLight: true,
                lightColor: new Color(0.25f, 0.85f, 1.0f),
                dropItemId: (int)BlockType.GlowShroom
            ));

            Register(new BlockData(
                type: BlockType.Bedrock,
                chineseName: "基岩",
                englishName: "Bedrock",
                hardness: -1.0f, // Unbreakable
                isSolid: true,
                isPassable: false,
                isTransparent: false,
                textureIndex: 26,
                defaultColor: new Color(0.12f, 0.12f, 0.12f),
                dropItemId: 0,
                dropCount: 0
            ));

            // Register Craftable Tools, Weapons, Materials & Supplies (Pure bilingual names, no mixed strings)
            Register(new BlockData(type: BlockType.Stick, chineseName: "木棍", englishName: "Stick", hardness: 0.1f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 30, defaultColor: new Color(0.55f, 0.38f, 0.20f), dropItemId: (int)BlockType.Stick));
            Register(new BlockData(type: BlockType.WoodenPickaxe, chineseName: "木镐", englishName: "Wooden Pickaxe", hardness: 0.2f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 31, defaultColor: new Color(0.65f, 0.45f, 0.25f), dropItemId: (int)BlockType.WoodenPickaxe));
            Register(new BlockData(type: BlockType.WoodenAxe, chineseName: "木斧", englishName: "Wooden Axe", hardness: 0.2f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 32, defaultColor: new Color(0.65f, 0.45f, 0.25f), dropItemId: (int)BlockType.WoodenAxe));
            Register(new BlockData(type: BlockType.WoodenSword, chineseName: "木剑", englishName: "Wooden Sword", hardness: 0.2f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 33, defaultColor: new Color(0.65f, 0.45f, 0.25f), dropItemId: (int)BlockType.WoodenSword));
            Register(new BlockData(type: BlockType.WoodenShovel, chineseName: "木铲", englishName: "Wooden Shovel", hardness: 0.2f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 34, defaultColor: new Color(0.65f, 0.45f, 0.25f), dropItemId: (int)BlockType.WoodenShovel));
            Register(new BlockData(type: BlockType.StonePickaxe, chineseName: "石镐", englishName: "Stone Pickaxe", hardness: 0.3f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 35, defaultColor: new Color(0.60f, 0.60f, 0.62f), dropItemId: (int)BlockType.StonePickaxe));
            Register(new BlockData(type: BlockType.StoneAxe, chineseName: "石斧", englishName: "Stone Axe", hardness: 0.3f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 36, defaultColor: new Color(0.60f, 0.60f, 0.62f), dropItemId: (int)BlockType.StoneAxe));
            Register(new BlockData(type: BlockType.StoneSword, chineseName: "石剑", englishName: "Stone Sword", hardness: 0.3f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 37, defaultColor: new Color(0.60f, 0.60f, 0.62f), dropItemId: (int)BlockType.StoneSword));
            Register(new BlockData(type: BlockType.IronPickaxe, chineseName: "铁镐", englishName: "Iron Pickaxe", hardness: 0.4f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 38, defaultColor: new Color(0.85f, 0.86f, 0.90f), dropItemId: (int)BlockType.IronPickaxe));
            Register(new BlockData(type: BlockType.IronAxe, chineseName: "铁斧", englishName: "Iron Axe", hardness: 0.4f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 39, defaultColor: new Color(0.85f, 0.86f, 0.90f), dropItemId: (int)BlockType.IronAxe));
            Register(new BlockData(type: BlockType.IronSword, chineseName: "铁剑", englishName: "Iron Sword", hardness: 0.4f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 40, defaultColor: new Color(0.85f, 0.86f, 0.90f), dropItemId: (int)BlockType.IronSword));
            Register(new BlockData(type: BlockType.DiamondPickaxe, chineseName: "钻石镐", englishName: "Diamond Pickaxe", hardness: 0.5f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 41, defaultColor: new Color(0.28f, 0.88f, 0.95f), dropItemId: (int)BlockType.DiamondPickaxe));
            Register(new BlockData(type: BlockType.DiamondSword, chineseName: "钻石剑", englishName: "Diamond Sword", hardness: 0.5f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 42, defaultColor: new Color(0.28f, 0.88f, 0.95f), dropItemId: (int)BlockType.DiamondSword));
            Register(new BlockData(type: BlockType.Gel, chineseName: "史莱姆凝胶", englishName: "Slime Gel", hardness: 0.1f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 43, defaultColor: new Color(0.22f, 0.88f, 0.45f), dropItemId: (int)BlockType.Gel));
            Register(new BlockData(type: BlockType.StickyBomb, chineseName: "粘性炸弹", englishName: "Sticky Bomb", hardness: 0.2f, isSolid: false, isPassable: true, isTransparent: true, textureIndex: 44, defaultColor: new Color(0.35f, 0.72f, 0.35f), dropItemId: (int)BlockType.StickyBomb));

            _initialized = true;
        }

        private static void Register(BlockData block)
        {
            byte id = (byte)block.type;
            Blocks[id] = block;
            SolidLookup[id] = block.isSolid;
            TransparentLookup[id] = block.isTransparent;
            LightEmissionLookup[id] = block.emitLight;
        }

        /// <summary>
        /// Fast O(1) lookup of block definition by enum.
        /// </summary>
        public static BlockData GetBlock(BlockType type)
        {
            if (!_initialized) Initialize();
            return Blocks[(byte)type];
        }

        /// <summary>
        /// Fast O(1) lookup of block definition by byte identifier.
        /// </summary>
        public static BlockData GetBlock(byte id)
        {
            if (!_initialized) Initialize();
            return Blocks[id];
        }

        /// <summary>
        /// Localized block name lookup by BlockType.
        /// </summary>
        public static string GetBlockName(BlockType type)
        {
            BlockData block = GetBlock(type);
            return block != null ? block.displayName : type.ToString();
        }

        /// <summary>
        /// Localized block name lookup by byte identifier.
        /// </summary>
        public static string GetBlockName(byte id)
        {
            BlockData block = GetBlock(id);
            return block != null ? block.displayName : ((BlockType)id).ToString();
        }

        /// <summary>
        /// Quick physical collision query without full BlockData dereferencing.
        /// </summary>
        public static bool IsSolid(BlockType type)
        {
            if (!_initialized) Initialize();
            return SolidLookup[(byte)type];
        }

        /// <summary>
        /// Quick light-permeability / face culling query.
        /// </summary>
        public static bool IsTransparent(BlockType type)
        {
            if (!_initialized) Initialize();
            return TransparentLookup[(byte)type];
        }

        /// <summary>
        /// Quick light emission query.
        /// </summary>
        public static bool EmitsLight(BlockType type)
        {
            if (!_initialized) Initialize();
            return LightEmissionLookup[(byte)type];
        }

        /// <summary>
        /// Retrieves total number of registered distinct block definitions.
        /// </summary>
        public static int Count => 27;

        /// <summary>
        /// Retrieves the corresponding background wall type for a given foreground block.
        /// Matches Terraria's dual-layer philosophy where background walls adapt to foreground strata.
        /// </summary>
        public static BlockType GetMatchingWallType(BlockType type)
        {
            switch (type)
            {
                case BlockType.Grass:
                case BlockType.Dirt:
                case BlockType.Clay:
                case BlockType.Mud:
                    return BlockType.Dirt;
                case BlockType.Stone:
                case BlockType.CoalOre:
                case BlockType.IronOre:
                case BlockType.GoldOre:
                case BlockType.DiamondOre:
                case BlockType.RubyOre:
                case BlockType.MossyStone:
                    return BlockType.Stone;
                case BlockType.Sand:
                case BlockType.Sandstone:
                case BlockType.Cactus:
                    return BlockType.Sandstone;
                case BlockType.Snow:
                case BlockType.Ice:
                    return BlockType.Ice;
                case BlockType.Wood:
                case BlockType.Leaves:
                    return BlockType.Wood;
                case BlockType.WoodPlank:
                    return BlockType.WoodPlank;
                case BlockType.Brick:
                    return BlockType.Brick;
                case BlockType.Obsidian:
                case BlockType.Magma:
                case BlockType.Bedrock:
                    return BlockType.Obsidian;
                case BlockType.GlowShroom:
                    return BlockType.Stone;
                case BlockType.Glass:
                    return BlockType.Glass;
                default:
                    return BlockType.Dirt;
            }
        }
    }
}
