#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

// ============================================================================
// Sound Effect Enumeration (11 Procedural Retro 2.5D Sound Effects)
// ============================================================================
typedef enum {
    SFX_CLICK = 0,      // 1.  UI button click (short square click)
    SFX_MINE,           // 2.  Block dig chip sound (crunch noise + impact)
    SFX_BREAK,          // 3.  Block destroyed (burst crunch)
    SFX_PLACE,          // 4.  Block placed (thud)
    SFX_STEP,           // 5.  Footstep (soft rustle/thud)
    SFX_JUMP,           // 6.  Jump spring (sine upward sweep)
    SFX_FLY,            // 7.  Creative flight whoosh (harmonic synth tone)
    SFX_HIT,            // 8.  Mob hit (punch impact)
    SFX_KILL,           // 9.  Mob death (8-bit explosion noise)
    SFX_PICKUP,         // 10. Item pickup (two-tone coin chime)
    SFX_CRAFT,          // 11. Crafting success fanfare (bright tri-tone chime)
    SFX_COUNT
} SoundEffectId;

// ============================================================================
// Audio Format & Internal Structures
// ============================================================================
#define AUDIO_SAMPLE_RATE     22050
#define AUDIO_BUFFER_SAMPLES  1024
#define AUDIO_NUM_BUFFERS     2
#define MAX_AUDIO_VOICES      16

#pragma pack(push, 1)
typedef struct {
    char     riff[4];        // "RIFF"
    uint32_t overallSize;    // 36 + dataSize
    char     wave[4];        // "WAVE"
    char     fmt[4];         // "fmt "
    uint32_t fmtLength;      // 16
    uint16_t formatType;     // 1 (PCM)
    uint16_t channels;       // 1 (Mono)
    uint32_t sampleRate;     // 22050
    uint32_t byteRate;       // 22050 * 2 = 44100
    uint16_t blockAlign;     // 2
    uint16_t bitsPerSample;  // 16
    char     data[4];        // "data"
    uint32_t dataSize;       // sampleCount * 2
} SfxWavHeader;
#pragma pack(pop)

typedef struct {
    int          sampleCount;
    int16_t*     pcmSamples;
    uint8_t*     wavBytes;
    int          wavSize;
} SfxSound;

typedef struct {
    int   sfxId;
    int   cursor;
    float volume;
    bool  active;
} SfxVoice;

// ============================================================================
// Internal State
// ============================================================================
static SfxSound          g_SfxTable[SFX_COUNT];
static SfxVoice          g_AudioVoices[MAX_AUDIO_VOICES];
static HWAVEOUT          g_hWaveOut = NULL;
static HANDLE            g_hAudioThread = NULL;
static HANDLE            g_hAudioEvent = NULL;
static CRITICAL_SECTION  g_AudioLock;
static volatile bool     g_AudioRunning = false;
static bool              g_AudioMixerActive = false;
static bool              g_SoundEngineInitialized = false;
static bool              g_SoundEnabled = true;

static inline bool isSoundEnabled(void) {
    return g_SoundEnabled;
}

static inline void setSoundEnabled(bool enabled) {
    g_SoundEnabled = enabled;
}

static inline void toggleSound(void) {
    g_SoundEnabled = !g_SoundEnabled;
}

static WAVEHDR           g_AudioWaveHdrs[AUDIO_NUM_BUFFERS];
static int16_t           g_AudioBuffers[AUDIO_NUM_BUFFERS][AUDIO_BUFFER_SAMPLES];

