#ifndef FONT_RENDER_H
#define FONT_RENDER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif

// ============================================================================
// Font Rendering Configuration & Cache Constants
// ============================================================================
#define FONT_CACHE_SIZE   512
#define MAX_CACHED_FONTS  32

typedef struct {
    int   fontSize;
    bool  isBold;
    HFONT hFont;
} CachedFontItem;

typedef struct {
    uint32_t hash;
    char*    utf8Text;
    int      fontSize;
    bool     isBold;
    GLuint   texId;
    int      width;
    int      height;
    int      ascent;
    uint32_t lastUsed;
    bool     active;
} FontCacheEntry;

// ============================================================================
// Internal State
// ============================================================================
static HDC            s_FontMemDC = NULL;
static CachedFontItem g_CachedFonts[MAX_CACHED_FONTS];
static int            g_NumCachedFonts = 0;
static FontCacheEntry g_FontCache[FONT_CACHE_SIZE];
static uint32_t       g_FontAccessCount = 0;
static bool           g_FontRendererInitialized = false;

// Forward Declarations
static inline bool  initFontRenderer(void);
static inline void  cleanupFontRenderer(void);
static inline void  drawTextUTF8(float x, float y, const char* utf8Text, float r, float g, float b, int fontSize, bool isBold);
static inline void  drawText(float x, float y, const char* str, float r, float g, float b, bool isTitle);
static inline float getTextWidthUTF8(const char* utf8Text, int fontSize, bool isBold);

// ============================================================================
// Helper: Get or Create Win32 HFONT
// ============================================================================
static HFONT getOrCreateWin32Font(int fontSize, bool isBold) {
    if (fontSize < 8)  fontSize = 8;
    if (fontSize > 96) fontSize = 96;

    for (int i = 0; i < g_NumCachedFonts; i++) {
        if (g_CachedFonts[i].fontSize == fontSize && g_CachedFonts[i].isBold == isBold) {
            return g_CachedFonts[i].hFont;
        }
    }

    // Create high-grade anti-aliased TrueType font (Microsoft YaHei for crisp Chinese & English)
    HFONT hFont = CreateFontW(
        -fontSize,
        0, 0, 0,
        isBold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei"
    );

    // Fallbacks if Microsoft YaHei is absent
    if (!hFont) {
        hFont = CreateFontW(
            -fontSize, 0, 0, 0,
            isBold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"SimHei"
        );
    }
    if (!hFont) {
        hFont = CreateFontW(
            -fontSize, 0, 0, 0,
            isBold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
    }
    if (!hFont) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    if (g_NumCachedFonts < MAX_CACHED_FONTS) {
        g_CachedFonts[g_NumCachedFonts].fontSize = fontSize;
        g_CachedFonts[g_NumCachedFonts].isBold = isBold;
        g_CachedFonts[g_NumCachedFonts].hFont = hFont;
        g_NumCachedFonts++;
    }

    return hFont;
}

// ============================================================================
// Helper: Hash Function for String Texture Cache
// ============================================================================
static inline uint32_t fontCalcHash(const char* str, int fontSize, bool isBold) {
    uint32_t h = 2166136261u;
    while (*str) {
        h ^= (uint8_t)(*str++);
        h *= 16777619u;
    }
    h ^= (uint32_t)fontSize;
    h *= 16777619u;
    h ^= isBold ? 1 : 0;
    h *= 16777619u;
    return h;
}

// ============================================================================
// Helper: Rasterize UTF-8 string to 32-bit RGBA OpenGL 2D Texture
// ============================================================================
static GLuint rasterizeTextTexture(const char* utf8Text, int fontSize, bool isBold, int* outW, int* outH, int* outAscent) {
    if (!s_FontMemDC) {
        s_FontMemDC = CreateCompatibleDC(NULL);
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Text, -1, NULL, 0);
    if (wlen <= 1) return 0;

    wchar_t* wstr = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wstr) return 0;
    MultiByteToWideChar(CP_UTF8, 0, utf8Text, -1, wstr, wlen);

    HFONT hFont = getOrCreateWin32Font(fontSize, isBold);
    HFONT oldFont = (HFONT)SelectObject(s_FontMemDC, hFont);

    // Measure exact TrueType bounding box & font ascent
    RECT rc = { 0, 0, 0, 0 };
    DrawTextW(s_FontMemDC, wstr, -1, &rc, DT_CALCRECT | DT_NOPREFIX);

    TEXTMETRICW tm;
    GetTextMetricsW(s_FontMemDC, &tm);

    int padX = 2;
    int padY = 2;
    int textW = rc.right - rc.left;
    int textH = rc.bottom - rc.top;
    if (textW <= 0) textW = 1;
    if (textH <= 0) textH = 1;

    int bmpW = textW + padX * 2;
    int bmpH = textH + padY * 2;
    int ascent = tm.tmAscent + padY;

    // Create 32-bit DIB section (top-down)
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmpW;
    bmi.bmiHeader.biHeight = -bmpH; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBmp = CreateDIBSection(s_FontMemDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBmp || !pBits) {
        SelectObject(s_FontMemDC, oldFont);
        free(wstr);
        return 0;
    }

    HBITMAP oldBmp = (HBITMAP)SelectObject(s_FontMemDC, hBmp);

    // Clear background to pure black (0x00000000)
    memset(pBits, 0, bmpW * bmpH * 4);

    // Render anti-aliased pure white text on black background
    SetBkMode(s_FontMemDC, TRANSPARENT);
    SetTextColor(s_FontMemDC, RGB(255, 255, 255));

    RECT drawRc = { padX, padY, bmpW - padX, bmpH - padY };
    DrawTextW(s_FontMemDC, wstr, -1, &drawRc, DT_LEFT | DT_TOP | DT_NOPREFIX);
    GdiFlush();

    // Convert grayscale anti-aliasing directly to RGBA alpha channel
    int totalPixels = bmpW * bmpH;
    uint8_t* rgba = (uint8_t*)malloc(totalPixels * 4);
    if (!rgba) {
        SelectObject(s_FontMemDC, oldBmp);
        DeleteObject(hBmp);
        SelectObject(s_FontMemDC, oldFont);
        free(wstr);
        return 0;
    }

    uint8_t* src = (uint8_t*)pBits;
    for (int i = 0; i < totalPixels; i++) {
        uint8_t b = src[i * 4 + 0];
        uint8_t g = src[i * 4 + 1];
        uint8_t r = src[i * 4 + 2];
        // Luminance-based alpha channel extraction preserves smooth subpixel borders
        uint8_t a = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = a;
    }

    // Upload to OpenGL Texture
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmpW, bmpH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Free temporary memory & GDI bitmap
    free(rgba);
    SelectObject(s_FontMemDC, oldBmp);
    DeleteObject(hBmp);
    SelectObject(s_FontMemDC, oldFont);
    free(wstr);

    *outW = bmpW;
    *outH = bmpH;
    *outAscent = ascent;
    return texId;
}

