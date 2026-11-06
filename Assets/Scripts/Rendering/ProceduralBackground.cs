using System;
using System.Collections.Generic;
using Core;
using Generation;
using UnityEngine;

namespace Rendering
{
    /// <summary>
    /// Luminora-like multi-layer 2.5D parallax background system.
    /// Manages 3 infinite scrolling depth planes (Far Mountains, Mid Hills, Clouds)
    /// that dynamically adapt their color silhouettes based on active biome and day/night progression.
    /// </summary>
    [ExecuteAlways]
    public class ProceduralBackground : MonoBehaviour
    {
        [Header("Target & Tracking")]
        [SerializeField] private Camera targetCamera;

        [Header("Layer Depths (Z-Axis)")]
        [SerializeField] private float farLayerZ = 40.0f;
        [SerializeField] private float midLayerZ = 20.0f;
        [SerializeField] private float cloudLayerZ = 10.0f;

        [Header("Parallax Coefficients")]
        [Tooltip("Horizontal tracking speed ratio relative to camera (near 1.0 = moves with camera / appears distant).")]
        [SerializeField] private float farParallax = 0.88f;
        [SerializeField] private float midParallax = 0.60f;
        [SerializeField] private float cloudParallax = 0.30f;

        [Header("Atmospheric Drift")]
        [SerializeField] private float cloudWindSpeed = 1.8f;
        [SerializeField] private float segmentWidth = 140.0f;

        // Runtime container transforms
        private Transform _farContainer;
        private Transform _midContainer;
        private Transform _cloudContainer;

        // Meshes and renderers
        private BackgroundLayer _farData;
        private BackgroundLayer _midData;
        private BackgroundLayer _cloudData;

        private float _cloudWindOffset = 0f;
        private BiomeType _currentBiome = BiomeType.Forest;
        private TerrainGenerator _terrainGenerator;

        // Current interpolated colors
        private Color _currentFarColor;
        private Color _currentMidColor;
        private Color _currentCloudColor;

        private class BackgroundLayer
        {
            public GameObject[] Segments;
            public MeshFilter[] Filters;
            public MeshRenderer[] Renderers;
            public Material SharedMaterial;
            public float ZDepth;
            public float ParallaxFactor;
        }

        private void Awake()
        {
            SetupLayers();
        }

        private void OnEnable()
        {
            SetupLayers();
        }

        private void Start()
        {
            if (targetCamera == null) targetCamera = Camera.main;
            _terrainGenerator = FindObjectOfType<TerrainGenerator>();
        }

        private void LateUpdate()
        {
            if (targetCamera == null)
            {
                targetCamera = Camera.main;
                if (targetCamera == null) return;
            }

            Vector3 camPos = targetCamera.transform.position;

            // Update wind
            if (Application.isPlaying)
            {
                _cloudWindOffset += cloudWindSpeed * Time.deltaTime;
            }

            // Determine active biome
            UpdateBiome(camPos.x);

            // Update biome and celestial colors
            UpdateAtmosphericColors();

            // Parallax scroll and infinite wrap
            UpdateLayerPositions(camPos);
        }

        /// <summary>
        /// Detects active biome from world X coordinate.
        /// </summary>
        private void UpdateBiome(float worldX)
        {
            if (_terrainGenerator == null)
            {
                _terrainGenerator = FindObjectOfType<TerrainGenerator>();
            }

            if (_terrainGenerator != null)
            {
                _currentBiome = _terrainGenerator.GetBiomeAt(worldX);
            }
            else
            {
                // Fallback deterministic biome band calculation
                float noise = Mathf.PerlinNoise(worldX * WorldSettings.BiomeScale + 12.3f, 0.5f);
                if (noise < 0.33f) _currentBiome = BiomeType.Desert;
                else if (noise < 0.67f) _currentBiome = BiomeType.Forest;
                else _currentBiome = BiomeType.Tundra;
            }
        }

