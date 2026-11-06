using System;
using System.Collections.Generic;
using Core;
using Player;
using UI;
using UnityEngine;

namespace Crafting
{
    /// <summary>
    /// Master Crafting System and Recipe Database Manager.
    /// Handles recipe catalogs, category filtering, inventory deduction/provisioning,
    /// stylized immediate-mode OnGUI Crafting/Inventory interface, keyboard shortcuts ([C] and [E]),
    /// floating celebration banners, and procedural fanfare audio synthesis (SFX_CRAFT).
    /// </summary>
    [DisallowMultipleComponent]
    public class CraftingManager : MonoBehaviour
    {
        public static CraftingManager Instance { get; private set; }

        public enum CraftingModalTab
        {
            Crafting = 0,
            Inventory = 1
        }

        [Header("Modal State")]
        [SerializeField] private bool isOpen = false;
        [SerializeField] private CraftingModalTab currentTab = CraftingModalTab.Crafting;
        [SerializeField] private int selectedRecipeIndex = 0;
        [SerializeField] private string selectedCategory = "All";

        [Header("Recipe Catalog")]
        [SerializeField] private List<RecipeDefinition> recipes = new List<RecipeDefinition>();

        // Audio & References
        private AudioSource _audioSource;
        private InventorySystem _inventory;
        private Vector2 _recipeScrollPos = Vector2.zero;
        private Vector2 _inventoryScrollPos = Vector2.zero;

        // Categories
        public static readonly string[] Categories = new string[]
        {
            "All",
            "Tools",
            "Weapons",
            "Blocks",
            "Supplies"
        };

        // Procedural Textures & Styles
        private Texture2D _panelBgTex;
        private Texture2D _cardBgTex;
        private Texture2D _selectedCardBgTex;
        private Texture2D _btnNormalTex;
        private Texture2D _btnHoverTex;
        private Texture2D _btnActiveTex;
        private Texture2D _slotBgTex;
        private GUIStyle _panelStyle;
        private GUIStyle _titleStyle;
        private GUIStyle _subTitleStyle;
        private GUIStyle _tabBtnStyle;
        private GUIStyle _activeTabBtnStyle;
        private GUIStyle _categoryBtnStyle;
        private GUIStyle _activeCategoryBtnStyle;
        private GUIStyle _recipeCardStyle;
        private GUIStyle _selectedRecipeCardStyle;
        private GUIStyle _craftBtnStyle;
        private GUIStyle _itemLabelStyle;
        private GUIStyle _ingredientSatisfiedStyle;
        private GUIStyle _ingredientMissingStyle;
        private bool _stylesInitialized = false;

        public bool IsOpen => isOpen;
        public IReadOnlyList<RecipeDefinition> Recipes => recipes;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            BuildRecipeCatalog();
            EnsureAudioSource();
        }

        private void Start()
        {
            _inventory = InventorySystem.Instance;
            GenerateTextures();
        }

