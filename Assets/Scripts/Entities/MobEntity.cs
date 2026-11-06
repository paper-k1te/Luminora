using System;
using System.Collections;
using System.Collections.Generic;
using Core;
using Player;
using World;
using UnityEngine;

namespace Entities
{
    /// <summary>
    /// Autonomous 2.5D creature entity with state-machine AI, procedural visuals,
    /// dynamic animation (squash-and-stretch, wing flapping, biped swagger),
    /// combat health/damage, knockback physics, and loot drops.
    /// </summary>
    [RequireComponent(typeof(CharacterController))]
    [DisallowMultipleComponent]
    [SelectionBase]
    public class MobEntity : MonoBehaviour
    {
        [Header("Mob Configuration")]
        [SerializeField] private MobType mobType = MobType.Slime;
        [SerializeField] private int maxHealth = 25;
        [SerializeField] private int currentHealth = 25;
        [SerializeField] private float moveSpeed = 3.5f;

        [Header("Runtime State")]
        [SerializeField] private bool isFacingRight = true;
        [SerializeField] private bool isGrounded = false;
        [SerializeField] private Vector3 velocity;
        [SerializeField] private int state = 0;

        // Visual transforms for procedural animation
        private Transform _visualRoot;
        private Transform _torsoTransform;
        private Transform _headTransform;
        private Transform _armLeft;
        private Transform _armRight;
        private Transform _legLeft;
        private Transform _legRight;
        private Transform _wingLeft;
        private Transform _wingRight;
        private Transform _slimeCore;
        private Transform _healthBarRoot;
        private Transform _healthBarFill;

        // Visual Materials & Hurt Flash
        private List<Renderer> _renderers = new List<Renderer>();
        private List<Color> _originalColors = new List<Color>();
        private float _hurtFlashTimer = 0f;
        private CharacterController _controller;
        private Transform _playerTransform;

        // Animation Timers
        private float _animTimer = 0f;
        private float _walkCycle = 0f;
        private float _stateTimer = 0f;

        // Sound Feedback
        private AudioSource _audioSource;

        public MobType Type => mobType;
        public int Health => currentHealth;
        public int MaxHealth => maxHealth;
        public bool IsDead => currentHealth <= 0;
        public Vector3 Position => transform.position;

        public static event Action<MobEntity> OnMobDied;

        private void Awake()
        {
            _controller = GetComponent<CharacterController>();
            ConfigureController();
            EnsureAudioSource();
        }

        private void Start()
        {
            SnapToGameplayPlane();
            InitializeStats();
            EnsureVisualRepresentation();
            FindPlayer();
        }

        private void ConfigureController()
        {
            if (_controller == null) return;
            _controller.skinWidth = 0.04f;
            _controller.minMoveDistance = 0f;
            _controller.stepOffset = 0.4f;

            switch (mobType)
            {
                case MobType.Slime:
                    _controller.radius = 0.42f;
                    _controller.height = 0.72f;
                    _controller.center = new Vector3(0f, 0.36f, 0f);
                    break;
                case MobType.Bat:
                    _controller.radius = 0.35f;
                    _controller.height = 0.55f;
                    _controller.center = new Vector3(0f, 0.28f, 0f);
                    break;
                case MobType.Zombie:
                default:
                    _controller.radius = 0.38f;
                    _controller.height = 1.80f;
                    _controller.center = new Vector3(0f, 0.90f, 0f);
                    break;
            }
        }

        private void EnsureAudioSource()
        {
            _audioSource = GetComponent<AudioSource>();
            if (_audioSource == null)
            {
                _audioSource = gameObject.AddComponent<AudioSource>();
                _audioSource.playOnAwake = false;
                _audioSource.spatialBlend = 0.8f;
                _audioSource.minDistance = 3f;
                _audioSource.maxDistance = 25f;
            }
        }

        private void InitializeStats()
        {
            switch (mobType)
            {
                case MobType.Slime:
                    maxHealth = 25;
                    moveSpeed = 4.2f;
                    break;
                case MobType.Bat:
                    maxHealth = 16;
                    moveSpeed = 4.5f;
                    break;
                case MobType.Zombie:
                default:
                    maxHealth = 45;
                    moveSpeed = 2.4f;
                    break;
            }
            currentHealth = maxHealth;
            _animTimer = UnityEngine.Random.Range(0f, 6.28f);
        }

        private void FindPlayer()
        {
            if (_playerTransform == null)
            {
                if (WorldManager.Instance != null && WorldManager.Instance.PlayerTransform != null)
                {
                    _playerTransform = WorldManager.Instance.PlayerTransform;
                }
                else
                {
                    GameObject p = GameObject.FindWithTag("Player");
                    if (p != null) _playerTransform = p.transform;
                }
            }
        }

