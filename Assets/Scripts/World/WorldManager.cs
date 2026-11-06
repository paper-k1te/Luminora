using System;
using System.Collections.Generic;
using Core;
using Generation;
using Rendering;
using UnityEngine;

namespace World
{
    /// <summary>
    /// Master coordinator for the 2.5D sandbox world.
    /// Manages chunk streaming around the player, dynamic block editing,
    /// cross-chunk block queries, and procedural material initialization.
    /// </summary>
    [DefaultExecutionOrder(-50)]
    public class WorldManager : MonoBehaviour
    {
        public static WorldManager Instance { get; private set; }

        [Header("References")]
        [SerializeField] private TerrainGenerator terrainGenerator;
        [SerializeField] private Transform playerTransform;
        [SerializeField] private Transform chunkContainer;

        [Header("Streaming & View Distance")]
        [Tooltip("Horizontal radius of chunks kept active around the player.")]
        [SerializeField] private int viewDistance = 5;

        [Header("Materials (Optional Overrides)")]
        [SerializeField] private Material blockMaterial;
        [SerializeField] private Material wallMaterial;

        // Active chunk storage keyed by chunk horizontal index
        private readonly Dictionary<int, Chunk> _activeChunks = new Dictionary<int, Chunk>();
        private readonly List<Chunk> _dirtyChunks = new List<Chunk>();
        private readonly HashSet<int> _chunksInProgress = new HashSet<int>();

        private int _lastPlayerChunkX = int.MinValue;

        public TerrainGenerator TerrainGen => terrainGenerator;
        public int ViewDistance => viewDistance;
        public Transform PlayerTransform => playerTransform;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            if (terrainGenerator == null)
            {
                terrainGenerator = GetComponent<TerrainGenerator>() ?? gameObject.AddComponent<TerrainGenerator>();
            }

            if (chunkContainer == null)
            {
                GameObject containerObj = new GameObject("ChunksContainer");
                containerObj.transform.SetParent(transform);
                chunkContainer = containerObj.transform;
            }

            // Ensure materials from TextureAtlasGenerator if not assigned
            if (blockMaterial == null)
            {
                blockMaterial = TextureAtlasGenerator.GetOrCreateAtlasMaterial();
            }
            if (wallMaterial == null)
            {
                wallMaterial = TextureAtlasGenerator.GetOrCreateWallMaterial();
            }

            // Wire cross-chunk neighbor block lookup for seamless bevel culling
            ChunkMeshBuilder.OnQueryNeighborBlock = (wx, wy) => GetBlock(wx, wy);
        }

        private void Start()
        {
            // Initial chunk generation around spawn origin
            int centerChunk = 0;
            if (playerTransform != null)
            {
                centerChunk = WorldToChunkX(Mathf.FloorToInt(playerTransform.position.x));
            }

            PreloadChunksAround(centerChunk);

            // Rebuild all initial chunk meshes
            RebuildAllDirtyChunks();

            // Position player safely on surface if needed
            if (playerTransform != null && playerTransform.position == Vector3.zero)
            {
                Vector3 spawnPos = FindSurfaceSpawnPosition(0f);
                playerTransform.position = spawnPos;
            }
        }

        private void Update()
        {
            if (playerTransform == null)
            {
                GameObject playerObj = GameObject.FindWithTag("Player");
                if (playerObj != null) playerTransform = playerObj.transform;
            }

            if (playerTransform != null)
            {
                int currentChunkX = WorldToChunkX(Mathf.FloorToInt(playerTransform.position.x));
                if (currentChunkX != _lastPlayerChunkX)
                {
                    _lastPlayerChunkX = currentChunkX;
                    UpdateChunkStreaming(currentChunkX);
                }
            }

            // Rebuild any chunks that were modified this frame
            if (_dirtyChunks.Count > 0)
            {
                RebuildAllDirtyChunks();
            }
        }

        /// <summary>
        /// Converts world X coordinate to chunk column index.
        /// </summary>
        public static int WorldToChunkX(int worldX)
        {
            if (worldX >= 0)
                return worldX / WorldSettings.ChunkWidth;
            return ((worldX + 1) / WorldSettings.ChunkWidth) - 1;
        }

        /// <summary>
        /// Converts world X coordinate to local chunk X coordinate (0 to ChunkWidth - 1).
        /// </summary>
        public static int WorldToLocalX(int worldX, int chunkX)
        {
            return worldX - (chunkX * WorldSettings.ChunkWidth);
        }

        /// <summary>
        /// Queries the foreground block at global world coordinates.
        /// </summary>
        public BlockType GetBlock(int worldX, int worldY)
        {
            if (worldY < 0 || worldY >= WorldSettings.ChunkHeight)
            {
                return BlockType.Air;
            }

            int chunkX = WorldToChunkX(worldX);
            int localX = WorldToLocalX(worldX, chunkX);

            if (_activeChunks.TryGetValue(chunkX, out Chunk chunk))
            {
                return chunk.GetBlock(localX, worldY);
            }

            return BlockType.Air;
        }

