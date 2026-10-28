#ifndef VAANI_INTERNAL_INFERENCE_ENGINE_HPP
#define VAANI_INTERNAL_INFERENCE_ENGINE_HPP

#include <string>
#include <vector>
#include <memory>
#include "vaani/vaani_c_api.h"
#include "vaani/internal/scoring_engine.hpp"

namespace vaani {
namespace inference {

class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;

    virtual vaani_status_t load_model(
        const std::string& package_dir,
        const std::string& model_id
    ) = 0;

    virtual vaani_status_t infer(
        const float* audio_samples,
        uint32_t sample_count,
        uint32_t sample_rate,
        scoring::AcousticEmission& out_emission
    ) = 0;

    virtual void unload() = 0;
    virtual bool is_loaded() const = 0;
};

} // namespace inference
} // namespace vaani

#endif // VAANI_INTERNAL_INFERENCE_ENGINE_HPP
