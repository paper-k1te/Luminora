using System;
using UnityEngine;

namespace Core
{
    /// <summary>
    /// Supported languages in the game.
    /// </summary>
    public enum GameLanguage
    {
        Chinese = 0,
        English = 1
    }

    /// <summary>
    /// Central manager for game-wide localization and language switching.
    /// Supports Chinese (default) and English with PlayerPrefs persistence and event dispatch.
    /// </summary>
    public static class LocalizationManager
    {
        private const string PrefsKey = "Game_Language";
        private static GameLanguage _currentLanguage = GameLanguage.Chinese;
        private static bool _loaded = false;

        /// <summary>
        /// Triggered whenever the active language changes.
        /// </summary>
        public static event Action OnLanguageChanged;

        static LocalizationManager()
        {
            LoadLanguage();
        }

        private static void LoadLanguage()
        {
            if (_loaded) return;
            // Default to Chinese (0)
            int saved = PlayerPrefs.GetInt(PrefsKey, (int)GameLanguage.Chinese);
            _currentLanguage = (saved == (int)GameLanguage.English) ? GameLanguage.English : GameLanguage.Chinese;
            _loaded = true;
        }

        /// <summary>
        /// Gets or sets the active game language with automatic persistence and notification.
        /// </summary>
        public static GameLanguage CurrentLanguage
        {
            get
            {
                if (!_loaded) LoadLanguage();
                return _currentLanguage;
            }
            set
            {
                SetLanguage(value);
            }
        }

        /// <summary>
        /// Explicitly sets the active game language and notifies subscribers.
        /// </summary>
        public static void SetLanguage(GameLanguage lang)
        {
            if (!_loaded) LoadLanguage();
            if (_currentLanguage == lang && _loaded) return;

            _currentLanguage = lang;
            _loaded = true;
            PlayerPrefs.SetInt(PrefsKey, (int)lang);
            PlayerPrefs.Save();

            try
            {
                OnLanguageChanged?.Invoke();
            }
            catch (Exception ex)
            {
                Debug.LogError($"[LocalizationManager] Exception dispatching OnLanguageChanged: {ex}");
            }
        }

        /// <summary>
        /// Toggles between Chinese and English.
        /// </summary>
        public static void ToggleLanguage()
        {
            SetLanguage(CurrentLanguage == GameLanguage.Chinese ? GameLanguage.English : GameLanguage.Chinese);
        }

        /// <summary>
        /// Returns the Chinese text if active language is Chinese, otherwise returns English text.
        /// </summary>
        public static string Get(string zh, string en)
        {
            return CurrentLanguage == GameLanguage.Chinese ? zh : en;
        }

        /// <summary>
        /// Convenience check for Chinese.
        /// </summary>
        public static bool IsChinese => CurrentLanguage == GameLanguage.Chinese;

        /// <summary>
        /// Convenience check for English.
        /// </summary>
        public static bool IsEnglish => CurrentLanguage == GameLanguage.English;
    }
}
