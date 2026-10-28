# Changelog

All notable changes to the `vaani-core` SDK are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.1.0] - 2026-10-28

### Milestone 1: Feasibility and Architecture
* **Added:** System architecture specifications and Architectural Decision Records (`docs/architecture.md`).
* **Added:** Empirical speech model evaluation and commercial licensing matrix (`docs/model-selection.md`).
* **Added:** Phonetic assessment theory, CTC Viterbi forced alignment, and Goodness of Pronunciation (GOP) mathematical formulation (`docs/pronunciation-scoring.md`).
* **Added:** Language package directory layout, atomic lifecycle specification, and Manifest Schema v1.0.0 (`docs/language-packages.md`).
* **Added:** Threat model, Ed25519 cryptographic signature design, and zero-telemetry audio privacy specification (`docs/security.md`).
* **Added:** Comprehensive milestone-by-milestone engineering roadmap (`docs/implementation-plan.md`).
* **Added:** Apache 2.0 license, contributing guide, and security policy.

### Milestone 2: Core Runtime and Manifest Engine
* **Added:** Stable C ABI header (`core/include/vaani/vaani_c_api.h`) defining runtime lifecycle, package management, session isolation, and structured assessment outputs.
* **Added:** Core runtime implementation (`core/src/runtime.cpp`) with session tracking and low-memory trim handling.
* **Added:** Package manifest parser and schema validator (`core/src/manifest.cpp`) enforcing version compatibility and path-traversal safety.
* **Added:** Cryptographic engine (`core/src/crypto.cpp`) with standard FIPS 180-4 SHA-256 and TweetNaCl-based Ed25519 signature verification.
* **Added:** Atomic package manager (`core/src/package_manager.cpp`) with SHA-256 artifact verification, Ed25519 signature checking, staging, atomic swap, and rollback.
* **Added:** Constrained CTC Viterbi forced alignment and Goodness of Pronunciation (GOP) segmental scoring engine (`core/src/scoring_engine.cpp`).
* **Added:** Pluggable `InferenceEngine` interface and deterministic `MockEngine` (`backends/mock/mock_engine.cpp`).
* **Added:** Root `CMakeLists.txt` compiling static and shared libraries with C++17.
* **Added:** 5 comprehensive automated test suites (`test_crypto`, `test_manifest`, `test_package_manager`, `test_scoring`, `test_runtime`) with 100% test pass rate.

### Milestone 3: First Real Language End-to-End (Tamil `lang-ta`)
* **Added:** Tamil phoneme inventory (`language-resources/ta/phoneme_inventory.json`) covering 31 canonical sounds.
* **Added:** English and Tamil localized articulatory feedback strings (`language-resources/ta/feedback/strings_en.json`, `strings_ta.json`) with specialized corrective advice for the liquid triplet (ழ /ɻ/, ள /ɭ/, ல /l/) and rhotics (ற /r/, ர /ɾ/).
* **Added:** Unicode Brahmic G2P engine (`core/src/g2p.cpp`) segmenting Tamil Aksharas and calculating exact byte spans for UI highlight.
* **Added:** End-to-end integration test (`core/tests/test_tamil_assessment.cpp`) validating G2P transformation, Akshara coordinate spans, correct pronunciation scoring, and substitution detection on ழ.
* **Added:** Tamil pilot empirical measurement report (`docs/tamil-pilot.md`).

### Milestone 4: Language Expansion (Sanskrit `lang-sa`, Hindi `lang-hi`, Telugu `lang-te`)
* **Added:** Sanskrit phonological model in G2P with strict inherent vowel retention and Visarga (ः) vowel echo-aspiration.
* **Added:** Hindi phonological model in G2P with 2-tier contextual schwa deletion (final schwa and internal $VC_1 ə C_2 V$) and Perso-Arabic Nukta loanwords (क़, ख़, ग़, ज़, फ़, ड़, ढ़).
* **Added:** Telugu phonological model in G2P with short/long vowel pairs (e/ē, o/ō), consonant gemination closure duration (vottulu), and Sunna (ం).
* **Added:** Language resources for `sa`, `hi`, and `te`: phoneme inventories, articulatory advice strings, and minimal-pair test cases.
* **Added:** Comprehensive automated multilingual test suite (`core/tests/test_multi_language.cpp`).
* **Added:** Language expansion verification documentation (`docs/language-expansion.md`).

### Milestone 5: Hybrid & Native Mobile SDK Bindings
* **Added:** Android Kotlin SDK and JNI native module bridge (`bindings/android/`).
* **Added:** iOS Swift framework and Swift Package Manager configuration (`bindings/ios/`).
* **Added:** React Native TypeScript package with native Android/iOS bridge modules (`bindings/react-native/`).
* **Added:** Flutter plugin backed by Dart FFI bindings (`bindings/flutter/`).
* **Added:** Capacitor/Ionic plugin implementation with web fallback (`bindings/capacitor/`).
* **Added:** Interactive hybrid mobile demonstration application (`examples/hybrid-pronunciation-app/`).
* **Added:** Step-by-step mobile integration guides (`docs/integration-guides.md`).

### Milestone 6: Production Readiness & Release
* **Added:** Reproducible performance benchmarking harness (`benchmarks/benchmark_engine.cpp`).
* **Added:** Multi-platform GitHub Actions CI workflow with AddressSanitizer checks (`.github/workflows/ci.yml`).
* **Added:** Complete C ABI and mobile bindings reference manual (`docs/api-reference.md`).
* **Added:** Comprehensive feature support matrix (`docs/feature-matrix.md`).
* **Added:** Known limitations, dialect variations, and technical constraints document (`docs/known-limitations.md`).
* **Added:** Formal release-readiness verification report (`docs/release-readiness-report.md`).
