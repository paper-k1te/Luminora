using System;
using Core;
using UnityEngine;

namespace World
{
    /// <summary>
    /// Represents a single vertical slice (column) of the voxel world.
    /// Manages foreground interactive block arrays, decorative background walls,
    /// and their associated rendering/physics components.
    /// </summary>
    [SelectionBase]
    [RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
    public class Chunk : MonoBehaviour
    {
        [Header("Chunk Coordinates")]
        [SerializeField] private int chunkX;

        [Header("State")]
        [SerializeField] private bool isDirty = false;
        [SerializeField] private bool isGenerated = false;

        // Foreground rendering & collision
        public MeshFilter meshFilter;
        public MeshRenderer meshRenderer;
        public MeshCollider meshCollider;

        // Background wall rendering
        public MeshFilter wallMeshFilter;
        public MeshRenderer wallMeshRenderer;

        // Voxel grids: [localX, y]
        public BlockType[,] blocks;
        public BlockType[,] wallBlocks;

        // Public accessors
        public int ChunkX => chunkX;
        public bool IsDirty
        {
            get => isDirty;
            set => isDirty = value;
        }
        public bool IsGenerated
        {
            get => isGenerated;
            set => isGenerated = value;
        }

        /// <summary>
        /// Global world position along X for the left boundary of this chunk.
        /// </summary>
        public float WorldStartX => chunkX * WorldSettings.ChunkWidth * WorldSettings.BlockSize;

        private void Awake()
        {
            EnsureComponents();
        }

        /// <summary>
        /// Ensures all necessary MeshFilters, Renderers, and Colliders exist on this GameObject
        /// and its designated background wall child object.
        /// </summary>
        public void EnsureComponents()
        {
            if (meshFilter == null) meshFilter = GetComponent<MeshFilter>();
            if (meshFilter == null) meshFilter = gameObject.AddComponent<MeshFilter>();

            if (meshRenderer == null) meshRenderer = GetComponent<MeshRenderer>();
            if (meshRenderer == null) meshRenderer = gameObject.AddComponent<MeshRenderer>();

            if (meshCollider == null) meshCollider = GetComponent<MeshCollider>();
            if (meshCollider == null) meshCollider = gameObject.AddComponent<MeshCollider>();

            // Setup or link background wall child
            Transform wallTransform = transform.Find("WallMesh");
            if (wallTransform == null)
            {
                GameObject wallObj = new GameObject("WallMesh");
                wallObj.transform.SetParent(transform, false);
                wallTransform = wallObj.transform;
            }

            if (wallMeshFilter == null)
            {
                wallMeshFilter = wallTransform.GetComponent<MeshFilter>() ?? wallTransform.gameObject.AddComponent<MeshFilter>();
            }

            if (wallMeshRenderer == null)
            {
                wallMeshRenderer = wallTransform.GetComponent<MeshRenderer>() ?? wallTransform.gameObject.AddComponent<MeshRenderer>();
            }

            // Ensure 2D arrays are allocated
            int width = WorldSettings.ChunkWidth;
            int height = WorldSettings.ChunkHeight;

            if (blocks == null || blocks.GetLength(0) != width || blocks.GetLength(1) != height)
            {
                blocks = new BlockType[width, height];
            }

            if (wallBlocks == null || wallBlocks.GetLength(0) != width || wallBlocks.GetLength(1) != height)
            {
                wallBlocks = new BlockType[width, height];
            }
        }

        /// <summary>
        /// Initializes the chunk at a given coordinate with specified materials.
        /// </summary>
        public void Initialize(int chunkXCoord, Material blockMaterial, Material wallMaterial)
        {
            EnsureComponents();

            chunkX = chunkXCoord;
            gameObject.name = $"Chunk_{chunkX}";
            transform.position = new Vector3(chunkX * WorldSettings.ChunkWidth * WorldSettings.BlockSize, 0f, 0f);

            if (blockMaterial != null && meshRenderer != null)
            {
                meshRenderer.sharedMaterial = blockMaterial;
            }

            if (wallMaterial != null && wallMeshRenderer != null)
            {
                wallMeshRenderer.sharedMaterial = wallMaterial;
            }

            isDirty = true;
            isGenerated = false;
        }

        /// <summary>
        /// Populates chunk block data directly from generated arrays.
        /// </summary>
        public void LoadData(BlockType[,] foregroundData, BlockType[,] backgroundData)
        {
            EnsureComponents();

            int width = WorldSettings.ChunkWidth;
            int height = WorldSettings.ChunkHeight;

            if (foregroundData != null && foregroundData.GetLength(0) == width && foregroundData.GetLength(1) == height)
            {
                Array.Copy(foregroundData, blocks, width * height);
            }

            if (backgroundData != null && backgroundData.GetLength(0) == width && backgroundData.GetLength(1) == height)
            {
                Array.Copy(backgroundData, wallBlocks, width * height);
            }

            isGenerated = true;
            isDirty = true;
        }

        /// <summary>
        /// Retrieves the foreground block at the specified local coordinates.
        /// Returns BlockType.Air if coordinates are out of bounds.
        /// </summary>
        public BlockType GetBlock(int localX, int y)
        {
            if (localX < 0 || localX >= WorldSettings.ChunkWidth || y < 0 || y >= WorldSettings.ChunkHeight)
            {
                return BlockType.Air;
            }
            return blocks[localX, y];
        }

        /// <summary>
        /// Updates the foreground block at the specified local coordinates.
        /// Automatically marks the chunk dirty for mesh reconstruction.
        /// </summary>
        public bool SetBlock(int localX, int y, BlockType type)
        {
            if (localX < 0 || localX >= WorldSettings.ChunkWidth || y < 0 || y >= WorldSettings.ChunkHeight)
            {
                return false;
            }

            if (blocks[localX, y] != type)
            {
                blocks[localX, y] = type;
                isDirty = true;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Retrieves the background wall at the specified local coordinates.
        /// Returns BlockType.Air if out of bounds.
        /// </summary>
        public BlockType GetWall(int localX, int y)
        {
            if (localX < 0 || localX >= WorldSettings.ChunkWidth || y < 0 || y >= WorldSettings.ChunkHeight)
            {
                return BlockType.Air;
            }
            return wallBlocks[localX, y];
        }

        /// <summary>
        /// Updates the background wall at the specified local coordinates.
        /// </summary>
        public bool SetWall(int localX, int y, BlockType type)
        {
            if (localX < 0 || localX >= WorldSettings.ChunkWidth || y < 0 || y >= WorldSettings.ChunkHeight)
            {
                return false;
            }

            if (wallBlocks[localX, y] != type)
            {
                wallBlocks[localX, y] = type;
                isDirty = true;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Triggers mesh regeneration for this chunk via ChunkMeshBuilder.
        /// Clears the dirty flag upon completion.
        /// </summary>
        public void RebuildMesh()
        {
            ChunkMeshBuilder.BuildChunkMesh(this);
            isDirty = false;
        }

        private void OnDestroy()
        {
            // Clean up dynamically created meshes to prevent leaks in editor or play mode
            if (meshFilter != null && meshFilter.sharedMesh != null)
            {
                DestroyImmediate(meshFilter.sharedMesh);
            }
            if (wallMeshFilter != null && wallMeshFilter.sharedMesh != null)
            {
                DestroyImmediate(wallMeshFilter.sharedMesh);
            }
        }
    }
}
