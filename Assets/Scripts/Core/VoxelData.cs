using System;
using UnityEngine;

namespace Core
{
    /// <summary>
    /// Cube face classifications for 2.5D voxel meshing.
    /// </summary>
    public enum BlockFace : byte
    {
        Front = 0,   // Facing camera (-Z)
        Top = 1,     // +Y
        Bottom = 2,  // -Y
        Left = 3,    // -X
        Right = 4,   // +X
        Back = 5     // +Z
    }

    /// <summary>
    /// Fundamental geometry tables, normal vectors, quad indices, and UV atlas mapping
    /// for 2.5D voxel blocks and decorative background walls.
    /// </summary>
    public static class VoxelData
    {
        // -------------------------------------------------------------
        // Dimensional Constants
        // -------------------------------------------------------------
        public const float BlockSize = WorldSettings.BlockSize;    // 1.0f
        public const float BlockDepth = WorldSettings.BlockDepth;  // 0.8f extrusion depth into Z
        public const float WallZ = 1.0f;                           // Background wall planar offset
        public const float WallDepth = 0.2f;                       // Extrusion depth for wall tiles

        // -------------------------------------------------------------
        // Texture Atlas Layout (16x16 grid = 256 tiles)
        // -------------------------------------------------------------
        public const int AtlasTilesAcross = 16;
        public const int AtlasTilesDown = 16;
        public const float TileNormalizedWidth = 1.0f / AtlasTilesAcross;
        public const float TileNormalizedHeight = 1.0f / AtlasTilesDown;
        public const float UvPadding = 0.0015f; // Prevents pixel bleeding across mipmaps/filtering

        // Standard quad indices (two triangles per quad)
        public static readonly int[] QuadTriangles = new int[6] { 0, 1, 2, 0, 2, 3 };

        // -------------------------------------------------------------
        // Face Normals
        // -------------------------------------------------------------
        public static readonly Vector3[] FaceNormals = new Vector3[6]
        {
            Vector3.back,    // Front: (0, 0, -1) towards camera
            Vector3.up,      // Top: (0, 1, 0)
            Vector3.down,    // Bottom: (0, -1, 0)
            Vector3.left,    // Left: (-1, 0, 0)
            Vector3.right,   // Right: (1, 0, 0)
            Vector3.forward  // Back: (0, 0, 1)
        };

        // -------------------------------------------------------------
        // Ambient Occlusion / Bevel Shading Vertex Colors
        // -------------------------------------------------------------
        public static readonly Color32 ColorFront = new Color32(255, 255, 255, 255);
        public static readonly Color32 ColorTop = new Color32(255, 255, 255, 255);
        public static readonly Color32 ColorBottom = new Color32(135, 135, 135, 255); // Under-bevel shadow
        public static readonly Color32 ColorLeft = new Color32(195, 195, 195, 255);   // Side shadow
        public static readonly Color32 ColorRight = new Color32(225, 225, 225, 255);  // Side bevel highlight
        public static readonly Color32 ColorBack = new Color32(110, 110, 110, 255);
        public static readonly Color32 ColorWall = new Color32(120, 120, 120, 255);   // Background contrast

        public static Color32 GetFaceTint(BlockFace face)
        {
            switch (face)
            {
                case BlockFace.Front: return ColorFront;
                case BlockFace.Top: return ColorTop;
                case BlockFace.Bottom: return ColorBottom;
                case BlockFace.Left: return ColorLeft;
                case BlockFace.Right: return ColorRight;
                case BlockFace.Back: return ColorBack;
                default: return ColorFront;
            }
        }

        // -------------------------------------------------------------
        // Vertex Offset Tables [Face, VertexIndex 0..3]
        // -------------------------------------------------------------
        private static readonly Vector3[][] RawFaceVertices = new Vector3[6][]
        {
            // Front (-Z)
            new Vector3[]
            {
                new Vector3(0f, 0f, 0f),
                new Vector3(0f, 1f, 0f),
                new Vector3(1f, 1f, 0f),
                new Vector3(1f, 0f, 0f)
            },
            // Top (+Y)
            new Vector3[]
            {
                new Vector3(0f, 1f, 0f),
                new Vector3(0f, 1f, BlockDepth),
                new Vector3(1f, 1f, BlockDepth),
                new Vector3(1f, 1f, 0f)
            },
            // Bottom (-Y)
            new Vector3[]
            {
                new Vector3(0f, 0f, BlockDepth),
                new Vector3(0f, 0f, 0f),
                new Vector3(1f, 0f, 0f),
                new Vector3(1f, 0f, BlockDepth)
            },
            // Left (-X)
            new Vector3[]
            {
                new Vector3(0f, 0f, BlockDepth),
                new Vector3(0f, 1f, BlockDepth),
                new Vector3(0f, 1f, 0f),
                new Vector3(0f, 0f, 0f)
            },
            // Right (+X)
            new Vector3[]
            {
                new Vector3(1f, 0f, 0f),
                new Vector3(1f, 1f, 0f),
                new Vector3(1f, 1f, BlockDepth),
                new Vector3(1f, 0f, BlockDepth)
            },
            // Back (+Z)
            new Vector3[]
            {
                new Vector3(1f, 0f, BlockDepth),
                new Vector3(1f, 1f, BlockDepth),
                new Vector3(0f, 1f, BlockDepth),
                new Vector3(0f, 0f, BlockDepth)
            }
        };