// ============================================================================
// Helper: Cache Management with LRU Eviction
// ============================================================================
static FontCacheEntry* getOrCreateFontEntry(const char* utf8Text, int fontSize, bool isBold) {
    if (!utf8Text || utf8Text[0] == '\0') return NULL;
    uint32_t hash = fontCalcHash(utf8Text, fontSize, isBold);

    // 1. Search for existing entry
    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        if (!g_FontCache[i].active) continue;
        if (g_FontCache[i].hash == hash &&
            g_FontCache[i].fontSize == fontSize &&
            g_FontCache[i].isBold == isBold &&
            g_FontCache[i].utf8Text != NULL &&
            strcmp(g_FontCache[i].utf8Text, utf8Text) == 0) {
            g_FontCache[i].lastUsed = ++g_FontAccessCount;
            return &g_FontCache[i];
        }
    }

    // 2. Find empty slot or least recently used (LRU) slot
    int target = -1;
    uint32_t oldest = 0xFFFFFFFF;
    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        if (!g_FontCache[i].active) {
            target = i;
            break;
        }
        if (g_FontCache[i].lastUsed < oldest) {
            oldest = g_FontCache[i].lastUsed;
            target = i;
        }
    }

    if (target < 0) target = 0;

    // Evict existing entry if occupied
    if (g_FontCache[target].active) {
        if (g_FontCache[target].texId) {
            glDeleteTextures(1, &g_FontCache[target].texId);
            g_FontCache[target].texId = 0;
        }
        if (g_FontCache[target].utf8Text) {
            free(g_FontCache[target].utf8Text);
            g_FontCache[target].utf8Text = NULL;
        }
        g_FontCache[target].active = false;
    }

    int bmpW = 0, bmpH = 0, ascent = 0;
    GLuint texId = rasterizeTextTexture(utf8Text, fontSize, isBold, &bmpW, &bmpH, &ascent);
    if (!texId) return NULL;

    g_FontCache[target].hash = hash;
    g_FontCache[target].utf8Text = strdup(utf8Text);
    g_FontCache[target].fontSize = fontSize;
    g_FontCache[target].isBold = isBold;
    g_FontCache[target].texId = texId;
    g_FontCache[target].width = bmpW;
    g_FontCache[target].height = bmpH;
    g_FontCache[target].ascent = ascent;
    g_FontCache[target].lastUsed = ++g_FontAccessCount;
    g_FontCache[target].active = true;

    return &g_FontCache[target];
}

