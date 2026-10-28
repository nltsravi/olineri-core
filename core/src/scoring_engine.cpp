#include "olineri/internal/scoring_engine.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace olineri {
namespace scoring {

ScoringEngine::ScoringEngine(float beta, float theta, float pass_threshold)
    : beta_(beta), theta_(theta), pass_threshold_(pass_threshold) {}

float ScoringEngine::calibrate_score(float lpr) const {
    float x = -beta_ * (lpr - theta_);
    if (x > 30.0f) return 0.0f;
    if (x < -30.0f) return 100.0f;
    return 100.0f / (1.0f + std::exp(x));
}

AssessmentOutput ScoringEngine::assess(
    const std::vector<PhonemeTarget>& targets,
    const AcousticEmission& emission,
    float frame_shift_ms
) {
    AssessmentOutput output;
    if (targets.empty()) {
        output.overall_score = 0.0f;
        return output;
    }

    uint32_t T = emission.num_frames;
    uint32_t V = emission.vocab_size;
    uint32_t blank = emission.blank_id;

    if (T == 0 || V == 0 || emission.data.empty()) {
        for (const auto& tgt : targets) {
            PhonemeResult res;
            res.expected_phoneme = tgt.ipa;
            res.score = 0.0f;
            res.status = OLINERI_PHONEME_UNCERTAIN;
            res.char_start_utf8 = tgt.char_start_utf8;
            res.char_end_utf8 = tgt.char_end_utf8;
            res.cluster_index = tgt.cluster_index;
            res.cluster_count = tgt.cluster_count;
            output.phonemes.push_back(res);
        }
        output.overall_score = 0.0f;
        return output;
    }

    // Build interleaved sequence S' = (blank, s1, blank, s2, ..., sN, blank)
    size_t N = targets.size();
    size_t M = 2 * N + 1;
    std::vector<uint32_t> S_prime(M);
    for (size_t i = 0; i < N; ++i) {
        S_prime[2 * i] = blank;
        S_prime[2 * i + 1] = targets[i].token_id;
    }
    S_prime[2 * N] = blank;

    // Viterbi forward trellis
    const float NEG_INF = -1e9f;
    std::vector<float> alpha(M, NEG_INF);
    std::vector<int> backptr(T * M, -1);

    // Initial frame t = 0
    alpha[0] = emission.at(0, S_prime[0]);
    if (M > 1) {
        alpha[1] = emission.at(0, S_prime[1]);
    }

    for (uint32_t t = 1; t < T; ++t) {
        std::vector<float> next_alpha(M, NEG_INF);
        for (size_t j = 0; j < M; ++j) {
            // Option 1: stay at j
            float best_val = alpha[j];
            int best_prev = static_cast<int>(j);

            // Option 2: step from j-1
            if (j > 0 && alpha[j - 1] > best_val) {
                best_val = alpha[j - 1];
                best_prev = static_cast<int>(j - 1);
            }

            // Option 3: skip blank from j-2
            if (j > 1 && S_prime[j] != blank && S_prime[j] != S_prime[j - 2]) {
                if (alpha[j - 2] > best_val) {
                    best_val = alpha[j - 2];
                    best_prev = static_cast<int>(j - 2);
                }
            }

            if (best_val > NEG_INF / 2) {
                next_alpha[j] = best_val + emission.at(t, S_prime[j]);
                backptr[t * M + j] = best_prev;
            }
        }
        alpha = std::move(next_alpha);
    }

    // Backtrack from best terminal state
    int best_end_j = static_cast<int>(M - 1);
    if (M >= 2 && alpha[M - 2] > alpha[M - 1]) {
        best_end_j = static_cast<int>(M - 2);
    }

    std::vector<int> path(T, -1);
    int curr_j = best_end_j;
    for (int t = static_cast<int>(T) - 1; t >= 0; --t) {
        path[t] = curr_j;
        if (t > 0 && curr_j >= 0) {
            curr_j = backptr[t * M + curr_j];
        }
    }

    // Map path back to target phoneme intervals
    // For each target i, its state in S_prime is 2*i + 1
    std::vector<int> t_start(N, -1);
    std::vector<int> t_end(N, -1);

    for (uint32_t t = 0; t < T; ++t) {
        int state = path[t];
        if (state > 0 && (state % 2 == 1)) {
            size_t target_idx = static_cast<size_t>(state / 2);
            if (target_idx < N) {
                if (t_start[target_idx] == -1) {
                    t_start[target_idx] = static_cast<int>(t);
                }
                t_end[target_idx] = static_cast<int>(t);
            }
        }
    }

    float total_score = 0.0f;
    uint32_t scored_count = 0;

    for (size_t i = 0; i < N; ++i) {
        PhonemeResult res;
        const auto& tgt = targets[i];
        res.expected_phoneme = tgt.ipa;
        res.char_start_utf8 = tgt.char_start_utf8;
        res.char_end_utf8 = tgt.char_end_utf8;
        res.cluster_index = tgt.cluster_index;
        res.cluster_count = tgt.cluster_count;

        int ts = t_start[i];
        int te = t_end[i];

        if (ts == -1 || te == -1 || te < ts) {
            // Omission: target received zero aligned frames
            res.score = 0.0f;
            res.status = OLINERI_PHONEME_OMISSION;
            res.start_time_ms = 0.0f;
            res.end_time_ms = 0.0f;
            res.feedback_instruction = "Sound was omitted or inaudible.";
        } else {
            res.start_time_ms = static_cast<float>(ts) * frame_shift_ms;
            res.end_time_ms = static_cast<float>(te + 1) * frame_shift_ms;
            uint32_t duration = static_cast<uint32_t>(te - ts + 1);

            // Compute Log Posterior Ratio (LPR) & find highest competitor
            float lpr_sum = 0.0f;
            std::vector<float> competitor_mass(V, 0.0f);

            for (int t = ts; t <= te; ++t) {
                float target_val = emission.at(t, tgt.token_id);
                float max_non_blank = -1e9f;

                for (uint32_t v = 0; v < V; ++v) {
                    if (v == blank) continue;
                    float val = emission.at(t, v);
                    competitor_mass[v] += val;
                    if (val > max_non_blank) {
                        max_non_blank = val;
                    }
                }
                lpr_sum += (target_val - max_non_blank);
            }

            float avg_lpr = lpr_sum / static_cast<float>(duration);
            res.score = calibrate_score(avg_lpr);

            if (res.score >= pass_threshold_) {
                res.status = OLINERI_PHONEME_CORRECT;
                res.detected_phoneme = tgt.ipa;
            } else {
                // Check if target was omitted (insufficient acoustic duration or below noise floor)
                float mean_target_prob = competitor_mass[tgt.token_id] / static_cast<float>(duration);
                if (duration <= 1 || (duration <= 2 && mean_target_prob < -3.0f)) {
                    res.status = OLINERI_PHONEME_OMISSION;
                    res.score = 0.0f;
                    res.feedback_instruction = "Sound was omitted or inaudible.";
                } else {
                    // Find top competitor
                    uint32_t best_alt = tgt.token_id;
                    float best_alt_val = -1e9f;
                    for (uint32_t v = 0; v < V; ++v) {
                        if (v == blank || v == tgt.token_id) continue;
                        if (competitor_mass[v] > best_alt_val) {
                            best_alt_val = competitor_mass[v];
                            best_alt = v;
                        }
                    }

                    if (best_alt != tgt.token_id && best_alt_val > competitor_mass[tgt.token_id]) {
                        res.status = OLINERI_PHONEME_SUBSTITUTION;
                        res.detected_phoneme = "token_" + std::to_string(best_alt);
                        res.feedback_instruction = "Articulatory placement substituted with alternative sound.";
                    } else {
                        res.status = OLINERI_PHONEME_UNCERTAIN;
                        res.feedback_instruction = "Sound articulation was unclear or low confidence.";
                    }
                }
            }
        }

        total_score += res.score;
        scored_count++;
        output.phonemes.push_back(res);
    }

    output.overall_score = scored_count > 0 ? (total_score / static_cast<float>(scored_count)) : 0.0f;
    return output;
}

} // namespace scoring
} // namespace olineri
