# Security Architecture & Threat Model

This document specifies the security requirements, threat model, and defense-in-depth mitigations implemented in the SDK (`vaani-core`).

---

## 1. Threat Model & Mitigations

| Threat | Description | Vector | SDK Mitigation |
| :--- | :--- | :--- | :--- |
| **T1: Malicious Model Package** | Attacker crafts a corrupted or weaponized package containing malicious files or exploits. | Downloaded or imported `.tar.gz` / `.zip` archive. | **Ed25519 Digital Signatures:** Manifests must be signed by trusted public keys compiled into the SDK or configured by the host app. Unsigned/tampered packages are rejected before execution. |
| **T2: Path Traversal (Zip Slip)** | Package contains files with paths like `../../system/lib.so` attempting to overwrite arbitrary app files. | Archive unpacker. | **Path Sanitization:** Rejects any entry containing `..`, absolute paths, symlinks, or special device nodes. Enforces unpacking within the designated sandbox folder. |
| **T3: Decompression Bomb** | Tiny archive expanding to gigabytes of data to exhaust device storage or crash the process. | Malicious archive. | **Quotas & Ratio Checks:** Limits decompression ratio to $\le 10\times$ and max uncompressed package size to $500\text{ MB}$. Aborts extraction immediately if threshold exceeded. |
| **T4: Model File Tampering** | Attacker modifies weights or tokens on device storage to manipulate scoring. | Local disk modification. | **Cryptographic Verification:** SHA-256 checksums of all model files, token files, and rule files are verified against the signed manifest upon loading. |
| **T5: Memory-Safety Exploits** | Buffer overflows, out-of-bounds reads, or use-after-free in native C++ core. | Malformed PCM audio, corrupted ONNX files, invalid Unicode strings. | **Modern C++17 Practices:** RAII, smart pointers (`std::unique_ptr`), bounds-checked span views (`std::span`), AddressSanitizer/UndefinedBehaviorSanitizer testing, and minimal raw pointer usage. |
| **T6: Malformed Audio DoS** | Audio containing NaNs, infinities, or extreme length causing infinite loops or crashes. | Audio input buffer. | **Audio Validation:** Checks for sample rate (16 kHz), finite float bounds $[-1.0, 1.0]$, max duration limit (e.g. 60 seconds), and zero-length inputs before pipeline entry. |
| **T7: Concurrent Mutation** | App tries to download/update a language while an inference job is active. | Multithreaded app calls. | **Read-Write Locking:** Internal mutexes protect the package registry. Models in active sessions are reference-counted; updates stage to shadow directories and apply atomically. |
| **T8: Data Leakage & Telemetry** | Audio recordings or transcripts leaked to cloud servers or diagnostic logs. | Network stack or logging. | **Zero-Network Engine:** The core runtime contains **zero network libraries**. All HTTP/download capabilities are isolated strictly to the package downloader module, which can be excluded at compile time. Audio buffers are ephemeral and zeroized after scoring. |

---

## 2. Cryptographic Verification Pipeline

```text
Host App initiates package install:
  1. Download package archive (HTTPS with TLS 1.3 certificate pinning)
  2. Verify archive SHA-256 matches catalog entry
  3. Extract to staging directory: <app_dir>/vaani/staging/<pkg_id>.<random_uuid>/
  4. Parse manifest.json:
     - Enforce schema validation
     - Check min SDK version <= current SDK version
  5. Verify manifest.sig using Ed25519 public key from trusted key store:
     crypto_sign_verify_detached(sig, manifest_bytes, public_key) == SUCCESS
  6. For each artifact in manifest.artifacts:
     - Compute SHA-256 of file on disk
     - Compare with manifest.artifacts[i].sha256
     - If mismatch: abort, purge staging, return ERR_CHECKSUM_MISMATCH
  7. Atomic directory rename:
     rename(staging_path, installed_path)
```

---

## 3. Privacy & Audio Lifecycle

1. **No Persistence by Default:** Spoken audio is held strictly in volatile RAM during feature extraction and inference. The SDK never writes recorded audio to flash storage unless the host application explicitly configures a diagnostic debug mode.
2. **Buffer Zeroization:** Once feature extraction completes, raw PCM buffers can be wiped (`memset_s`) to avoid memory retention.
3. **No Cloud Telemetry:** No user IDs, device fingerprints, audio recordings, recognized text, or pronunciation scores are collected or transmitted by `vaani-core`.
4. **Microphone Permissions:** The core library does not directly interact with OS permission prompts. Hybrid bindings request audio permissions only when the recording session is initiated, and audio capture stops immediately upon user release or timeout.
