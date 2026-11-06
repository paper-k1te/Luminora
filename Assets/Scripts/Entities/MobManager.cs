using System.Collections.Generic;
using Core;
using Rendering;
using World;
using UnityEngine;

namespace Entities
{
    /// <summary>
    /// Master coordinator for autonomous mob spawning and entity lifecycle management.
    /// Spawns and maintains ~10-16 mobs within chunk distance around the player,
    /// respecting biome varieties, depth zones (Surface, Cavern, Underworld), and time-of-day.
    /// </summary>
    [DisallowMultipleComponent]
    [DefaultExecutionOrder(-20)]
    public class MobManager : MonoBehaviour
    {
        public static MobManager Instance { get; private set; }

        [Header("Capacity & Streaming")]
        [SerializeField] private int targetMobCount = 14;
        [SerializeField] private float minSpawnDistance = 16.0f;
        [SerializeField] private float maxSpawnDistance = 34.0f;
        [SerializeField] private float despawnDistance = 50.0f;
        [SerializeField] private float spawnCheckInterval = 1.4f;

        [Header("Container & References")]
        [SerializeField] private Transform mobContainer;
        [SerializeField] private Transform playerTransform;

        // Active tracked mobs
        private readonly List<MobEntity> _activeMobs = new List<MobEntity>();
        private float _spawnTimer = 0f;

        public IReadOnlyList<MobEntity> ActiveMobs => _activeMobs;
        public int MobCount => _activeMobs.Count;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            if (mobContainer == null)
            {
                GameObject containerObj = new GameObject("MobsContainer");
                containerObj.transform.SetParent(transform);
                mobContainer = containerObj.transform;
            }

            MobEntity.OnMobDied += HandleMobDied;
        }

        private void OnDestroy()
        {
            if (Instance == this)
            {
                Instance = null;
            }
            MobEntity.OnMobDied -= HandleMobDied;
        }

        private void Start()
        {
            FindPlayer();
        }

        private void Update()
        {
            FindPlayer();

            float dt = Time.deltaTime;
            _spawnTimer += dt;

            // Clean up any destroyed or null references
            _activeMobs.RemoveAll(m => m == null);

            // Periodic spawn and despawn evaluation
            if (_spawnTimer >= spawnCheckInterval)
            {
                _spawnTimer = 0f;
                PerformDespawnCheck();
                PerformSpawnEvaluation();
            }
        }

        private void FindPlayer()
        {
            if (playerTransform == null)
            {
                if (WorldManager.Instance != null && WorldManager.Instance.PlayerTransform != null)
                {
                    playerTransform = WorldManager.Instance.PlayerTransform;
                }
                else
                {
                    GameObject p = GameObject.FindWithTag("Player");
                    if (p != null) playerTransform = p.transform;
                }
            }
        }

        // ====================================================================
        // Despawning & Spawning Logic
        // ====================================================================

        private void PerformDespawnCheck()
        {
            if (playerTransform == null) return;
            Vector3 playerPos = playerTransform.position;

            for (int i = _activeMobs.Count - 1; i >= 0; i--)
            {
                MobEntity mob = _activeMobs[i];
                if (mob == null)
                {
                    _activeMobs.RemoveAt(i);
                    continue;
                }

                float dist = Vector3.Distance(mob.Position, playerPos);
                if (dist > despawnDistance || mob.Position.y < 1f || mob.Position.y >= WorldSettings.ChunkHeight - 2)
                {
                    _activeMobs.RemoveAt(i);
                    Destroy(mob.gameObject);
                }
            }
        }

