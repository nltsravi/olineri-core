# Olineri Core SDK — API Reference Manual

The **Olineri Core SDK** provides a stable, zero-dependency C11/C++17 ABI, with native bindings for Android (Kotlin/JNI), iOS (Swift), React Native (TypeScript), Flutter (Dart/FFI), and Capacitor.

---

## 1. Core C ABI (`olineri_c_api.h`)

All C APIs return an `olineri_status_t` error code. Pointers returned by `out_*` parameters are owned by the SDK or must be released using corresponding `olineri_*_destroy` functions.

### Status Codes (`olineri_status_t`)

| Code | Enumeration | Description |
| :--- | :--- | :--- |
| `0` | `OLINERI_OK` | Operation completed successfully |
| `-1` | `OLINERI_ERR_INVALID_ARGUMENT` | Null pointer or out-of-range parameter |
| `-2` | `OLINERI_ERR_NOT_INITIALIZED` | Runtime or session handle was not initialized |
| `-3` | `OLINERI_ERR_PACKAGE_NOT_FOUND` | Requested language package is not installed |
| `-4` | `OLINERI_ERR_PACKAGE_CORRUPTED` | SHA-256 hash or Ed25519 signature mismatch |
| `-5` | `OLINERI_ERR_INCOMPATIBLE_VERSION` | Manifest schema or SDK version mismatch |
| `-6` | `OLINERI_ERR_INSUFFICIENT_STORAGE` | Not enough disk space to install package |
| `-7` | `OLINERI_ERR_INFERENCE_FAILED` | Acoustic scoring failure or empty trellis |
| `-8` | `OLINERI_ERR_OUT_OF_MEMORY` | Memory allocation limit exceeded |
| `-9` | `OLINERI_ERR_PATH_TRAVERSAL` | Malicious path traversal sequence in package archive |

---

### Runtime Lifecycle

#### `olineri_runtime_create`
```c
OLINERI_API olineri_status_t olineri_runtime_create(
    const char* storage_directory,
    olineri_runtime_handle_t* out_runtime
);
```
Initializes the master offline runtime. Scans `storage_directory` for previously verified language packages.

#### `olineri_runtime_destroy`
```c
OLINERI_API olineri_status_t olineri_runtime_destroy(olineri_runtime_handle_t runtime);
```
Gracefully shuts down the runtime, unloading active inference sessions and releasing cached assets.

#### `olineri_runtime_trim_memory`
```c
OLINERI_API void olineri_runtime_trim_memory(olineri_runtime_handle_t runtime);
```
Releases idle cached acoustic models in low-memory situations.

---

### Language Package Manager

#### `olineri_package_install`
```c
OLINERI_API olineri_status_t olineri_package_install(
    olineri_runtime_handle_t runtime,
    const char* package_directory_path,
    char* out_package_id,
    uint32_t max_id_len
);
```
Verifies Ed25519 digital signatures and SHA-256 hashes of all artifacts defined in `manifest.json`. Installs the package atomically.

#### `olineri_package_uninstall`
```c
OLINERI_API olineri_status_t olineri_package_uninstall(
    olineri_runtime_handle_t runtime,
    const char* package_id
);
```
Safely deletes package files from storage. Retains shared deduplicated assets if still required by other installed languages.

#### `olineri_package_count`
```c
OLINERI_API uint32_t olineri_package_count(olineri_runtime_handle_t runtime);
```
Returns the number of verified, ready-to-use packages on local storage.

---

### Assessment Session & Inference

#### `olineri_session_create`
```c
OLINERI_API olineri_status_t olineri_session_create(
    olineri_runtime_handle_t runtime,
    const char* language_tag,
    olineri_session_handle_t* out_session
);
```
Spawns an active inference session for the given BCP-47 language tag (e.g. `"ta-IN"`, `"sa-IN"`, `"hi-IN"`, `"te-IN"`).

#### `olineri_session_destroy`
```c
OLINERI_API olineri_status_t olineri_session_destroy(olineri_session_handle_t session);
```
Destroys the session and frees working acoustic memory.

#### `olineri_assess_pronunciation`
```c
OLINERI_API olineri_status_t olineri_assess_pronunciation(
    olineri_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    olineri_result_handle_t* out_result
);
```
Performs forced Viterbi alignment and Goodness of Pronunciation (GOP) segmental scoring on 16 kHz single-channel Float PCM samples.

---

### Assessment Results Inspection

#### `olineri_result_get_overall_score`
```c
OLINERI_API float olineri_result_get_overall_score(olineri_result_handle_t result);
```
Returns a calibrated score between `0.0` and `100.0`.

#### `olineri_result_get_phoneme_count`
```c
OLINERI_API uint32_t olineri_result_get_phoneme_count(olineri_result_handle_t result);
```
Returns the number of target phonemes evaluated.

#### `olineri_result_get_phoneme_feedback`
```c
OLINERI_API olineri_status_t olineri_result_get_phoneme_feedback(
    olineri_result_handle_t result,
    uint32_t index,
    olineri_phoneme_feedback_t* out_feedback
);
```
Retrieves the diagnostic record for a specific phoneme:
- `expected_phoneme`: IPA string (e.g. `"/ɻ/"`).
- `status`: `OLINERI_PHONEME_CORRECT`, `OLINERI_PHONEME_SUBSTITUTION`, `OLINERI_PHONEME_OMISSION`, `OLINERI_PHONEME_INSERTION`, or `OLINERI_PHONEME_UNCERTAIN`.
- `score`: Sigmoid-calibrated log-posterior ratio (`0.0`–`100.0`).
- `char_start_utf8`, `char_end_utf8`: Exact UTF-8 byte span in `reference_text` corresponding to the affected Akshara grapheme cluster.

#### `olineri_result_destroy`
```c
OLINERI_API void olineri_result_destroy(olineri_result_handle_t result);
```
Releases the assessment result handle.
