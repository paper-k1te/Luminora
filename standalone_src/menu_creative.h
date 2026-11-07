#ifndef MENU_CREATIVE_H
#define MENU_CREATIVE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "font_render.h"
#include "sound_engine.h"
#include "localization.h"

// ============================================================================
// State Enums
// ============================================================================
typedef enum {
    MODE_SURVIVAL = 0,
    MODE_CREATIVE = 1
} GameMode;

#include "save_system.h"

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING = 1,
    STATE_PAUSED = 2,
    STATE_SETTINGS = 3,
    STATE_WORLDS_LIST = 4,
    STATE_CREATE_WORLD = 5
} GameState;

// Forward references to main.c
static void startNewWorld(int slotIdx, const char* name, int mode, int seed);
static bool loadWorldSlot(int slotIdx);
static bool saveWorldSlot(int slotIdx);
static void cycleResolutionPreset(HWND hwnd);
static void toggleFullscreen(HWND hwnd);
static int getResPresetIndex(void);
static bool isFullscreen(void);
extern HWND g_MainHwnd;

static int g_SelectedSaveSlot = 0;
static int g_CreateWorldMode = 0; // 0: Survival, 1: Creative
static int g_CreateWorldNameIdx = 0;
static int g_CreateWorldSeed = 1337;

static const char* g_WorldNamePresets_ZH[] = {
    "冒险新大陆", "像素王国", "无尽荒野", "探索者之境", "星光圣境", "地下探秘"
};
static const char* g_WorldNamePresets_EN[] = {
    "New Adventure", "Pixel Kingdom", "Wilderness", "Explorer Base", "Starlight Realm", "Underworld Cave"
};
#define PRESET_NAMES_COUNT 6

// ============================================================================
// Button UI Structure
// ============================================================================
typedef struct {
    float x, y, w, h;
    const char* label;
    bool hovered;
    bool active;
} MenuButton;

// Forward references to main.c static globals and helpers
static bool saveGame(void);
static bool loadGame(void);
static void showNotification(const char* text, float r, float g, float b);
static bool isSolidBlock(int x, int y);

// ============================================================================
// Internal Menu, Pause & Settings State
// ============================================================================
static GameMode  g_CurrentMode = MODE_SURVIVAL;
static GameState g_CurrentState = STATE_MENU;
static GameState g_PreviousState = STATE_MENU;
static bool      g_IsCreativeFlying = false;

// Double-tap Space tracking for Creative Flight
static DWORD     g_LastSpacePressTick = 0;
static const float FLIGHT_TAP_WINDOW = 0.28f;

// Mouse coordinates for menu hover & clicks
static int g_MenuMouseX = 0;
static int g_MenuMouseY = 0;

// Main Menu Buttons (5 options)
#define MAIN_MENU_BTN_COUNT 5
static MenuButton g_MainMenuButtons[MAIN_MENU_BTN_COUNT] = {
    { 0, 0, 380.0f, 46.0f, "", false, false }, // 0: Survival Mode
    { 0, 0, 380.0f, 46.0f, "", false, false }, // 1: Creative Mode
    { 0, 0, 380.0f, 46.0f, "", false, false }, // 2: Load World
    { 0, 0, 380.0f, 46.0f, "", false, false }, // 3: Settings
    { 0, 0, 380.0f, 46.0f, "", false, false }  // 4: Exit Game
};

// Pause Menu Buttons (8 options)
#define PAUSE_MENU_BTN_COUNT 8
static MenuButton g_PauseMenuButtons[PAUSE_MENU_BTN_COUNT] = {
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 0: Resume Game
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 1: Switch Mode
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 2: Direct Language Toggle
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 3: Settings
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 4: Save World
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 5: Load World
    { 0, 0, 360.0f, 38.0f, "", false, false }, // 6: Return to Main Menu
    { 0, 0, 360.0f, 38.0f, "", false, false }  // 7: Exit Game
};

// Settings Modal Buttons (6 options: Language, Sound, Resolution, Font Scale, Game Mode, Done)
#define SETTINGS_BTN_COUNT 6
static MenuButton g_SettingsButtons[SETTINGS_BTN_COUNT] = {
    { 0, 0, 400.0f, 42.0f, "", false, false }, // 0: Language Option
    { 0, 0, 400.0f, 42.0f, "", false, false }, // 1: Sound Option
    { 0, 0, 400.0f, 42.0f, "", false, false }, // 2: Display / Resolution Option
    { 0, 0, 400.0f, 42.0f, "", false, false }, // 3: Font Scale Option
    { 0, 0, 400.0f, 42.0f, "", false, false }, // 4: Mode Option
    { 0, 0, 400.0f, 42.0f, "", false, false }  // 5: Done & Return
};

// ============================================================================
// Public Accessors
// ============================================================================
static inline GameMode getGameMode(void) {
    return g_CurrentMode;
}

static inline void setGameMode(GameMode mode) {
    g_CurrentMode = mode;
    if (g_CurrentMode != MODE_CREATIVE) {
        g_IsCreativeFlying = false;
    }
}

static inline GameState getGameState(void) {
    return g_CurrentState;
}

static inline void setGameState(GameState state) {
    g_CurrentState = state;
}

static inline GameState getPreviousGameState(void) {
    return g_PreviousState;
}

static inline void setPreviousGameState(GameState state) {
    g_PreviousState = state;
}

static inline bool isCreativeFlying(void) {
    return (g_CurrentMode == MODE_CREATIVE && g_IsCreativeFlying);
}

static inline bool isCreativeInstantBreak(void) {
    return (g_CurrentMode == MODE_CREATIVE);
}

static inline bool isCreativeInfinitePlace(void) {
    return (g_CurrentMode == MODE_CREATIVE);
}

