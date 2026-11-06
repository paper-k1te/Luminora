using System;
using System.Collections.Generic;
using Core;
using UnityEngine;
using UnityEngine.Rendering;

namespace World
{
    /// <summary>
    /// Placement orientation for torches and non-cube directional fixtures.
    /// </summary>
    public enum TorchOrientation
    {
        Floor = 0,     // Standing on solid block below
        LeftWall = 1,  // Mounted on left wall, leaning right
        RightWall = 2, // Mounted on right wall, leaning left
        BackWall = 3   // Mounted directly onto background wall
    }

    /// <summary>
    /// High-performance 2.5D voxel mesher with directional face culling,
    /// 3D bevel extrusion, ambient occlusion tinting, directional torch geometry,
    /// recessed background tree trunks, and optimized mesh collider generation.
    /// </summary>
    public static class ChunkMeshBuilder
    {
        // Global delegate for querying cross-chunk adjacent blocks
        public delegate BlockType NeighborBlockQuery(int worldX, int worldY);
        public static NeighborBlockQuery OnQueryNeighborBlock;

        // Reusable preallocated vertex buffers to eliminate GC allocations during meshing
        private static readonly List<Vector3> s_Vertices = new List<Vector3>(8192);
        private static readonly List<int> s_Triangles = new List<int>(12288);
        private static readonly List<Vector2> s_UVs = new List<Vector2>(8192);
        private static readonly List<Vector3> s_Normals = new List<Vector3>(8192);
        private static readonly List<Color32> s_Colors = new List<Color32>(8192);

        // Separate physical collider buffers (strictly solid blocks only: Leaves, Stone, Dirt, etc.)
        private static readonly List<Vector3> s_ColliderVertices = new List<Vector3>(8192);
        private static readonly List<int> s_ColliderTriangles = new List<int>(12288);

        // Reusable buffers for background wall mesh
        private static readonly List<Vector3> s_WallVertices = new List<Vector3>(4096);
        private static readonly List<int> s_WallTriangles = new List<int>(6144);
        private static readonly List<Vector2> s_WallUVs = new List<Vector2>(4096);
        private static readonly List<Vector3> s_WallNormals = new List<Vector3>(4096);
        private static readonly List<Color32> s_WallColors = new List<Color32>(4096);

        /// <summary>
        /// Generates foreground 2.5D extruded voxel mesh, physical collision mesh,
        /// and background wall quad mesh for a chunk.
        /// </summary>
        public static void BuildChunkMesh(Chunk chunk)
        {
            if (chunk == null) return;
            chunk.EnsureComponents();

            int width = WorldSettings.ChunkWidth;
            int height = WorldSettings.ChunkHeight;

            // Clear scratch buffers
            s_Vertices.Clear();
            s_Triangles.Clear();
            s_UVs.Clear();
            s_Normals.Clear();
            s_Colors.Clear();

            s_ColliderVertices.Clear();
            s_ColliderTriangles.Clear();

            s_WallVertices.Clear();
            s_WallTriangles.Clear();
            s_WallUVs.Clear();
            s_WallNormals.Clear();
            s_WallColors.Clear();

            // -------------------------------------------------------------
            // Pass 1: Build Foreground 2.5D Extruded Blocks & Collision
            // -------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    BlockType block = chunk.blocks[x, y];
                    if (block == BlockType.Air) continue;

                    // 1. Special Handling for Torches: Slender directional fixture
                    if (block == BlockType.Torch)
                    {
                        TorchOrientation torchOrient = DetermineTorchOrientation(chunk, x, y);
                        AddTorchMesh(new Vector3(x, y, 0f), torchOrient, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                        // Torches are non-solid and have no collision
                        continue;
                    }

                    // 2. Tree Trunks (Wood): Recessed in Z so player at Z = 0 is rendered in front of trunk
                    // Wood is non-solid (isSolid = false) so it produces no collision
                    float zOffset = (block == BlockType.Wood) ? 0.25f : 0f;
                    Vector3 origin = new Vector3(x, y, zOffset);
                    bool isSolid = BlockRegistry.IsSolid(block);

                    // Front face: facing -Z towards camera
                    int frontTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Front);
                    AddFace(origin, BlockFace.Front, frontTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                    if (isSolid)
                    {
                        AddColliderFace(origin, BlockFace.Front, s_ColliderVertices, s_ColliderTriangles);
                    }

                    // Top face (+Y)
                    BlockType topNeighbor = GetNeighborBlock(chunk, x, y + 1);
                    if (ShouldRenderSideFace(block, topNeighbor))
                    {
                        int topTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Top);
                        AddFace(origin, BlockFace.Top, topTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                        if (isSolid)
                        {
                            AddColliderFace(origin, BlockFace.Top, s_ColliderVertices, s_ColliderTriangles);
                        }
                    }

                    // Bottom face (-Y)
                    BlockType bottomNeighbor = GetNeighborBlock(chunk, x, y - 1);
                    if (ShouldRenderSideFace(block, bottomNeighbor))
                    {
                        int bottomTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Bottom);
                        AddFace(origin, BlockFace.Bottom, bottomTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                        if (isSolid)
                        {
                            AddColliderFace(origin, BlockFace.Bottom, s_ColliderVertices, s_ColliderTriangles);
                        }
                    }

                    // Left face (-X)
                    BlockType leftNeighbor = GetNeighborBlock(chunk, x - 1, y);
                    if (ShouldRenderSideFace(block, leftNeighbor))
                    {
                        int leftTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Left);
                        AddFace(origin, BlockFace.Left, leftTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                        if (isSolid)
                        {
                            AddColliderFace(origin, BlockFace.Left, s_ColliderVertices, s_ColliderTriangles);
                        }
                    }

                    // Right face (+X)
                    BlockType rightNeighbor = GetNeighborBlock(chunk, x + 1, y);
                    if (ShouldRenderSideFace(block, rightNeighbor))
                    {
                        int rightTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Right);
                        AddFace(origin, BlockFace.Right, rightTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                        if (isSolid)
                        {
                            AddColliderFace(origin, BlockFace.Right, s_ColliderVertices, s_ColliderTriangles);
                        }
                    }
                }
            }