// Pseudo-random float in [-1.0f, 1.0f]
static inline float sfxRandomFloat(void) {
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

// ============================================================================
// Procedural Synthesis of All 11 Sound Effects
// ============================================================================
static void synthesizeAllSounds(void) {
    // 1. SFX_CLICK: UI button click (short square click)
    {
        float duration = 0.035f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = (t < 0.010f) ? 1400.0f : 850.0f;
            float phase = 2.0f * (float)M_PI * freq * t;
            float wave = (sinf(phase) > 0.0f) ? 0.7f : -0.7f;
            wave += 0.3f * sinf(phase);
            float env = expf(-95.0f * t);
            float sample = wave * env * 18000.0f;
            pcm[i] = (int16_t)sample;
        }
        g_SfxTable[SFX_CLICK].sampleCount = count;
        g_SfxTable[SFX_CLICK].pcmSamples = pcm;
    }

    // 2. SFX_MINE: Block dig chip sound (crunch noise + impact)
    {
        float duration = 0.090f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 190.0f * expf(-32.0f * t) + 40.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float impact = sinf(phase) * expf(-26.0f * t);
            float noise = sfxRandomFloat() * expf(-36.0f * t);
            float mix = impact * 0.55f + noise * 0.45f;
            pcm[i] = (int16_t)(mix * 24000.0f);
        }
        g_SfxTable[SFX_MINE].sampleCount = count;
        g_SfxTable[SFX_MINE].pcmSamples = pcm;
    }

    // 3. SFX_BREAK: Block destroyed (burst crunch)
    {
        float duration = 0.180f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 250.0f * expf(-16.0f * t) + 45.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float rumble = sinf(phase);
            if (rumble > 0.4f) rumble = 0.4f;
            else if (rumble < -0.4f) rumble = -0.4f;
            rumble *= 2.5f;

            float crackle = sfxRandomFloat() * (1.0f + 0.4f * sinf(160.0f * (float)M_PI * t));
            float env = (t < 0.005f) ? (t / 0.005f) : expf(-13.0f * t);
            float mix = (rumble * 0.40f + crackle * 0.60f) * env;
            pcm[i] = (int16_t)(mix * 25000.0f);
        }
        g_SfxTable[SFX_BREAK].sampleCount = count;
        g_SfxTable[SFX_BREAK].pcmSamples = pcm;
    }

    // 4. SFX_PLACE: Block placed (solid thud)
    {
        float duration = 0.075f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 210.0f * expf(-45.0f * t) + 42.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float click = (t < 0.008f) ? (sfxRandomFloat() * 0.3f) : 0.0f;
            float wave = sinf(phase) + click;
            float env = expf(-36.0f * t);
            pcm[i] = (int16_t)(wave * env * 22000.0f);
        }
        g_SfxTable[SFX_PLACE].sampleCount = count;
        g_SfxTable[SFX_PLACE].pcmSamples = pcm;
    }

    // 5. SFX_STEP: Footstep (soft rustle/thud)
    {
        float duration = 0.055f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 90.0f * expf(-30.0f * t) + 40.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float thump = sinf(phase) * 0.6f;
            float rustle = sfxRandomFloat() * 0.4f;
            float env = expf(-55.0f * t);
            pcm[i] = (int16_t)((thump + rustle) * env * 13500.0f);
        }
        g_SfxTable[SFX_STEP].sampleCount = count;
        g_SfxTable[SFX_STEP].pcmSamples = pcm;
    }

    // 6. SFX_JUMP: Jump spring (sine upward sweep)
    {
        float duration = 0.160f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float progress = t / duration;
            float freq = 150.0f + 320.0f * powf(progress, 1.35f);
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float wave = sinf(phase) * 0.8f + sinf(phase * 2.0f) * 0.2f;
            float env = (t < 0.015f) ? (t / 0.015f) : expf(-9.0f * (t - 0.015f));
            pcm[i] = (int16_t)(wave * env * 22000.0f);
        }
        g_SfxTable[SFX_JUMP].sampleCount = count;
        g_SfxTable[SFX_JUMP].pcmSamples = pcm;
    }

    // 7. SFX_FLY: Creative flight whoosh (harmonic synth tone)
    {
        float duration = 0.260f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float vibrato = 1.0f + 0.025f * sinf(24.0f * (float)M_PI * t);
            float f1 = 330.0f * vibrato; // E4
            float f2 = 495.0f * vibrato; // B4
            float f3 = 660.0f * vibrato; // E5
            float tone = sinf(2.0f * (float)M_PI * f1 * t) * 0.45f +
                         sinf(2.0f * (float)M_PI * f2 * t) * 0.30f +
                         sinf(2.0f * (float)M_PI * f3 * t) * 0.25f;
            float air = sfxRandomFloat() * 0.15f;
            float env = (t < 0.035f) ? (t / 0.035f) : expf(-7.5f * (t - 0.035f));
            pcm[i] = (int16_t)((tone + air) * env * 21000.0f);
        }
        g_SfxTable[SFX_FLY].sampleCount = count;
        g_SfxTable[SFX_FLY].pcmSamples = pcm;
    }

    // 8. SFX_HIT: Mob hit (punch impact)
    {
        float duration = 0.115f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 440.0f * expf(-35.0f * t) + 65.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float s = sinf(phase);
            // Soft clipping overdrive distortion
            s = 1.5f * s - 0.5f * s * s * s;
            float punchNoise = (t < 0.030f) ? (sfxRandomFloat() * 0.5f) : 0.0f;
            float env = expf(-26.0f * t);
            pcm[i] = (int16_t)((s * 0.7f + punchNoise * 0.3f) * env * 24500.0f);
        }
        g_SfxTable[SFX_HIT].sampleCount = count;
        g_SfxTable[SFX_HIT].pcmSamples = pcm;
    }

    // 9. SFX_KILL: Mob death (8-bit explosion noise)
    {
        float duration = 0.340f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float phase = 0.0f;
        float holdNoise = 0.0f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq = 160.0f * expf(-8.5f * t) + 32.0f;
            phase += 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE;
            float rumble = sinf(phase) * 0.4f;

            // Sample and hold noise (classic retro 8-bit sound chip frequency divider)
            if ((i % 6) == 0) {
                holdNoise = sfxRandomFloat();
            }

            float env = (t < 0.020f) ? (t / 0.020f) : expf(-6.8f * t);
            float mix = (rumble + holdNoise * 0.65f) * env;
            pcm[i] = (int16_t)(mix * 25000.0f);
        }
        g_SfxTable[SFX_KILL].sampleCount = count;
        g_SfxTable[SFX_KILL].pcmSamples = pcm;
    }

    // 10. SFX_PICKUP: Item pickup (two-tone coin chime)
    {
        float duration = 0.150f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float tSplit = 0.055f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq, relT, env;
            if (t < tSplit) {
                freq = 987.77f; // B5
                relT = t;
                env = expf(-18.0f * relT);
            } else {
                freq = 1318.51f; // E6
                relT = t - tSplit;
                env = expf(-14.0f * relT);
            }
            float phase = 2.0f * (float)M_PI * freq * t;
            float tone = sinf(phase) * 0.85f + sinf(phase * 2.0f) * 0.15f;
            pcm[i] = (int16_t)(tone * env * 22000.0f);
        }
        g_SfxTable[SFX_PICKUP].sampleCount = count;
        g_SfxTable[SFX_PICKUP].pcmSamples = pcm;
    }

    // 11. SFX_CRAFT: Crafting success fanfare (bright tri-tone chime)
    {
        float duration = 0.380f;
        int count = (int)(duration * AUDIO_SAMPLE_RATE);
        int16_t* pcm = (int16_t*)calloc(count, sizeof(int16_t));
        float t1 = 0.080f;
        float t2 = 0.160f;
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float freq, env;
            if (t < t1) {
                freq = 1046.50f; // C6
                env = expf(-14.0f * t);
            } else if (t < t2) {
                freq = 1318.51f; // E6
                env = expf(-14.0f * (t - t1));
            } else {
                freq = 1567.98f; // G6
                env = expf(-6.2f * (t - t2));
            }
            float phase = 2.0f * (float)M_PI * freq * t;
            // Rich glockenspiel overtone structure
            float tone = sinf(phase) * 0.80f + sinf(phase * 3.0f) * 0.15f + sinf(phase * 4.0f) * 0.05f;
            pcm[i] = (int16_t)(tone * env * 23000.0f);
        }
        g_SfxTable[SFX_CRAFT].sampleCount = count;
        g_SfxTable[SFX_CRAFT].pcmSamples = pcm;
    }

    // Wrap each synthesized PCM stream into a valid in-memory RIFF WAV buffer
    for (int k = 0; k < SFX_COUNT; k++) {
        int sampleCount = g_SfxTable[k].sampleCount;
        int dataSize = sampleCount * sizeof(int16_t);
        int totalWavSize = sizeof(SfxWavHeader) + dataSize;

        uint8_t* wavBytes = (uint8_t*)malloc(totalWavSize);
        SfxWavHeader* hdr = (SfxWavHeader*)wavBytes;

        memcpy(hdr->riff, "RIFF", 4);
        hdr->overallSize = (uint32_t)(36 + dataSize);
        memcpy(hdr->wave, "WAVE", 4);
        memcpy(hdr->fmt, "fmt ", 4);
        hdr->fmtLength = 16;
        hdr->formatType = 1;        // PCM
        hdr->channels = 1;          // Mono
        hdr->sampleRate = AUDIO_SAMPLE_RATE;
        hdr->byteRate = AUDIO_SAMPLE_RATE * sizeof(int16_t);
        hdr->blockAlign = sizeof(int16_t);
        hdr->bitsPerSample = 16;
        memcpy(hdr->data, "data", 4);
        hdr->dataSize = (uint32_t)dataSize;

        memcpy(wavBytes + sizeof(SfxWavHeader), g_SfxTable[k].pcmSamples, dataSize);

        g_SfxTable[k].wavBytes = wavBytes;
        g_SfxTable[k].wavSize = totalWavSize;
    }
}