        private void Update()
        {
            float dt = Time.deltaTime;
            if (dt <= 0f) return;

            FindPlayer();
            UpdateHurtFlash(dt);

            switch (mobType)
            {
                case MobType.Slime:
                    UpdateSlimeAI(dt);
                    break;
                case MobType.Bat:
                    UpdateBatAI(dt);
                    break;
                case MobType.Zombie:
                    UpdateZombieAI(dt);
                    break;
            }

            UpdateFacingDirection();
            UpdateHealthBarVisual();
            CheckPlayerContactDamage();
        }

        /// <summary>
        /// Detects contact collision with player: when mob collides with player,
        /// if game mode is Survival and player hurtTimer <= 0, deal contact damage
        /// (Slime: 10, Bat: 8, Zombie: 18) and knock player back.
        /// </summary>
        private void CheckPlayerContactDamage()
        {
            if (IsDead || _playerTransform == null) return;

            PlayerController2D5 player = _playerTransform.GetComponent<PlayerController2D5>();
            if (player == null || player.IsDead) return;

            ApplyContactDamageToPlayer(player);
        }

        private void ApplyContactDamageToPlayer(PlayerController2D5 player)
        {
            if (IsDead || player == null || player.IsDead) return;
            if (player.CurrentGameMode != GameMode.Survival || player.HurtTimer > 0f) return;

            Vector3 playerCenter = player.Position + Vector3.up * 0.9f;
            Vector3 mobCenter = transform.position;
            float contactRadius = 0.85f;

            switch (mobType)
            {
                case MobType.Slime:
                    mobCenter += Vector3.up * 0.36f;
                    contactRadius = 0.95f;
                    break;
                case MobType.Bat:
                    mobCenter += Vector3.up * 0.28f;
                    contactRadius = 0.85f;
                    break;
                case MobType.Zombie:
                default:
                    mobCenter += Vector3.up * 0.90f;
                    contactRadius = 1.05f;
                    break;
            }

            float dist = Vector2.Distance(new Vector2(playerCenter.x, playerCenter.y), new Vector2(mobCenter.x, mobCenter.y));
            if (dist <= contactRadius)
            {
                int damage = 10;
                switch (mobType)
                {
                    case MobType.Slime:
                        damage = 10;
                        break;
                    case MobType.Bat:
                        damage = 8;
                        break;
                    case MobType.Zombie:
                    default:
                        damage = 18;
                        break;
                }

                Vector2 knockDir = (player.Position - transform.position);
                if (Mathf.Abs(knockDir.x) < 0.1f)
                {
                    knockDir.x = isFacingRight ? 1f : -1f;
                }
                knockDir.y = 0.6f;
                knockDir.Normalize();

                player.TakeDamage(damage, knockDir);
            }
        }

        private void OnControllerColliderHit(ControllerColliderHit hit)
        {
            if (IsDead || hit == null || hit.gameObject == null) return;
            PlayerController2D5 player = hit.gameObject.GetComponent<PlayerController2D5>();
            if (player != null)
            {
                ApplyContactDamageToPlayer(player);
            }
        }

        private void LateUpdate()
        {
            SnapToGameplayPlane();
        }

        private void SnapToGameplayPlane()
        {
            Vector3 pos = transform.position;
            if (Mathf.Abs(pos.z) > 0.001f)
            {
                pos.z = 0f;
                transform.position = pos;
            }
        }

        // ====================================================================
        // AI State Machines
        // ====================================================================

        #region Slime AI
        private void UpdateSlimeAI(float dt)
        {
            _animTimer += dt;
            _stateTimer -= dt;

            // Gravity
            velocity.y -= 28.0f * dt;

            float distToPlayer = 999f;
            float dirToPlayer = 0f;
            if (_playerTransform != null)
            {
                float dx = _playerTransform.position.x - transform.position.x;
                distToPlayer = Mathf.Abs(dx);
                dirToPlayer = Mathf.Sign(dx);
            }

            // State 0: Idle on ground
            if (state == 0)
            {
                velocity.x = Mathf.MoveTowards(velocity.x, 0f, 15.0f * dt);

                if (_stateTimer <= 0f && isGrounded)
                {
                    // Leap into the air!
                    state = 1; // Hopping
                    isGrounded = false;
                    _stateTimer = 0.8f;

                    float leapDir = (distToPlayer < 18f) ? dirToPlayer : (UnityEngine.Random.value < 0.5f ? 1f : -1f);
                    isFacingRight = leapDir > 0f;
                    velocity.x = leapDir * (moveSpeed + UnityEngine.Random.Range(-0.5f, 0.8f));
                    velocity.y = 9.8f + UnityEngine.Random.Range(0f, 2.0f);
                }
            }
            // State 1: In Air Hopping
            else if (state == 1)
            {
                if (isGrounded && velocity.y <= 0.01f)
                {
                    state = 2; // Impact landing
                    _stateTimer = 0.20f;
                    velocity.x *= 0.5f;
                }
            }
            // State 2: Landing squash
            else if (state == 2)
            {
                velocity.x = Mathf.MoveTowards(velocity.x, 0f, 20.0f * dt);
                if (_stateTimer <= 0f)
                {
                    state = 0; // Back to idle
                    _stateTimer = UnityEngine.Random.Range(0.6f, 1.4f);
                }
            }

            // Apply movement
            ApplyControllerMovement(dt);

            // Animate Squash and Stretch
            AnimateSlimeVisuals();
        }

