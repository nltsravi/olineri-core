#ifndef VAANI_BACKENDS_MOCK_ENGINE_HPP
#define VAANI_BACKENDS_MOCK_ENGINE_HPP

#include "vaani/internal/inference_engine.hpp"
#include <map>

namespace vaani {
namespace backends {

enum class MockScenario {
    PERFECT_PRONUNCIATION,
    SUBSTITUTE_SPECIFIC_TOKEN,
    OMIT_SPECIFIC_TOKEN,
    LOW_CONFIDENCE_NOISE
};

class MockEngine : public inference::InferenceEngine {
public:
    explicit MockEngine(uint32_t vocab_size = 64, uint32_t blank_id = 0);
    ~MockEngine() override = default;

    vaani_status_t load_model(const std::string& package_dir, const std::string& model_id) override;
    vaani_status_t infer(
        const float* audio_samples,
        uint32_t sample_count,
        uint32_t sample_rate,
        scoring::AcousticEmission& out_emission
    ) override;
    void unload() override;
    bool is_loaded() const override;

    // Test configuration hooks
    void set_scenario(MockScenario scenario);
    void set_sequence_to_emit(const std::vector<uint32_t>& tokens);
    void set_substitution(uint32_t expected_token, uint32_t competitor_token);

private:
    bool loaded_ = false;
    uint32_t vocab_size_;
    uint32_t blank_id_;
    MockScenario scenario_ = MockScenario::PERFECT_PRONUNCIATION;
    std::vector<uint32_t> sequence_to_emit_;
    std::map<uint32_t, uint32_t> substitutions_;
};

} // namespace backends
} // namespace vaani

#endif // VAANI_BACKENDS_MOCK_ENGINE_HPP
