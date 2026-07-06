# 🎛️ AuraScaler Wasm: 20-Engine Mathematical Scaling Array

[![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=flat-square&logo=webassembly&logoColor=white)](https://webassembly.org/)
[![Web Audio API](https://img.shields.io/badge/Web_Audio_API-API-blue?style=flat-square)](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://opensource.org/licenses/MIT)

**AuraScaler** (MicScaler) is a high-performance, browser-based Digital Signal Processing (DSP) environment. It utilizes a 20-engine mathematical multiplicative scaling array to generate complex, algorithmic audio transformations from live microphone input in real-time.

Originally built in pure JavaScript, the core DSP engine has been **migrated to C and compiled to WebAssembly (Wasm)**. This architectural shift eliminates JavaScript Garbage Collection (GC) pauses on the audio thread, ensuring glitch-free, studio-grade audio processing capable of executing over **960 million floating-point operations per second** directly in the browser.

---

## 📖 Overview & Architecture

Real-time audio processing in the browser is notoriously difficult due to the strict timing requirements of the Web Audio API (typically processing 128-sample blocks every ~2.6ms at 48kHz). 

AuraScaler solves this by adopting a **Hybrid JS/Wasm Architecture**:
1. **The UI & Routing (Main Thread):** JavaScript handles the DOM, Canvas visualizers (Oscilloscope/FFT), Web Audio API node graph routing, and user interactions.
2. **The DSP Core (Audio Thread):** An `AudioWorklet` fetches a compiled C binary (`dsp.wasm`). The heavy nested loops (20 engines × 1,000 tracks = 20,000 iterations per sample) are executed in native C code using static memory buffers, resulting in **zero memory allocation** and **zero GC pauses** during audio playback.

---

## ✨ Features

### Core DSP Engine
* **20-Engine Array:** 20 independent mathematical scaling engines running in parallel.
* **High-Density Tracking:** Up to 1,000 algorithmic tracks per engine (20,000 total concurrent math streams).
* **Pattern Polarity:** Configurable algorithmic polarity inversion (2nd, 3rd, 4th, 5th patterns) per engine.
* **Auto-Step LFOs:** Per-engine Low Frequency Oscillators with exponential/linear interpolation for dynamic step modulation.
* **Soft-Clipping:** Integrated `tanhf()` soft-clipping in the C-core to prevent harsh digital distortion when math streams accumulate.

### Routing & Effects
* **Pre-Scale Bandpass:** Configurable High-Pass and Low-Pass biquad filters before the math engine.
* **AuraConv Comb Filter:** Sample-accurate comb filtering with Burst/Gap gating modes.
* **Algorithmic Convolution Reverb:** Real-time convolution using generated impulse responses.
* **HRTF 3D Spatialization:** Automated 3D binaural rotation using the Web Audio PannerNode.

### Gating & Metering
* **Math Gate (Input):** Chops the data feeding *into* the DSP engines at configurable millisecond intervals.
* **Speaker Gate (Output):** Physically amplitude-modulates the final audio output stream.
* **Real-Time Analysis:** Dual FFT Spectral Distribution (Raw vs. Filtered) and Time-Domain Oscilloscope.
* **Precision dB Metering:** Throttled RMS dB metering for raw and post-gain signals.

### Recording & Export
* **Client-Side Encoding:** Record processed audio directly in the browser.
* **Multi-Format Support:** Export as uncompressed **WAV**, lossy **OPUS (WebM)**, or lossless **FLAC**.

---

## 🗺️ Roadmap

### Phase 1: Advanced Web Performance (Q3 2026)
- [ ] **WebAssembly SIMD:** Implement Single Instruction, Multiple Data (SIMD) instructions in the C code to vectorize the 20,000 track loops, potentially doubling performance on supported CPUs.
- [ ] **SharedArrayBuffer Integration:** Utilize COOP/COEP headers to share memory directly between the JS Audio Thread and Wasm, achieving true zero-copy audio buffer transfers.
- [ ] **OffscreenCanvas:** Move FFT and Oscilloscope rendering to a Web Worker to completely decouple UI rendering from the main thread.

### Phase 2: Audio Ecosystem Expansion (Q4 2026)
- [ ] **Custom Impulse Responses (IR):** Allow users to upload `.wav` files to replace the algorithmic reverb with real-world spatial IRs.
- [ ] **MIDI Controller Mapping:** Web MIDI API integration to map physical hardware knobs/faders to engine parameters and LFO rates.
- [ ] **Multi-Channel Output:** Support for 4-channel (Ambisonics) or 5.1 Surround Sound routing.

### Phase 3: Native Ports (2027)
- [ ] **Tauri Desktop App:** Wrap the web app in a lightweight Rust backend for native OS audio routing (ASIO/CoreAudio) and lower latency.
- [ ] **JUCE VST3/AU Plugin:** Port the C-core to the JUCE framework, allowing AuraScaler to run as a plugin inside DAWs like Ableton Live, Logic Pro, and FL Studio.

---

## 🛠️ Local Development & Compilation

Because the core DSP is written in C, you must compile it to WebAssembly before running the app locally.

### Prerequisites
1. Install the [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html).
2. Install a local static server (e.g., Node.js `npm install -g serve` or Python 3).

### 1. Compile the Wasm Binary
Open your terminal in the project root and run:
```bash
emcc dsp.c -O3 -s WASM=1 -s EXPORTED_FUNCTIONS='["_process_audio_block", "_update_engine", "_set_gate_config", "_set_ac_config", "_init_sample_rate", "_get_in_buf_ptr", "_get_out_buf_ptr"]' -o dsp.wasm
```
*Note: This generates `dsp.wasm`. The browser only needs this binary file, not the C source.*

### 2. Run Local Server
Browsers block microphone access and Wasm fetching on `file://` protocols. You must serve the files over HTTP/HTTPS.
```bash
# Using Python
python -m http.server 8000

# OR using Node.js
npx serve
```
Navigate to `http://localhost:8000` (or the port provided by serve) and click **Start Microphone**.

---

## 🚀 Deployment Guide

AuraScaler is designed to be hosted on any static site provider that supports HTTPS and correct MIME types for `.wasm` files.

### Option A: GitHub Pages (Automated via GitHub Actions)
You can configure GitHub to automatically compile your C code into Wasm every time you push to the repository, so you never have to commit binary files.

1. In your repository, create the file `.github/workflows/deploy.yml`.
2. Paste the following GitHub Action configuration:

```yaml
name: Build Wasm and Deploy to Pages

on:
  push:
    branches: ["main"]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout Repository
        uses: actions/checkout@v4

      - name: Setup Emscripten SDK
        uses: mymindstorm/setup-emsdk@v14

      - name: Compile C to WebAssembly
        run: |
          emcc dsp.c -O3 -s WASM=1 \
          -s EXPORTED_FUNCTIONS='["_process_audio_block", "_update_engine", "_set_gate_config", "_set_ac_config", "_init_sample_rate", "_get_in_buf_ptr", "_get_out_buf_ptr"]' \
          -o dsp.wasm

      - name: Setup Pages
        uses: actions/configure-pages@v4

      - name: Upload Artifact (HTML + Wasm)
        uses: actions/upload-pages-artifact@v3
        with:
          path: '.' # Uploads index.html and the newly compiled dsp.wasm

  deploy:
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
    needs: build
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```
3. Go to **Repository Settings > Pages** and set the Source to **GitHub Actions**.
4. Push to `main`. GitHub will compile the Wasm and deploy the site automatically!

### Option B: Cloudflare Pages (Recommended for Advanced Features)
Cloudflare Pages is highly recommended because it allows you to set custom HTTP headers. This is **required** if you want to enable `SharedArrayBuffer` (Cross-Origin Isolation) for zero-copy Wasm memory sharing in the future.

1. Push your repository (containing `index.html`, `dsp.wasm`, and `dsp.c`) to GitHub/GitLab.
2. Log into Cloudflare Dashboard > **Workers & Pages** > **Create** > **Pages** > **Connect to Git**.
3. **Build Settings:** 
   * Build Command: *(Leave blank, or use the emcc command if you add an emscripten build environment)*
   * Build Output: `/`
4. **Crucial Step:** In the root of your repository, create a file named `_headers` (no extension) and add this:
```text
   /*
     Cross-Origin-Opener-Policy: same-origin
     Cross-Origin-Embedder-Policy: require-corp
```
5. Deploy. Cloudflare will serve your app globally via their edge network with the strict security headers required for advanced WebAssembly threading.

---

## ⚠️ Troubleshooting

| Issue | Cause | Solution |
| :--- | :--- | :--- |
| **"Error accessing microphone"** | Insecure Context | Browsers block mic access on `http://` or `file://`. Ensure you are using `https://` or `localhost`. |
| **Wasm fails to load / MIME type error** | Server Config | The server is serving `.wasm` as `text/plain`. Use GitHub Pages or Cloudflare, which automatically serve `application/wasm`. |
| **Audio clicks, pops, or dropouts** | CPU Overload | 20 engines × 1000 tracks is heavy. Reduce the "Tracks/Engine" count in the UI, or disable the Convolution Reverb. |
| **"Wasm load failed" in Console** | CORS / Fetch Error | Ensure `dsp.wasm` is in the exact same directory as `index.html` and is being served from the same origin. |

---

## 📄 License

This project is open-source and available under the [MIT License](LICENSE).