// ============================================================================
// Procedural Background Music (BGM) Engine
// ============================================================================
// Multi-track dynamic synthesis with smooth crossfading
// Tracks supported:
//   1. BGM_MENU: Gentle, welcoming, ethereal melody (peaceful piano/chime vibe)
//   2. BGM_DAY: Lively, bright overworld exploration theme (pastoral chiptune/bells, upbeat)
//   3. BGM_NIGHT: Quiet, mysterious night ambiance (calm minor pentatonic synth with soft harmonics)
//   4. BGM_CAVERN: Deep subterranean cavern theme (slow resonant echoes, crystal marimba tones)
//   5. BGM_UNDERWORLD: Ominous, pulsating deep underworld/magma rhythm (slow deep bass pulse & drone)
// ============================================================================

typedef enum {
    BGM_NONE = 0,
    BGM_MENU,
    BGM_DAY,
    BGM_NIGHT,
    BGM_CAVERN,
    BGM_UNDERWORLD,
    BGM_TRACK_COUNT
} BgmTrackId;

#define BGM_MAX_VOICES         3
#define BGM_PATTERN_STEPS      64
#define BGM_CROSSFADE_SECONDS  2.5f
#define CAVERN_ECHO_SAMPLES    4851

#define BGM_NOTE_REST 0

#define BGM_NOTE_C1  24
#define BGM_NOTE_CS1 25
#define BGM_NOTE_D1  26
#define BGM_NOTE_DS1 27
#define BGM_NOTE_E1  28
#define BGM_NOTE_F1  29
#define BGM_NOTE_FS1 30
#define BGM_NOTE_G1  31
#define BGM_NOTE_GS1 32
#define BGM_NOTE_A1  33
#define BGM_NOTE_AS1 34
#define BGM_NOTE_B1  35

#define BGM_NOTE_C2  36
#define BGM_NOTE_CS2 37
#define BGM_NOTE_D2  38
#define BGM_NOTE_DS2 39
#define BGM_NOTE_E2  40
#define BGM_NOTE_F2  41
#define BGM_NOTE_FS2 42
#define BGM_NOTE_G2  43
#define BGM_NOTE_GS2 44
#define BGM_NOTE_A2  45
#define BGM_NOTE_AS2 46
#define BGM_NOTE_B2  47

#define BGM_NOTE_C3  48
#define BGM_NOTE_CS3 49
#define BGM_NOTE_D3  50
#define BGM_NOTE_DS3 51
#define BGM_NOTE_E3  52
#define BGM_NOTE_F3  53
#define BGM_NOTE_FS3 54
#define BGM_NOTE_G3  55
#define BGM_NOTE_GS3 56
#define BGM_NOTE_A3  57
#define BGM_NOTE_AS3 58
#define BGM_NOTE_B3  59

#define BGM_NOTE_C4  60
#define BGM_NOTE_CS4 61
#define BGM_NOTE_D4  62
#define BGM_NOTE_DS4 63
#define BGM_NOTE_E4  64
#define BGM_NOTE_F4  65
#define BGM_NOTE_FS4 66
#define BGM_NOTE_G4  67
#define BGM_NOTE_GS4 68
#define BGM_NOTE_A4  69
#define BGM_NOTE_AS4 70
#define BGM_NOTE_B4  71

#define BGM_NOTE_C5  72
#define BGM_NOTE_CS5 73
#define BGM_NOTE_D5  74
#define BGM_NOTE_DS5 75
#define BGM_NOTE_E5  76
#define BGM_NOTE_F5  77
#define BGM_NOTE_FS5 78
#define BGM_NOTE_G5  79
#define BGM_NOTE_GS5 80
#define BGM_NOTE_A5  81
#define BGM_NOTE_AS5 82
#define BGM_NOTE_B5  83

#define BGM_NOTE_C6  84
#define BGM_NOTE_CS6 85
#define BGM_NOTE_D6  86
#define BGM_NOTE_DS6 87
#define BGM_NOTE_E6  88
#define BGM_NOTE_F6  89
#define BGM_NOTE_FS6 90
#define BGM_NOTE_G6  91
#define BGM_NOTE_GS6 92
#define BGM_NOTE_A6  93
#define BGM_NOTE_AS6 94
#define BGM_NOTE_B6  95
#define BGM_NOTE_C7  96

// MIDI note frequencies (precomputed)
static float g_MidiFreq[128];
static bool  g_MidiFreqInitialized = false;

static void initMidiFreqTable(void) {
    if (g_MidiFreqInitialized) return;
    g_MidiFreq[0] = 0.0f;
    for (int i = 1; i < 128; i++) {
        g_MidiFreq[i] = 440.0f * powf(2.0f, (float)(i - 69) / 12.0f);
    }
    g_MidiFreqInitialized = true;
}

// ----------------------------------------------------------------------------
// Musical Pattern Sequencer Tables (64 steps = 4 bars in 4/4)
// ----------------------------------------------------------------------------

// 1. BGM_MENU (Key: G Major, peaceful Rhodes / chime arpeggio & pad)
static const uint8_t g_Pattern_Menu_V0[BGM_PATTERN_STEPS] = {
    // Bar 1 (G)
    BGM_NOTE_G4, 0, BGM_NOTE_B4, 0, BGM_NOTE_D5, 0, BGM_NOTE_G5, 0,
    BGM_NOTE_D5, 0, BGM_NOTE_B4, 0, BGM_NOTE_A4, 0, BGM_NOTE_G4, 0,
    // Bar 2 (Em7)
    BGM_NOTE_E4, 0, BGM_NOTE_G4, 0, BGM_NOTE_B4, 0, BGM_NOTE_E5, 0,
    BGM_NOTE_D5, 0, BGM_NOTE_B4, 0, BGM_NOTE_G4, 0, BGM_NOTE_A4, 0,
    // Bar 3 (Cadd9)
    BGM_NOTE_C4, 0, BGM_NOTE_E4, 0, BGM_NOTE_G4, 0, BGM_NOTE_D5, 0,
    BGM_NOTE_C5, 0, BGM_NOTE_G4, 0, BGM_NOTE_E4, 0, BGM_NOTE_G4, 0,
    // Bar 4 (D -> D7)
    BGM_NOTE_D4, 0, BGM_NOTE_FS4, 0, BGM_NOTE_A4, 0, BGM_NOTE_D5, 0,
    BGM_NOTE_C5, 0, BGM_NOTE_B4, 0, BGM_NOTE_A4, 0, BGM_NOTE_FS4, 0
};

