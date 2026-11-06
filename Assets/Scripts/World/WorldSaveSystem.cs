using System;
using System.Collections.Generic;
using System.IO;
using Core;
using Player;
using UnityEngine;

namespace World
{
    /// <summary>
    /// Save data container for serialized inventory slots (legacy compatibility).
    /// </summary>
    [Serializable]
    public struct SavedInventorySlot
    {
        public byte type;
        public int count;
    }

    /// <summary>
    /// Save data container for an individual modified voxel block in the world (legacy compatibility).
    /// </summary>
    [Serializable]
    public struct SavedBlockModification
    {
        public int x;
        public int y;
        public byte type;
    }

    /// <summary>
    /// Legacy save data container.
    /// </summary>
    [Serializable]
    public class WorldSaveData
    {
        public float playerX;
        public float playerY;
        public float playerZ;
        public int activeSlot;
        public SavedInventorySlot[] inventory;
        public List<SavedBlockModification> modifiedBlocks = new List<SavedBlockModification>();
        public List<SavedBlockModification> modifiedWalls = new List<SavedBlockModification>();
        public int tutorialStep;
        public int tutorialMinedCount;
        public string saveTimestamp;
        public int worldSeed;
    }

    /// <summary>
    /// Static persistence adapter linking legacy and UI calls to Core.SaveManager.Instance.
    /// Ensures 100% architectural cohesion and backwards compatibility with all subsystems.
    /// </summary>
    public static class WorldSaveSystem
    {
        public static string SaveFilePath => SaveManager.Instance.GetSaveFilePath("savegame.json");

        public static bool HasSaveFile => SaveManager.Instance.HasSaveFile("savegame.json");

        public static long CoordKey(int x, int y)
        {
            return ((long)x << 32) | (uint)y;
        }

        public static void RegisterBlockChange(int x, int y, BlockType type)
        {
            SaveManager.Instance.RecordBlockModification(x, y, type, isWall: false);
        }

        public static void RegisterWallChange(int x, int y, BlockType type)
        {
            SaveManager.Instance.RecordBlockModification(x, y, type, isWall: true);
        }

        public static void ApplyModificationsToChunk(Chunk chunk)
        {
            SaveManager.Instance.ApplyModificationsToChunk(chunk);
        }

        public static bool Save(Vector3 playerPos, InventorySystem inventory, int tutorialStep, int tutorialMinedCount = 0)
        {
            return SaveManager.Instance.SaveGame("savegame.json");
        }

        public static bool Load(
            out Vector3 playerPos,
            InventorySystem inventory,
            out int tutorialStep,
            out int tutorialMinedCount)
        {
            playerPos = Vector3.zero;
            tutorialStep = 0;
            tutorialMinedCount = 0;

            bool success = SaveManager.Instance.LoadGame("savegame.json");
            if (success)
            {
                PlayerController2D5 player = UnityEngine.Object.FindObjectOfType<PlayerController2D5>();
                if (player != null)
                {
                    playerPos = player.Position;
                }

                if (UI.UIManager.Instance != null)
                {
                    tutorialStep = UI.UIManager.Instance.TutorialStep;
                    tutorialMinedCount = UI.UIManager.Instance.MinedCount;
                }
            }

            return success;
        }
    }
}
