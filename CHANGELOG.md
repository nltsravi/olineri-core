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
