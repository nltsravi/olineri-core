#include "vaani/vaani_c_api.h"
#include "vaani/internal/package_manager.hpp"
#include "vaani/internal/scoring_engine.hpp"
#include "vaani/internal/inference_engine.hpp"
#include "vaani/internal/filesystem.hpp"
#include "vaani/internal/g2p.hpp"
#include "backends/mock/mock_engine.hpp"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstring>

/* ========================================================================= */
/* Internal Struct Definitions for Opaque Handles                            */
/* ========================================================================= */

struct vaani_session_s;

struct vaani_runtime_s {
    std::string storage_dir;
    std::unique_ptr<vaani::pkg::PackageManager> package_manager;
    std::mutex sessions_mutex;
    std::vector<vaani_session_s*> active_sessions;
};

struct vaani_session_s {
    vaani_runtime_s* runtime = nullptr;
    std::string language_tag;
    vaani::manifest::PackageManifest manifest;
    std::unique_ptr<vaani::inference::InferenceEngine> engine;
    std::unique_ptr<vaani::scoring::ScoringEngine> scorer;
    std::unique_ptr<vaani::g2p::BrahmicG2P> g2p;
};

struct vaani_result_s {
    vaani::scoring::AssessmentOutput output;
    std::vector<std::string> feedback_strings;
    std::vector<std::string> expected_phonemes;
    std::vector<std::string> detected_phonemes;
};

/* ========================================================================= */
/* C ABI Implementation                                                      */
/* ========================================================================= */

