using System;

namespace Core
{
    /// <summary>
    /// Defines the active gameplay mode for the sandbox world.
    /// Modeled after Minecraft / Terraria gameplay mechanics.
    /// </summary>
    public enum GameMode
    {
        /// <summary>
        /// Survival Mode:
        /// Standard physics, gravity, mining delay based on hardness, finite inventory resources.
        /// </summary>
        Survival = 0,

        /// <summary>
        /// Creative Mode:
        /// God-mode mechanics: double-tap Space flight, no gravity while flying,
        /// instant block breaking (0 delay), and infinite block placement.
        /// </summary>
        Creative = 1
    }

    /// <summary>
    /// Metadata attribute providing descriptive properties for game modes.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class GameModeInfoAttribute : Attribute
    {
        public string EnglishName { get; private set; }
        public string ChineseName { get; private set; }
        public string Description { get; private set; }
        public bool AllowsFlight { get; private set; }
        public bool InstantMining { get; private set; }
        public bool InfinitePlacement { get; private set; }

        public GameModeInfoAttribute(
            string englishName,
            string chineseName,
            string description,
            bool allowsFlight,
            bool instantMining,
            bool infinitePlacement)
        {
            EnglishName = englishName;
            ChineseName = chineseName;
            Description = description;
            AllowsFlight = allowsFlight;
            InstantMining = instantMining;
            InfinitePlacement = infinitePlacement;
        }
    }

    /// <summary>
    /// Extension methods and helpers for GameMode queries.
    /// </summary>
    public static class GameModeExtensions
    {
        public static string GetChineseName(this GameMode mode)
        {
            switch (mode)
            {
                case GameMode.Creative:
                    return "创造模式";
                case GameMode.Survival:
                default:
                    return "生存模式";
            }
        }

        public static string GetEnglishName(this GameMode mode)
        {
            switch (mode)
            {
                case GameMode.Creative:
                    return "Creative Mode";
                case GameMode.Survival:
                default:
                    return "Survival Mode";
            }
        }

        public static string GetDisplayName(this GameMode mode)
        {
            switch (mode)
            {
                case GameMode.Creative:
                    return LocalizationManager.Get("创造模式", "Creative Mode");
                case GameMode.Survival:
                default:
                    return LocalizationManager.Get("生存模式", "Survival Mode");
            }
        }

        public static bool CanFly(this GameMode mode)
        {
            return mode == GameMode.Creative;
        }

        public static bool HasInstantBreak(this GameMode mode)
        {
            return mode == GameMode.Creative;
        }

        public static bool HasInfinitePlacement(this GameMode mode)
        {
            return mode == GameMode.Creative;
        }

        public static string GetBadgeText(this GameMode mode, bool isFlying = false)
        {
            if (mode == GameMode.Creative)
            {
                return isFlying
                    ? LocalizationManager.Get("创造模式 [⚡ 飞行中]", "CREATIVE [⚡ FLYING]")
                    : LocalizationManager.Get("创造模式 [双击空格飞行]", "CREATIVE [FLY: 2x SPACE]");
            }
            return LocalizationManager.Get("生存模式", "SURVIVAL");
        }
    }
}