        /// <summary>
        /// Modifies the foreground block at global world coordinates.
        /// Automatically handles chunk dirty state and marks adjacent chunks if editing boundary blocks.
        /// </summary>
        public bool SetBlock(int worldX, int worldY, BlockType type, bool notifyNeighbors = true)
        {
            if (worldY < 0 || worldY >= WorldSettings.ChunkHeight)
            {
                return false;
            }

            int chunkX = WorldToChunkX(worldX);
            int localX = WorldToLocalX(worldX, chunkX);

            if (_activeChunks.TryGetValue(chunkX, out Chunk chunk))
            {
                bool changed = chunk.SetBlock(localX, worldY, type);
                if (changed)
                {
                    SaveManager.Instance?.RecordBlockModification(worldX, worldY, type, isWall: false);
                    WorldSaveSystem.RegisterBlockChange(worldX, worldY, type);

                    // Luminora dual-layer logic: if placing foreground block where background wall is empty, adapt background wall
                    if (type != BlockType.Air && chunk.GetWall(localX, worldY) == BlockType.Air)
                    {
                        BlockType matchingWall = BlockRegistry.GetMatchingWallType(type);
                        if (matchingWall != BlockType.Air)
                        {
                            chunk.SetWall(localX, worldY, matchingWall);
                            SaveManager.Instance?.RecordBlockModification(worldX, worldY, matchingWall, isWall: true);
                            WorldSaveSystem.RegisterWallChange(worldX, worldY, matchingWall);
                        }
                    }

                    MarkChunkDirty(chunk);

                    if (notifyNeighbors)
                    {
                        // Left boundary: update left neighboring chunk bevels
                        if (localX == 0 && _activeChunks.TryGetValue(chunkX - 1, out Chunk leftChunk))
                        {
                            MarkChunkDirty(leftChunk);
                        }
                        // Right boundary: update right neighboring chunk bevels
                        else if (localX == WorldSettings.ChunkWidth - 1 && _activeChunks.TryGetValue(chunkX + 1, out Chunk rightChunk))
                        {
                            MarkChunkDirty(rightChunk);
                        }
                    }
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Queries the background wall block at global world coordinates.
        /// </summary>
        public BlockType GetWall(int worldX, int worldY)
        {
            if (worldY < 0 || worldY >= WorldSettings.ChunkHeight)
            {
                return BlockType.Air;
            }

            int chunkX = WorldToChunkX(worldX);
            int localX = WorldToLocalX(worldX, chunkX);

            if (_activeChunks.TryGetValue(chunkX, out Chunk chunk))
            {
                return chunk.GetWall(localX, worldY);
            }

            return BlockType.Air;
        }

        /// <summary>
        /// Modifies the background wall block at global world coordinates.
        /// </summary>
        public bool SetWall(int worldX, int worldY, BlockType type)
        {
            if (worldY < 0 || worldY >= WorldSettings.ChunkHeight)
            {
                return false;
            }

            int chunkX = WorldToChunkX(worldX);
            int localX = WorldToLocalX(worldX, chunkX);

            if (_activeChunks.TryGetValue(chunkX, out Chunk chunk))
            {
                bool changed = chunk.SetWall(localX, worldY, type);
                if (changed)
                {
                    SaveManager.Instance?.RecordBlockModification(worldX, worldY, type, isWall: true);
                    WorldSaveSystem.RegisterWallChange(worldX, worldY, type);
                    MarkChunkDirty(chunk);
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Retrieves the macro biome at the specified horizontal position.
        /// </summary>
        public BiomeType GetBiomeAt(float worldX)
        {
            if (terrainGenerator != null)
            {
                return terrainGenerator.GetBiomeAt(worldX);
            }
            return BiomeType.Forest;
        }

        /// <summary>
        /// Calculates a safe surface spawn position for the player at or near preferredX.
        /// </summary>
        public Vector3 FindSurfaceSpawnPosition(float preferredX = 0f)
        {
            float targetY = WorldSettings.BaseSurfaceLevel + 4f;
            if (terrainGenerator != null)
            {
                targetY = terrainGenerator.GetSurfaceHeight(preferredX) + 2f;
            }

            return new Vector3(preferredX, targetY, 0f);
        }

        /// <summary>
        /// Gets an active chunk by chunk coordinate, or null if not currently loaded.
        /// </summary>
        public Chunk GetChunk(int chunkX)
        {
            _activeChunks.TryGetValue(chunkX, out Chunk chunk);
            return chunk;
        }

        /// <summary>
        /// Preloads and builds all chunks in a radius around the center chunk.
        /// </summary>
        public void PreloadChunksAround(int centerChunkX)
        {
            for (int x = centerChunkX - viewDistance; x <= centerChunkX + viewDistance; x++)
            {
                GetOrCreateChunk(x);
            }
        }

        /// <summary>
        /// Streams in new chunks that entered view and deactivates/unloads distant chunks.
        /// </summary>
        private void UpdateChunkStreaming(int centerChunkX)
        {
            // Load new chunks
            for (int x = centerChunkX - viewDistance; x <= centerChunkX + viewDistance; x++)
            {
                if (!_activeChunks.ContainsKey(x))
                {
                    GetOrCreateChunk(x);
                }
            }

            // Unload far chunks
            List<int> toRemove = null;
            foreach (var kvp in _activeChunks)
            {
                if (Mathf.Abs(kvp.Key - centerChunkX) > viewDistance + 2)
                {
                    if (toRemove == null) toRemove = new List<int>();
                    toRemove.Add(kvp.Key);
                }
            }

            if (toRemove != null)
            {
                foreach (int cx in toRemove)
                {
                    if (_activeChunks.TryGetValue(cx, out Chunk chunk))
                    {
                        _activeChunks.Remove(cx);
                        if (chunk != null)
                        {
                            Destroy(chunk.gameObject);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Retrieves an existing chunk or generates a new procedural chunk instance.
        /// </summary>
        public Chunk GetOrCreateChunk(int chunkX)
        {
            if (_activeChunks.TryGetValue(chunkX, out Chunk existing))
            {
                return existing;
            }

            if (_chunksInProgress.Contains(chunkX))
            {
                return null;
            }

            _chunksInProgress.Add(chunkX);

            // Create chunk GameObject
            GameObject chunkObj = new GameObject($"Chunk_{chunkX}");
            chunkObj.transform.SetParent(chunkContainer, false);
            chunkObj.transform.position = new Vector3(chunkX * WorldSettings.ChunkWidth * WorldSettings.BlockSize, 0f, 0f);

            Chunk chunk = chunkObj.AddComponent<Chunk>();
            chunk.Initialize(chunkX, blockMaterial, wallMaterial);

            // Generate procedural terrain data
            if (terrainGenerator != null)
            {
                terrainGenerator.GenerateChunkData(chunkX, out BlockType[,] fg, out BlockType[,] bg);
                chunk.LoadData(fg, bg);
            }

            // Apply any persisted block/wall modifications to this chunk
            SaveManager.Instance?.ApplyModificationsToChunk(chunk);
            WorldSaveSystem.ApplyModificationsToChunk(chunk);

            _activeChunks[chunkX] = chunk;
            _chunksInProgress.Remove(chunkX);

            MarkChunkDirty(chunk);
            return chunk;
        }

        public void MarkChunkDirty(Chunk chunk)
        {
            if (chunk != null && !_dirtyChunks.Contains(chunk))
            {
                chunk.IsDirty = true;
                _dirtyChunks.Add(chunk);
            }
        }

        private void RebuildAllDirtyChunks()
        {
            for (int i = 0; i < _dirtyChunks.Count; i++)
            {
                Chunk chunk = _dirtyChunks[i];
                if (chunk != null)
                {
                    chunk.RebuildMesh();
                }
            }
            _dirtyChunks.Clear();
        }

        /// <summary>
        /// Clears all chunks and regenerates world from scratch.
        /// </summary>
        public void RegenerateWorld(int newSeed = -1)
        {
            if (newSeed >= 0 && terrainGenerator != null)
            {
                terrainGenerator.Seed = newSeed;
            }

            foreach (var chunk in _activeChunks.Values)
            {
                if (chunk != null) Destroy(chunk.gameObject);
            }
            _activeChunks.Clear();
            _dirtyChunks.Clear();
            _chunksInProgress.Clear();

            int centerChunk = 0;
            if (playerTransform != null)
            {
                centerChunk = WorldToChunkX(Mathf.FloorToInt(playerTransform.position.x));
            }

            PreloadChunksAround(centerChunk);
            RebuildAllDirtyChunks();

            if (playerTransform != null)
            {
                playerTransform.position = FindSurfaceSpawnPosition(playerTransform.position.x);
            }
        }

        /// <summary>
        /// Clears and reconstructs world chunks for the specified seed and player position,
        /// applying all persisted block deltas from SaveManager.
        /// </summary>
        public void LoadWorldState(int seed, Vector3 playerPos)
        {
            if (terrainGenerator != null)
            {
                terrainGenerator.Seed = seed;
            }

            foreach (var chunk in _activeChunks.Values)
            {
                if (chunk != null) Destroy(chunk.gameObject);
            }
            _activeChunks.Clear();
            _dirtyChunks.Clear();
            _chunksInProgress.Clear();

            int centerChunk = WorldToChunkX(Mathf.FloorToInt(playerPos.x));
            PreloadChunksAround(centerChunk);
            RebuildAllDirtyChunks();
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
            ChunkMeshBuilder.OnQueryNeighborBlock = null;
        }
    }
}
