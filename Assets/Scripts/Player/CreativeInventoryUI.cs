using System;
using System.Collections.Generic;
using Core;
using UI;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Autonomous Creative Mode Big Inventory UI (创造模式全量物品库窗口).
    /// Toggled via [E] or [ESC] key in game.
    /// 
    /// Features:
    /// 1. Categorized Tab Navigation:
    ///    - 🧱 建筑方块 (Building Blocks)
    ///    - ⚔️ 工具武器 (Tools & Weapons)
    ///    - 💎 矿物材料 (Ores & Materials)
    ///    - 💡 装饰光源 (Decor & Lighting)
    /// 2. Grid Display of All Available Blocks, Materials, and 16 Tiered Tools.
    /// 3. Direct Hotbar Assignment: Left-click any grid item to equip into the active hotbar slot [1-9].
    /// 4. In Creative Mode: Items placed in hotbar enjoy infinite stacks (999 count).
    /// 5. Self-contained immediate-mode GUI with zero external prefab dependencies.
    /// </summary>
    [DisallowMultipleComponent]
    public class CreativeInventoryUI : MonoBehaviour
    {
        public static CreativeInventoryUI Instance { get; private set; }

        public enum TabType
        {
            Blocks = 0,
            Tools = 1,
            Materials = 2,
            Decor = 3
        }

        [Header("State")]
        [SerializeField] private bool isOpen = false;
        [SerializeField] private TabType activeTab = TabType.Blocks;
        [SerializeField] private int targetHotbarSlot = 0;

        public bool IsOpen
        {
            get => isOpen;
            set => isOpen = value;
        }

        // GUI Styles and procedurally generated textures
        private Texture2D _panelTex;
        private Texture2D _headerTex;
        private Texture2D _slotTex;
        private Texture2D _activeSlotTex;
        private Texture2D _tabActiveTex;
        private Texture2D _tabInactiveTex;

        private GUIStyle _windowStyle;
        private GUIStyle _headerStyle;
        private GUIStyle _tabActiveStyle;
        private GUIStyle _tabInactiveStyle;
        private GUIStyle _slotLabelStyle;
        private GUIStyle _tooltipTitleStyle;
        private GUIStyle _tooltipBodyStyle;
        private GUIStyle _hotbarNumStyle;
        private GUIStyle _closeBtnStyle;

        private bool _stylesInitialized = false;

        // Categorized Items Cache
        private static readonly BlockType[] BuildingBlocks = new BlockType[]
        {
            BlockType.Grass, BlockType.Dirt, BlockType.Stone, BlockType.Wood, BlockType.Leaves,
            BlockType.Sand, BlockType.Sandstone, BlockType.Snow, BlockType.Ice, BlockType.Clay,
            BlockType.Brick, BlockType.WoodPlank, BlockType.Glass, BlockType.MossyStone,
            BlockType.Mud, BlockType.Obsidian, BlockType.Bedrock
        };

        private static readonly BlockType[] OresAndMaterials = new BlockType[]
        {
            BlockType.CoalOre, BlockType.IronOre, BlockType.GoldOre,
            BlockType.DiamondOre, BlockType.RubyOre, BlockType.Magma
        };

        private static readonly BlockType[] DecorAndLighting = new BlockType[]
        {
            BlockType.Torch, BlockType.GlowShroom, BlockType.Cactus
        };

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
        }

        private void Update()
        {
            // Toggle Big Inventory with [E] key
            if (Input.GetKeyDown(KeyCode.E))
            {
                // Only allow opening if not on main menu
                if (UIManager.Instance == null || UIManager.Instance.CurrentState == GameState.Playing)
                {
                    isOpen = !isOpen;
                    if (isOpen)
                    {
                        if (InventorySystem.Instance != null)
                        {
                            targetHotbarSlot = InventorySystem.Instance.ActiveSlotIndex;
                        }
                    }
                }
            }

            // Close with [ESC] key if currently open
            if (isOpen && Input.GetKeyDown(KeyCode.Escape))
            {
                isOpen = false;
            }
        }

        private void InitializeStyles()
        {
            if (_stylesInitialized) return;

            _panelTex = CreateSolidTexture(new Color(0.10f, 0.12f, 0.16f, 0.98f));
            _headerTex = CreateSolidTexture(new Color(0.07f, 0.08f, 0.11f, 0.98f));
            _slotTex = CreateSolidTexture(new Color(0.14f, 0.16f, 0.22f, 0.95f));
            _activeSlotTex = CreateSolidTexture(new Color(0.24f, 0.30f, 0.42f, 1.0f));
            _tabActiveTex = CreateSolidTexture(new Color(0.22f, 0.26f, 0.35f, 1.0f));
            _tabInactiveTex = CreateSolidTexture(new Color(0.13f, 0.15f, 0.20f, 0.95f));

            _windowStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _panelTex }
            };

            _headerStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleLeft,
                normal = { textColor = new Color(1.0f, 0.86f, 0.25f) }
            };

            _tabActiveStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 13,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { background = _tabActiveTex, textColor = new Color(1.0f, 0.88f, 0.30f) }
            };

            _tabInactiveStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 12,
                fontStyle = FontStyle.Normal,
                alignment = TextAnchor.MiddleCenter,
                normal = { background = _tabInactiveTex, textColor = new Color(0.70f, 0.75f, 0.85f) }
            };

            _slotLabelStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 10,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.LowerCenter,
                normal = { textColor = Color.white }
            };

            _tooltipTitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(1.0f, 0.88f, 0.30f) }
            };

            _tooltipBodyStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 11,
                normal = { textColor = new Color(0.85f, 0.90f, 0.95f) }
            };

            _hotbarNumStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 10,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(1.0f, 0.85f, 0.25f) }
            };

            _closeBtnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                normal = { textColor = Color.white }
            };

            _stylesInitialized = true;
        }

        private void OnGUI()
        {
            if (!isOpen) return;

            InitializeStyles();

            // 1. Fullscreen Dimmed Backdrop Overlay
            GUI.color = new Color(0f, 0f, 0f, 0.70f);
            GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture);
            GUI.color = Color.white;

            // 2. Main Window Modal
            float winW = 860f;
            float winH = 580f;
            float winX = (Screen.width - winW) * 0.5f;
            float winY = (Screen.height - winH) * 0.5f;
            Rect winRect = new Rect(winX, winY, winW, winH);

            GUI.Box(winRect, GUIContent.none, _windowStyle);
            DrawBorder(winRect, 2, new Color(0.35f, 0.42f, 0.55f, 1f));
            DrawBorder(new Rect(winX + 4, winY + 4, winW - 8, winH - 8), 1, new Color(0.95f, 0.82f, 0.22f, 0.7f));

            // 3. Header Bar
            float headerH = 46f;
            Rect headerRect = new Rect(winX + 6, winY + 6, winW - 12, headerH);
            GUI.DrawTexture(headerRect, _headerTex);
            DrawBorder(headerRect, 1, new Color(0.25f, 0.30f, 0.40f));

            GUI.Label(new Rect(winX + 20, winY + 12, 420, 32), LocalizationManager.Get("🌟 创造模式全量物品库", "🌟 CREATIVE INVENTORY"), _headerStyle);
            GUI.Label(new Rect(winX + 440, winY + 16, 360, 24), LocalizationManager.Get("<color=#88CCFF>[E / ESC] 关闭 | 单击选择直接装配进快捷栏</color>", "<color=#88CCFF>[E / ESC] Close | Click to equip to hotbar</color>"), _tooltipBodyStyle);

            // Close button [X]
            Rect closeRect = new Rect(winX + winW - 38, winY + 10, 28, 28);
            if (GUI.Button(closeRect, "X", _closeBtnStyle))
            {
                isOpen = false;
                return;
            }

            // 4. Categorized Tabs
            float tabY = winY + headerH + 12f;
            float tabW = 196f;
            float tabH = 34f;
            float tabGap = 8f;
            float tabStartX = winX + 16f;

            string[] tabNames = new string[]
            {
                LocalizationManager.Get("🧱 建筑方块", "🧱 Blocks"),
                LocalizationManager.Get("⚔️ 工具武器", "⚔️ Tools"),
                LocalizationManager.Get("💎 矿物材料", "💎 Materials"),
                LocalizationManager.Get("💡 装饰光源", "💡 Decor")
            };

            for (int t = 0; t < 4; t++)
            {
                Rect tabRect = new Rect(tabStartX + t * (tabW + tabGap), tabY, tabW, tabH);
                bool isActive = (int)activeTab == t;
                GUIStyle tabStyle = isActive ? _tabActiveStyle : _tabInactiveStyle;

                if (GUI.Button(tabRect, tabNames[t], tabStyle))
                {
                    activeTab = (TabType)t;
                }

                if (isActive)
                {
                    DrawBorder(tabRect, 2, new Color(1.0f, 0.85f, 0.25f));
                }
            }

            // 5. Items Grid Panel
            float gridX = winX + 16f;
            float gridY = tabY + tabH + 8f;
            float gridW = winW - 32f;
            float gridH = 310f;
            Rect gridPanelRect = new Rect(gridX, gridY, gridW, gridH);

            GUI.Box(gridPanelRect, GUIContent.none, _windowStyle);
            DrawBorder(gridPanelRect, 1, new Color(0.25f, 0.30f, 0.38f));

            // Render current tab grid
            DrawCurrentTabGrid(gridX, gridY, gridW, gridH);

            // 6. Bottom Hotbar Equipment Bar
            float hotbarSectionY = gridY + gridH + 12f;
            GUI.Label(new Rect(winX + 20, hotbarSectionY + 8f, 500, 20),
                LocalizationManager.Get("<b>当前快捷栏 [1-9] (点击任意方格直接装配，创造模式无限使用):</b>", "<b>Active Hotbar [1-9] (Click any slot to equip, infinite in Creative):</b>"), _tooltipTitleStyle);

            DrawBottomHotbar(winX + 22, hotbarSectionY + 30f, winW);
        }

        private void DrawCurrentTabGrid(float gridX, float gridY, float gridW, float gridH)
        {
            int cols = 8;
            float slotSize = 68f;
            float gapX = 14f;
            float gapY = 14f;
            float startX = gridX + 22f;
            float startY = gridY + 16f;

            Vector2 mousePos = Event.current.mousePosition;
            string hoveredTooltipTitle = null;
            string hoveredTooltipBody = null;
            Rect hoveredRect = Rect.zero;

            InventorySystem inv = InventorySystem.Instance;

            switch (activeTab)
            {
                case TabType.Blocks:
                {
                    for (int i = 0; i < BuildingBlocks.Length; i++)
                    {
                        BlockType bt = BuildingBlocks[i];
                        int r = i / cols;
                        int c = i % cols;
                        Rect slotRect = new Rect(startX + c * (slotSize + gapX), startY + r * (slotSize + gapY), slotSize, slotSize);

                        bool isHovered = slotRect.Contains(mousePos);
                        GUI.DrawTexture(slotRect, isHovered ? _activeSlotTex : _slotTex);
                        DrawBorder(slotRect, isHovered ? 2 : 1, isHovered ? new Color(1f, 0.85f, 0.25f) : new Color(0.3f, 0.35f, 0.45f));

                        // Block icon & label
                        DrawBlockPreview(slotRect, bt);

                        if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                        {
                            EquipBlockToHotbar(bt);
                        }

                        if (isHovered)
                        {
                            BlockData bData = BlockRegistry.GetBlock(bt);
                            hoveredTooltipTitle = bData.displayName;
                            hoveredTooltipBody = $"{LocalizationManager.Get("硬度", "Hardness")}: {bData.hardness:F1} | {LocalizationManager.Get("固体", "Solid")}: {(bData.isSolid ? LocalizationManager.Get("是", "Yes") : LocalizationManager.Get("否", "No"))}\n" +
                                LocalizationManager.Get("<color=#55FF88>单击直接装备进快捷栏 [无限方块]</color>", "<color=#55FF88>Click to equip to hotbar [Infinite]</color>");
                            hoveredRect = slotRect;
                        }
                    }
                    break;
                }

                case TabType.Tools:
                {
                    IReadOnlyList<ToolData> tools = ToolRegistry.GetAllTools();
                    for (int i = 0; i < tools.Count; i++)
                    {
                        ToolData tool = tools[i];
                        int r = i / cols;
                        int c = i % cols;
                        Rect slotRect = new Rect(startX + c * (slotSize + gapX), startY + r * (slotSize + gapY), slotSize, slotSize);

                        bool isHovered = slotRect.Contains(mousePos);
                        GUI.DrawTexture(slotRect, isHovered ? _activeSlotTex : _slotTex);
                        DrawBorder(slotRect, isHovered ? 2 : 1, isHovered ? new Color(1f, 0.85f, 0.25f) : tool.tierColor);

                        // Tool icon & tier label
                        DrawToolPreview(slotRect, tool);

                        if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                        {
                            EquipToolToHotbar(tool);
                        }

                        if (isHovered)
                        {
                            hoveredTooltipTitle = tool.displayName;
                            hoveredTooltipBody = $"{tool.localizedDescription}\n{LocalizationManager.Get("伤害", "DMG")}: +{tool.damage:F0} | {LocalizationManager.Get("击退", "Knockback")}: {tool.knockbackMultiplier:F1}x | {LocalizationManager.Get("倍率", "Speed")}: {tool.efficiencyMultiplier:F1}x\n" +
                                LocalizationManager.Get("<color=#55FF88>单击直接装备进快捷栏</color>", "<color=#55FF88>Click to equip to hotbar</color>");
                            hoveredRect = slotRect;
                        }
                    }
                    break;
                }

                case TabType.Materials:
                {
                    for (int i = 0; i < OresAndMaterials.Length; i++)
                    {
                        BlockType bt = OresAndMaterials[i];
                        int r = i / cols;
                        int c = i % cols;
                        Rect slotRect = new Rect(startX + c * (slotSize + gapX), startY + r * (slotSize + gapY), slotSize, slotSize);

                        bool isHovered = slotRect.Contains(mousePos);
                        GUI.DrawTexture(slotRect, isHovered ? _activeSlotTex : _slotTex);
                        DrawBorder(slotRect, isHovered ? 2 : 1, isHovered ? new Color(1f, 0.85f, 0.25f) : new Color(0.3f, 0.35f, 0.45f));

                        DrawBlockPreview(slotRect, bt);

                        if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                        {
                            EquipBlockToHotbar(bt);
                        }

                        if (isHovered)
                        {
                            BlockData bData = BlockRegistry.GetBlock(bt);
                            hoveredTooltipTitle = bData.displayName;
                            hoveredTooltipBody = $"{LocalizationManager.Get("稀有矿脉原料", "Rare Ore Material")} | {LocalizationManager.Get("硬度", "Hardness")}: {bData.hardness:F1}\n" +
                                LocalizationManager.Get("<color=#55FF88>单击装备进快捷栏 [无限原料]</color>", "<color=#55FF88>Click to equip to hotbar [Infinite]</color>");
                            hoveredRect = slotRect;
                        }
                    }
                    break;
                }

                case TabType.Decor:
                {
                    for (int i = 0; i < DecorAndLighting.Length; i++)
                    {
                        BlockType bt = DecorAndLighting[i];
                        int r = i / cols;
                        int c = i % cols;
                        Rect slotRect = new Rect(startX + c * (slotSize + gapX), startY + r * (slotSize + gapY), slotSize, slotSize);

                        bool isHovered = slotRect.Contains(mousePos);
                        GUI.DrawTexture(slotRect, isHovered ? _activeSlotTex : _slotTex);
                        DrawBorder(slotRect, isHovered ? 2 : 1, isHovered ? new Color(1f, 0.85f, 0.25f) : new Color(0.3f, 0.35f, 0.45f));

                        DrawBlockPreview(slotRect, bt);

                        if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                        {
                            EquipBlockToHotbar(bt);
                        }

                        if (isHovered)
                        {
                            BlockData bData = BlockRegistry.GetBlock(bt);
                            hoveredTooltipTitle = bData.displayName;
                            hoveredTooltipBody = $"{LocalizationManager.Get("自发光装饰光源 | 光照范围: 7.5格", "Light Source | Range: 7.5 blocks")}\n" +
                                LocalizationManager.Get("<color=#55FF88>单击装备进快捷栏 [无限光源]</color>", "<color=#55FF88>Click to equip to hotbar [Infinite]</color>");
                            hoveredRect = slotRect;
                        }
                    }
                    break;
                }
            }

            // Draw Floating Tooltip
            if (!string.IsNullOrEmpty(hoveredTooltipTitle))
            {
                float tipW = 320f;
                float tipH = 80f;
                float tipX = mousePos.x + 18f;
                float tipY = mousePos.y + 18f;

                if (tipX + tipW > Screen.width - 20) tipX = Screen.width - tipW - 20;
                if (tipY + tipH > Screen.height - 20) tipY = Screen.height - tipH - 20;

                Rect tipRect = new Rect(tipX, tipY, tipW, tipH);
                GUI.Box(tipRect, GUIContent.none, _windowStyle);
                DrawBorder(tipRect, 1, new Color(1.0f, 0.85f, 0.25f));

                GUI.Label(new Rect(tipX + 10, tipY + 8, tipW - 20, 22), hoveredTooltipTitle, _tooltipTitleStyle);
                GUI.Label(new Rect(tipX + 10, tipY + 30, tipW - 20, 44), hoveredTooltipBody, _tooltipBodyStyle);
            }
        }

        private void DrawBottomHotbar(float startX, float startY, float totalWinWidth)
        {
            InventorySystem inv = InventorySystem.Instance;
            if (inv == null) return;

            float slotSize = 48f;
            float gap = 8f;

            for (int i = 0; i < InventorySystem.HotbarSlotCount; i++)
            {
                Rect slotRect = new Rect(startX + i * (slotSize + gap), startY, slotSize, slotSize);
                bool isActive = (i == inv.ActiveSlotIndex);

                GUI.DrawTexture(slotRect, isActive ? _activeSlotTex : _slotTex);
                DrawBorder(slotRect, isActive ? 2 : 1, isActive ? new Color(1f, 0.85f, 0.25f) : new Color(0.35f, 0.40f, 0.50f));

                InventorySlot slot = inv.GetSlot(i);
                if (slot != null && !slot.IsEmpty)
                {
                    if (slot.isTool)
                    {
                        ToolData td = ToolRegistry.Get(slot.toolId);
                        if (td != null)
                        {
                            DrawToolPreview(slotRect, td);
                        }
                    }
                    else
                    {
                        DrawBlockPreview(slotRect, slot.type);
                        GUI.Label(new Rect(slotRect.x + 2, slotRect.y + slotSize - 16, slotSize - 4, 14),
                            $"{slot.count}", _slotLabelStyle);
                    }
                }

                // Slot number badge [1-9]
                GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 2, 20, 16), $"{i + 1}", _hotbarNumStyle);

                // Clicking slot selects it as the target hotbar slot
                if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                {
                    inv.SetActiveSlot(i);
                    targetHotbarSlot = i;
                }
            }

            // Clear Active Slot Button
            float trashX = startX + 9 * (slotSize + gap) + 12f;
            Rect trashRect = new Rect(trashX, startY + 6f, 130f, 36f);
            if (GUI.Button(trashRect, LocalizationManager.Get("🗑️ 清空手持槽位", "🗑️ Clear Active Slot"), _tabInactiveStyle))
            {
                InventorySlot cur = inv.GetActiveItem();
                if (cur != null)
                {
                    cur.Clear();
                    UIManager.Instance?.SpawnFloatingText(LocalizationManager.Get("手持槽位已清空", "Active slot cleared"), new Color(1f, 0.6f, 0.4f));
                }
            }
        }

        private void EquipBlockToHotbar(BlockType bt)
        {
            InventorySystem inv = InventorySystem.Instance;
            if (inv == null) return;

            InventorySlot target = inv.GetSlot(inv.ActiveSlotIndex);
            if (target != null)
            {
                target.isTool = false;
                target.toolId = ToolID.None;
                target.type = bt;
                target.count = 999; // Infinite stack in Creative Mode
            }

            BlockData bData = BlockRegistry.GetBlock(bt);
            UIManager.Instance?.SpawnFloatingText($"{LocalizationManager.Get("已装备: ", "Equipped: ")}{bData.displayName} (999)", new Color(0.35f, 0.95f, 0.45f));
        }

        private void EquipToolToHotbar(ToolData tool)
        {
            InventorySystem inv = InventorySystem.Instance;
            if (inv == null) return;

            InventorySlot target = inv.GetSlot(inv.ActiveSlotIndex);
            if (target != null)
            {
                target.isTool = true;
                target.toolId = tool.id;
                target.type = BlockType.Air;
                target.count = 1;
            }

            UIManager.Instance?.SpawnFloatingText($"{LocalizationManager.Get("已装备神装: ", "Equipped: ")}{tool.displayName}!", tool.tierColor);
        }

        private void DrawBlockPreview(Rect rect, BlockType bt)
        {
            BlockData bData = BlockRegistry.GetBlock(bt);
            Color c = bData.defaultColor;

            float pad = 12f;
            Rect previewRect = new Rect(rect.x + pad, rect.y + 6f, rect.width - pad * 2, rect.height - pad * 2 - 8f);

            GUI.color = c;
            GUI.DrawTexture(previewRect, Texture2D.whiteTexture);
            GUI.color = Color.white;
            DrawBorder(previewRect, 1, new Color(0f, 0f, 0f, 0.6f));

            // Name abbreviation
            string fullName = bData.displayName;
            string shortName = fullName;
            if (LocalizationManager.CurrentLanguage == GameLanguage.Chinese && shortName.Length > 3)
                shortName = shortName.Substring(0, 3);
            else if (LocalizationManager.CurrentLanguage == GameLanguage.English && shortName.Length > 6)
                shortName = shortName.Substring(0, 6);
            GUI.Label(new Rect(rect.x, rect.y + rect.height - 18f, rect.width, 16f), shortName, _slotLabelStyle);
        }

        private void DrawToolPreview(Rect rect, ToolData tool)
        {
            float pad = 12f;
            Rect previewRect = new Rect(rect.x + pad, rect.y + 6f, rect.width - pad * 2, rect.height - pad * 2 - 8f);

            GUI.color = tool.tierColor;
            GUI.DrawTexture(previewRect, Texture2D.whiteTexture);
            GUI.color = Color.white;
            DrawBorder(previewRect, 1, new Color(0f, 0f, 0f, 0.6f));

            string fullName = tool.displayName;
            string shortName = fullName;
            if (LocalizationManager.CurrentLanguage == GameLanguage.Chinese && shortName.Length > 3)
                shortName = shortName.Substring(0, 3);
            else if (LocalizationManager.CurrentLanguage == GameLanguage.English && shortName.Length > 6)
                shortName = shortName.Substring(0, 6);
            GUI.Label(new Rect(rect.x, rect.y + rect.height - 18f, rect.width, 16f), shortName, _slotLabelStyle);
        }

        private void DrawBorder(Rect r, int thickness, Color color)
        {
            Color old = GUI.color;
            GUI.color = color;
            GUI.DrawTexture(new Rect(r.x, r.y, r.width, thickness), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(r.x, r.y + r.height - thickness, r.width, thickness), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(r.x, r.y, thickness, r.height), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(r.x + r.width - thickness, r.y, thickness, r.height), Texture2D.whiteTexture);
            GUI.color = old;
        }

        private Texture2D CreateSolidTexture(Color c)
        {
            Texture2D tex = new Texture2D(2, 2);
            Color[] cols = new Color[4] { c, c, c, c };
            tex.SetPixels(cols);
            tex.Apply();
            return tex;
        }
    }
}
