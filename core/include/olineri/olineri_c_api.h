/**
 * @file olineri_c_api.h
 * @brief Stable C ABI for the Olineri Offline Indian-Language Pronunciation Assessment SDK.
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef OLINERI_C_API_H
#define OLINERI_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(OLINERI_BUILD_SHARED)
    #define OLINERI_API __declspec(dllexport)
  #else
    #define OLINERI_API __declspec(dllimport)
  #endif
#else
  #define OLINERI_API __attribute__((visibility("default")))
#endif

#define OLINERI_VERSION_MAJOR 0
#define OLINERI_VERSION_MINOR 1
#define OLINERI_VERSION_PATCH 0
#define OLINERI_VERSION_STRING "0.1.0"

/* ========================================================================= */
/* Status and Error Codes                                                    */
/* ========================================================================= */

typedef enum {
    OLINERI_OK = 0,
    OLINERI_ERR_INVALID_ARGUMENT = 1,
    OLINERI_ERR_OUT_OF_MEMORY = 2,
    OLINERI_ERR_STORAGE_UNWRITABLE = 3,
    OLINERI_ERR_PACKAGE_NOT_FOUND = 4,
    OLINERI_ERR_PACKAGE_CORRUPTED = 5,
    OLINERI_ERR_SIGNATURE_INVALID = 6,
    OLINERI_ERR_INCOMPATIBLE_VERSION = 7,
    OLINERI_ERR_PATH_TRAVERSAL = 8,
    OLINERI_ERR_ARCHIVE_BOMB = 9,
    OLINERI_ERR_AUDIO_INVALID = 10,
    OLINERI_ERR_AUDIO_TOO_QUIET = 11,
    OLINERI_ERR_AUDIO_CLIPPED = 12,
    OLINERI_ERR_INFERENCE_FAILED = 13,
    OLINERI_ERR_RESOURCE_BUSY = 14,
    OLINERI_ERR_CANCELLED = 15,
    OLINERI_ERR_NOT_INITIALIZED = 16,
    OLINERI_ERR_INTERNAL = 99
} olineri_status_t;

/* ========================================================================= */
/* Assessment & Phoneme Types                                                */
/* ========================================================================= */

typedef enum {
    OLINERI_PHONEME_CORRECT = 0,
    OLINERI_PHONEME_SUBSTITUTION = 1,
    OLINERI_PHONEME_OMISSION = 2,
    OLINERI_PHONEME_INSERTION = 3,
    OLINERI_PHONEME_UNCERTAIN = 4
} olineri_phoneme_status_t;

typedef struct {
    const char* expected_phoneme;      /* Canonical IPA target (e.g. "/ɻ/") */
    const char* detected_phoneme;      /* Detected IPA sound if substitution (or empty if correct) */
    float score;                       /* Calibrated confidence score [0.0 - 100.0] */
    olineri_phoneme_status_t status;   /* Classification */
    uint32_t char_start_utf8;          /* Byte start offset in reference text */
    uint32_t char_end_utf8;            /* Byte end offset in reference text */
    uint32_t cluster_index;            /* Grapheme cluster (Akshara) index */
    uint32_t cluster_count;            /* Count of grapheme clusters covered */
    float start_time_ms;               /* Segment start time in audio (milliseconds) */
    float end_time_ms;                 /* Segment end time in audio (milliseconds) */
    const char* feedback_instruction;  /* Localized articulatory advice */
} olineri_phoneme_feedback_t;

/* ========================================================================= */
/* Opaque Handle Declarations                                                */
/* ========================================================================= */

typedef struct olineri_runtime_s* olineri_runtime_handle_t;
typedef struct olineri_session_s* olineri_session_handle_t;
typedef struct olineri_result_s*  olineri_result_handle_t;

/* ========================================================================= */
/* Runtime Lifecycle Functions                                               */
/* ========================================================================= */

/**
 * @brief Returns the version string of the Olineri SDK.
 */
OLINERI_API const char* olineri_version(void);