        private void AnimateSlimeVisuals()
        {
            if (_visualRoot == null) return;

            float sx = 1.0f, sy = 1.0f;
            if (state == 0)
            {
                if (_stateTimer < 0.22f)
                {
                    // Anticipation squash before hop
                    float t = 1.0f - (_stateTimer / 0.22f);
                    sx = 1.0f + 0.32f * t;
                    sy = 1.0f - 0.26f * t;
                }
                else
                {
                    // Gentle breathing idle
                    float b = Mathf.Sin(_animTimer * 5.0f);
                    sx = 1.0f + b * 0.05f;
                    sy = 1.0f - b * 0.05f;
                }
            }
            else if (state == 1)
            {
                // In-air vertical stretch
                sx = 0.82f;
                sy = 1.25f;
            }
            else if (state == 2)
            {
                // Ground impact squash
                float t = Mathf.Clamp01(_stateTimer / 0.20f);
                sx = 1.0f + 0.38f * t;
                sy = 1.0f - 0.30f * t;
            }

            _visualRoot.localScale = new Vector3(sx, sy, 1.0f);

            if (_slimeCore != null)
            {
                float coreBob = Mathf.Sin(_animTimer * 6.0f) * 0.04f;
                _slimeCore.localPosition = new Vector3(0f, 0.35f + coreBob, 0f);
            }
        }
        #endregion

        #region Bat AI
        private void UpdateBatAI(float dt)
        {
            _animTimer += dt;

            Vector3 targetPos = transform.position;
            bool chasing = false;

            if (_playerTransform != null)
            {
                Vector3 diff = _playerTransform.position - transform.position;
                float dist = diff.magnitude;

                if (dist < 16.0f)
                {
                    chasing = true;
                    float targetVX = Mathf.Sign(diff.x) * moveSpeed;
                    float targetVY = (diff.y / (dist + 0.01f)) * 3.8f + Mathf.Sin(_animTimer * 5.0f) * 1.5f;

                    velocity.x = Mathf.MoveTowards(velocity.x, targetVX, 12.0f * dt);
                    velocity.y = Mathf.MoveTowards(velocity.y, targetVY, 12.0f * dt);
                    isFacingRight = diff.x > 0f;
                }
            }

            if (!chasing)
            {
                // Smooth sine-wave gliding hover
                float targetVX = isFacingRight ? 2.5f : -2.5f;
                float targetVY = Mathf.Sin(_animTimer * 2.8f) * 2.0f;

                velocity.x = Mathf.MoveTowards(velocity.x, targetVX, 8.0f * dt);
                velocity.y = Mathf.MoveTowards(velocity.y, targetVY, 8.0f * dt);

                // Turn around if wall ahead
                RaycastHit hit;
                Vector3 fwd = isFacingRight ? Vector3.right : Vector3.left;
                if (Physics.Raycast(transform.position + Vector3.up * 0.3f, fwd, out hit, 1.0f))
                {
                    isFacingRight = !isFacingRight;
                }
            }

            // Apply movement (bats fly without gravity)
            Vector3 motion = velocity * dt;
            motion.z = 0f;
            _controller.Move(motion);

            // Animate Articulated Flapping Wings
            AnimateBatVisuals();
        }

        private void AnimateBatVisuals()
        {
            if (_wingLeft == null || _wingRight == null) return;

            // Wing flap frequency (14 Hz)
            float flapAngle = Mathf.Sin(_animTimer * 18.0f) * 45.0f;
            _wingLeft.localRotation = Quaternion.Euler(0f, 0f, flapAngle);
            _wingRight.localRotation = Quaternion.Euler(0f, 0f, -flapAngle);
        }
        #endregion

