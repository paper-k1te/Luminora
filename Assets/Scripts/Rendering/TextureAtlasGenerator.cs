using System;
using System.IO;
using Core;
using UnityEngine;

namespace Rendering
{
    /// <summary>
    /// Autonomous procedural pixel-art texture atlas and emission map generator.
    /// Produces exquisite, high-detail 16x16 pixel-art textures for all voxel blocks
    /// and multi-face variations (grass tops, tree rings, cactus caps, faceted gem ores with star glints),
    /// complete with specular glints, organic dirt pebbles, stone bevels, and glowing emission maps.
    /// Includes fail-safe shader fallbacks that NEVER render pink or missing textures.
    /// </summary>
    public static class TextureAtlasGenerator
    {
        public const int TileResolution = 16;
        public const int AtlasTilesAcross = 16;
        public const int AtlasTilesDown = 16;
        public const int AtlasResolution = TileResolution * AtlasTilesAcross; // 256 x 256

        // Special Multi-Face Atlas Indices
        public const int GrassTopTileIndex = 27;
        public const int WoodTopTileIndex = 28;
        public const int CactusTopTileIndex = 29;

        private static Texture2D s_AtlasTexture;
        private static Texture2D s_EmissionTexture;
        private static Material s_AtlasMaterial;
        private static Material s_WallMaterial;

        /// <summary>
        /// Retrieves the primary voxel material, procedurally generating it and its atlas textures
        /// if they do not yet exist, with full fallback shader protection.
        /// </summary>
        public static Material GetOrCreateAtlasMaterial()
        {
            if (s_AtlasMaterial != null && s_AtlasTexture != null)
            {
                return s_AtlasMaterial;
            }

            // Attempt to load pre-baked atlas from Resources, or generate procedurally
            if (s_AtlasTexture == null)
            {
                s_AtlasTexture = Resources.Load<Texture2D>("VoxelAtlas");
                s_EmissionTexture = Resources.Load<Texture2D>("VoxelEmission");

                if (s_AtlasTexture != null)
                {
                    s_AtlasTexture.filterMode = FilterMode.Point;
                    s_AtlasTexture.wrapMode = TextureWrapMode.Clamp;
                }
                if (s_EmissionTexture != null)
                {
                    s_EmissionTexture.filterMode = FilterMode.Point;
                    s_EmissionTexture.wrapMode = TextureWrapMode.Clamp;
                }
            }

            if (s_AtlasTexture == null || s_EmissionTexture == null)
            {
                GenerateAtlasTextures(out s_AtlasTexture, out s_EmissionTexture);
                SaveAtlasTexturesToDisk(s_AtlasTexture, s_EmissionTexture);
            }

            // Robust multi-pipeline shader resolution
            Shader shader = FindVoxelShader();
            s_AtlasMaterial = new Material(shader)
            {
                name = "VoxelAtlas_Material",
                mainTexture = s_AtlasTexture
            };

            // Set common properties across Standard, Custom, URP, and Legacy shaders
            if (s_AtlasMaterial.HasProperty("_MainTex"))
            {
                s_AtlasMaterial.SetTexture("_MainTex", s_AtlasTexture);
            }
            if (s_AtlasMaterial.HasProperty("_BaseMap"))
            {
                s_AtlasMaterial.SetTexture("_BaseMap", s_AtlasTexture);
            }
            if (s_AtlasMaterial.HasProperty("_Color"))
            {
                s_AtlasMaterial.SetColor("_Color", Color.white);
            }
            if (s_AtlasMaterial.HasProperty("_BaseColor"))
            {
                s_AtlasMaterial.SetColor("_BaseColor", Color.white);
            }

            // Standard Shader matte pixel-art properties
            if (s_AtlasMaterial.HasProperty("_Glossiness"))
            {
                s_AtlasMaterial.SetFloat("_Glossiness", 0.05f);
            }
            if (s_AtlasMaterial.HasProperty("_Metallic"))
            {
                s_AtlasMaterial.SetFloat("_Metallic", 0.0f);
            }

            // Emissive glow setup for torches, magma, glowshrooms, and gems
            if (s_EmissionTexture != null)
            {
                if (s_AtlasMaterial.HasProperty("_EmissionMap"))
                {
                    s_AtlasMaterial.SetTexture("_EmissionMap", s_EmissionTexture);
                    s_AtlasMaterial.SetColor("_EmissionColor", Color.white);
                    s_AtlasMaterial.EnableKeyword("_EMISSION");
                }
            }

            // Alpha cutout setup for torches, foliage, and glass
            if (s_AtlasMaterial.HasProperty("_Cutoff"))
            {
                s_AtlasMaterial.SetFloat("_Cutoff", 0.35f);
                s_AtlasMaterial.EnableKeyword("_ALPHATEST_ON");
                s_AtlasMaterial.renderQueue = 2450;
            }

            return s_AtlasMaterial;
        }

        /// <summary>
        /// Finds the best available shader with guaranteed fallbacks to prevent magenta/pink rendering.
        /// </summary>
        private static Shader FindVoxelShader()
        {
            string[] shaderCandidates = new string[]
            {
                "Custom/VoxelBlock",
                "Standard",
                "Universal Render Pipeline/Lit",
                "Universal Render Pipeline/Simple Lit",
                "Mobile/Diffuse",
                "Legacy Shaders/Diffuse",
                "Diffuse",
                "Unlit/Transparent Cutout",
                "Unlit/Texture",
                "Sprites/Default",
                "UI/Default"
            };

            foreach (string candidate in shaderCandidates)
            {
                Shader s = Shader.Find(candidate);
                if (s != null && s.isSupported)
                {
                    return s;
                }
            }

            // Absolute fallback
            return Shader.Find("Sprites/Default") ?? Shader.Find("Unlit/Texture");
        }

