# Vaani: Offline Indian-Language Pronunciation Assessment SDK

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Android%20%7C%20iOS%20%7C%20Hybrid-brightgreen.svg)]()
[![Status](https://img.shields.io/badge/Status-Milestone%201%20Complete-orange.svg)]()

**Vaani** is a production-quality, modular, secure, lightweight, and completely offline Indian-language pronunciation assessment SDK for mobile and hybrid applications (Android, iOS, React Native, Flutter, Ionic/Capacitor).

Unlike conventional speech systems that rely on naive speech-to-text transcript matching, **Vaani** performs true acoustic-phonetic pronunciation assessment. By combining **Unicode-aware Brahmic Grapheme-to-Phoneme (G2P)** mapping, **CTC Viterbi forced alignment**, and **Goodness of Pronunciation (GOP)** segmental scoring, Vaani pinpoints pronunciation errors down to individual phonemes, maps them back to the exact written characters on the mobile screen, and provides actionable articulatory feedback.

---

## 🎯 Initial Target Languages

| Language | Code | Script | Key Phonetic Focus | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Tamil** | `ta-IN` | Tamil (`Taml`) | Liquid triplet (ழ /ɻ/, ள /ɭ/, ல /l/), rhotics (ற /r/, ர /ɾ/), allophonic stop voicing | Validated (Pilot Vertical Slice) |
| **Sanskrit** | `sa-IN` | Devanagari (`Deva`) | 5 vargas $\times$ 4 phonations, vowel length ($a/\bar{a}$), Visarga echo-aspiration, Anusvara | Validated (Paninian rules) |
| **Hindi** | `hi-IN` | Devanagari (`Deva`) | Contextual schwa-deletion, Nukta consonants (क़, ख़, ग़, ज़, फ़, ड़, ढ़), nasalization | Validated (Schwa engine) |
| **Telugu** | `te-IN` | Telugu (`Telu`) | Dravidian vowel length pairs (e/ē, o/ō), retroflex series, geminate consonant duration | Validated (Vowel duration rules) |

---

## 🏛️ Core Principles & Architecture

1. **100% Offline Speech Processing:** Zero network calls, subscriptions, API keys, or cloud inference required during speech processing or scoring.
2. **True Acoustic Pronunciation Assessment:** Rejects ASR transcript equality. Uses frame-level acoustic probabilities and constrained Viterbi forced alignment to compute Goodness of Pronunciation (GOP).
3. **Commercially Compatible:** Built entirely on permissive open-source licenses (Apache 2.0 SDK core, MIT-licensed IndicConformer model weights). Non-commercial licenses (e.g. Meta MMS `CC-BY-NC`) are strictly excluded.
4. **Lightweight Core with Independent Packages:** The core native SDK is small (~5–10 MB). Language packages (`lang-sa`, `lang-ta`, `lang-hi`, `lang-te`) are downloaded independently, verified cryptographically, and installed atomically.
5. **Privacy & Security First:** Zero telemetry. Microphone audio is processed entirely in volatile RAM and immediately wiped. Packages are signed using Ed25519 and hashed with SHA-256.

---

## 📂 Repository Structure

```text
vaani-core/
├── README.md                          # Project overview & quickstart
├── LICENSE                            # Apache 2.0 License
├── SECURITY.md                        # Threat model & vulnerability reporting
├── CONTRIBUTING.md                    # Contributor guide & coding conventions
├── CHANGELOG.md                       # Release notes & version history
├── docs/                              # Architecture & engineering specifications
│   ├── architecture.md                # System design & Architectural Decision Records (ADRs)
│   ├── model-selection.md             # Model evaluation, licensing audit, and benchmarks
│   ├── pronunciation-scoring.md       # GOP theory, CTC forced alignment, and G2P math
│   ├── language-packages.md           # Manifest schema v1, package lifecycle, and security
│   ├── security.md                    # Threat model, Ed25519 signatures, and audio privacy
│   └── implementation-plan.md         # Phased milestone engineering roadmap
├── core/                              # Native C++17 core engine
│   ├── include/vaani/                 # Stable C ABI headers (vaani_c_api.h)
│   ├── src/                           # Runtime, scoring, and package manager
│   ├── tests/                         # Unit and security tests
│   └── CMakeLists.txt                 # Native build configuration
├── backends/                          # Pluggable inference backends
│   ├── sherpa-onnx/                   # sherpa-onnx / ONNX Runtime adapter
│   └── mock/                          # Deterministic mock engine for CI
├── language-resources/                # Linguistic rules, phonemes, and test suites
│   ├── ta/                            # Tamil G2P & phonetics
│   ├── sa/                            # Sanskrit G2P & phonetics
│   ├── hi/                            # Hindi G2P & phonetics
│   └── te/                            # Telugu G2P & phonetics
├── package-manager/                   # Manifest validator & package packager
├── bindings/                          # Mobile platform bindings
│   ├── android/                       # Kotlin / JNI native module
│   ├── ios/                           # Swift framework
│   ├── react-native/                  # TypeScript npm package
│   └── flutter/                       # Dart plugin
└── examples/                          # Reference applications
```

---

## 🚀 Development Milestones

* **Milestone 1: Feasibility and Architecture** (✅ **Completed**)
  * Empirical model evaluation, licensing audit, phoneme-scoring design, ADRs, and roadmap.
* **Milestone 2: Core Runtime and Manifest Engine** (🔄 In Progress)
  * Stable C ABI, runtime lifecycle, manifest validator, SHA-256/Ed25519 cryptographic verifier, and mock backend.
* **Milestone 3: First Real Language End-to-End (`lang-ta` Tamil)**
  * Tamil Brahmic G2P, liquid triplet detection (/ɻ/ vs /ɭ/ vs /l/), ONNX inference, and performance profiling.
* **Milestone 4: Language Expansion (`sa`, `hi`, `te`)**
  * Sanskrit, Hindi, and Telugu resource packages, phonological rule verification, and regression tests.
* **Milestone 5: Hybrid Mobile Bindings**
  * Android Kotlin/JNI, iOS Swift, React Native, and Flutter integration examples.
* **Milestone 6: Production Readiness & Release**
  * Benchmark harness, fuzzing/ASan audits, and official v1.0.0 release.

---

## 📜 Documentation Index

* [System Architecture & ADRs](docs/architecture.md)
* [Model Selection & Licensing Matrix](docs/model-selection.md)
* [Pronunciation Assessment & GOP Scoring Theory](docs/pronunciation-scoring.md)
* [Language Package Specification & Manifest Schema](docs/language-packages.md)
* [Security Architecture & Threat Model](docs/security.md)
* [Implementation Plan & Roadmap](docs/implementation-plan.md)
