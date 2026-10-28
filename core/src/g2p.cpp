#include "vaani/internal/g2p.hpp"
#include <map>

namespace vaani {
namespace g2p {

namespace {

struct CodePointInfo {
    uint32_t cp = 0;
    size_t byte_offset = 0;
    size_t byte_length = 0;
};

std::vector<CodePointInfo> decode_utf8(const std::string& str) {
    std::vector<CodePointInfo> result;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        uint32_t cp = 0;
        size_t len = 0;

        if (c < 0x80) {
            cp = c;
            len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 < str.size()) {
                cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(str[i + 1]) & 0x3F);
                len = 2;
            }
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 < str.size()) {
                cp = ((c & 0x0F) << 12) |
                     ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(str[i + 2]) & 0x3F);
                len = 3;
            }
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 < str.size()) {
                cp = ((c & 0x07) << 18) |
                     ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 12) |
                     ((static_cast<unsigned char>(str[i + 2]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(str[i + 3]) & 0x3F);
                len = 4;
            }
        }

        if (len == 0) len = 1; // skip malformed byte

        CodePointInfo info;
        info.cp = cp;
        info.byte_offset = i;
        info.byte_length = len;
        result.push_back(info);
        i += len;
    }
    return result;
}

} // anonymous namespace

BrahmicG2P::BrahmicG2P(const G2PConfig& config) : config_(config) {}

std::vector<scoring::PhonemeTarget> BrahmicG2P::convert(const std::string& unicode_text) const {
    if (config_.language_tag.rfind("ta", 0) == 0) {
        return convert_tamil(unicode_text);
    } else if (config_.language_tag.rfind("sa", 0) == 0 || config_.language_tag.rfind("hi", 0) == 0) {
        return convert_devanagari(unicode_text);
    } else if (config_.language_tag.rfind("te", 0) == 0) {
        return convert_telugu(unicode_text);
    }
    return convert_tamil(unicode_text); // Default to Tamil
}

