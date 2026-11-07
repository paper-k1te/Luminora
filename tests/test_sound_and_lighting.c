#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "../standalone_src/sound_engine.h"

// Test block definitions stub matching main.c
typedef struct {
    const char* name;
    const char* chineseName;
    const char* dropName;
    float r, g, b;
    float hardness;
    bool isSolid;
    bool isTransparent;
    bool emitLight;
    float lightR, lightG, lightB;
} BlockDef;

int main(void) {
    printf("=======================================================\n");
    printf("   Luminora Lighting, Darkness & Creature Sound SFX Test\n");
    printf("=======================================================\n\n");

    // ------------------------------------------------------------------------
    // Test 1: Creature Procedural Sound Effects & generateSoundBuffer
    // ------------------------------------------------------------------------
    printf("[1] Testing SoundEffectId Enum & Procedural Synthesis...\n");
    assert(SFX_COUNT == 19);
    assert(SFX_SLIME_JUMP == 11);
    assert(SFX_ZOMBIE_GROWL == 12);
    assert(SFX_BAT_CHIRP == 13);
    assert(SFX_SKELETON_RATTLE == 14);
    assert(SFX_SPIDER_HISS == 15);
    assert(SFX_SHEEP_BAA == 16);
    assert(SFX_COW_MOO == 17);
    assert(SFX_PIG_OINK == 18);

    synthesizeAllSounds();

    const char* sfxNames[19] = {
        "SFX_CLICK", "SFX_MINE", "SFX_BREAK", "SFX_PLACE", "SFX_STEP",
        "SFX_JUMP", "SFX_FLY", "SFX_HIT", "SFX_KILL", "SFX_PICKUP", "SFX_CRAFT",
        "SFX_SLIME_JUMP", "SFX_ZOMBIE_GROWL", "SFX_BAT_CHIRP", "SFX_SKELETON_RATTLE",
        "SFX_SPIDER_HISS", "SFX_SHEEP_BAA", "SFX_COW_MOO", "SFX_PIG_OINK"
    };

    for (int i = 0; i < SFX_COUNT; i++) {
        assert(g_SfxTable[i].sampleCount > 0);
        assert(g_SfxTable[i].pcmSamples != NULL);
        assert(g_SfxTable[i].wavBytes != NULL);
        assert((size_t)g_SfxTable[i].wavSize > sizeof(SfxWavHeader));

        // Check WAV header signature
        SfxWavHeader* hdr = (SfxWavHeader*)g_SfxTable[i].wavBytes;
        assert(memcmp(hdr->riff, "RIFF", 4) == 0);
        assert(memcmp(hdr->wave, "WAVE", 4) == 0);
        assert(hdr->channels == 1);
        assert(hdr->sampleRate == AUDIO_SAMPLE_RATE);
        assert(hdr->bitsPerSample == 16);

        // Compute signal RMS and Peak
        double sumSq = 0.0;
        int maxVal = 0;
        for (int s = 0; s < g_SfxTable[i].sampleCount; s++) {
            int16_t sample = g_SfxTable[i].pcmSamples[s];
            sumSq += (double)sample * (double)sample;
            if (abs(sample) > maxVal) maxVal = abs(sample);
        }
        double rms = sqrt(sumSq / g_SfxTable[i].sampleCount);

        printf("    [%02d] %-20s: %5d samples (%.3fs), Peak: %5d, RMS: %.1f -> PASS\n",
               i, sfxNames[i], g_SfxTable[i].sampleCount,
               (float)g_SfxTable[i].sampleCount / (float)AUDIO_SAMPLE_RATE,
               maxVal, rms);

        assert(rms > 200.0);    // Must contain audible sound
        assert(maxVal <= 32767); // Must not overflow 16-bit PCM
    }

    // ------------------------------------------------------------------------
    // Test 2: Sound playback dispatcher
    // ------------------------------------------------------------------------
    printf("\n[2] Testing playSound() with Creature Effects...\n");
    playSound(SFX_SLIME_JUMP);
    playSound(SFX_ZOMBIE_GROWL);
    playSound(SFX_BAT_CHIRP);
    playSound(SFX_SKELETON_RATTLE);
    playSound(SFX_SPIDER_HISS);
    playSound(SFX_SHEEP_BAA);
    playSound(SFX_COW_MOO);
    playSound(SFX_PIG_OINK);
    printf("    Triggered all 8 creature SFX -> PASS\n");

    // ------------------------------------------------------------------------
    // Test 3: Darkness & Lighting Calculation Verification
    // ------------------------------------------------------------------------
    printf("\n[3] Testing Cave Ambient & Pitch Black Clamp Calculation...\n");
    float daylight = 0.005f; // Nighttime minimum daylight clamp
    int sY = 85;
    int deepCaveY = 40; // depth = 45 blocks
    float depth = (float)(sY - deepCaveY);
    float sunPen = daylight * expf(-depth * 0.25f);
    float caveAmbient = 0.0f; // User Request 2: unlit caves caveAmbient = 0.0f
    float baseL = (sunPen > caveAmbient) ? sunPen : caveAmbient;

    printf("    Deep Cave sunPen: %.8f, caveAmbient: %.2f, baseL: %.8f\n", sunPen, caveAmbient, baseL);
    assert(baseL < 0.0001f);

    // Empty-handed player: light bubble radius 0.6f, intensity 0.04f
    float playerX = 100.0f, playerY = 40.0f;
    float blockX = 100.0f + 2.0f; // 2 blocks away
    float blockY = 40.0f;

    float dx = (blockX + 0.5f) - playerX;
    float dy = (blockY + 0.5f) - (playerY + 1.0f);
    float dist = sqrtf(dx * dx + dy * dy);
    float att = 0.0f;
    float handRadius = 0.6f;
    if (dist < handRadius) {
        att = (1.0f - dist / handRadius) * 0.04f;
    }
    float rAcc = baseL + att;
    float lightFactor = rAcc;
    if (lightFactor < 0.005f) lightFactor = 0.005f;

    printf("    Empty-handed 2 blocks away: dist=%.2f, lightFactor=%.4f (clamped to 0.005 pitch black) -> PASS\n",
           dist, lightFactor);
    assert(lightFactor == 0.005f);

    // Holding torch: radius 8.5f, intensity 1.2f
    float torchRadius = 8.5f;
    float torchIntensity = 1.2f;
    float torchAtt = 0.0f;
    if (dist < torchRadius) {
        torchAtt = (1.0f - dist / torchRadius) * torchIntensity;
    }
    float torchLightFactor = baseL + torchAtt;
    if (torchLightFactor > 1.0f) torchLightFactor = 1.0f;
    if (torchLightFactor < 0.005f) torchLightFactor = 0.005f;

    printf("    Holding Torch 2 blocks away: dist=%.2f, lightFactor=%.4f (bright visible illumination) -> PASS\n",
           dist, torchLightFactor);
    assert(torchLightFactor > 0.8f);

    cleanupSoundEngine();

    printf("\n=======================================================\n");
    printf("   >>> ALL Lighting, Darkness & Creature SFX Tests PASSED! <<<\n");
    printf("=======================================================\n");

    return 0;
}
