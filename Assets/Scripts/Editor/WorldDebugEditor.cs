#if UNITY_EDITOR
using Core;
using Generation;
using Player;
using UnityEditor;
using UnityEngine;
using World;

namespace Editor
{
    /// <summary>
    /// Custom editor and utility shortcuts for the 2.5D sandbox game.
    /// Provides quick actions to regenerate world, jump between biomes,
    /// toggle debug overlays, and inspect world settings in the Unity Editor.
    /// </summary>
    [CustomEditor(typeof(WorldManager))]
    public class WorldDebugEditor : UnityEditor.Editor
    {
        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            WorldManager manager = (WorldManager)target;

            EditorGUILayout.Space(12);
            EditorGUILayout.LabelField("World Debug Utilities", EditorStyles.boldLabel);

            if (GUILayout.Button("Regenerate World (New Random Seed)", GUILayout.Height(30)))
            {
                if (Application.isPlaying)
                {
                    int newSeed = Random.Range(1000, 99999);
                    manager.RegenerateWorld(newSeed);
                }
                else
                {
                    Debug.LogWarning("World regeneration with active mesh colliders is active during Play Mode.");
                }
            }

            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Button("Teleport to Forest (X = 0)"))
            {
                TeleportPlayerToBiome(manager, 0f);
            }
            if (GUILayout.Button("Teleport to Desert (X = -250)"))
            {
                TeleportPlayerToBiome(manager, -250f);
            }
            if (GUILayout.Button("Teleport to Tundra (X = 250)"))
            {
                TeleportPlayerToBiome(manager, 250f);
            }
            EditorGUILayout.EndHorizontal();

            if (Application.isPlaying && manager.PlayerTransform != null)
            {
                EditorGUILayout.Space(8);
                Vector3 playerPos = manager.PlayerTransform.position;
                BiomeType biome = manager.GetBiomeAt(playerPos.x);
                EditorGUILayout.HelpBox($"Player X: {playerPos.x:F1}, Y: {playerPos.y:F1}\nCurrent Biome: {biome}", MessageType.Info);
            }
        }

        private static void TeleportPlayerToBiome(WorldManager manager, float targetX)
        {
            if (!Application.isPlaying) return;

            PlayerController2D5 player = FindObjectOfType<PlayerController2D5>();
            if (player != null && manager != null)
            {
                Vector3 spawnPos = manager.FindSurfaceSpawnPosition(targetX);
                player.Teleport(spawnPos);
            }
        }

        [MenuItem("Luminora 2.5D/Regenerate World")]
        private static void MenuRegenerateWorld()
        {
            if (Application.isPlaying && WorldManager.Instance != null)
            {
                WorldManager.Instance.RegenerateWorld(Random.Range(1000, 99999));
            }
        }
    }
}
#endif
