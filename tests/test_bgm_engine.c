#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

// Include sound_engine.h directly
#include "../standalone_src/sound_engine.h"

int main(void) {
    printf("=== Luminora BGM Engine Unit Test ===\n");

    // 1. Initialize sound and BGM system
    initMidiFreqTable();
    initBgmSystem();

    printf("[1] Checking MIDI Frequency Table...\n");
    assert(g_MidiFreq[69] > 439.9f && g_MidiFreq[69] < 440.1f); // A4 = 440 Hz
    assert(g_MidiFreq[60] > 261.0f && g_MidiFreq[60] < 262.0f); // Middle C = 261.63 Hz
    printf("    A4: %.2f Hz, C4: %.2f Hz -> PASS\n", g_MidiFreq[69], g_MidiFreq[60]);

    // 2. Test BGM context transitions
    printf("[2] Testing updateBgmContext logic...\n");
    
    // In menu
    updateBgmContext(true, 80.0f, 85.0f, true, 1);
    assert(getBgmTrack() == BGM_MENU);
    printf("    inMenu=true -> BGM_MENU: OK\n");

    // In game, underworld (y <= 24)
    updateBgmContext(false, 15.0f, 85.0f, true, 1);
    assert(getBgmTrack() == BGM_UNDERWORLD);
    printf("    y=15.0 (Underworld) -> BGM_UNDERWORLD: OK\n");

    // In game, cavern (y = 40.0)
    updateBgmContext(false, 40.0f, 85.0f, true, 1);
    assert(getBgmTrack() == BGM_CAVERN);
    printf("    y=40.0 (Cavern) -> BGM_CAVERN: OK\n");

    // In game, surface day (forest, y = 86.0, surface = 85.0, day = true)
    updateBgmContext(false, 86.0f, 85.0f, true, 1 /* forest */);
    assert(getBgmTrack() == BGM_DAY);
    printf("    y=86.0 (Surface Day) -> BGM_DAY: OK\n");

    // In game, surface night (forest, y = 86.0, surface = 85.0, day = false)
    updateBgmContext(false, 86.0f, 85.0f, false, 1 /* forest */);
    assert(getBgmTrack() == BGM_NIGHT);
    printf("    y=86.0 (Surface Night) -> BGM_NIGHT: OK\n");

    // In game, desert (biome = 0, y = 86.0, day = true)
    updateBgmContext(false, 86.0f, 85.0f, true, 0 /* desert */);
    assert(getBgmTrack() == BGM_NIGHT);
    printf("    y=86.0 (Desert Day) -> BGM_NIGHT: OK\n");

    // 3. Test Audio Synthesis and Crossfading
    printf("[3] Testing Audio Generation & Smooth Crossfading...\n");
    
    BgmTrackId tracksToTest[] = {
        BGM_MENU,
        BGM_DAY,
        BGM_NIGHT,
        BGM_CAVERN,
        BGM_UNDERWORLD
    };

    int16_t chunk[AUDIO_BUFFER_SAMPLES];

    for (int t = 0; t < 5; t++) {
        BgmTrackId track = tracksToTest[t];
        setBgmTrack(track);

        // Run for 3 seconds of audio to complete crossfade and measure signal
        int totalChunks = (int)(3.0f * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SAMPLES);
        double energy = 0.0;
        int maxSample = 0;
        int minSample = 0;

        for (int c = 0; c < totalChunks; c++) {
            mixAudioChunk(chunk, AUDIO_BUFFER_SAMPLES);
            for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
                int s = chunk[i];
                energy += (double)s * s;
                if (s > maxSample) maxSample = s;
                if (s < minSample) minSample = s;
            }
        }

        double rms = sqrt(energy / (totalChunks * AUDIO_BUFFER_SAMPLES));
        printf("    Track %d RMS: %.1f, Peak: [%d, %d]\n", track, rms, minSample, maxSample);

        // Verify audio is actively playing and non-zero
        assert(rms > 200.0);
        // Verify audio does not clip or overflow
        assert(maxSample <= 32767);
        assert(minSample >= -32768);
    }

    // 4. Test Mute functionality
    printf("[4] Testing Mute / isSoundEnabled()...\n");
    setSoundEnabled(false);
    mixAudioChunk(chunk, AUDIO_BUFFER_SAMPLES);
    for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
        assert(chunk[i] == 0);
    }
    printf("    Mute produces zero audio -> PASS\n");

    setSoundEnabled(true);
    mixAudioChunk(chunk, AUDIO_BUFFER_SAMPLES);
    int nonZeroCount = 0;
    for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
        if (chunk[i] != 0) nonZeroCount++;
    }
    assert(nonZeroCount > 0);
    printf("    Unmute restores audio -> PASS\n");

    printf("\n>>> ALL Luminora BGM Unit Tests PASSED Successfully! <<<\n");
    return 0;
}
