using System;
using Core;
using World;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Smooth 2.5D side-scrolling platformer controller strictly constrained to the Z = 0 plane.
    /// Features responsive acceleration/deceleration, variable jump heights, coyote time,
    /// automatic 1-block Terraria step climbing, fall gravity with terminal velocity,
    /// and a procedurally generated charming 2.5D animated character model.
    /// </summary>
    [RequireComponent(typeof(CharacterController))]
    [DisallowMultipleComponent]
    public class PlayerController2D5 : MonoBehaviour
    {
        [Header("Locomotion Tuning")]
        [SerializeField] private float walkSpeed = 8.0f;
        [SerializeField] private float sprintSpeed = 11.0f;
        [SerializeField] private float acceleration = 50.0f;
        [SerializeField] private float deceleration = 65.0f;
        [SerializeField] private float airControl = 0.85f;

        [Header("Jump & Gravity")]
        [SerializeField] private float jumpVelocity = 13.5f;
        [SerializeField] private float jumpCutMultiplier = 0.5f;
        [SerializeField] private float gravity = -34.0f;
        [SerializeField] private float terminalVelocity = -28.0f;
        [SerializeField] private float coyoteTime = 0.15f;
        [SerializeField] private float jumpBufferTime = 0.15f;

        [Header("Terraria Step-Climbing")]
        [SerializeField] private float maxStepHeight = 1.15f; // Handles standard 1.0 unit blocks seamlessly
        [SerializeField] private float stepClimbSpeed = 16.0f;

        [Header("Game Mode & Creative Flight")]
        [SerializeField] private GameMode currentGameMode = GameMode.Survival;
        [SerializeField] private bool isFlying = false;
        [SerializeField] private float flightVerticalSpeed = 11.0f;
        [SerializeField] private float flightDamping = 22.0f;

        [Header("State")]
        [SerializeField] private bool isGrounded;
        [SerializeField] private bool isFacingRight = true;
        [SerializeField] private float moveInput;
        [SerializeField] private Vector3 velocity;

        [Header("Health & Combat (Survival Mode)")]
        [SerializeField] private float maxHealth = 100.0f;
        [SerializeField] private float currentHealth = 100.0f;
        [SerializeField] private float hurtTimer = 0.0f;
        [SerializeField] private float timeSinceCombat = 25.0f;
        [SerializeField] private bool isDead = false;
        private float _hurtFlashTimer = 0.0f;
        private const float InvulnerableDuration = 0.65f;
        public float TimeSinceCombat => timeSinceCombat;
        private readonly System.Collections.Generic.List<Renderer> _visualRenderers = new System.Collections.Generic.List<Renderer>();
        private readonly System.Collections.Generic.List<Color> _originalVisualColors = new System.Collections.Generic.List<Color>();
        private AudioSource _audioSource;

        // Components
        private CharacterController _controller;
        private Transform _visualRoot;

        // Limb transforms for procedural animation
        private Transform _headTransform;
        private Transform _torsoTransform;
        private Transform _armLeft;
        private Transform _armRight;
        private Transform _legLeft;
        private Transform _legRight;

        // Timers
        private float _coyoteTimer;
        private float _jumpBufferTimer;
        private float _currentHorizontalSpeed;
        private float _walkCyclePhase;

        // In-Hand Tool & Dynamic Swing Animation
        private float _toolSwingTimer;
        private bool _isToolSwinging;
        private const float ToolSwingDuration = 0.22f;
        private GameObject _heldToolObject;
        private ToolID _lastEquippedTool = ToolID.None;
        private BlockType _lastEquippedBlock = BlockType.Air;
        private float _lastSpacePressTime = -10.0f;

        // Public Properties
        public Vector3 Position => transform.position;
        public bool IsGrounded => isGrounded;
        public float MoveInput => moveInput;
        public bool IsFacingRight => isFacingRight;
        public Vector3 Velocity => velocity;
        public float CurrentHealth => currentHealth;
        public float MaxHealth => maxHealth;
        public float HurtTimer => hurtTimer;
        public bool IsDead => isDead;

        public GameMode CurrentGameMode
        {
            get => currentGameMode;
            set
            {
                currentGameMode = value;
                if (currentGameMode != GameMode.Creative)
                {
                    isFlying = false;
                }
            }
        }

        public bool IsFlying
        {
            get => isFlying;
            set
            {
                if (currentGameMode == GameMode.Creative)
                {
                    isFlying = value;
                    if (isFlying)
                    {
                        velocity.y = 0f;
                    }
                }
                else
                {
                    isFlying = false;
                }
            }
        }

        private void Awake()
        {
            _controller = GetComponent<CharacterController>();
            ConfigureCharacterController();
            EnsureVisualRepresentation();
        }

        private void Start()
        {
            // Lock Z coordinate to gameplay plane
            SnapToGameplayPlane();
            EnsureAudioSource();
            CacheVisualRenderers();
        }

        private void Update()
        {
            float dt = Time.deltaTime;

            UpdateHealthAndHurtFlash(dt);

            if (!isDead)
            {
                ReadInputs(dt);
            }
            else
            {
                moveInput = 0f;
            }

            UpdateGroundState();
            HandleStepClimbing(dt);
            ComputeHorizontalVelocity(dt);
            ComputeVerticalVelocity(dt);
            ApplyMovement(dt);
            UpdateFacingDirection();
            AnimateProceduralVisuals(dt);
        }

        private void LateUpdate()
        {
            // Strictly enforce Z = 0 plane and zero tilt rotation
            SnapToGameplayPlane();
        }

        /// <summary>
        /// Sets up CharacterController dimensions and collision parameters for 2.5D gameplay.
        /// </summary>
        private void ConfigureCharacterController()
        {
            _controller.height = 1.8f;
            _controller.radius = 0.38f;
            _controller.center = new Vector3(0f, 0.9f, 0f);
            _controller.stepOffset = maxStepHeight;
            _controller.slopeLimit = 65.0f;
            _controller.minMoveDistance = 0.001f;
        }

        /// <summary>
        /// Locks player strictly to the Z = 0 2D gameplay plane.
        /// </summary>
        private void SnapToGameplayPlane()
        {
            Vector3 pos = transform.position;
            pos.z = 0f;
            transform.position = pos;
            transform.rotation = Quaternion.identity;
        }

        /// <summary>
        /// Reads keyboard locomotion inputs and buffers jump requests.
        /// </summary>
        private void ReadInputs(float dt)
        {
            if (UI.UIManager.Instance != null && UI.UIManager.Instance.CurrentState != UI.GameState.Playing)
            {
                moveInput = 0f;
                return;
            }

            moveInput = Input.GetAxisRaw("Horizontal");

            // Double-tap Space flight toggle in Creative Mode (within 0.28 seconds)
            if (Input.GetKeyDown(KeyCode.Space))
            {
                float timeSinceLastSpace = Time.time - _lastSpacePressTime;
                if (currentGameMode == GameMode.Creative && timeSinceLastSpace <= 0.28f)
                {
                    isFlying = !isFlying;
                    _lastSpacePressTime = -10.0f; // Consume double-tap
                    if (isFlying)
                    {
                        velocity.y = 0f;
                    }
                }
                else
                {
                    _lastSpacePressTime = Time.time;
                }
            }

            // Coyote time countdown (only when grounded and not flying)
            if (isGrounded && !isFlying)
            {
                _coyoteTimer = coyoteTime;
            }
            else
            {
                _coyoteTimer = Mathf.Max(0f, _coyoteTimer - dt);
            }

            // Jump buffering (only if not flying)
            if (!isFlying && (Input.GetButtonDown("Jump") || Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.UpArrow)))
            {
                _jumpBufferTimer = jumpBufferTime;
            }
            else
            {
                _jumpBufferTimer = Mathf.Max(0f, _jumpBufferTimer - dt);
            }

            // Variable jump height cut on button release
            if (!isFlying && (Input.GetButtonUp("Jump") || Input.GetKeyUp(KeyCode.Space) || Input.GetKeyUp(KeyCode.W) || Input.GetKeyUp(KeyCode.UpArrow)) && velocity.y > 0f)
            {
                velocity.y *= jumpCutMultiplier;
            }
        }

        /// <summary>
        /// Robust ground detection combining CharacterController flags and multi-ray ground probe.
        /// </summary>
        private void UpdateGroundState()
        {
            bool controllerGrounded = _controller.isGrounded;

            // Supplemental downward raycasts at feet center and boundaries
            bool rayGrounded = false;
            float checkDist = 0.15f;
            Vector3 center = transform.position + Vector3.up * 0.1f;
            Vector3 leftOffset = center + Vector3.left * 0.28f;
            Vector3 rightOffset = center + Vector3.right * 0.28f;

            RaycastHit hit;
            if (Physics.Raycast(center, Vector3.down, out hit, checkDist + 0.1f, ~0, QueryTriggerInteraction.Ignore) ||
                Physics.Raycast(leftOffset, Vector3.down, out hit, checkDist + 0.1f, ~0, QueryTriggerInteraction.Ignore) ||
                Physics.Raycast(rightOffset, Vector3.down, out hit, checkDist + 0.1f, ~0, QueryTriggerInteraction.Ignore))
            {
                rayGrounded = true;
            }

            isGrounded = controllerGrounded || rayGrounded;
        }

        /// <summary>
        /// Terraria-style automatic step climbing:
        /// When walking into a 1-block high tile, detects if knee/step is clear and assists
        /// smooth elevation over the obstacle without needing to jump.
        /// </summary>
        private void HandleStepClimbing(float dt)
        {
            if (isFlying || Mathf.Abs(moveInput) < 0.05f || !isGrounded) return;

            Vector3 moveDir = moveInput > 0f ? Vector3.right : Vector3.left;
            float probeDistance = 0.42f;

            // 1. Probe low foot obstacle (y = 0.15f)
            Vector3 footOrigin = transform.position + Vector3.up * 0.15f;
            bool hitFootObstacle = Physics.Raycast(footOrigin, moveDir, out RaycastHit footHit, probeDistance, ~0, QueryTriggerInteraction.Ignore);

            if (hitFootObstacle)
            {
                // 2. Probe step height (y = 1.15f) - must be empty air
                Vector3 stepOrigin = transform.position + Vector3.up * (maxStepHeight + 0.05f);
                bool hitStepObstacle = Physics.Raycast(stepOrigin, moveDir, probeDistance + 0.1f, ~0, QueryTriggerInteraction.Ignore);

                // 3. Probe overhead clearance
                Vector3 headOrigin = transform.position + Vector3.up * 1.9f;
                bool hitHeadObstacle = Physics.Raycast(headOrigin, Vector3.up, 0.3f, ~0, QueryTriggerInteraction.Ignore);

                if (!hitStepObstacle && !hitHeadObstacle)
                {
                    // Smoothly elevate player over the 1-block step
                    float stepUpDelta = stepClimbSpeed * dt;
                    _controller.Move(Vector3.up * stepUpDelta);
                }
            }
        }

        /// <summary>
        /// Evaluates responsive horizontal movement with acceleration and deceleration.
        /// Boosts speed by 1.5x during Creative Flight with smooth velocity damping.
        /// </summary>
        private void ComputeHorizontalVelocity(float dt)
        {
            bool isSprinting = Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift);
            float baseSpeed = isSprinting ? sprintSpeed : walkSpeed;
            if (isFlying)
            {
                baseSpeed *= 1.5f; // Horizontal speed boosted by 1.5x when flying
            }

            float targetSpeed = baseSpeed * moveInput;
            float accelRate;

            if (isFlying)
            {
                // Smooth flight acceleration and deceleration
                accelRate = (Mathf.Abs(moveInput) > 0.05f) ? (acceleration * 1.5f) : flightDamping;
            }
            else
            {
                accelRate = Mathf.Abs(moveInput) > 0.05f ? acceleration : deceleration;
                if (!isGrounded)
                {
                    accelRate *= airControl;
                }
            }

            _currentHorizontalSpeed = Mathf.MoveTowards(_currentHorizontalSpeed, targetSpeed, accelRate * dt);
            velocity.x = _currentHorizontalSpeed;
        }

        /// <summary>
        /// Applies jump velocity, coyote time, and realistic gravity acceleration with terminal velocity.
        /// When flying: disables gravity, Space flies up (+Y), LeftShift/S flies down (-Y), with smooth damping.
        /// </summary>
        private void ComputeVerticalVelocity(float dt)
        {
            if (isFlying)
            {
                float targetVY = 0f;
                bool flyUp = Input.GetKey(KeyCode.Space);
                bool flyDown = Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift) || Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow);

                if (flyUp && !flyDown)
                {
                    targetVY = flightVerticalSpeed;
                }
                else if (flyDown && !flyUp)
                {
                    targetVY = -flightVerticalSpeed;
                }

                // Smooth vertical velocity damping when releasing or holding keys
                velocity.y = Mathf.MoveTowards(velocity.y, targetVY, flightDamping * dt);

                // Land if flying down onto ground
                if (isGrounded && flyDown && velocity.y < 0f)
                {
                    isFlying = false;
                    velocity.y = 0f;
                }
                return;
            }

            if (isGrounded && velocity.y < 0f)
            {
                // Gentle ground stick force
                velocity.y = -3.0f;
            }

            // Execute buffered jump if within coyote window
            if (_jumpBufferTimer > 0f && _coyoteTimer > 0f)
            {
                velocity.y = jumpVelocity;
                _jumpBufferTimer = 0f;
                _coyoteTimer = 0f;
            }

            // Apply gravity
            velocity.y += gravity * dt;

            // Clamp to terminal velocity
            if (velocity.y < terminalVelocity)
            {
                velocity.y = terminalVelocity;
            }
        }

        /// <summary>
        /// Submits computed motion vector to CharacterController.
        /// </summary>
        private void ApplyMovement(float dt)
        {
            Vector3 motion = velocity * dt;
            motion.z = 0f;
            _controller.Move(motion);
        }

        /// <summary>
        /// Updates horizontal facing orientation.
        /// </summary>
        private void UpdateFacingDirection()
        {
            if (moveInput > 0.05f)
            {
                isFacingRight = true;
            }
            else if (moveInput < -0.05f)
            {
                isFacingRight = false;
            }

            if (_visualRoot != null)
            {
                // Side-view orientation: facing +X (90 deg) or -X (-90 deg)
                float targetAngle = isFacingRight ? 90f : -90f;
                _visualRoot.localRotation = Quaternion.Euler(0f, targetAngle, 0f);
            }
        }

        /// <summary>
        /// Animates limbs and torso procedurally with charming walk, idle, and jump motions.
        /// </summary>
        private void AnimateProceduralVisuals(float dt)
        {
            if (_visualRoot == null) return;

            float horizontalMoveMagnitude = Mathf.Abs(_currentHorizontalSpeed);

            if (isFlying)
            {
                // Dynamic creative flight hovering / soaring pose
                float hover = Mathf.Sin(Time.time * 3.5f) * 0.04f;
                if (_torsoTransform != null) _torsoTransform.localPosition = new Vector3(0f, 0.9f + hover, 0f);
                if (_headTransform != null) _headTransform.localPosition = new Vector3(0f, 1.48f + hover, 0f);

                float armAngle = (horizontalMoveMagnitude > 0.2f) ? -35f : -15f;
                float legAngle = (horizontalMoveMagnitude > 0.2f) ? 24f : 12f;

                if (_armLeft != null) _armLeft.localRotation = Quaternion.Euler(armAngle, 0f, 0f);
                if (_armRight != null) _armRight.localRotation = Quaternion.Euler(armAngle, 0f, 0f);
                if (_legLeft != null) _legLeft.localRotation = Quaternion.Euler(legAngle, 0f, 0f);
                if (_legRight != null) _legRight.localRotation = Quaternion.Euler(legAngle, 0f, 0f);
                return;
            }

            if (isGrounded)
            {
                if (horizontalMoveMagnitude > 0.2f)
                {
                    // Walking animation
                    float animSpeed = horizontalMoveMagnitude * 1.8f;
                    _walkCyclePhase += animSpeed * dt;
                    float sin = Mathf.Sin(_walkCyclePhase);

                    // Limb swings
                    if (_legLeft != null) _legLeft.localRotation = Quaternion.Euler(sin * 36f, 0f, 0f);
                    if (_legRight != null) _legRight.localRotation = Quaternion.Euler(-sin * 36f, 0f, 0f);
                    if (_armLeft != null) _armLeft.localRotation = Quaternion.Euler(-sin * 30f, 0f, 0f);
                    if (_armRight != null) _armRight.localRotation = Quaternion.Euler(sin * 30f, 0f, 0f);

                    // Gentle torso bobbing during run
                    if (_torsoTransform != null)
                    {
                        float bob = Mathf.Abs(Mathf.Sin(_walkCyclePhase * 2f)) * 0.05f;
                        _torsoTransform.localPosition = new Vector3(0f, 0.9f - bob, 0f);
                    }
                }
                else
                {
                    // Idle breathing animation
                    float breath = Mathf.Sin(Time.time * 2.2f) * 0.02f;

                    if (_legLeft != null) _legLeft.localRotation = Quaternion.identity;
                    if (_legRight != null) _legRight.localRotation = Quaternion.identity;
                    if (_armLeft != null) _armLeft.localRotation = Quaternion.Euler(breath * 100f, 0f, 0f);
                    if (_armRight != null) _armRight.localRotation = Quaternion.Euler(-breath * 100f, 0f, 0f);
                    if (_torsoTransform != null) _torsoTransform.localPosition = new Vector3(0f, 0.9f + breath, 0f);
                    if (_headTransform != null) _headTransform.localPosition = new Vector3(0f, 1.48f + breath * 1.5f, 0f);
                }
            }
            else
            {
                // In-air dynamic jumping / falling pose
                float jumpArmAngle = velocity.y > 0f ? -45f : 30f;
                float jumpLegAngle = velocity.y > 0f ? 25f : -15f;

                if (_armLeft != null) _armLeft.localRotation = Quaternion.Euler(jumpArmAngle, 0f, 0f);
                if (_armRight != null) _armRight.localRotation = Quaternion.Euler(jumpArmAngle, 0f, 0f);
                if (_legLeft != null) _legLeft.localRotation = Quaternion.Euler(jumpLegAngle, 0f, 0f);
                if (_legRight != null) _legRight.localRotation = Quaternion.Euler(-jumpLegAngle, 0f, 0f);
            }

            // In-Hand Tool Visual & Dynamic Swing Animation
            AnimateHeldToolAndSwing(dt);
        }

        /// <summary>
        /// Updates the 2.5D held tool mesh and drives the dynamic swinging attack/mining arc animation.
        /// </summary>
        private void AnimateHeldToolAndSwing(float dt)
        {
            bool isAttackingOrMining = Input.GetMouseButton(0) &&
                (CreativeInventoryUI.Instance == null || !CreativeInventoryUI.Instance.IsOpen) &&
                (UI.UIManager.Instance == null || UI.UIManager.Instance.CurrentState == UI.GameState.Playing);

            if (isAttackingOrMining)
            {
                _isToolSwinging = true;
                _toolSwingTimer += dt / ToolSwingDuration;
                if (_toolSwingTimer >= 1.0f) _toolSwingTimer = Mathf.Repeat(_toolSwingTimer, 1.0f);
            }
            else
            {
                if (_isToolSwinging)
                {
                    _toolSwingTimer += dt / ToolSwingDuration;
                    if (_toolSwingTimer >= 1.0f)
                    {
                        _isToolSwinging = false;
                        _toolSwingTimer = 0f;
                    }
                }
            }

            // Update in-hand visual model
            UpdateInHandToolVisual();

            // Dynamic swinging arc animation: 45 degrees backward to -60 degrees forward swing with arm extension!
            if (_isToolSwinging && _armRight != null)
            {
                float t = _toolSwingTimer;
                float swingAngle;
                if (t < 0.22f)
                {
                    float p = t / 0.22f;
                    swingAngle = -15f + 60f * (p * p); // Winds up backward to +45 deg
                }
                else if (t < 0.70f)
                {
                    float p = (t - 0.22f) / 0.48f;
                    float ease = 1.0f - Mathf.Pow(1.0f - p, 3.0f);
                    swingAngle = 45f - 105f * ease; // Slashes forward-down to -60 deg!
                }
                else
                {
                    float p = (t - 0.70f) / 0.30f;
                    swingAngle = -60f + 45f * p; // Recovers back to -15 deg
                }

                _armRight.localRotation = Quaternion.Euler(swingAngle, 0f, 0f);
            }
        }

        private void UpdateInHandToolVisual()
        {
            if (_armRight == null) return;

            InventorySystem inv = InventorySystem.Instance;
            if (inv == null) return;

            InventorySlot active = inv.GetActiveItem();
            if (active == null || active.IsEmpty)
            {
                if (_heldToolObject != null) _heldToolObject.SetActive(false);
                return;
            }

            if (_heldToolObject == null)
            {
                _heldToolObject = new GameObject("HeldItemInHand");
                _heldToolObject.transform.SetParent(_armRight, false);
                _heldToolObject.transform.localPosition = new Vector3(0f, -0.42f, 0.16f);
                _heldToolObject.transform.localRotation = Quaternion.Euler(45f, 0f, 0f);
            }

            if (!_heldToolObject.activeSelf) _heldToolObject.SetActive(true);

            Shader shader = Shader.Find("Universal Render Pipeline/Lit") ??
                            Shader.Find("Standard") ??
                            Shader.Find("Mobile/Diffuse");

            if (active.isTool)
            {
                if (_lastEquippedTool != active.toolId)
                {
                    _lastEquippedTool = active.toolId;
                    _lastEquippedBlock = BlockType.Air;
                    RebuildHeldToolModel(active.toolId, shader);
                }
            }
            else
            {
                if (_lastEquippedBlock != active.type)
                {
                    _lastEquippedBlock = active.type;
                    _lastEquippedTool = ToolID.None;
                    RebuildHeldBlockModel(active.type, shader);
                }
            }
        }

        private void RebuildHeldToolModel(ToolID toolId, Shader shader)
        {
            if (_heldToolObject == null) return;

            foreach (Transform child in _heldToolObject.transform)
            {
                Destroy(child.gameObject);
            }

            ToolData tool = ToolRegistry.Get(toolId);
            if (tool == null) return;

            // 1. Wooden Handle
            GameObject handle = GameObject.CreatePrimitive(PrimitiveType.Cube);
            handle.name = "ToolHandle";
            handle.transform.SetParent(_heldToolObject.transform, false);
            handle.transform.localPosition = new Vector3(0f, 0.12f, 0f);
            handle.transform.localScale = new Vector3(0.06f, 0.48f, 0.06f);
            StripCollider(handle);
            SetColor(handle, new Color(0.55f, 0.36f, 0.18f), shader);

            // 2. Head based on tool type
            GameObject head = GameObject.CreatePrimitive(PrimitiveType.Cube);
            head.name = "ToolHead";
            head.transform.SetParent(_heldToolObject.transform, false);
            StripCollider(head);
            SetColor(head, tool.tierColor, shader);

            switch (tool.type)
            {
                case ToolType.Pickaxe:
                    head.transform.localPosition = new Vector3(0f, 0.36f, 0f);
                    head.transform.localScale = new Vector3(0.48f, 0.10f, 0.10f);
                    break;
                case ToolType.Axe:
                    head.transform.localPosition = new Vector3(0.06f, 0.32f, 0f);
                    head.transform.localScale = new Vector3(0.18f, 0.22f, 0.10f);
                    break;
                case ToolType.Sword:
                    head.transform.localPosition = new Vector3(0f, 0.42f, 0f);
                    head.transform.localScale = new Vector3(0.08f, 0.60f, 0.04f);

                    // Crossguard
                    GameObject guard = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    guard.name = "SwordCrossguard";
                    guard.transform.SetParent(_heldToolObject.transform, false);
                    guard.transform.localPosition = new Vector3(0f, 0.10f, 0f);
                    guard.transform.localScale = new Vector3(0.24f, 0.06f, 0.08f);
                    StripCollider(guard);
                    SetColor(guard, new Color(0.95f, 0.82f, 0.22f), shader);
                    break;
                case ToolType.Shovel:
                    head.transform.localPosition = new Vector3(0f, 0.36f, 0f);
                    head.transform.localScale = new Vector3(0.18f, 0.22f, 0.06f);
                    break;
            }
        }

        private void RebuildHeldBlockModel(BlockType blockType, Shader shader)
        {
            if (_heldToolObject == null) return;

            foreach (Transform child in _heldToolObject.transform)
            {
                Destroy(child.gameObject);
            }

            if (blockType == BlockType.Air) return;

            GameObject cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
            cube.name = "HeldBlock";
            cube.transform.SetParent(_heldToolObject.transform, false);
            cube.transform.localPosition = Vector3.zero;
            cube.transform.localScale = new Vector3(0.24f, 0.24f, 0.24f);
            StripCollider(cube);

            BlockData bData = BlockRegistry.GetBlock(blockType);
            SetColor(cube, bData.defaultColor, shader);
        }

        /// <summary>
        /// Generates a charming, high-detail procedural 2.5D character with head, helmet,
        /// eyes with pupils, adventurer tunic, belt, and articulated limbs.
        /// </summary>
        private void EnsureVisualRepresentation()
        {
            if (transform.Find("ProceduralCharacter") != null)
            {
                _visualRoot = transform.Find("ProceduralCharacter");
                CacheLimbReferences();
                return;
            }

            GameObject root = new GameObject("ProceduralCharacter");
            root.transform.SetParent(transform, false);
            root.transform.localPosition = Vector3.zero;
            root.transform.localRotation = Quaternion.Euler(0f, 90f, 0f); // Face +X by default
            _visualRoot = root.transform;

            Shader standardShader = Shader.Find("Mobile/Diffuse") ?? Shader.Find("Standard") ?? Shader.Find("Sprites/Default");

            // 1. Torso / Shirt
            GameObject torso = GameObject.CreatePrimitive(PrimitiveType.Cube);
            torso.name = "Torso";
            torso.transform.SetParent(_visualRoot, false);
            torso.transform.localPosition = new Vector3(0f, 0.9f, 0f);
            torso.transform.localScale = new Vector3(0.48f, 0.65f, 0.32f);
            StripCollider(torso);
            SetColor(torso, new Color(0.22f, 0.48f, 0.85f), standardShader); // Adventurer Blue Tunic
            _torsoTransform = torso.transform;

            // 1b. Belt
            GameObject belt = GameObject.CreatePrimitive(PrimitiveType.Cube);
            belt.name = "Belt";
            belt.transform.SetParent(_torsoTransform, false);
            belt.transform.localPosition = new Vector3(0f, -0.42f, 0f);
            belt.transform.localScale = new Vector3(1.05f, 0.18f, 1.05f);
            StripCollider(belt);
            SetColor(belt, new Color(0.35f, 0.22f, 0.12f), standardShader); // Brown Leather Belt

            // 2. Head
            GameObject head = GameObject.CreatePrimitive(PrimitiveType.Cube);
            head.name = "Head";
            head.transform.SetParent(_visualRoot, false);
            head.transform.localPosition = new Vector3(0f, 1.48f, 0f);
            head.transform.localScale = new Vector3(0.45f, 0.45f, 0.42f);
            StripCollider(head);
            SetColor(head, new Color(0.96f, 0.80f, 0.68f), standardShader); // Peach Skin
            _headTransform = head.transform;

            // 2b. Adventurer Helmet / Cap
            GameObject helmet = GameObject.CreatePrimitive(PrimitiveType.Cube);
            helmet.name = "Helmet";
            helmet.transform.SetParent(_headTransform, false);
            helmet.transform.localPosition = new Vector3(0f, 0.32f, 0f);
            helmet.transform.localScale = new Vector3(1.08f, 0.45f, 1.08f);
            StripCollider(helmet);
            SetColor(helmet, new Color(0.85f, 0.55f, 0.18f), standardShader); // Gold/Bronze Helmet

            // 2c. Eyes (facing forward towards +Z in local visual space)
            CreateEye(_headTransform, new Vector3(0.12f, 0.05f, 0.52f), standardShader);
            CreateEye(_headTransform, new Vector3(-0.12f, 0.05f, 0.52f), standardShader);

            // 3. Arms
            _armLeft = CreateLimb("ArmLeft", _visualRoot, new Vector3(-0.35f, 1.05f, 0f), new Vector3(0.18f, 0.55f, 0.22f), new Color(0.20f, 0.42f, 0.78f), standardShader);
            _armRight = CreateLimb("ArmRight", _visualRoot, new Vector3(0.35f, 1.05f, 0f), new Vector3(0.18f, 0.55f, 0.22f), new Color(0.20f, 0.42f, 0.78f), standardShader);

            // 4. Legs
            _legLeft = CreateLimb("LegLeft", _visualRoot, new Vector3(-0.15f, 0.55f, 0f), new Vector3(0.20f, 0.55f, 0.24f), new Color(0.22f, 0.25f, 0.32f), standardShader);
            _legRight = CreateLimb("LegRight", _visualRoot, new Vector3(0.15f, 0.55f, 0f), new Vector3(0.20f, 0.55f, 0.24f), new Color(0.22f, 0.25f, 0.32f), standardShader);

            // 5. Personal Adventurer Aura Light (Guarantees character & cave walls are never pitch black)
            GameObject auraObj = new GameObject("PlayerAuraLight");
            auraObj.transform.SetParent(transform, false);
            auraObj.transform.localPosition = new Vector3(0f, 1.0f, -0.5f);
            Light auraLight = auraObj.AddComponent<Light>();
            auraLight.type = LightType.Point;
            auraLight.range = 7.5f;
            auraLight.intensity = 1.6f;
            auraLight.color = new Color(1.0f, 0.96f, 0.88f);
            auraLight.shadows = LightShadows.None;
        }

        private void CacheLimbReferences()
        {
            _torsoTransform = _visualRoot.Find("Torso");
            _headTransform = _visualRoot.Find("Head");
            _armLeft = _visualRoot.Find("ArmLeft");
            _armRight = _visualRoot.Find("ArmRight");
            _legLeft = _visualRoot.Find("LegLeft");
            _legRight = _visualRoot.Find("LegRight");
        }

        private Transform CreateLimb(string name, Transform parent, Vector3 localPos, Vector3 localScale, Color color, Shader shader)
        {
            GameObject limbPivot = new GameObject(name);
            limbPivot.transform.SetParent(parent, false);
            limbPivot.transform.localPosition = localPos;

            GameObject limbMesh = GameObject.CreatePrimitive(PrimitiveType.Cube);
            limbMesh.name = name + "Mesh";
            limbMesh.transform.SetParent(limbPivot.transform, false);
            limbMesh.transform.localPosition = new Vector3(0f, -localScale.y * 0.5f, 0f);
            limbMesh.transform.localScale = localScale;
            StripCollider(limbMesh);
            SetColor(limbMesh, color, shader);

            return limbPivot.transform;
        }

        private void CreateEye(Transform parent, Vector3 localPos, Shader shader)
        {
            GameObject eye = GameObject.CreatePrimitive(PrimitiveType.Cube);
            eye.name = "Eye";
            eye.transform.SetParent(parent, false);
            eye.transform.localPosition = localPos;
            eye.transform.localScale = new Vector3(0.12f, 0.16f, 0.08f);
            StripCollider(eye);
            SetColor(eye, Color.white, shader);

            // Pupil
            GameObject pupil = GameObject.CreatePrimitive(PrimitiveType.Cube);
            pupil.name = "Pupil";
            pupil.transform.SetParent(eye.transform, false);
            pupil.transform.localPosition = new Vector3(0.18f, 0f, 0.55f);
            pupil.transform.localScale = new Vector3(0.55f, 0.7f, 0.35f);
            StripCollider(pupil);
            SetColor(pupil, new Color(0.1f, 0.1f, 0.15f), shader);
        }

        private static void StripCollider(GameObject go)
        {
            Collider col = go.GetComponent<Collider>();
            if (col != null) Destroy(col);
        }

        private static void SetColor(GameObject go, Color color, Shader shader)
        {
            MeshRenderer mr = go.GetComponent<MeshRenderer>();
            if (mr != null)
            {
                Material mat = new Material(shader) { color = color };
                if (mat.HasProperty("_EmissionColor"))
                {
                    mat.SetColor("_EmissionColor", color * 0.35f);
                    mat.EnableKeyword("_EMISSION");
                }
                mr.material = mat;
            }
        }

        /// <summary>
        /// Sets current and max health values.
        /// </summary>
        public void SetHealth(float current, float max = 100f)
        {
            maxHealth = Mathf.Max(1f, max);
            currentHealth = Mathf.Clamp(current, 0f, maxHealth);
            isDead = currentHealth <= 0f;
        }

        /// <summary>
        /// Restores player health up to maxHealth.
        /// </summary>
        public void Heal(float amount)
        {
            if (isDead) return;
            currentHealth = Mathf.Min(maxHealth, currentHealth + amount);
        }

        /// <summary>
        /// Deals damage to the player in Survival mode, triggers invulnerability frames,
        /// hurt sound, red hurt flash, and directional knockback.
        /// </summary>
        public void TakeDamage(float amount, Vector2 knockbackDirection)
        {
            if (currentGameMode != GameMode.Survival) return;
            if (isDead || hurtTimer > 0f) return;

            currentHealth = Mathf.Max(0f, currentHealth - amount);
            hurtTimer = InvulnerableDuration;
            timeSinceCombat = 0f;
            _hurtFlashTimer = 0.28f;

            // Apply knockback
            if (knockbackDirection.sqrMagnitude < 0.001f)
            {
                knockbackDirection = isFacingRight ? Vector2.left : Vector2.right;
            }
            knockbackDirection.Normalize();

            velocity.x = knockbackDirection.x * 9.5f;
            velocity.y = Mathf.Max(knockbackDirection.y * 7.0f, 6.5f);
            isGrounded = false;

            // Audio feedback
            PlayHurtAudio();

            // Floating damage number popup
            if (UI.UIManager.Instance != null)
            {
                UI.UIManager.Instance.SpawnFloatingText($"-{amount:F0} ❤️", new Color(1.0f, 0.25f, 0.25f));
            }

            if (currentHealth <= 0f)
            {
                Die();
            }
        }

        private void Die()
        {
            isDead = true;
            velocity = Vector3.zero;
            if (UI.UIManager.Instance != null)
            {
                UI.UIManager.Instance.SpawnFloatingText("☠️ 你阵亡了! [R] 键重生 (You Died! Press [R] to Respawn)", Color.red);
            }
        }

        private void PlayHurtAudio()
        {
            EnsureAudioSource();
            if (_audioSource == null) return;

            int sampleRate = 44100;
            float duration = 0.14f;
            int totalSamples = (int)(sampleRate * duration);
            float[] data = new float[totalSamples];

            for (int i = 0; i < totalSamples; i++)
            {
                float t = (float)i / sampleRate;
                float env = Mathf.Exp(-t * 26f);
                float freq = Mathf.Lerp(190f, 85f, (float)i / totalSamples);
                data[i] = Mathf.Sin(2f * Mathf.PI * freq * t) * env * 0.45f;
            }

            AudioClip clip = AudioClip.Create("PlayerHurt", totalSamples, 1, sampleRate, false);
            clip.SetData(data, 0);
            _audioSource.PlayOneShot(clip);
        }

        private void EnsureAudioSource()
        {
            if (_audioSource == null)
            {
                _audioSource = GetComponent<AudioSource>() ?? gameObject.AddComponent<AudioSource>();
                _audioSource.playOnAwake = false;
                _audioSource.spatialBlend = 0f;
            }
        }

        private void CacheVisualRenderers()
        {
            _visualRenderers.Clear();
            _originalVisualColors.Clear();
            if (_visualRoot == null) return;

            Renderer[] rends = _visualRoot.GetComponentsInChildren<Renderer>(true);
            for (int i = 0; i < rends.Length; i++)
            {
                Renderer r = rends[i];
                if (r != null && r.material != null)
                {
                    _visualRenderers.Add(r);
                    _originalVisualColors.Add(r.material.color);
                }
            }
        }

        private void UpdateHealthAndHurtFlash(float dt)
        {
            if (hurtTimer > 0f)
            {
                hurtTimer -= dt;
            }

            // Natural health regeneration: out of combat for 20s -> slow regen
            if (currentGameMode == GameMode.Survival && !isDead)
            {
                timeSinceCombat += dt;
                if (timeSinceCombat >= 20.0f && currentHealth < maxHealth)
                {
                    Heal(3.5f * dt);
                }
            }

            if (_hurtFlashTimer > 0f)
            {
                _hurtFlashTimer -= dt;
                float t = Mathf.Clamp01(_hurtFlashTimer / 0.28f);

                for (int i = 0; i < _visualRenderers.Count; i++)
                {
                    if (_visualRenderers[i] != null && _visualRenderers[i].material != null)
                    {
                        Color c = Color.Lerp(_originalVisualColors[i], Color.red, t * 0.85f);
                        _visualRenderers[i].material.color = c;
                    }
                }
            }
            else if (_visualRenderers.Count > 0 && hurtTimer <= 0f)
            {
                // Ensure restored
                for (int i = 0; i < _visualRenderers.Count; i++)
                {
                    if (_visualRenderers[i] != null && _visualRenderers[i].material != null)
                    {
                        if (_visualRenderers[i].material.color != _originalVisualColors[i])
                        {
                            _visualRenderers[i].material.color = _originalVisualColors[i];
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Instantly teleports the player to the specified target position on Z = 0.
        /// </summary>
        public void Teleport(Vector3 position)
        {
            position.z = 0f;
            _controller.enabled = false;
            transform.position = position;
            _controller.enabled = true;
            velocity = Vector3.zero;
            _currentHorizontalSpeed = 0f;
        }

        /// <summary>
        /// Respawns the player safely at the surface ground level, resetting health in Survival.
        /// </summary>
        public void Respawn(Vector3? targetPosition = null)
        {
            currentHealth = maxHealth;
            isDead = false;
            hurtTimer = 0f;
            timeSinceCombat = 25f;
            _hurtFlashTimer = 0f;

            for (int i = 0; i < _visualRenderers.Count; i++)
            {
                if (_visualRenderers[i] != null && _visualRenderers[i].material != null)
                {
                    _visualRenderers[i].material.color = _originalVisualColors[i];
                }
            }

            if (targetPosition.HasValue)
            {
                Teleport(targetPosition.Value);
                return;
            }

            float spawnX = transform.position.x;

            if (WorldManager.Instance != null)
            {
                Teleport(WorldManager.Instance.FindSurfaceSpawnPosition(spawnX));
                return;
            }

            // Fallback raycast down from high altitude or surface baseline
            float spawnY = WorldSettings.BaseSurfaceLevel + 10f;

            // Probe for solid surface
            RaycastHit hit;
            if (Physics.Raycast(new Vector3(spawnX, spawnY + 20f, 0f), Vector3.down, out hit, 60f, ~0, QueryTriggerInteraction.Ignore))
            {
                spawnY = hit.point.y + 1.5f;
            }
            else
            {
                spawnY = WorldSettings.BaseSurfaceLevel + 2f;
            }

            Teleport(new Vector3(spawnX, spawnY, 0f));
        }
    }
}
