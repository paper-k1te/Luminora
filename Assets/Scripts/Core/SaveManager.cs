using System;
using System.Collections.Generic;
using System.IO;
using Player;
using Rendering;
using UI;
using UnityEngine;
using World;

namespace Core
{
    /// <summary>
    /// Serialized representation of an individual item stack in the inventory.
    /// </summary>
    [Serializable]
    public struct InventorySlotData
    {
        public BlockType type;
        public int count;
        public bool isTool;
        public int toolId;

        public InventorySlotData(BlockType type, int count, bool isTool = false, int toolId = 0)
        {
            this.type = type;
            this.count = count;
            this.isTool = isTool;
            this.toolId = toolId;
        }
    }

    /// <summary>
    /// Stores discrete block deltas modified by the player relative to the base procedural world.
    /// Supports both interactive foreground blocks and background decorative walls.
    /// </summary>
    [Serializable]
    public struct BlockDelta
    {
        public int x;
        public int y;
        public BlockType newType;
        public bool isWall;

        public BlockDelta(int x, int y, BlockType newType, bool isWall = false)
        {
            this.x = x;
            this.y = y;
            this.newType = newType;
            this.isWall = isWall;
        }
    }

    /// <summary>
    /// Root JSON-serializable save payload structure.
    /// Stores minimal delta states ensuring save files remain lightweight (< 100 KB) and fast to parse.
    /// </summary>
    [Serializable]
    public class SaveData
    {
        public int seed;
        public float gameTime;
        public float playerX;
        public float playerY;
        public int activeSlot;
        public InventorySlotData[] inventory;
        public List<BlockDelta> modifiedBlocks = new List<BlockDelta>();
        public string timestamp;
        public int tutorialStep;
    }

    /// <summary>
    /// Master persistence and serialization coordinator for the 2.5D sandbox game.
    /// Provides high-performance delta-based world saving, hotkey quick saving/loading ([F5]/[F9]),
    /// configurable auto-saving, and full state restoration across Unity Editor and Standalone builds.
    /// </summary>
    [DisallowMultipleComponent]
    [DefaultExecutionOrder(-60)]
    public class SaveManager : MonoBehaviour
    {
        private static SaveManager _instance;