        #region Zombie AI
        private void UpdateZombieAI(float dt)
        {
            _animTimer += dt;

            // Gravity
            velocity.y -= 30.0f * dt;

            float targetVX = 0f;
            if (_playerTransform != null)
            {
                Vector3 diff = _playerTransform.position - transform.position;
                float dist = diff.magnitude;

                if (dist < 22.0f)
                {
                    // Pursue player
                    float dir = Mathf.Sign(diff.x);
                    isFacingRight = dir > 0f;
                    targetVX = dir * moveSpeed;
                }
                else
                {
                    // Idle wander
                    float wanderDir = Mathf.Sin(_animTimer * 0.35f) > 0f ? 1f : -1f;
                    isFacingRight = wanderDir > 0f;
                    targetVX = wanderDir * (moveSpeed * 0.5f);
                }
            }

            float accel = isGrounded ? 30.0f : 15.0f;
            velocity.x = Mathf.MoveTowards(velocity.x, targetVX, accel * dt);

            // Terraria 1-Block Step Climbing for Zombie
            HandleZombieStepClimbing(dt);

            // Apply movement
            ApplyControllerMovement(dt);

            // Animate Bipedal Swagger & Outstretched Arms
            AnimateZombieVisuals(dt);
        }

        private void HandleZombieStepClimbing(float dt)
        {
            if (!isGrounded || Mathf.Abs(velocity.x) < 0.2f) return;

            Vector3 stepDir = isFacingRight ? Vector3.right : Vector3.left;
            Vector3 footPos = transform.position + Vector3.up * 0.15f;
            Vector3 kneePos = transform.position + Vector3.up * 0.85f;
            Vector3 headPos = transform.position + Vector3.up * 1.95f;

            // Check if foot hits an obstacle, knee is clear or near top, and head is clear
            bool footBlocked = Physics.Raycast(footPos, stepDir, 0.55f);
            bool headBlocked = Physics.Raycast(headPos, stepDir, 0.55f);

            if (footBlocked && !headBlocked)
            {
                // Smooth step climb up
                velocity.y = 12.0f;
            }
        }

        private void AnimateZombieVisuals(float dt)
        {
            if (_visualRoot == null) return;

            float horizontalSpeed = Mathf.Abs(velocity.x);
            if (isGrounded && horizontalSpeed > 0.15f)
            {
                _walkCycle += horizontalSpeed * 2.2f * dt;
                float sinWalk = Mathf.Sin(_walkCycle);

                if (_legLeft != null) _legLeft.localRotation = Quaternion.Euler(sinWalk * 34f, 0f, 0f);
                if (_legRight != null) _legRight.localRotation = Quaternion.Euler(-sinWalk * 34f, 0f, 0f);

                // Slight torso lurching during walk
                if (_torsoTransform != null)
                {
                    float lurch = Mathf.Abs(Mathf.Sin(_walkCycle * 2f)) * 0.04f;
                    _torsoTransform.localPosition = new Vector3(0f, 0.90f - lurch, 0f);
                }
            }
            else
            {
                if (_legLeft != null) _legLeft.localRotation = Quaternion.identity;
                if (_legRight != null) _legRight.localRotation = Quaternion.identity;
            }

            // Signature Outstretched Zombie Arms
            float armLurch = Mathf.Sin(_animTimer * 3.5f) * 6.0f;
            if (_armLeft != null) _armLeft.localRotation = Quaternion.Euler(-80f + armLurch, 0f, 0f);
            if (_armRight != null) _armRight.localRotation = Quaternion.Euler(-80f - armLurch, 0f, 0f);
        }
        #endregion

        private void ApplyControllerMovement(float dt)
        {
            Vector3 motion = velocity * dt;
            motion.z = 0f;

            CollisionFlags flags = _controller.Move(motion);
            isGrounded = (flags & CollisionFlags.Below) != 0;

            if (isGrounded && velocity.y < 0f)
            {
                velocity.y = -0.5f; // Keep grounded contact
            }
        }

        private void UpdateFacingDirection()
        {
            if (_visualRoot != null)
            {
                float targetAngle = isFacingRight ? 90f : -90f;
                _visualRoot.localRotation = Quaternion.Euler(0f, targetAngle, 0f);
            }
        }

        // ====================================================================
        // Combat, Damage & Knockback
        // ====================================================================