        private void Update()
        {
            // Toggle crafting directly with [C]
            if (Input.GetKeyDown(KeyCode.C))
            {
                if (isOpen && currentTab == CraftingModalTab.Crafting)
                {
                    CloseModal();
                }
                else
                {
                    OpenModal(CraftingModalTab.Crafting);
                }
            }

            // Toggle inventory/crafting modal with [E]
            if (Input.GetKeyDown(KeyCode.E))
            {
                if (isOpen)
                {
                    CloseModal();
                }
                else
                {
                    OpenModal(CraftingModalTab.Inventory);
                }
            }

            // Close modal with [ESC]
            if (isOpen && Input.GetKeyDown(KeyCode.Escape))
            {
                CloseModal();
            }

            // Arrow key navigation through recipe list when open
            if (isOpen && currentTab == CraftingModalTab.Crafting)
            {
                List<RecipeDefinition> filtered = GetFilteredRecipes();
                if (Input.GetKeyDown(KeyCode.UpArrow) || Input.GetKeyDown(KeyCode.W))
                {
                    if (selectedRecipeIndex > 0)
                    {
                        selectedRecipeIndex--;
                        PlayTone(480f, 0.05f, 0.2f);
                    }
                }
                else if (Input.GetKeyDown(KeyCode.DownArrow) || Input.GetKeyDown(KeyCode.S))
                {
                    if (selectedRecipeIndex < filtered.Count - 1)
                    {
                        selectedRecipeIndex++;
                        PlayTone(520f, 0.05f, 0.2f);
                    }
                }
                else if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Space))
                {
                    if (selectedRecipeIndex >= 0 && selectedRecipeIndex < filtered.Count)
                    {
                        CraftRecipe(filtered[selectedRecipeIndex], 1);
                    }
                }
            }
        }

        public void OpenModal(CraftingModalTab tab = CraftingModalTab.Crafting)
        {
            isOpen = true;
            currentTab = tab;
            _inventory = InventorySystem.Instance;
            PlayTone(650f, 0.08f, 0.3f);
        }

        public void CloseModal()
        {
            isOpen = false;
            PlayTone(400f, 0.06f, 0.25f);
        }

        /// <summary>
        /// Populates the authentic classic Terraria / Minecraft recipe catalog.
        /// </summary>
        private void BuildRecipeCatalog()
        {
            recipes.Clear();
            int id = 1;

            // 1. 1 Wood -> 4 Wood Planks (木板)
            recipes.Add(new RecipeDefinition(
                id++, "Wood Planks", "木板", "Blocks",
                "Cut raw logs into sturdy lumber planks for building and carpentry.",
                BlockType.WoodPlank, 4, "🪵",
                new IngredientRequirement(BlockType.Wood, 1)
            ));

            // 2. 2 Wood Planks -> 4 Sticks (木棍)
            recipes.Add(new RecipeDefinition(
                id++, "Sticks", "木棍", "Materials",
                "Slender shafts of wood, indispensable for tool handles and torches.",
                BlockType.Stick, 4, "🥢",
                new IngredientRequirement(BlockType.WoodPlank, 2)
            ));

            // 3. 3 Wood Planks + 2 Sticks -> Wooden Pickaxe (木镐)
            recipes.Add(new RecipeDefinition(
                id++, "Wooden Pickaxe", "木镐", "Tools",
                "Basic entry-level mining tool. Breaks stone and soft ores.",
                BlockType.WoodenPickaxe, 1, "⛏️",
                new IngredientRequirement(BlockType.WoodPlank, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 4. 3 Wood Planks + 2 Sticks -> Wooden Axe (木斧)
            recipes.Add(new RecipeDefinition(
                id++, "Wooden Axe", "木斧", "Tools",
                "Standard timber axe. Accelerates tree chopping and wood collection.",
                BlockType.WoodenAxe, 1, "🪓",
                new IngredientRequirement(BlockType.WoodPlank, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 5. 2 Wood Planks + 1 Stick -> Wooden Sword (木剑)
            recipes.Add(new RecipeDefinition(
                id++, "Wooden Sword", "木剑", "Weapons",
                "Light defensive sidearm. Deals moderate knockback to slimes and zombies.",
                BlockType.WoodenSword, 1, "🗡️",
                new IngredientRequirement(BlockType.WoodPlank, 2),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 6. 1 Wood Plank + 2 Sticks -> Wooden Shovel (木铲)
            recipes.Add(new RecipeDefinition(
                id++, "Wooden Shovel", "木铲", "Tools",
                "Excavation spade. Digs dirt, sand, gravel and snow effortlessly.",
                BlockType.WoodenShovel, 1, "🥄",
                new IngredientRequirement(BlockType.WoodPlank, 1),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 7. 3 Stone + 2 Sticks -> Stone Pickaxe (石镐)
            recipes.Add(new RecipeDefinition(
                id++, "Stone Pickaxe", "石镐", "Tools",
                "Sturdy stone-headed pickaxe. Mines iron, coal and sandstone swiftly.",
                BlockType.StonePickaxe, 1, "⛏️",
                new IngredientRequirement(BlockType.Stone, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 8. 3 Stone + 2 Sticks -> Stone Axe (石斧)
            recipes.Add(new RecipeDefinition(
                id++, "Stone Axe", "石斧", "Tools",
                "Reinforced flint-edged woodcutter axe.",
                BlockType.StoneAxe, 1, "🪓",
                new IngredientRequirement(BlockType.Stone, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 9. 2 Stone + 1 Stick -> Stone Sword (石剑)
            recipes.Add(new RecipeDefinition(
                id++, "Stone Sword", "石剑", "Weapons",
                "Sharpened broadsword carved from subterranean granite.",
                BlockType.StoneSword, 1, "⚔️",
                new IngredientRequirement(BlockType.Stone, 2),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 10. 3 Iron Ore/Ingot + 2 Sticks -> Iron Pickaxe (铁镐)
            recipes.Add(new RecipeDefinition(
                id++, "Iron Pickaxe", "铁镐", "Tools",
                "High-durability forged iron pickaxe. Capable of cleaving gold and diamond seams.",
                BlockType.IronPickaxe, 1, "⛏️",
                new IngredientRequirement(BlockType.IronOre, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 11. 3 Iron Ore/Ingot + 2 Sticks -> Iron Axe (铁斧)
            recipes.Add(new RecipeDefinition(
                id++, "Iron Axe", "铁斧", "Tools",
                "Heavy iron battle axe and high-efficiency logging implement.",
                BlockType.IronAxe, 1, "🪓",
                new IngredientRequirement(BlockType.IronOre, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 12. 2 Iron Ore/Ingot + 1 Stick -> Iron Sword (铁剑)
            recipes.Add(new RecipeDefinition(
                id++, "Iron Sword", "铁剑", "Weapons",
                "Knightly iron longsword with lethal cutting edge.",
                BlockType.IronSword, 1, "⚔️",
                new IngredientRequirement(BlockType.IronOre, 2),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 13. 3 Diamond + 2 Sticks -> Diamond Pickaxe (钻石镐)
            recipes.Add(new RecipeDefinition(
                id++, "Diamond Pickaxe", "钻石镐", "Tools",
                "Legendary master pickaxe infused with crystallized diamond hardness. Breaks obsidian with ease.",
                BlockType.DiamondPickaxe, 1, "💎",
                new IngredientRequirement(BlockType.DiamondOre, 3),
                new IngredientRequirement(BlockType.Stick, 2)
            ));

            // 14. 2 Diamond + 1 Stick -> Diamond Sword (钻石剑)
            recipes.Add(new RecipeDefinition(
                id++, "Diamond Sword", "钻石剑", "Weapons",
                "Peerless crystalline blade glowing with azure radiance. Massive attack power.",
                BlockType.DiamondSword, 1, "💎",
                new IngredientRequirement(BlockType.DiamondOre, 2),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 15. 1 Coal Ore + 1 Stick -> 4 Torches (火把)
            recipes.Add(new RecipeDefinition(
                id++, "Torches (Coal)", "火把 (煤炭)", "Supplies",
                "Traditional coal-lit torches that dispel subterranean darkness.",
                BlockType.Torch, 4, "🔥",
                new IngredientRequirement(BlockType.CoalOre, 1),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 16. 1 Slime Gel + 1 Stick -> 4 Torches (凝胶火把)
            recipes.Add(new RecipeDefinition(
                id++, "Torches (Gel)", "火把 (凝胶)", "Supplies",
                "Terraria-style torch fueled by flammable slime gelatin.",
                BlockType.Torch, 4, "🔥",
                new IngredientRequirement(BlockType.Gel, 1),
                new IngredientRequirement(BlockType.Stick, 1)
            ));

            // 17. 4 Stone -> 4 Stone Bricks (石砖)
            recipes.Add(new RecipeDefinition(
                id++, "Stone Bricks", "石砖", "Blocks",
                "Polished masonry bricks for constructing castles, dungeons, and fortified bases.",
                BlockType.Brick, 4, "🧱",
                new IngredientRequirement(BlockType.Stone, 4)
            ));

            // 18. 1 Sand -> 1 Glass (玻璃)
            recipes.Add(new RecipeDefinition(
                id++, "Glass Block", "透明玻璃", "Blocks",
                "Heat-smelted desert sand cooled into crystalline transparent architectural glass.",
                BlockType.Glass, 1, "🪟",
                new IngredientRequirement(BlockType.Sand, 1)
            ));

            // 19. 1 Gel + 1 Torch -> 1 Sticky Bomb (粘性炸弹)
            recipes.Add(new RecipeDefinition(
                id++, "Sticky Bomb", "粘性炸弹", "Supplies",
                "Gel-coated demolition charge that clings steadfastly to vertical cavern walls.",
                BlockType.StickyBomb, 1, "💣",
                new IngredientRequirement(BlockType.Gel, 1),
                new IngredientRequirement(BlockType.Torch, 1)
            ));
        }

        public List<RecipeDefinition> GetFilteredRecipes()
        {
            if (string.IsNullOrEmpty(selectedCategory) || selectedCategory == "All")
            {
                return recipes;
            }
            List<RecipeDefinition> list = new List<RecipeDefinition>();
            for (int i = 0; i < recipes.Count; i++)
            {
                if (string.Equals(recipes[i].category, selectedCategory, StringComparison.OrdinalIgnoreCase))
                {
                    list.Add(recipes[i]);
                }
            }
            return list;
        }

        /// <summary>
        /// Executes crafting evaluation, deducts ingredients in Survival or grants freely in Creative,
        /// delivers crafted product into inventory, spawns celebration toast, and plays SFX_CRAFT fanfare.
        /// </summary>
        public bool CraftRecipe(RecipeDefinition recipe, int multiplier = 1)
        {
            if (recipe == null || multiplier <= 0) return false;
            _inventory = InventorySystem.Instance;
            if (_inventory == null) return false;

            bool isCreative = (WorldSettings.CurrentGameMode == GameMode.Creative);

            if (!isCreative)
            {
                if (!recipe.CanCraft(_inventory, false))
                {
                    PlayTone(220f, 0.15f, 0.4f);
                    if (UIManager.Instance != null)
                    {
                        UIManager.Instance.SpawnFloatingText(LocalizationManager.Get("✖ 材料不足", "✖ Missing Ingredients"), new Color(1.0f, 0.35f, 0.35f));
                    }
                    return false;
                }

                bool deducted = recipe.DeductIngredients(_inventory, multiplier);
                if (!deducted)
                {
                    return false;
                }
            }

            // Deliver output item into inventory
            int totalResult = recipe.resultCount * multiplier;
            bool added = _inventory.AddItem(recipe.resultItem, totalResult);

            // Celebration Fanfare and Toast Notification
            PlayCraftFanfareSound();

            string name = recipe.DisplayName;
            string toast = isCreative
                ? $"✨ {LocalizationManager.Get("[创造模式免费合成] ", "[Creative Free Craft] ")}{name} x{totalResult}!"
                : $"🎉 {LocalizationManager.Get("合成成功! ", "Crafted: ")}{name} x{totalResult}!";

            if (UIManager.Instance != null)
            {
                UIManager.Instance.SpawnFloatingText(toast, new Color(0.35f, 1.0f, 0.45f));
            }

            Debug.Log($"[CraftingManager] {toast} Delivered into inventory.");
            return true;
        }

        /// <summary>
        /// Generates procedural 4-tone ascending major fanfare audio (SFX_CRAFT) without external assets.
        /// Notes: C5 (523Hz) -> E5 (659Hz) -> G5 (784Hz) -> C6 (1046Hz).
        /// </summary>
        public void PlayCraftFanfareSound()
        {
            EnsureAudioSource();
            if (_audioSource == null) return;

            int sampleRate = 44100;
            float duration = 0.38f;
            int totalSamples = (int)(sampleRate * duration);
            float[] data = new float[totalSamples];

            float[] noteFreqs = new float[] { 523.25f, 659.25f, 783.99f, 1046.50f };
            float noteDuration = duration / noteFreqs.Length;

            for (int i = 0; i < totalSamples; i++)
            {
                float t = (float)i / sampleRate;
                int noteIndex = Mathf.Clamp((int)(t / noteDuration), 0, noteFreqs.Length - 1);
                float noteT = t - (noteIndex * noteDuration);

                float freq = noteFreqs[noteIndex];
                float env = Mathf.Exp(-noteT * 10f); // Quick decay per note
                if (noteIndex == noteFreqs.Length - 1)
                {
                    env = Mathf.Exp(-noteT * 5f); // Lingering final note chime
                }

                // Rich harmonic blend (fundamental + overtone shimmer)
                float wave = Mathf.Sin(2f * Mathf.PI * freq * noteT) * 0.7f
                           + Mathf.Sin(4f * Mathf.PI * freq * noteT) * 0.25f
                           + Mathf.Sin(6f * Mathf.PI * freq * noteT) * 0.05f;

                data[i] = wave * env * 0.35f;
            }

            AudioClip clip = AudioClip.Create("SFX_CRAFT", totalSamples, 1, sampleRate, false);
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
                float env = Mathf.Exp(-t * 25f);
                data[i] = Mathf.Sin(2f * Mathf.PI * freq * t) * env * volume;
            }

            AudioClip clip = AudioClip.Create("Tone", totalSamples, 1, sampleRate, false);
            clip.SetData(data, 0);
            _audioSource.PlayOneShot(clip);
        }

        private void EnsureAudioSource()
        {
            if (_audioSource == null)
            {
                _audioSource = GetComponent<AudioSource>();
                if (_audioSource == null)
                {
                    _audioSource = gameObject.AddComponent<AudioSource>();
                    _audioSource.playOnAwake = false;
                    _audioSource.spatialBlend = 0f; // 2D flat audio
                }
            }
        }

        // =====================================================================
        // Procedural GUI Textures & Style Initialization
        // =====================================================================

        private void GenerateTextures()
        {
            if (_panelBgTex != null) return;

            _panelBgTex = MakeColorTex(new Color(0.08f, 0.10f, 0.14f, 0.96f));
            _cardBgTex = MakeColorTex(new Color(0.14f, 0.16f, 0.22f, 0.90f));
            _selectedCardBgTex = MakeColorTex(new Color(0.24f, 0.28f, 0.38f, 0.95f));
            _btnNormalTex = MakeColorTex(new Color(0.20f, 0.22f, 0.28f, 1.0f));
            _btnHoverTex = MakeColorTex(new Color(0.35f, 0.38f, 0.48f, 1.0f));
            _btnActiveTex = MakeColorTex(new Color(0.50f, 0.42f, 0.18f, 1.0f));
            _slotBgTex = MakeColorTex(new Color(0.12f, 0.14f, 0.18f, 0.92f));
        }

        private Texture2D MakeColorTex(Color color)
        {
            Texture2D tex = new Texture2D(2, 2, TextureFormat.RGBA32, false);
            Color[] pix = new Color[4];
            for (int i = 0; i < 4; i++) pix[i] = color;
            tex.SetPixels(pix);
            tex.Apply();
            return tex;
        }

        private void InitStyles()
        {
            if (_stylesInitialized) return;
            GenerateTextures();

            _panelStyle = new GUIStyle(GUI.skin.box)
            {
                normal = { background = _panelBgTex },
                padding = new RectOffset(16, 16, 16, 16)
            };

            _titleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(1.0f, 0.88f, 0.35f) }
            };

            _subTitleStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                fontStyle = FontStyle.Normal,
                normal = { textColor = new Color(0.75f, 0.85f, 0.95f) }
            };

            _tabBtnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 13,
                fontStyle = FontStyle.Bold,
                normal = { background = _btnNormalTex, textColor = new Color(0.80f, 0.80f, 0.80f) },
                hover = { background = _btnHoverTex, textColor = Color.white }
            };

            _activeTabBtnStyle = new GUIStyle(_tabBtnStyle)
            {
                normal = { background = _btnActiveTex, textColor = new Color(1.0f, 0.90f, 0.40f) }
            };

            _categoryBtnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 11,
                fontStyle = FontStyle.Bold,
                normal = { background = _btnNormalTex, textColor = new Color(0.85f, 0.85f, 0.85f) },
                hover = { background = _btnHoverTex, textColor = Color.white }
            };

            _activeCategoryBtnStyle = new GUIStyle(_categoryBtnStyle)
            {
                normal = { background = _btnActiveTex, textColor = new Color(1.0f, 0.90f, 0.40f) }
            };

            _recipeCardStyle = new GUIStyle(GUI.skin.button)
            {
                alignment = TextAnchor.MiddleLeft,
                fontSize = 12,
                fontStyle = FontStyle.Normal,
                padding = new RectOffset(10, 10, 6, 6),
                normal = { background = _cardBgTex, textColor = Color.white },
                hover = { background = _btnHoverTex, textColor = new Color(1.0f, 0.95f, 0.60f) }
            };

            _selectedRecipeCardStyle = new GUIStyle(_recipeCardStyle)
            {
                fontStyle = FontStyle.Bold,
                normal = { background = _selectedCardBgTex, textColor = new Color(1.0f, 0.88f, 0.25f) }
            };

            _craftBtnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 15,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { background = _btnActiveTex, textColor = new Color(1.0f, 0.95f, 0.50f) },
                hover = { background = _btnHoverTex, textColor = Color.white }
            };

            _itemLabelStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                normal = { textColor = Color.white }
            };

            _ingredientSatisfiedStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(0.35f, 0.95f, 0.45f) }
            };

            _ingredientMissingStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(1.0f, 0.35f, 0.35f) }
            };

            _stylesInitialized = true;
        }

        // =====================================================================
        // Immediate-Mode OnGUI Rendering
        // =====================================================================

        private void OnGUI()
        {
            if (!isOpen) return;

            InitStyles();

            // Fullscreen translucent dark vignette backdrop
            GUI.color = new Color(0f, 0f, 0f, 0.60f);
            GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture);
            GUI.color = Color.white;

            float modalW = 840f;
            float modalH = 560f;
            float modalX = (Screen.width - modalW) * 0.5f;
            float modalY = (Screen.height - modalH) * 0.5f;
            Rect modalRect = new Rect(modalX, modalY, modalW, modalH);

            // Master modal background
            GUI.Box(modalRect, GUIContent.none, _panelStyle);

            // Modal Header
            DrawHeader(modalRect);

            // Content Area based on current active tab
            Rect contentRect = new Rect(modalX + 16f, modalY + 68f, modalW - 32f, modalH - 84f);
            if (currentTab == CraftingModalTab.Crafting)
            {
                DrawCraftingContent(contentRect);
            }
            else
            {
                DrawInventoryContent(contentRect);
            }
        }

        private void DrawHeader(Rect modalRect)
        {
            // Title & Mode Banner
            bool isCreative = (WorldSettings.CurrentGameMode == GameMode.Creative);
            string modeStr = isCreative 
                ? LocalizationManager.Get("[创造模式: 免费合成]", "[CREATIVE MODE: Free Crafting]") 
                : LocalizationManager.Get("[生存模式: 原料合成]", "[SURVIVAL MODE: Resource Crafting]");
            Color modeCol = isCreative ? new Color(0.4f, 0.85f, 1.0f) : new Color(0.35f, 0.95f, 0.45f);

            GUI.Label(new Rect(modalRect.x + 20f, modalRect.y + 14f, 380f, 26f), LocalizationManager.Get("⚒️ 合成站", "⚒️ CRAFTING STATION"), _titleStyle);
            GUI.color = modeCol;
            GUI.Label(new Rect(modalRect.x + 20f, modalRect.y + 38f, 380f, 20f), modeStr, _subTitleStyle);
            GUI.color = Color.white;

            // Top Navigation Tabs: [CRAFTING] and [INVENTORY]
            float tabW = 140f;
            float tabH = 32f;
            float tabX = modalRect.x + 390f;
            float tabY = modalRect.y + 18f;

            if (GUI.Button(new Rect(tabX, tabY, tabW, tabH), LocalizationManager.Get("🔨 合成", "🔨 Crafting"), currentTab == CraftingModalTab.Crafting ? _activeTabBtnStyle : _tabBtnStyle))
            {
                currentTab = CraftingModalTab.Crafting;
                PlayTone(520f, 0.05f, 0.2f);
            }

            if (GUI.Button(new Rect(tabX + tabW + 10f, tabY, tabW, tabH), LocalizationManager.Get("🎒 背包", "🎒 Inventory"), currentTab == CraftingModalTab.Inventory ? _activeTabBtnStyle : _tabBtnStyle))
            {
                currentTab = CraftingModalTab.Inventory;
                PlayTone(560f, 0.05f, 0.2f);
            }

            // Close Button [X]
            if (GUI.Button(new Rect(modalRect.x + modalRect.width - 42f, modalRect.y + 14f, 28f, 28f), "✕", _tabBtnStyle))
            {
                CloseModal();
            }
        }

        private void DrawCraftingContent(Rect contentRect)
        {
            // Category Filter Toolbar
            float catW = 100f;
            float catH = 26f;
            for (int i = 0; i < Categories.Length; i++)
            {
                string cat = Categories[i];
                string catName = cat == "All" ? LocalizationManager.Get("全部", "All") :
                                 cat == "Tools" ? LocalizationManager.Get("工具", "Tools") :
                                 cat == "Weapons" ? LocalizationManager.Get("武器", "Weapons") :
                                 cat == "Blocks" ? LocalizationManager.Get("建筑", "Blocks") : LocalizationManager.Get("消耗", "Supplies");

                bool isCatActive = string.Equals(selectedCategory, cat, StringComparison.OrdinalIgnoreCase);
                if (GUI.Button(new Rect(contentRect.x + i * (catW + 6f), contentRect.y, catW, catH), catName, isCatActive ? _activeCategoryBtnStyle : _categoryBtnStyle))
                {
                    selectedCategory = cat;
                    selectedRecipeIndex = 0;
                    PlayTone(460f + i * 30f, 0.05f, 0.2f);
                }
            }

            float listTop = contentRect.y + 36f;
            float listW = 340f;
            float listH = contentRect.height - 38f;
            Rect listRect = new Rect(contentRect.x, listTop, listW, listH);

            // Left Column: Scrollable Recipe List
            DrawRecipeList(listRect);

            // Right Column: Result Preview & Craft Action Pane
            float previewX = contentRect.x + listW + 16f;
            float previewW = contentRect.width - listW - 16f;
            float previewH = listH;
            Rect previewRect = new Rect(previewX, listTop, previewW, previewH);

            DrawPreviewAndCraftPane(previewRect);
        }

        private void DrawRecipeList(Rect listRect)
        {
            List<RecipeDefinition> filtered = GetFilteredRecipes();
            _inventory = InventorySystem.Instance;
            bool isCreative = (WorldSettings.CurrentGameMode == GameMode.Creative);

            float itemHeight = 44f;
            float totalViewH = filtered.Count * (itemHeight + 4f);
            Rect viewRect = new Rect(0, 0, listRect.width - 20f, Mathf.Max(totalViewH, listRect.height));

            _recipeScrollPos = GUI.BeginScrollView(listRect, _recipeScrollPos, viewRect, false, true);

            for (int i = 0; i < filtered.Count; i++)
            {
                RecipeDefinition r = filtered[i];
                bool isSelected = (i == selectedRecipeIndex);
                bool canCraft = r.CanCraft(_inventory, isCreative);

                float cardY = i * (itemHeight + 4f);
                Rect cardRect = new Rect(0, cardY, viewRect.width, itemHeight);

                string badge = canCraft 
                    ? LocalizationManager.Get("<color=#55FF55>[✔ 可合成]</color>", "<color=#55FF55>[✔ Craftable]</color>") 
                    : LocalizationManager.Get("<color=#888888>[✖ 缺材料]</color>", "<color=#888888>[✖ Missing]</color>");
                string labelText = $"{r.iconEmoji} {r.DisplayName} x{r.resultCount}  {badge}";

                if (GUI.Button(cardRect, labelText, isSelected ? _selectedRecipeCardStyle : _recipeCardStyle))
                {
                    selectedRecipeIndex = i;
                    PlayTone(500f, 0.04f, 0.25f);
                }
            }

            GUI.EndScrollView();
        }

        private void DrawPreviewAndCraftPane(Rect previewRect)
        {
            List<RecipeDefinition> filtered = GetFilteredRecipes();
            if (filtered.Count == 0 || selectedRecipeIndex < 0 || selectedRecipeIndex >= filtered.Count)
            {
                GUI.Label(new Rect(previewRect.x + 20f, previewRect.y + 40f, previewRect.width - 40f, 30f), LocalizationManager.Get("请从左侧选择要合成的配方。", "Please select a recipe from the left."), _subTitleStyle);
                return;
            }

            RecipeDefinition r = filtered[selectedRecipeIndex];
            _inventory = InventorySystem.Instance;
            bool isCreative = (WorldSettings.CurrentGameMode == GameMode.Creative);
            bool canCraft = r.CanCraft(_inventory, isCreative);
            int maxCraft = r.GetMaxCraftableCount(_inventory, isCreative);

            // Preview Box Frame
            GUI.Box(previewRect, GUIContent.none, _panelStyle);

            float curY = previewRect.y + 16f;

            // Product Title & Icon Banner
            string productTitle = $"{r.iconEmoji} {r.DisplayName}  x{r.resultCount}";
            GUI.Label(new Rect(previewRect.x + 20f, curY, previewRect.width - 40f, 28f), productTitle, _titleStyle);
            curY += 28f;

            // Category & Output Item Tag
            string resultItemName = BlockRegistry.GetBlockName(r.resultItem);
            string tagText = $"{LocalizationManager.Get("分类", "Category")}: {r.DisplayCategory}  |  {LocalizationManager.Get("产物", "Product")}: {resultItemName} (x{r.resultCount})";
            GUI.Label(new Rect(previewRect.x + 20f, curY, previewRect.width - 40f, 20f), tagText, _subTitleStyle);
            curY += 24f;

            // Description
            GUI.Label(new Rect(previewRect.x + 20f, curY, previewRect.width - 40f, 44f), r.description, _itemLabelStyle);
            curY += 48f;

            // Divider Line
            GUI.color = new Color(0.35f, 0.40f, 0.50f, 0.8f);
            GUI.DrawTexture(new Rect(previewRect.x + 16f, curY, previewRect.width - 32f, 2f), Texture2D.whiteTexture);
            GUI.color = Color.white;
            curY += 12f;

            // Ingredients Header
            GUI.Label(new Rect(previewRect.x + 20f, curY, previewRect.width - 40f, 22f), LocalizationManager.Get("📋 所需材料与现有库存:", "📋 Ingredients Required:"), _titleStyle);
            curY += 26f;

            // List each ingredient with owned / needed status
            for (int i = 0; i < r.ingredients.Length; i++)
            {
                IngredientRequirement req = r.ingredients[i];
                int owned = r.GetPlayerItemCount(_inventory, req.itemType);
                bool hasEnough = (owned >= req.amount) || isCreative;

                string ingName = BlockRegistry.GetBlockName(req.itemType);
                string ingStatus = hasEnough 
                    ? LocalizationManager.Get("充足", "Ready") 
                    : LocalizationManager.Get("缺少", "Missing");

                string ingText = hasEnough
                    ? $"  ✔ {ingName}: {owned} / {req.amount} ({ingStatus})"
                    : $"  ✖ {ingName}: {owned} / {req.amount} ({ingStatus})";

                GUIStyle ingStyle = hasEnough ? _ingredientSatisfiedStyle : _ingredientMissingStyle;
                GUI.Label(new Rect(previewRect.x + 20f, curY, previewRect.width - 40f, 22f), ingText, ingStyle);
                curY += 24f;
            }

            curY += 14f;

            // Craft Action Buttons Area
            float btnW = previewRect.width - 40f;
            float btnH = 46f;
            string craftBtnLabel = isCreative
                ? $"✨ {LocalizationManager.Get("免费合成", "Free Craft")} x{r.resultCount}"
                : (canCraft ? $"🔨 {LocalizationManager.Get("立即合成", "Craft")} x{r.resultCount}" : LocalizationManager.Get("✖ 材料不足 无法合成", "✖ Missing Ingredients"));

            GUI.enabled = canCraft || isCreative;
            if (GUI.Button(new Rect(previewRect.x + 20f, curY, btnW, btnH), craftBtnLabel, _craftBtnStyle))
            {
                CraftRecipe(r, 1);
            }
            GUI.enabled = true;

            curY += btnH + 10f;

            // Bulk Crafting: Craft x5 and Craft Max
            float smallBtnW = (btnW - 10f) * 0.5f;
            float smallBtnH = 30f;

            GUI.enabled = (canCraft && maxCraft >= 5) || isCreative;
            if (GUI.Button(new Rect(previewRect.x + 20f, curY, smallBtnW, smallBtnH), LocalizationManager.Get("批量合成 x5", "Craft x5"), _tabBtnStyle))
            {
                CraftRecipe(r, 5);
            }

            GUI.enabled = (canCraft && maxCraft > 0) || isCreative;
            int craftAllAmount = isCreative ? 10 : Mathf.Min(maxCraft, 64);
            if (GUI.Button(new Rect(previewRect.x + 20f + smallBtnW + 10f, curY, smallBtnW, smallBtnH), $"{LocalizationManager.Get("全部合成", "Craft All")} (x{craftAllAmount})", _tabBtnStyle))
            {
                CraftRecipe(r, craftAllAmount);
            }
            GUI.enabled = true;

            // Bottom Shortcuts Reminder
            float footerY = previewRect.y + previewRect.height - 30f;
            GUI.Label(new Rect(previewRect.x + 20f, footerY, previewRect.width - 40f, 20f),
                LocalizationManager.Get("快捷键: [C] 开关合成 | [E] 开关背包 | [↑/↓] 选择配方 | [回车] 立即合成 | [ESC] 关闭",
                                        "Hotkeys: [C] Toggle Crafting | [E] Backpack | [↑/↓] Select | [Enter] Craft | [ESC] Close"), _subTitleStyle);
        }

        private void DrawInventoryContent(Rect contentRect)
        {
            _inventory = InventorySystem.Instance;
            if (_inventory == null)
            {
                GUI.Label(new Rect(contentRect.x + 20f, contentRect.y + 20f, 300f, 30f), LocalizationManager.Get("物品栏未初始化", "Inventory not ready."), _titleStyle);
                return;
            }

            // Right-click anywhere in modal clears picked-up moving slot
            if (Event.current.type == EventType.MouseDown && Event.current.button == 1)
            {
                _inventory.ClearSelectedSlot();
            }

            float startX = contentRect.x + 10f;
            float startY = contentRect.y + 10f;

            GUI.Label(new Rect(startX, startY, 400f, 24f), LocalizationManager.Get("🎒 玩家全部物品栏 (9 快捷栏 + 27 背包):", "🎒 Player Storage (9 Hotbar + 27 Backpack):"), _titleStyle);
            
            // Instruction tip
            GUI.Label(new Rect(startX + 410f, startY + 2f, 380f, 20f), 
                LocalizationManager.Get("💡 单击槽位拿起 / 再次单击调换或合并 [右键取消]", "💡 Click to pick up / Click another to swap or stack [RMB cancel]"), _subTitleStyle);

            startY += 30f;

            // 1. Hotbar (Slots 0 to 8)
            GUI.Label(new Rect(startX, startY, 200f, 20f), LocalizationManager.Get("★ 快捷栏 (1-9):", "★ Active Hotbar (1-9):"), _subTitleStyle);
            startY += 22f;

            float slotSize = 64f;
            float slotGap = 8f;
            Vector2 mousePos = Event.current.mousePosition;

            for (int i = 0; i < InventorySystem.HotbarSlotCount; i++)
            {
                InventorySlot slot = _inventory.GetSlot(i);
                float sx = startX + i * (slotSize + slotGap);
                Rect slotRect = new Rect(sx, startY, slotSize, slotSize);

                bool isActive = (i == _inventory.ActiveSlotIndex);
                bool isSelected = (_inventory.SelectedSlotForMove == i);
                bool isHovered = slotRect.Contains(mousePos);

                GUI.Box(slotRect, GUIContent.none, _panelStyle);

                if (isSelected)
                {
                    float pulse = 0.5f + Mathf.Sin(Time.time * 8f) * 0.5f;
                    DrawBorder(slotRect, 2, new Color(0.35f, 0.85f, 1.0f, pulse));
                }
                else if (isActive)
                {
                    DrawBorder(slotRect, 2, new Color(1.0f, 0.85f, 0.25f, 1.0f));
                }
                else if (isHovered)
                {
                    DrawBorder(slotRect, 1, new Color(0.8f, 0.8f, 0.9f, 0.8f));
                }

                GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 2, 20f, 16f), $"{i + 1}", _subTitleStyle);

                if (slot != null && !slot.IsEmpty)
                {
                    string nameShort = BlockRegistry.GetBlockName(slot.type);
                    if (nameShort.Length > 7) nameShort = nameShort.Substring(0, 6) + "..";
                    GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 22, slotSize - 8, 18f), nameShort, _itemLabelStyle);
                    GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 42, slotSize - 8, 18f), $"x{slot.count}", _ingredientSatisfiedStyle);
                }

                if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                {
                    _inventory.ClickSlot(i);
                    PlayTone(600f, 0.04f, 0.2f);
                }
            }

            startY += slotSize + 24f;

            // 2. Backpack Grid (3 rows x 9 columns = 27 slots)
            GUI.Label(new Rect(startX, startY, 300f, 20f), LocalizationManager.Get("📦 背包主储物区 (27 格):", "📦 Backpack Storage (27 Slots):"), _subTitleStyle);
            startY += 22f;

            for (int row = 0; row < 3; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    int slotIdx = InventorySystem.HotbarSlotCount + (row * 9 + col);
                    InventorySlot slot = _inventory.GetSlot(slotIdx);

                    float sx = startX + col * (slotSize + slotGap);
                    float sy = startY + row * (slotSize + slotGap);
                    Rect slotRect = new Rect(sx, sy, slotSize, slotSize);

                    bool isSelected = (_inventory.SelectedSlotForMove == slotIdx);
                    bool isHovered = slotRect.Contains(mousePos);

                    GUI.Box(slotRect, GUIContent.none, _panelStyle);

                    if (isSelected)
                    {
                        float pulse = 0.5f + Mathf.Sin(Time.time * 8f) * 0.5f;
                        DrawBorder(slotRect, 2, new Color(0.35f, 0.85f, 1.0f, pulse));
                    }
                    else if (isHovered)
                    {
                        DrawBorder(slotRect, 1, new Color(0.8f, 0.8f, 0.9f, 0.8f));
                    }

                    if (slot != null && !slot.IsEmpty)
                    {
                        string nameShort = BlockRegistry.GetBlockName(slot.type);
                        if (nameShort.Length > 7) nameShort = nameShort.Substring(0, 6) + "..";
                        GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 20, slotSize - 8, 18f), nameShort, _itemLabelStyle);
                        GUI.Label(new Rect(slotRect.x + 4, slotRect.y + 40, slotSize - 8, 18f), $"x{slot.count}", _ingredientSatisfiedStyle);
                    }

                    if (GUI.Button(slotRect, GUIContent.none, GUIStyle.none))
                    {
                        _inventory.ClickSlot(slotIdx);
                        PlayTone(600f, 0.04f, 0.2f);
                    }
                }
            }

            // Floating preview of picked up slot
            if (_inventory.SelectedSlotForMove >= 0 && _inventory.SelectedSlotForMove < InventorySystem.TotalSlotCount)
            {
                InventorySlot selSlot = _inventory.GetSlot(_inventory.SelectedSlotForMove);
                if (selSlot != null && !selSlot.IsEmpty)
                {
                    Rect floatRect = new Rect(mousePos.x + 12f, mousePos.y + 12f, 100f, 36f);
                    GUI.Box(floatRect, GUIContent.none, _selectedRecipeCardStyle);
                    DrawBorder(floatRect, 1, new Color(0.35f, 0.85f, 1.0f));
                    string bName = BlockRegistry.GetBlockName(selSlot.type);
                    if (bName.Length > 8) bName = bName.Substring(0, 7) + "..";
                    GUI.Label(new Rect(floatRect.x + 6, floatRect.y + 2, floatRect.width - 12, 16f), bName, _subTitleStyle);
                    GUI.Label(new Rect(floatRect.x + 6, floatRect.y + 18, floatRect.width - 12, 16f), $"x{selSlot.count}", _ingredientSatisfiedStyle);
                }
            }

            // Quick Crafting button inside inventory tab
            float craftBtnY = startY + 3 * (slotSize + slotGap) + 16f;
            if (GUI.Button(new Rect(startX, craftBtnY, 260f, 36f), LocalizationManager.Get("🔨 切换至合成站", "🔨 Switch to Crafting"), _craftBtnStyle))
            {
                currentTab = CraftingModalTab.Crafting;
                PlayTone(580f, 0.05f, 0.2f);
            }
        }

        private void DrawBorder(Rect rect, int thickness, Color color)
        {
            GUI.color = color;
            GUI.DrawTexture(new Rect(rect.x, rect.y, rect.width, thickness), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(rect.x, rect.y + rect.height - thickness, rect.width, thickness), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(rect.x, rect.y, thickness, rect.height), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(rect.x + rect.width - thickness, rect.y, thickness, rect.height), Texture2D.whiteTexture);
            GUI.color = Color.white;
        }
    }
}
