using System;
using System.Collections.Generic;
using Core;
using UnityEngine;

namespace Generation
{
    /// <summary>
    /// Master procedural world generator responsible for multi-biome heightfields,
    /// cavern carving, mineral deposit distribution, vegetation spawning, and background walls.
    /// </summary>
    [ExecuteInEditMode]
    public class TerrainGenerator : MonoBehaviour
    {
        [Header("Global Settings")]
        [SerializeField] private int seed = 1337;
        [SerializeField] private bool randomizeSeedOnStart = false;

        [Header("Noise Configuration")]
        [SerializeField] private float surfaceFrequency = WorldSettings.TerrainNoiseFrequency;
        [SerializeField] private float caveFrequency = WorldSettings.CaveNoiseFrequency;
        [SerializeField] private float caveThreshold = WorldSettings.CaveThreshold;
        [SerializeField] private float biomeFrequency = WorldSettings.BiomeScale;

        // Cached biome configuration instances
        private readonly Dictionary<BiomeType, BiomeConfig> _biomeConfigs = new Dictionary<BiomeType, BiomeConfig>();

        public int Seed
        {
            get => seed;
            set
            {
                seed = value;
                WorldSettings.ActiveSeed = value;
            }
        }

        private void Awake()
        {
            if (randomizeSeedOnStart && Application.isPlaying)
            {
                seed = UnityEngine.Random.Range(10000, 99999);
            }
            WorldSettings.ActiveSeed = seed;
            InitializeBiomeCache();
        }

        public void InitializeBiomeCache()
        {
            _biomeConfigs.Clear();
            _biomeConfigs[BiomeType.Forest] = BiomeConfig.GetDefault(BiomeType.Forest);
            _biomeConfigs[BiomeType.Desert] = BiomeConfig.GetDefault(BiomeType.Desert);
            _biomeConfigs[BiomeType.Tundra] = BiomeConfig.GetDefault(BiomeType.Tundra);
            _biomeConfigs[BiomeType.Underworld] = BiomeConfig.GetDefault(BiomeType.Underworld);
        }

        /// <summary>
        /// Determines the macro biome at a given world horizontal coordinate.
        /// Uses 1D multi-octave noise to create natural, broad geographic bands.
        /// </summary>
        public BiomeType GetBiomeAt(float worldX)
        {
            float noiseVal = NoiseGenerator.Fbm1D(worldX, seed + 9999, octaves: 2, frequency: biomeFrequency);

            if (noiseVal < 0.33f)
            {
                return BiomeType.Desert;
            }
            if (noiseVal < 0.67f)
            {
                return BiomeType.Forest;
            }
            return BiomeType.Tundra;
        }

        /// <summary>
        /// Retrieves the config for the requested biome type.
        /// </summary>
        public BiomeConfig GetConfig(BiomeType type)
        {
            if (_biomeConfigs.Count == 0) InitializeBiomeCache();
            if (_biomeConfigs.TryGetValue(type, out BiomeConfig config))
            {
                return config;
            }
            return BiomeConfig.GetDefault(type);
        }

        /// <summary>
        /// Evaluates the surface ground level (Y index) at a specific horizontal world coordinate.
        /// </summary>
        public int GetSurfaceHeight(float worldX)
        {
            BiomeType biome = GetBiomeAt(worldX);
            BiomeConfig config = GetConfig(biome);

            float baseHeight = config.surfaceHeightBase;
            float variation = config.surfaceHeightVariation;

            // Base smooth rolling terrain
            float heightNoise = NoiseGenerator.Fbm1D(
                worldX,
                seed,
                octaves: 4,
                frequency: surfaceFrequency * config.surfaceRoughness,
                persistence: 0.5f,
                lacunarity: 2.0f
            );

            float calculatedY = baseHeight + (heightNoise - 0.5f) * 2.0f * variation;

            // Extra geological character per biome
            if (biome == BiomeType.Tundra)
            {
                // Jagged alpine crags
                float ridge = NoiseGenerator.RidgedNoise2D(worldX, 0f, seed + 888, octaves: 3, frequency: 0.015f);
                calculatedY += ridge * 8f;
            }
            else if (biome == BiomeType.Desert)
            {
                // Smooth wind-swept sand dunes
                float dune = Mathf.Sin(worldX * 0.05f) * 2.5f;
                calculatedY += dune;
            }

            int finalY = Mathf.RoundToInt(calculatedY);
            return Mathf.Clamp(finalY, WorldSettings.CavernLevel + 5, WorldSettings.ChunkHeight - 16);
        }

