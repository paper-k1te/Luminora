using UnityEngine;

namespace Core
{
    /// <summary>
    /// Global dimensional, procedural, and simulation constants for the voxel world.
    /// </summary>
    public static class WorldSettings
    {
        // -------------------------------------------------------------
        // Chunk & Voxel Geometry
        // -------------------------------------------------------------
        public const int ChunkWidth = 16;
        public const int ChunkHeight = 128;
        public const float BlockSize = 1.0f;
        public const float BlockDepth = 0.8f; // Extrusion depth on the Z-axis for 2.5D perspective

        // -------------------------------------------------------------
        // World Vertical Layer Partitioning (0 to ChunkHeight)
        // -------------------------------------------------------------
        public const int BedrockLevel = 2;          // Y 0 - 2: Indestructible bedrock foundation
        public const int UnderworldLevel = 22;      // Y 2 - 22: Lava caverns, Magma, Obsidian, GlowShroom
        public const int CavernLevel = 55;         // Y 22 - 55: Deep rock caverns, rare ores
        public const int SubsurfaceLevel = 75;     // Y 55 - 75: Transition dirt/rock layer, common ores
        public const int BaseSurfaceLevel = 82;    // Mean surface baseline elevation
        public const int MaxSurfaceVariation = 20; // Height variation above and below baseline (+/- 20)

        // -------------------------------------------------------------
        // Noise Frequencies & Scales
        // -------------------------------------------------------------
        public const float TerrainNoiseFrequency = 0.012f;
        public const float CaveNoiseFrequency = 0.038f;
        public const float CaveThreshold = 0.62f;       // Swiss-cheese density cutoff
        public const float CaveWormFrequency = 0.025f;  // Tubular interconnecting tunnels
        public const float BiomeScale = 0.0035f;        // Spatial frequency for biome transitions
        public const float OreNoiseScale = 0.12f;

        // -------------------------------------------------------------
        // Runtime Simulation & Streaming
        // -------------------------------------------------------------
        public const int MaxVisibleChunks = 12;         // Horizontal chunk render window around player
        public const int ChunkLoadDistance = 6;         // Half-extent of active simulation radius

        // -------------------------------------------------------------
        // Procedural Seeds
        // -------------------------------------------------------------
        public static int ActiveSeed = 1337;

        /// <summary>
        /// Converts world horizontal X position to chunk index.
        /// Handles negative coordinates cleanly via floor division.
        /// </summary>
        public static int WorldToChunkX(float worldX)
        {
            return Mathf.FloorToInt(worldX / (ChunkWidth * BlockSize));
        }

        /// <summary>
        /// Converts world horizontal X position to local block X within chunk [0, ChunkWidth - 1].
        /// </summary>
        public static int WorldToLocalBlockX(float worldX)
        {
            int blockX = Mathf.FloorToInt(worldX / BlockSize);
            int localX = blockX % ChunkWidth;
            if (localX < 0) localX += ChunkWidth;
            return localX;
        }

        /// <summary>
        /// Converts world Y position to vertical block index [0, ChunkHeight - 1].
        /// </summary>
        public static int WorldToBlockY(float worldY)
        {
            return Mathf.Clamp(Mathf.FloorToInt(worldY / BlockSize), 0, ChunkHeight - 1);
        }

        /// <summary>
        /// Calculates absolute world X position from chunk index and local block coordinate.
        /// </summary>
        public static float LocalToWorldX(int chunkX, int localX)
        {
            return (chunkX * ChunkWidth + localX) * BlockSize;
        }

        /// <summary>
        /// Calculates absolute world Y position from block Y coordinate.
        /// </summary>
        public static float LocalToWorldY(int blockY)
        {
            return blockY * BlockSize;
        }
    }
}