std::vector<scoring::PhonemeTarget> BrahmicG2P::convert_tamil(const std::string& text) const {
    std::vector<scoring::PhonemeTarget> targets;
    auto cps = decode_utf8(text);
    if (cps.empty()) return targets;

    // Tamil independent vowels (U+0B85 to U+0B94)
    static const std::map<uint32_t, std::string> independent_vowels = {
        {0x0B85, "/ɐ/"},  {0x0B86, "/aː/"}, {0x0B87, "/i/"},  {0x0B88, "/iː/"},
        {0x0B89, "/u/"},  {0x0B8A, "/uː/"}, {0x0B8E, "/e/"},  {0x0B8F, "/eː/"},
        {0x0B90, "/aɪ/"}, {0x0B92, "/o/"},  {0x0B93, "/oː/"}, {0x0B94, "/aʊ/"}
    };

    // Tamil consonants (U+0B95 to U+0BB9)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> consonants = {
        {0x0B95, {"/k/", 13}},  {0x0B99, {"/ŋ/", 14}},  {0x0B9A, {"/t͡ʃ/", 15}},
        {0x0B9E, {"/ɲ/", 16}},  {0x0B9F, {"/ʈ/", 17}},  {0x0BA3, {"/ɳ/", 18}},
        {0x0BA4, {"/t̪/", 19}},  {0x0BA8, {"/n̪/", 20}},  {0x0BA9, {"/n/", 30}},
        {0x0BAA, {"/p/", 21}},  {0x0BAE, {"/m/", 22}},  {0x0BAF, {"/j/", 23}},
        {0x0BB0, {"/ɾ/", 24}},  {0x0BB1, {"/r/", 29}},  {0x0BB2, {"/l/", 25}},
        {0x0BB3, {"/ɭ/", 28}},  {0x0BB4, {"/ɻ/", 27}},  {0x0BB5, {"/ʋ/", 26}},
        {0x0BB8, {"/s/", 32}},  {0x0BA7, {"/ʂ/", 33}},  {0x0B9C, {"/d͡ʒ/", 34}},
        {0x0BB9, {"/h/", 35}}
    };

    // Tamil vowel signs (matras, U+0BBE to U+0BCC)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> vowel_matras = {
        {0x0BBE, {"/aː/", 2}},  {0x0BBF, {"/i/", 3}},   {0x0BC0, {"/iː/", 4}},
        {0x0BC1, {"/u/", 5}},   {0x0BC2, {"/uː/", 6}},  {0x0BC6, {"/e/", 7}},
        {0x0BC7, {"/eː/", 8}},  {0x0BC8, {"/aɪ/", 9}},  {0x0BCA, {"/o/", 10}},
        {0x0BCB, {"/oː/", 11}}, {0x0BCC, {"/aʊ/", 12}}
    };

    const uint32_t VIRAMA = 0x0BCD; // Tamil Pulli
    const uint32_t AYTHAM = 0x0B83; // Aytham ஃ

    uint32_t cluster_idx = 0;
    size_t i = 0;

    while (i < cps.size()) {
        const auto& c = cps[i];

        // Skip whitespace and punctuation
        if (c.cp == ' ' || c.cp == '\t' || c.cp == '\n' || c.cp == '.' || c.cp == ',') {
            i++;
            continue;
        }

        // 1. Aytham ஃ
        if (c.cp == AYTHAM) {
            scoring::PhonemeTarget pt;
            pt.ipa = "/x/";
            pt.token_id = 31;
            pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
            pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
            pt.cluster_index = cluster_idx++;
            pt.cluster_count = 1;
            targets.push_back(pt);
            i++;
            continue;
        }

        // 2. Independent Vowel
        auto iv_it = independent_vowels.find(c.cp);
        if (iv_it != independent_vowels.end()) {
            scoring::PhonemeTarget pt;
            pt.ipa = iv_it->second;
            pt.token_id = static_cast<uint32_t>(targets.size() + 1);
            pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
            pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
            pt.cluster_index = cluster_idx++;
            pt.cluster_count = 1;
            targets.push_back(pt);
            i++;
            continue;
        }

        // 3. Consonant (with inherent vowel, pulli, or vowel matra)
        auto cons_it = consonants.find(c.cp);
        if (cons_it != consonants.end()) {
            uint32_t start_byte = static_cast<uint32_t>(c.byte_offset);
            uint32_t end_byte = static_cast<uint32_t>(c.byte_offset + c.byte_length);

            // Check following modifier
            bool has_pulli = false;
            std::string matra_ipa;
            uint32_t matra_token = 0;

            if (i + 1 < cps.size()) {
                uint32_t next_cp = cps[i + 1].cp;
                if (next_cp == VIRAMA) {
                    has_pulli = true;
                    end_byte = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++; // consume pulli
                } else {
                    auto mat_it = vowel_matras.find(next_cp);
                    if (mat_it != vowel_matras.end()) {
                        matra_ipa = mat_it->second.first;
                        matra_token = mat_it->second.second;
                        end_byte = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                        i++; // consume matra
                    }
                }
            }

            // Consonant phoneme
            scoring::PhonemeTarget pt_c;
            pt_c.ipa = cons_it->second.first;
            pt_c.token_id = cons_it->second.second;
            pt_c.char_start_utf8 = start_byte;
            pt_c.char_end_utf8 = end_byte;
            pt_c.cluster_index = cluster_idx;
            pt_c.cluster_count = 1;
            targets.push_back(pt_c);

            // Following vowel phoneme
            if (has_pulli) {
                // Pure consonant without vowel
            } else if (!matra_ipa.empty()) {
                scoring::PhonemeTarget pt_v;
                pt_v.ipa = matra_ipa;
                pt_v.token_id = matra_token;
                pt_v.char_start_utf8 = start_byte;
                pt_v.char_end_utf8 = end_byte;
                pt_v.cluster_index = cluster_idx;
                pt_v.cluster_count = 1;
                targets.push_back(pt_v);
            } else {
                // Inherent vowel /ɐ/
                scoring::PhonemeTarget pt_v;
                pt_v.ipa = "/ɐ/";
                pt_v.token_id = 1;
                pt_v.char_start_utf8 = start_byte;
                pt_v.char_end_utf8 = end_byte;
                pt_v.cluster_index = cluster_idx;
                pt_v.cluster_count = 1;
                targets.push_back(pt_v);
            }

            cluster_idx++;
            i++;
            continue;
        }

        // Fallback: character not in standard table
        scoring::PhonemeTarget pt;
        pt.ipa = text.substr(c.byte_offset, c.byte_length);
        pt.token_id = static_cast<uint32_t>(targets.size() + 1);
        pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
        pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
        pt.cluster_index = cluster_idx++;
        pt.cluster_count = 1;
        targets.push_back(pt);
        i++;
    }

    return targets;
}

std::vector<scoring::PhonemeTarget> BrahmicG2P::convert_devanagari(const std::string& text) const {
    // Basic Devanagari implementation (expanded in Milestone 4)
    std::vector<scoring::PhonemeTarget> targets;
    auto cps = decode_utf8(text);
    uint32_t cluster_idx = 0;

    for (const auto& c : cps) {
        if (c.cp == ' ' || c.cp == '\t' || c.cp == '\n') continue;
        scoring::PhonemeTarget pt;
        pt.ipa = text.substr(c.byte_offset, c.byte_length);
        pt.token_id = (cluster_idx % 50) + 1;
        pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
        pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
        pt.cluster_index = cluster_idx++;
        pt.cluster_count = 1;
        targets.push_back(pt);
    }
    return targets;
}

std::vector<scoring::PhonemeTarget> BrahmicG2P::convert_telugu(const std::string& text) const {
    // Basic Telugu implementation (expanded in Milestone 4)
    std::vector<scoring::PhonemeTarget> targets;
    auto cps = decode_utf8(text);
    uint32_t cluster_idx = 0;

    for (const auto& c : cps) {
        if (c.cp == ' ' || c.cp == '\t' || c.cp == '\n') continue;
        scoring::PhonemeTarget pt;
        pt.ipa = text.substr(c.byte_offset, c.byte_length);
        pt.token_id = (cluster_idx % 50) + 1;
        pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
        pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
        pt.cluster_index = cluster_idx++;
        pt.cluster_count = 1;
        targets.push_back(pt);
    }
    return targets;
}

} // namespace g2p
} // namespace vaani
