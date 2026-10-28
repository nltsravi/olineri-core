#include "backends/mock/mock_engine.hpp"
#include <cmath>
#include <algorithm>

namespace olineri {
namespace backends {

MockEngine::MockEngine(uint32_t vocab_size, uint32_t blank_id)
    : vocab_size_(vocab_size), blank_id_(blank_id) {}

olineri_status_t MockEngine::load_model(const std::string& /*package_dir*/, const std::string& /*model_id*/) {
    loaded_ = true;
    return OLINERI_OK;
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

olineri_status_t MockEngine::infer(
    const float* audio_samples,
    uint32_t sample_count,
    uint32_t sample_rate,
    scoring::AcousticEmission& out_emission
) {
    if (!audio_samples || sample_count == 0 || sample_rate == 0) {
        return OLINERI_ERR_AUDIO_INVALID;
    }

    // Target frame length (20 ms window, 10 ms stride -> approx sample_count / 160)
    uint32_t num_frames = sample_count / 160;
    if (num_frames == 0) num_frames = 1;

    out_emission.num_frames = num_frames;
    out_emission.vocab_size = vocab_size_;
    out_emission.blank_id = blank_id_;
    out_emission.data.assign(num_frames * vocab_size_, -5.0f); // Default background noise floor

    // If no explicit tokens set, synthesize dummy emissions with blank dominant
    if (sequence_to_emit_.empty()) {
        for (uint32_t t = 0; t < num_frames; ++t) {
            out_emission.data[t * vocab_size_ + blank_id_] = -0.1f;
        }
        return OLINERI_OK;
    }

    if (scenario_ == MockScenario::LOW_CONFIDENCE_NOISE) {
        // Uniform ambiguous distribution
        float uniform_log_prob = -std::log(static_cast<float>(vocab_size_));
        std::fill(out_emission.data.begin(), out_emission.data.end(), uniform_log_prob);
        return OLINERI_OK;
    }

    // Partition frames evenly across phonemes with blank transitions
    uint32_t N = static_cast<uint32_t>(sequence_to_emit_.size());
    uint32_t frames_per_token = num_frames / (N + 1);
    if (frames_per_token == 0) frames_per_token = 1;

    // Initial blank frames
    for (uint32_t t = 0; t < frames_per_token && t < num_frames; ++t) {
        out_emission.data[t * vocab_size_ + blank_id_] = -0.1f;
    }

    for (uint32_t i = 0; i < N; ++i) {
        uint32_t target_tok = sequence_to_emit_[i];
        uint32_t tok_to_emit = target_tok;

        // Apply substitution scenario if configured
        if (scenario_ == MockScenario::SUBSTITUTE_SPECIFIC_TOKEN && substitutions_.count(target_tok) > 0) {
            tok_to_emit = substitutions_[target_tok];
        }

        uint32_t start_f = (i + 1) * frames_per_token;
        uint32_t end_f = std::min(num_frames, (i + 2) * frames_per_token);

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

    return OLINERI_OK;
}

} // namespace backends
} // namespace olineri