        public void TakeDamage(int amount, Vector3 knockbackOrigin)
        {
            if (IsDead) return;

            currentHealth -= amount;
            _hurtFlashTimer = 0.32f;

            // Knockback away from attacker
            Vector3 knockDir = (transform.position - knockbackOrigin);
            knockDir.z = 0f;
            if (knockDir.sqrMagnitude > 0.001f)
            {
                knockDir.Normalize();
            }
            else
            {
                knockDir = isFacingRight ? Vector3.left : Vector3.right;
            }

            velocity = knockDir * 7.5f + Vector3.up * 5.5f;
            isGrounded = false;

            // Audio & Particle feedback
            PlayHitFeedback();

            // Death Check
            if (currentHealth <= 0)
            {
                Die();
            }
        }

        private void PlayHitFeedback()
        {
            if (_audioSource != null)
            {
                _audioSource.pitch = UnityEngine.Random.Range(0.85f, 1.25f);
                // Synthesize pop/whack click
                _audioSource.PlayOneShot(CreateBeepAudioClip(220f, 0.08f));
            }

            // Spawn hit particle puff
            SpawnHitParticles();
        }

        private void SpawnHitParticles()
        {
            Color pColor = Color.red;
            if (mobType == MobType.Slime) pColor = new Color(0.25f, 0.95f, 0.35f);
            else if (mobType == MobType.Bat) pColor = new Color(0.65f, 0.25f, 0.75f);

            GameObject puff = new GameObject("HitParticlePuff");
            puff.transform.position = transform.position + Vector3.up * 0.5f;
            ParticleSystem ps = puff.AddComponent<ParticleSystem>();
            var main = ps.main;
            main.startColor = pColor;
            main.startSize = 0.18f;
            main.startSpeed = 4.0f;
            main.startLifetime = 0.35f;
            main.duration = 0.2f;
            main.loop = false;
            var emission = ps.emission;
            emission.rateOverTime = 0;
            emission.SetBursts(new ParticleSystem.Burst[] { new ParticleSystem.Burst(0f, 10) });
            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.2f;

            Destroy(puff, 0.5f);
        }

        private void Die()
        {
            OnMobDied?.Invoke(this);

            // Spawn death particles
            SpawnDeathParticles();

            // Terraria Authentic Loot Drops
            DropLoot();

            // Clean up entity
            Destroy(gameObject);
        }

        private void DropLoot()
        {
            Vector3 dropPos = transform.position + Vector3.up * 0.4f;

            switch (mobType)
            {
                case MobType.Slime:
                    // Slime Gel drops
                    DroppedItem.Spawn(dropPos, BlockType.Gel, 2);
                    break;

                case MobType.Bat:
                    // Coal Ore or rare Ruby
                    BlockType batDrop = UnityEngine.Random.value < 0.25f ? BlockType.RubyOre : BlockType.CoalOre;
                    DroppedItem.Spawn(dropPos, batDrop, 1);
                    break;

                case MobType.Zombie:
                default:
                    // Iron Ore or rare Gold Ore
                    BlockType zombieDrop = UnityEngine.Random.value < 0.30f ? BlockType.GoldOre : BlockType.IronOre;
                    DroppedItem.Spawn(dropPos, zombieDrop, 1);
                    break;
            }
        }

        private void SpawnDeathParticles()
        {
            Color pColor = Color.red;
            if (mobType == MobType.Slime) pColor = new Color(0.25f, 0.95f, 0.35f);
            else if (mobType == MobType.Bat) pColor = new Color(0.65f, 0.25f, 0.75f);

            GameObject deathFX = new GameObject("DeathExplosionFX");
            deathFX.transform.position = transform.position + Vector3.up * 0.5f;
            ParticleSystem ps = deathFX.AddComponent<ParticleSystem>();
            var main = ps.main;
            main.startColor = pColor;
            main.startSize = 0.22f;
            main.startSpeed = 6.0f;
            main.startLifetime = 0.5f;
            main.duration = 0.3f;
            main.loop = false;
            var emission = ps.emission;
            emission.rateOverTime = 0;
            emission.SetBursts(new ParticleSystem.Burst[] { new ParticleSystem.Burst(0f, 18) });
            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.4f;

            Destroy(deathFX, 0.7f);
        }

        private void UpdateHurtFlash(float dt)
        {
            if (_hurtFlashTimer > 0f)
            {
                _hurtFlashTimer -= dt;
                float t = Mathf.Clamp01(_hurtFlashTimer / 0.32f);

                for (int i = 0; i < _renderers.Count; i++)
                {
                    if (_renderers[i] != null && _renderers[i].material != null)
                    {
                        Color c = Color.Lerp(_originalColors[i], Color.red, t * 0.85f);
                        _renderers[i].material.color = c;
                    }
                }
            }
        }

        // ====================================================================
        // Procedural 2.5D Visual Generation
        // ====================================================================

        public void Configure(MobType type)
        {
            this.mobType = type;
            ConfigureController();
            InitializeStats();
            EnsureVisualRepresentation();
        }