// ============================================================================
// 2D Drawing Primitives
// ============================================================================
static void menuDrawQuad(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void menuDrawBorder(float x, float y, float w, float h, float r, float g, float b, float width) {
    glColor3f(r, g, b);
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}

// Minecraft-style 3D bevel button renderer with hover highlight and crisp pixel borders
static void menuDrawMinecraftButton(MenuButton* btn) {
    float x = btn->x;
    float y = btn->y;
    float w = btn->w;
    float h = btn->h;
    bool hovered = btn->hovered;
    bool active = btn->active;

    // 1. Black outer shadow border (2px)
    menuDrawQuad(x, y, w, h, 0.05f, 0.05f, 0.07f, 1.0f);

    // 2. Bevel highlights & shadow
    float b = 2.5f;
    if (active) {
        // Pressed bevel: dark top-left, lighter bottom-right
        menuDrawQuad(x + b, y + b, w - b * 2, h - b * 2, 0.22f, 0.22f, 0.26f, 1.0f);
        menuDrawQuad(x + b, y + b, w - b * 2, b, 0.12f, 0.12f, 0.15f, 1.0f);
        menuDrawQuad(x + b, y + b, b, h - b * 2, 0.12f, 0.12f, 0.15f, 1.0f);
        menuDrawQuad(x + b, y + h - b * 2, w - b * 2, b, 0.40f, 0.40f, 0.45f, 1.0f);
        menuDrawQuad(x + w - b * 2, y + b, b, h - b * 2, 0.40f, 0.40f, 0.45f, 1.0f);
    } else if (hovered) {
        // Hovered button: glowing stone with golden-white highlight bevel
        menuDrawQuad(x + b, y + b, w - b * 2, h - b * 2, 0.44f, 0.45f, 0.52f, 1.0f);
        // Top & left bevel (bright gold-white)
        menuDrawQuad(x + b, y + b, w - b * 2, b, 1.0f, 0.90f, 0.45f, 1.0f);
        menuDrawQuad(x + b, y + b, b, h - b * 2, 1.0f, 0.90f, 0.45f, 1.0f);
        // Bottom & right bevel (dark slate)
        menuDrawQuad(x + b, y + h - b * 2, w - b * 2, b, 0.24f, 0.24f, 0.28f, 1.0f);
        menuDrawQuad(x + w - b * 2, y + b, b, h - b * 2, 0.24f, 0.24f, 0.28f, 1.0f);
    } else {
        // Standard Minecraft stone button bevel
        menuDrawQuad(x + b, y + b, w - b * 2, h - b * 2, 0.35f, 0.35f, 0.40f, 1.0f);
        // Top & left bevel (light gray stone)
        menuDrawQuad(x + b, y + b, w - b * 2, b, 0.60f, 0.60f, 0.66f, 1.0f);
        menuDrawQuad(x + b, y + b, b, h - b * 2, 0.60f, 0.60f, 0.66f, 1.0f);
        // Bottom & right bevel (dark shadow)
        menuDrawQuad(x + b, y + h - b * 2, w - b * 2, b, 0.18f, 0.18f, 0.22f, 1.0f);
        menuDrawQuad(x + w - b * 2, y + b, b, h - b * 2, 0.18f, 0.18f, 0.22f, 1.0f);
    }

    // 3. Centered button label with drop shadow
    float textPixelW = getTextWidthUTF8(btn->label, FONT_SIZE_BODY, true);
    float textX = x + (w - textPixelW) * 0.5f;
    float textY = y + (h * 0.5f) + 6.0f;

    // Drop shadow
    drawTextUTF8(textX + 1.5f, textY + 1.5f, btn->label, 0.10f, 0.10f, 0.12f, FONT_SIZE_BODY, true);

    // Front text (brightened on hover/active without glyph scale jumping)
    if (hovered) {
        drawTextUTF8(textX, textY, btn->label, 1.0f, 1.0f, 0.60f, FONT_SIZE_BODY, true);
    } else if (active) {
        drawTextUTF8(textX, textY, btn->label, 1.0f, 0.85f, 0.25f, FONT_SIZE_BODY, true);
    } else {
        drawTextUTF8(textX, textY, btn->label, 0.95f, 0.95f, 0.95f, FONT_SIZE_BODY, true);
    }
}

// Minecraft tiled dirt background pattern
static void menuDrawMinecraftDirtBackground(float width, float height) {
    float tileSize = 32.0f;
    int cols = (int)ceilf(width / tileSize);
    int rows = (int)ceilf(height / tileSize);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float px = c * tileSize;
            float py = r * tileSize;
            bool alt = ((c + r) % 2 == 0);
            if (alt) {
                menuDrawQuad(px, py, tileSize, tileSize, 0.24f, 0.16f, 0.10f, 1.0f);
            } else {
                menuDrawQuad(px, py, tileSize, tileSize, 0.30f, 0.20f, 0.13f, 1.0f);
            }
            menuDrawBorder(px, py, tileSize, tileSize, 0.18f, 0.12f, 0.07f, 1.0f);
        }
    }

    // Dark vignette overlay
    menuDrawQuad(0, 0, width, height, 0.05f, 0.06f, 0.08f, 0.76f);
}

// ============================================================================
// Initialization
// ============================================================================
static void initMenuAndCreative(void) {
    g_CurrentMode = MODE_SURVIVAL;
    g_CurrentState = STATE_MENU;
    g_PreviousState = STATE_MENU;
    g_IsCreativeFlying = false;
    g_LastSpacePressTick = 0;
}

// ============================================================================
// Settings Modal Dialog Handling & Rendering
// ============================================================================
static bool handleSettingsInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            for (int i = 0; i < SETTINGS_BTN_COUNT; i++) {
                MenuButton* btn = &g_SettingsButtons[i];
                btn->hovered = (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                                g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h);
            }
            return true;
        }

        case WM_LBUTTONDOWN: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            for (int i = 0; i < SETTINGS_BTN_COUNT; i++) {
                MenuButton* btn = &g_SettingsButtons[i];
                if (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                    g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h) {
                    btn->active = true;
                    playSound(SFX_CLICK);

                    if (i == 0) {
                        // Option 1: Toggle Language
                        toggleLanguage();
                        showNotification(LOC("语言已设置为: 简体中文", "Language set to: English"), 1.0f, 0.85f, 0.3f);
                    } else if (i == 1) {
                        // Option 2: Toggle Sound
                        toggleSound();
                        showNotification(isSoundEnabled() ? LOC("游戏音效: 开启", "Sound Effects: Enabled") : LOC("游戏音效: 静音", "Sound Effects: Muted"), 0.4f, 0.9f, 1.0f);
                    } else if (i == 2) {
                        // Option 3: Cycle Resolution Preset
                        cycleResolutionPreset(g_MainHwnd);
                        char resNotif[128];
                        if (isFullscreen()) {
                            snprintf(resNotif, sizeof(resNotif), "%s: %s", LOC("已切换至", "Switched to"), LOC("无边框全屏模式", "Borderless Fullscreen"));
                        } else {
                            snprintf(resNotif, sizeof(resNotif), "%s: %dx%d", LOC("已切换分辨率", "Resolution changed"), (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT);
                        }
                        showNotification(resNotif, 0.4f, 0.9f, 1.0f);
                    } else if (i == 3) {
                        // Option 4: Cycle Font Scale
                        cycleFontScale();
                        char fontNotif[128];
                        snprintf(fontNotif, sizeof(fontNotif), "%s: %s", LOC("字体缩放已设为", "Font Scale set to"), getFontScaleName());
                        showNotification(fontNotif, 1.0f, 0.9f, 0.4f);
                    } else if (i == 4) {
                        // Option 5: Toggle Mode
                        if (g_CurrentMode == MODE_SURVIVAL) {
                            setGameMode(MODE_CREATIVE);
                            showNotification(LOC("已切换至: 创造模式 [双击空格飞行]", "Switched to: Creative Mode [2x Space to Fly]"), 0.35f, 0.90f, 1.0f);
                        } else {
                            setGameMode(MODE_SURVIVAL);
                            showNotification(LOC("已切换至: 生存模式", "Switched to: Survival Mode"), 0.35f, 0.95f, 0.45f);
                        }
                    } else if (i == 5) {
                        // Option 6: Done & Return
                        setGameState(g_PreviousState);
                    }
                    return true;
                }
            }
            return true;
        }

        case WM_LBUTTONUP: {
            for (int i = 0; i < SETTINGS_BTN_COUNT; i++) {
                g_SettingsButtons[i].active = false;
            }
            return true;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                setGameState(g_PreviousState);
                return true;
            }
            return true;
        }

        case WM_KEYUP: {
            return true;
        }
    }
    return false;
}