static const uint8_t g_Pattern_Menu_V1[BGM_PATTERN_STEPS] = {
    // Bar 1 (G)
    BGM_NOTE_G2, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_D3, 0, 0, 0, 0, 0, 0, 0,
    // Bar 2 (Em)
    BGM_NOTE_E2, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_B2, 0, 0, 0, 0, 0, 0, 0,
    // Bar 3 (C)
    BGM_NOTE_C2, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_G2, 0, 0, 0, 0, 0, 0, 0,
    // Bar 4 (D)
    BGM_NOTE_D2, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_A2, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t g_Pattern_Menu_V2[BGM_PATTERN_STEPS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_B5, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_G5, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_E5, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_D6, 0
};

// 2. BGM_DAY (Key: C Major, pastoral chiptune flute & walking bass)
static const uint8_t g_Pattern_Day_V0[BGM_PATTERN_STEPS] = {
    // Bar 1 (C)
    BGM_NOTE_G4, 0, BGM_NOTE_C5, 0, BGM_NOTE_D5, 0, BGM_NOTE_E5, 0,
    BGM_NOTE_G5, 0, BGM_NOTE_E5, 0, BGM_NOTE_D5, 0, BGM_NOTE_C5, 0,
    // Bar 2 (Am)
    BGM_NOTE_A4, 0, BGM_NOTE_C5, 0, BGM_NOTE_E5, 0, BGM_NOTE_A5, 0,
    BGM_NOTE_G5, 0, BGM_NOTE_E5, 0, BGM_NOTE_D5, 0, BGM_NOTE_E5, 0,
    // Bar 3 (F)
    BGM_NOTE_F4, 0, BGM_NOTE_A4, 0, BGM_NOTE_C5, 0, BGM_NOTE_F5, 0,
    BGM_NOTE_E5, 0, BGM_NOTE_C5, 0, BGM_NOTE_A4, 0, BGM_NOTE_C5, 0,
    // Bar 4 (G -> C)
    BGM_NOTE_D5, 0, BGM_NOTE_E5, 0, BGM_NOTE_D5, 0, BGM_NOTE_B4, 0,
    BGM_NOTE_C5, 0, 0, 0, BGM_NOTE_C5, 0, 0, 0
};

static const uint8_t g_Pattern_Day_V1[BGM_PATTERN_STEPS] = {
    // Bar 1
    BGM_NOTE_C3, 0, 0, 0, BGM_NOTE_G2, 0, 0, 0, BGM_NOTE_C3, 0, 0, 0, BGM_NOTE_E3, 0, 0, 0,
    // Bar 2
    BGM_NOTE_A2, 0, 0, 0, BGM_NOTE_E2, 0, 0, 0, BGM_NOTE_A2, 0, 0, 0, BGM_NOTE_C3, 0, 0, 0,
    // Bar 3
    BGM_NOTE_F2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_F2, 0, 0, 0, BGM_NOTE_A2, 0, 0, 0,
    // Bar 4
    BGM_NOTE_G2, 0, 0, 0, BGM_NOTE_D2, 0, 0, 0, BGM_NOTE_G2, 0, 0, 0, BGM_NOTE_B2, 0, 0, 0
};

static const uint8_t g_Pattern_Day_V2[BGM_PATTERN_STEPS] = {
    // Offbeat shaker/chiptune tick
    0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
};

// 3. BGM_NIGHT (Key: E Minor Pentatonic, ocarina & desert harmonic drone)
static const uint8_t g_Pattern_Night_V0[BGM_PATTERN_STEPS] = {
    // Bar 1
    BGM_NOTE_E5, 0, 0, 0, 0, 0, BGM_NOTE_G5, 0, 0, 0, BGM_NOTE_A5, 0, 0, 0, 0, 0,
    // Bar 2
    BGM_NOTE_B5, 0, 0, 0, 0, 0, BGM_NOTE_D6, 0, 0, 0, BGM_NOTE_B5, 0, BGM_NOTE_A5, 0, 0, 0,
    // Bar 3
    BGM_NOTE_G5, 0, 0, 0, BGM_NOTE_E5, 0, 0, 0, 0, 0, BGM_NOTE_D5, 0, 0, 0, 0, 0,
    // Bar 4
    BGM_NOTE_E5, 0, 0, 0, 0, 0, BGM_NOTE_G5, 0, 0, 0, BGM_NOTE_E5, 0, 0, 0, 0, 0
};

static const uint8_t g_Pattern_Night_V1[BGM_PATTERN_STEPS] = {
    // Bar 1 & 2
    BGM_NOTE_E2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_B2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Bar 3 & 4
    BGM_NOTE_A2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_E2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t g_Pattern_Night_V2[BGM_PATTERN_STEPS] = {
    0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_E6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_B6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_G6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_D6, 0, 0, 0, 0, 0, 0, 0
};

// 4. BGM_CAVERN (Key: D Minor, crystal marimba with echoes & sub drone)
static const uint8_t g_Pattern_Cavern_V0[BGM_PATTERN_STEPS] = {
    // Bar 1
    BGM_NOTE_D5, 0, 0, 0, BGM_NOTE_F5, 0, 0, 0, BGM_NOTE_A5, 0, 0, 0, 0, 0, 0, 0,
    // Bar 2
    BGM_NOTE_C6, 0, 0, 0, BGM_NOTE_A5, 0, 0, 0, BGM_NOTE_F5, 0, 0, 0, BGM_NOTE_D5, 0, 0, 0,
    // Bar 3
    BGM_NOTE_AS4, 0, 0, 0, BGM_NOTE_D5, 0, 0, 0, BGM_NOTE_F5, 0, 0, 0, 0, 0, 0, 0,
    // Bar 4
    BGM_NOTE_G5, 0, 0, 0, BGM_NOTE_E5, 0, 0, 0, BGM_NOTE_CS5, 0, 0, 0, BGM_NOTE_D5, 0, 0, 0
};

static const uint8_t g_Pattern_Cavern_V1[BGM_PATTERN_STEPS] = {
    // Bar 1 & 2
    BGM_NOTE_D2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_A1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Bar 3 & 4
    BGM_NOTE_AS1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_F1, 0, 0, 0,
    BGM_NOTE_A1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_E1, 0, 0, 0
};

static const uint8_t g_Pattern_Cavern_V2[BGM_PATTERN_STEPS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_A5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_F6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_D6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BGM_NOTE_CS6, 0, 0, 0
};

// 5. BGM_UNDERWORLD (Key: Low C / Tritone, heavy magma heartbeat & ominous drone)
static const uint8_t g_Pattern_Underworld_V0[BGM_PATTERN_STEPS] = {
    // Bar 1
    BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_DS2, 0, 0, 0,
    // Bar 2 (tritone tension)
    BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_FS2, 0, 0, 0,
    // Bar 3
    BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_G2, 0, 0, 0,
    // Bar 4
    BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_C2, 0, 0, 0, BGM_NOTE_GS2, 0, 0, 0, BGM_NOTE_G2, 0, 0, 0
};

static const uint8_t g_Pattern_Underworld_V1[BGM_PATTERN_STEPS] = {
    // Bar 1 & 2
    BGM_NOTE_C1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_G1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // Bar 3 & 4
    BGM_NOTE_GS1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_FS1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t g_Pattern_Underworld_V2[BGM_PATTERN_STEPS] = {
    BGM_NOTE_C2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_C2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BGM_NOTE_DS2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// ----------------------------------------------------------------------------
// Voice and Track State Structures
// ----------------------------------------------------------------------------
typedef struct {
    float   phase;
    float   phase2;
    float   noteTime;
    uint8_t currentNote;
} BgmVoiceState;

typedef struct {
    int           stepIndex;
    int           stepSamples;
    int           samplesPerStep;
    int           totalSteps;
    BgmVoiceState voices[BGM_MAX_VOICES];
} BgmTrackState;

// BGM Engine Internal Variables
static BgmTrackState g_BgmTrackStates[BGM_TRACK_COUNT];
static float         g_BgmGains[BGM_TRACK_COUNT];
static BgmTrackId    g_TargetBgmTrack = BGM_NONE;
static float         g_BgmMasterVolume = 0.28f;
static float         g_CavernEchoBuf[CAVERN_ECHO_SAMPLES];
static int           g_CavernEchoCursor = 0;

// ----------------------------------------------------------------------------
// Procedural Synthesis Implementations for Each Biome Voice
// ----------------------------------------------------------------------------

static inline float synthMenuSample(int voiceIndex, BgmVoiceState* v, float dt) {
    if (v->currentNote == 0) return 0.0f;
    float freq = g_MidiFreq[v->currentNote];
    if (freq <= 0.0f) return 0.0f;

    float t = v->noteTime;
    v->noteTime += dt;
    v->phase += 2.0f * (float)M_PI * freq * dt;
    if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

    if (voiceIndex == 0) {
        // Voice 0: Electric Piano / Celesta Chime (2-op FM bell)
        float env = (t < 0.008f) ? (t / 0.008f) : expf(-3.5f * t);
        if (env < 0.0001f) return 0.0f;
        float fmMod = 0.35f * expf(-8.0f * t) * sinf(v->phase * 2.0f);
        float wave = sinf(v->phase + fmMod) * 0.85f + sinf(v->phase * 3.0f) * 0.15f;
        return wave * env * 0.32f;
    } else if (voiceIndex == 1) {
        // Voice 1: Warm Pad Drone
        float env = (t < 0.15f) ? (t / 0.15f) : expf(-0.45f * t);
        if (env < 0.0001f) return 0.0f;
        float tremolo = 1.0f + 0.06f * sinf(2.0f * (float)M_PI * 0.25f * t);
        float wave = sinf(v->phase) * 0.65f + sinf(v->phase * 2.0f) * 0.35f;
        return wave * env * tremolo * 0.20f;
    } else {
        // Voice 2: High Ethereal Bell
        float env = (t < 0.005f) ? (t / 0.005f) : expf(-4.2f * t);
        if (env < 0.0001f) return 0.0f;
        return sinf(v->phase) * env * 0.14f;
    }
}

static inline float synthDaySample(int voiceIndex, BgmVoiceState* v, float dt) {
    if (voiceIndex == 2) {
        // Voice 2: Light Chiptune Percussion (tick)
        if (v->currentNote == 0) return 0.0f;
        float t = v->noteTime;
        v->noteTime += dt;
        float env = expf(-75.0f * t);
        if (env < 0.0001f) return 0.0f;
        return sfxRandomFloat() * env * 0.08f;
    }

    if (v->currentNote == 0) return 0.0f;
    float freq = g_MidiFreq[v->currentNote];
    if (freq <= 0.0f) return 0.0f;

    float t = v->noteTime;
    v->noteTime += dt;
    v->phase += 2.0f * (float)M_PI * freq * dt;
    if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

    if (voiceIndex == 0) {
        // Voice 0: Pastoral Flute / Bright Melody
        float env = (t < 0.012f) ? (t / 0.012f) : expf(-4.8f * t);
        if (env < 0.0001f) return 0.0f;
        float wave = sinf(v->phase) * 0.70f + sinf(v->phase * 2.0f) * 0.20f + sinf(v->phase * 3.0f) * 0.10f;
        return wave * env * 0.30f;
    } else {
        // Voice 1: Bouncy Walking Bass (Triangle)
        float env = (t < 0.006f) ? (t / 0.006f) : expf(-6.2f * t);
        if (env < 0.0001f) return 0.0f;
        float tNorm = fmodf(v->phase / (2.0f * (float)M_PI), 1.0f);
        if (tNorm < 0.0f) tNorm += 1.0f;
        float tri = (tNorm < 0.5f) ? (4.0f * tNorm - 1.0f) : (3.0f - 4.0f * tNorm);
        return tri * env * 0.26f;
    }
}

static inline float synthNightSample(int voiceIndex, BgmVoiceState* v, float dt) {
    if (v->currentNote == 0) return 0.0f;
    float freq = g_MidiFreq[v->currentNote];
    if (freq <= 0.0f) return 0.0f;

    float t = v->noteTime;
    v->noteTime += dt;

    if (voiceIndex == 0) {
        // Voice 0: Contemplative Ocarina / Night Whistle
        float vib = 1.0f + 0.012f * sinf(2.0f * (float)M_PI * 4.8f * t);
        v->phase += 2.0f * (float)M_PI * (freq * vib) * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = (t < 0.045f) ? (t / 0.045f) : expf(-1.6f * t);
        if (env < 0.0001f) return 0.0f;
        float wave = sinf(v->phase) * 0.82f + sinf(v->phase * 2.0f) * 0.18f;
        return wave * env * 0.28f;
    } else if (voiceIndex == 1) {
        // Voice 1: Desert Harmonic Drone (Detuned dual sine)
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;
        v->phase2 += 2.0f * (float)M_PI * (freq * 1.004f) * dt;
        if (v->phase2 >= 2.0f * (float)M_PI) v->phase2 -= 2.0f * (float)M_PI;

        float env = (t < 0.18f) ? (t / 0.18f) : expf(-0.35f * t);
        if (env < 0.0001f) return 0.0f;
        float wave = 0.5f * sinf(v->phase) + 0.5f * sinf(v->phase2);
        return wave * env * 0.18f;
    } else {
        // Voice 2: Star Twinkle / Wind Chime
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = (t < 0.005f) ? (t / 0.005f) : expf(-3.5f * t);
        if (env < 0.0001f) return 0.0f;
        return sinf(v->phase) * env * 0.12f;
    }
}

static inline float synthCavernSample(int voiceIndex, BgmVoiceState* v, float dt) {
    if (v->currentNote == 0) return 0.0f;
    float freq = g_MidiFreq[v->currentNote];
    if (freq <= 0.0f) return 0.0f;

    float t = v->noteTime;
    v->noteTime += dt;

    if (voiceIndex == 0) {
        // Voice 0: Crystal Marimba with Subterranean Reverb Echo
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = (t < 0.004f) ? (t / 0.004f) : expf(-4.8f * t);
        float dry = (sinf(v->phase) * 0.75f + sinf(v->phase * 3.0f) * 0.25f) * env * 0.28f;

        float delayed = g_CavernEchoBuf[g_CavernEchoCursor];
        g_CavernEchoBuf[g_CavernEchoCursor] = dry + delayed * 0.40f;
        g_CavernEchoCursor++;
        if (g_CavernEchoCursor >= CAVERN_ECHO_SAMPLES) g_CavernEchoCursor = 0;

        return dry + delayed * 0.48f;
    } else if (voiceIndex == 1) {
        // Voice 1: Deep Cavern Sub Drone
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = (t < 0.12f) ? (t / 0.12f) : expf(-0.35f * t);
        if (env < 0.0001f) return 0.0f;
        return sinf(v->phase) * env * 0.22f;
    } else {
        // Voice 2: Cave Water Droplet / Mineral Ping
        float dropFreq = freq * (1.0f + 0.22f * expf(-45.0f * t));
        v->phase += 2.0f * (float)M_PI * dropFreq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = expf(-18.0f * t);
        if (env < 0.0001f) return 0.0f;
        return sinf(v->phase) * env * 0.15f;
    }
}

static inline float synthUnderworldSample(int voiceIndex, BgmVoiceState* v, float dt) {
    if (v->currentNote == 0) return 0.0f;
    float freq = g_MidiFreq[v->currentNote];
    if (freq <= 0.0f) return 0.0f;

    float t = v->noteTime;
    v->noteTime += dt;

    if (voiceIndex == 0) {
        // Voice 0: Pulsating Magma Heartbeat Bass (Overdrive Sub-bass)
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = (t < 0.012f) ? (t / 0.012f) : expf(-5.2f * t);
        if (env < 0.0001f) return 0.0f;
        float s = sinf(v->phase);
        s = 1.4f * s - 0.4f * s * s * s;
        return s * env * 0.32f;
    } else if (voiceIndex == 1) {
        // Voice 1: Volcanic Low Drone (Low detuned rumble)
        v->phase += 2.0f * (float)M_PI * freq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;
        v->phase2 += 2.0f * (float)M_PI * (freq * 1.006f) * dt;
        if (v->phase2 >= 2.0f * (float)M_PI) v->phase2 -= 2.0f * (float)M_PI;

        float tremolo = 1.0f + 0.15f * sinf(2.0f * (float)M_PI * 0.75f * t);
        float env = (t < 0.20f) ? (t / 0.20f) : expf(-0.25f * t);
        if (env < 0.0001f) return 0.0f;
        float s1 = sinf(v->phase);
        float s2 = sinf(v->phase2);
        float wave = 0.5f * s1 + 0.5f * s2;
        return wave * env * tremolo * 0.20f;
    } else {
        // Voice 2: Magma Impact / Flare (Deep frequency sweep thud + rumble)
        float sweepFreq = 42.0f + 85.0f * expf(-20.0f * t);
        v->phase += 2.0f * (float)M_PI * sweepFreq * dt;
        if (v->phase >= 2.0f * (float)M_PI) v->phase -= 2.0f * (float)M_PI;

        float env = expf(-6.0f * t);
        if (env < 0.0001f) return 0.0f;
        float impact = sinf(v->phase) * 0.75f + sfxRandomFloat() * 0.25f;
        return impact * env * 0.22f;
    }
}

// ----------------------------------------------------------------------------
// Step Sequencer Advancement and Real-time Audio Generation
// ----------------------------------------------------------------------------

static inline void advanceBgmStep(BgmTrackId t) {
    if (t <= BGM_NONE || t >= BGM_TRACK_COUNT) return;
    BgmTrackState* ts = &g_BgmTrackStates[t];

    for (int v = 0; v < BGM_MAX_VOICES; v++) {
        uint8_t note = 0;
        switch (t) {
            case BGM_MENU:
                note = (v == 0) ? g_Pattern_Menu_V0[ts->stepIndex] :
                       ((v == 1) ? g_Pattern_Menu_V1[ts->stepIndex] : g_Pattern_Menu_V2[ts->stepIndex]);
                break;
            case BGM_DAY:
                note = (v == 0) ? g_Pattern_Day_V0[ts->stepIndex] :
                       ((v == 1) ? g_Pattern_Day_V1[ts->stepIndex] : g_Pattern_Day_V2[ts->stepIndex]);
                break;
            case BGM_NIGHT:
                note = (v == 0) ? g_Pattern_Night_V0[ts->stepIndex] :
                       ((v == 1) ? g_Pattern_Night_V1[ts->stepIndex] : g_Pattern_Night_V2[ts->stepIndex]);
                break;
            case BGM_CAVERN:
                note = (v == 0) ? g_Pattern_Cavern_V0[ts->stepIndex] :
                       ((v == 1) ? g_Pattern_Cavern_V1[ts->stepIndex] : g_Pattern_Cavern_V2[ts->stepIndex]);
                break;
            case BGM_UNDERWORLD:
                note = (v == 0) ? g_Pattern_Underworld_V0[ts->stepIndex] :
                       ((v == 1) ? g_Pattern_Underworld_V1[ts->stepIndex] : g_Pattern_Underworld_V2[ts->stepIndex]);
                break;
            default:
                break;
        }
        if (note != 0) {
            ts->voices[v].currentNote = note;
            ts->voices[v].noteTime = 0.0f;
        }
    }

    ts->stepIndex = (ts->stepIndex + 1) % ts->totalSteps;
}

static inline float synthBgmSample(void) {
    if (!g_SoundEnabled) return 0.0f;

    float dt = 1.0f / (float)AUDIO_SAMPLE_RATE;
    float crossfadeRate = 1.0f / (BGM_CROSSFADE_SECONDS * (float)AUDIO_SAMPLE_RATE);
    float totalBgm = 0.0f;

    for (int t = 1; t < BGM_TRACK_COUNT; t++) {
        float target = (t == (int)g_TargetBgmTrack) ? 1.0f : 0.0f;
        if (g_BgmGains[t] < target) {
            g_BgmGains[t] += crossfadeRate;
            if (g_BgmGains[t] > target) g_BgmGains[t] = target;
        } else if (g_BgmGains[t] > target) {
            g_BgmGains[t] -= crossfadeRate;
            if (g_BgmGains[t] < target) g_BgmGains[t] = target;
        }

        if (g_BgmGains[t] <= 0.00001f) {
            continue;
        }

        BgmTrackState* ts = &g_BgmTrackStates[t];
        if (ts->stepSamples >= ts->samplesPerStep) {
            ts->stepSamples = 0;
            advanceBgmStep((BgmTrackId)t);
        }
        ts->stepSamples++;

        float trackAudio = 0.0f;
        for (int v = 0; v < BGM_MAX_VOICES; v++) {
            switch (t) {
                case BGM_MENU:
                    trackAudio += synthMenuSample(v, &ts->voices[v], dt);
                    break;
                case BGM_DAY:
                    trackAudio += synthDaySample(v, &ts->voices[v], dt);
                    break;
                case BGM_NIGHT:
                    trackAudio += synthNightSample(v, &ts->voices[v], dt);
                    break;
                case BGM_CAVERN:
                    trackAudio += synthCavernSample(v, &ts->voices[v], dt);
                    break;
                case BGM_UNDERWORLD:
                    trackAudio += synthUnderworldSample(v, &ts->voices[v], dt);
                    break;
                default:
                    break;
            }
        }

        // Equal-power sinusoidal crossfade curve
        float effGain = sinf(g_BgmGains[t] * 1.57079632679f);
        totalBgm += trackAudio * effGain;
    }

    return totalBgm * (g_BgmMasterVolume * 32767.0f);
}

// ----------------------------------------------------------------------------
// BGM Public API Functions
// ----------------------------------------------------------------------------

static inline void setBgmTrack(BgmTrackId track) {
    if (track < 0 || track >= BGM_TRACK_COUNT) return;
    if (track == g_TargetBgmTrack) return;

    if (g_SoundEngineInitialized) {
        EnterCriticalSection(&g_AudioLock);
    }

    if (track != BGM_NONE && g_BgmGains[track] < 0.01f) {
        g_BgmTrackStates[track].stepIndex = 0;
        g_BgmTrackStates[track].stepSamples = g_BgmTrackStates[track].samplesPerStep;
        for (int v = 0; v < BGM_MAX_VOICES; v++) {
            g_BgmTrackStates[track].voices[v].noteTime = 999.0f;
            g_BgmTrackStates[track].voices[v].currentNote = 0;
        }
    }

    g_TargetBgmTrack = track;

    if (g_SoundEngineInitialized) {
        LeaveCriticalSection(&g_AudioLock);
    }
}

static inline BgmTrackId getBgmTrack(void) {
    return g_TargetBgmTrack;
}

static inline void setBgmVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    g_BgmMasterVolume = vol;
}

static inline float getBgmVolume(void) {
    return g_BgmMasterVolume;
}

static inline void updateBgmContext(bool inMenu, float playerY, float surfaceY, bool isDay, int biome) {
    if (inMenu) {
        setBgmTrack(BGM_MENU);
        return;
    }

    // In-game dynamic biome/scene progression:
    // 1. Underworld: deep bedrock magma layer (y <= 24.0f)
    if (playerY <= 24.0f) {
        setBgmTrack(BGM_UNDERWORLD);
    }
    // 2. Cavern: subterranean cave network (y < 55.0f or deeper than 6 blocks below surface)
    else if (playerY < 55.0f || playerY < surfaceY - 6.0f) {
        setBgmTrack(BGM_CAVERN);
    }
    // 3. Surface Night / Desert: calm pentatonic night ambiance
    else if (biome == 0 /* BIOME_DESERT */ || !isDay) {
        setBgmTrack(BGM_NIGHT);
    }
    // 4. Surface Day (Forest, Tundra): upbeat adventurous exploration
    else {
        setBgmTrack(BGM_DAY);
    }
}

static inline void initBgmSystem(void) {
    initMidiFreqTable();
    memset(g_BgmTrackStates, 0, sizeof(g_BgmTrackStates));
    memset(g_BgmGains, 0, sizeof(g_BgmGains));
    memset(g_CavernEchoBuf, 0, sizeof(g_CavernEchoBuf));
    g_CavernEchoCursor = 0;

    // Track tempos:
    // BGM_MENU: 82 BPM
    g_BgmTrackStates[BGM_MENU].samplesPerStep = (int)(AUDIO_SAMPLE_RATE * 60.0f / (82.0f * 4.0f));
    g_BgmTrackStates[BGM_MENU].totalSteps = BGM_PATTERN_STEPS;
    g_BgmTrackStates[BGM_MENU].stepSamples = g_BgmTrackStates[BGM_MENU].samplesPerStep;

    // BGM_DAY: 105 BPM
    g_BgmTrackStates[BGM_DAY].samplesPerStep = (int)(AUDIO_SAMPLE_RATE * 60.0f / (105.0f * 4.0f));
    g_BgmTrackStates[BGM_DAY].totalSteps = BGM_PATTERN_STEPS;
    g_BgmTrackStates[BGM_DAY].stepSamples = g_BgmTrackStates[BGM_DAY].samplesPerStep;

    // BGM_NIGHT: 76 BPM
    g_BgmTrackStates[BGM_NIGHT].samplesPerStep = (int)(AUDIO_SAMPLE_RATE * 60.0f / (76.0f * 4.0f));
    g_BgmTrackStates[BGM_NIGHT].totalSteps = BGM_PATTERN_STEPS;
    g_BgmTrackStates[BGM_NIGHT].stepSamples = g_BgmTrackStates[BGM_NIGHT].samplesPerStep;

    // BGM_CAVERN: 68 BPM
    g_BgmTrackStates[BGM_CAVERN].samplesPerStep = (int)(AUDIO_SAMPLE_RATE * 60.0f / (68.0f * 4.0f));
    g_BgmTrackStates[BGM_CAVERN].totalSteps = BGM_PATTERN_STEPS;
    g_BgmTrackStates[BGM_CAVERN].stepSamples = g_BgmTrackStates[BGM_CAVERN].samplesPerStep;

    // BGM_UNDERWORLD: 86 BPM
    g_BgmTrackStates[BGM_UNDERWORLD].samplesPerStep = (int)(AUDIO_SAMPLE_RATE * 60.0f / (86.0f * 4.0f));
    g_BgmTrackStates[BGM_UNDERWORLD].totalSteps = BGM_PATTERN_STEPS;
    g_BgmTrackStates[BGM_UNDERWORLD].stepSamples = g_BgmTrackStates[BGM_UNDERWORLD].samplesPerStep;

    for (int t = 0; t < BGM_TRACK_COUNT; t++) {
        for (int v = 0; v < BGM_MAX_VOICES; v++) {
            g_BgmTrackStates[t].voices[v].noteTime = 999.0f;
        }
    }

    g_TargetBgmTrack = BGM_MENU;
    g_BgmMasterVolume = 0.28f;
}

// ============================================================================
// Multi-Voice Audio Mixer Thread
// ============================================================================
static void mixAudioChunk(int16_t* outBuffer, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        // 1. Procedural Background Music (BGM)
        float bgmVal = synthBgmSample();

        // 2. Sound Effects (SFX)
        int32_t sfxMix = 0;
        for (int v = 0; v < MAX_AUDIO_VOICES; v++) {
            if (!g_AudioVoices[v].active) continue;
            int id = g_AudioVoices[v].sfxId;
            int cur = g_AudioVoices[v].cursor;
            if (id >= 0 && id < SFX_COUNT && cur < g_SfxTable[id].sampleCount) {
                sfxMix += (int32_t)(g_SfxTable[id].pcmSamples[cur] * g_AudioVoices[v].volume);
                g_AudioVoices[v].cursor++;
            } else {
                g_AudioVoices[v].active = false;
            }
        }

        // 3. Combined Output with Soft-Limiting / Saturation Clamp
        int32_t mixed = (int32_t)bgmVal + sfxMix;
        if (mixed > 32767) mixed = 32767;
        else if (mixed < -32768) mixed = -32768;
        outBuffer[i] = (int16_t)mixed;
    }
}

static DWORD WINAPI AudioThreadProc(LPVOID lpParam) {
    (void)lpParam;
    while (g_AudioRunning) {
        WaitForSingleObject(g_hAudioEvent, 25);
        if (!g_AudioRunning) break;

        for (int b = 0; b < AUDIO_NUM_BUFFERS; b++) {
            if (g_AudioWaveHdrs[b].dwFlags & WHDR_DONE) {
                waveOutUnprepareHeader(g_hWaveOut, &g_AudioWaveHdrs[b], sizeof(WAVEHDR));

                EnterCriticalSection(&g_AudioLock);
                mixAudioChunk(g_AudioBuffers[b], AUDIO_BUFFER_SAMPLES);
                LeaveCriticalSection(&g_AudioLock);

                g_AudioWaveHdrs[b].dwFlags = 0;
                waveOutPrepareHeader(g_hWaveOut, &g_AudioWaveHdrs[b], sizeof(WAVEHDR));
                waveOutWrite(g_hWaveOut, &g_AudioWaveHdrs[b], sizeof(WAVEHDR));
            }
        }
    }
    return 0;
}

// ============================================================================
// Public API Functions
// ============================================================================

// Initialize sound engine, synthesize sounds, and setup waveOut playback
static inline bool initSoundEngine(void) {
    if (g_SoundEngineInitialized) return true;

    InitializeCriticalSection(&g_AudioLock);
    synthesizeAllSounds();
    initBgmSystem();
    memset(g_AudioVoices, 0, sizeof(g_AudioVoices));

    // Configure standard PCM 22050 Hz 16-bit Mono format
    WAVEFORMATEX wfx;
    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = AUDIO_SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (WORD)(wfx.nChannels * (wfx.wBitsPerSample / 8));
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    g_hAudioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    MMRESULT res = waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)g_hAudioEvent, 0, CALLBACK_EVENT);

    if (res == MMSYSERR_NOERROR && g_hWaveOut != NULL) {
        g_AudioRunning = true;
        g_AudioMixerActive = true;

        for (int i = 0; i < AUDIO_NUM_BUFFERS; i++) {
            memset(g_AudioBuffers[i], 0, sizeof(g_AudioBuffers[i]));
            memset(&g_AudioWaveHdrs[i], 0, sizeof(WAVEHDR));
            g_AudioWaveHdrs[i].lpData = (LPSTR)g_AudioBuffers[i];
            g_AudioWaveHdrs[i].dwBufferLength = sizeof(g_AudioBuffers[i]);
            waveOutPrepareHeader(g_hWaveOut, &g_AudioWaveHdrs[i], sizeof(WAVEHDR));
            waveOutWrite(g_hWaveOut, &g_AudioWaveHdrs[i], sizeof(WAVEHDR));
        }

        g_hAudioThread = CreateThread(NULL, 0, AudioThreadProc, NULL, 0, NULL);
    } else {
        // Fallback to PlaySoundA if waveOut device cannot be opened
        g_AudioMixerActive = false;
        if (g_hAudioEvent) {
            CloseHandle(g_hAudioEvent);
            g_hAudioEvent = NULL;
        }
    }

    g_SoundEngineInitialized = true;
    return true;
}