        /// <summary>
        /// Retrieves or creates a dedicated background wall material with ambient contrast shading.
        /// </summary>
        public static Material GetOrCreateWallMaterial()
        {
            if (s_WallMaterial != null) return s_WallMaterial;

            Material baseMat = GetOrCreateAtlasMaterial();
            s_WallMaterial = new Material(baseMat)
            {
                name = "VoxelWall_Material",
                color = new Color(0.60f, 0.60f, 0.60f, 1.0f) // Ambient back-wall contrast
            };

            return s_WallMaterial;
        }

        /// <summary>
        /// Saves generated atlas textures as PNG files into Assets/Resources.
        /// </summary>
        public static void SaveAtlasTexturesToDisk(Texture2D diffuse, Texture2D emission)
        {
            try
            {
                string resourcesPath = Path.Combine(Application.dataPath, "Resources");
                if (!Directory.Exists(resourcesPath))
                {
                    Directory.CreateDirectory(resourcesPath);
                }

                if (diffuse != null)
                {
                    byte[] diffuseBytes = diffuse.EncodeToPNG();
                    File.WriteAllBytes(Path.Combine(resourcesPath, "VoxelAtlas.png"), diffuseBytes);
                }

                if (emission != null)
                {
                    byte[] emissionBytes = emission.EncodeToPNG();
                    File.WriteAllBytes(Path.Combine(resourcesPath, "VoxelEmission.png"), emissionBytes);
                }
            }
            catch (Exception ex)
            {
                Debug.LogWarning("[TextureAtlasGenerator] Disk save skipped: " + ex.Message);
            }
        }

        /// <summary>
        /// Generates both the main diffuse atlas texture and the corresponding emission mask.
        /// </summary>
        public static void GenerateAtlasTextures(out Texture2D diffuseAtlas, out Texture2D emissionAtlas)
        {
            diffuseAtlas = new Texture2D(AtlasResolution, AtlasResolution, TextureFormat.RGBA32, false)
            {
                name = "ProceduralVoxelAtlas",
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };

            emissionAtlas = new Texture2D(AtlasResolution, AtlasResolution, TextureFormat.RGBA32, false)
            {
                name = "ProceduralVoxelEmission",
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };

            // Initialize backgrounds
            Color32[] clearPixels = new Color32[AtlasResolution * AtlasResolution];
            Color32[] blackPixels = new Color32[AtlasResolution * AtlasResolution];
            for (int i = 0; i < clearPixels.Length; i++)
            {
                clearPixels[i] = new Color32(0, 0, 0, 0);
                blackPixels[i] = new Color32(0, 0, 0, 255);
            }
            diffuseAtlas.SetPixels32(clearPixels);
            emissionAtlas.SetPixels32(blackPixels);

            // Paint all 27 standard block types
            for (int id = 0; id < BlockRegistry.Count; id++)
            {
                BlockType type = (BlockType)id;
                int texIndex = VoxelData.GetFaceTextureIndex(type, BlockFace.Front);

                Color32[] tileBuffer = new Color32[TileResolution * TileResolution];
                Color32[] emitBuffer = new Color32[TileResolution * TileResolution];
                for (int i = 0; i < emitBuffer.Length; i++) emitBuffer[i] = new Color32(0, 0, 0, 255);

                PaintBlockTile(type, tileBuffer, emitBuffer);
                WriteTileToAtlas(diffuseAtlas, emissionAtlas, texIndex, tileBuffer, emitBuffer);
            }

            // Paint multi-face variants
            Color32[] variantTile = new Color32[TileResolution * TileResolution];
            Color32[] variantEmit = new Color32[TileResolution * TileResolution];
            for (int i = 0; i < variantEmit.Length; i++) variantEmit[i] = new Color32(0, 0, 0, 255);

            // 1. Grass Top Carpet
            PaintGrassTop(variantTile);
            WriteTileToAtlas(diffuseAtlas, emissionAtlas, GrassTopTileIndex, variantTile, variantEmit);

            // 2. Wood Log Annual Rings
            PaintWoodTop(variantTile);
            WriteTileToAtlas(diffuseAtlas, emissionAtlas, WoodTopTileIndex, variantTile, variantEmit);

            // 3. Cactus Crown
            PaintCactusTop(variantTile);
            WriteTileToAtlas(diffuseAtlas, emissionAtlas, CactusTopTileIndex, variantTile, variantEmit);

            diffuseAtlas.Apply(false);
            emissionAtlas.Apply(false);
        }

        private static void WriteTileToAtlas(Texture2D diffuseAtlas, Texture2D emissionAtlas, int tileIndex, Color32[] tile, Color32[] emit)
        {
            int tileX = tileIndex % AtlasTilesAcross;
            int tileY = (AtlasTilesDown - 1) - (tileIndex / AtlasTilesAcross);
            int startX = tileX * TileResolution;
            int startY = tileY * TileResolution;

            diffuseAtlas.SetPixels32(startX, startY, TileResolution, TileResolution, tile);
            emissionAtlas.SetPixels32(startX, startY, TileResolution, TileResolution, emit);
        }

        // =====================================================================
        // Procedural Pixel Art Painting Engine
        // =====================================================================