        private void PerformSpawnEvaluation()
        {
            if (playerTransform == null) return;
            if (_activeMobs.Count >= targetMobCount) return;

            Vector3 playerPos = playerTransform.position;

            // Pick horizontal candidate offset
            float sign = UnityEngine.Random.value < 0.5f ? -1f : 1f;
            float offsetX = sign * UnityEngine.Random.Range(minSpawnDistance, maxSpawnDistance);
            float candidateX = playerPos.x + offsetX;

            int worldX = Mathf.FloorToInt(candidateX);

            // Determine depth zone
            float playerY = playerPos.y;
            MobType chosenType = MobType.Slime;
            Vector3 spawnPos = Vector3.zero;
            bool foundLocation = false;

            // Zone 1: Cavern & Deep Underworld
            if (playerY < WorldSettings.CavernLevel)
            {
                if (playerY < WorldSettings.UnderworldLevel)
                {
                    // Underworld: Tough Zombies and Underworld Bats
                    chosenType = UnityEngine.Random.value < 0.55f ? MobType.Zombie : MobType.Bat;
                }
                else
                {
                    // Deep Cavern: Cave Bats (~60%) and Zombies (~40%)
                    chosenType = UnityEngine.Random.value < 0.60f ? MobType.Bat : MobType.Zombie;
                }

                int sampleY = Mathf.FloorToInt(playerY + UnityEngine.Random.Range(-7, 8));
                sampleY = Mathf.Clamp(sampleY, 4, WorldSettings.CavernLevel - 2);

                if (chosenType == MobType.Bat)
                {
                    // Bats need open air
                    if (IsAirVoxel(worldX, sampleY) && IsAirVoxel(worldX, sampleY + 1))
                    {
                        spawnPos = new Vector3(worldX + 0.5f, sampleY + 0.5f, 0f);
                        foundLocation = true;
                    }
                }
                else
                {
                    // Ground mobs need solid floor
                    for (int checkY = sampleY; checkY >= 3; checkY--)
                    {
                        if (IsSolidVoxel(worldX, checkY) && IsAirVoxel(worldX, checkY + 1) && IsAirVoxel(worldX, checkY + 2))
                        {
                            spawnPos = new Vector3(worldX + 0.5f, checkY + 1.05f, 0f);
                            foundLocation = true;
                            break;
                        }
                    }
                }
            }
            // Zone 2: Surface & Subsurface
            else
            {
                bool isNight = false;
                if (DayNightCycle.Instance != null)
                {
                    isNight = DayNightCycle.Instance.IsNight;
                }

                if (isNight)
                {
                    // Night surface: aggressive Zombies and nocturnal Slimes
                    chosenType = UnityEngine.Random.value < 0.65f ? MobType.Zombie : MobType.Slime;
                }
                else
                {
                    // Verdant Forest / Grasslands: lively Green Slimes
                    chosenType = MobType.Slime;
                }

                // Locate surface elevation
                int surfaceY = FindSurfaceHeight(worldX);
                if (surfaceY > 3 && surfaceY < WorldSettings.ChunkHeight - 4)
                {
                    spawnPos = new Vector3(worldX + 0.5f, surfaceY + 1.05f, 0f);
                    foundLocation = true;
                }
            }

            if (foundLocation)
            {
                SpawnMob(chosenType, spawnPos);
            }
        }

        private bool IsAirVoxel(int wx, int wy)
        {
            if (WorldManager.Instance == null) return true;
            BlockType bt = WorldManager.Instance.GetBlock(wx, wy);
            return bt == BlockType.Air;
        }

        private bool IsSolidVoxel(int wx, int wy)
        {
            if (WorldManager.Instance == null) return false;
            BlockType bt = WorldManager.Instance.GetBlock(wx, wy);
            if (bt == BlockType.Air) return false;
            BlockData data = BlockRegistry.GetBlock(bt);
            return data.isSolid;
        }

        private int FindSurfaceHeight(int wx)
        {
            if (WorldManager.Instance != null && WorldManager.Instance.TerrainGen != null)
            {
                return WorldManager.Instance.TerrainGen.GetSurfaceHeight(wx);
            }

            // Raycast fallback
            RaycastHit hit;
            if (Physics.Raycast(new Vector3(wx + 0.5f, WorldSettings.ChunkHeight, 0f), Vector3.down, out hit, WorldSettings.ChunkHeight))
            {
                return Mathf.FloorToInt(hit.point.y);
            }

            return WorldSettings.BaseSurfaceLevel;
        }

        // ====================================================================
        // Public API
        // ====================================================================

        /// <summary>
        /// Instantiates and initializes a 2.5D mob creature at the specified world coordinates.
        /// </summary>
        public MobEntity SpawnMob(MobType type, Vector3 worldPosition)
        {
            worldPosition.z = 0f;

            GameObject mobObj = new GameObject($"Mob_{type}_{_activeMobs.Count + 1}");
            mobObj.transform.position = worldPosition;
            if (mobContainer != null)
            {
                mobObj.transform.SetParent(mobContainer, true);
            }

            MobEntity mob = mobObj.AddComponent<MobEntity>();
            mob.Configure(type);

            _activeMobs.Add(mob);
            return mob;
        }

        /// <summary>
        /// Checks if a player interaction click falls within attack range of an active mob.
        /// If hit, deals damage with knockback and returns true.
        /// </summary>
        public bool CheckAttackAt(Vector3 clickPoint, float radius, int damage, Vector3 attackerPos)
        {
            clickPoint.z = 0f;
            attackerPos.z = 0f;

            MobEntity closestMob = null;
            float closestDist = float.MaxValue;

            for (int i = 0; i < _activeMobs.Count; i++)
            {
                MobEntity mob = _activeMobs[i];
                if (mob == null || mob.IsDead) continue;

                Vector3 mobCenter = mob.Position + Vector3.up * 0.5f;
                mobCenter.z = 0f;

                float dist = Vector3.Distance(clickPoint, mobCenter);
                if (dist <= radius && dist < closestDist)
                {
                    closestDist = dist;
                    closestMob = mob;
                }
            }

            if (closestMob != null)
            {
                closestMob.TakeDamage(damage, attackerPos);
                return true;
            }

            return false;
        }

        private void HandleMobDied(MobEntity mob)
        {
            _activeMobs.Remove(mob);
        }
    }
}