            // -------------------------------------------------------------
            // Pass 2: Build Background Wall Quads (at Z = WallZ)
            // -------------------------------------------------------------
            if (chunk.wallBlocks != null)
            {
                for (int x = 0; x < width; x++)
                {
                    for (int y = 0; y < height; y++)
                    {
                        BlockType wall = chunk.wallBlocks[x, y];
                        if (wall == BlockType.Air) continue;

                        // Occlusion culling: if foreground block is completely opaque, wall is hidden
                        BlockType fg = chunk.blocks[x, y];
                        if (fg != BlockType.Air && !BlockRegistry.IsTransparent(fg) && fg != BlockType.Wood)
                        {
                            continue;
                        }

                        Vector3 origin = new Vector3(x, y, 0f);
                        int wallTex = VoxelData.GetFaceTextureIndex(wall, BlockFace.Front);
                        AddWallQuad(origin, wallTex, s_WallVertices, s_WallTriangles, s_WallUVs, s_WallNormals, s_WallColors);
                    }
                }
            }

            // -------------------------------------------------------------
            // Apply Foreground Visual Mesh
            // -------------------------------------------------------------
            if (chunk.meshFilter != null)
            {
                Mesh mesh = chunk.meshFilter.sharedMesh;
                if (mesh == null)
                {
                    mesh = new Mesh { name = $"Chunk_FG_{chunk.ChunkX}" };
                    chunk.meshFilter.sharedMesh = mesh;
                }
                else
                {
                    mesh.Clear();
                }

                if (s_Vertices.Count > 0)
                {
                    mesh.indexFormat = s_Vertices.Count > 65000 ? IndexFormat.UInt32 : IndexFormat.UInt16;
                    mesh.SetVertices(s_Vertices);
                    mesh.SetTriangles(s_Triangles, 0);
                    mesh.SetUVs(0, s_UVs);
                    mesh.SetNormals(s_Normals);
                    mesh.SetColors(s_Colors);
                    mesh.RecalculateBounds();
                }

                // Update physical collider with solid geometry only (Leaves, Stone, Dirt, etc.)
                // Tree trunks (Wood) and Torches have no collision
                if (chunk.meshCollider != null)
                {
                    Mesh colMesh = chunk.meshCollider.sharedMesh;
                    if (colMesh == null || colMesh == mesh)
                    {
                        colMesh = new Mesh { name = $"Chunk_Col_{chunk.ChunkX}" };
                    }
                    else
                    {
                        colMesh.Clear();
                    }

                    if (s_ColliderVertices.Count > 0)
                    {
                        colMesh.indexFormat = s_ColliderVertices.Count > 65000 ? IndexFormat.UInt32 : IndexFormat.UInt16;
                        colMesh.SetVertices(s_ColliderVertices);
                        colMesh.SetTriangles(s_ColliderTriangles, 0);
                        colMesh.RecalculateBounds();
                        chunk.meshCollider.sharedMesh = colMesh;
                    }
                    else
                    {
                        chunk.meshCollider.sharedMesh = null;
                    }
                }
            }