static void renderSettingsDialog(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Background backdrop
    if (g_PreviousState == STATE_MENU) {
        menuDrawMinecraftDirtBackground((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    } else {
        menuDrawQuad(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, 0.0f, 0.0f, 0.72f);
    }

    // 2. Settings Stone Panel
    float boxW = 480.0f;
    float boxH = 460.0f;
    float boxX = ((float)WINDOW_WIDTH - boxW) * 0.5f;
    float boxY = ((float)WINDOW_HEIGHT - boxH) * 0.5f;

    // Outer stone panel & borders
    menuDrawQuad(boxX, boxY, boxW, boxH, 0.12f, 0.14f, 0.18f, 0.96f);
    menuDrawBorder(boxX, boxY, boxW, boxH, 0.35f, 0.45f, 0.58f, 2.0f);
    menuDrawBorder(boxX + 3.0f, boxY + 3.0f, boxW - 6.0f, boxH - 6.0f, 1.0f, 0.82f, 0.20f, 1.5f);

    // Header Title
    const char* titleText = LOC("游戏设置", "Settings");
    float tW = getTextWidth(titleText, true);
    drawText(boxX + (boxW - tW) * 0.5f, boxY + 32.0f, titleText, 1.0f, 0.85f, 0.25f, true);

    // Subtitle
    const char* subText = LOC("配置语言、音效、分辨率、字体与模式偏好", "Configure Language, Audio, Display, Font & Game Mode");
    float sW = getTextWidth(subText, false);
    drawText(boxX + (boxW - sW) * 0.5f, boxY + 54.0f, subText, 0.75f, 0.85f, 0.95f, false);

    // 3. Settings Buttons
    float btnW = 400.0f;
    float btnH = 42.0f;
    float btnGap = 10.0f;
    float btnX = boxX + (boxW - btnW) * 0.5f;
    float btnStartY = boxY + 76.0f;

    g_SettingsButtons[0].label = LOC("语言: 简体中文 [点击切换]", "Language: English [Click to Switch]");
    g_SettingsButtons[1].label = isSoundEnabled() ? LOC("游戏音效: 开启", "Sound Effects: Enabled") : LOC("游戏音效: 静音", "Sound Effects: Muted");
    
    char resLabel[128];
    if (isFullscreen()) {
        snprintf(resLabel, sizeof(resLabel), "%s: %s [F11 %s]",
            LOC("显示模式", "Display"), LOC("无边框全屏", "Borderless Fullscreen"), LOC("窗口", "Window"));
    } else {
        snprintf(resLabel, sizeof(resLabel), "%s: %dx%d [%s/F11全屏]",
            LOC("分辨率", "Resolution"), (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT, LOC("点击切换", "Click"));
    }
    g_SettingsButtons[2].label = resLabel;

    char fontLabel[128];
    snprintf(fontLabel, sizeof(fontLabel), "%s: %s [%s]",
        LOC("字体大小", "Font Size"), getFontScaleName(), LOC("点击切换", "Click"));
    g_SettingsButtons[3].label = fontLabel;

    g_SettingsButtons[4].label = (g_CurrentMode == MODE_SURVIVAL) ? LOC("当前模式: 生存模式", "Game Mode: Survival") : LOC("当前模式: 创造模式", "Game Mode: Creative");
    g_SettingsButtons[5].label = LOC("完成并返回", "Done & Return");

    for (int i = 0; i < SETTINGS_BTN_COUNT; i++) {
        g_SettingsButtons[i].x = btnX;
        g_SettingsButtons[i].y = btnStartY + i * (btnH + btnGap);
        g_SettingsButtons[i].w = btnW;
        g_SettingsButtons[i].h = btnH;
        menuDrawMinecraftButton(&g_SettingsButtons[i]);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


// ============================================================================
// Multi-Save Worlds List Screen (STATE_WORLDS_LIST)
// ============================================================================
static bool handleWorldsListInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    float panelW = 720.0f;
    float panelH = 550.0f;
    float panelX = ((float)WINDOW_WIDTH - panelW) * 0.5f;
    float panelY = ((float)WINDOW_HEIGHT - panelH) * 0.5f;

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);
            return true;
        }

        case WM_LBUTTONDOWN: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            // 1. Check each of the 4 world slots
            for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                float slotY = panelY + 68.0f + i * 94.0f;
                float slotW = panelW - 60.0f;
                float slotX = panelX + 30.0f;
                
                WorldSlotInfo info;
                bool exists = getSlotInfo(i, &info);

                if (exists) {
                    // Play Button [进入]
                    float playX = slotX + slotW - 170.0f;
                    float playY = slotY + 20.0f;
                    float playW = 75.0f;
                    float playH = 42.0f;
                    if (g_MenuMouseX >= playX && g_MenuMouseX <= playX + playW &&
                        g_MenuMouseY >= playY && g_MenuMouseY <= playY + playH) {
                        playSound(SFX_CLICK);
                        g_ActiveSaveSlot = i;
                        if (loadWorldSlot(i)) {
                            setGameState(STATE_PLAYING);
                        }
                        return true;
                    }

                    // Delete Button [删除]
                    float delX = slotX + slotW - 85.0f;
                    float delY = slotY + 20.0f;
                    float delW = 70.0f;
                    float delH = 42.0f;
                    if (g_MenuMouseX >= delX && g_MenuMouseX <= delX + delW &&
                        g_MenuMouseY >= delY && g_MenuMouseY <= delY + delH) {
                        playSound(SFX_BREAK);
                        deleteWorldSlot(i);
                        showNotification(LOC("世界存档已删除", "World save deleted"), 1.0f, 0.4f, 0.4f);
                        return true;
                    }
                } else {
                    // Create Button [+ 新建世界]
                    float createX = slotX + slotW - 165.0f;
                    float createY = slotY + 20.0f;
                    float createW = 150.0f;
                    float createH = 42.0f;
                    if (g_MenuMouseX >= createX && g_MenuMouseX <= createX + createW &&
                        g_MenuMouseY >= createY && g_MenuMouseY <= createY + createH) {
                        playSound(SFX_CLICK);
                        g_SelectedSaveSlot = i;
                        g_CreateWorldSeed = rand() % 90000 + 1000;
                        setGameState(STATE_CREATE_WORLD);
                        return true;
                    }
                }
            }

            // 2. Back Button
            float backW = 240.0f;
            float backH = 40.0f;
            float backX = panelX + (panelW - backW) * 0.5f;
            float backY = panelY + panelH - 52.0f;
            if (g_MenuMouseX >= backX && g_MenuMouseX <= backX + backW &&
                g_MenuMouseY >= backY && g_MenuMouseY <= backY + backH) {
                playSound(SFX_CLICK);
                setGameState(STATE_MENU);
                return true;
            }
            return true;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                setGameState(STATE_MENU);
                return true;
            }
            return true;
        }
    }
    return false;
}