        private static void PaintBlockTile(BlockType type, Color32[] tile, Color32[] emit)
        {
            switch (type)
            {
                case BlockType.Air:
                    // Transparent
                    break;

                case BlockType.Grass:
                    PaintGrass(tile);
                    break;

                case BlockType.Dirt:
                    PaintDirt(tile);
                    break;

                case BlockType.Stone:
                    PaintStone(tile);
                    break;

                case BlockType.Wood:
                    PaintWood(tile);
                    break;

                case BlockType.Leaves:
                    PaintLeaves(tile);
                    break;

                case BlockType.Sand:
                    PaintSand(tile);
                    break;

                case BlockType.Sandstone:
                    PaintSandstone(tile);
                    break;

                case BlockType.Cactus:
                    PaintCactus(tile);
                    break;

                case BlockType.Snow:
                    PaintSnow(tile);
                    break;

                case BlockType.Ice:
                    PaintIce(tile);
                    break;

                case BlockType.Clay:
                    PaintClay(tile);
                    break;

                case BlockType.CoalOre:
                    PaintOre(tile, emit, new Color32(24, 25, 28, 255), new Color32(49, 52, 59, 255), new Color32(86, 91, 99, 255), false);
                    break;

                case BlockType.IronOre:
                    PaintOre(tile, emit, new Color32(110, 61, 29, 255), new Color32(170, 104, 56, 255), new Color32(212, 149, 99, 255), false);
                    break;

                case BlockType.GoldOre:
                    PaintOre(tile, emit, new Color32(158, 107, 5, 255), new Color32(240, 184, 12, 255), new Color32(255, 222, 67, 255), false);
                    break;

                case BlockType.DiamondOre:
                    PaintOre(tile, emit, new Color32(0, 119, 138, 255), new Color32(0, 212, 235, 255), new Color32(122, 233, 247, 255), true);
                    break;

                case BlockType.RubyOre:
                    PaintOre(tile, emit, new Color32(89, 2, 18, 255), new Color32(196, 8, 40, 255), new Color32(255, 43, 79, 255), true);
                    break;

                case BlockType.Brick:
                    PaintBrick(tile);
                    break;

                case BlockType.WoodPlank:
                    PaintWoodPlank(tile);
                    break;

                case BlockType.Glass:
                    PaintGlass(tile);
                    break;

                case BlockType.Torch:
                    PaintTorch(tile, emit);
                    break;

                case BlockType.Obsidian:
                    PaintObsidian(tile);
                    break;

                case BlockType.Magma:
                    PaintMagma(tile, emit);
                    break;

                case BlockType.MossyStone:
                    PaintMossyStone(tile);
                    break;

                case BlockType.Mud:
                    PaintMud(tile);
                    break;

                case BlockType.GlowShroom:
                    PaintGlowShroom(tile, emit);
                    break;

                case BlockType.Bedrock:
                    PaintBedrock(tile);
                    break;

                default:
                    FillSolid(tile, new Color32(180, 180, 180, 255));
                    break;
            }
        }

        private static void SetPixel(Color32[] buffer, int x, int y, Color32 color)
        {
            if (x < 0 || x >= TileResolution || y < 0 || y >= TileResolution) return;
            buffer[y * TileResolution + x] = color;
        }

        private static void FillSolid(Color32[] buffer, Color32 color)
        {
            for (int i = 0; i < buffer.Length; i++)
            {
                buffer[i] = color;
            }
        }

        private static uint Hash2D(int x, int y, int seed)
        {
            uint n = (uint)(x * 374761393 + y * 668265263 + seed * 91234567);
            n = (n ^ (n >> 13)) * 1274126177;
            return n ^ (n >> 16);
        }

        // -------------------------------------------------------------
        // Pixel Art Generators
        // -------------------------------------------------------------

        private static void PaintDirt(Color32[] tile)
        {
            Color32 dDark = new Color32(61, 36, 20, 255);
            Color32 dBase = new Color32(89, 56, 34, 255);
            Color32 dMid = new Color32(110, 70, 42, 255);
            Color32 dLight = new Color32(133, 87, 52, 255);
            Color32 pebDark = new Color32(66, 62, 59, 255);
            Color32 pebMid = new Color32(122, 117, 111, 255);
            Color32 pebHigh = new Color32(166, 160, 153, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 101) % 100;
                    Color32 c = h < 25 ? dDark : (h < 60 ? dBase : (h < 85 ? dMid : dLight));
                    SetPixel(tile, x, y, c);
                }
            }

            // Embedded river pebbles
            SetPixel(tile, 3, 4, pebDark); SetPixel(tile, 4, 4, pebMid);  SetPixel(tile, 5, 4, pebDark);
            SetPixel(tile, 3, 5, pebMid);  SetPixel(tile, 4, 5, pebHigh); SetPixel(tile, 5, 5, pebMid);

            SetPixel(tile, 10, 10, pebDark); SetPixel(tile, 11, 10, pebMid);  SetPixel(tile, 12, 10, pebDark);
            SetPixel(tile, 10, 11, pebMid);  SetPixel(tile, 11, 11, pebHigh); SetPixel(tile, 12, 11, pebMid);

