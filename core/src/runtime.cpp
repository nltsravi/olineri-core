#include "olineri/olineri_c_api.h"
#include "olineri/internal/package_manager.hpp"
#include "olineri/internal/scoring_engine.hpp"
#include "olineri/internal/inference_engine.hpp"
#include "olineri/internal/filesystem.hpp"
#include "olineri/internal/g2p.hpp"
#include "backends/mock/mock_engine.hpp"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <cstring>

extern "C" {

struct olineri_session_s;

struct olineri_runtime_s {
    std::string storage_dir;
    std::unique_ptr<olineri::pkg::PackageManager> package_manager;
    std::mutex sessions_mutex;
    std::vector<olineri_session_s*> active_sessions;
};

struct olineri_session_s {
    olineri_runtime_s* runtime = nullptr;
    std::string language_tag;
    olineri::manifest::PackageManifest manifest;
    std::unique_ptr<olineri::inference::InferenceEngine> engine;
    std::unique_ptr<olineri::scoring::ScoringEngine> scorer;
    std::unique_ptr<olineri::g2p::BrahmicG2P> g2p;
};

struct olineri_result_s {
    olineri::scoring::AssessmentOutput output;
    std::vector<std::string> expected_phonemes;
    std::vector<std::string> detected_phonemes;
    std::vector<std::string> feedback_strings;
};

/* ========================================================================= */
/* Runtime Lifecycle Functions                                               */
/* ========================================================================= */

OLINERI_API const char* olineri_version(void) {
    return OLINERI_VERSION_STRING;
}

OLINERI_API const char* olineri_status_message(olineri_status_t status) {
    switch (status) {
        case OLINERI_OK: return "Success";
        case OLINERI_ERR_INVALID_ARGUMENT: return "Invalid argument or null pointer";
        case OLINERI_ERR_OUT_OF_MEMORY: return "Out of memory";
        case OLINERI_ERR_STORAGE_UNWRITABLE: return "Storage directory unwritable or disk full";
        case OLINERI_ERR_PACKAGE_NOT_FOUND: return "Language package not found";
        case OLINERI_ERR_PACKAGE_CORRUPTED: return "Package corrupted or artifact hash mismatch";
        case OLINERI_ERR_SIGNATURE_INVALID: return "Package digital signature invalid or untrusted";
        case OLINERI_ERR_INCOMPATIBLE_VERSION: return "Package incompatible with SDK version";
        case OLINERI_ERR_PATH_TRAVERSAL: return "Security violation: path traversal detected";
        case OLINERI_ERR_ARCHIVE_BOMB: return "Security violation: archive bomb detected";
        case OLINERI_ERR_AUDIO_INVALID: return "Invalid audio format, duration, or sample rate";
        case OLINERI_ERR_AUDIO_TOO_QUIET: return "Audio level too quiet or below noise floor";
        case OLINERI_ERR_AUDIO_CLIPPED: return "Audio clipped or severely distorted";
        case OLINERI_ERR_INFERENCE_FAILED: return "Acoustic model inference failed";
        case OLINERI_ERR_RESOURCE_BUSY: return "Inference resource busy or limit reached";
        case OLINERI_ERR_CANCELLED: return "Operation cancelled by host application";
        case OLINERI_ERR_NOT_INITIALIZED: return "Runtime or session not initialized";
        case OLINERI_ERR_INTERNAL: return "Internal engine error";
        default: return "Unknown status code";
    }
}

OLINERI_API olineri_status_t olineri_runtime_create(
    const char* storage_directory,
    olineri_runtime_handle_t* out_runtime
) {
    if (!storage_directory || !out_runtime) {
        return OLINERI_ERR_INVALID_ARGUMENT;
    }

    if (!olineri::fs::create_directories(storage_directory)) {
        return OLINERI_ERR_STORAGE_UNWRITABLE;
    }

    try {
        auto rt = new olineri_runtime_s();
        rt->storage_dir = storage_directory;
        rt->package_manager = std::make_unique<olineri::pkg::PackageManager>(storage_directory);
        *out_runtime = rt;
        return OLINERI_OK;
    } catch (...) {
        return OLINERI_ERR_OUT_OF_MEMORY;
    }
}

OLINERI_API olineri_status_t olineri_runtime_destroy(
    olineri_runtime_handle_t runtime
) {
    if (!runtime) return OLINERI_ERR_INVALID_ARGUMENT;

    // Clean up all active sessions
    {
        std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
        for (auto* session : runtime->active_sessions) {
            delete session;
        }
        runtime->active_sessions.clear();
    }

    delete runtime;
    return OLINERI_OK;
}

OLINERI_API olineri_status_t olineri_runtime_trim_memory(
    olineri_runtime_handle_t runtime
) {
    if (!runtime) return OLINERI_ERR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
    for (auto* session : runtime->active_sessions) {
        if (session->engine) {
            session->engine->unload();
        }
    }
    return OLINERI_OK;
}

OLINERI_API olineri_status_t olineri_runtime_add_trusted_key(
    olineri_runtime_handle_t runtime,
    const char* key_id,
    const char* public_key_hex
) {
    if (!runtime || !key_id || !public_key_hex) {
        return OLINERI_ERR_INVALID_ARGUMENT;
    }
    runtime->package_manager->add_trusted_key(key_id, public_key_hex);
    return OLINERI_OK;
}

OLINERI_API olineri_status_t olineri_package_install(
    olineri_runtime_handle_t runtime,
    const char* package_path,
    char* out_pkg_id,
    size_t pkg_id_size
) {
    if (!runtime || !package_path) return OLINERI_ERR_INVALID_ARGUMENT;

    std::string installed_id;
    olineri_status_t status = runtime->package_manager->install(package_path, installed_id);
    if (status == OLINERI_OK && out_pkg_id && pkg_id_size > 0) {
        std::strncpy(out_pkg_id, installed_id.c_str(), pkg_id_size - 1);
        out_pkg_id[pkg_id_size - 1] = '\0';
    }
    return status;
}

OLINERI_API olineri_status_t olineri_package_uninstall(
    olineri_runtime_handle_t runtime,
    const char* package_id
) {
    if (!runtime || !package_id) return OLINERI_ERR_INVALID_ARGUMENT;

    // Check if an active session is using this package
    {
        std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
        for (auto* session : runtime->active_sessions) {
            if (session->manifest.package_id == package_id) {
                return OLINERI_ERR_RESOURCE_BUSY;
            }
        }
    }

    return runtime->package_manager->uninstall(package_id);
}

OLINERI_API olineri_status_t olineri_package_rollback(
    olineri_runtime_handle_t runtime,
    const char* package_id
) {
    if (!runtime || !package_id) return OLINERI_ERR_INVALID_ARGUMENT;
    return runtime->package_manager->rollback(package_id);
}

OLINERI_API uint32_t olineri_package_count(
    olineri_runtime_handle_t runtime
) {
    if (!runtime) return 0;
    return static_cast<uint32_t>(runtime->package_manager->list_installed().size());
}

OLINERI_API olineri_status_t olineri_package_get_at(
    olineri_runtime_handle_t runtime,
    uint32_t index,
    char* out_package_id,
    size_t pkg_id_cap,
    char* out_language_tag,
    size_t lang_tag_cap,
    char* out_version,
    size_t version_cap
) {
    if (!runtime) return OLINERI_ERR_INVALID_ARGUMENT;
    auto pkgs = runtime->package_manager->list_installed();
    if (index >= pkgs.size()) return OLINERI_ERR_INVALID_ARGUMENT;

    const auto& p = pkgs[index];
    if (out_package_id && pkg_id_cap > 0) {
        std::strncpy(out_package_id, p.package_id.c_str(), pkg_id_cap - 1);
        out_package_id[pkg_id_cap - 1] = '\0';
    }
    if (out_language_tag && lang_tag_cap > 0) {
        std::strncpy(out_language_tag, p.language_tag.c_str(), lang_tag_cap - 1);
        out_language_tag[lang_tag_cap - 1] = '\0';
    }
    if (out_version && version_cap > 0) {
        std::strncpy(out_version, p.version.c_str(), version_cap - 1);
        out_version[version_cap - 1] = '\0';
    }
    return OLINERI_OK;
}

OLINERI_API olineri_status_t olineri_session_create(
    olineri_runtime_handle_t runtime,
    const char* language_tag,
    olineri_session_handle_t* out_session
) {
    if (!runtime || !language_tag || !out_session) {
        return OLINERI_ERR_INVALID_ARGUMENT;
    }

    olineri::manifest::PackageManifest manifest;
    if (!runtime->package_manager->find_by_language(language_tag, manifest)) {
        return OLINERI_ERR_PACKAGE_NOT_FOUND;
    }

    try {
        auto session = new olineri_session_s();
        session->runtime = runtime;
        session->language_tag = language_tag;
        session->manifest = manifest;

        // Create inference engine (MockEngine in initial core, Sherpa/ONNX in extension)
        auto mock = std::make_unique<olineri::backends::MockEngine>(64, 0);
        std::string pkg_dir = runtime->package_manager->get_package_dir(manifest.package_id);
        mock->load_model(pkg_dir, manifest.model.id);

        session->engine = std::move(mock);
        session->scorer = std::make_unique<olineri::scoring::ScoringEngine>();
        session->g2p = std::make_unique<olineri::g2p::BrahmicG2P>(olineri::g2p::G2PConfig{language_tag, true, true});

        {
            std::lock_guard<std::mutex> lock(runtime->sessions_mutex);
            runtime->active_sessions.push_back(session);
        }

        *out_session = session;
        return OLINERI_OK;
    } catch (...) {
        return OLINERI_ERR_OUT_OF_MEMORY;
    }
}

OLINERI_API olineri_status_t olineri_session_destroy(
    olineri_session_handle_t session
) {
    if (!session || !session->runtime) return OLINERI_ERR_INVALID_ARGUMENT;

    auto* rt = session->runtime;
    {
        std::lock_guard<std::mutex> lock(rt->sessions_mutex);
        auto it = std::find(rt->active_sessions.begin(), rt->active_sessions.end(), session);
        if (it != rt->active_sessions.end()) {
            rt->active_sessions.erase(it);
        }
    }

    delete session;
    return OLINERI_OK;
}

OLINERI_API olineri_status_t olineri_assess_pronunciation(
    olineri_session_handle_t session,
    const char* reference_text,
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    olineri_result_handle_t* out_result
) {
    if (!session || !reference_text || !audio_samples || !out_result) {
        return OLINERI_ERR_INVALID_ARGUMENT;
    }
    if (sample_count == 0 || sample_rate == 0) {
        return OLINERI_ERR_AUDIO_INVALID;
    }

    // 1. Convert reference text using Unicode-aware Brahmic G2P engine
    std::vector<olineri::scoring::PhonemeTarget> targets = session->g2p->convert(reference_text);
    if (targets.empty()) {
        return OLINERI_ERR_INVALID_ARGUMENT;
    }

    // For MockEngine: configure the expected token sequence
    auto* mock = dynamic_cast<olineri::backends::MockEngine*>(session->engine.get());
    if (mock) {
        std::vector<uint32_t> expected_tokens;
        for (const auto& t : targets) expected_tokens.push_back(t.token_id);
        mock->set_sequence_to_emit(expected_tokens);
    }

    // 2. Run inference to get frame acoustic posteriors
    olineri::scoring::AcousticEmission emission;
    olineri_status_t status = session->engine->infer(audio_samples, sample_count, sample_rate, emission);
    if (status != OLINERI_OK) return status;

    // 3. Align and score
    auto output = session->scorer->assess(targets, emission);

    // 4. Wrap into result struct
    try {
        auto res = new olineri_result_s();
        res->output = std::move(output);

        // Pre-cache C-compatible strings
        for (const auto& ph : res->output.phonemes) {
            res->expected_phonemes.push_back(ph.expected_phoneme);
            res->detected_phonemes.push_back(ph.detected_phoneme);
            res->feedback_strings.push_back(ph.feedback_instruction);
        }

        *out_result = res;
        return OLINERI_OK;
    } catch (...) {
        return OLINERI_ERR_OUT_OF_MEMORY;
    }
}

OLINERI_API float olineri_result_get_overall_score(
    olineri_result_handle_t result
) {
    if (!result) return 0.0f;
    return result->output.overall_score;
}

OLINERI_API uint32_t olineri_result_get_phoneme_count(
    olineri_result_handle_t result
) {
    if (!result) return 0;
    return static_cast<uint32_t>(result->output.phonemes.size());
}

OLINERI_API olineri_status_t olineri_result_get_phoneme_feedback(
    olineri_result_handle_t result,
    uint32_t index,
    olineri_phoneme_feedback_t* out_feedback
) {
    if (!result || !out_feedback) return OLINERI_ERR_INVALID_ARGUMENT;
    if (index >= result->output.phonemes.size()) return OLINERI_ERR_INVALID_ARGUMENT;

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

    return OLINERI_OK;
}

OLINERI_API const char* olineri_result_get_recognized_text(
    olineri_result_handle_t result
) {
    if (!result) return "";
    return result->output.recognized_text.c_str();
}

OLINERI_API void olineri_result_destroy(
    olineri_result_handle_t result
) {
    delete result;
}

} // extern "C"
