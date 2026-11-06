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
// Multi-Voice Audio Mixer Thread
// ============================================================================
static void mixAudioChunk(int16_t* outBuffer, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        int32_t mixed = 0;
        for (int v = 0; v < MAX_AUDIO_VOICES; v++) {
            if (!g_AudioVoices[v].active) continue;
            int id = g_AudioVoices[v].sfxId;
            int cur = g_AudioVoices[v].cursor;
            if (id >= 0 && id < SFX_COUNT && cur < g_SfxTable[id].sampleCount) {
                mixed += (int32_t)(g_SfxTable[id].pcmSamples[cur] * g_AudioVoices[v].volume);
                g_AudioVoices[v].cursor++;
            } else {
                g_AudioVoices[v].active = false;
            }
        }
        // Soft-limiting / saturation clamp
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
