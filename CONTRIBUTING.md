# Contributing to Olineri

Thank you for your interest in contributing to **Olineri**, the offline Indian-language pronunciation assessment SDK.

---

## 1. Code of Conduct & Principles

* **Offline-First:** No PR will be merged that introduces mandatory cloud dependencies, telemetry, or remote telemetry logging into the core engine.
* **Commercially Compatible:** All code, models, and dependencies must adhere to Apache 2.0, MIT, or compatible permissive licenses. Do not submit code or weights with `CC-BY-NC` or non-commercial restrictions.
* **Linguistic Integrity:** Phonological rules for Sanskrit, Tamil, Hindi, and Telugu must be backed by authoritative linguistic references, not ad-hoc heuristics.

---

## 2. Development Workflow

1. Fork the repository and create a feature branch (`feature/your-feature-name`).
2. Adhere to **C++17** modern idioms (smart pointers, RAII, zero raw pointer ownership across API boundaries).
3. Ensure all tests pass:
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j4
   ctest --output-on-failure
   ```
4. Submit a Pull Request with a clear description of changes, tests added, and any impacts on mobile memory or binary size.