        // Background Wall Vertices at Z = WallZ
        private static readonly Vector3[] RawWallVertices = new Vector3[4]
        {
            new Vector3(0f, 0f, WallZ),
            new Vector3(0f, 1f, WallZ),
            new Vector3(1f, 1f, WallZ),
            new Vector3(1f, 0f, WallZ)
        };

        /// <summary>
        /// Retrieves the 4 localized vertex positions for a specific block face.
        /// </summary>
        public static Vector3 GetFaceVertex(BlockFace face, int vertexIndex)
        {
            return RawFaceVertices[(int)face][vertexIndex];
        }

        /// <summary>
        /// Populates destination array with 4 localized vertex positions for a block face.
        /// </summary>
        public static void GetFaceVertices(BlockFace face, Vector3 origin, Vector3[] destination, int offset = 0)
        {
            Vector3[] src = RawFaceVertices[(int)face];
            destination[offset + 0] = origin + src[0];
            destination[offset + 1] = origin + src[1];
            destination[offset + 2] = origin + src[2];
            destination[offset + 3] = origin + src[3];
        }

        /// <summary>
        /// Populates destination array with 4 localized vertex positions for a background wall quad.
        /// </summary>
        public static void GetWallVertices(Vector3 origin, Vector3[] destination, int offset = 0)
        {
            destination[offset + 0] = origin + RawWallVertices[0];
            destination[offset + 1] = origin + RawWallVertices[1];
            destination[offset + 2] = origin + RawWallVertices[2];
            destination[offset + 3] = origin + RawWallVertices[3];
        }

        // -------------------------------------------------------------
        // UV Coordinate Helpers
        // -------------------------------------------------------------

        /// <summary>
        /// Calculates the normalized UV bounding rectangle for a given tile index in the atlas.
        /// Accounts for subpixel padding to avoid edge bleeding.
        /// </summary>
        public static Rect GetAtlasUVRect(int textureIndex)
        {
            if (textureIndex < 0) textureIndex = 0;

            int tileX = textureIndex % AtlasTilesAcross;
            int tileY = AtlasTilesDown - 1 - (textureIndex / AtlasTilesAcross);

            float uMin = (tileX * TileNormalizedWidth) + UvPadding;
            float uMax = ((tileX + 1) * TileNormalizedWidth) - UvPadding;
            float vMin = (tileY * TileNormalizedHeight) + UvPadding;
            float vMax = ((tileY + 1) * TileNormalizedHeight) - UvPadding;

            return new Rect(uMin, vMin, uMax - uMin, vMax - vMin);
        }

        /// <summary>
        /// Returns 4 UV coordinates (bottom-left, top-left, top-right, bottom-right) for a tile index.
        /// </summary>
        public static Vector2[] GetUVs(int textureIndex)
        {
            Rect r = GetAtlasUVRect(textureIndex);
            return new Vector2[4]
            {
                new Vector2(r.xMin, r.yMin), // Bottom-Left (vertex 0)
                new Vector2(r.xMin, r.yMax), // Top-Left    (vertex 1)
                new Vector2(r.xMax, r.yMax), // Top-Right   (vertex 2)
                new Vector2(r.xMax, r.yMin)  // Bottom-Right(vertex 3)
            };
        }

        /// <summary>
        /// Fills a preallocated buffer with 4 UV coordinates for a tile index.
        /// </summary>
        public static void GetUVs(int textureIndex, Vector2[] buffer, int offset = 0)
        {
            Rect r = GetAtlasUVRect(textureIndex);
            buffer[offset + 0] = new Vector2(r.xMin, r.yMin);
            buffer[offset + 1] = new Vector2(r.xMin, r.yMax);
            buffer[offset + 2] = new Vector2(r.xMax, r.yMax);
            buffer[offset + 3] = new Vector2(r.xMax, r.yMin);
        }

        /// <summary>
        /// Resolves the specific texture index for a block type based on the rendering face.
        /// Allows multi-textured blocks (such as grass with unique top and side textures).
        /// </summary>
        public static int GetFaceTextureIndex(BlockType type, BlockFace face)
        {
            if (type == BlockType.Air) return 0;

            // Handle unique face variations
            if (type == BlockType.Grass)
            {
                if (face == BlockFace.Top)
                {
                    // Pure lush grass top: texture index 27
                    return 27;
                }
                if (face == BlockFace.Bottom)
                {
                    // Dirt underbelly: texture index 2
                    return 2;
                }
                // Front and side bevels: grass-fringed side (texture index 1)
                return 1;
            }

            if (type == BlockType.Wood)
            {
                if (face == BlockFace.Top || face == BlockFace.Bottom)
                {
                    // Tree rings cross-section: texture index 28
                    return 28;
                }
                // Vertical bark side: texture index 4
                return 4;
            }

            if (type == BlockType.Cactus)
            {
                if (face == BlockFace.Top || face == BlockFace.Bottom)
                {
                    // Radial cactus crown: texture index 29
                    return 29;
                }
                // Ribbed cactus stem: texture index 8
                return 8;
            }

            // Default: look up in central registry
            BlockData data = BlockRegistry.GetBlock(type);
            return data != null ? data.textureIndex : (int)type;
        }
    }
}