        /// <summary>
        /// Smoothly interpolates background colors between biomes and day/night cycle.
        /// </summary>
        private void UpdateAtmosphericColors()
        {
            Color targetFar;
            Color targetMid;
            Color targetCloud;

            switch (_currentBiome)
            {
                case BiomeType.Desert:
                    targetFar = new Color(0.85f, 0.60f, 0.38f);
                    targetMid = new Color(0.75f, 0.45f, 0.28f);
                    targetCloud = new Color(0.98f, 0.94f, 0.85f);
                    break;

                case BiomeType.Tundra:
                    targetFar = new Color(0.70f, 0.82f, 0.92f);
                    targetMid = new Color(0.42f, 0.56f, 0.62f);
                    targetCloud = new Color(0.92f, 0.96f, 1.00f);
                    break;

                case BiomeType.Underworld:
                    targetFar = new Color(0.22f, 0.08f, 0.14f);
                    targetMid = new Color(0.38f, 0.14f, 0.08f);
                    targetCloud = new Color(0.35f, 0.20f, 0.18f);
                    break;

                case BiomeType.Forest:
                default:
                    targetFar = new Color(0.40f, 0.52f, 0.68f);
                    targetMid = new Color(0.22f, 0.42f, 0.25f);
                    targetCloud = new Color(0.96f, 0.98f, 1.00f);
                    break;
            }

            // Modulate with DayNightCycle if present
            if (DayNightCycle.Instance != null)
            {
                Color ambient = DayNightCycle.Instance.CurrentAmbientColor;
                Color sky = DayNightCycle.Instance.CurrentSkyColor;
                Color light = DayNightCycle.Instance.CurrentLightColor;

                // Blend sky and light tint into background silhouettes
                Color celestialMod = Color.Lerp(ambient, sky, 0.45f) * light;

                targetFar *= celestialMod * 1.3f;
                targetMid *= celestialMod * 1.1f;
                targetCloud *= Color.Lerp(sky, light, 0.5f);
            }

            // Smooth color transition
            float dt = Application.isPlaying ? Time.deltaTime * 3.0f : 1.0f;
            _currentFarColor = Color.Lerp(_currentFarColor, targetFar, dt);
            _currentMidColor = Color.Lerp(_currentMidColor, targetMid, dt);
            _currentCloudColor = Color.Lerp(_currentCloudColor, targetCloud, dt);

            // Apply to shared materials
            if (_farData?.SharedMaterial != null) _farData.SharedMaterial.color = _currentFarColor;
            if (_midData?.SharedMaterial != null) _midData.SharedMaterial.color = _currentMidColor;
            if (_cloudData?.SharedMaterial != null) _cloudData.SharedMaterial.color = _currentCloudColor;
        }

        /// <summary>
        /// Updates the horizontal position of each layer segment according to parallax factors.
        /// Horizontally wraps segments as the camera moves infinitely across the world.
        /// </summary>
        private void UpdateLayerPositions(Vector3 camPos)
        {
            UpdateLayerTracking(_farData, camPos.x, camPos.y, 0f);
            UpdateLayerTracking(_midData, camPos.x, camPos.y, 0f);
            UpdateLayerTracking(_cloudData, camPos.x, camPos.y, _cloudWindOffset);
        }

        private void UpdateLayerTracking(BackgroundLayer layer, float camX, float camY, float extraOffset)
        {
            if (layer == null || layer.Segments == null) return;

            // Parallax position calculation:
            // Position = camX * parallaxFactor + extraOffset
            float parallaxX = (camX * layer.ParallaxFactor) + extraOffset;
            float baseY = camY * 0.25f; // Gentle vertical parallax

            // Seamless 3-segment modular positioning:
            // Anchor index centered around camera
            int centerSegmentIndex = Mathf.FloorToInt((camX - parallaxX) / segmentWidth);

            for (int i = 0; i < layer.Segments.Length; i++)
            {
                int segOffset = i - 1; // -1, 0, 1
                float segX = parallaxX + (centerSegmentIndex + segOffset) * segmentWidth;

                // Keep segments spanning [-segmentWidth, +segmentWidth] relative to camera
                while (segX < camX - segmentWidth * 1.5f) segX += segmentWidth * 3f;
                while (segX > camX + segmentWidth * 1.5f) segX -= segmentWidth * 3f;

                layer.Segments[i].transform.position = new Vector3(segX, baseY, layer.ZDepth);
            }
        }

        // =====================================================================
        // Procedural Layer Construction & Mesh Synthesis
        // =====================================================================

