#include "backends/mock/mock_engine.hpp"
#include <cmath>
#include <algorithm>

namespace vaani {
namespace backends {

MockEngine::MockEngine(uint32_t vocab_size, uint32_t blank_id)
    : vocab_size_(vocab_size), blank_id_(blank_id) {}

vaani_status_t MockEngine::load_model(const std::string& /*package_dir*/, const std::string& /*model_id*/) {
    loaded_ = true;
    return VAANI_OK;
}

void MockEngine::unload() {
    loaded_ = false;
}

bool MockEngine::is_loaded() const {
    return loaded_;
}

void MockEngine::set_scenario(MockScenario scenario) {
    scenario_ = scenario;
}

void MockEngine::set_sequence_to_emit(const std::vector<uint32_t>& tokens) {
    sequence_to_emit_ = tokens;
}

void MockEngine::set_substitution(uint32_t expected_token, uint32_t competitor_token) {
    substitutions_[expected_token] = competitor_token;
}

vaani_status_t MockEngine::infer(
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    scoring::AcousticEmission& out_emission
) {
    if (!audio_samples || sample_count == 0 || sample_rate == 0) {
        return VAANI_ERR_AUDIO_INVALID;
    }

    // Audio health checks
    float sum_sq = 0.0f;
    uint32_t clipped_count = 0;

    for (uint32_t i = 0; i < sample_count; ++i) {
        float val = audio_samples[i];
        if (std::isnan(val) || std::isinf(val)) {
            return VAANI_ERR_AUDIO_INVALID;
        }
        if (val > 1.0f || val < -1.0f) {
            clipped_count++;
        }
        sum_sq += val * val;
    }

    if (clipped_count > sample_count / 100) {
        return VAANI_ERR_AUDIO_CLIPPED;
    }

    float rms = std::sqrt(sum_sq / static_cast<float>(sample_count));
    if (rms < 0.001f) {
        return VAANI_ERR_AUDIO_TOO_QUIET;
    }

    // Compute frames (10 ms shift at 16000 Hz = 160 samples/frame)
    uint32_t num_frames = std::max(sample_count / 160, 20u);

    out_emission.num_frames = num_frames;
    out_emission.vocab_size = vocab_size_;
    out_emission.blank_id = blank_id_;
    out_emission.data.assign(num_frames * vocab_size_, -5.0f); // background log prob

    if (scenario_ == MockScenario::LOW_CONFIDENCE_NOISE) {
        float uniform_log_prob = -std::log(static_cast<float>(vocab_size_));
        std::fill(out_emission.data.begin(), out_emission.data.end(), uniform_log_prob);
        return VAANI_OK;
    }

    if (sequence_to_emit_.empty()) {
        // Fallback: emit blanks
        for (uint32_t t = 0; t < num_frames; ++t) {
            out_emission.data[t * vocab_size_ + blank_id_] = 0.0f;
        }
        return VAANI_OK;
    }

    // Distribute emitted tokens across frames
    size_t num_tokens = sequence_to_emit_.size();
    uint32_t frames_per_token = num_frames / static_cast<uint32_t>(num_tokens);
    if (frames_per_token == 0) frames_per_token = 1;

    for (size_t i = 0; i < num_tokens; ++i) {
        uint32_t target_tok = sequence_to_emit_[i];
        uint32_t start_f = static_cast<uint32_t>(i) * frames_per_token;
        uint32_t end_f = std::min(start_f + frames_per_token, num_frames);

        uint32_t tok_to_emit = target_tok;
        if (scenario_ == MockScenario::SUBSTITUTE_SPECIFIC_TOKEN) {
            auto it = substitutions_.find(target_tok);
            if (it != substitutions_.end()) {
                tok_to_emit = it->second;
            }
        }

        bool is_omitted = (scenario_ == MockScenario::OMIT_SPECIFIC_TOKEN && substitutions_.count(target_tok) > 0);

        for (uint32_t t = start_f; t < end_f; ++t) {
            if (is_omitted) {
                // Emit blank instead of target sound
                out_emission.data[t * vocab_size_ + blank_id_] = -0.1f;
            } else {
                // High probability for emitted token
                out_emission.data[t * vocab_size_ + tok_to_emit] = -0.05f;
                // Suppress blank during active speech
                out_emission.data[t * vocab_size_ + blank_id_] = -4.0f;
            }
        }
    }

    return VAANI_OK;
}

} // namespace backends
} // namespace vaani
