#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <stdbool.h>
#include <stdio.h>

// ============================================================================
// Luminora 2.5D Localization (i18n) Architecture
// ============================================================================
typedef enum {
    LANG_ZH = 0, // 简体中文 (Default)
    LANG_EN = 1  // English
} Language;

static Language g_CurrentLanguage = LANG_ZH;

static inline Language getLanguage(void) {
    return g_CurrentLanguage;
}

static inline void saveSettings(void);

static inline void setLanguage(Language lang) {
    g_CurrentLanguage = lang;
    saveSettings();
}

static inline void toggleLanguage(void) {
    g_CurrentLanguage = (g_CurrentLanguage == LANG_ZH) ? LANG_EN : LANG_ZH;
    saveSettings();
}

static inline const char* LOC(const char* zh, const char* en) {
    return (g_CurrentLanguage == LANG_ZH) ? zh : en;
}

// ============================================================================
// Settings Persistence (settings.bin)
// ============================================================================
#define SETTINGS_MAGIC 0x53455454 // "SETT"

static inline void saveSettings(void) {
    FILE* f = fopen("settings.bin", "wb");
    if (!f) return;
    unsigned int magic = SETTINGS_MAGIC;
    int lang = (int)g_CurrentLanguage;
    fwrite(&magic, sizeof(unsigned int), 1, f);
    fwrite(&lang, sizeof(int), 1, f);
    fclose(f);
}

static inline void loadSettings(void) {
    FILE* f = fopen("settings.bin", "rb");
    if (!f) return;
    unsigned int magic = 0;
    int lang = 0;
    if (fread(&magic, sizeof(unsigned int), 1, f) == 1 && magic == SETTINGS_MAGIC) {
        if (fread(&lang, sizeof(int), 1, f) == 1) {
            if (lang == LANG_ZH || lang == LANG_EN) {
                g_CurrentLanguage = (Language)lang;
            }
        }
    }
    fclose(f);
}

#endif // LOCALIZATION_H