        private void EnsureVisualRepresentation()
        {
            if (_visualRoot != null)
            {
                Destroy(_visualRoot.gameObject);
            }

            GameObject root = new GameObject("ProceduralMobVisual");
            root.transform.SetParent(transform, false);
            root.transform.localPosition = Vector3.zero;
            root.transform.localRotation = Quaternion.Euler(0f, 90f, 0f); // Side-scrolling facing
            _visualRoot = root.transform;

            _renderers.Clear();
            _originalColors.Clear();

            Shader standardShader = Shader.Find("Mobile/Diffuse") ?? Shader.Find("Standard") ?? Shader.Find("Sprites/Default");

            switch (mobType)
            {
                case MobType.Slime:
                    BuildSlimeVisuals(standardShader);
                    break;
                case MobType.Bat:
                    BuildBatVisuals(standardShader);
                    break;
                case MobType.Zombie:
                default:
                    BuildZombieVisuals(standardShader);
                    break;
            }

            BuildHealthBar();
        }

        private void BuildSlimeVisuals(Shader shader)
        {
            // Outer Translucent Gelatinous Body
            GameObject body = GameObject.CreatePrimitive(PrimitiveType.Cube);
            body.name = "SlimeBody";
            body.transform.SetParent(_visualRoot, false);
            body.transform.localPosition = new Vector3(0f, 0.36f, 0f);
            body.transform.localScale = new Vector3(0.85f, 0.72f, 0.45f);
            StripCollider(body);
            Color slimeGreen = new Color(0.24f, 0.90f, 0.35f, 0.80f);
            SetColor(body, slimeGreen, shader);

            // Inner Floating Core Nucleus
            GameObject core = GameObject.CreatePrimitive(PrimitiveType.Cube);
            core.name = "SlimeCore";
            core.transform.SetParent(_visualRoot, false);
            core.transform.localPosition = new Vector3(0f, 0.35f, 0f);
            core.transform.localScale = new Vector3(0.36f, 0.32f, 0.30f);
            StripCollider(core);
            Color coreGreen = new Color(0.12f, 0.58f, 0.22f, 0.95f);
            SetColor(core, coreGreen, shader);
            _slimeCore = core.transform;

            // Two Cute Dark Eyes (facing +Z in local space)
            CreateSlimeEye(_visualRoot, new Vector3(0.16f, 0.40f, 0.24f), shader);
            CreateSlimeEye(_visualRoot, new Vector3(-0.16f, 0.40f, 0.24f), shader);
        }

        private void CreateSlimeEye(Transform parent, Vector3 localPos, Shader shader)
        {
            GameObject eye = GameObject.CreatePrimitive(PrimitiveType.Cube);
            eye.name = "Eye";
            eye.transform.SetParent(parent, false);
            eye.transform.localPosition = localPos;
            eye.transform.localScale = new Vector3(0.10f, 0.16f, 0.05f);
            StripCollider(eye);
            SetColor(eye, new Color(0.08f, 0.12f, 0.08f), shader);

            // Specular dot
            GameObject pupil = GameObject.CreatePrimitive(PrimitiveType.Cube);
            pupil.name = "Dot";
            pupil.transform.SetParent(eye.transform, false);
            pupil.transform.localPosition = new Vector3(0.15f, 0.25f, 0.52f);
            pupil.transform.localScale = new Vector3(0.4f, 0.4f, 0.2f);
            StripCollider(pupil);
            SetColor(pupil, Color.white, shader);
        }

