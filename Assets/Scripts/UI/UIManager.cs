using System;
using System.Collections.Generic;
using Core;
using Player;
using World;
using Generation;
using Rendering;
using UnityEngine;

namespace UI
{
    /// <summary>
    /// Metadata helper for bilingual block display and inspector tooling.
    /// </summary>
    public struct BlockInspectorInfo
    {
        public string chineseName;
        public string englishName;
        public string icon;
        public string recommendedTool;
        public string dropText;

        public string displayName => LocalizationManager.Get(chineseName, englishName);
    }

    /// <summary>
    /// Animated on-screen floating text popup instance.
    /// </summary>
    public class FloatingTextPopup
    {
        public string text;
        public Color color;
        public Vector2 position;
        public float timeRemaining;
        public float totalDuration;
    }

    /// <summary>
    /// Master game state machine.
    /// Controls whether the player is at the Minecraft-style Main Menu, active in-game, or Paused.
    /// </summary>
    public enum GameState
    {
        MainMenu = 0,
        Playing = 1,
        Paused = 2
    }

    /// <summary>
    /// Autonomous real-time UI/UX Guidance and HUD system for the 2.5D sandbox game.
    /// Implemented via immediate-mode Unity OnGUI for 100% self-contained runtime operation
    /// without external Canvas prefabs or font dependencies.
    /// 
    /// Features:
    /// 1. Interactive Novice Tutorial Guide (新手引导任务系统) with animated checkmarks, sparkles & completion fanfare
    /// 2. Block Inspector Reticle Tooltip (目标方块信息悬浮提示) with hardness, tool, drop info & reach status
    /// 3. Floating Text Popup System (飘字系统) for pickups, quick save [F5] & quick load [F9]
    /// 4. Enhanced 9-slot Hotbar HUD with golden filigree, block pixel art icons, slot badges & item counts
    /// 5. Keybinds & Help Modal [H / F1] with categorized locomotion, mining/building, inventory & save/load guides
    /// 6. Integrated Quick Save [F5] and Quick Load [F9] world persistence
    /// 7. Minecraft-style Main Menu (Survival / Creative / Continue / Exit) and In-Game Pause Menu [ESC]
    /// </summary>
    [DisallowMultipleComponent]
    public class UIManager : MonoBehaviour
    {
        public static UIManager Instance { get; private set; }

        [Header("Game State & Flow")]
        [SerializeField] private GameState currentGameState = GameState.MainMenu;

        [Header("Display Settings")]
        [SerializeField] private bool showHUD = true;
        [SerializeField] private bool showHelpOverlay = false;
        [SerializeField] private bool showFPS = true;
        [SerializeField] private bool questBoxCollapsed = false;

        [Header("Localization Settings")]
        [Tooltip("Active language setting in Unity Editor / Inspector.")]
        [SerializeField] private GameLanguage activeLanguage = GameLanguage.Chinese;

        public GameState CurrentState
        {
            get => currentGameState;
            set => currentGameState = value;
        }

        [Header("Hotbar Configuration")]
        [SerializeField] private float slotSize = 54.0f;
        [SerializeField] private float slotPadding = 6.0f;
        [SerializeField] private float bottomOffset = 18.0f;

        [Header("Tutorial State")]
        [SerializeField] private int tutorialStep = 0; // 0: Locomotion, 1: Gathering, 2: Construction, 3: Illumination, 4: Sandbox
        [SerializeField] private int minedBlocksCount = 0;
        [SerializeField] private bool hasMoved = false;
        [SerializeField] private bool hasJumped = false;
        [SerializeField] private bool hasOpenedHelp = false;
        [SerializeField] private bool hasSavedWorld = false;

        // References
        private PlayerController2D5 _player;
        private BlockInteraction _interaction;
        private InventorySystem _inventory;
        private AudioSource _audioSource;
        private Texture2D _atlasTex;

        // Floating Text Popups
        private readonly List<FloatingTextPopup> _floatingPopups = new List<FloatingTextPopup>();

        // Quest celebration banner
        private float _questBannerTimer;
        private string _questBannerTitle = "";
        private string _questBannerSubtitle = "";

        // FPS tracking
        private float _fps;
        private float _fpsTimer;
        private int _fpsFrames;

        // Cached GUIStyles
        private GUIStyle _slotBoxStyle;
        private GUIStyle _activeSlotBoxStyle;
        private GUIStyle _slotNumberStyle;
        private GUIStyle _slotCountStyle;
        private GUIStyle _activeItemNameStyle;
        private GUIStyle _infoPanelStyle;
        private GUIStyle _questPanelStyle;
        private GUIStyle _helpPanelStyle;
        private GUIStyle _tooltipBoxStyle;
        private GUIStyle _titleStyle;
        private GUIStyle _bodyStyle;
        private GUIStyle _keyBadgeStyle;
        private GUIStyle _progressBarBgStyle;
        private GUIStyle _progressBarFillStyle;
        private GUIStyle _floatingTextStyle;
        private GUIStyle _tooltipTitleStyle;
        private GUIStyle _tooltipBodyStyle;
        private GUIStyle _bannerTitleStyle;
        private GUIStyle _bannerSubStyle;
        private GUIStyle _mcBtnStyle;
        private GUIStyle _menuTitleStyle;
        private GUIStyle _menuSubtitleStyle;
        private GUIStyle _splashStyle;

        // Save Slot Management State
        private bool _showWorldSelectModal = false;
        private int _creatingSlotIndex = -1;
        private string _newWorldName = "World 1";
        private GameMode _newWorldMode = GameMode.Survival;
        private string _newWorldSeedStr = "12345";
        private List<SaveMetadata> _cachedSlots;

        // Procedural Textures
        private Texture2D _panelTex;
        private Texture2D _questPanelTex;
        private Texture2D _slotTex;
        private Texture2D _activeSlotBorderTex;
        private Texture2D _tooltipTex;
        private Texture2D _whiteTex;
        private Texture2D _progressBgTex;
        private Texture2D _progressFillTex;
        private Texture2D _badgeTex;
        private Texture2D _bannerTex;
        private Texture2D _mcBtnNormalTex;
        private Texture2D _mcBtnHoverTex;
        private Texture2D _mcBtnActiveTex;
        private Texture2D _mcDirtTex;

        public int TutorialStep
        {
            get => tutorialStep;
            set => tutorialStep = value;
        }
        public int MinedCount => minedBlocksCount;

        /// <summary>
        /// Displays an on-screen toast notification banner.
        /// </summary>
        public static void ShowNotification(string message, float duration = 3.0f)
        {
            if (Instance != null)
            {
                Color col = new Color(1.0f, 0.90f, 0.35f);
                if (message.Contains("Saved") || message.Contains("存档成功"))
                {
                    col = new Color(0.35f, 0.95f, 0.65f); // Vibrant Emerald Green
                }
                else if (message.Contains("Loaded") || message.Contains("读取成功"))
                {
                    col = new Color(0.40f, 0.85f, 1.0f); // Bright Cyan
                }
                else if (message.Contains("Failed") || message.Contains("失败"))
                {
                    col = new Color(1.0f, 0.35f, 0.35f); // Soft Red
                }

                Instance.SpawnFloatingText(message, col);
            }
            else
            {
                Debug.Log($"[Notification] {message}");
            }
        }

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            EnsureAudioSource();
        }

        private void Start()
        {
            FindSceneReferences();
            GenerateProceduralTextures();

            // Cache atlas texture for crisp 16x16 pixel-art icons
            try
            {
                Material atlasMat = TextureAtlasGenerator.GetOrCreateAtlasMaterial();
                if (atlasMat != null)
                {
                    _atlasTex = atlasMat.mainTexture as Texture2D;
                }
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"[UIManager] Could not cache atlas texture: {ex.Message}");
            }

            // Sync activeLanguage with LocalizationManager
            activeLanguage = LocalizationManager.CurrentLanguage;

