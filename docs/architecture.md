# System Architecture & Decision Records (ADR)

This document outlines the core architecture, module decomposition, and Architectural Decision Records for the `olineri-core` offline pronunciation assessment SDK.

---

## 1. Architectural Decision Records (ADRs)

### ADR-0001: Core Implementation in C++17 with a Stable C ABI
* **Context:** The SDK must run efficiently on Android (via Kotlin/JNI), iOS (via Swift/ObjC++), React Native, Flutter, and potential embedded devices without duplicating the speech engine or audio pipelines.
* **Decision:** Implement the core runtime in modern standard C++17. Expose all public capabilities through a versioned, extern "C" stable ABI (`olineri_c_api.h`).
* **Consequences:**
  * Clean boundary: Native mobile bindings (Kotlin, Swift, Dart, TypeScript) only interact with the C ABI.
  * No C++ ABI mangling issues across compiler versions or NDK/Xcode upgrades.
  * Opaque pointers (handles) prevent memory leaks across language barriers.

### ADR-0002: Modular Backend Abstraction (`InferenceEngine`)
* **Context:** Inference backends on mobile vary (ONNX Runtime, sherpa-onnx, Apple CoreML, Android NNAPI). Hardcoding a single library couples the SDK to that runtime's build quirks and licensing.
* **Decision:** Introduce an abstract C++ interface `InferenceEngine` with concrete implementations:
  1. `SherpaOnnxBackend`: Leverages sherpa-onnx for optimized CPU/NEON CTC decoding.
  2. `OnnxRuntimeBackend`: Direct ONNX Runtime execution for raw logit extraction.
  3. `MockBackend`: Deterministic synthetic engine for automated CI testing without multi-hundred megabyte model files.
* **Consequences:** Any model format or backend can be swapped without touching the scoring or package management code.

### ADR-0003: Acoustic Assessment via Forced Alignment & Goodness of Pronunciation (GOP)
* **Context:** ASR transcript matching fails to provide phoneme-level diagnostic feedback and autocorrects mispronunciations.
* **Decision:** Use CTC Viterbi forced alignment against the canonical phonetic sequence derived from expected text, followed by frame-level Log-Posterior Ratio (GOP) scoring.
* **Consequences:** Produces objective acoustic scores per phoneme, identifies exact substitutions/omissions/insertions, and avoids language-model bias.

### ADR-0004: Brahmic Akshara-to-Phoneme Alignment
* **Context:** Indian scripts (Tamil, Devanagari, Telugu) are syllabic (abugidas). Users must see which exact written letter/matra was mispronounced in the host app UI.
* **Decision:** Implement a Brahmic G2P engine that segments Unicode text into grapheme clusters, tracks byte/character spans, and maintains 1:1 or 1:N coordinate mappings to IPA phonemes.
* **Consequences:** Errors like substituting Tamil ழ with ல directly highlight the character `ழ` in the user's mobile screen.

### ADR-0005: Decoupled, Atomic, Cryptographically Verified Language Packages
* **Context:** Shipping models for Sanskrit, Tamil, Hindi, and Telugu in the base app would bloat download size by 150–400 MB.
* **Decision:** The core SDK contains zero models (~5–10 MB). Language packages (`lang-sa`, `lang-ta`, `lang-hi`, `lang-te`) are downloaded independently, verified via Ed25519 signatures and SHA-256 hashes, and installed atomically.
* **Consequences:** Minimal initial app footprint; users download only the languages they practice.

---

## 2. High-Level Modular Architecture

```text
┌────────────────────────────────────────────────────────────────────────┐
│                      Hybrid Application Layer                          │
│         (React Native / Flutter / Android Native / iOS Native)         │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ C ABI / JNI / Swift FFI
┌───────────────────────────────────▼────────────────────────────────────┐
│                            olineri-core                                │
│                                                                        │
│  ┌───────────────────────┐              ┌───────────────────────────┐  │
│  │ Module A: Runtime     │              │ Module B: Package Manager │  │
│  │ - Session Lifecycle   │              │ - Catalog & Downloader    │  │
│  │ - Resource Management │              │ - Ed25519 & SHA256 Check  │  │
│  │ - Memory Limits       │              │ - Atomic Install/Rollback │  │
│  └───────────┬───────────┘              └─────────────┬─────────────┘  │
│              │                                        │                │
│  ┌───────────▼───────────┐              ┌─────────────▼─────────────┐  │
│  │ Module E: Language    │              │ Module C: Speech Recog    │  │
│  │   Resources           │              │ - 16 kHz Audio Input      │  │
│  │ - Unicode Normalizer  │              │ - Feature Extraction      │  │
│  │ - Brahmic G2P Engine  │              │ - Streaming / Final CTC   │  │
│  │ - Akshara Spans       │              └─────────────┬─────────────┘  │
│  └───────────┬───────────┘                            │                │
│              │                                        │                │
│  ┌───────────▼────────────────────────────────────────▼─────────────┐  │
│  │ Module D: Pronunciation Assessment Engine                        │  │
│  │ - Constrained CTC Viterbi Forced Alignment                       │  │
│  │ - Goodness of Pronunciation (GOP) Segmental Calculator           │  │
│  │ - Error Classifier (Substitution, Omission, Insertion)           │  │
│  └───────────────────────────────────┬──────────────────────────────┘  │
│                                      │                                 │
│  ┌───────────────────────────────────▼──────────────────────────────┐  │
│  │ Module F: Feedback & Learning Interface                          │  │
│  │ - Structured Diagnostics & Calibrated Phoneme Scores             │  │
│  │ - Articulatory Guidance & Character Group Highlights             │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Stable C ABI Specification (`olineri_c_api.h`)

### 3.1. Core Types and Handles
```c
// Opaque handles
typedef struct olineri_runtime* olineri_runtime_handle_t;
typedef struct olineri_session* olineri_session_handle_t;
typedef struct olineri_result*  olineri_result_handle_t;