        private void BuildBatVisuals(Shader shader)
        {
            Color batDark = new Color(0.28f, 0.22f, 0.34f);
            Color wingMembrane = new Color(0.38f, 0.28f, 0.44f);

            // Central Bat Body
            GameObject body = GameObject.CreatePrimitive(PrimitiveType.Cube);
            body.name = "BatBody";
            body.transform.SetParent(_visualRoot, false);
            body.transform.localPosition = new Vector3(0f, 0.28f, 0f);
            body.transform.localScale = new Vector3(0.32f, 0.40f, 0.28f);
            StripCollider(body);
            SetColor(body, batDark, shader);

            // Pointed Bat Ears
            CreateBatEar(_visualRoot, new Vector3(-0.10f, 0.52f, 0f), batDark, shader);
            CreateBatEar(_visualRoot, new Vector3(0.10f, 0.52f, 0f), batDark, shader);

            // Glowing Crimson Vampire Eyes
            CreateBatEye(_visualRoot, new Vector3(-0.08f, 0.32f, 0.15f), shader);
            CreateBatEye(_visualRoot, new Vector3(0.08f, 0.32f, 0.15f), shader);

            // Left Flapping Wing Pivot & Hierarchy
            GameObject wingLPivot = new GameObject("WingLeftPivot");
            wingLPivot.transform.SetParent(_visualRoot, false);
            wingLPivot.transform.localPosition = new Vector3(-0.16f, 0.30f, 0f);
            _wingLeft = wingLPivot.transform;

            GameObject wingLMesh = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wingLMesh.name = "WingL";
            wingLMesh.transform.SetParent(_wingLeft, false);
            wingLMesh.transform.localPosition = new Vector3(-0.30f, 0f, 0f);
            wingLMesh.transform.localScale = new Vector3(0.60f, 0.32f, 0.05f);
            StripCollider(wingLMesh);
            SetColor(wingLMesh, wingMembrane, shader);

            // Right Flapping Wing Pivot & Hierarchy
            GameObject wingRPivot = new GameObject("WingRightPivot");
            wingRPivot.transform.SetParent(_visualRoot, false);
            wingRPivot.transform.localPosition = new Vector3(0.16f, 0.30f, 0f);
            _wingRight = wingRPivot.transform;

            GameObject wingRMesh = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wingRMesh.name = "WingR";
            wingRMesh.transform.SetParent(_wingRight, false);
            wingRMesh.transform.localPosition = new Vector3(0.30f, 0f, 0f);
            wingRMesh.transform.localScale = new Vector3(0.60f, 0.32f, 0.05f);
            StripCollider(wingRMesh);
            SetColor(wingRMesh, wingMembrane, shader);
        }

        private void CreateBatEar(Transform parent, Vector3 localPos, Color color, Shader shader)
        {
            GameObject ear = GameObject.CreatePrimitive(PrimitiveType.Cube);
            ear.name = "Ear";
            ear.transform.SetParent(parent, false);
            ear.transform.localPosition = localPos;
            ear.transform.localScale = new Vector3(0.08f, 0.16f, 0.08f);
            StripCollider(ear);
            SetColor(ear, color, shader);
        }

        private void CreateBatEye(Transform parent, Vector3 localPos, Shader shader)
        {
            GameObject eye = GameObject.CreatePrimitive(PrimitiveType.Cube);
            eye.name = "BatEye";
            eye.transform.SetParent(parent, false);
            eye.transform.localPosition = localPos;
            eye.transform.localScale = new Vector3(0.06f, 0.06f, 0.04f);
            StripCollider(eye);
            SetColor(eye, new Color(1.0f, 0.15f, 0.15f), shader);
        }

        private void BuildZombieVisuals(Shader shader)
        {
            Color skinGreen = new Color(0.34f, 0.64f, 0.36f);
            Color shirtBlue = new Color(0.22f, 0.46f, 0.62f);
            Color pantsBlue = new Color(0.16f, 0.20f, 0.32f);

            // 1. Torso
            GameObject torso = GameObject.CreatePrimitive(PrimitiveType.Cube);
            torso.name = "Torso";
            torso.transform.SetParent(_visualRoot, false);
            torso.transform.localPosition = new Vector3(0f, 0.90f, 0f);
            torso.transform.localScale = new Vector3(0.48f, 0.65f, 0.32f);
            StripCollider(torso);
            SetColor(torso, shirtBlue, shader);
            _torsoTransform = torso.transform;

            // 2. Head
            GameObject head = GameObject.CreatePrimitive(PrimitiveType.Cube);
            head.name = "Head";
            head.transform.SetParent(_visualRoot, false);
            head.transform.localPosition = new Vector3(0f, 1.48f, 0f);
            head.transform.localScale = new Vector3(0.45f, 0.45f, 0.42f);
            StripCollider(head);
            SetColor(head, skinGreen, shader);
            _headTransform = head.transform;

            // 2b. Menacing Eyes
            CreateZombieEye(_headTransform, new Vector3(0.12f, 0.05f, 0.52f), shader);
            CreateZombieEye(_headTransform, new Vector3(-0.12f, 0.05f, 0.52f), shader);

            // 3. Outstretched Horizontal Arms
            _armLeft = CreateLimb("ArmLeft", _visualRoot, new Vector3(-0.35f, 1.05f, 0f), new Vector3(0.18f, 0.55f, 0.22f), skinGreen, shader);
            _armRight = CreateLimb("ArmRight", _visualRoot, new Vector3(0.35f, 1.05f, 0f), new Vector3(0.18f, 0.55f, 0.22f), skinGreen, shader);

            // 4. Legs
            _legLeft = CreateLimb("LegLeft", _visualRoot, new Vector3(-0.15f, 0.55f, 0f), new Vector3(0.20f, 0.55f, 0.24f), pantsBlue, shader);
            _legRight = CreateLimb("LegRight", _visualRoot, new Vector3(0.15f, 0.55f, 0f), new Vector3(0.20f, 0.55f, 0.24f), pantsBlue, shader);
        }

