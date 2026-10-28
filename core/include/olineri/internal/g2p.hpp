#ifndef OLINERI_INTERNAL_G2P_HPP
#define OLINERI_INTERNAL_G2P_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "olineri/internal/scoring_engine.hpp"

namespace olineri {
namespace g2p {

struct G2PConfig {
    std::string language_tag; // e.g. "ta-IN", "sa-IN", "hi-IN", "te-IN"
    bool apply_tamil_allophones = true;
    bool apply_schwa_deletion = true;
};

class BrahmicG2P {
public:
    explicit BrahmicG2P(const G2PConfig& config = {"ta-IN", true, true});

    /**
     * @brief Transforms Brahmic script text into canonical phoneme targets with character coordinate spans.
     */
    std::vector<scoring::PhonemeTarget> convert(const std::string& unicode_text) const;

private:
    G2PConfig config_;

    std::vector<scoring::PhonemeTarget> convert_tamil(const std::string& text) const;
    std::vector<scoring::PhonemeTarget> convert_devanagari(const std::string& text) const;
    std::vector<scoring::PhonemeTarget> convert_telugu(const std::string& text) const;
};

} // namespace g2p
} // namespace olineri

#endif // OLINERI_INTERNAL_G2P_HPP