        public static SaveManager Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = FindObjectOfType<SaveManager>();
                    if (_instance == null)
                    {
                        GameObject obj = new GameObject("[SaveManager]");
                        _instance = obj.AddComponent<SaveManager>();
                    }
                }
                return _instance;
            }
            private set => _instance = value;
        }

        [Header("Save Configuration")]
        [SerializeField] private string defaultSaveFilename = "savegame.json";

        [Header("Auto-Save Settings")]
        [Tooltip("Automatically save the game periodically in the background.")]
        [SerializeField] private bool autoSaveEnabled = true;

        [Tooltip("Interval between automatic background saves in seconds.")]
        [SerializeField] private float autoSaveInterval = 120.0f;

        [Tooltip("Save world automatically when the application quits.")]
        [SerializeField] private bool autoSaveOnQuit = true;

        [Header("Hotkey Bindings")]
        [SerializeField] private KeyCode quickSaveKey = KeyCode.F5;
        [SerializeField] private KeyCode quickLoadKey = KeyCode.F9;

        // Internal modified blocks dictionary for O(1) delta query and replacement
        // Key: composite 64-bit integer encoding (x, y, isWall)
        private readonly Dictionary<ulong, BlockDelta> _deltaMap = new Dictionary<ulong, BlockDelta>();

        // Timer for auto-save
        private float _autoSaveTimer = 0f;

        // Events for external integrations
        public event Action OnSaveCompleted;
        public event Action OnLoadCompleted;

        public int ModifiedBlockCount => _deltaMap.Count;

        private void Awake()
        {
            if (_instance != null && _instance != this)
            {
                Destroy(gameObject);
                return;
            }

            _instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void Update()
        {
            HandleHotkeys();
            HandleAutoSave();
        }

        private void OnApplicationQuit()
        {
            if (autoSaveOnQuit)
            {
                Debug.Log("[SaveManager] Application quitting -> Executing automatic exit save...");
                SaveGame(defaultSaveFilename);
            }
        }

        /// <summary>
        /// Reads keyboard hotkeys: [F5] for Quick Save and [F9] for Quick Load.
        /// </summary>
        private void HandleHotkeys()
        {
            if (Input.GetKeyDown(quickSaveKey))
            {
                bool success = SaveGame(defaultSaveFilename);
                if (success)
                {
                    UIManager.ShowNotification("快捷存档成功 (World Saved!) [F5]");
                }
                else
                {
                    UIManager.ShowNotification("<color=#FF6666>快捷存档失败 (Save Failed!)</color>");
                }
            }

            if (Input.GetKeyDown(quickLoadKey))
            {
                if (HasSaveFile(defaultSaveFilename))
                {
                    bool success = LoadGame(defaultSaveFilename);
                    if (success)
                    {
                        UIManager.ShowNotification("存档读取成功 (World Loaded!) [F9]");
                    }
                    else
                    {
                        UIManager.ShowNotification("<color=#FF6666>存档读取失败 (Load Failed!)</color>");
                    }
                }
                else
                {
                    UIManager.ShowNotification("<color=#FFCC00>暂无存档记录 (No save file found) [F9]!</color>");
                }
            }
        }

        /// <summary>
        /// Counts elapsed in-game time and automatically performs background persistence.
        /// </summary>
        private void HandleAutoSave()
        {
            if (!autoSaveEnabled || autoSaveInterval <= 0f) return;

            _autoSaveTimer += Time.deltaTime;
            if (_autoSaveTimer >= autoSaveInterval)
            {
                _autoSaveTimer = 0f;
                bool saved = SaveGame(defaultSaveFilename);
                if (saved)
                {
                    UIManager.ShowNotification("Auto-Saved World!");
                }
            }
        }

        /// <summary>
        /// Generates a collision-free 64-bit coordinate key from 2D voxel coordinates and layer flag.
        /// </summary>
        public static ulong GetDeltaKey(int x, int y, bool isWall)
        {
            ulong ux = (ulong)(uint)x;
            ulong uy = (ulong)(uint)y;
            ulong wallBit = isWall ? 1UL : 0UL;
            return (ux << 32) | (uy << 8) | wallBit;
        }

        /// <summary>
        /// Records a player-initiated block modification into the active delta buffer.
        /// Called automatically whenever WorldManager.SetBlock or SetWall modifies a block.
        /// </summary>
        public void RecordBlockModification(int worldX, int worldY, BlockType type, bool isWall)
        {
            ulong key = GetDeltaKey(worldX, worldY, isWall);
            _deltaMap[key] = new BlockDelta(worldX, worldY, type, isWall);
        }

        /// <summary>
        /// Applies all recorded block modifications that fall within the boundaries of a given chunk.
        /// Called when new chunks stream in, ensuring player modifications persist during exploration.
        /// </summary>
        public void ApplyModificationsToChunk(Chunk chunk)
        {
            if (chunk == null || _deltaMap.Count == 0) return;

            int chunkX = chunk.ChunkX;
            int startX = chunkX * WorldSettings.ChunkWidth;
            int endX = startX + WorldSettings.ChunkWidth;

            foreach (var kvp in _deltaMap)
            {
                BlockDelta delta = kvp.Value;
                if (delta.x >= startX && delta.x < endX && delta.y >= 0 && delta.y < WorldSettings.ChunkHeight)
                {
                    int localX = delta.x - startX;
                    if (delta.isWall)
                    {
                        if (chunk.wallBlocks != null)
                        {
                            chunk.wallBlocks[localX, delta.y] = delta.newType;
                        }
                    }
                    else
                    {
                        if (chunk.blocks != null)
                        {
                            chunk.blocks[localX, delta.y] = delta.newType;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Clears all recorded in-memory block modifications.
        /// </summary>
        public void ClearModifications()
        {
            _deltaMap.Clear();
        }

        /// <summary>
        /// Retrieves the complete list of modified block deltas.
        /// </summary>
        public List<BlockDelta> GetModifiedBlocksList()
        {
            return new List<BlockDelta>(_deltaMap.Values);
        }

        /// <summary>
        /// Computes the absolute platform-independent file path for a save file.
        /// </summary>
        public string GetSaveFilePath(string filename = "savegame.json")
        {
            if (string.IsNullOrEmpty(filename))
            {
                filename = defaultSaveFilename;
            }

            string basePath = Application.persistentDataPath;
            if (string.IsNullOrEmpty(basePath))
            {
                basePath = Path.Combine(Directory.GetCurrentDirectory(), "Saves");
            }

            return Path.Combine(basePath, filename);
        }

        /// <summary>
        /// Checks whether a save file with the given name currently exists on disk.
        /// </summary>
        public bool HasSaveFile(string filename = "savegame.json")
        {
            string path = GetSaveFilePath(filename);
            return File.Exists(path);
        }

        /// <summary>
        /// Deletes a save file from disk.
        /// </summary>
        public bool DeleteSave(string filename = "savegame.json")
        {
            try
            {
                string path = GetSaveFilePath(filename);
                if (File.Exists(path))
                {
                    File.Delete(path);
                    Debug.Log($"[SaveManager] Deleted save file: {path}");
                    return true;
                }
                return false;
            }
            catch (Exception ex)
            {
                Debug.LogError($"[SaveManager] Failed to delete save file '{filename}': {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Serializes the entire world state into JSON and saves it to persistent storage.
        /// Captures player position, inventory, game time, world seed, tutorial progress, and block deltas.
        /// </summary>
        public bool SaveGame(string filename = "savegame.json")
        {
            try
            {
                // 1. Gather player transform
                Vector3 playerPos = Vector3.zero;
                PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    playerPos = player.Position;
                }
                else if (WorldManager.Instance != null && WorldManager.Instance.PlayerTransform != null)
                {
                    playerPos = WorldManager.Instance.PlayerTransform.position;
                }

                // 2. Gather active seed
                int activeSeed = WorldSettings.ActiveSeed;
                if (WorldManager.Instance != null && WorldManager.Instance.TerrainGen != null)
                {
                    activeSeed = WorldManager.Instance.TerrainGen.Seed;
                }

                // 3. Gather in-game celestial time
                float gameTime = 0.25f; // default noon
                if (DayNightCycle.Instance != null)
                {
                    gameTime = DayNightCycle.Instance.TimeOfDay;
                }

                // 4. Gather inventory state
                InventorySystem inv = InventorySystem.Instance ?? FindObjectOfType<InventorySystem>();
                int activeSlot = 0;
                InventorySlotData[] inventoryData = null;

                if (inv != null)
                {
                    activeSlot = inv.ActiveSlotIndex;
                    inventoryData = inv.GetInventoryData();
                }

                // 5. Gather tutorial step
                int tutorialStep = 0;
                if (UIManager.Instance != null)
                {
                    tutorialStep = UIManager.Instance.TutorialStep;
                }

                // 6. Build SaveData payload
                SaveData saveData = new SaveData
                {
                    seed = activeSeed,
                    gameTime = gameTime,
                    playerX = playerPos.x,
                    playerY = playerPos.y,
                    activeSlot = activeSlot,
                    inventory = inventoryData,
                    modifiedBlocks = GetModifiedBlocksList(),
                    timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
                    tutorialStep = tutorialStep
                };

                // 7. Serialize to formatted JSON
                string json = JsonUtility.ToJson(saveData, true);

                // 8. Ensure directory and write file
                string filePath = GetSaveFilePath(filename);
                string directory = Path.GetDirectoryName(filePath);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                File.WriteAllText(filePath, json);

                Debug.Log($"[SaveManager] World state successfully saved to '{filePath}' ({saveData.modifiedBlocks.Count} modified blocks, seed: {activeSeed}).");
                OnSaveCompleted?.Invoke();
                return true;
            }
            catch (Exception ex)
            {
                Debug.LogError($"[SaveManager] Failed to save game to '{filename}': {ex}");
                return false;
            }
        }

        /// <summary>
        /// Reads save file from persistent storage and completely restores world, player,
        /// inventory, celestial lighting, and tutorial state.
        /// </summary>
        public bool LoadGame(string filename = "savegame.json")
        {
            string filePath = GetSaveFilePath(filename);
            if (!File.Exists(filePath))
            {
                Debug.LogWarning($"[SaveManager] Save file not found at: {filePath}");
                return false;
            }

            try
            {
                string json = File.ReadAllText(filePath);
                SaveData data = JsonUtility.FromJson<SaveData>(json);

                if (data == null)
                {
                    Debug.LogError($"[SaveManager] Failed to deserialize save data from: {filePath}");
                    return false;
                }

                // 1. Populate delta modifications in SaveManager memory
                ClearModifications();
                if (data.modifiedBlocks != null)
                {
                    for (int i = 0; i < data.modifiedBlocks.Count; i++)
                    {
                        BlockDelta delta = data.modifiedBlocks[i];
                        ulong key = GetDeltaKey(delta.x, delta.y, delta.isWall);
                        _deltaMap[key] = delta;
                    }
                }

                // 2. Restore World and regenerates chunks with modifications applied
                Vector3 restoredPlayerPos = new Vector3(data.playerX, data.playerY, 0f);
                if (WorldManager.Instance != null)
                {
                    WorldManager.Instance.LoadWorldState(data.seed, restoredPlayerPos);
                }

                // 3. Teleport player safely
                PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    player.Teleport(restoredPlayerPos);
                }

                // 4. Restore Inventory & Active Slot
                InventorySystem inv = InventorySystem.Instance ?? FindObjectOfType<InventorySystem>();
                if (inv != null)
                {
                    if (data.inventory != null)
                    {
                        inv.SetInventoryData(data.inventory);
                    }
                    inv.SetActiveSlot(data.activeSlot);
                }

                // 5. Restore In-Game Time of Day
                if (DayNightCycle.Instance != null)
                {
                    DayNightCycle.Instance.SetTimeOfDay(data.gameTime);
                }

                // 6. Restore Tutorial Step
                if (UIManager.Instance != null)
                {
                    UIManager.Instance.TutorialStep = data.tutorialStep;
                }

                Debug.Log($"[SaveManager] World state successfully loaded from '{filePath}' (Seed: {data.seed}, Modified Blocks: {_deltaMap.Count}).");
                OnLoadCompleted?.Invoke();
                return true;
            }
            catch (Exception ex)
            {
                Debug.LogError($"[SaveManager] Failed to load game from '{filename}': {ex}");
                return false;
            }
        }
    }
}
