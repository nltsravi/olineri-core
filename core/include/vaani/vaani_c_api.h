/**
 * @file vaani_c_api.h
 * @brief Stable C ABI for the Vaani Offline Indian-Language Pronunciation Assessment SDK.
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef VAANI_C_API_H
#define VAANI_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(VAANI_BUILD_SHARED)
    #define VAANI_API __declspec(dllexport)
  #else
    #define VAANI_API __declspec(dllimport)
  #endif
#else
  #define VAANI_API __attribute__((visibility("default")))
#endif

#define VAANI_VERSION_MAJOR 0
#define VAANI_VERSION_MINOR 1
#define VAANI_VERSION_PATCH 0
#define VAANI_VERSION_STRING "0.1.0"

/* ========================================================================= */
/* Status and Error Codes                                                    */
/* ========================================================================= */

typedef enum {
    VAANI_OK = 0,
    VAANI_ERR_INVALID_ARGUMENT = 1,
    VAANI_ERR_OUT_OF_MEMORY = 2,
    VAANI_ERR_STORAGE_UNWRITABLE = 3,
    VAANI_ERR_PACKAGE_NOT_FOUND = 4,
    VAANI_ERR_PACKAGE_CORRUPTED = 5,
    VAANI_ERR_SIGNATURE_INVALID = 6,
    VAANI_ERR_INCOMPATIBLE_VERSION = 7,
    VAANI_ERR_PATH_TRAVERSAL = 8,
    VAANI_ERR_ARCHIVE_BOMB = 9,
    VAANI_ERR_AUDIO_INVALID = 10,
    VAANI_ERR_AUDIO_TOO_QUIET = 11,
    VAANI_ERR_AUDIO_CLIPPED = 12,
    VAANI_ERR_INFERENCE_FAILED = 13,
    VAANI_ERR_RESOURCE_BUSY = 14,
    VAANI_ERR_CANCELLED = 15,
    VAANI_ERR_NOT_INITIALIZED = 16,
    VAANI_ERR_INTERNAL = 99
} vaani_status_t;

/* ========================================================================= */
/* Assessment & Phoneme Types                                                */
/* ========================================================================= */

typedef enum {
    VAANI_PHONEME_CORRECT = 0,
    VAANI_PHONEME_SUBSTITUTION = 1,
    VAANI_PHONEME_OMISSION = 2,
    VAANI_PHONEME_INSERTION = 3,
    VAANI_PHONEME_UNCERTAIN = 4
} vaani_phoneme_status_t;

typedef struct {
    const char* expected_phoneme;      /* Canonical IPA target (e.g. "/ɻ/") */
    const char* detected_phoneme;      /* Detected IPA sound if substitution (or empty if correct) */
    float score;                       /* Calibrated confidence score [0.0 - 100.0] */
    vaani_phoneme_status_t status;     /* Classification */
    uint32_t char_start_utf8;          /* Byte start offset in reference text */
    uint32_t char_end_utf8;            /* Byte end offset in reference text */
    uint32_t cluster_index;            /* Grapheme cluster (Akshara) index */
    uint32_t cluster_count;            /* Count of grapheme clusters covered */
    float start_time_ms;               /* Segment start time in audio (milliseconds) */
    float end_time_ms;                 /* Segment end time in audio (milliseconds) */
    const char* feedback_instruction;  /* Localized articulatory advice */
} vaani_phoneme_feedback_t;

/* ========================================================================= */
/* Opaque Handle Declarations                                                */
/* ========================================================================= */

typedef struct vaani_runtime_s* vaani_runtime_handle_t;
typedef struct vaani_session_s* vaani_session_handle_t;
typedef struct vaani_result_s*  vaani_result_handle_t;

/* ========================================================================= */
/* Runtime Lifecycle Functions                                               */
/* ========================================================================= */

/**
 * @brief Returns the version string of the Vaani SDK.
 */
VAANI_API const char* vaani_version(void);

/**
 * @brief Translates a status code into a human-readable English error string.
 */
VAANI_API const char* vaani_status_message(vaani_status_t status);

/**
 * @brief Initializes the Vaani native runtime.
 *
 * @param storage_directory Directory for private SDK data, package installations, and cache.
 *                          Must be writable by the host application process.
 * @param out_runtime       Receives the initialized runtime handle.
 */
VAANI_API vaani_status_t vaani_runtime_create(
    const char* storage_directory,
    vaani_runtime_handle_t* out_runtime
);

/**
 * @brief Destroys the runtime, closing all active sessions and releasing memory.
 */
VAANI_API vaani_status_t vaani_runtime_destroy(
    vaani_runtime_handle_t runtime
);