static void renderWorldsListDialog(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dirt background
    menuDrawMinecraftDirtBackground((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    float panelW = 720.0f;
    float panelH = 550.0f;
    float panelX = ((float)WINDOW_WIDTH - panelW) * 0.5f;
    float panelY = ((float)WINDOW_HEIGHT - panelH) * 0.5f;

    // Panel
    menuDrawQuad(panelX, panelY, panelW, panelH, 0.10f, 0.12f, 0.16f, 0.95f);
    menuDrawBorder(panelX, panelY, panelW, panelH, 0.35f, 0.45f, 0.55f, 2.0f);
    menuDrawBorder(panelX + 3.0f, panelY + 3.0f, panelW - 6.0f, panelH - 6.0f, 1.0f, 0.82f, 0.20f, 1.5f);

    // Title
    const char* title = LOC("选择世界存档", "Select World");
    float tw = getTextWidth(title, true);
    drawText(panelX + (panelW - tw) * 0.5f, panelY + 34.0f, title, 1.0f, 0.88f, 0.25f, true);

    const char* sub = LOC("请选择一个存档位进入游戏或创建新世界", "Select a world slot to play or create new world");
    float sw = getTextWidth(sub, false);
    drawText(panelX + (panelW - sw) * 0.5f, panelY + 54.0f, sub, 0.7f, 0.8f, 0.9f, false);

    // Slots
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        float slotY = panelY + 68.0f + i * 94.0f;
        float slotW = panelW - 60.0f;
        float slotX = panelX + 30.0f;
        float slotH = 82.0f;
        WorldSlotInfo info;
        bool exists = getSlotInfo(i, &info);

        if (exists) {
            menuDrawQuad(slotX, slotY, slotW, slotH, 0.16f, 0.19f, 0.26f, 0.95f);
            menuDrawBorder(slotX, slotY, slotW, slotH, 0.45f, 0.55f, 0.70f, 1.5f);

            // World Name & Badge
            char nameBuf[128];
            snprintf(nameBuf, sizeof(nameBuf), "%d. %s", i + 1, info.worldName);
            drawText(slotX + 16.0f, slotY + 28.0f, nameBuf, 1.0f, 0.92f, 0.40f, true);

            const char* modeBadge = (info.mode == 0) ? LOC("【生存模式】", "[Survival]") : LOC("【创造模式】", "[Creative]");
            float mr = (info.mode == 0) ? 1.0f : 0.35f;
            float mg = (info.mode == 0) ? 0.65f : 0.90f;
            float mb = (info.mode == 0) ? 0.25f : 1.0f;
            drawText(slotX + 16.0f + getTextWidth(nameBuf, true) + 12.0f, slotY + 28.0f, modeBadge, mr, mg, mb, true);

            char detailBuf[128];
            snprintf(detailBuf, sizeof(detailBuf), "%s: %d  |  %s: %s", LOC("种子", "Seed"), info.seed, LOC("存档时间", "Saved"), info.dateStr);
            drawText(slotX + 16.0f, slotY + 54.0f, detailBuf, 0.75f, 0.80f, 0.88f, false);

            // Play Button
            float playX = slotX + slotW - 170.0f;
            float playY = slotY + 20.0f;
            float playW = 75.0f;
            float playH = 42.0f;
            bool playHov = (g_MenuMouseX >= playX && g_MenuMouseX <= playX + playW && g_MenuMouseY >= playY && g_MenuMouseY <= playY + playH);
            menuDrawQuad(playX, playY, playW, playH, playHov ? 0.25f : 0.18f, playHov ? 0.55f : 0.38f, playHov ? 0.25f : 0.18f, 1.0f);
            menuDrawBorder(playX, playY, playW, playH, 0.4f, 0.9f, 0.4f, 1.5f);
            float pw = getTextWidth(LOC("进入", "Play"), true);
            drawText(playX + (playW - pw) * 0.5f, playY + 26.0f, LOC("进入", "Play"), 1.0f, 1.0f, 1.0f, true);

            // Delete Button
            float delX = slotX + slotW - 85.0f;
            float delY = slotY + 20.0f;
            float delW = 70.0f;
            float delH = 42.0f;
            bool delHov = (g_MenuMouseX >= delX && g_MenuMouseX <= delX + delW && g_MenuMouseY >= delY && g_MenuMouseY <= delY + delH);
            menuDrawQuad(delX, delY, delW, delH, delHov ? 0.55f : 0.32f, delHov ? 0.18f : 0.14f, delHov ? 0.18f : 0.14f, 1.0f);
            menuDrawBorder(delX, delY, delW, delH, 0.9f, 0.3f, 0.3f, 1.5f);
            float dw = getTextWidth(LOC("删除", "Del"), true);
            drawText(delX + (delW - dw) * 0.5f, delY + 26.0f, LOC("删除", "Del"), 1.0f, 0.85f, 0.85f, true);

        } else {
            menuDrawQuad(slotX, slotY, slotW, slotH, 0.11f, 0.13f, 0.17f, 0.85f);
            menuDrawBorder(slotX, slotY, slotW, slotH, 0.25f, 0.30f, 0.38f, 1.0f);

            char emptyBuf[64];
            snprintf(emptyBuf, sizeof(emptyBuf), LOC("【 空存档位 %d 】", "[ Empty Slot %d ]"), i + 1);
            drawText(slotX + 20.0f, slotY + 32.0f, emptyBuf, 0.65f, 0.70f, 0.78f, true);
            drawText(slotX + 20.0f, slotY + 54.0f, LOC("点击右侧按钮新建世界", "Click button on right to create world"), 0.50f, 0.55f, 0.62f, false);

            // Create Button
            float createX = slotX + slotW - 165.0f;
            float createY = slotY + 20.0f;
            float createW = 150.0f;
            float createH = 42.0f;
            bool creHov = (g_MenuMouseX >= createX && g_MenuMouseX <= createX + createW && g_MenuMouseY >= createY && g_MenuMouseY <= createY + createH);
            menuDrawQuad(createX, createY, createW, createH, creHov ? 0.35f : 0.20f, creHov ? 0.40f : 0.26f, creHov ? 0.55f : 0.38f, 1.0f);
            menuDrawBorder(createX, createY, createW, createH, 0.4f, 0.75f, 1.0f, 1.5f);
            float cw = getTextWidth(LOC("+ 创建新世界", "+ Create World"), true);
            drawText(createX + (createW - cw) * 0.5f, createY + 26.0f, LOC("+ 创建新世界", "+ Create World"), 1.0f, 0.95f, 0.45f, true);
        }
    }

    // Back Button
    float backW = 240.0f;
    float backH = 40.0f;
    float backX = panelX + (panelW - backW) * 0.5f;
    float backY = panelY + panelH - 52.0f;
    bool bHov = (g_MenuMouseX >= backX && g_MenuMouseX <= backX + backW && g_MenuMouseY >= backY && g_MenuMouseY <= backY + backH);
    menuDrawQuad(backX, backY, backW, backH, bHov ? 0.28f : 0.18f, bHov ? 0.30f : 0.20f, bHov ? 0.36f : 0.26f, 1.0f);
    menuDrawBorder(backX, backY, backW, backH, 0.45f, 0.55f, 0.68f, 1.5f);
    float bw = getTextWidth(LOC("返回主菜单", "Back to Menu"), true);
    drawText(backX + (backW - bw) * 0.5f, backY + 25.0f, LOC("返回主菜单", "Back to Menu"), 1.0f, 1.0f, 1.0f, true);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// World Creation Screen with Options (STATE_CREATE_WORLD)
// ============================================================================
static bool handleCreateWorldInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    float boxW = 600.0f;
    float boxH = 450.0f;
    float boxX = ((float)WINDOW_WIDTH - boxW) * 0.5f;
    float boxY = ((float)WINDOW_HEIGHT - boxH) * 0.5f;

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);
            return true;
        }

        case WM_LBUTTONDOWN: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            // Option 1: Cycle Name Preset
            float nameBtnY = boxY + 85.0f;
            float nameBtnW = 480.0f;
            float nameBtnH = 46.0f;
            float nameBtnX = boxX + (boxW - nameBtnW) * 0.5f;
            if (g_MenuMouseX >= nameBtnX && g_MenuMouseX <= nameBtnX + nameBtnW &&
                g_MenuMouseY >= nameBtnY && g_MenuMouseY <= nameBtnY + nameBtnH) {
                playSound(SFX_CLICK);
                g_CreateWorldNameIdx = (g_CreateWorldNameIdx + 1) % PRESET_NAMES_COUNT;
                return true;
            }

            // Option 2: Toggle Game Mode (Survival <-> Creative)
            float modeBtnY = boxY + 165.0f;
            float modeBtnW = 480.0f;
            float modeBtnH = 50.0f;
            float modeBtnX = boxX + (boxW - modeBtnW) * 0.5f;
            if (g_MenuMouseX >= modeBtnX && g_MenuMouseX <= modeBtnX + modeBtnW &&
                g_MenuMouseY >= modeBtnY && g_MenuMouseY <= modeBtnY + modeBtnH) {
                playSound(SFX_CLICK);
                g_CreateWorldMode = 1 - g_CreateWorldMode;
                return true;
            }

            // Option 3: Randomize Seed
            float seedBtnY = boxY + 245.0f;
            float seedBtnW = 480.0f;
            float seedBtnH = 46.0f;
            float seedBtnX = boxX + (boxW - seedBtnW) * 0.5f;
            if (g_MenuMouseX >= seedBtnX && g_MenuMouseX <= seedBtnX + seedBtnW &&
                g_MenuMouseY >= seedBtnY && g_MenuMouseY <= seedBtnY + seedBtnH) {
                playSound(SFX_CLICK);
                g_CreateWorldSeed = rand() % 90000 + 1000;
                return true;
            }

            // Button: Create & Play
            float cBtnW = 240.0f;
            float cBtnH = 46.0f;
            float cBtnX = boxX + 45.0f;
            float cBtnY = boxY + boxH - 70.0f;
            if (g_MenuMouseX >= cBtnX && g_MenuMouseX <= cBtnX + cBtnW &&
                g_MenuMouseY >= cBtnY && g_MenuMouseY <= cBtnY + cBtnH) {
                playSound(SFX_CRAFT);
                const char* chosenName = (getLanguage() == LANG_ZH) ? g_WorldNamePresets_ZH[g_CreateWorldNameIdx] : g_WorldNamePresets_EN[g_CreateWorldNameIdx];
                startNewWorld(g_SelectedSaveSlot, chosenName, g_CreateWorldMode, g_CreateWorldSeed);
                g_ActiveSaveSlot = g_SelectedSaveSlot;
                setGameState(STATE_PLAYING);
                return true;
            }

            // Button: Cancel
            float cancelW = 200.0f;
            float cancelH = 46.0f;
            float cancelX = boxX + boxW - cancelW - 45.0f;
            float cancelY = boxY + boxH - 70.0f;
            if (g_MenuMouseX >= cancelX && g_MenuMouseX <= cancelX + cancelW &&
                g_MenuMouseY >= cancelY && g_MenuMouseY <= cancelY + cancelH) {
                playSound(SFX_CLICK);
                setGameState(STATE_WORLDS_LIST);
                return true;
            }

            return true;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                setGameState(STATE_WORLDS_LIST);
                return true;
            }
            return true;
        }
    }
    return false;
}

