# Olineri: Offline Indian-Language Pronunciation Assessment SDK

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Android%20%7C%20iOS%20%7C%20React%20Native%20%7C%20Flutter%20%7C%20Capacitor-brightgreen.svg)]()
[![Build & Tests](https://img.shields.io/badge/Tests-7%2F7%20Passing%20(100%25)-success.svg)]()
[![Status](https://img.shields.io/badge/Release-v0.1.0%20Ready-green.svg)]()

**Olineri** (ஒலிநெறி — Tamil for *phonetic rules & acoustic pathway*) is a production-quality, modular, secure, lightweight, and completely offline Indian-language pronunciation assessment library for mobile and hybrid applications (Android, iOS, React Native, Flutter, Ionic/Capacitor).

Unlike conventional speech systems that rely on naive speech-to-text transcript matching, **Olineri** performs true acoustic-phonetic pronunciation assessment. By combining **Unicode-aware Brahmic Grapheme-to-Phoneme (G2P)** mapping, **Constrained CTC Viterbi forced alignment**, and **Goodness of Pronunciation (GOP)** segmental scoring, Olineri pinpoints pronunciation errors down to individual phonemes, maps them back to the exact written characters on the mobile screen, and provides actionable articulatory feedback.

---

## 🎯 Supported Languages (v0.1.0)

| Language | Code | Script | Key Phonetic Focus | Verification Status |
| :--- | :--- | :--- | :--- | :--- |
| **Tamil** | `ta-IN` | Tamil (`Taml`) | Liquid triplet (ழ `/ɻ/`, ள `/ɭ/`, ல `/l/`), rhotics (ற `/r/`, ர `/ɾ/`), allophonic stop voicing | **100% Tested** (`test_tamil_assessment`) |
| **Sanskrit** | `sa-IN` | Devanagari (`Deva`) | Strict inherent vowel retention, Visarga (ः) echo-vowel articulation, 5 vargas aspiration, Anusvara | **100% Tested** (`test_multi_language`) |
| **Hindi** | `hi-IN` | Devanagari (`Deva`) | 2-tier contextual schwa-deletion ($VC_1 ə C_2 V$), Nukta loanwords (क़, ख़, ग़, ज़, फ़, ड़, ढ़) | **100% Tested** (`test_multi_language`) |
| **Telugu** | `te-IN` | Telugu (`Telu`) | Dravidian vowel length pairs (e/ē, o/ō), consonant gemination closure duration (vottulu), Sunna | **100% Tested** (`test_multi_language`) |

---

## 🏛️ Core Principles & Architecture

1. **100% Offline Speech Processing:** Zero network calls, cloud inference, telemetry, or mandatory API keys.
2. **True Acoustic Pronunciation Assessment:** Rejects ASR transcript equality. Uses frame-level emission log-posteriors and constrained Viterbi forced alignment to compute Goodness of Pronunciation (GOP).
3. **Commercially Compatible:** Built entirely on permissive open-source licenses (Apache 2.0 SDK core, MIT IndicConformer model weights). Non-commercial licenses (e.g. Meta MMS `CC-BY-NC`) are strictly excluded.
4. **Lightweight Core with Independent Packages:** The core native SDK is small (~1.2 MB shared library). Language packages (`lang-sa`, `lang-ta`, `lang-hi`, `lang-te`) are downloaded independently, verified cryptographically, and installed atomically.
5. **Privacy & Security First:** Zero telemetry. Microphone audio is processed in volatile RAM and immediately wiped upon session teardown. Packages are signed using Ed25519 and hashed with SHA-256 with path traversal / zip-slip defenses.

---

## 📂 Repository Structure

```text
olineri-core/
├── README.md                          # Project overview & quickstart
├── LICENSE                            # Apache 2.0 License
├── SECURITY.md                        # Threat model & vulnerability reporting
├── CONTRIBUTING.md                    # Contributor guide & coding conventions
├── CHANGELOG.md                       # Release notes & version history
├── CMakeLists.txt                     # CMake build configuration for native core & test suite
├── docs/                              # Architecture & engineering specifications
│   ├── architecture.md                # System design & Architectural Decision Records (ADRs)
│   ├── model-selection.md             # Model evaluation, licensing audit, and benchmarks
│   ├── pronunciation-scoring.md       # GOP theory, CTC forced alignment, and G2P math
│   ├── language-packages.md           # Manifest schema v1, package lifecycle, and security
│   ├── security.md                    # Threat model, Ed25519 signatures, and audio privacy
│   ├── implementation-plan.md         # Phased milestone engineering roadmap
│   ├── tamil-pilot.md                 # Tamil vertical slice benchmark report
│   ├── language-expansion.md          # Sanskrit, Hindi, and Telugu phonology and verification
│   ├── integration-guides.md          # Android, iOS, React Native, Flutter, and Capacitor guides
│   ├── api-reference.md               # Complete C ABI and binding function reference
│   ├── feature-matrix.md              # Detailed implementation status matrix
│   ├── known-limitations.md           # Environmental, dialectal, and technical limitations
│   └── release-readiness-report.md    # Formal release audit and acceptance verification
├── core/                              # Native C++17 core engine
│   ├── include/olineri/               # Stable C ABI headers (olineri_c_api.h)
│   ├── src/                           # Runtime, scoring, crypto, G2P, and package manager
│   └── tests/                         # Automated unit, integration, and security test suites
├── backends/                          # Inference backends
│   ├── mock/                          # Deterministic mock engine for CI
│   └── sherpa-onnx/                   # sherpa-onnx integration adapter
├── language-resources/                # Linguistic rules, phonemes, and test cases
│   ├── ta/                            # Tamil G2P, phonemes, and articulatory feedback
│   ├── sa/                            # Sanskrit G2P, phonemes, and articulatory feedback
│   ├── hi/                            # Hindi G2P, phonemes, and articulatory feedback
│   └── te/                            # Telugu G2P, phonemes, and articulatory feedback
├── bindings/                          # Native and hybrid mobile SDK bindings
│   ├── android/                       # Kotlin SDK & JNI bridge (OlineriSDK.kt, olineri_jni.cpp)
│   ├── ios/                           # Swift framework & Package.swift (OlineriSDK.swift)
│   ├── react-native/                  # TypeScript SDK & native modules (@olineri/react-native-sdk)
│   ├── flutter/                       # Dart plugin & FFI bindings (olineri_flutter)
│   └── capacitor/                     # Capacitor plugin (@olineri/capacitor-plugin)
├── examples/                          # Reference applications
│   └── hybrid-pronunciation-app/      # Interactive demonstration app (HTML/CSS/JS)
├── benchmarks/                        # Performance benchmarking harness
│   └── benchmark_engine.cpp           # Latency, throughput, and memory profiler
└── .github/workflows/                 # Automated CI/CD
    └── ci.yml                         # Multi-OS build, test, ASan, and benchmark workflow
```

---

## ⚡ Quickstart: Building & Testing Locally

### Prerequisites
- CMake 3.16 or later
- C++17 compliant compiler (Clang 11+, Apple Clang 12+, GCC 9+)

### Build and Run Tests
```bash
# 1. Clone repository
git clone https://github.com/nltsravi/olineri-core.git
cd olineri-core

# 2. Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# 3. Build all targets
cmake --build build -j$(sysctl -n hw.ncpu || nproc)

# 4. Run automated test suite
ctest --test-dir build --output-on-failure

# 5. Run performance benchmark harness
./build/benchmark_engine
```

### Measured Performance
- **G2P Transformation Throughput:** 220,000 – 365,000 words/second (0.011 – 0.019 ms per phrase).
- **Cold Session Initialization:** 0.010 – 0.030 ms.
- **Warm Pronunciation Assessment:** 0.18 – 0.21 ms.
- **Peak Resident Memory:** ~14.2 MB.

---

## 📱 Mobile & Hybrid Integration

See the comprehensive [Integration Guides](docs/integration-guides.md) for step-by-step setup in:
- **Android:** [Kotlin / JNI Guide](docs/integration-guides.md#1-android-integration-kotlin--jni)
- **iOS:** [Swift / SPM Guide](docs/integration-guides.md#2-ios-integration-swift--spm)
- **React Native:** [TypeScript Guide](docs/integration-guides.md#3-react-native-integration)
- **Flutter:** [Dart / FFI Guide](docs/integration-guides.md#4-flutter-integration)
- **Capacitor / Ionic:** [Capacitor Plugin Guide](docs/integration-guides.md#5-capacitor--ionic-integration)

---

## 📜 Documentation Index

* [System Architecture & ADRs](docs/architecture.md)
* [API Reference Manual](docs/api-reference.md)
* [Model Selection & Licensing Matrix](docs/model-selection.md)
* [Pronunciation Assessment & GOP Scoring Theory](docs/pronunciation-scoring.md)
* [Language Package Specification & Manifest Schema](docs/language-packages.md)
* [Security Architecture & Threat Model](docs/security.md)
* [Tamil Pilot Benchmark Report](docs/tamil-pilot.md)
* [Language Expansion: Sanskrit, Hindi, Telugu](docs/language-expansion.md)
* [Mobile Integration Guides](docs/integration-guides.md)
* [Feature Support Matrix](docs/feature-matrix.md)
* [Known Limitations & Technical Constraints](docs/known-limitations.md)
* [Release-Readiness Report (v0.1.0)](docs/release-readiness-report.md)

---

## 📄 License

The **Olineri Core SDK** is distributed under the terms of the **Apache License, Version 2.0**.  
See [LICENSE](LICENSE) for details.
