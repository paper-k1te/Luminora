using System;
using Core;
using Rendering;
using UnityEngine;

namespace Player
{
    /// <summary>
    /// Interactive floating 3D voxel mini-block dropped in the world upon block destruction.
    /// Features procedural miniature cube rendering, bobbing and spinning animations,
    /// gravity settle physics on the Z = 0 plane, and magnetic attraction toward the player.
    /// </summary>
    [SelectionBase]
    [DisallowMultipleComponent]
    public class DroppedItem : MonoBehaviour
    {
        [Header("Item Data")]
        [SerializeField] private BlockType blockType = BlockType.Dirt;
        [SerializeField] private int itemCount = 1;

        [Header("Animation & Magnetics")]
        [SerializeField] private float bobAmplitude = 0.12f;
        [SerializeField] private float bobFrequency = 3.5f;
        [SerializeField] private float spinSpeed = 135f;
        [SerializeField] private float pickupRadius = 2.8f;
        [SerializeField] private float pickupCollectDistance = 0.45f;
        [SerializeField] private float initialAttractSpeed = 6f;
        [SerializeField] private float maxAttractSpeed = 22f;

        [Header("Physics")]
        [SerializeField] private float gravity = -20f;
        [SerializeField] private float groundCheckRadius = 0.18f;

        // Runtime state
        private Vector3 _velocity;
        private bool _isGrounded;
        private float _bobOffset;
        private float _spawnTime;
        private Transform _visualChild;
        private Transform _playerTransform;
        private MeshRenderer _meshRenderer;
        private Light _pointLight;

        public BlockType BlockType => blockType;
        public int Count => itemCount;

        public static event Action<BlockType, int> OnItemPickedUp;

        private void Awake()
        {
            _bobOffset = UnityEngine.Random.Range(0f, 10f);
            _spawnTime = Time.time;
        }

        private void Start()
        {
            FindPlayer();
            EnsureVisuals();
        }

        private void Update()
        {
            float dt = Time.deltaTime;

            // Enforce 2.5D plane constraint
            Vector3 pos = transform.position;
            pos.z = 0f;
            transform.position = pos;

            // Locate player if lost
            if (_playerTransform == null)
            {
                FindPlayer();
            }

            // Magnetic attraction check
            bool beingAttracted = false;
            if (_playerTransform != null && Time.time - _spawnTime > 0.2f)
            {
                Vector3 playerCenter = _playerTransform.position + Vector3.up * 0.9f;
                playerCenter.z = 0f;
                float distToPlayer = Vector3.Distance(transform.position, playerCenter);

                if (distToPlayer <= pickupRadius)
                {
                    beingAttracted = true;
                    float progress = 1f - Mathf.Clamp01(distToPlayer / pickupRadius);
                    float attractSpeed = Mathf.Lerp(initialAttractSpeed, maxAttractSpeed, progress * progress);

                    transform.position = Vector3.MoveTowards(transform.position, playerCenter, attractSpeed * dt);

                    if (distToPlayer <= pickupCollectDistance)
                    {
                        CollectItem();
                        return;
                    }
                }
            }

            // Physics gravity and ground settling when not pulled by magnet
            if (!beingAttracted)
            {
                ApplyGravity(dt);
            }

            // Visual spinning and bobbing
            AnimateVisuals(dt);
        }

        /// <summary>
        /// Applies simple vertical gravity with terrain ground collision detection.
        /// </summary>
        private void ApplyGravity(float dt)
        {
            if (!_isGrounded)
            {
                _velocity.y += gravity * dt;
                Vector3 newPos = transform.position + _velocity * dt;

                // Ground raycast check
                RaycastHit hit;
                Vector3 rayOrigin = transform.position + Vector3.up * 0.1f;
                if (Physics.SphereCast(rayOrigin, groundCheckRadius, Vector3.down, out hit, 0.25f, ~0, QueryTriggerInteraction.Ignore))
                {
                    if (hit.point.y <= transform.position.y)
                    {
                        newPos.y = hit.point.y + groundCheckRadius;
                        _velocity = Vector3.zero;
                        _isGrounded = true;
                    }
                }

                transform.position = newPos;
            }
        }

        /// <summary>
        /// Spins and bobs the visual representation independently of root physics.
        /// </summary>
        private void AnimateVisuals(float dt)
        {
            if (_visualChild != null)
            {
                // Continuous 3D rotation
                _visualChild.Rotate(Vector3.up, spinSpeed * dt, Space.World);

                // Gentle vertical bobbing
                float bobY = Mathf.Sin((Time.time + _bobOffset) * bobFrequency) * bobAmplitude;
                _visualChild.localPosition = new Vector3(0f, bobY, 0f);
            }
        }

        /// <summary>
        /// Delivers item to inventory and plays collection effects.
        /// </summary>
        private void CollectItem()
        {
            if (InventorySystem.Instance != null)
            {
                bool accepted = InventorySystem.Instance.AddItem(blockType, itemCount);
                if (accepted)
                {
                    PlayPickupFeedback();
                    OnItemPickedUp?.Invoke(blockType, itemCount);
                    Destroy(gameObject);
                }
            }
            else
            {
                Destroy(gameObject);
            }
        }

