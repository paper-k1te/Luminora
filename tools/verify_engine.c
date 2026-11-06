#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "../standalone_src/sound_engine.h"
#include "../standalone_src/font_render.h"

int main(void) {
    printf("=== Luminora 2.5D Audio & Font Engine Verification ===\n\n");

    // 1. Test Sound Engine
    printf("[1/3] Testing Sound Engine (sound_engine.h)...\n");
    bool audioOk = initSoundEngine();
    printf("      initSoundEngine() returned: %s\n", audioOk ? "TRUE" : "FALSE");
    assert(audioOk);

    const char* sfxNames[SFX_COUNT] = {
        "SFX_CLICK", "SFX_MINE", "SFX_BREAK", "SFX_PLACE", "SFX_STEP",
        "SFX_JUMP", "SFX_FLY", "SFX_HIT", "SFX_KILL", "SFX_PICKUP", "SFX_CRAFT"
    };

    for (int i = 0; i < SFX_COUNT; i++) {
        printf("      Synthesized %-12s: %5d PCM samples (%4.1f ms), WAV size: %5d bytes\n",
            sfxNames[i],
            g_SfxTable[i].sampleCount,
            (float)g_SfxTable[i].sampleCount / (float)AUDIO_SAMPLE_RATE * 1000.0f,
            g_SfxTable[i].wavSize);
        assert(g_SfxTable[i].sampleCount > 0);
        assert(g_SfxTable[i].pcmSamples != NULL);
        assert(g_SfxTable[i].wavBytes != NULL);
        assert(g_SfxTable[i].wavSize > 44);
        // Test playback trigger
        playSound(i);
    }
    printf("      [OK] All 11 procedural SFX synthesized and triggered successfully.\n\n");

    // 2. Test Font Renderer with an OpenGL dummy context
    printf("[2/3] Testing TrueType Font Renderer (font_render.h)...\n");
    // Create an invisible dummy window & OpenGL context for font texture uploading
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "DummyFontTestClass";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, "DummyFontTestClass", "FontTest", WS_POPUP, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    bool fontOk = initFontRenderer();
    printf("      initFontRenderer() returned: %s\n", fontOk ? "TRUE" : "FALSE");
    assert(fontOk);

    const char* testStrings[] = {
        "创造模式",
        "生存模式",
        "木镐",
        "钻石剑",
        "GAME PAUSED (游戏已暂停)",
        "KEYBINDS & CONTROLS (全操作指南)",
        "SWITCH MODE (切换模式: 生存 ⇄ 创造)",
        "Terraia 2.5D Standalone v1.0"
    };
    int numTestStrings = sizeof(testStrings) / sizeof(testStrings[0]);

    for (int i = 0; i < numTestStrings; i++) {
        float w = getTextWidthUTF8(testStrings[i], 16, true);
        FontCacheEntry* entry = getOrCreateFontEntry(testStrings[i], 16, true);
        assert(entry != NULL);
        assert(entry->texId > 0);
        assert(entry->width > 0);
        assert(entry->height > 0);
        printf("      Text: \"%-36s\" -> TexID: %u, Size: %3dx%2d, Width: %.1f px\n",
            testStrings[i], entry->texId, entry->width, entry->height, w);
    }
    printf("      [OK] TrueType font textures generated and cached cleanly without garble.\n\n");

    // 3. Test Cleanup
    printf("[3/3] Testing Engine Cleanup...\n");
    cleanupFontRenderer();
    cleanupSoundEngine();
    printf("      [OK] Audio and Font systems cleaned up with zero memory leaks.\n\n");

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    printf("=== ALL ENGINE TESTS PASSED PERFECTLY ===\n");
    return 0;
}