// Error codes
typedef enum {
    OLINERI_OK = 0,
    OLINERI_ERR_INVALID_ARGUMENT = -1,
    OLINERI_ERR_NOT_INITIALIZED = -2,
    OLINERI_ERR_PACKAGE_NOT_FOUND = -3,
    OLINERI_ERR_PACKAGE_CORRUPTED = -4,
    OLINERI_ERR_INCOMPATIBLE_VERSION = -5,
    OLINERI_ERR_INSUFFICIENT_STORAGE = -6,
    OLINERI_ERR_INFERENCE_FAILED = -7,
    OLINERI_ERR_OUT_OF_MEMORY = -8,
    OLINERI_ERR_PATH_TRAVERSAL = -9
} olineri_status_t;

// Phoneme assessment result
typedef enum {
    OLINERI_PHONEME_CORRECT = 0,
    OLINERI_PHONEME_SUBSTITUTION = 1,
    OLINERI_PHONEME_OMISSION = 2,
    OLINERI_PHONEME_INSERTION = 3,
    OLINERI_PHONEME_UNCERTAIN = 4
} olineri_phoneme_status_t;

typedef struct {
    const char* expected_phoneme;     // Canonical IPA (e.g. "/ɻ/")
    const char* detected_phoneme;     // Detected IPA if substitution
    float score;                      // Calibrated score 0.0 - 100.0
    olineri_phoneme_status_t status;  // Classification
    uint32_t char_start_utf8;         // Byte offset in reference text
    uint32_t char_end_utf8;           // Byte offset in reference text
    uint32_t cluster_index;           // Grapheme cluster index for UI highlight
    float start_time_ms;              // Segment onset in audio
    float end_time_ms;                // Segment offset in audio
    const char* feedback_instruction; // Human-readable corrective prompt
} olineri_phoneme_feedback_t;
```

### 3.2. Lifecycle Functions
```c
// Runtime initialization
olineri_status_t olineri_runtime_create(const char* storage_directory, 
                                        olineri_runtime_handle_t* out_runtime);
olineri_status_t olineri_runtime_destroy(olineri_runtime_handle_t runtime);

// Language Session management
olineri_status_t olineri_session_create(olineri_runtime_handle_t runtime, 
                                        const char* language_tag, 
                                        olineri_session_handle_t* out_session);
olineri_status_t olineri_session_destroy(olineri_session_handle_t session);

// Assessment execution
olineri_status_t olineri_assess_pronunciation(olineri_session_handle_t session,
                                              const char* reference_text,
                                              const float* audio_samples,
                                              uint32_t sample_count,
                                              uint32_t sample_rate,
                                              olineri_result_handle_t* out_result);

// Result extraction
float olineri_result_get_overall_score(olineri_result_handle_t result);
uint32_t olineri_result_get_phoneme_count(olineri_result_handle_t result);
olineri_status_t olineri_result_get_phoneme_feedback(olineri_result_handle_t result,
                                                     uint32_t index,
                                                     olineri_phoneme_feedback_t* out_feedback);
void olineri_result_destroy(olineri_result_handle_t result);
```

---

## 4. Mobile Lifecycle & Thread Safety

* **Thread Safety:** Runtime handles are safe for concurrent read operations (e.g. querying installed languages). Individual sessions are single-threaded; multiple concurrent sessions are isolated.
* **Low Memory (Trim Memory):** When Android/iOS signals low memory (`onTrimMemory` / `didReceiveMemoryWarning`), calling `olineri_runtime_trim_memory()` releases idle ONNX model weights and feature caches back to the OS.
* **Cancellation:** Long-running inferences support cooperative cancellation tokens so UI navigations immediately terminate background compute.
