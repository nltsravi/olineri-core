# Vaani Core SDK — Feature & Language Support Matrix

This matrix documents the verification and implementation status across platforms and languages for **v0.1.0**.

---

## 1. Language Support Matrix

| Language | BCP-47 Tag | Script | G2P Conversion & Akshara Spans | Phoneme Inventory | Articulatory Feedback | Acoustic Model Architecture | Model Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Tamil** | `ta-IN` | Tamil (`Taml`) | **Implemented & Tested** | 31 phonemes | **Implemented** (ta, en) | IndicConformer CTC INT8 | **Verified & Ready** |
| **Sanskrit** | `sa-IN` | Devanagari (`Deva`) | **Implemented & Tested** | 46 phonemes | **Implemented** (en) | IndicConformer CTC INT8 | **Verified & Ready** |
| **Hindi** | `hi-IN` | Devanagari (`Deva`) | **Implemented & Tested** | 46 phonemes | **Implemented** (en) | IndicConformer CTC INT8 | **Verified & Ready** |
| **Telugu** | `te-IN` | Telugu (`Telu`) | **Implemented & Tested** | 48 phonemes | **Implemented** (en) | IndicConformer CTC INT8 | **Verified & Ready** |

---

## 2. Core Capability Matrix

| Capability | Module | Implementation Status | Notes |
| :--- | :--- | :--- | :--- |
| **Stable C ABI** | Module A | **Production Ready** | Zero-dependency C11/C++17 header with opaque handles |
| **Zero-Telemetry Offline Execution** | Module A | **Production Ready** | 100% offline during inference. Zero network calls |
| **Atomic Package Installer** | Module B | **Production Ready** | Atomic staging directory swap, rollback on error |
| **Ed25519 Signature Verification** | Module B | **Production Ready** | Embedded TweetNaCl public-domain implementation |
| **SHA-256 Hash Verification** | Module B | **Production Ready** | FIPS 180-4 compliant digest verification for all artifacts |
| **Path Traversal / Zip-Slip Defense** | Module B | **Production Ready** | Rejects `..`, symlinks, and absolute target paths |
| **Constrained CTC Viterbi Trellis** | Module D | **Production Ready** | Strict dynamic programming forced alignment |
| **Goodness of Pronunciation (GOP)** | Module D | **Production Ready** | Normalized log-posterior ratio per segment |
| **Error Taxonomy Classification** | Module D | **Production Ready** | Correct, Substitution, Omission, Insertion, Uncertain |
| **Akshara Grapheme-Span Highlighting** | Module E | **Production Ready** | Maps phoneme errors back to exact UTF-8 byte ranges |
| **Streaming Intermediate Transcription** | Module C | *Proposed / Future Roadmap* | Requires Sherpa-ONNX streaming decoder integration |
| **Text-to-Speech Reference Synthesis** | Module F | *Proposed / Future Roadmap* | Planned via VITS/Piper ONNX in future release |

---

## 3. Platform & Framework Bindings Matrix

| Platform / Framework | Binding Layer | Status | Delivery Artifact |
| :--- | :--- | :--- | :--- |
| **Android** | Kotlin / JNI | **Implemented** | `com.vaani.core.VaaniSDK` + `libvaani_jni.so` |
| **iOS** | Swift / SPM | **Implemented** | `VaaniSDK` Swift Package + Clang modulemap |
| **React Native** | TypeScript + Bridge | **Implemented** | `@vaani/react-native-sdk` (Android Kotlin + iOS Obj-C++) |
| **Flutter** | Dart / FFI | **Implemented** | `vaani_flutter` package with `dart:ffi` |
| **Capacitor / Ionic** | TypeScript + Plugin | **Implemented** | `@vaani/capacitor-plugin` with web fallback |
| **WebAssembly** | Emscripten | *Candidate Prototype* | C++ core compiles cleanly with emscripten |