        private void SetupLayers()
        {
            Shader bgShader = Shader.Find("Sprites/Default") ?? Shader.Find("UI/Default") ?? Shader.Find("Unlit/Color");

            // 1. Far Layer (Mountains)
            if (_farContainer == null)
            {
                _farContainer = GetOrCreateContainer("Far_Mountains_Layer");
                _farData = CreateLayer("FarMountain", farLayerZ, farParallax, bgShader, GenerateFarMountainMesh());
            }

            // 2. Mid Layer (Foothills & Forest)
            if (_midContainer == null)
            {
                _midContainer = GetOrCreateContainer("Mid_Foothills_Layer");
                _midData = CreateLayer("MidHill", midLayerZ, midParallax, bgShader, GenerateMidHillMesh());
            }

            // 3. Cloud Layer
            if (_cloudContainer == null)
            {
                _cloudContainer = GetOrCreateContainer("Cloud_Layer");
                _cloudData = CreateLayer("CloudCluster", cloudLayerZ, cloudParallax, bgShader, GenerateCloudMesh());
            }
        }

        private Transform GetOrCreateContainer(string name)
        {
            Transform existing = transform.Find(name);
            if (existing != null) return existing;

            GameObject go = new GameObject(name);
            go.transform.SetParent(transform, false);
            return go.transform;
        }

        private BackgroundLayer CreateLayer(string prefix, float zDepth, float parallax, Shader shader, Mesh mesh)
        {
            BackgroundLayer layer = new BackgroundLayer
            {
                Segments = new GameObject[3],
                Filters = new MeshFilter[3],
                Renderers = new MeshRenderer[3],
                SharedMaterial = new Material(shader) { name = $"{prefix}_Material" },
                ZDepth = zDepth,
                ParallaxFactor = parallax
            };

            Transform parent = (prefix.Contains("Far")) ? _farContainer : (prefix.Contains("Mid") ? _midContainer : _cloudContainer);

            for (int i = 0; i < 3; i++)
            {
                string segName = $"{prefix}_Seg_{i}";
                Transform segTrans = parent.Find(segName);
                GameObject segGo = segTrans != null ? segTrans.gameObject : new GameObject(segName);
                segGo.transform.SetParent(parent, false);

                MeshFilter mf = segGo.GetComponent<MeshFilter>() ?? segGo.AddComponent<MeshFilter>();
                MeshRenderer mr = segGo.GetComponent<MeshRenderer>() ?? segGo.AddComponent<MeshRenderer>();

                mf.sharedMesh = mesh;
                mr.sharedMaterial = layer.SharedMaterial;
                mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                mr.receiveShadows = false;

                layer.Segments[i] = segGo;
                layer.Filters[i] = mf;
                layer.Renderers[i] = mr;
            }

            return layer;
        }

        /// <summary>
        /// Generates a jagged panoramic alpine mountain ridge mesh with seamless horizontal boundary matching.
        /// </summary>
        private Mesh GenerateFarMountainMesh()
        {
            Mesh mesh = new Mesh { name = "FarMountain_Mesh" };

            int segments = 40;
            float step = segmentWidth / segments;
            float baseBottom = -25f;

            List<Vector3> verts = new List<Vector3>();
            List<int> tris = new List<int>();
            List<Color32> colors = new List<Color32>();

            for (int i = 0; i <= segments; i++)
            {
                float x = i * step;
                // Periodic harmonic wave ensuring seam alignment at x=0 and x=segmentWidth
                float normX = (float)i / segments;
                float angle = normX * Mathf.PI * 2f;

                float peak = Mathf.Sin(angle * 2f) * 12f
                           + Mathf.Sin(angle * 5f + 0.4f) * 6f
                           + Mathf.Cos(angle * 9f) * 3f + 32f;

                // Bottom vertex
                verts.Add(new Vector3(x, baseBottom, 0f));
                colors.Add(new Color32(180, 180, 180, 255));

                // Top ridge vertex
                verts.Add(new Vector3(x, peak, 0f));
                colors.Add(new Color32(255, 255, 255, 255));
            }

            for (int i = 0; i < segments; i++)
            {
                int b0 = i * 2;
                int t0 = i * 2 + 1;
                int b1 = (i + 1) * 2;
                int t1 = (i + 1) * 2 + 1;

                tris.Add(b0); tris.Add(t0); tris.Add(t1);
                tris.Add(b0); tris.Add(t1); tris.Add(b1);
            }

            mesh.SetVertices(verts);
            mesh.SetTriangles(tris, 0);
            mesh.SetColors(colors);
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            return mesh;
        }

