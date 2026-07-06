#include <emscripten.h>
#include <math.h>
#include <stdint.h>

#define ENGINE_COUNT 20
#define MAX_TRACKS 1000
#define BLOCK_SIZE 128 // Standard Web Audio API block size

// --- ENGINE STATE ---
typedef struct {
    int active;
    int tracks;
    int patternN;
    float currentStep;
    int lfoActive;
    float lfoStart;
    float lfoEnd;
    float lfoTime;
    int lfoSamples;
} EngineConfig;

EngineConfig engines[ENGINE_COUNT];
float sampleRate = 48000.0f;
int globalSampleCounter = 0;

// --- GLOBAL GATE STATE ---
int inGateEnabled = 1;
int inGateSamples = 4800;
int acEnabled = 0;
int acBurst = 1;
int acSamples = 100;

// --- STATIC MEMORY BUFFERS (Zero Allocation DSP) ---
float in_buffer[BLOCK_SIZE];
float out_buffer[BLOCK_SIZE];

// --- EXPORTS: MEMORY POINTERS ---
EMSCRIPTEN_KEEPALIVE
float* get_in_buf_ptr() { return in_buffer; }

EMSCRIPTEN_KEEPALIVE
float* get_out_buf_ptr() { return out_buffer; }

// --- EXPORTS: CONFIGURATION ---
EMSCRIPTEN_KEEPALIVE
void init_sample_rate(float sr) { sampleRate = sr; }

EMSCRIPTEN_KEEPALIVE
void set_gate_config(int enabled, int samples) {
    inGateEnabled = enabled;
    inGateSamples = samples;
}

EMSCRIPTEN_KEEPALIVE
void set_ac_config(int enabled, int burst, int samples) {
    acEnabled = enabled;
    acBurst = burst;
    acSamples = samples;
}

EMSCRIPTEN_KEEPALIVE
void update_engine(int index, int active, int tracks, int patternN, 
                   int lfoActive, float lfoStart, float lfoEnd, float lfoTime) {
    if (index >= 0 && index < ENGINE_COUNT) {
        engines[index].active = active;
        engines[index].tracks = tracks;
        engines[index].patternN = patternN;
        engines[index].lfoActive = lfoActive;
        engines[index].lfoStart = lfoStart;
        engines[index].lfoEnd = lfoEnd;
        engines[index].lfoTime = lfoTime;
        engines[index].lfoSamples = 0;
        engines[index].currentStep = lfoStart;
    }
}

// --- MAIN DSP LOOP ---
EMSCRIPTEN_KEEPALIVE
void process_audio_block(int num_frames) {
    
    // 1. Update LFOs
    for (int e = 0; e < ENGINE_COUNT; e++) {
        EngineConfig* eng = &engines[e];
        if (eng->active && eng->lfoActive) {
            float totalSamples = eng->lfoTime * sampleRate;
            eng->lfoSamples += num_frames;
            if (eng->lfoSamples >= totalSamples) eng->lfoSamples = fmod(eng->lfoSamples, totalSamples);
            
            float progress = eng->lfoSamples / totalSamples;
            float high = fmax(0.000001f, eng->lfoStart);
            float low = fmax(0.000001f, eng->lfoEnd);
            eng->currentStep = high * powf(low / high, progress);
        }
    }

    // 2. Process Audio Samples
    for (int i = 0; i < num_frames; i++) {
        float val = in_buffer[i];
        int allowed = 1;

        // Math Gate
        if (inGateEnabled && ((globalSampleCounter / inGateSamples) % 2 != 0)) {
            allowed = 0;
        }

        // AuraConv Gate
        if (acEnabled) {
            int chunk = globalSampleCounter / acSamples;
            if (acBurst ? (chunk % 2 != 0) : (chunk % 2 == 0)) {
                allowed = 0;
            }
        }

        float processed_val = allowed ? val : 0.0f;
        float dsp_output = 0.0f;
        int active_engine_count = 0;

        // THE HEAVY MATH (20,000 iterations per sample)
        for (int e = 0; e < ENGINE_COUNT; e++) {
            EngineConfig eng = engines[e];
            if (!eng.active) continue;
            
            active_engine_count++;
            
            for (int t = 0; t < eng.tracks; t++) {
                float scaleMultiplier = (0.01f + (t * eng.currentStep)) / 100.0f;
                float patternPolarity = (eng.patternN > 0 && ((t + 1) % eng.patternN == 0)) ? -1.0f : 1.0f;
                
                dsp_output += (processed_val * scaleMultiplier * patternPolarity);
            }
        }

        // Mix dry mic with wet DSP output
        if (active_engine_count > 0) {
            // Soft-clip the massive accumulated math using tanhf to prevent harsh digital distortion
            float wet_signal = tanhf(dsp_output * 0.005f); 
            out_buffer[i] = (processed_val * 0.5f) + (wet_signal * 0.5f); 
        } else {
            out_buffer[i] = processed_val;
        }

        globalSampleCounter++;
    }
}
