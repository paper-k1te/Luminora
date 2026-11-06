using System;
using System.Collections.Generic;
using Core;
using UnityEngine;

namespace Generation
{
    /// <summary>
    /// Supported macroscopic biome classifications across the horizontal world span.
    /// </summary>
    public enum BiomeType
    {
        Forest,
        Desert,
        Tundra,
        Underworld
    }

    /// <summary>
    /// Type of procedural flora / tree features to spawn on top of surface tiles.
    /// </summary>
    public enum VegetationType
    {
        None,
        OakTree,
        Cactus,
        PineTree,
        GlowShroomTree
    }

    /// <summary>
    /// Configuration specifying the generation rule for a mineral vein.
    /// </summary>
    [Serializable]
    public struct OreConfig
    {
        public BlockType oreType;
        public int minDepth;       // Lowest Y boundary
        public int maxDepth;       // Highest Y boundary
        public float rarity;      // 0.0 to 1.0 threshold (higher = rarer)
        public float veinScale;   // Perlin frequency for vein cluster size

        public OreConfig(BlockType oreType, int minDepth, int maxDepth, float rarity, float veinScale)
        {
            this.oreType = oreType;
            this.minDepth = minDepth;
            this.maxDepth = maxDepth;
            this.rarity = rarity;
            this.veinScale = veinScale;
        }
    }

    /// <summary>
    /// Ecological and geological parameters controlling terrain appearance, blocks, and flora.
    /// </summary>
    [Serializable]
    public class BiomeConfig
    {
        [Header("Biome Type")]
        public BiomeType biomeType;

        [Header("Voxel Blocks")]
        public BlockType surfaceBlock;
        public BlockType subsurfaceBlock;
        public BlockType deepStoneBlock;
        public BlockType backgroundWallBlock;

        [Header("Flora & Decorations")]
        public VegetationType vegetationType;
        public float treeSpawnProbability; // Chance per surface block

        [Header("Atmosphere & Lighting")]
        public Color skyColor;
        public Color ambientLightColor;

        [Header("Surface Topography")]
        public int surfaceHeightBase;
        public int surfaceHeightVariation;
        public float surfaceRoughness;

        [Header("Ores")]
        public List<OreConfig> ores;

        public BiomeConfig()
        {
            ores = new List<OreConfig>();
        }

        /// <summary>
        /// Retrieves the standardized preset configuration for a given biome.
        /// </summary>
        public static BiomeConfig GetDefault(BiomeType type)
        {
            switch (type)
            {
                case BiomeType.Desert:
                    return new BiomeConfig
                    {
                        biomeType = BiomeType.Desert,
                        surfaceBlock = BlockType.Sand,
                        subsurfaceBlock = BlockType.Sandstone,
                        deepStoneBlock = BlockType.Stone,
                        backgroundWallBlock = BlockType.Sandstone,
                        vegetationType = VegetationType.Cactus,
                        treeSpawnProbability = 0.08f,
                        skyColor = new Color(0.62f, 0.78f, 0.98f),
                        ambientLightColor = new Color(1.0f, 0.98f, 0.90f),
                        surfaceHeightBase = 79,
                        surfaceHeightVariation = 7,
                        surfaceRoughness = 0.85f,
                        ores = new List<OreConfig>
                        {
                            new OreConfig(BlockType.CoalOre, 20, 75, 0.75f, 0.12f),
                            new OreConfig(BlockType.IronOre, 15, 65, 0.80f, 0.14f),
                            new OreConfig(BlockType.GoldOre, 5, 45, 0.88f, 0.16f),
                            new OreConfig(BlockType.DiamondOre, 3, 25, 0.93f, 0.18f)
                        }
                    };

                case BiomeType.Tundra:
                    return new BiomeConfig
                    {
                        biomeType = BiomeType.Tundra,
                        surfaceBlock = BlockType.Snow,
                        subsurfaceBlock = BlockType.Ice,
                        deepStoneBlock = BlockType.Stone,
                        backgroundWallBlock = BlockType.Stone,
                        vegetationType = VegetationType.PineTree,
                        treeSpawnProbability = 0.14f,
                        skyColor = new Color(0.70f, 0.82f, 0.96f),
                        ambientLightColor = new Color(0.92f, 0.95f, 1.0f),
                        surfaceHeightBase = 86,
                        surfaceHeightVariation = 15, // Steeper craggy mountains
                        surfaceRoughness = 1.35f,
                        ores = new List<OreConfig>
                        {
                            new OreConfig(BlockType.CoalOre, 20, 80, 0.74f, 0.12f),
                            new OreConfig(BlockType.IronOre, 15, 70, 0.79f, 0.14f),
                            new OreConfig(BlockType.RubyOre, 5, 40, 0.89f, 0.16f),
                            new OreConfig(BlockType.DiamondOre, 3, 25, 0.93f, 0.18f)
                        }
                    };

                case BiomeType.Underworld:
                    return new BiomeConfig
                    {
                        biomeType = BiomeType.Underworld,
                        surfaceBlock = BlockType.Magma,
                        subsurfaceBlock = BlockType.Obsidian,
                        deepStoneBlock = BlockType.Obsidian,
                        backgroundWallBlock = BlockType.Obsidian,
                        vegetationType = VegetationType.GlowShroomTree,
                        treeSpawnProbability = 0.05f,
                        skyColor = new Color(0.18f, 0.04f, 0.04f),
                        ambientLightColor = new Color(0.45f, 0.20f, 0.15f),
                        surfaceHeightBase = 22,
                        surfaceHeightVariation = 4,
                        surfaceRoughness = 0.6f,
                        ores = new List<OreConfig>
                        {
                            new OreConfig(BlockType.RubyOre, 2, 22, 0.84f, 0.15f),
                            new OreConfig(BlockType.DiamondOre, 2, 20, 0.87f, 0.17f),
                            new OreConfig(BlockType.Magma, 2, 22, 0.72f, 0.10f)
                        }
                    };

                case BiomeType.Forest:
                default:
                    return new BiomeConfig
                    {
                        biomeType = BiomeType.Forest,
                        surfaceBlock = BlockType.Grass,
                        subsurfaceBlock = BlockType.Dirt,
                        deepStoneBlock = BlockType.Stone,
                        backgroundWallBlock = BlockType.Dirt,
                        vegetationType = VegetationType.OakTree,
                        treeSpawnProbability = 0.18f,
                        skyColor = new Color(0.48f, 0.72f, 0.98f),
                        ambientLightColor = new Color(0.96f, 0.96f, 0.96f),
                        surfaceHeightBase = 82,
                        surfaceHeightVariation = 9,
                        surfaceRoughness = 1.0f,
                        ores = new List<OreConfig>
                        {
                            new OreConfig(BlockType.CoalOre, 25, 80, 0.72f, 0.12f),
                            new OreConfig(BlockType.IronOre, 15, 68, 0.78f, 0.14f),
                            new OreConfig(BlockType.GoldOre, 5, 42, 0.87f, 0.16f),
                            new OreConfig(BlockType.RubyOre, 4, 32, 0.91f, 0.17f),
                            new OreConfig(BlockType.DiamondOre, 3, 24, 0.94f, 0.18f)
                        }
                    };
            }
        }
    }
}