            // Ensure SaveManager listeners are attached
            if (SaveManager.Instance != null)
            {
                SaveManager.Instance.OnSaveCompleted -= HandleWorldSaveCompleted;
                SaveManager.Instance.OnSaveCompleted += HandleWorldSaveCompleted;
                SaveManager.Instance.OnLoadCompleted -= HandleWorldLoadCompleted;
                SaveManager.Instance.OnLoadCompleted += HandleWorldLoadCompleted;
            }
        }

        private void OnEnable()
        {
            BlockInteraction.OnBlockMined += HandleBlockMined;
            BlockInteraction.OnBlockPlaced += HandleBlockPlaced;
            DroppedItem.OnItemPickedUp += HandleItemPickedUp;
            LocalizationManager.OnLanguageChanged += HandleLanguageChanged;

            if (SaveManager.Instance != null)
            {
                SaveManager.Instance.OnSaveCompleted += HandleWorldSaveCompleted;
                SaveManager.Instance.OnLoadCompleted += HandleWorldLoadCompleted;
            }
        }

        private void OnDisable()
        {
            BlockInteraction.OnBlockMined -= HandleBlockMined;
            BlockInteraction.OnBlockPlaced -= HandleBlockPlaced;
            DroppedItem.OnItemPickedUp -= HandleItemPickedUp;
            LocalizationManager.OnLanguageChanged -= HandleLanguageChanged;

            if (SaveManager.Instance != null)
            {
                SaveManager.Instance.OnSaveCompleted -= HandleWorldSaveCompleted;
                SaveManager.Instance.OnLoadCompleted -= HandleWorldLoadCompleted;
            }
        }

        private void OnValidate()
        {
            if (Application.isPlaying)
            {
                if (LocalizationManager.CurrentLanguage != activeLanguage)
                {
                    LocalizationManager.SetLanguage(activeLanguage);
                }
            }
        }

        private void HandleLanguageChanged()
        {
            activeLanguage = LocalizationManager.CurrentLanguage;
            // Immediate floating notification on language switch
            string msg = LocalizationManager.Get("已切换为简体中文", "Language switched to English");
            SpawnFloatingText(msg, Color.cyan);
        }

        private void HandleWorldSaveCompleted()
        {
            PlaySaveChime();
            if (tutorialStep == 4)
            {
                hasSavedWorld = true;
                CheckStep5Completion();
            }
        }

        private void HandleWorldLoadCompleted()
        {
            PlayLoadChime();
        }

        private void Update()
        {
            float dt = Time.deltaTime;

            // F11 or Alt+Enter toggles fullscreen
            if (Input.GetKeyDown(KeyCode.F11) || ((Input.GetKey(KeyCode.LeftAlt) || Input.GetKey(KeyCode.RightAlt)) && Input.GetKeyDown(KeyCode.Return)))
            {
                Screen.fullScreen = !Screen.fullScreen;
            }

            // Toggle help guide with H or F1 (in-game only)
            if (currentGameState == GameState.Playing && (Input.GetKeyDown(KeyCode.H) || Input.GetKeyDown(KeyCode.F1)))
            {
                showHelpOverlay = !showHelpOverlay;
                if (showHelpOverlay && tutorialStep == 4)
                {
                    hasOpenedHelp = true;
                    CheckStep5Completion();
                }
            }

            // ESC toggles Pause Menu or closes modals
            if (Input.GetKeyDown(KeyCode.Escape))
            {
                if (_showWorldSelectModal)
                {
                    if (_creatingSlotIndex != -1)
                    {
                        _creatingSlotIndex = -1;
                    }
                    else
                    {
                        _showWorldSelectModal = false;
                    }
                }
                else if (showHelpOverlay)
                {
                    showHelpOverlay = false;
                }
                else if (currentGameState == GameState.Playing)
                {
                    currentGameState = GameState.Paused;
                }
                else if (currentGameState == GameState.Paused)
                {
                    currentGameState = GameState.Playing;
                }
            }

            // Quick Respawn with R (in-game only)
            if (currentGameState == GameState.Playing && Input.GetKeyDown(KeyCode.R))
            {
                if (_player != null)
                {
                    _player.Respawn();
                    SpawnFloatingText(LocalizationManager.Get("⚡ 已重生至地表安全区域", "⚡ Respawned safely at surface"), new Color(0.4f, 0.9f, 1.0f));
                    PlayTone(600f, 0.12f, 0.3f);
                }
            }

            // Locate references if unassigned
            if (_player == null || _interaction == null || _inventory == null)
            {
                FindSceneReferences();
            }

            // Tutorial Step 0: Locomotion Tracking
            if (currentGameState == GameState.Playing)
            {
                TrackLocomotionStep();
            }

            // Update floating popup timers and motion
            UpdateFloatingPopups(dt);

            // Update quest celebration banner timer
            if (_questBannerTimer > 0f)
            {
                _questBannerTimer -= dt;
            }

            // FPS calculation
            _fpsFrames++;
            _fpsTimer += Time.unscaledDeltaTime;
            if (_fpsTimer >= 0.5f)
            {
                _fps = _fpsFrames / _fpsTimer;
                _fpsFrames = 0;
                _fpsTimer = 0f;
            }
        }

        private void FindSceneReferences()
        {
            if (_player == null) _player = FindObjectOfType<PlayerController2D5>();
            if (_interaction == null) _interaction = FindObjectOfType<BlockInteraction>();
            if (_inventory == null) _inventory = InventorySystem.Instance ?? FindObjectOfType<InventorySystem>();
            EnsureAudioSource();
        }

        private void EnsureAudioSource()
        {
            if (_audioSource == null)
            {
                _audioSource = GetComponent<AudioSource>() ?? gameObject.AddComponent<AudioSource>();
                _audioSource.playOnAwake = false;
                _audioSource.spatialBlend = 0f; // 2D UI sound
            }
        }

        // =====================================================================
        // Tutorial State Machine & Event Listeners
        // =====================================================================

        private void TrackLocomotionStep()
        {
            if (tutorialStep != 0 || _player == null) return;

            if (Mathf.Abs(_player.MoveInput) > 0.1f || Mathf.Abs(_player.Velocity.x) > 0.4f)
            {
                hasMoved = true;
            }

            if (Input.GetButtonDown("Jump") || Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.W) || _player.Velocity.y > 4.5f)
            {
                hasJumped = true;
            }

            if (hasMoved && hasJumped)
            {
                CompleteStep(0, LocalizationManager.Get("基础移动", "Basic Locomotion"), LocalizationManager.Get("掌握了左右奔跑与跳跃技巧！", "Mastered running and jumping skills!"));
            }
        }

        private void HandleBlockMined(BlockType blockType, int x, int y)
        {
            if (tutorialStep == 1)
            {
                minedBlocksCount++;
                if (minedBlocksCount >= 5)
                {
                    CompleteStep(1, LocalizationManager.Get("资源采掘", "Gathering"), LocalizationManager.Get("成功开采 5 个方块，获得第一批资源！", "Mined 5 blocks and gathered materials!"));
                }
            }
        }

        private void HandleBlockPlaced(BlockType blockType, int x, int y)
        {
            if (tutorialStep == 2)
            {
                CompleteStep(2, LocalizationManager.Get("方块建筑", "Construction"), LocalizationManager.Get("成功放置方块，开启建造之路！", "Placed blocks to start construction!"));
            }
            else if (tutorialStep == 3)
            {
                if (blockType == BlockType.Torch || blockType == BlockType.GlowShroom)
                {
                    CompleteStep(3, LocalizationManager.Get("光明使者", "Illumination"), LocalizationManager.Get("点亮地底黑暗，驱散危险！", "Illuminated underground darkness!"));
                }
            }
        }

        private void HandleItemPickedUp(BlockType blockType, int count)
        {
            BlockInspectorInfo info = GetBlockInspectorInfo(blockType);
            Color rarityColor = GetBlockRarityColor(blockType);
            SpawnFloatingText($"+{count} {info.icon} {info.displayName}", rarityColor);
            PlayPopSound();
        }

        private void CheckStep5Completion()
        {
            if (tutorialStep == 4 && hasOpenedHelp && hasSavedWorld)
            {
                CompleteStep(4, LocalizationManager.Get("自由探索", "Open World Sandbox"), LocalizationManager.Get("新手向导全部达成！向地底深渊进发！", "Tutorial completed! Venture into the depths!"));
            }
        }

        private void CompleteStep(int stepIndex, string stepTitle, string stepSubtitle)
        {
            if (tutorialStep == stepIndex)
            {
                tutorialStep = stepIndex + 1;
                _questBannerTitle = $"⭐ {LocalizationManager.Get("任务达成", "Quest Completed")}: {stepTitle}!";
                _questBannerSubtitle = stepSubtitle;
                _questBannerTimer = 4.2f;

                SpawnFloatingText($"⭐ {LocalizationManager.Get("达成目标", "Objective Complete")}: {stepTitle}!", new Color(1.0f, 0.85f, 0.25f));
                PlayQuestFanfare();
            }
        }

        // =====================================================================
        // Persistence: Quick Save [F5] & Quick Load [F9]
        // =====================================================================

        public void QuickSave()
        {
            Vector3 playerPos = _player != null ? _player.Position : Vector3.zero;
            bool success = WorldSaveSystem.Save(playerPos, _inventory, tutorialStep, minedBlocksCount);
            int slotDisplay = WorldSaveSystem.CurrentSlotIndex + 1;
            if (success)
            {
                SpawnFloatingText(LocalizationManager.Get($"💾 槽位 {slotDisplay} 存档成功! [F5]", $"💾 Slot {slotDisplay} Saved! [F5]"), new Color(0.35f, 0.95f, 0.65f));
                PlaySaveChime();

                if (tutorialStep == 4)
                {
                    hasSavedWorld = true;
                    CheckStep5Completion();
                }
            }
            else
            {
                SpawnFloatingText(LocalizationManager.Get($"❌ 槽位 {slotDisplay} 存档失败", $"❌ Slot {slotDisplay} Save Failed"), new Color(1.0f, 0.35f, 0.35f));
            }
        }

        public void QuickLoad()
        {
            bool success = WorldSaveSystem.Load(out Vector3 loadedPos, _inventory, out int loadedStep, out int loadedMined);
            int slotDisplay = WorldSaveSystem.CurrentSlotIndex + 1;
            if (success)
            {
                if (_player != null && loadedPos != Vector3.zero)
                {
                    _player.Teleport(loadedPos);
                }
                tutorialStep = loadedStep;
                minedBlocksCount = loadedMined;

                SpawnFloatingText(LocalizationManager.Get($"📂 槽位 {slotDisplay} 读取成功! [F9]", $"📂 Slot {slotDisplay} Loaded! [F9]"), new Color(0.40f, 0.85f, 1.0f));
                PlayLoadChime();
            }
            else
            {
                SpawnFloatingText(LocalizationManager.Get($"⚠️ 槽位 {slotDisplay} 暂无存档 [F5 快速存档]", $"⚠️ Slot {slotDisplay} Empty [F5 to Save]"), new Color(1.0f, 0.70f, 0.30f));
            }
        }

        // =====================================================================
        // Floating Text Popups
        // =====================================================================

        public void SpawnFloatingText(string message, Color color)
        {
            float startX = Screen.width * 0.5f;
            float startY = Screen.height - 180f - (_floatingPopups.Count * 28f);

            _floatingPopups.Add(new FloatingTextPopup
            {
                text = message,
                color = color,
                position = new Vector2(startX, startY),
                timeRemaining = 2.4f,
                totalDuration = 2.4f
            });

            // Cap simultaneous popups
            if (_floatingPopups.Count > 8)
            {
                _floatingPopups.RemoveAt(0);
            }
        }

        private void UpdateFloatingPopups(float dt)
        {
            for (int i = _floatingPopups.Count - 1; i >= 0; i--)
            {
                FloatingTextPopup popup = _floatingPopups[i];
                popup.timeRemaining -= dt;
                popup.position.y -= 26f * dt; // Drift smoothly upwards

                if (popup.timeRemaining <= 0f)
                {
                    _floatingPopups.RemoveAt(i);
                }
            }
        }

        // =====================================================================
        // GUI Drawing Pipeline
        // =====================================================================

        private void OnGUI()
        {
            if (!showHUD) return;

            InitStyles();

            // Main Menu state renders the Minecraft-style Title Banner & Game Mode select buttons
            if (currentGameState == GameState.MainMenu)
            {
                if (_showWorldSelectModal)
                {
                    DrawWorldSelectModal();
                }
                else
                {
                    DrawMainMenu();
                }
                DrawFloatingTextPopups();
                return;
            }

            DrawTopLeftStatusPanel();
            DrawMiningProgressBar();
            DrawHotbarHUD();
            DrawPlayerHealthHUD();
            if (_player == null || _player.CurrentGameMode != GameMode.Survival)
            {
                DrawNoviceTutorialQuestBox();
            }
            DrawBlockInspectorTooltip();
            DrawFloatingTextPopups();
            DrawQuestBannerCelebration();

            if (showHelpOverlay)
            {
                DrawHelpModal();
            }

            // Pause Menu overlay when state is Paused
            if (currentGameState == GameState.Paused)
            {
                if (_showWorldSelectModal)
                {
                    DrawWorldSelectModal();
                }
                else
                {
                    DrawPauseMenu();
                }
            }
        }

        /// <summary>
        /// 1. Top-Left Status Panel: Coordinates, depth layer, biome badge, mode badge, FPS & quick keys.
        /// </summary>
        private void DrawTopLeftStatusPanel()
        {
            float panelWidth = 286f;
            float panelHeight = 132f;
            Rect panelRect = new Rect(16f, 16f, panelWidth, panelHeight);

            GUI.Box(panelRect, GUIContent.none, _infoPanelStyle);

            GUILayout.BeginArea(new Rect(panelRect.x + 12f, panelRect.y + 8f, panelWidth - 24f, panelHeight - 16f));

            // Title
            GUILayout.Label(LocalizationManager.Get("⚔️ <b>Luminora｜流明洛原</b>", "⚔️ <b>LUMINORA 2.5D SANDBOX</b>"), _titleStyle);

            // Biome & Geological Depth
            Vector3 playerPos = _player != null ? _player.Position : Vector3.zero;
            string biomeName = DetermineCurrentBiome(playerPos.x);
            string depthName = DetermineDepthZone(playerPos.y);

            GUILayout.Label($"<b>{LocalizationManager.Get("生态", "Biome")}:</b> <color=#55FF88>{biomeName}</color> | <b>{LocalizationManager.Get("地层", "Depth")}:</b> <color=#FFDD55>{depthName}</color>", _bodyStyle);
            GUILayout.Label($"<b>{LocalizationManager.Get("坐标", "Pos")}:</b> X: {playerPos.x:F1}, Y: {playerPos.y:F1}", _bodyStyle);

            // Current Game Mode badge
            GameMode curMode = _player != null ? _player.CurrentGameMode : GameMode.Survival;
            bool isFlying = _player != null && _player.IsFlying;
            string modeBadge = curMode.GetBadgeText(isFlying);
            string modeColor = (curMode == GameMode.Creative) ? (isFlying ? "#55FFFF" : "#FFDD55") : "#55FF88";
            GUILayout.Label($"<b>{LocalizationManager.Get("模式", "Mode")}:</b> <color={modeColor}><b>{modeBadge}</b></color>", _bodyStyle);

            // Hotkeys info
            string fpsText = showFPS ? $" | FPS: {_fps:F0}" : "";
            string hotkeyGuide = LocalizationManager.Get("[ESC] 菜单 | [F5] 存档 | [F9] 读档 | [H] 帮助", "[ESC] Menu | [F5] Save | [F9] Load | [H] Help");
            GUILayout.Label($"<color=#AAAAAA>{hotkeyGuide}{fpsText}</color>", _bodyStyle);

            GUILayout.EndArea();
        }

        /// <summary>
        /// 2. Interactive Novice Tutorial Guide (新手引导任务系统).
        /// Stylized Quest Box in the top-right corner with progress bars, checkmarks, and collapsible header.
        /// </summary>
        private void DrawNoviceTutorialQuestBox()
        {
            if (_player != null && _player.CurrentGameMode == GameMode.Survival) return;

            float questWidth = 330f;
            float questX = Screen.width - questWidth - 16f;
            float questY = 16f;

            if (questBoxCollapsed)
            {
                // Collapsed Compact Pill
                Rect collapsedRect = new Rect(questX, questY, questWidth, 34f);
                GUI.Box(collapsedRect, GUIContent.none, _questPanelStyle);

                string stageTag = tutorialStep >= 5 
                    ? LocalizationManager.Get("🎉 恭喜通关", "🎉 Completed") 
                    : $"{LocalizationManager.Get("阶段", "Stage")} {tutorialStep + 1}/5";
                string toggleText = $"📜 <b>{LocalizationManager.Get("新手任务向导", "Quest Guide")} [{stageTag}]</b> {LocalizationManager.Get("(点击展开)", "(Click to expand)")}";

                if (GUI.Button(collapsedRect, toggleText, _questPanelStyle))
                {
                    questBoxCollapsed = false;
                }
                return;
            }

            // Expanded Quest Box
            float questHeight = 192f;
            Rect questRect = new Rect(questX, questY, questWidth, questHeight);
            GUI.Box(questRect, GUIContent.none, _questPanelStyle);

            GUILayout.BeginArea(new Rect(questX + 12f, questY + 10f, questWidth - 24f, questHeight - 20f));

            // Header row with title, stage pill & collapse button
            GUILayout.BeginHorizontal();
            GUILayout.Label($"📜 <b>{LocalizationManager.Get("新手冒险任务指南", "Novice Adventure Guide")}</b>", _titleStyle);
            GUILayout.FlexibleSpace();

            string stepBadge = tutorialStep >= 5 
                ? LocalizationManager.Get("<color=#FFDD44>[已达成]</color>", "<color=#FFDD44>[Done]</color>") 
                : $"<color=#88DDFF>[{tutorialStep + 1}/5]</color>";
            GUILayout.Label(stepBadge, _bodyStyle);

            if (GUILayout.Button(LocalizationManager.Get("[折叠]", "[Hide]"), _keyBadgeStyle, GUILayout.Width(46f), GUILayout.Height(18f)))
            {
                questBoxCollapsed = true;
            }
            GUILayout.EndHorizontal();

            GUILayout.Space(6f);

            // Step Content
            switch (tutorialStep)
            {
                case 0:
                    // Step 1: Basic Locomotion
                    GUILayout.Label($"<b>{LocalizationManager.Get("目标 1: 基础移动", "Objective 1: Basic Locomotion")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>熟悉 2.5D 横版身法，掌握移动与可控跳跃。</color>", "<color=#CCCCCC>Learn 2.5D movement, running and jumping.</color>"), _bodyStyle);
                    GUILayout.Space(4f);

                    string moveCheck = hasMoved 
                        ? LocalizationManager.Get("<color=#55FF88>✔ [A / D] 键左右移动 (已完成)</color>", "<color=#55FF88>✔ [A / D] Move Left/Right (Done)</color>") 
                        : LocalizationManager.Get("<color=#FFAA33>○ [A / D] 键左右移动 (未完成)</color>", "<color=#FFAA33>○ [A / D] Move Left/Right</color>");
                    string jumpCheck = hasJumped 
                        ? LocalizationManager.Get("<color=#55FF88>✔ [Space] 空格键跳跃 (已完成)</color>", "<color=#55FF88>✔ [Space] Jump (Done)</color>") 
                        : LocalizationManager.Get("<color=#FFAA33>○ [Space] 空格键跳跃 (未完成)</color>", "<color=#FFAA33>○ [Space] Jump</color>");
                    GUILayout.Label(moveCheck, _bodyStyle);
                    GUILayout.Label(jumpCheck, _bodyStyle);
                    break;

                case 1:
                    // Step 2: Gathering
                    GUILayout.Label($"<b>{LocalizationManager.Get("目标 2: 资源采掘", "Objective 2: Gathering")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>使用 [鼠标左键] 持续破坏挖掘 5 个方块。</color>", "<color=#CCCCCC>Hold [Left Click] to mine 5 blocks.</color>"), _bodyStyle);
                    GUILayout.Space(4f);

                    GUILayout.Label($"<b>{LocalizationManager.Get("开采进度", "Mining Progress")}:</b> <color=#55FF88>{minedBlocksCount} / 5</color>", _bodyStyle);

                    // Mini Progress Bar
                    float pWidth = questWidth - 24f;
                    Rect pBarBg = GUILayoutUtility.GetRect(pWidth, 14f);
                    GUI.Box(pBarBg, GUIContent.none, _progressBarBgStyle);
                    float fillFrac = Mathf.Clamp01((float)minedBlocksCount / 5f);
                    Rect pBarFill = new Rect(pBarBg.x + 2f, pBarBg.y + 2f, (pBarBg.width - 4f) * fillFrac, pBarBg.height - 4f);
                    GUI.Box(pBarFill, GUIContent.none, _progressBarFillStyle);
                    break;

                case 2:
                    // Step 3: Construction
                    GUILayout.Label($"<b>{LocalizationManager.Get("目标 3: 方块建筑", "Objective 3: Construction")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>按 [1-9] 切换快捷栏方块，[鼠标右键] 放置建造。</color>", "<color=#CCCCCC>Press [1-9] for hotbar, [Right Click] to place blocks.</color>"), _bodyStyle);
                    GUILayout.Space(4f);
                    GUILayout.Label(LocalizationManager.Get("<color=#FFAA33>○ 鼠标右键放置任意方块 (与周围方块相邻)</color>", "<color=#FFAA33>○ Right Click to place block adjacent to existing</color>"), _bodyStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#888888>💡 提示: 方块必须与已有方块或背景墙贴合放置。</color>", "<color=#888888>💡 Hint: Blocks must be placed against existing blocks or walls.</color>"), _bodyStyle);
                    break;

                case 3:
                    // Step 4: Illumination
                    GUILayout.Label($"<b>{LocalizationManager.Get("目标 4: 光明使者", "Objective 4: Illumination")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>手持火把或发光蘑菇，插在地底照亮黑暗。</color>", "<color=#CCCCCC>Hold torches or glowing mushrooms to illuminate underground.</color>"), _bodyStyle);
                    GUILayout.Space(4f);
                    GUILayout.Label(LocalizationManager.Get("<color=#FFAA33>○ 按快捷键 [5] 选中火把，右键插入地表或矿洞</color>", "<color=#FFAA33>○ Select torch [5] and Right Click to place</color>"), _bodyStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#888888>💡 提示: 光源可驱散地底迷雾，保护视线！</color>", "<color=#888888>💡 Hint: Light sources dispel subterranean darkness!</color>"), _bodyStyle);
                    break;

                case 4:
                    // Step 5: Open World Sandbox
                    GUILayout.Label($"<b>{LocalizationManager.Get("目标 5: 自由探索", "Objective 5: Open World Sandbox")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>按 [H] 查看全键位指南，按 [F5] 快速存档！</color>", "<color=#CCCCCC>Press [H] for keybinds guide, [F5] for quick save!</color>"), _bodyStyle);
                    GUILayout.Space(4f);

                    string helpCheck = hasOpenedHelp 
                        ? LocalizationManager.Get("<color=#55FF88>✔ 按 [H] 打开全键位指南 (已完成)</color>", "<color=#55FF88>✔ Press [H] Open Guide (Done)</color>") 
                        : LocalizationManager.Get("<color=#FFAA33>○ 按 [H] 打开全键位指南 (未完成)</color>", "<color=#FFAA33>○ Press [H] Open Guide</color>");
                    string saveCheck = hasSavedWorld 
                        ? LocalizationManager.Get("<color=#55FF88>✔ 按 [F5] 进行快速存档 (已完成)</color>", "<color=#55FF88>✔ Press [F5] Quick Save (Done)</color>") 
                        : LocalizationManager.Get("<color=#FFAA33>○ 按 [F5] 进行快速存档 (未完成)</color>", "<color=#FFAA33>○ Press [F5] Quick Save</color>");
                    GUILayout.Label(helpCheck, _bodyStyle);
                    GUILayout.Label(saveCheck, _bodyStyle);
                    break;

                default:
                    // Completed All
                    GUILayout.Label($"<b>{LocalizationManager.Get("🎉 新手向导已全部完成！", "🎉 Tutorial Completed!")}</b>", _activeItemNameStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#FFEEAA>你已掌握流明洛原核心法则！</color>", "<color=#FFEEAA>You have mastered Luminora essentials!</color>"), _bodyStyle);
                    GUILayout.Space(4f);
                    GUILayout.Label(LocalizationManager.Get("<color=#CCCCCC>探索沙漠、雪原与地下熔岩深渊！</color>", "<color=#CCCCCC>Explore deserts, snow peaks, and underworld caverns!</color>"), _bodyStyle);
                    GUILayout.Label(LocalizationManager.Get("<color=#88DDFF>[F5] 随时快捷存档 | [F9] 随时快捷读取</color>", "<color=#88DDFF>[F5] Quick Save anytime | [F9] Quick Load</color>"), _bodyStyle);
                    break;
            }

            GUILayout.EndArea();
        }

        /// <summary>
        /// 3. Block Inspector Reticle Tooltip (目标方块信息悬浮提示).
        /// When crosshair/mouse hovers over a block, displays block name, hardness, recommended tool, drop & reach status.
        /// </summary>
        private void DrawBlockInspectorTooltip()
        {
            if (_interaction == null) return;
            BlockType hovered = _interaction.HoveredBlockType;
            if (hovered == BlockType.Air) return;

            BlockData data = BlockRegistry.GetBlock(hovered);
            BlockInspectorInfo info = GetBlockInspectorInfo(hovered);

            Vector2 mousePos = Event.current.mousePosition;
            float tooltipW = 236f;
            float tooltipH = 96f;

            float tooltipX = mousePos.x + 18f;
            float tooltipY = mousePos.y + 12f;

            // Screen boundary clamping
            if (tooltipX + tooltipW > Screen.width - 12f)
            {
                tooltipX = mousePos.x - tooltipW - 14f;
            }
            if (tooltipY + tooltipH > Screen.height - 12f)
            {
                tooltipY = mousePos.y - tooltipH - 12f;
            }

            Rect tooltipRect = new Rect(tooltipX, tooltipY, tooltipW, tooltipH);
            GUI.Box(tooltipRect, GUIContent.none, _tooltipBoxStyle);

            // Left: 32x32 pixel-art block preview
            Rect iconRect = new Rect(tooltipRect.x + 10f, tooltipRect.y + 12f, 32f, 32f);
            DrawBlockIcon(iconRect, hovered);
            DrawOutline(iconRect, 1, new Color(1.0f, 0.85f, 0.35f, 0.6f));

            // Right: Block Metadata
            float textX = tooltipRect.x + 48f;
            float textW = tooltipW - 56f;

            // Row 1: Block Title
            Rect titleRect = new Rect(textX, tooltipRect.y + 7f, textW, 20f);
            GUI.Label(titleRect, $"<b>{info.icon} {info.displayName}</b>", _tooltipTitleStyle);

            // Row 2: Hardness & Tool
            string hardnessStr = data.hardness < 0f 
                ? LocalizationManager.Get("硬度: 坚不可摧", "Hardness: Unbreakable") 
                : $"{LocalizationManager.Get("硬度", "Hardness")}: {data.hardness:F1}";
            Rect row2Rect = new Rect(textX, tooltipRect.y + 27f, textW, 18f);
            GUI.Label(row2Rect, $"<color=#E0E0E0>{hardnessStr}</color> | <color=#88DDFF>{info.recommendedTool}</color>", _tooltipBodyStyle);

            // Row 3: Drop loot
            Rect row3Rect = new Rect(textX, tooltipRect.y + 45f, textW, 18f);
            GUI.Label(row3Rect, $"<color=#AAAAAA>{LocalizationManager.Get("掉落: ", "Drop: ")}</color><color=#FFDD55>{info.dropText}</color>", _tooltipBodyStyle);

            // Row 4: Reach status
            Rect row4Rect = new Rect(textX, tooltipRect.y + 64f, textW, 18f);
            if (_interaction.IsTargetInReach)
            {
                GUI.Label(row4Rect, LocalizationManager.Get("<color=#55FF88>✔ 射程内 (可挖掘/交互)</color>", "<color=#55FF88>✔ In Reach (Mine/Interact)</color>"), _tooltipBodyStyle);
            }
            else
            {
                GUI.Label(row4Rect, LocalizationManager.Get("<color=#FF6666>✖ 超出范围 (请靠近)</color>", "<color=#FF6666>✖ Out of Range (Get Closer)</color>"), _tooltipBodyStyle);
            }
        }

        /// <summary>
        /// 4. Enhanced 9-slot Hotbar HUD:
        /// Centered along bottom of the screen with golden filigree, block pixel art, badges and counts.
        /// </summary>
        private void DrawHotbarHUD()
        {
            if (_inventory == null) return;

            int slotCount = InventorySystem.HotbarSlotCount;
            float totalWidth = slotCount * slotSize + (slotCount - 1) * slotPadding;
            float startX = (Screen.width - totalWidth) * 0.5f;
            float startY = Screen.height - slotSize - bottomOffset;

            // Active item title displayed boldly directly above hotbar
            InventorySlot activeSlot = _inventory.GetActiveItem();
            if (activeSlot != null && !activeSlot.IsEmpty)
            {
                string activeItemText;
                if (activeSlot.isTool)
                {
                    ToolData td = ToolRegistry.Get(activeSlot.toolId);
                    activeItemText = $"[{_inventory.ActiveSlotIndex + 1}] ⚔️ <b>{td.displayName}</b> | {LocalizationManager.Get("伤害", "DMG")}: +{td.damage:F0} | {LocalizationManager.Get("击退", "Knockback")}: {td.knockbackMultiplier:F1}x";
                }
                else
                {
                    BlockInspectorInfo activeInfo = GetBlockInspectorInfo(activeSlot.type);
                    activeItemText = $"[{_inventory.ActiveSlotIndex + 1}] {activeInfo.icon} <b>{activeInfo.displayName}</b> × {activeSlot.count}";
                }

                Rect titleRect = new Rect(startX, startY - 28f, totalWidth, 24f);
                GUI.Label(titleRect, activeItemText, _activeItemNameStyle);
            }

            // Creative Mode flight status indicator badge
            if (_player != null && _player.CurrentGameMode == GameMode.Creative)
            {
                float badgeW = 440f;
                float badgeH = 24f;
                float badgeX = (Screen.width - badgeW) * 0.5f;
                float badgeY = (activeSlot != null && !activeSlot.IsEmpty) ? startY - 56f : startY - 30f;
                Rect badgeRect = new Rect(badgeX, badgeY, badgeW, badgeH);

                GUI.Box(badgeRect, GUIContent.none, _infoPanelStyle);
                string badgeText = _player.IsFlying
                    ? LocalizationManager.Get("⚡ <color=#55FFFF><b>[飞行模式已激活]</b> 空格上升 | Shift/S下降 | 双击空格降落</color>",
                                              "⚡ <color=#55FFFF><b>[FLYING ACTIVATED]</b> Space Up | Shift/S Down | 2x Space Land</color>")
                    : LocalizationManager.Get("✨ <color=#FFDD55><b>[创造模式]</b> 双击空格开启飞行 | 无限方块 | 秒破方块</color>",
                                              "✨ <color=#FFDD55><b>[CREATIVE MODE]</b> Double-tap Space Fly | Infinite Blocks | Instant Break</color>");
                GUI.Label(badgeRect, badgeText, _floatingTextStyle);
            }

            // Draw 9 Slots
            for (int i = 0; i < slotCount; i++)
            {
                float slotX = startX + i * (slotSize + slotPadding);
                Rect slotRect = new Rect(slotX, startY, slotSize, slotSize);
                bool isActive = (i == _inventory.ActiveSlotIndex);

                // Slot background
                GUIStyle boxStyle = isActive ? _activeSlotBoxStyle : _slotBoxStyle;
                GUI.Box(slotRect, GUIContent.none, boxStyle);

                // Mouse click to select slot
                if (Event.current.type == EventType.MouseDown && slotRect.Contains(Event.current.mousePosition))
                {
                    _inventory.SetActiveSlot(i);
                    PlayTone(520f + i * 40f, 0.06f, 0.25f);
                    Event.current.Use();
                }

                // Slot number badge in top-left
                Rect badgeRect = new Rect(slotRect.x + 3f, slotRect.y + 3f, 16f, 14f);
                DrawSolidRect(badgeRect, new Color(0.06f, 0.08f, 0.12f, 0.85f));
                GUI.Label(badgeRect, (i + 1).ToString(), _slotNumberStyle);

                // Icon preview
                InventorySlot slot = _inventory.GetSlot(i);
                if (slot != null && !slot.IsEmpty)
                {
                    float iconSize = slotSize * 0.56f;
                    Rect iconRect = new Rect(
                        slotRect.x + (slotSize - iconSize) * 0.5f,
                        slotRect.y + (slotSize - iconSize) * 0.5f,
                        iconSize,
                        iconSize
                    );

                    if (slot.isTool)
                    {
                        ToolData td = ToolRegistry.Get(slot.toolId);
                        if (td != null)
                        {
                            DrawSolidRect(iconRect, td.tierColor);
                            DrawOutline(iconRect, 1, new Color(0f, 0f, 0f, 0.7f));

                            string tierAbbr = td.tier.ToString().ToUpper();
                            if (tierAbbr.Length > 4) tierAbbr = tierAbbr.Substring(0, 4);

                            Rect countRect = new Rect(slotRect.x + 2f, slotRect.y + slotSize - 18f, slotSize - 4f, 16f);
                            _slotCountStyle.normal.textColor = td.tierColor;
                            GUI.Label(countRect, tierAbbr, _slotCountStyle);
                        }
                    }
                    else
                    {
                        BlockData blockData = BlockRegistry.GetBlock(slot.type);
                        DrawBlockIcon(iconRect, slot.type);

                        // Emissive glow for light emitters
                        if (blockData.emitLight)
                        {
                            float pulseAlpha = 0.5f + Mathf.Sin(Time.time * 6f) * 0.25f;
                            DrawOutline(iconRect, 1, new Color(blockData.lightColor.r, blockData.lightColor.g, blockData.lightColor.b, pulseAlpha));
                        }
                        else
                        {
                            DrawOutline(iconRect, 1, new Color(0f, 0f, 0f, 0.4f));
                        }

                        // Stack Count in bottom-right with drop shadow
                        Rect countRect = new Rect(slotRect.x + 4f, slotRect.y + slotSize - 18f, slotSize - 8f, 16f);
                        Rect shadowRect = new Rect(countRect.x + 1f, countRect.y + 1f, countRect.width, countRect.height);

                        _slotCountStyle.normal.textColor = new Color(0f, 0f, 0f, 0.9f);
                        GUI.Label(shadowRect, slot.count.ToString(), _slotCountStyle);

                        _slotCountStyle.normal.textColor = Color.white;
                        GUI.Label(countRect, slot.count.ToString(), _slotCountStyle);
                    }
                }
            }
        }

        /// <summary>
        /// Luminora-style Player Health HUD for Survival Mode:
        /// 10 animated heart icons, smooth status bar with HP numbers, low-health danger pulse,
        /// and death respawn banner.
        /// </summary>
        private void DrawPlayerHealthHUD()
        {
            if (_player == null || _player.CurrentGameMode != GameMode.Survival) return;

            float health = _player.CurrentHealth;
            float maxHealth = _player.MaxHealth > 0f ? _player.MaxHealth : 100f;
            bool isDead = _player.IsDead;

            // Centered above the hotbar
            float totalHotbarWidth = InventorySystem.HotbarSlotCount * slotSize + (InventorySystem.HotbarSlotCount - 1) * slotPadding;
            float barW = 260f;
            float barH = 18f;
            float startX = (Screen.width - barW) * 0.5f;

            InventorySlot activeSlot = _inventory != null ? _inventory.GetActiveItem() : null;
            float baseOffset = (activeSlot != null && !activeSlot.IsEmpty) ? 60f : 32f;
            float startY = Screen.height - slotSize - bottomOffset - baseOffset;

            // Death overlay banner
            if (isDead)
            {
                float deathW = 440f;
                float deathH = 54f;
                float deathX = (Screen.width - deathW) * 0.5f;
                float deathY = startY - 24f;
                Rect deathRect = new Rect(deathX, deathY, deathW, deathH);

                DrawSolidRect(deathRect, new Color(0.12f, 0.02f, 0.02f, 0.92f));
                DrawOutline(deathRect, 2, new Color(1.0f, 0.25f, 0.25f, 0.95f));

                GUIStyle dTitleStyle = new GUIStyle(_titleStyle)
                {
                    fontSize = 18,
                    alignment = TextAnchor.MiddleCenter,
                    normal = { textColor = new Color(1.0f, 0.25f, 0.25f) }
                };
                GUI.Label(new Rect(deathX, deathY + 6f, deathW, 20f), LocalizationManager.Get("💀 你死掉了！(YOU DIED)", "💀 YOU DIED!"), dTitleStyle);

                GUIStyle dSubStyle = new GUIStyle(_bodyStyle)
                {
                    alignment = TextAnchor.MiddleCenter,
                    normal = { textColor = new Color(1.0f, 0.88f, 0.40f) }
                };
                GUI.Label(new Rect(deathX, deathY + 28f, deathW, 18f), LocalizationManager.Get("按 [R] 键在地表安全重生并恢复生命", "Press [R] to Respawn safely on Surface"), dSubStyle);
                return;
            }

            // Health Bar Background
            Rect bgRect = new Rect(startX, startY, barW, barH);
            DrawSolidRect(bgRect, new Color(0.08f, 0.09f, 0.12f, 0.88f));

            // Low health pulsing red border
            bool isLowHealth = (health <= 25f);
            Color borderColor = isLowHealth 
                ? new Color(1.0f, 0.2f, 0.2f, 0.6f + Mathf.Sin(Time.time * 10f) * 0.4f) 
                : new Color(0.35f, 0.40f, 0.50f, 0.8f);
            DrawOutline(bgRect, 1, borderColor);

            // Health bar fill
            float fillFrac = Mathf.Clamp01(health / maxHealth);
            float fillW = (barW - 4f) * fillFrac;
            Rect fillRect = new Rect(startX + 2f, startY + 2f, fillW, barH - 4f);

            Color fillColor = (health > 50f)
                ? new Color(0.25f, 0.85f, 0.35f, 0.95f)
                : (health > 25f ? new Color(1.0f, 0.75f, 0.20f, 0.95f) : new Color(1.0f, 0.25f, 0.25f, 0.95f));
            DrawSolidRect(fillRect, fillColor);

            // 10 Luminora Heart Icons ❤️ directly above the bar
            int totalHearts = Mathf.Max(1, Mathf.RoundToInt(maxHealth / 10f));
            float heartSize = 16f;
            float heartGap = 2f;
            float heartsTotalW = totalHearts * (heartSize + heartGap);
            float heartsStartX = (Screen.width - heartsTotalW) * 0.5f;
            float heartsY = startY - 18f;

            GUIStyle heartStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                alignment = TextAnchor.MiddleCenter
            };

            for (int h = 0; h < totalHearts; h++)
            {
                float hHealth = (h + 1) * 10f;
                string heartIcon;
                if (health >= hHealth)
                {
                    heartIcon = "<color=#FF3344>❤️</color>";
                }
                else if (health >= hHealth - 5f)
                {
                    heartIcon = "<color=#FFAA33>💔</color>";
                }
                else
                {
                    heartIcon = "<color=#555555>🖤</color>";
                }

                Rect hRect = new Rect(heartsStartX + h * (heartSize + heartGap), heartsY, heartSize + 4f, heartSize + 4f);
                GUI.Label(hRect, heartIcon, heartStyle);
            }

            // Numerical HP text overlay inside bar
            GUIStyle hpTextStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 13,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = Color.white }
            };
            string hpString;
            if (health >= maxHealth)
            {
                hpString = $"HP: {Mathf.CeilToInt(health)} / {Mathf.CeilToInt(maxHealth)}";
            }
            else if (_player != null && _player.TimeSinceCombat >= 20f)
            {
                hpString = $"HP: {Mathf.CeilToInt(health)} / {Mathf.CeilToInt(maxHealth)} [Regen +3.5/s]";
            }
            else
            {
                hpString = $"HP: {Mathf.CeilToInt(health)} / {Mathf.CeilToInt(maxHealth)}";
            }
            GUI.Label(bgRect, hpString, hpTextStyle);
        }

        /// <summary>
        /// 5. Floating Text Popups (飘字系统).
        /// </summary>
        private void DrawFloatingTextPopups()
        {
            if (_floatingPopups.Count == 0) return;

            for (int i = 0; i < _floatingPopups.Count; i++)
            {
                FloatingTextPopup popup = _floatingPopups[i];
                float progress = popup.timeRemaining / popup.totalDuration;
                float alpha = Mathf.Clamp01(progress * 2.2f);

                Color textColor = popup.color;
                textColor.a = alpha;

                GUIContent content = new GUIContent(popup.text);
                Vector2 size = _floatingTextStyle.CalcSize(content);

                Rect textRect = new Rect(
                    popup.position.x - size.x * 0.5f,
                    popup.position.y - size.y * 0.5f,
                    size.x + 18f,
                    size.y + 8f
                );

                // Background pill
                DrawSolidRect(textRect, new Color(0.08f, 0.09f, 0.12f, 0.85f * alpha));
                DrawOutline(textRect, 1, new Color(textColor.r, textColor.g, textColor.b, 0.65f * alpha));

                // Drop shadow
                _floatingTextStyle.normal.textColor = new Color(0f, 0f, 0f, 0.9f * alpha);
                GUI.Label(new Rect(textRect.x + 1f, textRect.y + 1f, textRect.width, textRect.height), popup.text, _floatingTextStyle);

                // Main text
                _floatingTextStyle.normal.textColor = textColor;
                GUI.Label(textRect, popup.text, _floatingTextStyle);
            }
        }

        /// <summary>
        /// 6. Quest Banner Celebration Popup when achieving mission stages.
        /// </summary>
        private void DrawQuestBannerCelebration()
        {
            if (_questBannerTimer <= 0f) return;

            float bannerW = 460f;
            float bannerH = 56f;
            float bannerX = (Screen.width - bannerW) * 0.5f;
            float bannerY = 80f;

            float alpha = Mathf.Clamp01(_questBannerTimer * 2f);
            Color prevColor = GUI.color;
            GUI.color = new Color(1f, 1f, 1f, alpha);

            Rect bannerRect = new Rect(bannerX, bannerY, bannerW, bannerH);
            GUI.Box(bannerRect, GUIContent.none, _infoPanelStyle);
            DrawOutline(bannerRect, 2, new Color(1.0f, 0.85f, 0.25f, 0.9f * alpha));

            Rect titleRect = new Rect(bannerX, bannerY + 6f, bannerW, 22f);
            GUI.Label(titleRect, _questBannerTitle, _bannerTitleStyle);

            Rect subRect = new Rect(bannerX, bannerY + 28f, bannerW, 20f);
            GUI.Label(subRect, _questBannerSubtitle, _bannerSubStyle);

            GUI.color = prevColor;
        }

        /// <summary>
        /// 7. Dynamic Mining Progress Bar while holding left click.
        /// </summary>
        private void DrawMiningProgressBar()
        {
            if (_interaction == null || _interaction.MiningProgress <= 0.01f) return;

            float barWidth = 260f;
            float barHeight = 20f;
            float barX = (Screen.width - barWidth) * 0.5f;
            float barY = Screen.height - 118f;

            Rect bgRect = new Rect(barX, barY, barWidth, barHeight);
            GUI.Box(bgRect, GUIContent.none, _progressBarBgStyle);

            float fillWidth = (barWidth - 4f) * Mathf.Clamp01(_interaction.MiningProgress);
            Rect fillRect = new Rect(barX + 2f, barY + 2f, fillWidth, barHeight - 4f);
            GUI.Box(fillRect, GUIContent.none, _progressBarFillStyle);

            // Progress text
            Rect textRect = new Rect(barX, barY - 18f, barWidth, 18f);
            GUIStyle labelStyle = new GUIStyle(GUI.skin.label)
            {
                alignment = TextAnchor.MiddleCenter,
                fontStyle = FontStyle.Bold,
                fontSize = 12
            };
            labelStyle.normal.textColor = Color.white;
            GUI.Label(textRect, $"{LocalizationManager.Get("采掘中", "Mining")}... {(_interaction.MiningProgress * 100f):F0}%", labelStyle);
        }

        /// <summary>
        /// 8. Keybinds & Help Modal [H / F1].
        /// Beautifully organized into Locomotion, Mining & Building, Inventory, and Save/Load categories.
        /// </summary>
        private void DrawHelpModal()
        {
            // Dim background
            DrawSolidRect(new Rect(0, 0, Screen.width, Screen.height), new Color(0f, 0f, 0f, 0.72f));

            float modalWidth = 660f;
            float modalHeight = 520f;
            float modalX = (Screen.width - modalWidth) * 0.5f;
            float modalY = (Screen.height - modalHeight) * 0.5f;

            Rect modalRect = new Rect(modalX, modalY, modalWidth, modalHeight);
            GUI.Box(modalRect, GUIContent.none, _helpPanelStyle);

            GUILayout.BeginArea(new Rect(modalX + 24f, modalY + 20f, modalWidth - 48f, modalHeight - 40f));

            GUILayout.Label(LocalizationManager.Get("<size=16><b>⚔️ 全键位与游戏玩法指南 ⚔️</b></size>", "<size=16><b>⚔️ CONTROLS & GAMEPLAY GUIDE ⚔️</b></size>"), _titleStyle);
            GUILayout.Label(LocalizationManager.Get("<color=#AAAAAA>深入沙盒旷野，自由开采矿石、建造庇护所与探索无尽地底</color>", "<color=#AAAAAA>Explore the sandbox, mine ores, build shelters, and venture into the depths.</color>"), _bodyStyle);
            GUILayout.Space(10f);

            // Category 1: Locomotion
            GUILayout.Label($"<b>{LocalizationManager.Get("🏃 角色身法与移动", "🏃 Locomotion & Movement")}</b>", _titleStyle);
            DrawControlRow(LocalizationManager.Get("[A] / [D] 或 [←] / [→]", "[A] / [D] or [←] / [→]"), LocalizationManager.Get("左右平滑移动", "Smooth horizontal movement"));
            DrawControlRow("[Left Shift]", LocalizationManager.Get("疾跑冲刺加速", "Sprint Boost"));
            DrawControlRow(LocalizationManager.Get("[Space] 或 [W]", "[Space] or [W]"), LocalizationManager.Get("动态跳跃 (轻按小跳，长按大跳)", "Dynamic Jump (Tap short, hold high)"));
            DrawControlRow(LocalizationManager.Get("[自动爬坡]", "[Auto Step]"), LocalizationManager.Get("遇到 1 格高方块自动平滑跨越，无需跳跃", "Automatically steps over 1-block obstacles"));
            DrawControlRow(LocalizationManager.Get("[R 键]", "[R Key]"), LocalizationManager.Get("紧急回城 / 地表安全重生", "Safe surface respawn"));
            GUILayout.Space(8f);

            // Category 2: Mining & Building
            GUILayout.Label($"<b>{LocalizationManager.Get("⛏️ 采掘与建筑放置", "⛏️ Mining & Building")}</b>", _titleStyle);
            DrawControlRow(LocalizationManager.Get("[鼠标左键 (长按)]", "[Left Click (Hold)]"), LocalizationManager.Get("采掘破坏光标处方块 (有效距离 6 格)", "Mine target block (Reach: 6 blocks)"));
            DrawControlRow(LocalizationManager.Get("[鼠标右键 (点击)]", "[Right Click (Tap)]"), LocalizationManager.Get("放置选中的方块或火把 (需与已有方块相邻)", "Place block or torch adjacent to existing"));
            DrawControlRow(LocalizationManager.Get("[目标悬浮检视]", "[Target Inspector]"), LocalizationManager.Get("鼠标悬浮任意方块查看名称、硬度与掉落信息", "Hover crosshair over any block for stats & drops"));
            GUILayout.Space(8f);

            // Category 3: Inventory
            GUILayout.Label($"<b>{LocalizationManager.Get("🎒 物品栏与装备", "🎒 Inventory & Equipment")}</b>", _titleStyle);
            DrawControlRow(LocalizationManager.Get("[1] - [9] 快捷键", "[1] - [9] Keys"), LocalizationManager.Get("快速切换底部 9 格快捷栏道具", "Select hotbar slots 1-9"));
            DrawControlRow(LocalizationManager.Get("[鼠标滚轮]", "[Mouse Scroll]"), LocalizationManager.Get("顺畅循环滚选快捷槽", "Cycle through hotbar slots"));
            DrawControlRow(LocalizationManager.Get("[鼠标点击快捷栏]", "[Hotbar Click]"), LocalizationManager.Get("直接点选手持对应方块道具", "Equip clicked hotbar item directly"));
            DrawControlRow(LocalizationManager.Get("[C 键]", "[C Key]"), LocalizationManager.Get("🔨 打开/关闭合成工作台", "🔨 Toggle Crafting Station"));
            DrawControlRow(LocalizationManager.Get("[E 键]", "[E Key]"), LocalizationManager.Get("🌟 打开/关闭创造模式大背包/全量物品库", "🌟 Toggle Creative Big Inventory"));
            GUILayout.Space(8f);

            // Category 4: Save & Load
            GUILayout.Label($"<b>{LocalizationManager.Get("💾 存档与快捷功能", "💾 Save, Load & System")}</b>", _titleStyle);
            DrawControlRow(LocalizationManager.Get("[F5 键]", "[F5 Key]"), LocalizationManager.Get("⚡ 实时快速存档 (保存世界所有方块改动与背包数据)", "⚡ Quick Save world modifications and inventory"));
            DrawControlRow(LocalizationManager.Get("[F9 键]", "[F9 Key]"), LocalizationManager.Get("🔄 快速读取存档 (即刻恢复上次保存的世界与角色进度)", "🔄 Quick Load saved world and progress"));
            DrawControlRow(LocalizationManager.Get("[H] 或 [F1]", "[H] or [F1]"), LocalizationManager.Get("随时开启或关闭此指南面板", "Toggle this keybinds & help guide"));
            GUILayout.Space(8f);

            // Category 5: Modes & Flight
            GUILayout.Label($"<b>{LocalizationManager.Get("✨ 游戏模式与创造飞行", "✨ Modes & Flight")}</b>", _titleStyle);
            DrawControlRow(LocalizationManager.Get("[ESC 键]", "[ESC Key]"), LocalizationManager.Get("呼出游戏暂停菜单 / 自由切换生存与创造模式", "Pause Menu / Toggle Survival & Creative"));
            DrawControlRow(LocalizationManager.Get("[双击空格]", "[Double-tap Space]"), LocalizationManager.Get("创造模式下开启/关闭飞行", "Toggle Creative Flight mode"));
            DrawControlRow(LocalizationManager.Get("[Space] / [Shift 或 S]", "[Space] / [Shift or S]"), LocalizationManager.Get("飞行中上升 (+Y) 与下降 (-Y) / 1.5倍水平加速", "Ascend / Descend while flying / 1.5x speed"));

            GUILayout.Space(14f);

            if (GUILayout.Button(LocalizationManager.Get("<b>关闭操作指南 [H]</b>", "<b>Close Guide [H]</b>"), GUILayout.Height(34f)))
            {
                showHelpOverlay = false;
            }

            GUILayout.EndArea();
        }

        /// <summary>
        /// Minecraft-style Main Menu UI when in MainMenu state:
        /// Title banner, [SURVIVAL MODE (生存模式)], [CREATIVE MODE (创造模式)], [SAVE SLOTS (存档管理)], [EXIT (退出)].
        /// </summary>
        private void DrawMainMenu()
        {
            // Fullscreen Dirt/Stone textured dark backdrop
            Rect screenRect = new Rect(0, 0, Screen.width, Screen.height);
            if (_mcDirtTex != null)
            {
                GUI.DrawTexture(screenRect, _mcDirtTex, ScaleMode.StretchToFill);
            }
            DrawSolidRect(screenRect, new Color(0.04f, 0.05f, 0.08f, 0.82f));

            // Title Banner Box
            float bannerW = 640f;
            float bannerH = 110f;
            float bannerX = (Screen.width - bannerW) * 0.5f;
            float bannerY = Mathf.Max(26f, Screen.height * 0.12f);

            // Title drop shadow
            GUI.Label(new Rect(bannerX + 2f, bannerY + 2f, bannerW, 46f), "LUMINORA", new GUIStyle(_menuTitleStyle) { normal = { textColor = Color.black } });
            GUI.Label(new Rect(bannerX, bannerY, bannerW, 46f), "LUMINORA", _menuTitleStyle);

            GUI.Label(new Rect(bannerX, bannerY + 46f, bannerW, 22f), "— 2.5D PROCEDURAL PIXEL SANDBOX —", _menuSubtitleStyle);

            // Floating pulsing splash text
            string splash = LocalizationManager.Get("★ 4大独立存档槽位 & 生存0道具开荒 & 实体伤害击退! ★", "★ 4 Independent Save Slots & Survival Fresh Start & Mob Knockback! ★");
            GUI.Label(new Rect(bannerX + 40f, bannerY + 70f, bannerW - 80f, 24f), splash, _splashStyle);

            // Main Menu Buttons
            float btnW = 380f;
            float btnH = 44f;
            float btnGap = 10f;
            float btnX = (Screen.width - btnW) * 0.5f;
            float btnStartY = bannerY + bannerH + 24f;

            // 1. [SURVIVAL MODE (生存模式 / 选择存档位)]
            Rect r1 = new Rect(btnX, btnStartY, btnW, btnH);
            if (GUI.Button(r1, LocalizationManager.Get("⚔️ 生存模式 (从零开荒)", "⚔️ Survival Mode (Fresh Start)"), _mcBtnStyle))
            {
                _newWorldMode = GameMode.Survival;
                _showWorldSelectModal = true;
                RefreshSaveSlots();
                PlayPopSound();
            }

            // 2. [CREATIVE MODE (创造模式 / 自由建造)]
            Rect r2 = new Rect(btnX, btnStartY + (btnH + btnGap), btnW, btnH);
            if (GUI.Button(r2, LocalizationManager.Get("✨ 创造模式 (飞行建造)", "✨ Creative Mode (Fly & Build)"), _mcBtnStyle))
            {
                _newWorldMode = GameMode.Creative;
                _showWorldSelectModal = true;
                RefreshSaveSlots();
                PlayPopSound();
            }

            // 3. [SAVE SLOTS / LOAD (存档位管理)]
            Rect r3 = new Rect(btnX, btnStartY + (btnH + btnGap) * 2, btnW, btnH);
            if (GUI.Button(r3, LocalizationManager.Get("📂 世界存档管理 (4 独立槽位)", "📂 World Save Slots (4 Slots)"), _mcBtnStyle))
            {
                _showWorldSelectModal = true;
                RefreshSaveSlots();
                PlayPopSound();
            }

            // 4. [LANGUAGE TOGGLE (语言切换)]
            Rect r4 = new Rect(btnX, btnStartY + (btnH + btnGap) * 3, btnW, btnH);
            if (GUI.Button(r4, LocalizationManager.Get("语言: 简体中文", "Language: English"), _mcBtnStyle))
            {
                LocalizationManager.ToggleLanguage();
                PlayPopSound();
            }

            // 5. [EXIT (退出)]
            Rect r5 = new Rect(btnX, btnStartY + (btnH + btnGap) * 4, btnW, btnH);
            if (GUI.Button(r5, LocalizationManager.Get("❌ 退出游戏", "❌ Exit Game"), _mcBtnStyle))
            {
                Application.Quit();
#if UNITY_EDITOR
                UnityEditor.EditorApplication.isPlaying = false;
#endif
            }

            // Footer credits
            float footerY = Screen.height - 32f;
            GUI.Label(new Rect(0, footerY, Screen.width, 24f), LocalizationManager.Get("<color=#888888>Luminora｜流明洛原 | 4组独立存档系统 | 游戏内按 [ESC] 暂停 | [F5] 快速存档</color>", "<color=#888888>Luminora Sandbox | 4 Save Slots System | Press [ESC] in game for Pause | [F5] Quick Save</color>"), _bodyStyle);
        }

        /// <summary>
        /// Minecraft-style Pause Menu overlay:
        /// Resumes game, toggles GameMode, Quick Save, Quick Load, Save Slots Manager, Language Toggle, Return to Main Menu, Quit.
        /// </summary>
        private void DrawPauseMenu()
        {
            // Dim backdrop
            DrawSolidRect(new Rect(0, 0, Screen.width, Screen.height), new Color(0f, 0f, 0f, 0.68f));

            float boxW = 440f;
            float boxH = 500f;
            float boxX = (Screen.width - boxW) * 0.5f;
            float boxY = (Screen.height - boxH) * 0.5f;

            Rect boxRect = new Rect(boxX, boxY, boxW, boxH);
            GUI.Box(boxRect, GUIContent.none, _helpPanelStyle);

            // Title
            Rect titleRect = new Rect(boxX, boxY + 14f, boxW, 26f);
            GUI.Label(titleRect, LocalizationManager.Get("<b>游戏已暂停</b>", "<b>GAME PAUSED</b>"), _titleStyle);

            // Current world and mode badge
            int curSlot = SaveManager.Instance != null ? SaveManager.Instance.CurrentSlotIndex : 0;
            string curWorld = SaveManager.Instance != null ? SaveManager.Instance.CurrentWorldName : $"World {curSlot + 1}";
            GameMode curMode = _player != null ? _player.CurrentGameMode : GameMode.Survival;
            string modeName = curMode.GetDisplayName();
            Rect modeRect = new Rect(boxX, boxY + 40f, boxW, 22f);
            GUI.Label(modeRect, $"{LocalizationManager.Get("当前世界", "Current World")}: <color=#FFDD55>槽位 {curSlot + 1} ({curWorld})</color> | <color=#55FFFF>{modeName}</color>", _bodyStyle);

            // Buttons
            float btnW = 360f;
            float btnH = 38f;
            float btnGap = 8f;
            float btnX = boxX + (boxW - btnW) * 0.5f;
            float btnStartY = boxY + 68f;

            // 1. Resume Game
            if (GUI.Button(new Rect(btnX, btnStartY, btnW, btnH), LocalizationManager.Get("▶ 继续游戏", "▶ Resume Game"), _mcBtnStyle))
            {
                currentGameState = GameState.Playing;
                PlayPopSound();
            }

            // 2. Switch Mode
            string nextModeLabel = curMode == GameMode.Survival 
                ? LocalizationManager.Get("🎮 切换至: 创造模式", "🎮 Switch to: Creative Mode") 
                : LocalizationManager.Get("🎮 切换至: 生存模式", "🎮 Switch to: Survival Mode");
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap), btnW, btnH), nextModeLabel, _mcBtnStyle))
            {
                if (_player != null)
                {
                    _player.CurrentGameMode = (curMode == GameMode.Survival) ? GameMode.Creative : GameMode.Survival;
                    SpawnFloatingText($"{LocalizationManager.Get("已切换模式: ", "Mode switched: ")}{_player.CurrentGameMode.GetDisplayName()}", Color.yellow);
                    PlayTone(700f, 0.1f, 0.3f);
                }
            }

            // 3. Save World
            string saveLabel = LocalizationManager.Get($"💾 保存本世界 (槽位 {curSlot + 1}) [F5]", $"💾 Save World (Slot {curSlot + 1}) [F5]");
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 2, btnW, btnH), saveLabel, _mcBtnStyle))
            {
                QuickSave();
            }

            // 4. Load World
            string loadLabel = LocalizationManager.Get($"📂 读取本世界 (槽位 {curSlot + 1}) [F9]", $"📂 Load World (Slot {curSlot + 1}) [F9]");
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 3, btnW, btnH), loadLabel, _mcBtnStyle))
            {
                QuickLoad();
                currentGameState = GameState.Playing;
            }

            // 5. Save Slots Manager
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 4, btnW, btnH), LocalizationManager.Get("🌐 切换/管理存档位 (4 独立槽位)", "🌐 Switch / Manage Save Slots (4 Slots)"), _mcBtnStyle))
            {
                _showWorldSelectModal = true;
                RefreshSaveSlots();
                PlayPopSound();
            }

            // 6. Language Toggle Button
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 5, btnW, btnH), LocalizationManager.Get("语言: 简体中文", "Language: English"), _mcBtnStyle))
            {
                LocalizationManager.ToggleLanguage();
                PlayPopSound();
            }

            // 7. Return to Main Menu
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 6, btnW, btnH), LocalizationManager.Get("🏠 返回主菜单", "🏠 Main Menu"), _mcBtnStyle))
            {
                currentGameState = GameState.MainMenu;
                PlayPopSound();
            }

            // 8. Quit Game
            if (GUI.Button(new Rect(btnX, btnStartY + (btnH + btnGap) * 7, btnW, btnH), LocalizationManager.Get("❌ 退出游戏", "❌ Quit Game"), _mcBtnStyle))
            {
                Application.Quit();
#if UNITY_EDITOR
                UnityEditor.EditorApplication.isPlaying = false;
#endif
            }
        }

        /// <summary>
        /// Refreshes metadata cache for all 4 save slots from SaveManager.
        /// </summary>
        public void RefreshSaveSlots()
        {
            if (SaveManager.Instance != null)
            {
                _cachedSlots = SaveManager.Instance.GetAllSaveSlots();
            }
            else
            {
                _cachedSlots = new List<SaveMetadata>();
                for (int i = 0; i < 4; i++)
                {
                    _cachedSlots.Add(new SaveMetadata { slotIndex = i, worldName = $"World {i + 1}", exists = false });
                }
            }
        }

        /// <summary>
        /// Renders the multi-save slot selection window (Slots 1 to 4 / Indices 0 to 3),
        /// showing world name, game mode, seed, health, modified blocks count, and last played timestamp.
        /// </summary>
        private void DrawWorldSelectModal()
        {
            // Dim backdrop
            DrawSolidRect(new Rect(0, 0, Screen.width, Screen.height), new Color(0f, 0f, 0f, 0.82f));

            if (_cachedSlots == null)
            {
                RefreshSaveSlots();
            }

            float modalW = 760f;
            float modalH = 550f;
            float modalX = (Screen.width - modalW) * 0.5f;
            float modalY = (Screen.height - modalH) * 0.5f;
            Rect modalRect = new Rect(modalX, modalY, modalW, modalH);

            GUI.Box(modalRect, GUIContent.none, _helpPanelStyle);

            // If creating a world for a specific slot, draw the creation dialog
            if (_creatingSlotIndex >= 0 && _creatingSlotIndex < 4)
            {
                DrawNewWorldCreationDialog(modalX, modalY, modalW, modalH);
                return;
            }

            // Header
            GUI.Label(new Rect(modalX + 24f, modalY + 16f, modalW - 48f, 26f), 
                LocalizationManager.Get("🌐 <b>世界存档管理 (4 个独立存档槽位)</b>", "🌐 <b>WORLD SAVE SLOTS (4 Independent Slots)</b>"), _titleStyle);
            GUI.Label(new Rect(modalX + 24f, modalY + 42f, modalW - 48f, 20f), 
                LocalizationManager.Get("选择一个存档位载入游玩，或创建全新的独立冒险世界", "Select a slot to load or create a brand new adventure world"), _bodyStyle);

            float curY = modalY + 68f;
            float cardW = modalW - 48f;
            float cardH = 92f;
            float cardGap = 10f;

            for (int i = 0; i < 4; i++)
            {
                SaveMetadata meta = (_cachedSlots != null && i < _cachedSlots.Count) ? _cachedSlots[i] : null;
                bool exists = meta != null && meta.exists;
                bool isActive = (SaveManager.Instance != null && SaveManager.Instance.CurrentSlotIndex == i);

                Rect cardRect = new Rect(modalX + 24f, curY, cardW, cardH);
                GUI.Box(cardRect, GUIContent.none, _slotBoxStyle);

                // Slot border: Gold if currently active loaded slot
                if (isActive)
                {
                    DrawOutline(cardRect, 2, new Color(1.0f, 0.85f, 0.25f, 1.0f));
                }
                else
                {
                    DrawOutline(cardRect, 1, new Color(0.28f, 0.32f, 0.42f, 0.85f));
                }

                // Slot Badge
                Rect badgeRect = new Rect(cardRect.x + 12f, cardRect.y + 10f, 74f, 20f);
                DrawSolidRect(badgeRect, new Color(0.14f, 0.16f, 0.22f, 0.95f));
                GUI.Label(badgeRect, LocalizationManager.Get($"槽位 {i + 1}", $"Slot {i + 1}"), _keyBadgeStyle);

                if (isActive)
                {
                    Rect activeBadgeRect = new Rect(badgeRect.x + 80f, cardRect.y + 10f, 80f, 20f);
                    DrawSolidRect(activeBadgeRect, new Color(0.35f, 0.30f, 0.10f, 0.95f));
                    GUI.Label(activeBadgeRect, LocalizationManager.Get("<color=#FFDD55>[当前世界]</color>", "<color=#FFDD55>[Active]</color>"), _bodyStyle);
                }

                if (exists)
                {
                    // World Title
                    GUI.Label(new Rect(cardRect.x + 12f, cardRect.y + 34f, 320f, 22f), $"<b>{meta.worldName}</b>", _titleStyle);

                    // Mode badge + Seed + Health + Last Played
                    string modeTag = (meta.gameMode == GameMode.Survival)
                        ? LocalizationManager.Get("<color=#55FF88>⚔️ 生存模式</color>", "<color=#55FF88>⚔️ Survival</color>")
                        : LocalizationManager.Get("<color=#55FFFF>✨ 创造模式</color>", "<color=#55FFFF>✨ Creative</color>");
                    string statsLine = $"{modeTag}  |  {LocalizationManager.Get("种子", "Seed")}: <color=#FFDD55>{meta.seed}</color>  |  <color=#FF5566>❤️ HP: {meta.playerHealth:F0}/{meta.maxHealth:F0}</color>";
                    GUI.Label(new Rect(cardRect.x + 12f, cardRect.y + 54f, 430f, 18f), statsLine, _bodyStyle);

                    string timeStr = string.IsNullOrEmpty(meta.lastPlayedTimestamp) ? "" : $"{LocalizationManager.Get("存档时间", "Saved")}: {meta.lastPlayedTimestamp}  |  {LocalizationManager.Get("修改方块", "Blocks")}: {meta.modifiedBlocksCount}";
                    GUI.Label(new Rect(cardRect.x + 12f, cardRect.y + 70f, 430f, 18f), $"<color=#888888>{timeStr}</color>", _bodyStyle);

                    // Action Buttons on Right
                    float btnAreaX = cardRect.x + cardW - 220f;
                    float btnY = cardRect.y + (cardH - 36f) * 0.5f;

                    // Play Button
                    if (GUI.Button(new Rect(btnAreaX, btnY, 120f, 36f), LocalizationManager.Get("▶ 进入世界", "▶ Play World"), _mcBtnStyle))
                    {
                        if (SaveManager.Instance != null)
                        {
                            SaveManager.Instance.CurrentSlotIndex = i;
                            SaveManager.Instance.LoadGame(i);
                        }
                        currentGameState = GameState.Playing;
                        _showWorldSelectModal = false;
                        SpawnFloatingText(LocalizationManager.Get($"已载入世界: {meta.worldName} (槽位 {i + 1})", $"Loaded World: {meta.worldName} (Slot {i + 1})"), new Color(0.35f, 0.95f, 0.65f));
                        PlayLoadChime();
                    }

                    // Delete Button
                    if (GUI.Button(new Rect(btnAreaX + 128f, btnY, 76f, 36f), LocalizationManager.Get("🗑️ 删除", "🗑️ Delete"), _mcBtnStyle))
                    {
                        if (SaveManager.Instance != null)
                        {
                            SaveManager.Instance.DeleteSave(i);
                            RefreshSaveSlots();
                            SpawnFloatingText(LocalizationManager.Get($"槽位 {i + 1} 存档已删除", $"Slot {i + 1} Deleted"), new Color(1.0f, 0.4f, 0.4f));
                            PlayTone(350f, 0.1f, 0.3f);
                        }
                    }
                }
                else
                {
                    // Empty slot
                    GUI.Label(new Rect(cardRect.x + 12f, cardRect.y + 40f, 340f, 22f), 
                        LocalizationManager.Get("<color=#888888>[空存档位 - 暂无数据]</color>", "<color=#888888>[Empty Slot - No Data]</color>"), _bodyStyle);

                    // Create New World Button
                    float btnAreaX = cardRect.x + cardW - 190f;
                    float btnY = cardRect.y + (cardH - 36f) * 0.5f;
                    if (GUI.Button(new Rect(btnAreaX, btnY, 175f, 36f), LocalizationManager.Get("➕ 创建新世界", "➕ Create World"), _mcBtnStyle))
                    {
                        _creatingSlotIndex = i;
                        _newWorldName = $"World {i + 1}";
                        _newWorldSeedStr = UnityEngine.Random.Range(10000, 99999).ToString();
                        _newWorldMode = GameMode.Survival;
                        PlayPopSound();
                    }
                }

                curY += cardH + cardGap;
            }

            // Bottom Buttons: Back
            float footerY = modalY + modalH - 44f;
            if (GUI.Button(new Rect(modalX + 24f, footerY, 150f, 34f), LocalizationManager.Get("◀ 返回", "◀ Back"), _mcBtnStyle))
            {
                _showWorldSelectModal = false;
                PlayPopSound();
            }
        }

        /// <summary>
        /// Modal dialog allowing the player to customize worldName, gameMode (Survival vs Creative), and seed.
        /// </summary>
        private void DrawNewWorldCreationDialog(float modalX, float modalY, float modalW, float modalH)
        {
            float dialogW = 540f;
            float dialogH = 410f;
            float dx = modalX + (modalW - dialogW) * 0.5f;
            float dy = modalY + (modalH - dialogH) * 0.5f;
            Rect dialogRect = new Rect(dx, dy, dialogW, dialogH);

            DrawSolidRect(dialogRect, new Color(0.10f, 0.11f, 0.16f, 0.98f));
            DrawOutline(dialogRect, 2, new Color(1.0f, 0.85f, 0.25f, 1.0f));

            float curY = dy + 18f;
            GUI.Label(new Rect(dx + 24f, curY, dialogW - 48f, 26f), 
                LocalizationManager.Get($"✨ <b>创建新世界 (槽位 {_creatingSlotIndex + 1})</b>", $"✨ <b>CREATE NEW WORLD (Slot {_creatingSlotIndex + 1})</b>"), _titleStyle);
            curY += 32f;

            // 1. World Name Field
            GUI.Label(new Rect(dx + 24f, curY, dialogW - 48f, 20f), LocalizationManager.Get("世界名称 (World Name):", "World Name:"), _bodyStyle);
            curY += 22f;
            _newWorldName = GUI.TextField(new Rect(dx + 24f, curY, dialogW - 48f, 28f), _newWorldName, 32);
            curY += 36f;

            // 2. Game Mode Selection
            GUI.Label(new Rect(dx + 24f, curY, dialogW - 48f, 20f), LocalizationManager.Get("游戏模式 (Game Mode):", "Game Mode:"), _bodyStyle);
            curY += 22f;

            float modeBtnW = (dialogW - 58f) * 0.5f;
            bool isSurv = (_newWorldMode == GameMode.Survival);
            GUI.color = isSurv ? new Color(0.4f, 1.0f, 0.5f) : Color.white;
            if (GUI.Button(new Rect(dx + 24f, curY, modeBtnW, 36f), LocalizationManager.Get("⚔️ 生存模式 (0道具开荒)", "⚔️ Survival (Fresh 0 Items)"), _mcBtnStyle))
            {
                _newWorldMode = GameMode.Survival;
            }
            GUI.color = !isSurv ? new Color(0.4f, 0.85f, 1.0f) : Color.white;
            if (GUI.Button(new Rect(dx + 24f + modeBtnW + 10f, curY, modeBtnW, 36f), LocalizationManager.Get("✨ 创造模式 (飞行/无限资源)", "✨ Creative (Fly/Infinite)"), _mcBtnStyle))
            {
                _newWorldMode = GameMode.Creative;
            }
            GUI.color = Color.white;
            curY += 44f;

            // Mode description tip
            string modeTip = (_newWorldMode == GameMode.Survival)
                ? LocalizationManager.Get("💡 生存模式: 从零开始！背包初始清空为 0 道具，怪兽造成真实伤害与击退。", "💡 Survival: Fresh start! Inventory starts at 0 items. Mobs deal damage & knockback.")
                : LocalizationManager.Get("💡 创造模式: 初始配备完整工具套装，双击空格可自由飞行，秒破所有方块。", "💡 Creative: Full starter tools kit. Double-tap space to fly freely and instant-break blocks.");
            GUI.Label(new Rect(dx + 24f, curY, dialogW - 48f, 34f), $"<color=#CCCCCC>{modeTip}</color>", _bodyStyle);
            curY += 38f;

            // 3. World Seed Field
            GUI.Label(new Rect(dx + 24f, curY, 200f, 20f), LocalizationManager.Get("世界种子 (World Seed):", "World Seed:"), _bodyStyle);
            curY += 22f;
            _newWorldSeedStr = GUI.TextField(new Rect(dx + 24f, curY, 320f, 28f), _newWorldSeedStr, 16);
            if (GUI.Button(new Rect(dx + 354f, curY, 138f, 28f), LocalizationManager.Get("🎲 随机种子", "🎲 Randomize"), _mcBtnStyle))
            {
                _newWorldSeedStr = UnityEngine.Random.Range(10000, 999999).ToString();
            }
            curY += 46f;

            // Action Buttons: Confirm & Cancel
            float actBtnW = (dialogW - 58f) * 0.5f;
            if (GUI.Button(new Rect(dx + 24f, curY, actBtnW, 38f), LocalizationManager.Get("✔ 确认创建并开始", "✔ Create & Start"), _mcBtnStyle))
            {
                if (!int.TryParse(_newWorldSeedStr, out int seed))
                {
                    seed = Mathf.Abs(_newWorldSeedStr.GetHashCode());
                }

                if (SaveManager.Instance != null)
                {
                    SaveManager.Instance.CreateNewSave(_creatingSlotIndex, _newWorldName, _newWorldMode, seed);
                }
                else
                {
                    WorldSettings.CurrentGameMode = _newWorldMode;
                    WorldSettings.ActiveSeed = seed;
                }

                currentGameState = GameState.Playing;
                _showWorldSelectModal = false;
                _creatingSlotIndex = -1;
                SpawnFloatingText(LocalizationManager.Get($"新世界已生成: {_newWorldName}", $"New World Created: {_newWorldName}"), new Color(0.35f, 0.95f, 0.65f));
                PlayQuestFanfare();
            }

            if (GUI.Button(new Rect(dx + 24f + actBtnW + 10f, curY, actBtnW, 38f), LocalizationManager.Get("✕ 取消", "✕ Cancel"), _mcBtnStyle))
            {
                _creatingSlotIndex = -1;
                PlayPopSound();
            }
        }

        private void DrawControlRow(string key, string description)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(key, _keyBadgeStyle, GUILayout.Width(175f));
            GUILayout.Label(description, _bodyStyle);
            GUILayout.EndHorizontal();
            GUILayout.Space(2f);
        }

        // =====================================================================
        // Procedural Audio Synthesizer (0 external file dependencies)
        // =====================================================================

        public void PlayPopSound()
        {
            PlayTone(880f, 0.07f, 0.35f);
        }

        public void PlaySaveChime()
        {
            PlayChord(new float[] { 523.25f, 659.25f, 783.99f }, 0.25f, 0.4f);
        }

        public void PlayLoadChime()
        {
            PlayChord(new float[] { 659.25f, 783.99f, 1046.50f }, 0.25f, 0.4f);
        }

        public void PlayQuestFanfare()
        {
            EnsureAudioSource();
            if (_audioSource == null) return;

            // Arpeggio Fanfare: C5 (523Hz), E5 (659Hz), G5 (784Hz), C6 (1046Hz)
            int sampleRate = 44100;
            float duration = 0.48f;
            int totalSamples = (int)(sampleRate * duration);
            float[] data = new float[totalSamples];

            float[] notes = { 523.25f, 659.25f, 783.99f, 1046.50f };
            float noteDuration = duration / notes.Length;

            for (int i = 0; i < totalSamples; i++)
            {
                float t = (float)i / sampleRate;
                int noteIdx = Mathf.Clamp((int)(t / noteDuration), 0, notes.Length - 1);
                float noteT = t - (noteIdx * noteDuration);
                float env = Mathf.Exp(-noteT * 10f);
                data[i] = Mathf.Sin(2f * Mathf.PI * notes[noteIdx] * t) * env * 0.45f;
            }

            AudioClip clip = AudioClip.Create("QuestFanfare", totalSamples, 1, sampleRate, false);
            clip.SetData(data, 0);
            _audioSource.PlayOneShot(clip);
        }

        private void PlayTone(float freq, float duration, float volume)
        {
            EnsureAudioSource();
            if (_audioSource == null) return;

            int sampleRate = 44100;
            int totalSamples = (int)(sampleRate * duration);
            float[] data = new float[totalSamples];

            for (int i = 0; i < totalSamples; i++)
            {
                float t = (float)i / sampleRate;
                float env = Mathf.Exp(-t * 30f);
                data[i] = Mathf.Sin(2f * Mathf.PI * freq * t) * env * volume;
            }

            AudioClip clip = AudioClip.Create("Tone", totalSamples, 1, sampleRate, false);
            clip.SetData(data, 0);
            _audioSource.PlayOneShot(clip);
        }

        private void PlayChord(float[] freqs, float duration, float volume)
        {
            EnsureAudioSource();
            if (_audioSource == null) return;

            int sampleRate = 44100;
            int totalSamples = (int)(sampleRate * duration);
            float[] data = new float[totalSamples];

            for (int i = 0; i < totalSamples; i++)
            {
                float t = (float)i / sampleRate;
                float env = Mathf.Exp(-t * 12f);
                float sum = 0f;
                for (int f = 0; f < freqs.Length; f++)
                {
                    sum += Mathf.Sin(2f * Mathf.PI * freqs[f] * t);
                }
                data[i] = (sum / freqs.Length) * env * volume;
            }

            AudioClip clip = AudioClip.Create("Chord", totalSamples, 1, sampleRate, false);
            clip.SetData(data, 0);
            _audioSource.PlayOneShot(clip);
        }

        // =====================================================================
        // Block Metadata & Rarity Helpers
        // =====================================================================

        public static BlockInspectorInfo GetBlockInspectorInfo(BlockType type)
        {
            switch (type)
            {
                case BlockType.Grass:
                    return new BlockInspectorInfo { chineseName = "草方块", englishName = "Grass Block", icon = "🌿", recommendedTool = LocalizationManager.Get("铲子 / 徒手", "Shovel / Hand"), dropText = LocalizationManager.Get("🌱 泥土 x1", "🌱 Dirt x1") };
                case BlockType.Dirt:
                    return new BlockInspectorInfo { chineseName = "泥土", englishName = "Dirt", icon = "🌱", recommendedTool = LocalizationManager.Get("铲子 / 徒手", "Shovel / Hand"), dropText = LocalizationManager.Get("🌱 泥土 x1", "🌱 Dirt x1") };
                case BlockType.Stone:
                    return new BlockInspectorInfo { chineseName = "原石", englishName = "Stone", icon = "🪨", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🪨 原石 x1", "🪨 Stone x1") };
                case BlockType.Wood:
                    return new BlockInspectorInfo { chineseName = "原木", englishName = "Wood Log", icon = "🌲", recommendedTool = LocalizationManager.Get("斧类", "Axe"), dropText = LocalizationManager.Get("🌲 原木 x1", "🌲 Wood Log x1") };
                case BlockType.Leaves:
                    return new BlockInspectorInfo { chineseName = "树叶", englishName = "Leaves", icon = "🍃", recommendedTool = LocalizationManager.Get("剪刀 / 徒手", "Shears / Hand"), dropText = LocalizationManager.Get("🍃 树叶 x1", "🍃 Leaves x1") };
                case BlockType.Sand:
                    return new BlockInspectorInfo { chineseName = "沙子", englishName = "Sand", icon = "⏳", recommendedTool = LocalizationManager.Get("铲子 / 徒手", "Shovel / Hand"), dropText = LocalizationManager.Get("⏳ 沙子 x1", "⏳ Sand x1") };
                case BlockType.Sandstone:
                    return new BlockInspectorInfo { chineseName = "砂岩", englishName = "Sandstone", icon = "🏜️", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🏜️ 砂岩 x1", "🏜️ Sandstone x1") };
                case BlockType.Cactus:
                    return new BlockInspectorInfo { chineseName = "仙人掌", englishName = "Cactus", icon = "🌵", recommendedTool = LocalizationManager.Get("斧类 / 徒手", "Axe / Hand"), dropText = LocalizationManager.Get("🌵 仙人掌 x1", "🌵 Cactus x1") };
                case BlockType.Snow:
                    return new BlockInspectorInfo { chineseName = "雪块", englishName = "Snow Block", icon = "❄️", recommendedTool = LocalizationManager.Get("铲子 / 徒手", "Shovel / Hand"), dropText = LocalizationManager.Get("❄️ 雪块 x1", "❄️ Snow Block x1") };
                case BlockType.Ice:
                    return new BlockInspectorInfo { chineseName = "冰块", englishName = "Ice", icon = "🧊", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🧊 冰块 x1", "🧊 Ice x1") };
                case BlockType.Clay:
                    return new BlockInspectorInfo { chineseName = "黏土", englishName = "Clay", icon = "🧱", recommendedTool = LocalizationManager.Get("铲子", "Shovel"), dropText = LocalizationManager.Get("🧱 黏土 x1", "🧱 Clay x1") };
                case BlockType.CoalOre:
                    return new BlockInspectorInfo { chineseName = "煤炭矿石", englishName = "Coal Ore", icon = "🔥", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🔥 煤炭 x1", "🔥 Coal x1") };
                case BlockType.IronOre:
                    return new BlockInspectorInfo { chineseName = "铁矿石", englishName = "Iron Ore", icon = "⛏️", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("⛏️ 铁矿 x1", "⛏️ Iron Ore x1") };
                case BlockType.GoldOre:
                    return new BlockInspectorInfo { chineseName = "黄金矿石", englishName = "Gold Ore", icon = "👑", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("👑 黄金 x1", "👑 Gold Ore x1") };
                case BlockType.DiamondOre:
                    return new BlockInspectorInfo { chineseName = "钻石矿石", englishName = "Diamond Ore", icon = "💎", recommendedTool = LocalizationManager.Get("精钢镐", "Diamond Pickaxe"), dropText = LocalizationManager.Get("💎 钻石 x1", "💎 Diamond x1") };
                case BlockType.RubyOre:
                    return new BlockInspectorInfo { chineseName = "红宝石矿石", englishName = "Ruby Ore", icon = "💍", recommendedTool = LocalizationManager.Get("精钢镐", "Ruby Pickaxe"), dropText = LocalizationManager.Get("💍 红宝石 x1", "💍 Ruby x1") };
                case BlockType.Brick:
                    return new BlockInspectorInfo { chineseName = "石砖", englishName = "Stone Brick", icon = "🧱", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🧱 石砖 x1", "🧱 Stone Brick x1") };
                case BlockType.WoodPlank:
                    return new BlockInspectorInfo { chineseName = "木板", englishName = "Wood Plank", icon = "🪵", recommendedTool = LocalizationManager.Get("斧类", "Axe"), dropText = LocalizationManager.Get("🪵 木板 x1", "🪵 Wood Plank x1") };
                case BlockType.Glass:
                    return new BlockInspectorInfo { chineseName = "玻璃", englishName = "Glass", icon = "🪟", recommendedTool = LocalizationManager.Get("任意工具", "Any Tool"), dropText = LocalizationManager.Get("🪟 玻璃 x1", "🪟 Glass x1") };
                case BlockType.Torch:
                    return new BlockInspectorInfo { chineseName = "火把", englishName = "Torch", icon = "🕯️", recommendedTool = LocalizationManager.Get("徒手采集", "Hand"), dropText = LocalizationManager.Get("🕯️ 火把 x1", "🕯️ Torch x1") };
                case BlockType.Obsidian:
                    return new BlockInspectorInfo { chineseName = "黑曜石", englishName = "Obsidian", icon = "🔮", recommendedTool = LocalizationManager.Get("强化镐", "Heavy Pickaxe"), dropText = LocalizationManager.Get("🔮 黑曜石 x1", "🔮 Obsidian x1") };
                case BlockType.Magma:
                    return new BlockInspectorInfo { chineseName = "熔岩块", englishName = "Magma Block", icon = "🌋", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🌋 熔岩块 x1", "🌋 Magma Block x1") };
                case BlockType.MossyStone:
                    return new BlockInspectorInfo { chineseName = "苔石", englishName = "Mossy Stone", icon = "🪨", recommendedTool = LocalizationManager.Get("镐类", "Pickaxe"), dropText = LocalizationManager.Get("🪨 苔石 x1", "🪨 Mossy Stone x1") };
                case BlockType.Mud:
                    return new BlockInspectorInfo { chineseName = "泥泞", englishName = "Mud", icon = "🌾", recommendedTool = LocalizationManager.Get("铲子", "Shovel"), dropText = LocalizationManager.Get("🌾 泥泞 x1", "🌾 Mud x1") };
                case BlockType.GlowShroom:
                    return new BlockInspectorInfo { chineseName = "发光蘑菇", englishName = "Glow Shroom", icon = "🍄", recommendedTool = LocalizationManager.Get("徒手采集", "Hand"), dropText = LocalizationManager.Get("🍄 发光蘑菇 x1", "🍄 Glow Shroom x1") };
                case BlockType.Bedrock:
                    return new BlockInspectorInfo { chineseName = "基岩", englishName = "Bedrock", icon = "⛔", recommendedTool = LocalizationManager.Get("坚不可摧", "Unbreakable"), dropText = LocalizationManager.Get("无", "None") };
                case BlockType.Stick:
                    return new BlockInspectorInfo { chineseName = "木棍", englishName = "Stick", icon = "🥢", recommendedTool = LocalizationManager.Get("基础材料", "Material"), dropText = LocalizationManager.Get("🥢 木棍 x1", "🥢 Stick x1") };
                case BlockType.WoodenPickaxe:
                    return new BlockInspectorInfo { chineseName = "木镐", englishName = "Wooden Pickaxe", icon = "⛏️", recommendedTool = LocalizationManager.Get("基础采矿", "Mining"), dropText = LocalizationManager.Get("⛏️ 木镐 x1", "⛏️ Wooden Pickaxe x1") };
                case BlockType.WoodenAxe:
                    return new BlockInspectorInfo { chineseName = "木斧", englishName = "Wooden Axe", icon = "🪓", recommendedTool = LocalizationManager.Get("基础伐木", "Chopping"), dropText = LocalizationManager.Get("🪓 木斧 x1", "🪓 Wooden Axe x1") };
                case BlockType.WoodenSword:
                    return new BlockInspectorInfo { chineseName = "木剑", englishName = "Wooden Sword", icon = "🗡️", recommendedTool = LocalizationManager.Get("初级防御", "Defense"), dropText = LocalizationManager.Get("🗡️ 木剑 x1", "🗡️ Wooden Sword x1") };
                case BlockType.WoodenShovel:
                    return new BlockInspectorInfo { chineseName = "木铲", englishName = "Wooden Shovel", icon = "🥄", recommendedTool = LocalizationManager.Get("挖掘泥沙", "Excavation"), dropText = LocalizationManager.Get("🥄 木铲 x1", "🥄 Wooden Shovel x1") };
                case BlockType.StonePickaxe:
                    return new BlockInspectorInfo { chineseName = "石镐", englishName = "Stone Pickaxe", icon = "⛏️", recommendedTool = LocalizationManager.Get("采掘铁矿", "Mining"), dropText = LocalizationManager.Get("⛏️ 石镐 x1", "⛏️ Stone Pickaxe x1") };
                case BlockType.StoneAxe:
                    return new BlockInspectorInfo { chineseName = "石斧", englishName = "Stone Axe", icon = "🪓", recommendedTool = LocalizationManager.Get("高效伐木", "Chopping"), dropText = LocalizationManager.Get("🪓 石斧 x1", "🪓 Stone Axe x1") };
                case BlockType.StoneSword:
                    return new BlockInspectorInfo { chineseName = "石剑", englishName = "Stone Sword", icon = "⚔️", recommendedTool = LocalizationManager.Get("武器格斗", "Combat"), dropText = LocalizationManager.Get("⚔️ 石剑 x1", "⚔️ Stone Sword x1") };
                case BlockType.IronPickaxe:
                    return new BlockInspectorInfo { chineseName = "铁镐", englishName = "Iron Pickaxe", icon = "⛏️", recommendedTool = LocalizationManager.Get("强力采矿", "Mining"), dropText = LocalizationManager.Get("⛏️ 铁镐 x1", "⛏️ Iron Pickaxe x1") };
                case BlockType.IronAxe:
                    return new BlockInspectorInfo { chineseName = "铁斧", englishName = "Iron Axe", icon = "🪓", recommendedTool = LocalizationManager.Get("重型伐木", "Chopping"), dropText = LocalizationManager.Get("🪓 铁斧 x1", "🪓 Iron Axe x1") };
                case BlockType.IronSword:
                    return new BlockInspectorInfo { chineseName = "铁剑", englishName = "Iron Sword", icon = "⚔️", recommendedTool = LocalizationManager.Get("锋利斩击", "Combat"), dropText = LocalizationManager.Get("⚔️ 铁剑 x1", "⚔️ Iron Sword x1") };
                case BlockType.DiamondPickaxe:
                    return new BlockInspectorInfo { chineseName = "钻石镐", englishName = "Diamond Pickaxe", icon = "💎", recommendedTool = LocalizationManager.Get("终极神镐", "Master Miner"), dropText = LocalizationManager.Get("💎 钻石镐 x1", "💎 Diamond Pickaxe x1") };
                case BlockType.DiamondSword:
                    return new BlockInspectorInfo { chineseName = "钻石剑", englishName = "Diamond Sword", icon = "💎", recommendedTool = LocalizationManager.Get("绝世神兵", "Master Blade"), dropText = LocalizationManager.Get("💎 钻石剑 x1", "💎 Diamond Sword x1") };
                case BlockType.Gel:
                    return new BlockInspectorInfo { chineseName = "史莱姆凝胶", englishName = "Slime Gel", icon = "🟢", recommendedTool = LocalizationManager.Get("合成材料", "Material"), dropText = LocalizationManager.Get("🟢 凝胶 x1", "🟢 Slime Gel x1") };
                case BlockType.StickyBomb:
                    return new BlockInspectorInfo { chineseName = "粘性炸弹", englishName = "Sticky Bomb", icon = "💣", recommendedTool = LocalizationManager.Get("爆破开山", "Demolition"), dropText = LocalizationManager.Get("💣 炸弹 x1", "💣 Sticky Bomb x1") };
                default:
                    return new BlockInspectorInfo { chineseName = type.ToString(), englishName = type.ToString(), icon = "📦", recommendedTool = LocalizationManager.Get("徒手", "Hand"), dropText = type.ToString() + " x1" };
            }
        }

        public static Color GetBlockRarityColor(BlockType type)
        {
            switch (type)
            {
                case BlockType.DiamondOre:
                case BlockType.RubyOre:
                case BlockType.DiamondPickaxe:
                case BlockType.DiamondSword:
                    return new Color(0.35f, 0.90f, 1.0f); // Cyan Diamond
                case BlockType.GoldOre:
                    return new Color(1.0f, 0.85f, 0.20f); // Gold
                case BlockType.IronOre:
                case BlockType.IronPickaxe:
                case BlockType.IronAxe:
                case BlockType.IronSword:
                    return new Color(0.85f, 0.90f, 0.98f); // Metallic Iron Silver
                case BlockType.Torch:
                case BlockType.Magma:
                case BlockType.StickyBomb:
                    return new Color(1.0f, 0.65f, 0.20f); // Fire Orange / Explosive
                case BlockType.GlowShroom:
                case BlockType.Gel:
                    return new Color(0.25f, 0.95f, 0.55f); // Bio-cyan / Slime Green
                case BlockType.Obsidian:
                    return new Color(0.80f, 0.40f, 1.0f); // Purple
                default:
                    return new Color(0.85f, 0.95f, 0.85f); // Natural white/green
            }
        }

        /// <summary>
        /// Draws the 16x16 pixel-art icon for a block type sampled from master atlas.
        /// Falls back to block default color if atlas is missing.
        /// </summary>
        private void DrawBlockIcon(Rect rect, BlockType type)
        {
            if (type == BlockType.Air) return;

            BlockData data = BlockRegistry.GetBlock(type);
            if (_atlasTex != null && data.textureIndex > 0)
            {
                int tileX = data.textureIndex % 16;
                int tileY = data.textureIndex / 16;
                float u = tileX / 16.0f;
                float v = 1.0f - (tileY + 1) / 16.0f;
                float s = 1.0f / 16.0f;

                GUI.DrawTextureWithTexCoords(rect, _atlasTex, new Rect(u, v, s, s));
            }
            else
            {
                DrawSolidRect(rect, data.defaultColor);
            }
        }

        private string DetermineCurrentBiome(float worldX)
        {
            if (WorldManager.Instance != null)
            {
                return LocalizeBiome(WorldManager.Instance.GetBiomeAt(worldX).ToString());
            }

            TerrainGenerator tg = FindObjectOfType<TerrainGenerator>();
            if (tg != null)
            {
                return LocalizeBiome(tg.GetBiomeAt(worldX).ToString());
            }

            return LocalizationManager.Get("森林", "Forest");
        }

        private string LocalizeBiome(string biomeName)
        {
            switch (biomeName)
            {
                case "Forest": return LocalizationManager.Get("森林", "Forest");
                case "Desert": return LocalizationManager.Get("沙漠", "Desert");
                case "Tundra": return LocalizationManager.Get("苔原", "Tundra");
                case "Underworld": return LocalizationManager.Get("熔岩界", "Underworld");
                default: return biomeName;
            }
        }

        private string DetermineDepthZone(float y)
        {
            if (y >= WorldSettings.BaseSurfaceLevel - 5) return LocalizationManager.Get("天空", "Sky");
            if (y >= WorldSettings.SubsurfaceLevel) return LocalizationManager.Get("地表", "Surface");
            if (y >= WorldSettings.CavernLevel) return LocalizationManager.Get("地层", "Subsurface");
            if (y >= WorldSettings.UnderworldLevel) return LocalizationManager.Get("溶洞", "Cavern");
            return LocalizationManager.Get("熔岩界", "Underworld");
        }

        // =====================================================================
        // Procedural GUIStyles and Textures
        // =====================================================================

        private void InitStyles()
        {
            if (_slotBoxStyle != null) return;

            _slotBoxStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _slotTex }
            };

            _activeSlotBoxStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _activeSlotBorderTex }
            };

            _slotNumberStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 13,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(0.85f, 0.90f, 1.0f, 0.9f) }
            };

            _slotCountStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.LowerRight,
                normal = { textColor = Color.white }
            };

            _activeItemNameStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 16,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(1.0f, 0.88f, 0.35f, 1.0f) }
            };

            _infoPanelStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _panelTex }
            };

            _questPanelStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _questPanelTex }
            };

            _helpPanelStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _panelTex }
            };

            _tooltipBoxStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _tooltipTex }
            };

            _titleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 16,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = new Color(1.0f, 0.82f, 0.25f, 1.0f) }
            };

            _bodyStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 13,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = new Color(0.92f, 0.92f, 0.95f, 1.0f) }
            };

            _keyBadgeStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 13,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = new Color(0.45f, 0.85f, 1.0f, 1.0f) }
            };

            _progressBarBgStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _progressBgTex }
            };

            _progressBarFillStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _progressFillTex }
            };

            _floatingTextStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 15,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter
            };

            _tooltipTitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = new Color(1.0f, 0.88f, 0.35f, 1.0f) }
            };

            _tooltipBodyStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 13,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = Color.white }
            };

            _bannerTitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(1.0f, 0.88f, 0.30f, 1.0f) }
            };

            _bannerSubStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(0.9f, 0.95f, 1.0f, 1.0f) }
            };

            // Minecraft-style stone buttons
            _mcBtnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 15,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { background = _mcBtnNormalTex, textColor = Color.white },
                hover = { background = _mcBtnHoverTex, textColor = new Color(1.0f, 1.0f, 0.65f) },
                active = { background = _mcBtnActiveTex, textColor = new Color(1.0f, 0.85f, 0.25f) }
            };

            _menuTitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 40,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(1.0f, 0.85f, 0.22f) }
            };

            _menuSubtitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(0.75f, 0.85f, 0.95f) }
            };

            _splashStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 15,
                fontStyle = FontStyle.BoldAndItalic,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = new Color(1.0f, 1.0f, 0.35f) }
            };
        }

        private void GenerateProceduralTextures()
        {
            _whiteTex = MakeTex(2, 2, Color.white);
            _panelTex = MakeOrnamentalTex(64, 64, new Color(0.10f, 0.11f, 0.15f, 0.90f), new Color(0.28f, 0.32f, 0.42f, 0.85f), new Color(0.85f, 0.70f, 0.25f, 1.0f), 2);
            _questPanelTex = MakeOrnamentalTex(64, 64, new Color(0.11f, 0.12f, 0.16f, 0.94f), new Color(0.35f, 0.32f, 0.25f, 0.90f), new Color(1.0f, 0.82f, 0.25f, 1.0f), 2);
            _slotTex = MakeBorderedTex(48, 48, new Color(0.12f, 0.13f, 0.17f, 0.88f), new Color(0.32f, 0.35f, 0.45f, 0.75f), 2);
            _activeSlotBorderTex = MakeOrnamentalTex(48, 48, new Color(0.16f, 0.18f, 0.24f, 0.95f), new Color(1.0f, 0.82f, 0.20f, 1.0f), new Color(1.0f, 0.95f, 0.60f, 1.0f), 3);
            _tooltipTex = MakeBorderedTex(48, 48, new Color(0.08f, 0.09f, 0.12f, 0.94f), new Color(0.85f, 0.70f, 0.25f, 0.85f), 1);
            _progressBgTex = MakeBorderedTex(32, 16, new Color(0.10f, 0.10f, 0.13f, 0.92f), new Color(0.35f, 0.35f, 0.42f, 0.8f), 1);
            _progressFillTex = MakeTex(16, 16, new Color(1.0f, 0.65f, 0.15f, 1.0f));
            _badgeTex = MakeTex(16, 16, new Color(0.06f, 0.08f, 0.12f, 0.85f));
            _bannerTex = MakeOrnamentalTex(64, 32, new Color(0.12f, 0.13f, 0.18f, 0.95f), new Color(1.0f, 0.82f, 0.20f, 1.0f), Color.white, 2);

            // Minecraft button and background textures
            _mcBtnNormalTex = MakeMinecraftButtonTex(32, 32, new Color(0.38f, 0.38f, 0.42f, 0.96f), new Color(0.55f, 0.55f, 0.60f, 1.0f), new Color(0.20f, 0.20f, 0.24f, 1.0f), new Color(0.10f, 0.10f, 0.12f, 1.0f));
            _mcBtnHoverTex = MakeMinecraftButtonTex(32, 32, new Color(0.48f, 0.48f, 0.54f, 0.98f), new Color(1.0f, 0.88f, 0.35f, 1.0f), new Color(0.26f, 0.26f, 0.30f, 1.0f), new Color(0.12f, 0.12f, 0.14f, 1.0f));
            _mcBtnActiveTex = MakeMinecraftButtonTex(32, 32, new Color(0.30f, 0.30f, 0.34f, 1.0f), new Color(0.18f, 0.18f, 0.22f, 1.0f), new Color(0.50f, 0.50f, 0.55f, 1.0f), new Color(0.08f, 0.08f, 0.10f, 1.0f));
            _mcDirtTex = MakeMinecraftDirtTex(32, 32);
        }

        private Texture2D MakeMinecraftButtonTex(int width, int height, Color fill, Color lightBevel, Color darkShadow, Color borderCol)
        {
            Texture2D tex = new Texture2D(width, height);
            Color[] pix = new Color[width * height];

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    bool isOuterBorder = (x == 0 || x == width - 1 || y == 0 || y == height - 1);
                    if (isOuterBorder)
                    {
                        pix[y * width + x] = borderCol;
                        continue;
                    }

                    bool isTopLeftBevel = (x <= 2 || y >= height - 3);
                    bool isBottomRightBevel = (x >= width - 3 || y <= 2);

                    if (isTopLeftBevel)
                    {
                        pix[y * width + x] = lightBevel;
                    }
                    else if (isBottomRightBevel)
                    {
                        pix[y * width + x] = darkShadow;
                    }
                    else
                    {
                        float grad = 0.95f + ((float)y / height) * 0.10f;
                        pix[y * width + x] = new Color(fill.r * grad, fill.g * grad, fill.b * grad, fill.a);
                    }
                }
            }

            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private Texture2D MakeMinecraftDirtTex(int width, int height)
        {
            Texture2D tex = new Texture2D(width, height);
            Color[] pix = new Color[width * height];
            Color darkBrown = new Color(0.24f, 0.16f, 0.10f, 0.95f);
            Color lightBrown = new Color(0.32f, 0.22f, 0.14f, 0.95f);

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    bool alt = ((x / 4) + (y / 4)) % 2 == 0;
                    pix[y * width + x] = alt ? lightBrown : darkBrown;
                }
            }

            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private Texture2D MakeTex(int width, int height, Color col)
        {
            Color[] pix = new Color[width * height];
            for (int i = 0; i < pix.Length; i++) pix[i] = col;
            Texture2D tex = new Texture2D(width, height);
            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private Texture2D MakeBorderedTex(int width, int height, Color fill, Color border, int borderWidth)
        {
            Texture2D tex = new Texture2D(width, height);
            Color[] pix = new Color[width * height];

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    bool isBorder = (x < borderWidth || x >= width - borderWidth || y < borderWidth || y >= height - borderWidth);
                    pix[y * width + x] = isBorder ? border : fill;
                }
            }

            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private Texture2D MakeOrnamentalTex(int width, int height, Color fill, Color border, Color goldCorner, int borderWidth)
        {
            Texture2D tex = new Texture2D(width, height);
            Color[] pix = new Color[width * height];

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    bool isCorner = (x < borderWidth * 2 && y < borderWidth * 2) ||
                                    (x >= width - borderWidth * 2 && y < borderWidth * 2) ||
                                    (x < borderWidth * 2 && y >= height - borderWidth * 2) ||
                                    (x >= width - borderWidth * 2 && y >= height - borderWidth * 2);

                    bool isBorder = (x < borderWidth || x >= width - borderWidth || y < borderWidth || y >= height - borderWidth);

                    if (isCorner)
                    {
                        pix[y * width + x] = goldCorner;
                    }
                    else if (isBorder)
                    {
                        pix[y * width + x] = border;
                    }
                    else
                    {
                        // Subtle vertical shading gradient
                        float grad = 0.90f + (float)y / height * 0.18f;
                        pix[y * width + x] = new Color(fill.r * grad, fill.g * grad, fill.b * grad, fill.a);
                    }
                }
            }

            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private void DrawSolidRect(Rect rect, Color color)
        {
            Color prevColor = GUI.color;
            GUI.color = color;
            GUI.DrawTexture(rect, _whiteTex);
            GUI.color = prevColor;
        }

        private void DrawOutline(Rect rect, int thickness, Color color)
        {
            DrawSolidRect(new Rect(rect.x, rect.y, rect.width, thickness), color);
            DrawSolidRect(new Rect(rect.x, rect.y + rect.height - thickness, rect.width, thickness), color);
            DrawSolidRect(new Rect(rect.x, rect.y, thickness, rect.height), color);
            DrawSolidRect(new Rect(rect.x + rect.width - thickness, rect.y, thickness, rect.height), color);
        }
    }
}