            // -------------------------------------------------------------
            // Apply Background Wall Mesh
            // -------------------------------------------------------------
            if (chunk.wallMeshFilter != null)
            {
                Mesh wallMesh = chunk.wallMeshFilter.sharedMesh;
                if (wallMesh == null)
                {
                    wallMesh = new Mesh { name = $"Chunk_Wall_{chunk.ChunkX}" };
                    chunk.wallMeshFilter.sharedMesh = wallMesh;
                }
                else
                {
                    wallMesh.Clear();
                }

                if (s_WallVertices.Count > 0)
                {
                    wallMesh.indexFormat = s_WallVertices.Count > 65000 ? IndexFormat.UInt32 : IndexFormat.UInt16;
                    wallMesh.SetVertices(s_WallVertices);
                    wallMesh.SetTriangles(s_WallTriangles, 0);
                    wallMesh.SetUVs(0, s_WallUVs);
                    wallMesh.SetNormals(s_WallNormals);
                    wallMesh.SetColors(s_WallColors);
                    wallMesh.RecalculateBounds();
                }
            }
        }

        /// <summary>
        /// Determines directional orientation of a torch based on its neighboring solid blocks.
        /// Priority: Floor > Left Wall > Right Wall > Back Wall.
        /// </summary>
        public static TorchOrientation DetermineTorchOrientation(Chunk chunk, int x, int y)
        {
            BlockType below = GetNeighborBlock(chunk, x, y - 1);
            if (BlockRegistry.IsSolid(below))
            {
                return TorchOrientation.Floor;
            }

            BlockType left = GetNeighborBlock(chunk, x - 1, y);
            if (BlockRegistry.IsSolid(left))
            {
                return TorchOrientation.LeftWall;
            }

            BlockType right = GetNeighborBlock(chunk, x + 1, y);
            if (BlockRegistry.IsSolid(right))
            {
                return TorchOrientation.RightWall;
            }

            if (chunk.wallBlocks != null && chunk.wallBlocks[x, y] != BlockType.Air)
            {
                return TorchOrientation.BackWall;
            }

            return TorchOrientation.Floor;
        }