// ============================================================================
// Public API Functions
// ============================================================================

// Initialize font rendering system
static inline bool initFontRenderer(void) {
    if (g_FontRendererInitialized) return true;

    if (!s_FontMemDC) {
        s_FontMemDC = CreateCompatibleDC(NULL);
    }
    memset(g_FontCache, 0, sizeof(g_FontCache));
    memset(g_CachedFonts, 0, sizeof(g_CachedFonts));
    g_NumCachedFonts = 0;
    g_FontAccessCount = 0;

    g_FontRendererInitialized = true;
    return true;
}

// Draw UTF-8 string with TrueType font rasterization & texture cache
static inline void drawTextUTF8(float x, float y, const char* utf8Text, float r, float g, float b, int fontSize, bool isBold) {
    if (!utf8Text || utf8Text[0] == '\0') return;
    if (!g_FontRendererInitialized) {
        initFontRenderer();
    }

    FontCacheEntry* entry = getOrCreateFontEntry(utf8Text, fontSize, isBold);
    if (!entry || !entry->texId) return;

    // Baseline alignment matching standard OpenGL raster position
    float qx = x - 2.0f;
    float qy = y - (float)entry->ascent;
    float qw = (float)entry->width;
    float qh = (float)entry->height;

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, entry->texId);
    glColor4f(r, g, b, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(qx, qy);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(qx + qw, qy);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(qx + qw, qy + qh);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(qx, qy + qh);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    if (!blendEnabled) glDisable(GL_BLEND);
}

// Drop-in replacement for old drawText(x, y, str, r, g, b, isTitle)
static inline void drawText(float x, float y, const char* str, float r, float g, float b, bool isTitle) {
    drawTextUTF8(x, y, str, r, g, b, isTitle ? 18 : 15, true);
}

// Measure pixel width of a UTF-8 string
static inline float getTextWidthUTF8(const char* utf8Text, int fontSize, bool isBold) {
    if (!utf8Text || utf8Text[0] == '\0') return 0.0f;
    if (!g_FontRendererInitialized) {
        initFontRenderer();
    }
    FontCacheEntry* entry = getOrCreateFontEntry(utf8Text, fontSize, isBold);
    if (!entry) return 0.0f;
    return (float)(entry->width - 4); // minus the 2*padX
}

// Cleanup all texture cache entries, HFONTs, and memory DC
static inline void cleanupFontRenderer(void) {
    if (!g_FontRendererInitialized) return;

    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        if (g_FontCache[i].active) {
            if (g_FontCache[i].texId) {
                glDeleteTextures(1, &g_FontCache[i].texId);
                g_FontCache[i].texId = 0;
            }
            if (g_FontCache[i].utf8Text) {
                free(g_FontCache[i].utf8Text);
                g_FontCache[i].utf8Text = NULL;
            }
            g_FontCache[i].active = false;
        }
    }

    for (int i = 0; i < g_NumCachedFonts; i++) {
        if (g_CachedFonts[i].hFont) {
            DeleteObject(g_CachedFonts[i].hFont);
            g_CachedFonts[i].hFont = NULL;
        }
    }
    g_NumCachedFonts = 0;

    if (s_FontMemDC) {
        DeleteDC(s_FontMemDC);
        s_FontMemDC = NULL;
    }

    g_FontRendererInitialized = false;
}

#endif // FONT_RENDER_H
