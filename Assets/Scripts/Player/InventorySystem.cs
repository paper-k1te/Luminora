using System;
using System.Collections.Generic;
using Core;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Represents an individual item stack in the inventory grid.
    /// </summary>
    [Serializable]
    public class InventorySlot
    {
        public BlockType type;
        public int count;
        public bool isTool;
        public ToolID toolId;

        public bool IsEmpty => (type == BlockType.Air && !isTool) || count <= 0;

        public InventorySlot()
        {
            type = BlockType.Air;
            count = 0;
            isTool = false;
            toolId = ToolID.None;
        }

        public InventorySlot(BlockType type, int count)
        {
            this.type = type;
            this.count = count;
            this.isTool = false;
            this.toolId = ToolID.None;
        }

        public InventorySlot(ToolID toolId)
        {
            this.type = BlockType.Air;
            this.count = 1;
            this.isTool = true;
            this.toolId = toolId;
        }

        public void Clear()
        {
            type = BlockType.Air;
            count = 0;
            isTool = false;
            toolId = ToolID.None;
        }

        public override string ToString()
        {
            if (IsEmpty) return "Empty";
            return isTool ? $"{toolId}" : $"{type} x{count}";
        }
    }

    /// <summary>
    /// Master inventory and hotbar management system for the 2.5D player.
    /// Manages 9 active hotbar slots, backpack storage, active item switching,
    /// stack merging, and initial equipment provisioning.
    /// </summary>
    [DisallowMultipleComponent]
    public class InventorySystem : MonoBehaviour
    {
        public static InventorySystem Instance { get; private set; }

        public const int HotbarSlotCount = 9;
        public const int BackpackSlotCount = 27;
        public const int TotalSlotCount = HotbarSlotCount + BackpackSlotCount; // 36 slots
        public const int MaxStackSize = 999;

        [Header("State")]
        [SerializeField] private int activeSlotIndex = 0;
        [SerializeField] private int selectedSlotForMove = -1;
        [SerializeField] private InventorySlot[] slots;

        // Events
        public event Action<int> OnActiveSlotChanged;
        public event Action OnInventoryChanged;
        public static event Action<BlockType, int> OnItemAdded;

        public int ActiveSlotIndex => activeSlotIndex;
        public int SlotCount => slots != null ? slots.Length : 0;
        public int SelectedSlotForMove => selectedSlotForMove;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            InitializeSlots();
        }

        private void Start()
        {
            InitializeSlots();

            // Check current game mode:
            // In Survival mode (GameMode.Survival), start from 0 items (completely empty, starting from scratch).
            // Do not pre-populate diamond pickaxes or stacks of blocks.
            GameMode currentMode = GameMode.Survival;
            PlayerController2D5 player = GetComponent<PlayerController2D5>() ?? FindObjectOfType<PlayerController2D5>();
            if (player != null)
            {
                currentMode = player.CurrentGameMode;
            }
            else
            {
                currentMode = WorldSettings.CurrentGameMode;
            }

            if (currentMode == GameMode.Creative && IsInventoryEmpty())
            {
                ProvisionStarterKit();
            }
            else if (currentMode == GameMode.Survival)
            {
                // Ensure inventory starts completely empty in survival mode
                if (IsInventoryEmpty())
                {
                    ClearAllSlots();
                }
            }
        }

        private void Update()
        {
            HandleSlotSwitchInput();
        }

        /// <summary>
        /// Allocates storage slots if not already allocated in inspector.
        /// </summary>
        private void InitializeSlots()
        {
            if (slots == null || slots.Length != TotalSlotCount)
            {
                slots = new InventorySlot[TotalSlotCount];
                for (int i = 0; i < TotalSlotCount; i++)
                {
                    slots[i] = new InventorySlot();
                }
            }
        }

        /// <summary>
        /// Reads keyboard number keys (1-9) and mouse wheel scroll delta to switch active hotbar slot.
        /// </summary>
        private void HandleSlotSwitchInput()
        {
            // Direct hotkey selection (1 through 9)
            for (int i = 0; i < HotbarSlotCount; i++)
            {
                if (Input.GetKeyDown(KeyCode.Alpha1 + i))
                {
                    SetActiveSlot(i);
                    return;
                }
            }

            // Mouse scroll wheel navigation
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            if (Mathf.Abs(scroll) > 0.01f)
            {
                if (scroll > 0f)
                {
                    // Scroll Up -> Previous slot
                    int prevSlot = (activeSlotIndex - 1 + HotbarSlotCount) % HotbarSlotCount;
                    SetActiveSlot(prevSlot);
                }
                else if (scroll < 0f)
                {
                    // Scroll Down -> Next slot
                    int nextSlot = (activeSlotIndex + 1) % HotbarSlotCount;
                    SetActiveSlot(nextSlot);
                }
            }
        }

        /// <summary>
        /// Switches active hotbar slot index with bounds checking and event notification.
        /// </summary>
        public void SetActiveSlot(int index)
        {
            index = Mathf.Clamp(index, 0, HotbarSlotCount - 1);
            if (activeSlotIndex != index)
            {
                activeSlotIndex = index;
                OnActiveSlotChanged?.Invoke(activeSlotIndex);
            }
        }

        /// <summary>
        /// Retrieves the currently active hotbar item slot.
        /// </summary>
        public InventorySlot GetActiveItem()
        {
            return GetSlot(activeSlotIndex);
        }

        /// <summary>
        /// Retrieves the block type held in the active hotbar slot, or Air if empty.
        /// </summary>
        public BlockType GetActiveBlockType()
        {
            InventorySlot active = GetActiveItem();
            return active != null && !active.IsEmpty ? active.type : BlockType.Air;
        }

        /// <summary>
        /// Retrieves the ToolData held in active hotbar slot, or null if holding a block or empty.
        /// </summary>
        public ToolData GetActiveTool()
        {
            InventorySlot active = GetActiveItem();
            return (active != null && active.isTool) ? ToolRegistry.Get(active.toolId) : null;
        }

        /// <summary>
        /// Retrieves the slot at the specified global index [0, TotalSlotCount - 1].
        /// </summary>
        public InventorySlot GetSlot(int index)
        {
            if (slots == null || index < 0 || index >= slots.Length)
            {
                return null;
            }
            return slots[index];
        }

        /// <summary>
        /// Adds a quantity of a block type into inventory:
        /// First attempts to stack with existing matching slots up to 999.
        /// Then places remainder into the first available empty slot.
        /// Returns true if all or part of the quantity was successfully added.
        /// </summary>
        public bool AddItem(BlockType type, int count)
        {
            if (type == BlockType.Air || count <= 0) return false;

            InitializeSlots();
            int remaining = count;

            // Pass 1: Stack with existing slots containing the same block type
            for (int i = 0; i < slots.Length && remaining > 0; i++)
            {
                if (!slots[i].IsEmpty && slots[i].type == type)
                {
                    int spaceAvailable = MaxStackSize - slots[i].count;
                    if (spaceAvailable > 0)
                    {
                        int toAdd = Mathf.Min(spaceAvailable, remaining);
                        slots[i].count += toAdd;
                        remaining -= toAdd;
                    }
                }
            }

            // Pass 2: Place remaining into first empty slots
            for (int i = 0; i < slots.Length && remaining > 0; i++)
            {
                if (slots[i].IsEmpty)
                {
                    int toAdd = Mathf.Min(MaxStackSize, remaining);
                    slots[i].type = type;
                    slots[i].count = toAdd;
                    remaining -= toAdd;
                }
            }

            bool addedAny = (remaining < count);
            if (addedAny)
            {
                OnInventoryChanged?.Invoke();
                OnItemAdded?.Invoke(type, count - remaining);
            }

            return remaining == 0;
        }

        /// <summary>
        /// Removes or decrements item quantity from a specified slot index.
        /// Clears slot to Air when count reaches zero.
        /// </summary>
        public bool RemoveItem(int slotIndex, int count)
        {
            if (slots == null || slotIndex < 0 || slotIndex >= slots.Length || count <= 0)
            {
                return false;
            }

            InventorySlot slot = slots[slotIndex];
            if (slot.IsEmpty) return false;

            if (slot.count <= count)
            {
                slot.Clear();
            }
            else
            {
                slot.count -= count;
            }

            OnInventoryChanged?.Invoke();
            return true;
        }

        /// <summary>
        /// Consumes 1 or more items from the currently active hotbar slot.
        /// Used directly by block placement mechanics.
        /// </summary>
        public bool ConsumeActiveItem(int count = 1)
        {
            return RemoveItem(activeSlotIndex, count);
        }

        /// <summary>
        /// Checks whether all inventory slots are currently empty.
        /// </summary>
        public bool IsInventoryEmpty()
        {
            if (slots == null) return true;
            for (int i = 0; i < slots.Length; i++)
            {
                if (!slots[i].IsEmpty) return false;
            }
            return true;
        }

        /// <summary>
        /// Initializes the player's hotbar with a rich starter kit for sandbox building and testing.
        /// </summary>
        public void ProvisionStarterKit()
        {
            InitializeSlots();

            // Hotbar starter tools and building blocks
            slots[0] = new InventorySlot(ToolID.DiamondPickaxe);
            slots[1] = new InventorySlot(ToolID.DiamondSword);
            slots[2] = new InventorySlot(BlockType.Dirt, 64);
            slots[3] = new InventorySlot(BlockType.Wood, 32);
            slots[4] = new InventorySlot(BlockType.WoodPlank, 32);
            slots[5] = new InventorySlot(BlockType.Stone, 32);
            slots[6] = new InventorySlot(BlockType.Torch, 16);
            slots[7] = new InventorySlot(BlockType.Glass, 16);
            slots[8] = new InventorySlot(BlockType.Brick, 16);

            OnInventoryChanged?.Invoke();
        }

        /// <summary>
        /// Clears all slots in inventory (used for starting survival from scratch or resetting).
        /// </summary>
        public void ClearAllSlots()
        {
            InitializeSlots();
            for (int i = 0; i < slots.Length; i++)
            {
                slots[i].Clear();
            }
            selectedSlotForMove = -1;
            OnInventoryChanged?.Invoke();
        }

        /// <summary>
        /// Selects a specific slot for moving / swapping.
        /// </summary>
        public void SelectSlotForMove(int index)
        {
            if (index >= 0 && index < TotalSlotCount)
            {
                selectedSlotForMove = index;
            }
        }

        /// <summary>
        /// Clears the currently selected move slot.
        /// </summary>
        public void ClearSelectedSlot()
        {
            selectedSlotForMove = -1;
        }

        /// <summary>
        /// Handles click-to-swap logic across any slots:
        /// 1. If nothing held, click a non-empty slot to pick it up.
        /// 2. If clicking the same slot, drop / deselect it.
        /// 3. If clicking another slot, call SwapSlots to swap or merge stacks.
        /// </summary>
        public void ClickSlot(int slotIndex)
        {
            if (slotIndex < 0 || slotIndex >= TotalSlotCount) return;

            InitializeSlots();

            if (selectedSlotForMove == -1)
            {
                if (slots[slotIndex] != null && !slots[slotIndex].IsEmpty)
                {
                    selectedSlotForMove = slotIndex;
                }
            }
            else
            {
                if (selectedSlotForMove == slotIndex)
                {
                    selectedSlotForMove = -1;
                }
                else
                {
                    SwapSlots(selectedSlotForMove, slotIndex);
                    if (slots[selectedSlotForMove] == null || slots[selectedSlotForMove].IsEmpty)
                    {
                        selectedSlotForMove = -1;
                    }
                }
            }
        }

        /// <summary>
        /// Swaps the contents of two slots (e.g. for inventory drag-and-drop, hotbar/backpack reordering, or sorting).
        /// If both slots hold the same block type and neither is a tool, combines stacks up to MaxStackSize.
        /// </summary>
        public void SwapSlots(int indexA, int indexB)
        {
            if (slots == null || indexA < 0 || indexA >= slots.Length || indexB < 0 || indexB >= slots.Length)
            {
                return;
            }
            if (indexA == indexB) return;

            InventorySlot slotA = slots[indexA];
            InventorySlot slotB = slots[indexB];

            if (slotA == null) slotA = slots[indexA] = new InventorySlot();
            if (slotB == null) slotB = slots[indexB] = new InventorySlot();

            // Try stack combination if same block type and neither is a tool
            if (!slotA.IsEmpty && !slotB.IsEmpty && !slotA.isTool && !slotB.isTool && slotA.type == slotB.type)
            {
                int spaceAvailable = MaxStackSize - slotB.count;
                if (spaceAvailable > 0)
                {
                    int transfer = Mathf.Min(spaceAvailable, slotA.count);
                    slotB.count += transfer;
                    slotA.count -= transfer;
                    if (slotA.count <= 0)
                    {
                        slotA.Clear();
                    }
                    OnInventoryChanged?.Invoke();
                    return;
                }
            }

            // Otherwise perform standard swap
            slots[indexA] = slotB;
            slots[indexB] = slotA;

            OnInventoryChanged?.Invoke();
        }

        /// <summary>
        /// Serializes the inventory into an array of InventorySlotData for persistent saving.
        /// </summary>
        public InventorySlotData[] GetInventoryData()
        {
            InitializeSlots();
            InventorySlotData[] data = new InventorySlotData[slots.Length];
            for (int i = 0; i < slots.Length; i++)
            {
                data[i] = new InventorySlotData(slots[i].type, slots[i].count, slots[i].isTool, (int)slots[i].toolId);
            }
            return data;
        }

        /// <summary>
        /// Restores inventory slots from serialized InventorySlotData array.
        /// </summary>
        public void SetInventoryData(InventorySlotData[] data)
        {
            InitializeSlots();
            if (data == null) return;

            int count = Mathf.Min(slots.Length, data.Length);
            for (int i = 0; i < count; i++)
            {
                if (data[i].isTool && data[i].toolId > 0)
                {
                    slots[i] = new InventorySlot((ToolID)data[i].toolId);
                }
                else
                {
                    slots[i] = new InventorySlot(data[i].type, data[i].count);
                }
            }
            for (int i = count; i < slots.Length; i++)
            {
                slots[i] = new InventorySlot();
            }

            OnInventoryChanged?.Invoke();
        }
    }
}
