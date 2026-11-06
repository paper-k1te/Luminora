using System;
using System.Collections.Generic;
using Core;
using UnityEngine;
using UnityEngine.Rendering;

namespace World
{
    /// <summary>
    /// High-performance 2.5D voxel mesher with directional face culling,
    /// 3D bevel extrusion, ambient occlusion tinting, and optimized mesh collider updates.
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

        // Reusable buffers for background wall mesh
        private static readonly List<Vector3> s_WallVertices = new List<Vector3>(4096);
        private static readonly List<int> s_WallTriangles = new List<int>(6144);
        private static readonly List<Vector2> s_WallUVs = new List<Vector2>(4096);
        private static readonly List<Vector3> s_WallNormals = new List<Vector3>(4096);
        private static readonly List<Color32> s_WallColors = new List<Color32>(4096);

        /// <summary>
        /// Generates foreground 2.5D extruded voxel mesh and background wall quad mesh for a chunk.
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

            s_WallVertices.Clear();
            s_WallTriangles.Clear();
            s_WallUVs.Clear();
            s_WallNormals.Clear();
            s_WallColors.Clear();

            // -------------------------------------------------------------
            // Pass 1: Build Foreground 2.5D Extruded Blocks
            // -------------------------------------------------------------
            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    BlockType block = chunk.blocks[x, y];
                    if (block == BlockType.Air) continue;

                    Vector3 origin = new Vector3(x, y, 0f);

                    // 1. Front face: facing -Z towards camera (always rendered for active blocks)
                    int frontTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Front);
                    AddFace(origin, BlockFace.Front, frontTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);

                    // 2. Top face (+Y)
                    BlockType topNeighbor = GetNeighborBlock(chunk, x, y + 1);
                    if (ShouldRenderSideFace(block, topNeighbor))
                    {
                        int topTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Top);
                        AddFace(origin, BlockFace.Top, topTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                    }

                    // 3. Bottom face (-Y)
                    BlockType bottomNeighbor = GetNeighborBlock(chunk, x, y - 1);
                    if (ShouldRenderSideFace(block, bottomNeighbor))
                    {
                        int bottomTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Bottom);
                        AddFace(origin, BlockFace.Bottom, bottomTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                    }

                    // 4. Left face (-X)
                    BlockType leftNeighbor = GetNeighborBlock(chunk, x - 1, y);
                    if (ShouldRenderSideFace(block, leftNeighbor))
                    {
                        int leftTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Left);
                        AddFace(origin, BlockFace.Left, leftTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
                    }

                    // 5. Right face (+X)
                    BlockType rightNeighbor = GetNeighborBlock(chunk, x + 1, y);
                    if (ShouldRenderSideFace(block, rightNeighbor))
                    {
                        int rightTex = VoxelData.GetFaceTextureIndex(block, BlockFace.Right);
                        AddFace(origin, BlockFace.Right, rightTex, s_Vertices, s_Triangles, s_UVs, s_Normals, s_Colors);
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
                        if (fg != BlockType.Air && !BlockRegistry.IsTransparent(fg))
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
            // Apply Foreground Mesh & Collider
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

                // Update physical collider
                if (chunk.meshCollider != null)
                {
                    chunk.meshCollider.sharedMesh = null;
                    if (s_Vertices.Count > 0)
                    {
                        chunk.meshCollider.sharedMesh = mesh;
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