        /// <summary>
        /// Generates custom slender/angled 3D mesh geometry for a torch based on its orientation.
        /// </summary>
        private static void AddTorchMesh(
            Vector3 origin,
            TorchOrientation orientation,
            List<Vector3> vertices,
            List<int> triangles,
            List<Vector2> uvs,
            List<Vector3> normals,
            List<Color32> colors)
        {
            int torchTex = VoxelData.GetFaceTextureIndex(BlockType.Torch, BlockFace.Front);
            Rect uvRect = VoxelData.GetAtlasUVRect(torchTex);
            Color32 torchTint = new Color32(255, 248, 205, 255); // Warm glowing emission tint

            float x = origin.x;
            float y = origin.y;

            switch (orientation)
            {
                case TorchOrientation.Floor:
                {
                    // Slender vertical post standing on the floor
                    float x0 = x + 0.42f, x1 = x + 0.58f;
                    float y0 = y + 0.0f, y1 = y + 0.65f;
                    float z0 = 0.08f, z1 = 0.24f;

                    AddBoxGeometry(x0, x1, y0, y1, z0, z1, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    break;
                }

                case TorchOrientation.LeftWall:
                {
                    // Slender angled bracket attached to left wall, leaning right
                    float z0 = 0.08f, z1 = 0.24f;
                    Vector3 p0 = new Vector3(x + 0.02f, y + 0.18f, z0);
                    Vector3 p1 = new Vector3(x + 0.14f, y + 0.12f, z0);
                    Vector3 p2 = new Vector3(x + 0.38f, y + 0.68f, z0);
                    Vector3 p3 = new Vector3(x + 0.26f, y + 0.74f, z0);

                    Vector3 q0 = new Vector3(x + 0.02f, y + 0.18f, z1);
                    Vector3 q1 = new Vector3(x + 0.14f, y + 0.12f, z1);
                    Vector3 q2 = new Vector3(x + 0.38f, y + 0.68f, z1);
                    Vector3 q3 = new Vector3(x + 0.26f, y + 0.74f, z1);

                    // Front
                    AddCustomQuad(p0, p3, p2, p1, Vector3.back, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Back
                    AddCustomQuad(q1, q2, q3, q0, Vector3.forward, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Left / Top
                    AddCustomQuad(p0, q0, q3, p3, new Vector3(-0.8f, 0.6f, 0f).normalized, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Right / Bottom
                    AddCustomQuad(p1, p2, q2, q1, new Vector3(0.8f, -0.6f, 0f).normalized, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Top head cap
                    AddCustomQuad(p3, q3, q2, p2, Vector3.up, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    break;
                }

                case TorchOrientation.RightWall:
                {
                    // Slender angled bracket attached to right wall, leaning left
                    float z0 = 0.08f, z1 = 0.24f;
                    Vector3 p0 = new Vector3(x + 0.98f, y + 0.18f, z0);
                    Vector3 p1 = new Vector3(x + 0.86f, y + 0.12f, z0);
                    Vector3 p2 = new Vector3(x + 0.62f, y + 0.68f, z0);
                    Vector3 p3 = new Vector3(x + 0.74f, y + 0.74f, z0);

                    Vector3 q0 = new Vector3(x + 0.98f, y + 0.18f, z1);
                    Vector3 q1 = new Vector3(x + 0.86f, y + 0.12f, z1);
                    Vector3 q2 = new Vector3(x + 0.62f, y + 0.68f, z1);
                    Vector3 q3 = new Vector3(x + 0.74f, y + 0.74f, z1);

                    // Front
                    AddCustomQuad(p1, p2, p3, p0, Vector3.back, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Back
                    AddCustomQuad(q0, q3, q2, q1, Vector3.forward, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Right / Top
                    AddCustomQuad(p0, p3, q3, q0, new Vector3(0.8f, 0.6f, 0f).normalized, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Left / Bottom
                    AddCustomQuad(p1, q1, q2, p2, new Vector3(-0.8f, -0.6f, 0f).normalized, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    // Top head cap
                    AddCustomQuad(p2, q2, q3, p3, Vector3.up, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    break;
                }

                case TorchOrientation.BackWall:
                default:
                {
                    // Vertical stick mounted against background wall
                    float x0 = x + 0.42f, x1 = x + 0.58f;
                    float y0 = y + 0.18f, y1 = y + 0.83f;
                    float z0 = 0.15f, z1 = 0.31f;

                    AddBoxGeometry(x0, x1, y0, y1, z0, z1, uvRect, torchTint, vertices, triangles, uvs, normals, colors);
                    break;
                }
            }
        }

        private static void AddBoxGeometry(
            float x0, float x1, float y0, float y1, float z0, float z1,
            Rect uvRect, Color32 tint,
            List<Vector3> vertices, List<int> triangles,
            List<Vector2> uvs, List<Vector3> normals, List<Color32> colors)
        {
            // Front (-Z)
            AddCustomQuad(new Vector3(x0, y0, z0), new Vector3(x0, y1, z0), new Vector3(x1, y1, z0), new Vector3(x1, y0, z0),
                Vector3.back, uvRect, tint, vertices, triangles, uvs, normals, colors);
            // Back (+Z)
            AddCustomQuad(new Vector3(x1, y0, z1), new Vector3(x1, y1, z1), new Vector3(x0, y1, z1), new Vector3(x0, y0, z1),
                Vector3.forward, uvRect, tint, vertices, triangles, uvs, normals, colors);
            // Top (+Y)
            AddCustomQuad(new Vector3(x0, y1, z0), new Vector3(x0, y1, z1), new Vector3(x1, y1, z1), new Vector3(x1, y1, z0),
                Vector3.up, uvRect, tint, vertices, triangles, uvs, normals, colors);
            // Left (-X)
            AddCustomQuad(new Vector3(x0, y0, z1), new Vector3(x0, y1, z1), new Vector3(x0, y1, z0), new Vector3(x0, y0, z0),
                Vector3.left, uvRect, tint, vertices, triangles, uvs, normals, colors);
            // Right (+X)
            AddCustomQuad(new Vector3(x1, y0, z0), new Vector3(x1, y1, z0), new Vector3(x1, y1, z1), new Vector3(x1, y0, z1),
                Vector3.right, uvRect, tint, vertices, triangles, uvs, normals, colors);
        }

        private static void AddCustomQuad(
            Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3,
            Vector3 normal, Rect uvRect, Color32 tint,
            List<Vector3> vertices, List<int> triangles,
            List<Vector2> uvs, List<Vector3> normals, List<Color32> colors)
        {
            int startIdx = vertices.Count;
            vertices.Add(p0);
            vertices.Add(p1);
            vertices.Add(p2);
            vertices.Add(p3);

            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);

            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);

            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMin));
            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMin));

            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 1);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 3);
        }

        /// <summary>
        /// Face culling condition: side bevels are only exposed when the adjacent neighbor
        /// is air or a transparent block.
        /// </summary>
        private static bool ShouldRenderSideFace(BlockType current, BlockType neighbor)
        {
            if (neighbor == BlockType.Air) return true;

            // Do not render inner faces between identical transparent blocks (e.g. adjacent glass)
            if (current == neighbor && BlockRegistry.IsTransparent(current))
            {
                return false;
            }

            return BlockRegistry.IsTransparent(neighbor);
        }

        /// <summary>
        /// Fast neighbor block retrieval. Directly accesses chunk memory if inside chunk;
        /// delegates to world callback if crossing chunk horizontal borders.
        /// </summary>
        private static BlockType GetNeighborBlock(Chunk chunk, int localX, int y)
        {
            if (y < 0 || y >= WorldSettings.ChunkHeight)
            {
                return BlockType.Air;
            }

            if (localX >= 0 && localX < WorldSettings.ChunkWidth)
            {
                return chunk.blocks[localX, y];
            }

            // Cross-chunk query
            if (OnQueryNeighborBlock != null)
            {
                int worldX = chunk.ChunkX * WorldSettings.ChunkWidth + localX;
                return OnQueryNeighborBlock(worldX, y);
            }

            return BlockType.Air;
        }

        /// <summary>
        /// Appends a quad face to the active vertex buffers.
        /// </summary>
        private static void AddFace(
            Vector3 origin,
            BlockFace face,
            int textureIndex,
            List<Vector3> vertices,
            List<int> triangles,
            List<Vector2> uvs,
            List<Vector3> normals,
            List<Color32> colors)
        {
            int startIdx = vertices.Count;
            Vector3 normal = VoxelData.FaceNormals[(int)face];
            Color32 tint = VoxelData.GetFaceTint(face);
            Rect uvRect = VoxelData.GetAtlasUVRect(textureIndex);

            // Vertices
            vertices.Add(origin + VoxelData.GetFaceVertex(face, 0));
            vertices.Add(origin + VoxelData.GetFaceVertex(face, 1));
            vertices.Add(origin + VoxelData.GetFaceVertex(face, 2));
            vertices.Add(origin + VoxelData.GetFaceVertex(face, 3));

            // Normals
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);

            // Colors
            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);

            // UVs
            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMin));
            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMin));

            // Triangles (two triangles per quad)
            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 1);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 3);
        }

        /// <summary>
        /// Appends a physical collider quad face for solid blocks.
        /// </summary>
        private static void AddColliderFace(
            Vector3 origin,
            BlockFace face,
            List<Vector3> colVertices,
            List<int> colTriangles)
        {
            int startIdx = colVertices.Count;
            colVertices.Add(origin + VoxelData.GetFaceVertex(face, 0));
            colVertices.Add(origin + VoxelData.GetFaceVertex(face, 1));
            colVertices.Add(origin + VoxelData.GetFaceVertex(face, 2));
            colVertices.Add(origin + VoxelData.GetFaceVertex(face, 3));

            colTriangles.Add(startIdx + 0);
            colTriangles.Add(startIdx + 1);
            colTriangles.Add(startIdx + 2);
            colTriangles.Add(startIdx + 0);
            colTriangles.Add(startIdx + 2);
            colTriangles.Add(startIdx + 3);
        }

        /// <summary>
        /// Appends a background wall quad at Z = WallZ.
        /// </summary>
        private static void AddWallQuad(
            Vector3 origin,
            int textureIndex,
            List<Vector3> vertices,
            List<int> triangles,
            List<Vector2> uvs,
            List<Vector3> normals,
            List<Color32> colors)
        {
            int startIdx = vertices.Count;
            Vector3 normal = Vector3.back;
            Color32 tint = VoxelData.ColorWall;
            Rect uvRect = VoxelData.GetAtlasUVRect(textureIndex);

            float wallZ = VoxelData.WallZ;
            vertices.Add(origin + new Vector3(0f, 0f, wallZ));
            vertices.Add(origin + new Vector3(0f, 1f, wallZ));
            vertices.Add(origin + new Vector3(1f, 1f, wallZ));
            vertices.Add(origin + new Vector3(1f, 0f, wallZ));

            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);

            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);
            colors.Add(tint);

            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMin));
            uvs.Add(new Vector2(uvRect.xMin, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMax));
            uvs.Add(new Vector2(uvRect.xMax, uvRect.yMin));

            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 1);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 0);
            triangles.Add(startIdx + 2);
            triangles.Add(startIdx + 3);
        }
    }
}