static void renderCreateWorldDialog(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    menuDrawMinecraftDirtBackground((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    float boxW = 600.0f;
    float boxH = 450.0f;
    float boxX = ((float)WINDOW_WIDTH - boxW) * 0.5f;
    float boxY = ((float)WINDOW_HEIGHT - boxH) * 0.5f;

    menuDrawQuad(boxX, boxY, boxW, boxH, 0.12f, 0.14f, 0.18f, 0.96f);
    menuDrawBorder(boxX, boxY, boxW, boxH, 0.35f, 0.45f, 0.58f, 2.0f);
    menuDrawBorder(boxX + 3.0f, boxY + 3.0f, boxW - 6.0f, boxH - 6.0f, 1.0f, 0.82f, 0.20f, 1.5f);

    // Title
    const char* titleText = LOC("创建新世界", "Create New World");
    float tW = getTextWidth(titleText, true);
    drawText(boxX + (boxW - tW) * 0.5f, boxY + 34.0f, titleText, 1.0f, 0.85f, 0.25f, true);

    const char* subText = LOC("选择世界名称、游戏模式与初始种子", "Configure world name, game mode, and initial seed");
    float sW = getTextWidth(subText, false);
    drawText(boxX + (boxW - sW) * 0.5f, boxY + 56.0f, subText, 0.75f, 0.85f, 0.95f, false);

    // 1. World Name Option
    float nameBtnY = boxY + 85.0f;
    float nameBtnW = 480.0f;
    float nameBtnH = 46.0f;
    float nameBtnX = boxX + (boxW - nameBtnW) * 0.5f;
    bool nHov = (g_MenuMouseX >= nameBtnX && g_MenuMouseX <= nameBtnX + nameBtnW && g_MenuMouseY >= nameBtnY && g_MenuMouseY <= nameBtnY + nameBtnH);
    menuDrawQuad(nameBtnX, nameBtnY, nameBtnW, nameBtnH, nHov ? 0.22f : 0.16f, nHov ? 0.26f : 0.19f, nHov ? 0.34f : 0.25f, 1.0f);
    menuDrawBorder(nameBtnX, nameBtnY, nameBtnW, nameBtnH, 0.45f, 0.55f, 0.65f, 1.5f);

    const char* currentName = (getLanguage() == LANG_ZH) ? g_WorldNamePresets_ZH[g_CreateWorldNameIdx] : g_WorldNamePresets_EN[g_CreateWorldNameIdx];
    char namePrompt[128];
    snprintf(namePrompt, sizeof(namePrompt), "%s: %s [%s]", LOC("世界名称", "World Name"), currentName, LOC("点击切换预设", "Click to Cycle"));
    drawText(nameBtnX + 20.0f, nameBtnY + 28.0f, namePrompt, 1.0f, 0.95f, 0.45f, true);

    // 2. Game Mode Option
    float modeBtnY = boxY + 165.0f;
    float modeBtnW = 480.0f;
    float modeBtnH = 50.0f;
    float modeBtnX = boxX + (boxW - modeBtnW) * 0.5f;
    bool mHov = (g_MenuMouseX >= modeBtnX && g_MenuMouseX <= modeBtnX + modeBtnW && g_MenuMouseY >= modeBtnY && g_MenuMouseY <= modeBtnY + modeBtnH);
    menuDrawQuad(modeBtnX, modeBtnY, modeBtnW, modeBtnH, mHov ? 0.24f : 0.18f, mHov ? 0.28f : 0.21f, mHov ? 0.36f : 0.27f, 1.0f);

    if (g_CreateWorldMode == 0) {
        menuDrawBorder(modeBtnX, modeBtnY, modeBtnW, modeBtnH, 1.0f, 0.65f, 0.25f, 2.0f);
        drawText(modeBtnX + 20.0f, modeBtnY + 24.0f, LOC("游戏模式: 【生存模式】 [点击切换]", "Game Mode: [Survival Mode] [Click to Toggle]"), 1.0f, 0.70f, 0.25f, true);
        drawText(modeBtnX + 20.0f, modeBtnY + 42.0f, LOC("从0开局 · 拥有生命值 · 受到怪物攻击掉血", "Start from 0 · Player HP · Take damage from mobs"), 0.85f, 0.85f, 0.85f, false);
    } else {
        menuDrawBorder(modeBtnX, modeBtnY, modeBtnW, modeBtnH, 0.35f, 0.90f, 1.0f, 2.0f);
        drawText(modeBtnX + 20.0f, modeBtnY + 24.0f, LOC("游戏模式: 【创造模式】 [点击切换]", "Game Mode: [Creative Mode] [Click to Toggle]"), 0.35f, 0.90f, 1.0f, true);
        drawText(modeBtnX + 20.0f, modeBtnY + 42.0f, LOC("双击空格飞行 · 全量物品库 · 自由建筑无伤", "Double Space to fly · Full big inventory · God build"), 0.85f, 0.85f, 0.85f, false);
    }

    // 3. Seed Option
    float seedBtnY = boxY + 245.0f;
    float seedBtnW = 480.0f;
    float seedBtnH = 46.0f;
    float seedBtnX = boxX + (boxW - seedBtnW) * 0.5f;
    bool sHov = (g_MenuMouseX >= seedBtnX && g_MenuMouseX <= seedBtnX + seedBtnW && g_MenuMouseY >= seedBtnY && g_MenuMouseY <= seedBtnY + seedBtnH);
    menuDrawQuad(seedBtnX, seedBtnY, seedBtnW, seedBtnH, sHov ? 0.22f : 0.16f, sHov ? 0.26f : 0.19f, sHov ? 0.34f : 0.25f, 1.0f);
    menuDrawBorder(seedBtnX, seedBtnY, seedBtnW, seedBtnH, 0.45f, 0.55f, 0.65f, 1.5f);

    char seedBuf[128];
    snprintf(seedBuf, sizeof(seedBuf), "%s: %d  [%s]", LOC("世界种子", "World Seed"), g_CreateWorldSeed, LOC("🎲 点击随机种子", "🎲 Randomize"));
    drawText(seedBtnX + 20.0f, seedBtnY + 28.0f, seedBuf, 0.40f, 0.85f, 1.0f, true);

    // Action Buttons
    float cBtnW = 240.0f;
    float cBtnH = 46.0f;
    float cBtnX = boxX + 45.0f;
    float cBtnY = boxY + boxH - 70.0f;
    bool crHov = (g_MenuMouseX >= cBtnX && g_MenuMouseX <= cBtnX + cBtnW && g_MenuMouseY >= cBtnY && g_MenuMouseY <= cBtnY + cBtnH);
    menuDrawQuad(cBtnX, cBtnY, cBtnW, cBtnH, crHov ? 0.25f : 0.18f, crHov ? 0.55f : 0.38f, crHov ? 0.25f : 0.18f, 1.0f);
    menuDrawBorder(cBtnX, cBtnY, cBtnW, cBtnH, 0.4f, 0.9f, 0.4f, 2.0f);
    float cw = getTextWidth(LOC("确定创建并进入世界", "Create & Enter World"), true);
    drawText(cBtnX + (cBtnW - cw) * 0.5f, cBtnY + 28.0f, LOC("确定创建并进入世界", "Create & Enter World"), 1.0f, 1.0f, 1.0f, true);

    float cancelW = 200.0f;
    float cancelH = 46.0f;
    float cancelX = boxX + boxW - cancelW - 45.0f;
    float cancelY = boxY + boxH - 70.0f;
    bool canHov = (g_MenuMouseX >= cancelX && g_MenuMouseX <= cancelX + cancelW && g_MenuMouseY >= cancelY && g_MenuMouseY <= cancelY + cancelH);
    menuDrawQuad(cancelX, cancelY, cancelW, cancelH, canHov ? 0.30f : 0.20f, canHov ? 0.32f : 0.22f, canHov ? 0.38f : 0.28f, 1.0f);
    menuDrawBorder(cancelX, cancelY, cancelW, cancelH, 0.5f, 0.6f, 0.7f, 1.5f);
    float canW = getTextWidth(LOC("取消并返回", "Cancel"), true);
    drawText(cancelX + (cancelW - canW) * 0.5f, cancelY + 28.0f, LOC("取消并返回", "Cancel"), 0.9f, 0.9f, 0.9f, true);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// Input Handling
// ============================================================================
static bool handleMenuInput(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_CurrentState == STATE_SETTINGS) {
        return handleSettingsInput(uMsg, wParam, lParam);
    }
    if (g_CurrentState == STATE_WORLDS_LIST) {
        return handleWorldsListInput(uMsg, wParam, lParam);
    }
    if (g_CurrentState == STATE_CREATE_WORLD) {
        return handleCreateWorldInput(uMsg, wParam, lParam);
    }

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            if (g_CurrentState == STATE_MENU) {
                for (int i = 0; i < MAIN_MENU_BTN_COUNT; i++) {
                    MenuButton* btn = &g_MainMenuButtons[i];
                    btn->hovered = (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                                    g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h);
                }
                return true;
            } else if (g_CurrentState == STATE_PAUSED) {
                for (int i = 0; i < PAUSE_MENU_BTN_COUNT; i++) {
                    MenuButton* btn = &g_PauseMenuButtons[i];
                    btn->hovered = (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                                    g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h);
                }
                return true;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            g_MenuMouseX = (int)(short)LOWORD(lParam);
            g_MenuMouseY = (int)(short)HIWORD(lParam);

            if (g_CurrentState == STATE_MENU) {
                for (int i = 0; i < MAIN_MENU_BTN_COUNT; i++) {
                    MenuButton* btn = &g_MainMenuButtons[i];
                    if (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                        g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h) {
                        btn->active = true;
                        playSound(SFX_CLICK);

                        if (i == 0) {
                            // [SURVIVAL MODE] -> Open World List / Create with Survival preset
                            g_CreateWorldMode = 0;
                            setGameState(STATE_WORLDS_LIST);
                        } else if (i == 1) {
                            // [CREATIVE MODE] -> Open World List / Create with Creative preset
                            g_CreateWorldMode = 1;
                            setGameState(STATE_WORLDS_LIST);
                        } else if (i == 2) {
                            // [CONTINUE / LOAD WORLDS]
                            setGameState(STATE_WORLDS_LIST);
                        } else if (i == 3) {
                            // [SETTINGS]
                            g_PreviousState = STATE_MENU;
                            setGameState(STATE_SETTINGS);
                        } else if (i == 4) {
                            // [EXIT]
                            PostQuitMessage(0);
                        }
                        return true;
                    }
                }
                return true;
            } else if (g_CurrentState == STATE_PAUSED) {
                for (int i = 0; i < PAUSE_MENU_BTN_COUNT; i++) {
                    MenuButton* btn = &g_PauseMenuButtons[i];
                    if (g_MenuMouseX >= btn->x && g_MenuMouseX <= btn->x + btn->w &&
                        g_MenuMouseY >= btn->y && g_MenuMouseY <= btn->y + btn->h) {
                        btn->active = true;
                        playSound(SFX_CLICK);

                        if (i == 0) {
                            // Resume Game
                            setGameState(STATE_PLAYING);
                        } else if (i == 1) {
                            // Switch GameMode (Survival <-> Creative)
                            if (g_CurrentMode == MODE_SURVIVAL) {
                                setGameMode(MODE_CREATIVE);
                                showNotification(LOC("已切换至: 创造模式 [双击空格飞行]", "Switched to: Creative Mode [2x Space to Fly]"), 0.35f, 0.90f, 1.0f);
                            } else {
                                setGameMode(MODE_SURVIVAL);
                                showNotification(LOC("已切换至: 生存模式", "Switched to: Survival Mode"), 0.35f, 0.95f, 0.45f);
                            }
                        } else if (i == 2) {
                            // Direct Language Toggle
                            toggleLanguage();
                            showNotification(LOC("语言已切换为: 简体中文", "Language switched to: English"), 1.0f, 0.85f, 0.3f);
                        } else if (i == 3) {
                            // Settings Modal
                            g_PreviousState = STATE_PAUSED;
                            setGameState(STATE_SETTINGS);
                        } else if (i == 4) {
                            // Save World
                            saveGame();
                        } else if (i == 5) {
                            // Load World
                            loadGame();
                            setGameState(STATE_PLAYING);
                        } else if (i == 6) {
                            // Return to Main Menu
                            setGameState(STATE_MENU);
                        } else if (i == 7) {
                            // Exit to Desktop
                            saveGame();
                            PostQuitMessage(0);
                        }
                        return true;
                    }
                }
                return true;
            }
            break;
        }

        case WM_LBUTTONUP: {
            for (int i = 0; i < MAIN_MENU_BTN_COUNT; i++) g_MainMenuButtons[i].active = false;
            for (int i = 0; i < PAUSE_MENU_BTN_COUNT; i++) g_PauseMenuButtons[i].active = false;
            if (g_CurrentState == STATE_MENU || g_CurrentState == STATE_PAUSED) {
                return true;
            }
            break;
        }

        case WM_KEYDOWN: {
            // [ESC] toggles Pause Menu
            if (wParam == VK_ESCAPE) {
                if (g_CurrentState == STATE_PLAYING) {
                    setGameState(STATE_PAUSED);
                    return true;
                } else if (g_CurrentState == STATE_PAUSED) {
                    setGameState(STATE_PLAYING);
                    return true;
                }
            }

            // Double-tap Space Flight Toggle in Creative Mode
            if (wParam == VK_SPACE && g_CurrentState == STATE_PLAYING && g_CurrentMode == MODE_CREATIVE) {
                DWORD now = GetTickCount();
                float diffSec = (float)(now - g_LastSpacePressTick) / 1000.0f;
                if (diffSec <= FLIGHT_TAP_WINDOW) {
                    g_IsCreativeFlying = !g_IsCreativeFlying;
                    g_LastSpacePressTick = 0; // Consume double tap

                    if (g_IsCreativeFlying) {
                        playSound(SFX_FLY);
                        showNotification(LOC("创造飞行: 已开启 [空格: 上升, Shift/S: 下降]", "Creative Flight: Enabled [Space: Up, Shift/S: Down]"), 0.35f, 0.90f, 1.0f);
                    } else {
                        showNotification(LOC("创造飞行: 已关闭", "Creative Flight: Disabled"), 1.0f, 0.85f, 0.30f);
                    }
                } else {
                    g_LastSpacePressTick = now;
                }
            }

            // In menu or pause, consume keys so gameplay doesn't move
            if (g_CurrentState == STATE_MENU || g_CurrentState == STATE_PAUSED) {
                return true;
            }
            break;
        }

        case WM_KEYUP: {
            if (g_CurrentState == STATE_MENU || g_CurrentState == STATE_PAUSED) {
                return true;
            }
            break;
        }
    }
    return false;
}

// ============================================================================
// Creative Flight Physics
// ============================================================================
static void updateFlightPhysics(float dt) {
    // 1. Horizontal flight locomotion (boosted by 1.5x with smooth damping)
    float targetVX = 0.0f;
    float flightSpeed = g_Player.moveSpeed * 1.5f;

    if (g_Keys['A'] || g_Keys[VK_LEFT]) {
        targetVX -= flightSpeed;
        g_Player.isFacingRight = false;
    }
    if (g_Keys['D'] || g_Keys[VK_RIGHT]) {
        targetVX += flightSpeed;
        g_Player.isFacingRight = true;
    }

    // Smooth horizontal acceleration & deceleration
    float accelX = (fabsf(targetVX) > 0.1f) ? 35.0f : 20.0f;
    g_Player.vx += (targetVX - g_Player.vx) * accelX * dt;

    // 2. Vertical flight locomotion: Space flies up (+Y), Shift/S/Down flies down (-Y)
    float targetVY = 0.0f;
    float verticalFlySpeed = 11.0f;

    bool flyUp = g_Keys[VK_SPACE] || g_Keys['W'] || g_Keys[VK_UP];
    bool flyDown = g_Keys[VK_SHIFT] || g_Keys['S'] || g_Keys[VK_DOWN];

    if (flyUp && !flyDown) {
        targetVY = verticalFlySpeed;
    } else if (flyDown && !flyUp) {
        targetVY = -verticalFlySpeed;
    }

    // Zero gravity during flight! Smooth velocity damping
    float accelY = (fabsf(targetVY) > 0.1f) ? 32.0f : 22.0f;
    g_Player.vy += (targetVY - g_Player.vy) * accelY * dt;

    // 3. Horizontal block collision
    float nextX = g_Player.x + g_Player.vx * dt;
    int minX = (int)floorf(nextX - g_Player.width * 0.5f);
    int maxX = (int)floorf(nextX + g_Player.width * 0.5f);
    int minY = (int)floorf(g_Player.y + 0.1f);
    int maxY = (int)floorf(g_Player.y + g_Player.height - 0.1f);

    for (int y = minY; y <= maxY; y++) {
        if (g_Player.vx > 0.0f && isSolidBlock(maxX, y)) {
            nextX = (float)maxX - g_Player.width * 0.5f - 0.001f;
            g_Player.vx = 0.0f;
            break;
        } else if (g_Player.vx < 0.0f && isSolidBlock(minX, y)) {
            nextX = (float)(minX + 1) + g_Player.width * 0.5f + 0.001f;
            g_Player.vx = 0.0f;
            break;
        }
    }
    g_Player.x = nextX;

    // 4. Vertical block collision
    float nextY = g_Player.y + g_Player.vy * dt;
    minX = (int)floorf(g_Player.x - g_Player.width * 0.45f);
    maxX = (int)floorf(g_Player.x + g_Player.width * 0.45f);
    minY = (int)floorf(nextY);
    maxY = (int)floorf(nextY + g_Player.height);

    g_Player.isGrounded = false;
    if (g_Player.vy <= 0.0f) {
        for (int x = minX; x <= maxX; x++) {
            if (isSolidBlock(x, minY)) {
                nextY = (float)(minY + 1);
                g_Player.vy = 0.0f;
                g_Player.isGrounded = true;
                break;
            }
        }
    } else {
        for (int x = minX; x <= maxX; x++) {
            if (isSolidBlock(x, maxY)) {
                nextY = (float)maxY - g_Player.height - 0.001f;
                g_Player.vy = 0.0f;
                break;
            }
        }
    }
    g_Player.y = nextY;

    // Subtle hover animation
    g_Player.walkCycle += dt * 1.5f;

    // Camera follow
    float lookahead = g_Player.isFacingRight ? 2.5f : -2.5f;
    g_CamX += (g_Player.x + lookahead - g_CamX) * 5.0f * dt;
    g_CamY += (g_Player.y + 1.2f - g_CamY) * 5.0f * dt;
    if (g_CamY < 8.0f) g_CamY = 8.0f;
}

// ============================================================================
// Minecraft-Style Main Menu Rendering
// ============================================================================
static void renderMainMenu(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Tiled Dirt / Stone background
    menuDrawMinecraftDirtBackground((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    // 2. Title Banner Box (Centered Top)
    float bannerW = 680.0f;
    float bannerH = 115.0f;
    float bannerX = ((float)WINDOW_WIDTH - bannerW) * 0.5f;
    float bannerY = 40.0f;

    menuDrawQuad(bannerX, bannerY, bannerW, bannerH, 0.08f, 0.10f, 0.14f, 0.90f);
    menuDrawBorder(bannerX, bannerY, bannerW, bannerH, 0.35f, 0.45f, 0.58f, 2.0f);

    // Title: Luminora | 流明洛原 with drop shadow
    const char* titleStr = (getLanguage() == LANG_ZH) ? "Luminora | 流明洛原" : "LUMINORA";
    float titleW = getTextWidthUTF8(titleStr, FONT_SIZE_BANNER, true);
    float titleX = bannerX + (bannerW - titleW) * 0.5f;
    drawTextUTF8(titleX + 2.0f, bannerY + 45.0f + 2.0f, titleStr, 0.0f, 0.0f, 0.0f, FONT_SIZE_BANNER, true);
    drawTextUTF8(titleX, bannerY + 45.0f, titleStr, 1.0f, 0.88f, 0.25f, FONT_SIZE_BANNER, true);

    // Subtitle
    const char* subText = LOC("-- 光影交织的 2.5D 像素沙盒冒险 --", "-- 2.5D PROCEDURAL PIXEL SANDBOX ADVENTURE --");
    float subW = getTextWidth(subText, false);
    drawText(bannerX + (bannerW - subW) * 0.5f, bannerY + 74.0f, subText, 0.75f, 0.85f, 0.95f, false);

    // Animated Splash text
    static float s_SplashAngle = 0.0f;
    s_SplashAngle += 0.04f;
    float splashR = 1.0f;
    float splashG = 0.85f + sinf(s_SplashAngle) * 0.15f;
    float splashB = 0.20f;
    const char* splashText = LOC("* 包含创造模式与双击空格自由飞行！*", "* Now with Creative Mode & Double-Tap Space Flight! *");
    float splashW = getTextWidth(splashText, false);
    drawText(bannerX + (bannerW - splashW) * 0.5f, bannerY + 98.0f, splashText, splashR, splashG, splashB, false);

    // 3. Main Menu Buttons
    float btnW = 380.0f;
    float btnH = 46.0f;
    float btnGap = 12.0f;
    float btnX = ((float)WINDOW_WIDTH - btnW) * 0.5f;
    float btnStartY = bannerY + bannerH + 30.0f;

    g_MainMenuButtons[0].label = LOC("生存模式", "Survival Mode");
    g_MainMenuButtons[1].label = LOC("创造模式", "Creative Mode");
    g_MainMenuButtons[2].label = LOC("读取世界", "Load World");
    g_MainMenuButtons[3].label = LOC("游戏设置", "Settings");
    g_MainMenuButtons[4].label = LOC("退出游戏", "Exit Game");

    for (int i = 0; i < MAIN_MENU_BTN_COUNT; i++) {
        g_MainMenuButtons[i].x = btnX;
        g_MainMenuButtons[i].y = btnStartY + i * (btnH + btnGap);
        g_MainMenuButtons[i].w = btnW;
        g_MainMenuButtons[i].h = btnH;
        menuDrawMinecraftButton(&g_MainMenuButtons[i]);
    }

    // 4. Footer credits
    float footY = (float)WINDOW_HEIGHT - 28.0f;
    const char* footText = LOC("Luminora｜流明洛原 v0.4.0 独立引擎 | [F11] 切换全屏/窗口 | [ESC] 暂停菜单", "Luminora Standalone Engine v0.4.0 | [F11] Toggle Fullscreen | [ESC] Pause Menu");
    float footW = getTextWidth(footText, false);
    drawText(((float)WINDOW_WIDTH - footW) * 0.5f, footY, footText, 0.65f, 0.70f, 0.78f, false);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================================
// Minecraft-Style Pause Menu Rendering
// ============================================================================
static void renderPauseMenu(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Fullscreen dark overlay
    menuDrawQuad(0, 0, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, 0.0f, 0.0f, 0.68f);

    // 2. Pause Menu Stone Panel
    float boxW = 440.0f;
    float boxH = 490.0f;
    float boxX = ((float)WINDOW_WIDTH - boxW) * 0.5f;
    float boxY = ((float)WINDOW_HEIGHT - boxH) * 0.5f;

    menuDrawQuad(boxX, boxY, boxW, boxH, 0.12f, 0.14f, 0.18f, 0.96f);
    menuDrawBorder(boxX, boxY, boxW, boxH, 0.35f, 0.45f, 0.58f, 2.0f);
    menuDrawBorder(boxX + 3.0f, boxY + 3.0f, boxW - 6.0f, boxH - 6.0f, 1.0f, 0.82f, 0.20f, 1.5f);

    // Title
    const char* pauseTitle = LOC("游戏已暂停", "Game Paused");
    float tW = getTextWidth(pauseTitle, true);
    drawText(boxX + (boxW - tW) * 0.5f, boxY + 30.0f, pauseTitle, 1.0f, 0.85f, 0.25f, true);

    // Mode Status badge
    char modeStatusBuf[128];
    snprintf(modeStatusBuf, sizeof(modeStatusBuf), "%s: %s %s",
        LOC("当前模式", "Active Mode"),
        (g_CurrentMode == MODE_CREATIVE ? LOC("创造模式", "Creative Mode") : LOC("生存模式", "Survival Mode")),
        (isCreativeFlying() ? LOC("[飞行开启]", "[Flying Active]") : ""));
    float sW = getTextWidth(modeStatusBuf, false);
    drawText(boxX + (boxW - sW) * 0.5f, boxY + 54.0f, modeStatusBuf, 0.4f, 0.9f, 1.0f, false);

    // 3. Pause Menu Buttons
    float btnW = 360.0f;
    float btnH = 38.0f;
    float btnGap = 8.0f;
    float btnX = boxX + (boxW - btnW) * 0.5f;
    float btnStartY = boxY + 76.0f;

    g_PauseMenuButtons[0].label = LOC("继续游戏", "Resume Game");
    g_PauseMenuButtons[1].label = (g_CurrentMode == MODE_SURVIVAL) ?
        LOC("切换至: 创造模式", "Switch to: Creative Mode") :
        LOC("切换至: 生存模式", "Switch to: Survival Mode");
    g_PauseMenuButtons[2].label = LOC("语言: 简体中文", "Language: English");
    g_PauseMenuButtons[3].label = LOC("游戏设置", "Settings");
    g_PauseMenuButtons[4].label = LOC("保存世界 [F5]", "Save World [F5]");
    g_PauseMenuButtons[5].label = LOC("读取世界 [F9]", "Load World [F9]");
    g_PauseMenuButtons[6].label = LOC("返回主菜单", "Return to Main Menu");
    g_PauseMenuButtons[7].label = LOC("退出游戏", "Exit to Desktop");

    for (int i = 0; i < PAUSE_MENU_BTN_COUNT; i++) {
        g_PauseMenuButtons[i].x = btnX;
        g_PauseMenuButtons[i].y = btnStartY + i * (btnH + btnGap);
        g_PauseMenuButtons[i].w = btnW;
        g_PauseMenuButtons[i].h = btnH;
        menuDrawMinecraftButton(&g_PauseMenuButtons[i]);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

#endif // MENU_CREATIVE_H