extern "C" {

VAANI_API const char* vaani_version(void) {
    return VAANI_VERSION_STRING;
}

VAANI_API const char* vaani_status_message(vaani_status_t status) {
    switch (status) {
        case VAANI_OK: return "Success";
        case VAANI_ERR_INVALID_ARGUMENT: return "Invalid argument provided";
        case VAANI_ERR_OUT_OF_MEMORY: return "Out of memory";
        case VAANI_ERR_STORAGE_UNWRITABLE: return "Storage directory is not writable";
        case VAANI_ERR_PACKAGE_NOT_FOUND: return "Language package not found";
        case VAANI_ERR_PACKAGE_CORRUPTED: return "Language package is corrupted or failed checksum verification";
        case VAANI_ERR_SIGNATURE_INVALID: return "Digital signature verification failed";
        case VAANI_ERR_INCOMPATIBLE_VERSION: return "Package is incompatible with this SDK version";
        case VAANI_ERR_PATH_TRAVERSAL: return "Security violation: archive contains path traversal sequence";
        case VAANI_ERR_ARCHIVE_BOMB: return "Security violation: archive exceeds safe decompression limit";
        case VAANI_ERR_AUDIO_INVALID: return "Audio buffer is empty, null, or contains invalid float values";
        case VAANI_ERR_AUDIO_TOO_QUIET: return "Audio level is too quiet or silent";
        case VAANI_ERR_AUDIO_CLIPPED: return "Audio is severely clipped or distorted";
        case VAANI_ERR_INFERENCE_FAILED: return "Acoustic inference execution failed";
        case VAANI_ERR_RESOURCE_BUSY: return "Resource is currently locked by an active session";
        case VAANI_ERR_CANCELLED: return "Operation was cancelled";
        case VAANI_ERR_NOT_INITIALIZED: return "Runtime or session is not initialized";
        case VAANI_ERR_INTERNAL: return "Internal runtime error";
        default: return "Unknown error";
    }
}

VAANI_API vaani_status_t vaani_runtime_create(
    const char* storage_directory,
    vaani_runtime_handle_t* out_runtime
) {
    if (!storage_directory || !out_runtime) {
        return VAANI_ERR_INVALID_ARGUMENT;
    }

    if (!vaani::fs::create_directories(storage_directory)) {
        return VAANI_ERR_STORAGE_UNWRITABLE;
    }

    try {
        auto rt = new vaani_runtime_s();
        rt->storage_dir = storage_directory;
        rt->package_manager = std::make_unique<vaani::pkg::PackageManager>(storage_directory);
        *out_runtime = rt;
        return VAANI_OK;
    } catch (...) {
        return VAANI_ERR_OUT_OF_MEMORY;
    }
}

VAANI_API vaani_status_t vaani_runtime_destroy(
    vaani_runtime_handle_t runtime
) {
    if (!runtime) return VAANI_ERR_INVALID_ARGUMENT;

    // Close any remaining active sessions
    {
        std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
        for (auto* session : runtime->active_sessions) {
            delete session;
        }
        runtime->active_sessions.clear();
    }

    delete runtime;
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_runtime_trim_memory(
    vaani_runtime_handle_t runtime
) {
    if (!runtime) return VAANI_ERR_INVALID_ARGUMENT;
    std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
    for (auto* s : runtime->active_sessions) {
        if (s && s->engine) {
            s->engine->unload();
        }
    }
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_runtime_add_trusted_key(
    vaani_runtime_handle_t runtime,
    const char* key_id,
    const char* public_key_hex
) {
    if (!runtime || !key_id || !public_key_hex) {
        return VAANI_ERR_INVALID_ARGUMENT;
    }
    runtime->package_manager->add_trusted_key(key_id, public_key_hex);
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_package_install(
    vaani_runtime_handle_t runtime,
    const char* package_path,
    char* out_pkg_id,
    size_t pkg_id_size
) {
    if (!runtime || !package_path) return VAANI_ERR_INVALID_ARGUMENT;

    std::string installed_id;
    vaani_status_t status = runtime->package_manager->install(package_path, installed_id);
    if (status != VAANI_OK) return status;

    if (out_pkg_id && pkg_id_size > 0) {
        strncpy(out_pkg_id, installed_id.c_str(), pkg_id_size - 1);
        out_pkg_id[pkg_id_size - 1] = '\0';
    }
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_package_uninstall(
    vaani_runtime_handle_t runtime,
    const char* package_id
) {
    if (!runtime || !package_id) return VAANI_ERR_INVALID_ARGUMENT;

    // Prevent uninstalling while active in a session
    {
        std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
        for (const auto* s : runtime->active_sessions) {
            if (s && s->manifest.package_id == package_id) {
                return VAANI_ERR_RESOURCE_BUSY;
            }
        }
    }

    return runtime->package_manager->uninstall(package_id);
}

VAANI_API vaani_status_t vaani_package_rollback(
    vaani_runtime_handle_t runtime,
    const char* package_id
) {
    if (!runtime || !package_id) return VAANI_ERR_INVALID_ARGUMENT;
    return runtime->package_manager->rollback(package_id);
}

VAANI_API uint32_t vaani_package_count(
    vaani_runtime_handle_t runtime
) {
    if (!runtime) return 0;
    return static_cast<uint32_t>(runtime->package_manager->list_installed().size());
}

VAANI_API vaani_status_t vaani_package_get_at(
    vaani_runtime_handle_t runtime,
    uint32_t index,
    char* out_package_id,
    size_t pkg_id_cap,
    char* out_language_tag,
    size_t lang_tag_cap,
    char* out_version,
    size_t version_cap
) {
    if (!runtime) return VAANI_ERR_INVALID_ARGUMENT;
    auto list = runtime->package_manager->list_installed();
    if (index >= list.size()) return VAANI_ERR_INVALID_ARGUMENT;

    const auto& pkg = list[index];
    if (out_package_id && pkg_id_cap > 0) {
        strncpy(out_package_id, pkg.package_id.c_str(), pkg_id_cap - 1);
        out_package_id[pkg_id_cap - 1] = '\0';
    }
    if (out_language_tag && lang_tag_cap > 0) {
        strncpy(out_language_tag, pkg.language_tag.c_str(), lang_tag_cap - 1);
        out_language_tag[lang_tag_cap - 1] = '\0';
    }
    if (out_version && version_cap > 0) {
        strncpy(out_version, pkg.version.c_str(), version_cap - 1);
        out_version[version_cap - 1] = '\0';
    }
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_session_create(
    vaani_runtime_handle_t runtime,
    const char* language_tag,
    vaani_session_handle_t* out_session
) {
    if (!runtime || !language_tag || !out_session) return VAANI_ERR_INVALID_ARGUMENT;

    vaani::manifest::PackageManifest manifest;
    if (!runtime->package_manager->find_by_language(language_tag, manifest)) {
        return VAANI_ERR_PACKAGE_NOT_FOUND;
    }

    try {
        auto session = new vaani_session_s();
        session->runtime = runtime;
        session->language_tag = language_tag;
        session->manifest = manifest;

        // In Milestone 2, use the deterministic MockEngine (swappable with ONNX/sherpa-onnx in M3)
        auto mock = std::make_unique<vaani::backends::MockEngine>(64, 0);
        std::string pkg_dir = runtime->package_manager->get_package_dir(manifest.package_id);
        mock->load_model(pkg_dir, manifest.model.id);

        session->engine = std::move(mock);
        session->scorer = std::make_unique<vaani::scoring::ScoringEngine>();
        session->g2p = std::make_unique<vaani::g2p::BrahmicG2P>(vaani::g2p::G2PConfig{language_tag, true, true});

        {
            std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
            runtime->active_sessions.push_back(session);
        }

        *out_session = session;
        return VAANI_OK;
    } catch (...) {
        return VAANI_ERR_OUT_OF_MEMORY;
    }
}

VAANI_API vaani_status_t vaani_session_destroy(
    vaani_session_handle_t session
) {
    if (!session || !session->runtime) return VAANI_ERR_INVALID_ARGUMENT;

    auto* rt = session->runtime;
    {
        std::lock_guard<std::mutex> lock(rt->sessions_mutex);
        auto it = std::find(rt->active_sessions.begin(), rt->active_sessions.end(), session);
        if (it != rt->active_sessions.end()) {
            rt->active_sessions.erase(it);
        }
    }

    delete session;
    return VAANI_OK;
}

VAANI_API vaani_status_t vaani_assess_pronunciation(
    vaani_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    vaani_result_handle_t* out_result
) {
    if (!session || !reference_text || !audio_samples || !out_result) {
        return VAANI_ERR_INVALID_ARGUMENT;
    }
    if (sample_count == 0 || sample_rate == 0) {
        return VAANI_ERR_AUDIO_INVALID;
    }

    // 1. Convert reference text using Unicode-aware Brahmic G2P engine
    std::vector<vaani::scoring::PhonemeTarget> targets = session->g2p->convert(reference_text);
    if (targets.empty()) {
        return VAANI_ERR_INVALID_ARGUMENT;
    }

    // For MockEngine: configure the expected token sequence
    auto* mock = dynamic_cast<vaani::backends::MockEngine*>(session->engine.get());
    if (mock) {
        std::vector<uint32_t> expected_tokens;
        for (const auto& t : targets) expected_tokens.push_back(t.token_id);
        mock->set_sequence_to_emit(expected_tokens);
    }

    // 2. Run inference to get frame acoustic posteriors
    vaani::scoring::AcousticEmission emission;
    vaani_status_t status = session->engine->infer(audio_samples, sample_count, sample_rate, emission);
    if (status != VAANI_OK) return status;

    // 3. Align and score
    auto output = session->scorer->assess(targets, emission);

    // 4. Wrap into result struct
    try {
        auto res = new vaani_result_s();
        res->output = std::move(output);

        // Pre-cache C-compatible strings
        for (const auto& ph : res->output.phonemes) {
            res->expected_phonemes.push_back(ph.expected_phoneme);
            res->detected_phonemes.push_back(ph.detected_phoneme);
            res->feedback_strings.push_back(ph.feedback_instruction);
        }

        *out_result = res;
        return VAANI_OK;
    } catch (...) {
        return VAANI_ERR_OUT_OF_MEMORY;
    }
}

VAANI_API float vaani_result_get_overall_score(
    vaani_result_handle_t result
) {
    if (!result) return 0.0f;
    return result->output.overall_score;
}

VAANI_API uint32_t vaani_result_get_phoneme_count(
    vaani_result_handle_t result
) {
    if (!result) return 0;
    return static_cast<uint32_t>(result->output.phonemes.size());
}

VAANI_API vaani_status_t vaani_result_get_phoneme_feedback(
    vaani_result_handle_t result,
    uint32_t index,
    vaani_phoneme_feedback_t* out_feedback
) {
    if (!result || !out_feedback) return VAANI_ERR_INVALID_ARGUMENT;
    if (index >= result->output.phonemes.size()) return VAANI_ERR_INVALID_ARGUMENT;

    const auto& src = result->output.phonemes[index];
    out_feedback->expected_phoneme = result->expected_phonemes[index].c_str();
    out_feedback->detected_phoneme = result->detected_phonemes[index].c_str();
    out_feedback->score = src.score;
    out_feedback->status = src.status;
    out_feedback->char_start_utf8 = src.char_start_utf8;
    out_feedback->char_end_utf8 = src.char_end_utf8;
    out_feedback->cluster_index = src.cluster_index;
    out_feedback->cluster_count = src.cluster_count;
    out_feedback->start_time_ms = src.start_time_ms;
    out_feedback->end_time_ms = src.end_time_ms;
    out_feedback->feedback_instruction = result->feedback_strings[index].c_str();

    return VAANI_OK;
}

VAANI_API const char* vaani_result_get_recognized_text(
    vaani_result_handle_t result
) {
    if (!result) return "";
    return result->output.recognized_text.c_str();
}

VAANI_API void vaani_result_destroy(
    vaani_result_handle_t result
) {
    delete result;
}

} // extern "C"
