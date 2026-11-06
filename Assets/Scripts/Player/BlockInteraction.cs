using System;
using Core;
using World;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Luminora-style mining and block placement interaction controller.
    /// Projects mouse cursor ray onto the Z = 0 gameplay plane, displays a 3D wireframe
    /// selection highlight box with distance and validity states, tracks mining progress
    /// with crack feedback, spawns debris and dropped items upon destruction,
    /// and handles placement validation (adjacency, collision overlap, inventory check).
    /// </summary>
    [DisallowMultipleComponent]
    public class BlockInteraction : MonoBehaviour
    {
        [Header("Reach & Speed Settings")]
        [SerializeField] private float reachDistance = 6.0f;
        [SerializeField] private float miningSpeedMultiplier = 1.0f;
        [SerializeField] private float placeCooldown = 0.16f;

        [Header("Visual Feedback")]
        [SerializeField] private Color validColor = new Color(0.25f, 0.95f, 0.35f, 0.50f);
        [SerializeField] private Color outOfReachColor = new Color(0.95f, 0.25f, 0.25f, 0.35f);
        [SerializeField] private Color miningColor = new Color(1.0f, 0.75f, 0.20f, 0.65f);

        [Header("Runtime State")]
        [SerializeField] private int targetBlockX;
        [SerializeField] private int targetBlockY;
        [SerializeField] private bool isTargetInReach;
        [SerializeField] private BlockType hoveredBlockType;
        [SerializeField] private float currentMiningProgress;

        // Visual targeting components
        private GameObject _highlightBox;
        private MeshRenderer _boxRenderer;
        private Material _boxMaterial;
        private ParticleSystem _debrisParticleSystem;

        // Cooldowns & references
        private float _lastPlaceTime;
        private int _lastMiningTargetX = int.MinValue;
        private int _lastMiningTargetY = int.MinValue;
        private PlayerController2D5 _playerController;
        private CharacterController _characterController;
        private Camera _mainCamera;

        public float ReachDistance => reachDistance;
        public int TargetX => targetBlockX;
        public int TargetY => targetBlockY;
        public float MiningProgress => currentMiningProgress;
        public bool IsTargetInReach => isTargetInReach;
        public BlockType HoveredBlockType => hoveredBlockType;

        // Interaction events for UI, tutorials, and game logic
        public static event Action<BlockType, int, int> OnBlockMined;
        public static event Action<BlockType, int, int> OnBlockPlaced;

        private void Awake()
        {
            _playerController = GetComponent<PlayerController2D5>();
            _characterController = GetComponent<CharacterController>();
            _mainCamera = Camera.main;

            InitializeHighlightBox();
            InitializeDebrisParticles();
        }

        private void Start()
        {
            if (_mainCamera == null)
            {
                _mainCamera = Camera.main;
            }
        }

        private void Update()
        {
            if ((UI.UIManager.Instance != null && UI.UIManager.Instance.CurrentState != UI.GameState.Playing) ||
                (CreativeInventoryUI.Instance != null && CreativeInventoryUI.Instance.IsOpen))
            {
                isTargetInReach = false;
                currentMiningProgress = 0f;
                if (_highlightBox != null) _highlightBox.SetActive(false);
                return;
            }

            if (_mainCamera == null)
            {
                _mainCamera = Camera.main;
                if (_mainCamera == null) return;
            }

            UpdateMouseTargeting();
            HandleMiningInput();
            HandlePlacementInput();
            UpdateHighlightVisuals();
        }

        /// <summary>
        /// Projects a ray from the mouse position through the camera onto the Z = 0 gameplay plane.
        /// Computes discrete voxel coordinates and reach distance.
        /// </summary>
        private void UpdateMouseTargeting()
        {
            Plane gameplayPlane = new Plane(Vector3.forward, Vector3.zero);
            Ray mouseRay = _mainCamera.ScreenPointToRay(Input.mousePosition);

            if (gameplayPlane.Raycast(mouseRay, out float enterDistance))
            {
                Vector3 hitPoint = mouseRay.GetPoint(enterDistance);
                targetBlockX = Mathf.FloorToInt(hitPoint.x);
                targetBlockY = Mathf.FloorToInt(hitPoint.y);

                Vector3 playerCenter = transform.position + Vector3.up * 0.9f;
                Vector3 targetBlockCenter = new Vector3(targetBlockX + 0.5f, targetBlockY + 0.5f, 0f);

                float distance = Vector3.Distance(playerCenter, targetBlockCenter);
                isTargetInReach = (distance <= reachDistance);

                hoveredBlockType = GetWorldBlock(targetBlockX, targetBlockY);
            }
            else
            {
                isTargetInReach = false;
                hoveredBlockType = BlockType.Air;
            }
        }

        /// <summary>
        /// Handles Left-Click mining hold, damage accumulation, particles, and block destruction.
        /// </summary>
        private void HandleMiningInput()
        {
            // First check mob attack interaction
            if (Input.GetMouseButtonDown(0))
            {
                if (Entities.MobManager.Instance != null)
                {
                    Vector3 clickTarget = new Vector3(targetBlockX + 0.5f, targetBlockY + 0.5f, 0f);
                    int attackDmg = 5;
                    var heldTool = InventorySystem.Instance != null ? InventorySystem.Instance.GetActiveTool() : null;
                    if (heldTool != null)
                    {
                        attackDmg = Mathf.RoundToInt(heldTool.damage);
                    }
                    if (Entities.MobManager.Instance.CheckAttackAt(clickTarget, 1.4f, attackDmg, transform.position))
                    {
                        currentMiningProgress = 0f;
                        return;
                    }
                }
            }

            // Reset mining progress if target changed
            if (targetBlockX != _lastMiningTargetX || targetBlockY != _lastMiningTargetY)
            {
                currentMiningProgress = 0f;
                _lastMiningTargetX = targetBlockX;
                _lastMiningTargetY = targetBlockY;
            }

            if (Input.GetMouseButton(0) && isTargetInReach && hoveredBlockType != BlockType.Air)
            {
                BlockData blockData = BlockRegistry.GetBlock(hoveredBlockType);
                bool isCreative = _playerController != null && _playerController.CurrentGameMode == GameMode.Creative;

                // Unbreakable blocks in survival (e.g. Bedrock with hardness < 0)
                if (blockData.hardness < 0f && !isCreative)
                {
                    currentMiningProgress = 0f;
                    return;
                }

                // In Creative Mode: Instant block break (0 delay)
                if (isCreative)
                {
                    currentMiningProgress = 1.0f;
                    DestroyTargetBlock(blockData);
                    return;
                }

                float baseHardness = Mathf.Max(0.1f, blockData.hardness);
                float toolEfficiency = 1.0f;
                var heldTool = InventorySystem.Instance != null ? InventorySystem.Instance.GetActiveTool() : null;
                if (heldTool != null)
                {
                    toolEfficiency = heldTool.GetEfficiency(hoveredBlockType);
                }
                float damageDelta = ((miningSpeedMultiplier * toolEfficiency) / baseHardness) * Time.deltaTime;
                currentMiningProgress += damageDelta;

                // Emit chipping debris particles
                EmitDebrisParticles(targetBlockX, targetBlockY, blockData.defaultColor, 1);

                if (currentMiningProgress >= 1.0f)
                {
                    DestroyTargetBlock(blockData);
                }
            }
            else
            {
                // Gradually decay or reset mining progress on mouse release
                currentMiningProgress = Mathf.MoveTowards(currentMiningProgress, 0f, Time.deltaTime * 3.0f);
            }
        }

        /// <summary>
        /// Destroys the block at current target coordinates, spawns debris burst and dropped item.
        /// </summary>
        private void DestroyTargetBlock(BlockData blockData)
        {
            Vector3 blockCenter = new Vector3(targetBlockX + 0.5f, targetBlockY + 0.5f, 0f);

            // 1. Clear block in voxel world
            SetWorldBlock(targetBlockX, targetBlockY, BlockType.Air);

            // 2. Play shatter debris burst
            EmitDebrisParticles(targetBlockX, targetBlockY, blockData.defaultColor, 18);

            // 3. Spawn floating DroppedItem
            BlockType dropType = (blockData.dropItemId > 0 && blockData.dropItemId < BlockRegistry.Count)
                ? (BlockType)blockData.dropItemId
                : blockData.type;

            int dropCount = Mathf.Max(1, blockData.dropCount);
            DroppedItem.Spawn(blockCenter, dropType, dropCount);

            // Notify listeners (UI tutorial, achievements, etc.)
            OnBlockMined?.Invoke(blockData.type, targetBlockX, targetBlockY);

            // Reset interaction state
            currentMiningProgress = 0f;
            hoveredBlockType = BlockType.Air;
        }

        /// <summary>
        /// Handles Right-Click block placement with adjacency, inventory, and player overlap checks.
        /// </summary>
        private void HandlePlacementInput()
        {
            if (!Input.GetMouseButton(1)) return;
            if (Time.time - _lastPlaceTime < placeCooldown) return;
            if (!isTargetInReach) return;

            // Target must be empty (Air)
            if (hoveredBlockType != BlockType.Air) return;

            // Check active inventory item
            InventorySystem inv = InventorySystem.Instance;
            if (inv == null) return;

            InventorySlot activeItem = inv.GetActiveItem();
            if (activeItem == null || activeItem.IsEmpty || activeItem.isTool) return;

            BlockType blockToPlace = activeItem.type;
            BlockData blockData = BlockRegistry.GetBlock(blockToPlace);

            // Validate adjacency (must be touching another solid block or background wall, directional for torches)
            if (!IsPlacementLocationSupported(targetBlockX, targetBlockY, blockToPlace))
            {
                return;
            }

            // Solid blocks cannot be placed intersecting the player collider!
            if (blockData.isSolid && IsIntersectingPlayer(targetBlockX, targetBlockY))
            {
                return;
            }

            // Perform placement
            bool placed = SetWorldBlock(targetBlockX, targetBlockY, blockToPlace);
            if (placed)
            {
                bool isCreative = _playerController != null && _playerController.CurrentGameMode == GameMode.Creative;
                if (!isCreative)
                {
                    inv.ConsumeActiveItem(1);
                }
                _lastPlaceTime = Time.time;

                // Subtle placement dust effect
                EmitDebrisParticles(targetBlockX, targetBlockY, blockData.defaultColor, 6);

                // Notify listeners
                OnBlockPlaced?.Invoke(blockToPlace, targetBlockX, targetBlockY);
            }
        }

        /// <summary>
        /// Luminora placement rule: placement is valid if adjacent to at least one solid block,
        /// or placed directly against a non-air background wall.
        /// For Torches: directional support requires solid floor (y - 1), solid left wall (x - 1),
        /// solid right wall (x + 1), or an existing background wall.
        /// </summary>
        private bool IsPlacementLocationSupported(int x, int y, BlockType blockType = BlockType.Dirt)
        {
            // Check background wall support
            BlockType wall = GetWorldWall(x, y);
            if (wall != BlockType.Air) return true;

            // Directional support checks specifically for torches
            if (blockType == BlockType.Torch)
            {
                if (BlockRegistry.IsSolid(GetWorldBlock(x, y - 1))) return true; // Floor-standing
                if (BlockRegistry.IsSolid(GetWorldBlock(x - 1, y))) return true; // Left-wall mounted
                if (BlockRegistry.IsSolid(GetWorldBlock(x + 1, y))) return true; // Right-wall mounted
                return false;
            }

            // Standard blocks: check 4-connected neighbors
            if (BlockRegistry.IsSolid(GetWorldBlock(x + 1, y))) return true;
            if (BlockRegistry.IsSolid(GetWorldBlock(x - 1, y))) return true;
            if (BlockRegistry.IsSolid(GetWorldBlock(x, y + 1))) return true;
            if (BlockRegistry.IsSolid(GetWorldBlock(x, y - 1))) return true;

            return false;
        }

        /// <summary>
        /// Checks whether the target block bounding box overlaps the player's physical collider.
        /// </summary>
        private bool IsIntersectingPlayer(int blockX, int blockY)
        {
            Bounds blockBounds = new Bounds(
                new Vector3(blockX + 0.5f, blockY + 0.5f, 0f),
                new Vector3(0.96f, 0.96f, 1.2f)
            );

            if (_characterController != null)
            {
                return _characterController.bounds.Intersects(blockBounds);
            }

            Collider col = GetComponent<Collider>();
            if (col != null)
            {
                return col.bounds.Intersects(blockBounds);
            }

            return false;
        }

        /// <summary>
        /// Updates the 3D targeting highlight box position, scale, and color tint.
        /// </summary>
        private void UpdateHighlightVisuals()
        {
            if (_highlightBox == null) return;

            // Position at target block center
            Vector3 targetCenter = new Vector3(
                targetBlockX + 0.5f,
                targetBlockY + 0.5f,
                WorldSettings.BlockDepth * 0.5f
            );
            _highlightBox.transform.position = targetCenter;

            // Determine highlight state color
            Color currentColor;
            if (!isTargetInReach)
            {
                currentColor = outOfReachColor;
            }
            else if (currentMiningProgress > 0.05f)
            {
                // Pulsate/vibrate slightly while breaking
                float pulse = 1.0f + Mathf.Sin(Time.time * 25f) * 0.05f;
                _highlightBox.transform.localScale = new Vector3(1.02f * pulse, 1.02f * pulse, WorldSettings.BlockDepth + 0.04f);
                currentColor = Color.Lerp(validColor, miningColor, currentMiningProgress);
            }
            else
            {
                _highlightBox.transform.localScale = new Vector3(1.02f, 1.02f, WorldSettings.BlockDepth + 0.04f);
                currentColor = validColor;
            }

            if (_boxMaterial != null)
            {
                _boxMaterial.color = currentColor;
            }
        }

        /// <summary>
        /// Creates the 3D translucent wireframe targeting box.
        /// </summary>
        private void InitializeHighlightBox()
        {
            _highlightBox = GameObject.CreatePrimitive(PrimitiveType.Cube);
            _highlightBox.name = "BlockHighlightBox";
            _highlightBox.transform.localScale = new Vector3(1.02f, 1.02f, WorldSettings.BlockDepth + 0.04f);

            Collider col = _highlightBox.GetComponent<Collider>();
            if (col != null) Destroy(col);

            _boxRenderer = _highlightBox.GetComponent<MeshRenderer>();
            Shader unlitTransparent = Shader.Find("Sprites/Default") ?? Shader.Find("Mobile/Particles/Alpha Blended") ?? Shader.Find("Unlit/Transparent");
            _boxMaterial = new Material(unlitTransparent)
            {
                color = validColor
            };
            _boxRenderer.material = _boxMaterial;
        }

        /// <summary>
        /// Configures procedural particle debris system for mining chips and block shatter.
        /// </summary>
        private void InitializeDebrisParticles()
        {
            GameObject pObj = new GameObject("BlockDebrisEmitter");
            pObj.transform.SetParent(transform, false);

            _debrisParticleSystem = pObj.AddComponent<ParticleSystem>();
            ParticleSystemRenderer psr = pObj.GetComponent<ParticleSystemRenderer>();

            var main = _debrisParticleSystem.main;
            main.playOnAwake = false;
            main.loop = false;
            main.startLifetime = 0.5f;
            main.startSpeed = 4.0f;
            main.startSize = 0.12f;
            main.gravityModifier = 1.8f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;

            var emission = _debrisParticleSystem.emission;
            emission.enabled = false;

            var shape = _debrisParticleSystem.shape;
            shape.shapeType = ParticleSystemShapeType.Box;
            shape.scale = new Vector3(0.5f, 0.5f, 0.2f);
        }

        /// <summary>
        /// Emits block debris particles at the targeted block location.
        /// </summary>
        private void EmitDebrisParticles(int x, int y, Color blockColor, int count)
        {
            if (_debrisParticleSystem == null) return;

            Vector3 blockCenter = new Vector3(x + 0.5f, y + 0.5f, 0f);
            _debrisParticleSystem.transform.position = blockCenter;

            var emitParams = new ParticleSystem.EmitParams
            {
                startColor = blockColor,
                position = blockCenter + (Vector3)UnityEngine.Random.insideUnitCircle * 0.25f,
                velocity = new Vector3(
                    UnityEngine.Random.Range(-2.5f, 2.5f),
                    UnityEngine.Random.Range(1.0f, 4.5f),
                    0f
                )
            };

            for (int i = 0; i < count; i++)
            {
                _debrisParticleSystem.Emit(emitParams, 1);
            }
        }

        // =============================================================
        // WorldManager Proxy Helpers (Null-safe)
        // =============================================================

        private BlockType GetWorldBlock(int x, int y)
        {
            if (WorldManager.Instance != null)
            {
                return WorldManager.Instance.GetBlock(x, y);
            }
            return BlockType.Air;
        }

        private bool SetWorldBlock(int x, int y, BlockType type)
        {
            if (WorldManager.Instance != null)
            {
                return WorldManager.Instance.SetBlock(x, y, type);
            }
            return false;
        }

        private BlockType GetWorldWall(int x, int y)
        {
            if (WorldManager.Instance != null)
            {
                return WorldManager.Instance.GetWall(x, y);
            }
            return BlockType.Air;
        }
    }
}
