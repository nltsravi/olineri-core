# Engineering Implementation Plan & Milestone Roadmap

This roadmap governs the phased, milestone-by-milestone development of the `vaani-core` offline pronunciation assessment SDK.

---

## Milestone 1: Feasibility, Model Evaluation, and Architecture (Current)

### Objectives
* Perform an empirical evaluation of open-source models for **Sanskrit (`sa`)**, **Tamil (`ta`)**, **Hindi (`hi`)**, and **Telugu (`te`)**.
* Evaluate phoneme-level scoring capabilities (GOP vs. ASR transcript matching).
* Audit model and runtime licenses (verifying commercial compatibility, rejecting non-commercial weights).
* Document architectural decisions (ADRs) and formalize the C ABI.
* Document missing components and mitigation strategies.

### Deliverables
1. `LICENSE` (Apache 2.0).
2. `docs/model-selection.md`: Technical and licensing audit across architectures and candidates.
3. `docs/pronunciation-scoring.md`: Mathematical formulation of CTC Viterbi forced alignment and Goodness of Pronunciation (GOP).
4. `docs/language-packages.md`: Package directory structure, manifest schema v1, and lifecycle management.
5. `docs/security.md`: Threat model, Ed25519 signature verification, zip-slip defense, and privacy guarantees.
6. `docs/architecture.md`: System decomposition, ADR-0001 through ADR-0005, and C ABI specification.
7. `docs/implementation-plan.md`: This comprehensive phased execution plan.

### Acceptance Criteria
- [x] Zero fabrication of model capabilities or licensing terms.
- [x] Clear technical justification for rejecting ASR transcript matching.
- [x] Commercially unencumbered model paths identified (MIT-licensed IndicConformer CTC).
- [x] Clear architectural separation between core engine and language resources.

---

## Milestone 2: Core Runtime and Manifest Engine

### Objectives
* Implement the core C++17 runtime and extern "C" stable ABI (`include/vaani/vaani_c_api.h`).
* Implement package manifest parser and validator (`manifest-schema/`).
* Implement cryptographic verification (SHA-256 integrity and Ed25519 signature checks).
* Implement the atomic package installer, updater, and rollback manager.
* Implement the `InferenceEngine` interface and a high-fidelity `MockBackend` for unit testing.
* Create automated unit tests for runtime lifecycle, package validation, and error recovery.

### Key Technical Deliverables
* `core/include/vaani/vaani_c_api.h`: Stable C header.
* `core/src/runtime.cpp`: Runtime lifecycle, session management, resource limits.
* `core/src/package_manager.cpp`: Secure archive unpacking, checksum validation, atomic staging swap.
* `core/src/manifest.cpp`: Schema validation, version compatibility enforcement.
* `core/src/scoring_engine.cpp`: CTC Viterbi aligner and GOP segmental score calculator.
* `backends/mock/mock_engine.cpp`: Deterministic test engine producing controlled acoustic logits.
* `core/tests/`: Automated unit and security tests.

### Acceptance Criteria
- Core runtime compiles cleanly on macOS and Linux with zero warnings.
- Package manager rejects tampered hashes, invalid signatures, and path-traversal archives.
- Failed package updates preserve the previously working package without corruption.
- Automated tests pass in CI.

---

## Milestone 3: First Real Language End-to-End (`lang-ta` Tamil)

### Objectives
* Select Tamil (`ta-IN`) as the primary vertical slice due to its high pedagogical demand for distinctive liquid contrasts (ழ /ɻ/, ள /ɭ/, ல /l/) and rhotic contrasts (ற /r/, ர /ɾ/).
* Implement the Dravidian Brahmic G2P rule engine for Tamil with contextual intervocalic lenition and allophonic voicing.
* Implement bidirectional character-to-phoneme span mapping for Tamil aksharas.
* Integrate quantized IndicConformer CTC ONNX model for Tamil.
* Run end-to-end pronunciation assessment with real audio recordings:
  1. Correct pronunciation: "பழம்" articulated with /ɻ/.
  2. Mispronounced substitution: "பலம்" articulated with /l/.
* Measure cold-start latency, warm inference latency, peak memory, and package size.

### Acceptance Criteria
- Tamil G2P correctly transforms "தமிழ்" $\to$ `[t̪ɐmɨɻ]` and "பழம்" $\to$ `[pɐɻɐm]` with exact character spans.
- CTC forced alignment correctly isolates the /ɻ/ segment in the audio.
- Mispronouncing /ɻ/ as /l/ reliably drops the GOP score for character `ழ` below the acceptance threshold, returning specific corrective articulatory instruction.
- Cold-start $< 250\text{ ms}$; peak RAM $< 100\text{ MB}$.

---

## Milestone 4: Multi-Language Package Expansion (`sa`, `hi`, `te`)

### Objectives
* Implement independent linguistic resource packages for:
  * **Sanskrit (`lang-sa`)**: Paninian G2P, vowel duration modeling, Visarga echo-aspiration, Anusvara assimilation.
  * **Hindi (`lang-hi`)**: Indo-Aryan G2P, contextual schwa-deletion engine, Nukta loan consonant handling, Chandrabindu nasalization.
  * **Telugu (`lang-te`)**: Dravidian G2P, short/long vowel pairs (e/ē, o/ō), gemination closure timing.
* Validate that core runtime remains language-agnostic.
* Add test matrices covering pronunciation variants, edge cases, and difficult phonetic contrasts for each language.

### Acceptance Criteria
- Sanskrit engine differentiates aspirated vs unaspirated stops without schwa deletion.
- Hindi engine correctly deletes final schwas in words like कमल (`/kəməl/`), but preserves schwas in conjuncts.
- Telugu engine accurately distinguishes vowel length in minimal pairs.
- All four language packages can be installed, updated, and uninstalled independently.

---

## 5. Milestone 5: Hybrid Mobile Bindings & Example Applications

### Objectives
* Implement Android native JNI wrapper and Kotlin SDK (`bindings/android/`).
* Implement iOS native Swift framework with C interop (`bindings/ios/`).
* Implement React Native TypeScript package (`bindings/react-native/`).
* Implement Flutter Dart plugin (`bindings/flutter/`).
* Create interactive example hybrid applications demonstrating:
  * Dynamic language catalog browsing and download progress.
  * Offline assessment with microphone capture.
  * Phoneme-level visual feedback with character highlighting.

### Acceptance Criteria
- React Native and Flutter example apps run on Android and iOS.
- Complete offline functionality verified under Airplane Mode.
- Native memory released when language package is unloaded.

---

## 6. Milestone 6: Production Readiness & Release Benchmarking

### Objectives
* Comprehensive security, memory leak (Valgrind / ASan), and fuzzing audit.
* Benchmark on mid-range Android hardware and iOS devices.
* Generate automated release artifacts and developer documentation.
* Publish final Release-Readiness Report.
