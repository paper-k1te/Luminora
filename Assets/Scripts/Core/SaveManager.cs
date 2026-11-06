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
    /// Lightweight summary metadata describing a save slot without loading all voxel delta blocks.
    /// </summary>
    [Serializable]
    public class SaveMetadata
    {
        public int slotIndex;
        public string worldName = "New World";
        public GameMode gameMode = GameMode.Survival;
        public int seed;
        public string lastPlayedTimestamp = "";
        public float playerHealth = 100f;
        public float maxHealth = 100f;
        public int modifiedBlocksCount = 0;
        public bool exists = false;
    }

    /// <summary>
    /// Root JSON-serializable save payload structure.
    /// Stores world metadata, player state, inventory, celestial time, and block deltas.
    /// </summary>
    [Serializable]
    public class SaveData
    {
        public int slotIndex = 0;
        public string worldName = "World 1";
        public GameMode gameMode = GameMode.Survival;
        public int seed;
        public float gameTime;
        public float playerX;
        public float playerY;
        public float playerHealth = 100f;
        public float maxHealth = 100f;
        public int activeSlot;
        public InventorySlotData[] inventory;
        public List<BlockDelta> modifiedBlocks = new List<BlockDelta>();
        public string timestamp;
        public int tutorialStep;
    }

    /// <summary>
    /// Master persistence and multi-save coordinator for the 2.5D sandbox game.
    /// Supports multi-slot management (slots 0..3: "world_0.json" .. "world_3.json"),
    /// metadata extraction, customizable world creation options, quick-saving/loading,
    /// and auto-saving.
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

        public const int MaxSaveSlots = 4; // Slots 0, 1, 2, 3

        [Header("Active Save Slot")]
        [SerializeField] private int currentSlotIndex = 0;
        [SerializeField] private string currentWorldName = "World 1";

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

        public int CurrentSlotIndex
        {
            get => currentSlotIndex;
            set => currentSlotIndex = Mathf.Clamp(value, 0, MaxSaveSlots - 1);
        }

        public string CurrentWorldName
        {
            get => currentWorldName;
            set => currentWorldName = string.IsNullOrEmpty(value) ? $"World {currentSlotIndex + 1}" : value;
        }

        public int ModifiedBlockCount => _deltaMap.Count;

        public static string GetSlotFileName(int slotIndex)
        {
            int s = Mathf.Clamp(slotIndex, 0, MaxSaveSlots - 1);
            return $"world_{s}.json";
        }

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
                SaveGame(currentSlotIndex);
            }
        }

        /// <summary>
        /// Reads keyboard hotkeys: [F5] for Quick Save and [F9] for Quick Load.
        /// </summary>
        private void HandleHotkeys()
        {
            if (Input.GetKeyDown(quickSaveKey))
            {
                bool success = SaveGame(currentSlotIndex);
                if (success)
                {
                    UIManager.ShowNotification($"快捷存档成功 (Slot {currentSlotIndex + 1} Saved!) [F5]");
                }
                else
                {
                    UIManager.ShowNotification("<color=#FF6666>快捷存档失败 (Save Failed!)</color>");
                }
            }

            if (Input.GetKeyDown(quickLoadKey))
            {
                if (HasSaveFile(GetSlotFileName(currentSlotIndex)))
                {
                    bool success = LoadGame(currentSlotIndex);
                    if (success)
                    {
                        UIManager.ShowNotification($"存档读取成功 (Slot {currentSlotIndex + 1} Loaded!) [F9]");
                    }
                    else
                    {
                        UIManager.ShowNotification("<color=#FF6666>存档读取失败 (Load Failed!)</color>");
                    }
                }
                else
                {
                    UIManager.ShowNotification($"<color=#FFCC00>槽位 {currentSlotIndex + 1} 暂无存档记录 (No save found) [F9]!</color>");
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
                bool saved = SaveGame(currentSlotIndex);
                if (saved)
                {
                    UIManager.ShowNotification($"Auto-Saved Slot {currentSlotIndex + 1}!");
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
        /// </summary>
        public void RecordBlockModification(int worldX, int worldY, BlockType type, bool isWall)
        {
            ulong key = GetDeltaKey(worldX, worldY, isWall);
            _deltaMap[key] = new BlockDelta(worldX, worldY, type, isWall);
        }

        /// <summary>
        /// Applies all recorded block modifications that fall within the boundaries of a given chunk.
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
        public string GetSaveFilePath(string filename = null)
        {
            if (string.IsNullOrEmpty(filename))
            {
                filename = GetSlotFileName(currentSlotIndex);
            }

            string basePath = Application.persistentDataPath;
            if (string.IsNullOrEmpty(basePath))
            {
                basePath = Path.Combine(Directory.GetCurrentDirectory(), "Saves");
            }

            return Path.Combine(basePath, filename);
        }

        /// <summary>
        /// Checks whether a save file exists on disk.
        /// </summary>
        public bool HasSaveFile(string filename = null)
        {
            string path = GetSaveFilePath(filename);
            return File.Exists(path);
        }

        /// <summary>
        /// Reads lightweight metadata for a specific save slot index.
        /// </summary>
        public SaveMetadata GetSaveMetadata(int slotIndex)
        {
            string filename = GetSlotFileName(slotIndex);
            string filePath = GetSaveFilePath(filename);

            SaveMetadata meta = new SaveMetadata
            {
                slotIndex = slotIndex,
                worldName = $"World {slotIndex + 1}",
                exists = false
            };

            if (!File.Exists(filePath))
            {
                return meta;
            }

            try
            {
                string json = File.ReadAllText(filePath);
                SaveData data = JsonUtility.FromJson<SaveData>(json);
                if (data != null)
                {
                    meta.worldName = string.IsNullOrEmpty(data.worldName) ? $"World {slotIndex + 1}" : data.worldName;
                    meta.gameMode = data.gameMode;
                    meta.seed = data.seed;
                    meta.lastPlayedTimestamp = data.timestamp;
                    meta.playerHealth = data.playerHealth;
                    meta.maxHealth = data.maxHealth > 0f ? data.maxHealth : 100f;
                    meta.modifiedBlocksCount = data.modifiedBlocks != null ? data.modifiedBlocks.Count : 0;
                    meta.exists = true;
                }
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"[SaveManager] Error reading metadata for slot {slotIndex}: {ex.Message}");
            }

            return meta;
        }

        /// <summary>
        /// Retrieves metadata for all 4 save slots.
        /// </summary>
        public List<SaveMetadata> GetAllSaveSlots()
        {
            List<SaveMetadata> list = new List<SaveMetadata>(MaxSaveSlots);
            for (int i = 0; i < MaxSaveSlots; i++)
            {
                list.Add(GetSaveMetadata(i));
            }
            return list;
        }

        /// <summary>
        /// Deletes a save slot file from disk.
        /// </summary>
        public bool DeleteSave(int slotIndex)
        {
            try
            {
                string filename = GetSlotFileName(slotIndex);
                string path = GetSaveFilePath(filename);
                if (File.Exists(path))
                {
                    File.Delete(path);
                    Debug.Log($"[SaveManager] Deleted save slot {slotIndex}: {path}");
                    return true;
                }
                return false;
            }
            catch (Exception ex)
            {
                Debug.LogError($"[SaveManager] Failed to delete save slot {slotIndex}: {ex.Message}");
                return false;
            }
        }

        public bool DeleteSave(string filename)
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
        /// Creates a brand-new save slot with customizable options:
        /// worldName, gameMode (Survival vs Creative), and seed.
        /// In Survival mode, starts with 0 items (completely empty from scratch).
        /// </summary>
        public void CreateNewSave(int slotIndex, string worldName, GameMode mode, int seed)
        {
            currentSlotIndex = Mathf.Clamp(slotIndex, 0, MaxSaveSlots - 1);
            currentWorldName = string.IsNullOrEmpty(worldName) ? $"World {currentSlotIndex + 1}" : worldName;

            WorldSettings.CurrentGameMode = mode;
            WorldSettings.ActiveSeed = seed;

            ClearModifications();

            // Setup player
            PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
            if (player != null)
            {
                player.CurrentGameMode = mode;
                player.SetHealth(100f, 100f);
            }

            // Setup inventory
            InventorySystem inv = InventorySystem.Instance ?? FindObjectOfType<InventorySystem>();
            if (inv != null)
            {
                if (mode == GameMode.Survival)
                {
                    // Survival mode starts with 0 items (completely empty from scratch)
                    inv.ClearAllSlots();
                }
                else
                {
                    inv.ProvisionStarterKit();
                }
            }

            // Regenerate world
            Vector3 spawnPos = new Vector3(0f, WorldSettings.BaseSurfaceLevel + 2f, 0f);
            if (WorldManager.Instance != null)
            {
                WorldManager.Instance.LoadWorldState(seed, spawnPos);
            }
            if (player != null)
            {
                player.Respawn();
            }

            // Save initial state to disk
            SaveGame(currentSlotIndex);
        }

        /// <summary>
        /// Saves current world state to a designated slot index.
        /// </summary>
        public bool SaveGame(int slotIndex)
        {
            currentSlotIndex = Mathf.Clamp(slotIndex, 0, MaxSaveSlots - 1);
            return SaveGame(GetSlotFileName(currentSlotIndex));
        }

        /// <summary>
        /// Loads world state from a designated slot index.
        /// </summary>
        public bool LoadGame(int slotIndex)
        {
            currentSlotIndex = Mathf.Clamp(slotIndex, 0, MaxSaveSlots - 1);
            return LoadGame(GetSlotFileName(currentSlotIndex));
        }

        /// <summary>
        /// Serializes the entire world state into JSON and saves it to persistent storage.
        /// Captures worldName, gameMode, player position, health, inventory, celestial time, seed, and block deltas.
        /// </summary>
        public bool SaveGame(string filename = null)
        {
            try
            {
                if (string.IsNullOrEmpty(filename))
                {
                    filename = GetSlotFileName(currentSlotIndex);
                }

                // 1. Gather player transform, health, and mode
                Vector3 playerPos = Vector3.zero;
                float playerHealth = 100f;
                float maxHealth = 100f;
                GameMode activeMode = WorldSettings.CurrentGameMode;

                PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    playerPos = player.Position;
                    playerHealth = player.CurrentHealth;
                    maxHealth = player.MaxHealth;
                    activeMode = player.CurrentGameMode;
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
                    slotIndex = currentSlotIndex,
                    worldName = currentWorldName,
                    gameMode = activeMode,
                    seed = activeSeed,
                    gameTime = gameTime,
                    playerX = playerPos.x,
                    playerY = playerPos.y,
                    playerHealth = playerHealth,
                    maxHealth = maxHealth,
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

                Debug.Log($"[SaveManager] World '{currentWorldName}' successfully saved to '{filePath}' (Slot {currentSlotIndex}, Mode: {activeMode}, Health: {playerHealth:F0}/{maxHealth:F0}, Seed: {activeSeed}).");
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
        /// game mode, health, inventory, celestial lighting, and tutorial state.
        /// </summary>
        public bool LoadGame(string filename = null)
        {
            if (string.IsNullOrEmpty(filename))
            {
                filename = GetSlotFileName(currentSlotIndex);
            }

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

                currentSlotIndex = Mathf.Clamp(data.slotIndex, 0, MaxSaveSlots - 1);
                currentWorldName = string.IsNullOrEmpty(data.worldName) ? $"World {currentSlotIndex + 1}" : data.worldName;
                WorldSettings.CurrentGameMode = data.gameMode;
                WorldSettings.ActiveSeed = data.seed;

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

                // 2. Restore World and regenerate chunks with modifications applied
                Vector3 restoredPlayerPos = new Vector3(data.playerX, data.playerY, 0f);
                if (WorldManager.Instance != null)
                {
                    WorldManager.Instance.LoadWorldState(data.seed, restoredPlayerPos);
                }

                // 3. Restore Player position, game mode, and health
                PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    player.CurrentGameMode = data.gameMode;
                    float hp = data.playerHealth > 0f ? data.playerHealth : 100f;
                    float maxHp = data.maxHealth > 0f ? data.maxHealth : 100f;
                    player.SetHealth(hp, maxHp);
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

                Debug.Log($"[SaveManager] World '{currentWorldName}' loaded successfully from '{filePath}' (Slot {currentSlotIndex}, Mode: {data.gameMode}, Health: {data.playerHealth:F0}, Seed: {data.seed}).");
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