        /// <summary>
        /// Generates a charming burst of sparkly pick-up particles and audio feedback.
        /// </summary>
        private void PlayPickupFeedback()
        {
            // Spawn miniature particle burst
            GameObject fxObj = new GameObject($"PickupFX_{blockType}");
            fxObj.transform.position = transform.position;

            ParticleSystem ps = fxObj.AddComponent<ParticleSystem>();
            ParticleSystemRenderer psr = fxObj.GetComponent<ParticleSystemRenderer>();

            var main = ps.main;
            main.startLifetime = 0.45f;
            main.startSpeed = 3.5f;
            main.startSize = 0.12f;
            main.startColor = BlockRegistry.GetBlock(blockType).defaultColor;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.stopAction = ParticleSystemStopAction.Destroy;

            var emission = ps.emission;
            emission.SetBursts(new ParticleSystem.Burst[] { new ParticleSystem.Burst(0f, 12) });

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.2f;

            // Generate procedural pop audio tone
            AudioSource audio = fxObj.AddComponent<AudioSource>();
            audio.playOnAwake = false;
            audio.spatialBlend = 0f;
            audio.pitch = UnityEngine.Random.Range(1.2f, 1.4f);
            audio.volume = 0.4f;

            // Synthesize tiny pop sound
            int sampleRate = 44100;
            int length = (int)(sampleRate * 0.08f);
            float[] samples = new float[length];
            for (int i = 0; i < length; i++)
            {
                float t = (float)i / sampleRate;
                float envelope = Mathf.Exp(-t * 60f);
                samples[i] = Mathf.Sin(2f * Mathf.PI * 880f * t) * envelope;
            }
            AudioClip clip = AudioClip.Create("Pop", length, 1, sampleRate, false);
            clip.SetData(samples, 0);
            audio.clip = clip;
            audio.Play();

            Destroy(fxObj, 0.6f);
        }

        /// <summary>
        /// Finds player reference in scene.
        /// </summary>
        private void FindPlayer()
        {
            PlayerController2D5 pc = FindObjectOfType<PlayerController2D5>();
            if (pc != null)
            {
                _playerTransform = pc.transform;
            }
        }

        /// <summary>
        /// Constructs 3D miniature cube visual representation with proper color, texture, and emission.
        /// </summary>
        private void EnsureVisuals()
        {
            if (_visualChild != null) return;

            GameObject visual = GameObject.CreatePrimitive(PrimitiveType.Cube);
            visual.name = "MiniBlockVisual";
            visual.transform.SetParent(transform, false);
            visual.transform.localScale = Vector3.one * 0.38f;
            visual.transform.localPosition = Vector3.zero;

            // Remove primitive collider so it doesn't interfere with player physics
            Collider col = visual.GetComponent<Collider>();
            if (col != null) Destroy(col);

            _visualChild = visual.transform;
            _meshRenderer = visual.GetComponent<MeshRenderer>();

            ApplyBlockMaterial();
        }

        /// <summary>
        /// Configures block material, color, and optional light emission.
        /// </summary>
        private void ApplyBlockMaterial()
        {
            if (_meshRenderer == null) return;

            BlockData data = BlockRegistry.GetBlock(blockType);

            Material mat = null;
            try
            {
                // Attempt to bind atlas material
                mat = new Material(TextureAtlasGenerator.GetOrCreateAtlasMaterial());
            }
            catch
            {
                Shader diffuseShader = Shader.Find("Mobile/Diffuse") ?? Shader.Find("Standard") ?? Shader.Find("Sprites/Default");
                mat = new Material(diffuseShader);
            }

            if (mat != null)
            {
                mat.color = data.defaultColor;
                _meshRenderer.material = mat;
            }

            // Emissive block illumination (e.g. Torches, Magma, Glowing Mushrooms)
            if (data.emitLight)
            {
                GameObject lightObj = new GameObject("ItemGlow");
                lightObj.transform.SetParent(transform, false);
                _pointLight = lightObj.AddComponent<Light>();
                _pointLight.type = LightType.Point;
                _pointLight.range = 3.5f;
                _pointLight.intensity = 1.2f;
                _pointLight.color = data.lightColor;
            }
        }

        /// <summary>
        /// Configures item attributes after instantiation.
        /// </summary>
        public void Initialize(BlockType type, int count = 1, Vector3? initialVelocity = null)
        {
            this.blockType = type;
            this.itemCount = Mathf.Max(1, count);

            if (initialVelocity.HasValue)
            {
                _velocity = initialVelocity.Value;
                _velocity.z = 0f;
            }
            else
            {
                // Charming initial pop jump
                _velocity = new Vector3(
                    UnityEngine.Random.Range(-1.8f, 1.8f),
                    UnityEngine.Random.Range(3.5f, 5.5f),
                    0f
                );
            }

            EnsureVisuals();
            ApplyBlockMaterial();
        }

        /// <summary>
        /// Static factory method to instantiate a DroppedItem at the given world position.
        /// </summary>
        public static DroppedItem Spawn(Vector3 worldPos, BlockType type, int count = 1)
        {
            if (type == BlockType.Air) return null;

            worldPos.z = 0f;
            GameObject itemObj = new GameObject($"DroppedItem_{type}");
            itemObj.transform.position = worldPos;

            DroppedItem droppedItem = itemObj.AddComponent<DroppedItem>();
            droppedItem.Initialize(type, count);

            return droppedItem;
        }
    }
}