        /// <summary>
        /// Generates the complete 2D foreground and background block grids for an entire chunk column.
        /// </summary>
        /// <param name="chunkX">Horizontal chunk coordinate.</param>
        /// <param name="foreground">Output 2D array of interactive solid/passable blocks.</param>
        /// <param name="background">Output 2D array of decorative structural background walls.</param>
        public void GenerateChunkData(int chunkX, out BlockType[,] foreground, out BlockType[,] background)
        {
            if (_biomeConfigs.Count == 0) InitializeBiomeCache();

            int width = WorldSettings.ChunkWidth;
            int height = WorldSettings.ChunkHeight;

            foreground = new BlockType[width, height];
            background = new BlockType[width, height];

            int[] surfaceHeights = new int[width];
            BiomeType[] columnBiomes = new BiomeType[width];

            // -----------------------------------------------------------------
            // Pass 1: Surface Heights & Column Biomes
            // -----------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                float worldX = WorldSettings.LocalToWorldX(chunkX, x);
                columnBiomes[x] = GetBiomeAt(worldX);
                surfaceHeights[x] = GetSurfaceHeight(worldX);
            }

            // -----------------------------------------------------------------
            // Pass 2: Base Geological Strata & Background Walls
            // -----------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                float worldX = WorldSettings.LocalToWorldX(chunkX, x);
                int surfaceY = surfaceHeights[x];
                BiomeType biome = columnBiomes[x];
                BiomeConfig config = GetConfig(biome);

                // Subsurface transition depth (e.g. 4 to 7 blocks of dirt/sandstone)
                float depthNoise = NoiseGenerator.Fbm1D(worldX, seed + 333, octaves: 2, frequency: 0.08f);
                int subsurfaceDepth = 4 + Mathf.RoundToInt(depthNoise * 3f);
                int subsurfaceStartY = surfaceY - subsurfaceDepth;

                for (int y = 0; y < height; y++)
                {
                    // 1. Bedrock base
                    if (y <= WorldSettings.BedrockLevel)
                    {
                        foreground[x, y] = BlockType.Bedrock;
                        background[x, y] = BlockType.Obsidian;
                        continue;
                    }

                    // 2. Open air above surface
                    if (y > surfaceY)
                    {
                        foreground[x, y] = BlockType.Air;
                        background[x, y] = BlockType.Air;
                        continue;
                    }

                    // 3. Ground layers
                    if (y == surfaceY)
                    {
                        // Immediate topsoil
                        foreground[x, y] = config.surfaceBlock;
                        background[x, y] = config.backgroundWallBlock;
                    }
                    else if (y > subsurfaceStartY)
                    {
                        // Subsurface transitional layer (Dirt, Sandstone, Ice)
                        foreground[x, y] = config.subsurfaceBlock;
                        background[x, y] = config.backgroundWallBlock;
                    }
                    else if (y > WorldSettings.UnderworldLevel)
                    {
                        // Deep rock cavern layer
                        foreground[x, y] = config.deepStoneBlock;
                        background[x, y] = (biome == BiomeType.Desert) ? BlockType.Sandstone : BlockType.Stone;
                    }
                    else
                    {
                        // Deep Underworld / Magma mantle
                        foreground[x, y] = BlockType.Obsidian;
                        background[x, y] = BlockType.Obsidian;
                    }
                }
            }

            // -----------------------------------------------------------------
            // Pass 3: Cavern & Tunnel Carving (Swiss Cheese + Worm Tunnels)
            // -----------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                float worldX = WorldSettings.LocalToWorldX(chunkX, x);
                int surfaceY = surfaceHeights[x];

