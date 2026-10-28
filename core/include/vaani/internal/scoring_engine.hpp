#ifndef VAANI_INTERNAL_SCORING_ENGINE_HPP
#define VAANI_INTERNAL_SCORING_ENGINE_HPP

#include <string>
#include <vector>
#include <memory>
#include "vaani/vaani_c_api.h"

namespace vaani {
namespace scoring {

struct PhonemeTarget {
    std::string ipa;
    uint32_t token_id = 0;
    uint32_t char_start_utf8 = 0;
    uint32_t char_end_utf8 = 0;
    uint32_t cluster_index = 0;
    uint32_t cluster_count = 1;
};

struct PhonemeResult {
    std::string expected_phoneme;
    std::string detected_phoneme;
    float score = 0.0f;
    vaani_phoneme_status_t status = VAANI_PHONEME_CORRECT;
    uint32_t char_start_utf8 = 0;
    uint32_t char_end_utf8 = 0;
    uint32_t cluster_index = 0;
    uint32_t cluster_count = 1;
    float start_time_ms = 0.0f;
    float end_time_ms = 0.0f;
    std::string feedback_instruction;
};

struct AssessmentOutput {
    float overall_score = 0.0f;
    std::vector<PhonemeResult> phonemes;
    std::string recognized_text;
};

/**
 * @brief CTC Alignment matrix: T frames x V vocabulary logits or log probabilities.
 */
struct AcousticEmission {
    uint32_t num_frames = 0;
    uint32_t vocab_size = 0;
    uint32_t blank_id = 0;
    std::vector<float> data; // Size: num_frames * vocab_size

    float at(uint32_t t, uint32_t v) const {
        return data[t * vocab_size + v];
    }
};

/**
 * @brief Aligns acoustic frames against target phoneme sequence and computes GOP scores.
 */
class ScoringEngine {
public:
    ScoringEngine(float beta = 1.5f, float theta = -1.2f, float pass_threshold = 65.0f);

    AssessmentOutput assess(
        const std::vector<PhonemeTarget>& targets,
        const AcousticEmission& emission,
        float frame_shift_ms = 10.0f
    );

private:
    float beta_;
    float theta_;
    float pass_threshold_;

    float calibrate_score(float lpr) const;
};

} // namespace scoring
} // namespace vaani

#endif // VAANI_INTERNAL_SCORING_ENGINE_HPP