            SetPixel(tile, 7, 1, pebDark); SetPixel(tile, 8, 1, pebDark);
            SetPixel(tile, 7, 2, pebMid);  SetPixel(tile, 8, 2, pebHigh);
        }

        private static void PaintGrass(Color32[] tile)
        {
            PaintDirt(tile);

            Color32 gTip = new Color32(167, 244, 50, 255);
            Color32 gLight = new Color32(110, 204, 38, 255);
            Color32 gMid = new Color32(70, 158, 28, 255);
            Color32 gDark = new Color32(38, 97, 16, 255);
            Color32 gShade = new Color32(46, 28, 16, 255);

            int[] bladeDepths = new int[] { 12, 10, 12, 11, 9, 11, 12, 10, 12, 11, 9, 11, 10, 12, 11, 12 };

            for (int x = 0; x < TileResolution; x++)
            {
                int d = bladeDepths[x];
                if (d > 0)
                {
                    SetPixel(tile, x, d - 1, gShade);
                }
                for (int y = d; y < TileResolution; y++)
                {
                    Color32 c;
                    if (y == 15) c = (x % 3 == 0) ? gTip : gLight;
                    else if (y >= 13) c = ((x + y) % 2 == 0) ? gLight : gMid;
                    else if (y >= 11) c = ((x ^ y) % 2 == 0) ? gMid : gDark;
                    else c = gDark;

                    SetPixel(tile, x, y, c);
                }
            }
        }

        private static void PaintGrassTop(Color32[] tile)
        {
            Color32 g1 = new Color32(46, 125, 20, 255);
            Color32 g2 = new Color32(65, 156, 28, 255);
            Color32 g3 = new Color32(92, 184, 46, 255);
            Color32 g4 = new Color32(130, 214, 56, 255);
            Color32 flowerY = new Color32(255, 235, 59, 255);
            Color32 flowerW = new Color32(255, 255, 255, 255);
            Color32 flowerC = new Color32(255, 152, 0, 255);
            Color32 clover = new Color32(160, 240, 60, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 202) % 100;
                    Color32 c = h < 20 ? g1 : (h < 60 ? g2 : (h < 85 ? g3 : g4));
                    SetPixel(tile, x, y, c);
                }
            }

            // Clover patch
            SetPixel(tile, 4, 5, clover);
            SetPixel(tile, 5, 5, clover);
            SetPixel(tile, 4, 6, clover);

            // Buttercup
            SetPixel(tile, 10, 11, flowerC);
            SetPixel(tile, 9, 11, flowerY);
            SetPixel(tile, 11, 11, flowerY);
            SetPixel(tile, 10, 12, flowerY);
            SetPixel(tile, 10, 10, flowerY);

            // Daisy
            SetPixel(tile, 13, 3, flowerC);
            SetPixel(tile, 12, 3, flowerW);
            SetPixel(tile, 14, 3, flowerW);
            SetPixel(tile, 13, 4, flowerW);
            SetPixel(tile, 13, 2, flowerW);
        }

        private static void PaintStone(Color32[] tile)
        {
            Color32 grout = new Color32(38, 41, 46, 255);
            Color32 base1 = new Color32(95, 100, 107, 255);
            Color32 base2 = new Color32(115, 121, 129, 255);
            Color32 base3 = new Color32(135, 142, 151, 255);
            Color32 bevelHigh = new Color32(180, 188, 198, 255);
            Color32 bevelDark = new Color32(60, 64, 70, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 303) % 100;
                    Color32 c = h < 35 ? base1 : (h < 75 ? base2 : base3);
                    SetPixel(tile, x, y, c);
                }
            }

            // Horizontal seam at y = 8
            for (int x = 0; x < TileResolution; x++)
            {
                SetPixel(tile, x, 8, grout);
                SetPixel(tile, x, 9, bevelHigh);
                SetPixel(tile, x, 7, bevelDark);
            }

            // Upper vertical seam at x = 8, y = 9..15
            for (int y = 9; y < TileResolution; y++)
            {
                SetPixel(tile, 8, y, grout);
                SetPixel(tile, 9, y, bevelHigh);
                SetPixel(tile, 7, y, bevelDark);
            }

            // Lower vertical seam at x = 12, y = 0..7
            for (int y = 0; y < 8; y++)
            {
                SetPixel(tile, 12, y, grout);
                SetPixel(tile, 13, y, bevelHigh);
                SetPixel(tile, 11, y, bevelDark);
            }

            // Micro-fracture in lower stone
            SetPixel(tile, 2, 2, grout); SetPixel(tile, 2, 3, bevelHigh);
            SetPixel(tile, 3, 3, grout); SetPixel(tile, 3, 4, bevelHigh);
            SetPixel(tile, 4, 4, grout); SetPixel(tile, 4, 5, bevelHigh);
            SetPixel(tile, 5, 4, grout);
            SetPixel(tile, 6, 5, grout); SetPixel(tile, 6, 6, bevelHigh);
        }

        private static void PaintWood(Color32[] tile)
        {
            Color32 barkDarkest = new Color32(32, 19, 11, 255);
            Color32 barkFurrow = new Color32(49, 30, 19, 255);
            Color32 barkMid = new Color32(94, 59, 35, 255);
            Color32 barkLight = new Color32(138, 87, 52, 255);
            Color32 barkHigh = new Color32(168, 109, 67, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                int f1 = 3 + ((y >= 4 && y <= 10) ? 1 : 0);
                int f2 = 8 + ((y >= 11 || y <= 3) ? 1 : 0);
                int f3 = 13 + ((y >= 6 && y <= 12) ? 1 : 0);

                for (int x = 0; x < TileResolution; x++)
                {
                    Color32 c;
                    if (x == f1 || x == f2 || x == f3)
                    {
                        c = (y % 2 == 0) ? barkDarkest : barkFurrow;
                    }
                    else if (x == f1 + 1 || x == f2 + 1 || x == f3 + 1)
                    {
                        c = (y % 3 == 0) ? barkHigh : barkLight;
                    }
                    else if (x == f1 - 1 || x == f2 - 1 || x == f3 - 1)
                    {
                        c = barkFurrow;
                    }
                    else
                    {
                        uint h = Hash2D(x, y, 404) % 100;
                        c = h < 60 ? barkMid : barkLight;
                    }
                    SetPixel(tile, x, y, c);
                }
            }

            // Bark knot whorl
            SetPixel(tile, 9, 6, barkFurrow); SetPixel(tile, 10, 6, barkDarkest); SetPixel(tile, 11, 6, barkFurrow);
            SetPixel(tile, 9, 7, barkLight);  SetPixel(tile, 10, 7, barkHigh);    SetPixel(tile, 11, 7, barkLight);
        }

        private static void PaintWoodTop(Color32[] tile)
        {
            Color32 barkRim = new Color32(49, 30, 19, 255);
            Color32 barkMid = new Color32(94, 59, 35, 255);
            Color32 sapLight = new Color32(215, 181, 135, 255);
            Color32 sapMid = new Color32(194, 158, 112, 255);
            Color32 ringDark = new Color32(153, 115, 72, 255);
            Color32 pithCore = new Color32(112, 75, 40, 255);
            Color32 checkLine = new Color32(32, 19, 11, 255);

            float cx = 7.5f, cy = 7.5f;
            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    Color32 c;
                    if (x == 0 || x == 15 || y == 0 || y == 15)
                    {
                        c = (x + y) % 2 == 0 ? barkRim : barkMid;
                    }
                    else
                    {
                        float dist = Mathf.Sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                        if (dist < 1.8f) c = pithCore;
                        else if (dist >= 3.0f && dist <= 3.8f) c = ringDark;
                        else if (dist >= 5.2f && dist <= 6.0f) c = ringDark;
                        else c = (x + y) % 2 == 0 ? sapLight : sapMid;
                    }
                    SetPixel(tile, x, y, c);
                }
            }

            // Radial check crack
            SetPixel(tile, 8, 8, checkLine);
            SetPixel(tile, 9, 9, checkLine);
            SetPixel(tile, 10, 10, checkLine);
            SetPixel(tile, 11, 11, checkLine);
            SetPixel(tile, 12, 12, checkLine);
        }

        private static void PaintLeaves(Color32[] tile)
        {
            Color32 lHigh = new Color32(141, 227, 68, 255);
            Color32 lMid = new Color32(78, 168, 44, 255);
            Color32 lDark = new Color32(39, 97, 24, 255);
            Color32 lDeep = new Color32(22, 61, 12, 255);
            Color32 transparent = new Color32(0, 0, 0, 0);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 505) % 100;
                    Color32 c = h < 25 ? lDeep : (h < 60 ? lDark : (h < 85 ? lMid : lHigh));
                    SetPixel(tile, x, y, c);
                }
            }

            // Transparent pixel cutouts for canopy foliage volume
            int[,] cutouts = new int[,]
            {
                {0,0}, {15,0}, {0,15}, {15,15},
                {1,0}, {0,1}, {14,15}, {15,14},
                {7,7}, {8,7}, {7,8},
                {3,11}, {4,11}, {12,4}, {13,4}
            };
            for (int i = 0; i < cutouts.GetLength(0); i++)
            {
                SetPixel(tile, cutouts[i, 0], cutouts[i, 1], transparent);
            }
        }

        private static void PaintSand(Color32[] tile)
        {
            Color32 sCrest = new Color32(255, 245, 165, 255);
            Color32 sLight = new Color32(245, 220, 125, 255);
            Color32 sMid = new Color32(225, 190, 95, 255);
            Color32 sTrough = new Color32(195, 155, 65, 255);
            Color32 sGlint = new Color32(255, 255, 220, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    float wave = Mathf.Sin(x * 0.45f + (y / 4) * 1.5f) * 1.2f;
                    float val = (y + wave) % 4.0f;
                    if (val < 0f) val += 4.0f;

                    Color32 c;
                    if (val < 0.8f) c = sCrest;
                    else if (val < 2.0f) c = sLight;
                    else if (val < 3.2f) c = sMid;
                    else c = sTrough;

                    if (Hash2D(x, y, 606) % 23 == 0) c = sGlint;
                    SetPixel(tile, x, y, c);
                }
            }
        }

        private static void PaintSandstone(Color32[] tile)
        {
            Color32 seam = new Color32(115, 62, 31, 255);
            Color32 b0Dark = new Color32(140, 82, 50, 255);
            Color32 b0Light = new Color32(165, 98, 62, 255);
            Color32 b1Dark = new Color32(205, 155, 80, 255);
            Color32 b1Light = new Color32(225, 175, 95, 255);
            Color32 b2Dark = new Color32(230, 210, 155, 255);
            Color32 b2Light = new Color32(245, 228, 180, 255);
            Color32 b3Dark = new Color32(186, 117, 76, 255);
            Color32 b3Light = new Color32(206, 133, 90, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                if (y == 3 || y == 7 || y == 11 || y == 15)
                {
                    for (int x = 0; x < TileResolution; x++) SetPixel(tile, x, y, seam);
                    continue;
                }

                Color32 cd = (y < 3) ? b0Dark : (y < 7 ? b1Dark : (y < 11 ? b2Dark : b3Dark));
                Color32 cl = (y < 3) ? b0Light : (y < 7 ? b1Light : (y < 11 ? b2Light : b3Light));

                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 707) % 100;
                    SetPixel(tile, x, y, h < 55 ? cd : cl);
                }
            }
        }

        private static void PaintCactus(Color32[] tile)
        {
            Color32 cFurrow = new Color32(27, 88, 18, 255);
            Color32 cFlank = new Color32(50, 135, 35, 255);
            Color32 cCrest = new Color32(82, 192, 58, 255);
            Color32 cSpine = new Color32(255, 255, 230, 255);
            Color32 cSpbase = new Color32(255, 238, 88, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    int col = x % 4;
                    Color32 c = (col == 0 || col == 3) ? cFurrow : (col == 1 ? cCrest : cFlank);
                    SetPixel(tile, x, y, c);

                    if (col == 1 && (y % 4 == 0))
                    {
                        SetPixel(tile, x, y, cSpbase);
                        if (x > 0) SetPixel(tile, x - 1, y, cSpine);
                    }
                    else if (col == 3 && (y % 4 == 2))
                    {
                        SetPixel(tile, x, y, cSpbase);
                        if (x + 1 < TileResolution) SetPixel(tile, x + 1, y, cSpine);
                    }
                }
            }
        }

        private static void PaintCactusTop(Color32[] tile)
        {
            Color32 cFurrow = new Color32(27, 88, 18, 255);
            Color32 cFlank = new Color32(50, 135, 35, 255);
            Color32 cCrest = new Color32(82, 192, 58, 255);
            Color32 cCenter = new Color32(255, 238, 88, 255);
            Color32 cSpine = new Color32(255, 255, 230, 255);

            float cx = 7.5f, cy = 7.5f;
            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    float dx = x - cx;
                    float dy = y - cy;
                    float angle = Mathf.Atan2(dy, dx);
                    float dist = Mathf.Sqrt(dx * dx + dy * dy);

                    Color32 c;
                    if (dist < 2.0f)
                    {
                        c = cCenter;
                    }
                    else
                    {
                        float normAngle = (angle + Mathf.PI) / (Mathf.PI / 4.0f);
                        int rib = Mathf.RoundToInt(normAngle) % 8;
                        float diff = Mathf.Abs(normAngle - rib);

                        if (diff < 0.15f) c = cCrest;
                        else if (diff < 0.35f) c = cFlank;
                        else c = cFurrow;
                    }
                    SetPixel(tile, x, y, c);
                }
            }

            SetPixel(tile, 7, 7, cSpine); SetPixel(tile, 8, 7, cSpine);
            SetPixel(tile, 7, 8, cSpine); SetPixel(tile, 8, 8, cSpine);
        }

        private static void PaintSnow(Color32[] tile)
        {
            Color32 sPure = new Color32(255, 255, 255, 255);
            Color32 sLight = new Color32(240, 248, 255, 255);
            Color32 sShadow = new Color32(205, 228, 245, 255);
            Color32 sDeep = new Color32(168, 202, 226, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 808) % 100;
                    float wave = Mathf.Sin(x * 0.4f) * 1.5f + Mathf.Cos(y * 0.5f) * 1.0f;
                    Color32 c;
                    if (wave > 1.2f) c = h < 60 ? sPure : sLight;
                    else if (wave > -0.5f) c = h < 70 ? sLight : sShadow;
                    else c = h < 60 ? sShadow : sDeep;

                    SetPixel(tile, x, y, c);
                }
            }
        }

        private static void PaintIce(Color32[] tile)
        {
            Color32 iceDeep = new Color32(38, 140, 166, 220);
            Color32 iceMid = new Color32(68, 176, 204, 230);
            Color32 iceLight = new Color32(120, 215, 235, 240);
            Color32 iceCrack = new Color32(255, 255, 255, 255);
            Color32 iceHalo = new Color32(210, 250, 255, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 909) % 100;
                    Color32 c = h < 30 ? iceDeep : (h < 75 ? iceMid : iceLight);
                    SetPixel(tile, x, y, c);
                }
            }

            int[,] cracks = new int[,]
            {
                {2,2}, {3,3}, {4,4}, {5,6}, {6,7}, {7,8}, {8,9},
                {9,10}, {10,12}, {11,13}, {12,14}, {13,14},
                {8,9}, {9,8}, {10,7}, {11,7}, {12,6}
            };
            for (int i = 0; i < cracks.GetLength(0); i++)
            {
                int cx = cracks[i, 0];
                int cy = cracks[i, 1];
                SetPixel(tile, cx, cy, iceCrack);
                if (cx + 1 < TileResolution) SetPixel(tile, cx + 1, cy, iceHalo);
            }
        }

        private static void PaintClay(Color32[] tile)
        {
            Color32 c1 = new Color32(125, 104, 96, 255);
            Color32 c2 = new Color32(145, 124, 116, 255);
            Color32 c3 = new Color32(168, 145, 136, 255);
            Color32 c4 = new Color32(188, 166, 156, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    int swirl = Mathf.Abs((int)(Mathf.Sin(x * 0.4f + y * 0.3f) * 2.5f + (x + y * 2) * 0.25f)) % 4;
                    Color32 c = swirl == 0 ? c1 : (swirl == 1 ? c2 : (swirl == 2 ? c3 : c4));
                    SetPixel(tile, x, y, c);
                }
            }
        }

        private static void PaintOre(Color32[] tile, Color32[] emit, Color32 gemDark, Color32 gemMid, Color32 gemLight, bool isEmissive)
        {
            PaintStone(tile);

            Color32 gemShadow = new Color32(20, 20, 20, 255);
            Color32 glint = new Color32(255, 255, 255, 255);

            int[,] clusters = new int[,]
            {
                // Cluster 1
                {3, 11, 0}, {4, 10, 0}, {5, 10, 0}, {6, 11, 0},
                {4, 11, 1}, {5, 11, 1}, {4, 12, 2}, {5, 12, 1},
                {4, 12, 3}, // 1x1 Specular star glint!
                // Cluster 2
                {9, 4, 0}, {10, 3, 0}, {11, 3, 0}, {12, 4, 0}, {13, 5, 0},
                {10, 4, 1}, {11, 4, 1}, {12, 5, 1}, {11, 5, 2},
                {11, 5, 3}, // 1x1 Specular star glint!
                // Cluster 3
                {7, 6, 0}, {8, 6, 1}, {7, 7, 2}, {8, 7, 3}
            };

            for (int i = 0; i < clusters.GetLength(0); i++)
            {
                int gx = clusters[i, 0];
                int gy = clusters[i, 1];
                int type = clusters[i, 2];

                Color32 c = type == 0 ? gemShadow : (type == 1 ? gemMid : (type == 2 ? gemLight : glint));
                SetPixel(tile, gx, gy, c);

                if (isEmissive && type >= 1)
                {
                    SetPixel(emit, gx, gy, c);
                }
            }
        }

        private static void PaintBrick(Color32[] tile)
        {
            Color32 mortarDark = new Color32(133, 138, 145, 255);
            Color32 mortarLight = new Color32(209, 213, 219, 255);
            Color32 bDark = new Color32(145, 39, 20, 255);
            Color32 bMid = new Color32(184, 58, 35, 255);
            Color32 bLight = new Color32(217, 90, 65, 255);
            Color32 bBevel = new Color32(240, 120, 95, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                bool isHMortar = (y % 4 == 0);
                int row = y / 4;
                int xShift = (row % 2 == 0) ? 0 : 4;

                for (int x = 0; x < TileResolution; x++)
                {
                    bool isVMortar = ((x + xShift) % 8 == 0);
                    if (isHMortar || isVMortar)
                    {
                        SetPixel(tile, x, y, (x + y) % 2 == 0 ? mortarDark : mortarLight);
                    }
                    else
                    {
                        int localY = y % 4;
                        int localX = (x + xShift) % 8;
                        if (localY == 3 || localX == 1)
                        {
                            SetPixel(tile, x, y, bBevel);
                        }
                        else
                        {
                            uint h = Hash2D(x, y, 1111) % 100;
                            Color32 c = h < 30 ? bDark : (h < 75 ? bMid : bLight);
                            SetPixel(tile, x, y, c);
                        }
                    }
                }
            }
        }

        private static void PaintWoodPlank(Color32[] tile)
        {
            Color32 groove = new Color32(46, 24, 11, 255);
            Color32 pDark = new Color32(140, 91, 48, 255);
            Color32 pMid = new Color32(168, 114, 62, 255);
            Color32 pLight = new Color32(194, 138, 82, 255);
            Color32 pBevel = new Color32(215, 160, 105, 255);
            Color32 nailDark = new Color32(48, 48, 48, 255);
            Color32 nailHigh = new Color32(160, 160, 160, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                bool isGroove = (y % 4 == 0);
                for (int x = 0; x < TileResolution; x++)
                {
                    if (isGroove)
                    {
                        SetPixel(tile, x, y, groove);
                    }
                    else
                    {
                        int localY = y % 4;
                        if (localY == 3)
                        {
                            SetPixel(tile, x, y, pBevel);
                        }
                        else
                        {
                            uint h = Hash2D(x, y, 1212) % 100;
                            Color32 c = h < 35 ? pDark : (h < 80 ? pMid : pLight);
                            SetPixel(tile, x, y, c);
                        }
                    }
                }
            }

            // Nails
            int[,] nails = new int[,] { {1,2}, {14,2}, {2,6}, {13,6}, {1,10}, {14,10}, {2,14}, {13,14} };
            for (int i = 0; i < nails.GetLength(0); i++)
            {
                int nx = nails[i, 0];
                int ny = nails[i, 1];
                SetPixel(tile, nx, ny, nailDark);
                SetPixel(tile, nx, ny + 1, nailHigh);
            }
        }

        private static void PaintGlass(Color32[] tile)
        {
            Color32 border = new Color32(75, 85, 99, 255);
            Color32 tint = new Color32(195, 235, 247, 55);
            Color32 streak1 = new Color32(255, 255, 255, 195);
            Color32 streak2 = new Color32(255, 255, 255, 135);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    if (x == 0 || x == 15 || y == 0 || y == 15)
                    {
                        SetPixel(tile, x, y, border);
                    }
                    else if ((x + y == 17) || (x + y == 18 && x >= 4 && x <= 12))
                    {
                        SetPixel(tile, x, y, streak1);
                    }
                    else if ((x + y == 9 && x >= 2 && x <= 6) || (x + y == 10 && x >= 3 && x <= 5))
                    {
                        SetPixel(tile, x, y, streak2);
                    }
                    else
                    {
                        SetPixel(tile, x, y, tint);
                    }
                }
            }
        }

        private static void PaintTorch(Color32[] tile, Color32[] emit)
        {
            Color32 transparent = new Color32(0, 0, 0, 0);
            FillSolid(tile, transparent);

            Color32 wDark = new Color32(54, 27, 12, 255);
            Color32 wCore = new Color32(117, 68, 33, 255);
            Color32 wLight = new Color32(148, 89, 45, 255);
            Color32 charred = new Color32(36, 32, 28, 255);

            Color32 fRed = new Color32(211, 47, 47, 255);
            Color32 fOrg = new Color32(255, 109, 0, 255);
            Color32 fYel = new Color32(255, 214, 0, 255);
            Color32 fWhite = new Color32(255, 255, 255, 255);

            // Wooden shaft
            for (int y = 1; y < 8; y++)
            {
                SetPixel(tile, 6, y, wDark);
                SetPixel(tile, 7, y, wCore);
                SetPixel(tile, 8, y, wLight);
                SetPixel(tile, 9, y, wDark);
            }

            for (int x = 6; x < 10; x++)
            {
                SetPixel(tile, x, 8, charred);
            }

            // Dynamic flame teardrop
            int[,] flame = new int[,]
            {
                {7, 9, 0}, {8, 9, 0},
                {6, 10, 0}, {7, 10, 1}, {8, 10, 1}, {9, 10, 0},
                {5, 11, 0}, {6, 11, 1}, {7, 11, 2}, {8, 11, 2}, {9, 11, 1}, {10, 11, 0},
                {5, 12, 0}, {6, 12, 2}, {7, 12, 3}, {8, 12, 3}, {9, 12, 2}, {10, 12, 0},
                {6, 13, 0}, {7, 13, 2}, {8, 13, 2}, {9, 13, 0},
                {7, 14, 1}, {8, 14, 1},
                {7, 15, 0}
            };

            for (int i = 0; i < flame.GetLength(0); i++)
            {
                int fx = flame[i, 0];
                int fy = flame[i, 1];
                int colorType = flame[i, 2];
                Color32 c = colorType == 0 ? fRed : (colorType == 1 ? fOrg : (colorType == 2 ? fYel : fWhite));
                SetPixel(tile, fx, fy, c);
                SetPixel(emit, fx, fy, c);
            }
        }

        private static void PaintObsidian(Color32[] tile)
        {
            Color32 oBlack = new Color32(18, 9, 28, 255);
            Color32 oDark = new Color32(28, 15, 42, 255);
            Color32 oMid = new Color32(48, 24, 72, 255);
            Color32 oPurple = new Color32(104, 36, 140, 255);
            Color32 oViolet = new Color32(156, 56, 199, 255);
            Color32 oGlint = new Color32(241, 225, 255, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 1313) % 100;
                    Color32 c = h < 45 ? oBlack : (h < 85 ? oDark : oMid);
                    SetPixel(tile, x, y, c);
                }
            }

            int[,] arcs = new int[,]
            {
                {2,4}, {3,5}, {4,6}, {5,8}, {6,9}, {7,10}, {8,12}, {9,13},
                {8,3}, {9,4}, {10,5}, {11,7}, {12,8}, {13,10}
            };
            for (int i = 0; i < arcs.GetLength(0); i++)
            {
                int ax = arcs[i, 0];
                int ay = arcs[i, 1];
                SetPixel(tile, ax, ay, oPurple);
                if (ax + 1 < TileResolution) SetPixel(tile, ax + 1, ay, oViolet);
            }

            SetPixel(tile, 6, 9, oGlint);
            SetPixel(tile, 11, 7, oGlint);
        }

        private static void PaintMagma(Color32[] tile, Color32[] emit)
        {
            Color32 crustDark = new Color32(23, 21, 21, 255);
            Color32 crustMid = new Color32(43, 40, 40, 255);
            Color32 crustHigh = new Color32(65, 60, 60, 255);

            Color32 lRed = new Color32(191, 21, 0, 255);
            Color32 lOrange = new Color32(255, 109, 0, 255);
            Color32 lYellow = new Color32(255, 214, 0, 255);
            Color32 lCore = new Color32(255, 249, 196, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 1414) % 100;
                    Color32 c = h < 40 ? crustDark : (h < 85 ? crustMid : crustHigh);
                    SetPixel(tile, x, y, c);
                }
            }

            int[,] fissures = new int[,]
            {
                {0, 3, 0}, {1, 4, 1}, {2, 4, 2}, {3, 5, 3},
                {4, 5, 2}, {5, 6, 1}, {6, 7, 3}, {7, 7, 2},
                {8, 8, 3}, {9, 8, 2}, {10, 9, 1}, {11, 10, 3},
                {12, 11, 2}, {13, 12, 1}, {14, 13, 0}, {15, 14, 0},
                {8, 7, 1}, {8, 6, 2}, {9, 5, 3}, {10, 4, 2}, {11, 3, 1}, {12, 2, 0},
                {6, 8, 1}, {6, 9, 2}, {5, 10, 3}, {5, 11, 2}, {4, 12, 1}, {3, 13, 0}
            };

            for (int i = 0; i < fissures.GetLength(0); i++)
            {
                int fx = fissures[i, 0];
                int fy = fissures[i, 1];
                int colorType = fissures[i, 2];
                Color32 c = colorType == 0 ? lRed : (colorType == 1 ? lOrange : (colorType == 2 ? lYellow : lCore));
                SetPixel(tile, fx, fy, c);
                SetPixel(emit, fx, fy, c);
            }
        }

        private static void PaintMossyStone(Color32[] tile)
        {
            PaintStone(tile);

            Color32 mDark = new Color32(46, 82, 16, 255);
            Color32 mMid = new Color32(78, 140, 34, 255);
            Color32 mLight = new Color32(128, 196, 51, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    if ((x < 9 && y > 6) || (x > 8 && y < 8) || (x % 3 == 0 && y % 2 == 0))
                    {
                        uint h = Hash2D(x, y, 1515) % 100;
                        Color32 c = h < 30 ? mDark : (h < 75 ? mMid : (h < 95 ? mLight : new Color32(0, 0, 0, 0)));
                        if (c.a > 0) SetPixel(tile, x, y, c);
                    }
                }
            }
        }

        private static void PaintMud(Color32[] tile)
        {
            Color32 mPeat = new Color32(44, 26, 14, 255);
            Color32 mMuck = new Color32(61, 38, 22, 255);
            Color32 mLoam = new Color32(77, 49, 30, 255);
            Color32 mPuddle = new Color32(99, 67, 43, 255);
            Color32 mSheen = new Color32(140, 104, 74, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 1616) % 100;
                    Color32 c = h < 35 ? mPeat : (h < 75 ? mMuck : mLoam);

                    if ((x >= 3 && x <= 7 && y >= 4 && y <= 7) || (x >= 10 && x <= 13 && y >= 9 && y <= 12))
                    {
                        c = ((x == 4 && y == 5) || (x == 11 && y == 10)) ? mSheen : mPuddle;
                    }
                    SetPixel(tile, x, y, c);
                }
            }
        }

        private static void PaintGlowShroom(Color32[] tile, Color32[] emit)
        {
            Color32 transparent = new Color32(0, 0, 0, 0);
            FillSolid(tile, transparent);

            Color32 stemDark = new Color32(40, 120, 140, 255);
            Color32 stemLight = new Color32(84, 197, 217, 255);
            Color32 capDeep = new Color32(0, 107, 133, 255);
            Color32 capBright = new Color32(0, 229, 255, 255);
            Color32 capSpot = new Color32(224, 255, 255, 255);

            // Stalk
            for (int y = 1; y < 7; y++)
            {
                SetPixel(tile, 6, y, stemDark);
                SetPixel(tile, 7, y, stemLight);
                SetPixel(tile, 8, y, stemLight);
                SetPixel(tile, 9, y, stemDark);
            }

            // Cap dome
            for (int y = 7; y <= 12; y++)
            {
                for (int x = 2; x <= 13; x++)
                {
                    if (y == 7 && (x < 3 || x > 12)) continue;
                    if (y == 11 && (x < 3 || x > 12)) continue;
                    if (y == 12 && (x < 5 || x > 10)) continue;

                    Color32 c = (y == 7 || y == 8) ? capDeep : capBright;
                    SetPixel(tile, x, y, c);
                    SetPixel(emit, x, y, c);
                }
            }

            // Spots
            int[,] spots = new int[,] { {5,10}, {7,11}, {10,10}, {4,8}, {11,8} };
            for (int i = 0; i < spots.GetLength(0); i++)
            {
                int sx = spots[i, 0];
                int sy = spots[i, 1];
                SetPixel(tile, sx, sy, capSpot);
                SetPixel(emit, sx, sy, capSpot);
            }

            // Floating spores
            int[,] spores = new int[,] { {3,14}, {6,15}, {11,14}, {13,13} };
            for (int i = 0; i < spores.GetLength(0); i++)
            {
                int sx = spores[i, 0];
                int sy = spores[i, 1];
                SetPixel(tile, sx, sy, capSpot);
                SetPixel(emit, sx, sy, capSpot);
            }
        }

        private static void PaintBedrock(Color32[] tile)
        {
            Color32 bVoid = new Color32(8, 9, 10, 255);
            Color32 bDark = new Color32(20, 22, 24, 255);
            Color32 bMineral = new Color32(34, 37, 42, 255);
            Color32 bFacet = new Color32(56, 60, 68, 255);

            for (int y = 0; y < TileResolution; y++)
            {
                for (int x = 0; x < TileResolution; x++)
                {
                    uint h = Hash2D(x, y, 1717) % 100;
                    Color32 c = h < 35 ? bVoid : (h < 75 ? bDark : (h < 93 ? bMineral : bFacet));
                    SetPixel(tile, x, y, c);
                }
            }
        }
    }
}