                for (int y = WorldSettings.BedrockLevel + 1; y < surfaceY - 1; y++)
                {
                    // Domain-warped cheese caverns
                    float caveSample = NoiseGenerator.DomainWarpedNoise2D(
                        worldX,
                        y,
                        seed + 1234,
                        warpMagnitude: 14f,
                        octaves: 3,
                        frequency: caveFrequency
                    );

                    // Tubular interconnecting worm tunnels
                    bool isWorm = NoiseGenerator.IsWormTunnel(
                        worldX,
                        y,
                        seed + 5678,
                        frequency: WorldSettings.CaveWormFrequency,
                        tunnelThickness: 0.065f
                    );

                    // Dynamic threshold: caverns open up deeper down
                    float depthFactor = Mathf.Clamp01((float)(surfaceY - y) / (surfaceY - WorldSettings.UnderworldLevel));
                    float activeThreshold = Mathf.Lerp(caveThreshold + 0.08f, caveThreshold - 0.05f, depthFactor);

                    if (caveSample > activeThreshold || isWorm)
                    {
                        if (y <= WorldSettings.UnderworldLevel - 3)
                        {
                            // In deep underworld, cavern bottoms can pool Magma
                            foreground[x, y] = (y <= WorldSettings.BedrockLevel + 4) ? BlockType.Magma : BlockType.Air;
                        }
                        else
                        {
                            foreground[x, y] = BlockType.Air;
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Pass 4: Ore Veins & Mineral Pockets
            // -----------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                float worldX = WorldSettings.LocalToWorldX(chunkX, x);
                int surfaceY = surfaceHeights[x];
                BiomeConfig config = GetConfig(columnBiomes[x]);

                for (int y = WorldSettings.BedrockLevel + 1; y < surfaceY; y++)
                {
                    // Only embed ores into solid rock/dirt, not into empty cave air
                    if (foreground[x, y] == BlockType.Air) continue;

                    // Evaluate each ore vein candidate in the biome configuration
                    for (int o = 0; o < config.ores.Count; o++)
                    {
                        OreConfig ore = config.ores[o];
                        if (y >= ore.minDepth && y <= ore.maxDepth)
                        {
                            float oreNoise = NoiseGenerator.Fbm2D(
                                worldX,
                                y,
                                seed + (int)ore.oreType * 777,
                                octaves: 2,
                                frequency: ore.veinScale,
                                persistence: 0.5f,
                                lacunarity: 2.0f
                            );

                            if (oreNoise > ore.rarity)
                            {
                                foreground[x, y] = ore.oreType;
                                break; // Only one ore per voxel cell
                            }
                        }
                    }

                    // Special Underworld features: pockets of Magma & Mud
                    if (y <= WorldSettings.UnderworldLevel && foreground[x, y] == BlockType.Obsidian)
                    {
                        float magmaNoise = NoiseGenerator.Fbm2D(worldX, y, seed + 991, 2, 0.09f);
                        if (magmaNoise > 0.72f)
                        {
                            foreground[x, y] = BlockType.Magma;
                        }
                        else if (magmaNoise < 0.28f)
                        {
                            foreground[x, y] = BlockType.Mud;
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Pass 5: Cavern Foliage & Glowing Shrooms
            // -----------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                float worldX = WorldSettings.LocalToWorldX(chunkX, x);
                for (int y = WorldSettings.BedrockLevel + 2; y < WorldSettings.CavernLevel; y++)
                {
                    // Check if block is Air and has a solid floor beneath it
                    if (foreground[x, y] == BlockType.Air && BlockRegistry.IsSolid(foreground[x, y - 1]))
                    {
                        // Bioluminescent glowing mushrooms in deep cavern and underworld
                        float shroomNoise = NoiseGenerator.Fbm2D(worldX, y, seed + 4411, 1, 0.2f);
                        if (shroomNoise > 0.82f)
                        {
                            foreground[x, y] = BlockType.GlowShroom;
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // Pass 6: Surface Flora & Tree Generation
            // -----------------------------------------------------------------
            for (int x = 1; x < width - 1; x++)
            {
                int surfaceY = surfaceHeights[x];
                BlockType surfaceBlock = foreground[x, surfaceY];

                // Vegetation requires a solid surface floor
                if (!BlockRegistry.IsSolid(surfaceBlock)) continue;

                BiomeType biome = columnBiomes[x];
                BiomeConfig config = GetConfig(biome);

                // Deterministic pseudo-random roll per column
                int columnHash = (chunkX * width + x) * 31337 ^ seed;
                System.Random rng = new System.Random(columnHash);
                double roll = rng.NextDouble();

                if (roll < config.treeSpawnProbability)
                {
                    switch (config.vegetationType)
                    {
                        case VegetationType.OakTree:
                            SpawnOakTree(foreground, x, surfaceY + 1, rng, width, height);
                            break;

                        case VegetationType.Cactus:
                            SpawnCactus(foreground, x, surfaceY + 1, rng, width, height);
                            break;

                        case VegetationType.PineTree:
                            SpawnPineTree(foreground, x, surfaceY + 1, rng, width, height);
                            break;

                        case VegetationType.GlowShroomTree:
                            SpawnGlowShroomTree(foreground, x, surfaceY + 1, rng, width, height);
                            break;
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Flora Spawners (Trunks, Foliage & Branches)
        // ---------------------------------------------------------------------

        private void SpawnOakTree(BlockType[,] grid, int originX, int originY, System.Random rng, int width, int height)
        {
            int trunkHeight = rng.Next(5, 8);
            int topY = originY + trunkHeight;

            if (topY + 3 >= height) return;

            // Wood Trunk
            for (int y = originY; y < topY; y++)
            {
                grid[originX, y] = BlockType.Wood;
            }

            // Leaf Canopy (Rounded 5x5 crown with trimmed corners)
            for (int dx = -2; dx <= 2; dx++)
            {
                for (int dy = -1; dy <= 2; dy++)
                {
                    int lx = originX + dx;
                    int ly = topY + dy;

                    if (lx >= 0 && lx < width && ly >= 0 && ly < height)
                    {
                        // Round corners of outer ring
                        if (Mathf.Abs(dx) == 2 && dy == 2) continue;
                        if (Mathf.Abs(dx) == 2 && dy == -1 && rng.NextDouble() > 0.5) continue;

                        if (grid[lx, ly] == BlockType.Air)
                        {
                            grid[lx, ly] = BlockType.Leaves;
                        }
                    }
                }
            }
        }

        private void SpawnCactus(BlockType[,] grid, int originX, int originY, System.Random rng, int width, int height)
        {
            int cactusHeight = rng.Next(3, 6);
            int topY = originY + cactusHeight;

            if (topY >= height) return;

            // Main vertical trunk
            for (int y = originY; y < topY; y++)
            {
                grid[originX, y] = BlockType.Cactus;
            }

            // Left lateral arm
            if (cactusHeight >= 4 && originX > 0 && rng.NextDouble() > 0.4)
            {
                int armY = originY + rng.Next(1, cactusHeight - 1);
                grid[originX - 1, armY] = BlockType.Cactus;
                if (armY + 1 < height)
                {
                    grid[originX - 1, armY + 1] = BlockType.Cactus;
                }
            }

            // Right lateral arm
            if (cactusHeight >= 4 && originX < width - 1 && rng.NextDouble() > 0.4)
            {
                int armY = originY + rng.Next(1, cactusHeight - 1);
                grid[originX + 1, armY] = BlockType.Cactus;
                if (armY + 1 < height)
                {
                    grid[originX + 1, armY + 1] = BlockType.Cactus;
                }
            }
        }

        private void SpawnPineTree(BlockType[,] grid, int originX, int originY, System.Random rng, int width, int height)
        {
            int trunkHeight = rng.Next(6, 10);
            int topY = originY + trunkHeight;

            if (topY + 2 >= height) return;

            // Wood Trunk
            for (int y = originY; y < topY; y++)
            {
                grid[originX, y] = BlockType.Wood;
            }

            // Conical pine tiers
            int foliageBottom = originY + 2;
            for (int y = foliageBottom; y <= topY + 1; y++)
            {
                int radius = (topY + 1 - y) / 2 + 1;
                for (int dx = -radius; dx <= radius; dx++)
                {
                    int lx = originX + dx;
                    if (lx >= 0 && lx < width && y >= 0 && y < height)
                    {
                        if (grid[lx, y] == BlockType.Air)
                        {
                            grid[lx, y] = BlockType.Leaves;
                        }
                    }
                }
            }

            // Snow tip
            if (topY + 2 < height && grid[originX, topY + 2] == BlockType.Air)
            {
                grid[originX, topY + 2] = BlockType.Snow;
            }
        }

        private void SpawnGlowShroomTree(BlockType[,] grid, int originX, int originY, System.Random rng, int width, int height)
        {
            int stemHeight = rng.Next(2, 4);
            int topY = originY + stemHeight;

            if (topY + 1 >= height) return;

            for (int y = originY; y < topY; y++)
            {
                grid[originX, y] = BlockType.Mud;
            }

            // Giant glowing cap
            for (int dx = -1; dx <= 1; dx++)
            {
                int lx = originX + dx;
                if (lx >= 0 && lx < width && topY < height)
                {
                    grid[lx, topY] = BlockType.GlowShroom;
                }
            }
        }
    }
}