        /// <summary>
        /// Generates a rolling hillside ridge mesh with seamless looping.
        /// </summary>
        private Mesh GenerateMidHillMesh()
        {
            Mesh mesh = new Mesh { name = "MidHill_Mesh" };

            int segments = 32;
            float step = segmentWidth / segments;
            float baseBottom = -20f;

            List<Vector3> verts = new List<Vector3>();
            List<int> tris = new List<int>();
            List<Color32> colors = new List<Color32>();

            for (int i = 0; i <= segments; i++)
            {
                float x = i * step;
                float normX = (float)i / segments;
                float angle = normX * Mathf.PI * 2f;

                float hill = Mathf.Sin(angle) * 8f
                           + Mathf.Sin(angle * 3f) * 4f + 16f;

                verts.Add(new Vector3(x, baseBottom, 0f));
                colors.Add(new Color32(190, 190, 190, 255));

                verts.Add(new Vector3(x, hill, 0f));
                colors.Add(new Color32(255, 255, 255, 255));
            }

            for (int i = 0; i < segments; i++)
            {
                int b0 = i * 2;
                int t0 = i * 2 + 1;
                int b1 = (i + 1) * 2;
                int t1 = (i + 1) * 2 + 1;

                tris.Add(b0); tris.Add(t0); tris.Add(t1);
                tris.Add(b0); tris.Add(t1); tris.Add(b1);
            }

            mesh.SetVertices(verts);
            mesh.SetTriangles(tris, 0);
            mesh.SetColors(colors);
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            return mesh;
        }

        /// <summary>
        /// Generates puffy floating cumulus clouds scattered horizontally across the sky.
        /// </summary>
        private Mesh GenerateCloudMesh()
        {
            Mesh mesh = new Mesh { name = "Cloud_Mesh" };

            List<Vector3> verts = new List<Vector3>();
            List<int> tris = new List<int>();
            List<Color32> colors = new List<Color32>();

            // Generate several cloud clumps along the segment width
            int cloudCount = 5;
            float spacing = segmentWidth / cloudCount;

            for (int c = 0; c < cloudCount; c++)
            {
                float cx = (c + 0.5f) * spacing + ((c % 2 == 0) ? -6f : 6f);
                float cy = 35f + (c % 3) * 6f;
                float cloudWidth = 14f + (c % 2) * 5f;
                float cloudHeight = 4f + (c % 3) * 1.5f;

                AddCloudPuff(cx, cy, cloudWidth, cloudHeight, verts, tris, colors);
            }

            mesh.SetVertices(verts);
            mesh.SetTriangles(tris, 0);
            mesh.SetColors(colors);
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            return mesh;
        }

        private void AddCloudPuff(float cx, float cy, float w, float h, List<Vector3> verts, List<int> tris, List<Color32> colors)
        {
            int startIdx = verts.Count;
            int numPoints = 12;

            // Center vertex
            verts.Add(new Vector3(cx, cy, 0f));
            colors.Add(new Color32(255, 255, 255, 255));

            // Perimeter vertices
            for (int i = 0; i <= numPoints; i++)
            {
                float angle = (float)i / numPoints * Mathf.PI * 2f;
                float rx = Mathf.Cos(angle) * (w * 0.5f);
                float ry = Mathf.Sin(angle) * (h * 0.5f);

                // Add puff lobes
                float lobe = 1f + 0.2f * Mathf.Sin(angle * 4f);
                verts.Add(new Vector3(cx + rx * lobe, cy + ry * lobe, 0f));
                colors.Add(new Color32(255, 255, 255, 200));
            }

            for (int i = 1; i <= numPoints; i++)
            {
                tris.Add(startIdx);
                tris.Add(startIdx + i);
                tris.Add(startIdx + i + 1);
            }
        }
    }
}