/**
 * @brief Releases idle model weights and cached memory back to the OS.
 *        Intended to be called when mobile OS emits low memory callbacks.
 */
VAANI_API vaani_status_t vaani_runtime_trim_memory(
    vaani_runtime_handle_t runtime
);

/**
 * @brief Adds a trusted Ed25519 public key (32 bytes raw or 64 hex characters)
 *        used for verifying language package signatures.
 */
VAANI_API vaani_status_t vaani_runtime_add_trusted_key(
    vaani_runtime_handle_t runtime,
    const char* key_id,
    const char* public_key_hex
);

/* ========================================================================= */
/* Language Package Management                                               */
/* ========================================================================= */

/**
 * @brief Installs a language package from a local directory or unpacks from archive.
 *        Performs cryptographic signature validation, SHA-256 verification of all artifacts,
 *        and atomic installation.
 *
 * @param runtime       Runtime handle.
 * @param package_path  Local path to a package directory or package archive.
 * @param out_pkg_id    Buffer to receive the installed package ID (e.g. "lang-ta").
 * @param pkg_id_size   Capacity of out_pkg_id buffer.
 */
VAANI_API vaani_status_t vaani_package_install(
    vaani_runtime_handle_t runtime,
    const char* package_path,
    char* out_pkg_id,
    size_t pkg_id_size
);

/**
 * @brief Uninstalls a language package and deletes its local files.
 */
VAANI_API vaani_status_t vaani_package_uninstall(
    vaani_runtime_handle_t runtime,
    const char* package_id
);

/**
 * @brief Rolls back an updated package to its previous working version.
 */
VAANI_API vaani_status_t vaani_package_rollback(
    vaani_runtime_handle_t runtime,
    const char* package_id
);

/**
 * @brief Returns the count of currently installed language packages.
 */
VAANI_API uint32_t vaani_package_count(
    vaani_runtime_handle_t runtime
);

/**
 * @brief Retrieves information about an installed language package by index.
 */
VAANI_API vaani_status_t vaani_package_get_at(
    vaani_runtime_handle_t runtime,
    uint32_t index,
    char* out_package_id,
    size_t pkg_id_cap,
    char* out_language_tag,
    size_t lang_tag_cap,
    char* out_version,
    size_t version_cap
);

/* ========================================================================= */
/* Session & Pronunciation Assessment                                        */
/* ========================================================================= */

/**
 * @brief Creates an assessment session for a specific language tag (e.g. "ta-IN", "sa-IN").
 *        Loads the corresponding language package and initializes the acoustic model.
 */
VAANI_API vaani_status_t vaani_session_create(
    vaani_runtime_handle_t runtime,
    const char* language_tag,
    vaani_session_handle_t* out_session
);

/**
 * @brief Destroys an assessment session, unloading its model weights from memory.
 */
VAANI_API vaani_status_t vaani_session_destroy(
    vaani_session_handle_t session
);

/**
 * @brief Assesses the pronunciation of the provided audio against reference text.
 *
 * @param session         Active language session handle.
 * @param reference_text  Expected target text in native script (Unicode NFC).
 * @param audio_samples   Pointer to mono 32-bit float PCM audio samples [-1.0, 1.0].
 * @param sample_count    Number of audio samples.
 * @param sample_rate     Sampling rate in Hz (typically 16000).
 * @param out_result      Receives the assessment result handle.
 */
VAANI_API vaani_status_t vaani_assess_pronunciation(
    vaani_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    vaani_result_handle_t* out_result
);

/* ========================================================================= */
/* Result Inspection & Cleanup                                               */
/* ========================================================================= */

/**
 * @brief Returns the calibrated overall pronunciation score [0.0 - 100.0].
 */
VAANI_API float vaani_result_get_overall_score(
    vaani_result_handle_t result
);

/**
 * @brief Returns the total count of phoneme segments evaluated.
 */
VAANI_API uint32_t vaani_result_get_phoneme_count(
    vaani_result_handle_t result
);

/**
 * @brief Retrieves detailed feedback for a specific phoneme segment.
 */
VAANI_API vaani_status_t vaani_result_get_phoneme_feedback(
    vaani_result_handle_t result,
    uint32_t index,
    vaani_phoneme_feedback_t* out_feedback
);

/**
 * @brief Retrieves the recognized text transcript, if available from ASR decoding.
 */
VAANI_API const char* vaani_result_get_recognized_text(
    vaani_result_handle_t result
);

/**
 * @brief Destroys the assessment result and frees its allocated structures.
 */
VAANI_API void vaani_result_destroy(
    vaani_result_handle_t result
);

#ifdef __cplusplus
}
#endif

#endif /* VAANI_C_API_H */
