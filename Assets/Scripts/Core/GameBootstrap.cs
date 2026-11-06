using System;
using Generation;
using Player;
using Rendering;
using UI;
using UnityEngine;
using World;

namespace Core
{
    /// <summary>
    /// Autonomous runtime bootstrapper.
    /// Ensures that regardless of scene configuration or editor state, all core sandbox
    /// systems (WorldManager, Player, Camera, DayNightCycle, Parallax, UI) exist and are linked.
    /// </summary>
    [DefaultExecutionOrder(-100)]
    public class GameBootstrap : MonoBehaviour
    {
        [Header("Auto-Setup Settings")]
        [SerializeField] private bool autoSetupIfMissing = true;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void OnRuntimeMethodLoad()
        {
            // If no GameBootstrap or WorldManager exists in the scene, instantiate one
            if (FindObjectOfType<WorldManager>() == null && FindObjectOfType<GameBootstrap>() == null)
            {
                GameObject bootObj = new GameObject("[GameBootstrap]");
                bootObj.AddComponent<GameBootstrap>();
            }
        }

        private void Awake()
        {
            if (autoSetupIfMissing)
            {
                EnsureEnvironmentSetup();
            }
        }

        public static void EnsureEnvironmentSetup()
        {
            // 1. Ensure Directional Light & DayNightCycle
            Light sunLight = FindObjectOfType<Light>();
            if (sunLight == null)
            {
                GameObject lightObj = new GameObject("Directional Light (Sun)");
                sunLight = lightObj.AddComponent<Light>();
                sunLight.type = LightType.Directional;
                sunLight.color = new Color(1.0f, 0.96f, 0.88f);
                sunLight.intensity = 1.15f;
                sunLight.shadows = LightShadows.Soft;
                lightObj.transform.rotation = Quaternion.Euler(38f, 15f, 0f);
            }

            if (sunLight.GetComponent<DayNightCycle>() == null)
            {
                sunLight.gameObject.AddComponent<DayNightCycle>();
            }

            // 2. Ensure TerrainGenerator & WorldManager
            WorldManager worldManager = WorldManager.Instance ?? FindObjectOfType<WorldManager>();
            if (worldManager == null)
            {
                GameObject worldObj = new GameObject("WorldManager");
                worldManager = worldObj.AddComponent<WorldManager>();
            }

            // 3. Ensure Procedural Parallax Background
            ProceduralBackground bg = FindObjectOfType<ProceduralBackground>();
            if (bg == null)
            {
                GameObject bgObj = new GameObject("ProceduralBackground");
                bgObj.AddComponent<ProceduralBackground>();
            }

            // 4. Ensure Player
            PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
            if (player == null)
            {
                GameObject playerObj = new GameObject("Player");
                playerObj.tag = "Player";
                player = playerObj.AddComponent<PlayerController2D5>();
                playerObj.AddComponent<BlockInteraction>();
                playerObj.AddComponent<InventorySystem>();

                // Spawn at surface
                Vector3 spawnPos = worldManager.FindSurfaceSpawnPosition(0f);
                playerObj.transform.position = spawnPos;
            }

            // 5. Ensure Camera & CameraController2D5
            Camera mainCam = Camera.main;
            if (mainCam == null)
            {
                GameObject camObj = new GameObject("Main Camera");
                camObj.tag = "MainCamera";
                mainCam = camObj.AddComponent<Camera>();
                camObj.AddComponent<AudioListener>();
            }

            mainCam.clearFlags = CameraClearFlags.SolidColor;
            mainCam.backgroundColor = new Color(0.52f, 0.72f, 0.95f);
            mainCam.nearClipPlane = 0.1f;
            mainCam.farClipPlane = 500f;
            mainCam.fieldOfView = 55f;

            CameraController2D5 camController = mainCam.GetComponent<CameraController2D5>();
            if (camController == null)
            {
                camController = mainCam.gameObject.AddComponent<CameraController2D5>();
            }
            camController.Target = player.transform;

            // 6. Ensure UIManager
            UIManager ui = UIManager.Instance ?? FindObjectOfType<UIManager>();
            if (ui == null)
            {
                GameObject uiObj = new GameObject("UIManager");
                uiObj.AddComponent<UIManager>();
            }

            // 7. Ensure SaveManager
            SaveManager saveManager = SaveManager.Instance ?? FindObjectOfType<SaveManager>();
            if (saveManager == null)
            {
                GameObject saveObj = new GameObject("SaveManager");
                saveObj.AddComponent<SaveManager>();
            }

            // 8. Ensure MobManager (2.5D Creature Spawning & AI System)
            Entities.MobManager mobManager = Entities.MobManager.Instance ?? FindObjectOfType<Entities.MobManager>();
            if (mobManager == null)
            {
                GameObject mobObj = new GameObject("MobManager");
                mobObj.AddComponent<Entities.MobManager>();
            }

            // 9. Ensure CraftingManager (Item Crafting & Recipe Database System)
            Crafting.CraftingManager craftingManager = Crafting.CraftingManager.Instance ?? FindObjectOfType<Crafting.CraftingManager>();
            if (craftingManager == null)
            {
                GameObject craftObj = new GameObject("CraftingManager");
                craftObj.AddComponent<Crafting.CraftingManager>();
            }

            // 10. Ensure CreativeInventoryUI (Big Creative Inventory [E])
            CreativeInventoryUI creativeUI = CreativeInventoryUI.Instance ?? FindObjectOfType<CreativeInventoryUI>();
            if (creativeUI == null)
            {
                GameObject cuiObj = new GameObject("CreativeInventoryUI");
                cuiObj.AddComponent<CreativeInventoryUI>();
            }
        }
    }
}