// Play a sound effect by ID (polyphonic via waveOut, or asynchronous PlaySoundA)
static inline void playSound(int sfxId) {
    if (!g_SoundEnabled) return;
    if (!g_SoundEngineInitialized) {
        initSoundEngine();
    }
    if (sfxId < 0 || sfxId >= SFX_COUNT) return;

    if (g_AudioMixerActive) {
        EnterCriticalSection(&g_AudioLock);
        int target = -1;
        int maxCursor = -1;
        for (int i = 0; i < MAX_AUDIO_VOICES; i++) {
            if (!g_AudioVoices[i].active) {
                target = i;
                break;
            }
            if (g_AudioVoices[i].cursor > maxCursor) {
                maxCursor = g_AudioVoices[i].cursor;
                target = i;
            }
        }
        if (target >= 0) {
            g_AudioVoices[target].sfxId = sfxId;
            g_AudioVoices[target].cursor = 0;
            g_AudioVoices[target].volume = 0.85f;
            g_AudioVoices[target].active = true;
        }
        LeaveCriticalSection(&g_AudioLock);
    } else {
        // Fallback: in-memory asynchronous PlaySoundA
        if (g_SfxTable[sfxId].wavBytes != NULL) {
            PlaySoundA((LPCSTR)g_SfxTable[sfxId].wavBytes, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        }
    }
}

// Clean up sound engine resources, thread, and memory
static inline void cleanupSoundEngine(void) {
    if (!g_SoundEngineInitialized) return;

    if (g_AudioRunning) {
        g_AudioRunning = false;
        if (g_hAudioEvent) SetEvent(g_hAudioEvent);
        if (g_hAudioThread) {
            WaitForSingleObject(g_hAudioThread, 500);
            CloseHandle(g_hAudioThread);
            g_hAudioThread = NULL;
        }
    }

    if (g_hWaveOut) {
        waveOutReset(g_hWaveOut);
        for (int b = 0; b < AUDIO_NUM_BUFFERS; b++) {
            waveOutUnprepareHeader(g_hWaveOut, &g_AudioWaveHdrs[b], sizeof(WAVEHDR));
        }
        waveOutClose(g_hWaveOut);
        g_hWaveOut = NULL;
    }

    if (g_hAudioEvent) {
        CloseHandle(g_hAudioEvent);
        g_hAudioEvent = NULL;
    }

    DeleteCriticalSection(&g_AudioLock);

    for (int k = 0; k < SFX_COUNT; k++) {
        if (g_SfxTable[k].pcmSamples) {
            free(g_SfxTable[k].pcmSamples);
            g_SfxTable[k].pcmSamples = NULL;
        }
        if (g_SfxTable[k].wavBytes) {
            free(g_SfxTable[k].wavBytes);
            g_SfxTable[k].wavBytes = NULL;
        }
    }

    g_SoundEngineInitialized = false;
}

#endif // SOUND_ENGINE_H