/**
 * @brief Translates a status code into a human-readable English error string.
 */
OLINERI_API const char* olineri_status_message(olineri_status_t status);

/**
 * @brief Initializes the Olineri native runtime.
 *
 * @param storage_directory Directory for private SDK data, package installations, and cache.
 *                          Must be writable by the host application process.
 * @param out_runtime       Receives the initialized runtime handle.
 */
OLINERI_API olineri_status_t olineri_runtime_create(
    const char* storage_directory,
    olineri_runtime_handle_t* out_runtime
);

/**
 * @brief Destroys the runtime, closing all active sessions and releasing memory.
 */
OLINERI_API olineri_status_t olineri_runtime_destroy(
    olineri_runtime_handle_t runtime
);

/**
 * @brief Releases idle model weights and cached memory back to the OS.
 *        Intended to be called when mobile OS emits low memory callbacks.
 */
OLINERI_API olineri_status_t olineri_runtime_trim_memory(
    olineri_runtime_handle_t runtime
);

/**
 * @brief Adds a trusted Ed25519 public key (32 bytes raw or 64 hex characters)
 *        used for verifying language package signatures.
 */
OLINERI_API olineri_status_t olineri_runtime_add_trusted_key(
    olineri_runtime_handle_t runtime,
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
OLINERI_API olineri_status_t olineri_package_install(
    olineri_runtime_handle_t runtime,
    const char* package_path,
    char* out_pkg_id,
    size_t pkg_id_size
);

/**
 * @brief Uninstalls a language package and deletes its local files.
 */
OLINERI_API olineri_status_t olineri_package_uninstall(
    olineri_runtime_handle_t runtime,
    const char* package_id
);

/**
 * @brief Rolls back an updated package to its previous working version.
 */
OLINERI_API olineri_status_t olineri_package_rollback(
    olineri_runtime_handle_t runtime,
    const char* package_id
);

/**
 * @brief Returns the count of currently installed language packages.
 */
OLINERI_API uint32_t olineri_package_count(
    olineri_runtime_handle_t runtime
);

/**
 * @brief Retrieves information about an installed language package by index.
 */
OLINERI_API olineri_status_t olineri_package_get_at(
    olineri_runtime_handle_t runtime,
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
OLINERI_API olineri_status_t olineri_session_create(
    olineri_runtime_handle_t runtime,
    const char* language_tag,
    olineri_session_handle_t* out_session
);

/**
 * @brief Destroys an assessment session, unloading its model weights from memory.
 */
OLINERI_API olineri_status_t olineri_session_destroy(
    olineri_session_handle_t session
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
OLINERI_API olineri_status_t olineri_assess_pronunciation(
    olineri_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    olineri_result_handle_t* out_result
);

/* ========================================================================= */
/* Result Inspection & Cleanup                                               */
/* ========================================================================= */

/**
 * @brief Returns the calibrated overall pronunciation score [0.0 - 100.0].
 */
OLINERI_API float olineri_result_get_overall_score(
    olineri_result_handle_t result
);

/**
 * @brief Returns the total count of phoneme segments evaluated.
 */
OLINERI_API uint32_t olineri_result_get_phoneme_count(
    olineri_result_handle_t result
);

/**
 * @brief Retrieves detailed feedback for a specific phoneme segment.
 */
OLINERI_API olineri_status_t olineri_result_get_phoneme_feedback(
    olineri_result_handle_t result,
    uint32_t index,
    olineri_phoneme_feedback_t* out_feedback
);

/**
 * @brief Retrieves the recognized text transcript, if available from ASR decoding.
 */
OLINERI_API const char* olineri_result_get_recognized_text(
    olineri_result_handle_t result
);

/**
 * @brief Destroys the assessment result and frees its allocated structures.
 */
OLINERI_API void olineri_result_destroy(
    olineri_result_handle_t result
);

#ifdef __cplusplus
}
#endif

#endif /* OLINERI_C_API_H */
