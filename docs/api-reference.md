# Vaani Core SDK — API Reference Manual

The **Vaani Core SDK** provides a stable, zero-dependency C11/C++17 ABI, with native bindings for Android (Kotlin/JNI), iOS (Swift), React Native (TypeScript), and Flutter (Dart/FFI).

---

## 1. Core C ABI (`vaani_c_api.h`)

All C APIs return a `vaani_status_t` error code. Pointers returned by `out_*` parameters are owned by the SDK or must be released using corresponding `vaani_*_destroy` functions.

### Status Codes (`vaani_status_t`)

| Code | Enumeration | Description |
| :--- | :--- | :--- |
| `0` | `VAANI_OK` | Operation completed successfully |
| `-1` | `VAANI_ERROR_INVALID_ARGUMENT` | Null pointer or out-of-range parameter |
| `-2` | `VAANI_ERROR_NOT_INITIALIZED` | Runtime or session handle was not initialized |
| `-3` | `VAANI_ERROR_PACKAGE_NOT_FOUND` | Requested language package is not installed |
| `-4` | `VAANI_ERROR_CORRUPTED_PACKAGE` | SHA-256 hash or Ed25519 signature mismatch |
| `-5` | `VAANI_ERROR_INCOMPATIBLE_VERSION` | Manifest schema or SDK version mismatch |
| `-6` | `VAANI_ERROR_INSUFFICIENT_STORAGE` | Not enough disk space to install package |
| `-7` | `VAANI_ERROR_INFERENCE_FAILED` | Acoustic scoring failure or empty trellis |
| `-8` | `VAANI_ERROR_OUT_OF_MEMORY` | Memory allocation limit exceeded |

---

### Runtime Lifecycle

#### `vaani_runtime_create`
```c
VAANI_API vaani_status_t vaani_runtime_create(
    const char* storage_directory,
    vaani_runtime_handle_t* out_runtime
);
```
Initializes the master offline runtime. Scans `storage_directory` for previously verified language packages.

#### `vaani_runtime_destroy`
```c
VAANI_API void vaani_runtime_destroy(vaani_runtime_handle_t runtime);
```
Gracefully shuts down the runtime, unloading active inference sessions and releasing cached assets.

#### `vaani_runtime_trim_memory`
```c
VAANI_API void vaani_runtime_trim_memory(vaani_runtime_handle_t runtime);
```
Releases idle cached acoustic models in low-memory situations.

---

### Language Package Manager

#### `vaani_package_install`
```c
VAANI_API vaani_status_t vaani_package_install(
    vaani_runtime_handle_t runtime,
    const char* package_directory_path,
    char* out_package_id,
    uint32_t max_id_len
);
```
Verifies Ed25519 digital signatures and SHA-256 hashes of all artifacts defined in `manifest.json`. Installs the package atomically.

#### `vaani_package_uninstall`
```c
VAANI_API vaani_status_t vaani_package_uninstall(
    vaani_runtime_handle_t runtime,
    const char* package_id
);
```
Safely deletes package files from storage. Retains shared deduplicated assets if still required by other installed languages.

#### `vaani_package_get_installed_count`
```c
VAANI_API vaani_status_t vaani_package_get_installed_count(
    vaani_runtime_handle_t runtime,
    uint32_t* out_count
);
```
Returns the number of verified, ready-to-use packages on local storage.

---

### Assessment Session & Inference

#### `vaani_session_create`
```c
VAANI_API vaani_status_t vaani_session_create(
    vaani_runtime_handle_t runtime,
    const char* language_tag,
    vaani_session_handle_t* out_session
);
```
Spawns an active inference session for the given BCP-47 language tag (e.g. `"ta-IN"`, `"sa-IN"`, `"hi-IN"`, `"te-IN"`).

#### `vaani_session_destroy`
```c
VAANI_API void vaani_session_destroy(vaani_session_handle_t session);
```
Destroys the session and frees working acoustic memory.

#### `vaani_assess_pronunciation`
```c
VAANI_API vaani_status_t vaani_assess_pronunciation(
    vaani_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    vaani_result_handle_t* out_result
);
```
Performs forced Viterbi alignment and Goodness of Pronunciation (GOP) segmental scoring on 16 kHz single-channel Float PCM samples.

---

### Assessment Results Inspection

#### `vaani_result_get_overall_score`
```c
VAANI_API float vaani_result_get_overall_score(vaani_result_handle_t result);
```
Returns a calibrated score between `0.0` and `100.0`.

#### `vaani_result_get_phoneme_count`
```c
VAANI_API uint32_t vaani_result_get_phoneme_count(vaani_result_handle_t result);
```
Returns the number of target phonemes evaluated.

#### `vaani_result_get_phoneme_feedback`
```c
VAANI_API vaani_status_t vaani_result_get_phoneme_feedback(
    vaani_result_handle_t result,
    uint32_t index,
    vaani_phoneme_feedback_t* out_feedback
);
```
Retrieves the diagnostic record for a specific phoneme:
- `expected_phoneme`: IPA string (e.g. `"/ɻ/"`).
- `status`: `VAANI_PHONEME_CORRECT`, `VAANI_PHONEME_SUBSTITUTION`, `VAANI_PHONEME_OMISSION`, `VAANI_PHONEME_INSERTION`, or `VAANI_PHONEME_UNCERTAIN`.
- `score`: Sigmoid-calibrated log-posterior ratio (`0.0`–`100.0`).
- `char_start_utf8`, `char_end_utf8`: Exact UTF-8 byte span in `reference_text` corresponding to the affected Akshara grapheme cluster.

#### `vaani_result_destroy`
```c
VAANI_API void vaani_result_destroy(vaani_result_handle_t result);
```
Releases the assessment result handle.