        private void CreateZombieEye(Transform parent, Vector3 localPos, Shader shader)
        {
            GameObject eye = GameObject.CreatePrimitive(PrimitiveType.Cube);
            eye.name = "ZombieEye";
            eye.transform.SetParent(parent, false);
            eye.transform.localPosition = localPos;
            eye.transform.localScale = new Vector3(0.10f, 0.12f, 0.06f);
            StripCollider(eye);
            SetColor(eye, new Color(0.85f, 0.18f, 0.18f), shader);
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

        // ====================================================================
        // Floating Health Bar
        // ====================================================================

        private void BuildHealthBar()
        {
            float topY = (mobType == MobType.Zombie) ? 2.1f : ((mobType == MobType.Slime) ? 1.0f : 0.8f);

            GameObject barObj = new GameObject("HealthBar");
            barObj.transform.SetParent(transform, false);
            barObj.transform.localPosition = new Vector3(0f, topY, -0.05f);
            _healthBarRoot = barObj.transform;

            Shader shader = Shader.Find("Mobile/Diffuse") ?? Shader.Find("Standard") ?? Shader.Find("Sprites/Default");

            // Dark background bar
            GameObject bg = GameObject.CreatePrimitive(PrimitiveType.Cube);
            bg.name = "BG";
            bg.transform.SetParent(_healthBarRoot, false);
            bg.transform.localScale = new Vector3(0.85f, 0.10f, 0.02f);
            StripCollider(bg);
            SetColor(bg, new Color(0.12f, 0.12f, 0.12f, 0.85f), shader);

            // Fill bar
            GameObject fill = GameObject.CreatePrimitive(PrimitiveType.Cube);
            fill.name = "Fill";
            fill.transform.SetParent(_healthBarRoot, false);
            fill.transform.localPosition = new Vector3(0f, 0f, -0.01f);
            fill.transform.localScale = new Vector3(0.82f, 0.07f, 0.02f);
            StripCollider(fill);
            SetColor(fill, Color.green, shader);
            _healthBarFill = fill.transform;

            _healthBarRoot.gameObject.SetActive(false); // Hidden until damaged
        }

        private void UpdateHealthBarVisual()
        {
            if (_healthBarRoot == null || _healthBarFill == null) return;

            if (currentHealth < maxHealth && currentHealth > 0)
            {
                if (!_healthBarRoot.gameObject.activeSelf)
                {
                    _healthBarRoot.gameObject.SetActive(true);
                }

                float pct = Mathf.Clamp01((float)currentHealth / maxHealth);
                _healthBarFill.localScale = new Vector3(0.82f * pct, 0.07f, 0.02f);
                _healthBarFill.localPosition = new Vector3(-0.41f * (1f - pct), 0f, -0.01f);

                Renderer r = _healthBarFill.GetComponent<Renderer>();
                if (r != null && r.material != null)
                {
                    Color c = Color.Lerp(Color.red, Color.green, pct);
                    r.material.color = c;
                }
            }
            else
            {
                if (_healthBarRoot.gameObject.activeSelf)
                {
                    _healthBarRoot.gameObject.SetActive(false);
                }
            }
        }

        // ====================================================================
        // Helpers & Interaction
        // ====================================================================

        private void StripCollider(GameObject go)
        {
            Collider col = go.GetComponent<Collider>();
            if (col != null) Destroy(col);
        }

        private void SetColor(GameObject go, Color color, Shader shader)
        {
            Renderer rend = go.GetComponent<Renderer>();
            if (rend != null)
            {
                Material mat = new Material(shader);
                mat.color = color;
                rend.material = mat;

                _renderers.Add(rend);
                _originalColors.Add(color);
            }
        }

        private AudioClip CreateBeepAudioClip(float frequency, float duration)
        {
            int sampleRate = 44100;
            int samples = (int)(sampleRate * duration);
            float[] data = new float[samples];

            for (int i = 0; i < samples; i++)
            {
                float t = (float)i / sampleRate;
                data[i] = Mathf.Sin(2f * Mathf.PI * frequency * t) * (1f - (float)i / samples);
            }

            AudioClip clip = AudioClip.Create("HitBeep", samples, 1, sampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        private void OnMouseDown()
        {
            // Direct click attack fallback
            Vector3 attackerPos = _playerTransform != null ? _playerTransform.position : transform.position - Vector3.right;
            TakeDamage(10, attackerPos);
        }
    }
}
