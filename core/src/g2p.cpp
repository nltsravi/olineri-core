#include "olineri/internal/g2p.hpp"
#include <map>

namespace olineri {
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
    std::vector<scoring::PhonemeTarget> targets;
    auto cps = decode_utf8(text);
    if (cps.empty()) return targets;

    bool is_sanskrit = (config_.language_tag.rfind("sa", 0) == 0);

    // Devanagari Independent Vowels (U+0905 to U+0914)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> iv_sa = {
        {0x0905, {"/ɐ/", 1}},  {0x0906, {"/aː/", 2}}, {0x0907, {"/i/", 3}},   {0x0908, {"/iː/", 4}},
        {0x0909, {"/u/", 5}},  {0x090A, {"/uː/", 6}}, {0x090B, {"/r̩/", 7}},  {0x090F, {"/eː/", 8}},
        {0x0910, {"/aɪ/", 9}}, {0x0913, {"/oː/", 10}},{0x0914, {"/aʊ/", 11}}
    };

    static const std::map<uint32_t, std::pair<std::string, uint32_t>> iv_hi = {
        {0x0905, {"/ə/", 1}},  {0x0906, {"/aː/", 2}}, {0x0907, {"/ɪ/", 3}},   {0x0908, {"/iː/", 4}},
        {0x0909, {"/ʊ/", 5}},  {0x090A, {"/uː/", 6}}, {0x090B, {"/rɪ/", 7}}, {0x090F, {"/eː/", 7}},
        {0x0910, {"/ɛː/", 8}}, {0x0913, {"/oː/", 9}}, {0x0914, {"/ɔː/", 10}}
    };

    // Devanagari Consonants (U+0915 to U+0939)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> cons_sa = {
        {0x0915, {"/k/", 12}},  {0x0916, {"/kʰ/", 13}},  {0x0917, {"/ɡ/", 14}},  {0x0918, {"/ɡʱ/", 15}}, {0x0919, {"/ŋ/", 16}},
        {0x091A, {"/t͡ʃ/", 17}}, {0x091B, {"/t͡ʃʰ/", 18}}, {0x091C, {"/d͡ʒ/", 19}}, {0x091D, {"/d͡ʒʱ/", 20}},{0x091E, {"/ɲ/", 21}},
        {0x091F, {"/ʈ/", 22}},  {0x0920, {"/ʈʰ/", 23}},  {0x0921, {"/ɖ/", 24}},  {0x0922, {"/ɖʱ/", 25}}, {0x0923, {"/ɳ/", 26}},
        {0x0924, {"/t̪/", 27}},  {0x0925, {"/t̪ʰ/", 28}},  {0x0926, {"/d̪/", 29}},  {0x0927, {"/d̪ʱ/", 30}}, {0x0928, {"/n̪/", 31}},
        {0x092A, {"/p/", 32}},  {0x092B, {"/pʰ/", 33}},  {0x092C, {"/b/", 34}},  {0x092D, {"/bʱ/", 35}}, {0x092E, {"/m/", 36}},
        {0x092F, {"/j/", 37}},  {0x0930, {"/r/", 38}},   {0x0932, {"/l/", 39}},  {0x0935, {"/ʋ/", 40}},
        {0x0936, {"/ɕ/", 41}},  {0x0937, {"/ʂ/", 42}},   {0x0938, {"/s/", 43}},  {0x0939, {"/ɦ/", 44}}
    };

    static const std::map<uint32_t, std::pair<std::string, uint32_t>> cons_hi = {
        {0x0915, {"/k/", 11}},  {0x0916, {"/kʰ/", 12}},  {0x0917, {"/ɡ/", 13}},  {0x0918, {"/ɡʱ/", 14}}, {0x0919, {"/ŋ/", 11}},
        {0x091A, {"/t͡ʃ/", 15}}, {0x091B, {"/t͡ʃʰ/", 16}}, {0x091C, {"/d͡ʒ/", 17}}, {0x091D, {"/d͡ʒʱ/", 18}},{0x091E, {"/ɲ/", 15}},
        {0x091F, {"/ʈ/", 19}},  {0x0920, {"/ʈʰ/", 20}},  {0x0921, {"/ɖ/", 21}},  {0x0922, {"/ɖʱ/", 22}}, {0x0923, {"/ɳ/", 19}},
        {0x0924, {"/t̪/", 23}},  {0x0925, {"/t̪ʰ/", 24}},  {0x0926, {"/d̪/", 25}},  {0x0927, {"/d̪ʱ/", 26}}, {0x0928, {"/n/", 27}},
        {0x092A, {"/p/", 28}},  {0x092B, {"/pʰ/", 29}},  {0x092C, {"/b/", 30}},  {0x092D, {"/bʱ/", 31}}, {0x092E, {"/m/", 32}},
        {0x092F, {"/j/", 33}},  {0x0930, {"/r/", 34}},   {0x0932, {"/l/", 35}},  {0x0935, {"/ʋ/", 36}},
        {0x0936, {"/ʃ/", 37}},  {0x0937, {"/ʂ/", 37}},   {0x0938, {"/s/", 38}},  {0x0939, {"/ɦ/", 39}},
        {0x0958, {"/q/", 40}},  {0x0959, {"/x/", 41}},   {0x095A, {"/ɣ/", 42}},  {0x095B, {"/z/", 43}},
        {0x095C, {"/ɽ/", 45}},  {0x095D, {"/ɽʱ/", 46}},  {0x095E, {"/f/", 44}}
    };

    // Matras (Vowel signs, U+093E to U+094C)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> matra_sa = {
        {0x093E, {"/aː/", 2}}, {0x093F, {"/i/", 3}},   {0x0940, {"/iː/", 4}},
        {0x0941, {"/u/", 5}},  {0x0942, {"/uː/", 6}},  {0x0943, {"/r̩/", 7}},
        {0x0947, {"/eː/", 8}}, {0x0948, {"/aɪ/", 9}},  {0x094B, {"/oː/", 10}},{0x094C, {"/aʊ/", 11}}
    };

    static const std::map<uint32_t, std::pair<std::string, uint32_t>> matra_hi = {
        {0x093E, {"/aː/", 2}}, {0x093F, {"/ɪ/", 3}},   {0x0940, {"/iː/", 4}},
        {0x0941, {"/ʊ/", 5}},  {0x0942, {"/uː/", 6}},  {0x0943, {"/rɪ/", 7}},
        {0x0947, {"/eː/", 7}}, {0x0948, {"/ɛː/", 8}},  {0x094B, {"/oː/", 9}}, {0x094C, {"/ɔː/", 10}}
    };

    const uint32_t VIRAMA = 0x094D; // Halant
    const uint32_t ANUSVARA = 0x0902;
    const uint32_t CHANDRABINDU = 0x0901;
    const uint32_t VISARGA = 0x0903;
    const uint32_t NUKTA = 0x093C;

    const auto& iv_map = is_sanskrit ? iv_sa : iv_hi;
    const auto& cons_map = is_sanskrit ? cons_sa : cons_hi;
    const auto& matra_map = is_sanskrit ? matra_sa : matra_hi;
    std::string default_inherent = is_sanskrit ? "/ɐ/" : "/ə/";
    uint32_t default_inherent_token = 1;

    struct AksharaUnit {
        bool is_vowel = false;
        std::string v_ipa;
        uint32_t v_token = 0;
        uint32_t v_start = 0;
        uint32_t v_end = 0;

        bool has_cons = false;
        std::string c_ipa;
        uint32_t c_token = 0;
        uint32_t c_start = 0;
        uint32_t c_end = 0;

        bool has_halant = false;
        bool has_matra = false;
        bool has_inherent = false;

        bool has_visarga = false;
        uint32_t vis_start = 0;
        uint32_t vis_end = 0;

        bool has_anusvara = false;
        uint32_t anu_start = 0;
        uint32_t anu_end = 0;
    };

    // Parse words
    std::vector<std::vector<AksharaUnit>> words;
    std::vector<AksharaUnit> current_word;

    size_t i = 0;
    while (i < cps.size()) {
        const auto& c = cps[i];

        if (c.cp == ' ' || c.cp == '\t' || c.cp == '\n' || c.cp == '.' || c.cp == ',' || c.cp == 0x0964) {
            if (!current_word.empty()) {
                words.push_back(current_word);
                current_word.clear();
            }
            i++;
            continue;
        }

        // 1. Independent Vowel
        auto iv_it = iv_map.find(c.cp);
        if (iv_it != iv_map.end()) {
            AksharaUnit u;
            u.is_vowel = true;
            u.v_ipa = iv_it->second.first;
            u.v_token = iv_it->second.second;
            u.v_start = static_cast<uint32_t>(c.byte_offset);
            u.v_end = static_cast<uint32_t>(c.byte_offset + c.byte_length);

            // Check following modifiers
            if (i + 1 < cps.size()) {
                if (cps[i + 1].cp == ANUSVARA || cps[i + 1].cp == CHANDRABINDU) {
                    u.has_anusvara = true;
                    u.anu_start = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    u.anu_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++;
                } else if (cps[i + 1].cp == VISARGA) {
                    u.has_visarga = true;
                    u.vis_start = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    u.vis_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++;
                }
            }
            current_word.push_back(u);
            i++;
            continue;
        }

        // 2. Consonant
        auto cons_it = cons_map.find(c.cp);
        if (cons_it != cons_map.end()) {
            AksharaUnit u;
            u.has_cons = true;
            u.c_ipa = cons_it->second.first;
            u.c_token = cons_it->second.second;
            u.c_start = static_cast<uint32_t>(c.byte_offset);
            u.c_end = static_cast<uint32_t>(c.byte_offset + c.byte_length);

            // Check Nukta modifier
            if (i + 1 < cps.size() && cps[i + 1].cp == NUKTA) {
                u.c_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                if (c.cp == 0x0915) { u.c_ipa = "/q/"; u.c_token = 40; }
                else if (c.cp == 0x0916) { u.c_ipa = "/x/"; u.c_token = 41; }
                else if (c.cp == 0x0917) { u.c_ipa = "/ɣ/"; u.c_token = 42; }
                else if (c.cp == 0x091C) { u.c_ipa = "/z/"; u.c_token = 43; }
                else if (c.cp == 0x0921) { u.c_ipa = "/ɽ/"; u.c_token = 45; }
                else if (c.cp == 0x0922) { u.c_ipa = "/ɽʱ/"; u.c_token = 46; }
                else if (c.cp == 0x092B) { u.c_ipa = "/f/"; u.c_token = 44; }
                i++;
            }

            // Check following Virama or Matra
            if (i + 1 < cps.size() && cps[i + 1].cp == VIRAMA) {
                u.has_halant = true;
                u.c_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                i++;
            } else if (i + 1 < cps.size() && matra_map.find(cps[i + 1].cp) != matra_map.end()) {
                const auto& mat_info = matra_map.at(cps[i + 1].cp);
                u.has_matra = true;
                u.v_ipa = mat_info.first;
                u.v_token = mat_info.second;
                u.v_start = static_cast<uint32_t>(cps[i + 1].byte_offset);
                u.v_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                i++;
            } else {
                // Inherent vowel
                u.has_inherent = true;
                u.v_ipa = default_inherent;
                u.v_token = default_inherent_token;
                u.v_start = u.c_start;
                u.v_end = u.c_end;
            }

            // Check following Anusvara or Visarga
            if (i + 1 < cps.size()) {
                if (cps[i + 1].cp == ANUSVARA || cps[i + 1].cp == CHANDRABINDU) {
                    u.has_anusvara = true;
                    u.anu_start = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    u.anu_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++;
                } else if (cps[i + 1].cp == VISARGA) {
                    u.has_visarga = true;
                    u.vis_start = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    u.vis_end = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++;
                }
            }

            current_word.push_back(u);
            i++;
            continue;
        }

        // Other character
        i++;
    }

    if (!current_word.empty()) {
        words.push_back(current_word);
    }

    // Apply phonological rules (Hindi Schwa Deletion)
    if (!is_sanskrit && config_.apply_schwa_deletion) {
        for (auto& w : words) {
            size_t N = w.size();
            if (N >= 2) {
                // 1. Final schwa deletion
                if (w[N - 1].has_inherent && !w[N - 1].has_visarga && !w[N - 1].has_anusvara) {
                    w[N - 1].has_inherent = false;
                }

                // 2. Internal schwa deletion (VC1əC2V -> VC1C2V)
                for (size_t k = 1; k + 1 < N; ++k) {
                    if (w[k].has_inherent && !w[k].has_visarga && !w[k].has_anusvara) {
                        bool prev_vowel = (w[k - 1].is_vowel || w[k - 1].has_matra || w[k - 1].has_inherent);
                        bool next_vowel = (w[k + 1].has_matra || w[k + 1].is_vowel);
                        if (prev_vowel && next_vowel) {
                            w[k].has_inherent = false;
                        }
                    }
                }
            }
        }
    }

    // Serialize targets
    uint32_t cluster_idx = 0;
    for (const auto& w : words) {
        for (const auto& u : w) {
            std::string prev_vowel_for_visarga;

            if (u.is_vowel) {
                scoring::PhonemeTarget pt;
                pt.ipa = u.v_ipa;
                pt.token_id = u.v_token;
                pt.char_start_utf8 = u.v_start;
                pt.char_end_utf8 = u.v_end;
                pt.cluster_index = cluster_idx;
                pt.cluster_count = 1;
                targets.push_back(pt);
                prev_vowel_for_visarga = u.v_ipa;
            } else if (u.has_cons) {
                scoring::PhonemeTarget pt_c;
                pt_c.ipa = u.c_ipa;
                pt_c.token_id = u.c_token;
                pt_c.char_start_utf8 = u.c_start;
                pt_c.char_end_utf8 = u.c_end;
                pt_c.cluster_index = cluster_idx;
                pt_c.cluster_count = 1;
                targets.push_back(pt_c);

                if (u.has_matra) {
                    scoring::PhonemeTarget pt_v;
                    pt_v.ipa = u.v_ipa;
                    pt_v.token_id = u.v_token;
                    pt_v.char_start_utf8 = u.v_start;
                    pt_v.char_end_utf8 = u.v_end;
                    pt_v.cluster_index = cluster_idx;
                    pt_v.cluster_count = 1;
                    targets.push_back(pt_v);
                    prev_vowel_for_visarga = u.v_ipa;
                } else if (u.has_inherent) {
                    scoring::PhonemeTarget pt_v;
                    pt_v.ipa = u.v_ipa;
                    pt_v.token_id = u.v_token;
                    pt_v.char_start_utf8 = u.v_start;
                    pt_v.char_end_utf8 = u.v_end;
                    pt_v.cluster_index = cluster_idx;
                    pt_v.cluster_count = 1;
                    targets.push_back(pt_v);
                    prev_vowel_for_visarga = u.v_ipa;
                }
            }

            if (u.has_anusvara) {
                scoring::PhonemeTarget pt_a;
                pt_a.ipa = is_sanskrit ? "/m̃/" : "/n/";
                pt_a.token_id = is_sanskrit ? 46 : 27;
                pt_a.char_start_utf8 = u.anu_start;
                pt_a.char_end_utf8 = u.anu_end;
                pt_a.cluster_index = cluster_idx;
                pt_a.cluster_count = 1;
                targets.push_back(pt_a);
            }

            if (u.has_visarga) {
                scoring::PhonemeTarget pt_h;
                pt_h.ipa = "/h/";
                pt_h.token_id = is_sanskrit ? 45 : 39;
                pt_h.char_start_utf8 = u.vis_start;
                pt_h.char_end_utf8 = u.vis_end;
                pt_h.cluster_index = cluster_idx;
                pt_h.cluster_count = 1;
                targets.push_back(pt_h);

                // Sanskrit Visarga echo vowel
                if (is_sanskrit && !prev_vowel_for_visarga.empty()) {
                    scoring::PhonemeTarget pt_echo;
                    // Echo short vowel counterpart (e.g. /aː/ -> /ɐ/, /iː/ -> /i/)
                    if (prev_vowel_for_visarga == "/aː/" || prev_vowel_for_visarga == "/ɐ/") {
                        pt_echo.ipa = "/ɐ/";
                        pt_echo.token_id = 1;
                    } else if (prev_vowel_for_visarga == "/iː/" || prev_vowel_for_visarga == "/i/") {
                        pt_echo.ipa = "/i/";
                        pt_echo.token_id = 3;
                    } else if (prev_vowel_for_visarga == "/uː/" || prev_vowel_for_visarga == "/u/") {
                        pt_echo.ipa = "/u/";
                        pt_echo.token_id = 5;
                    } else {
                        pt_echo.ipa = prev_vowel_for_visarga;
                        pt_echo.token_id = 1;
                    }
                    pt_echo.char_start_utf8 = u.vis_start;
                    pt_echo.char_end_utf8 = u.vis_end;
                    pt_echo.cluster_index = cluster_idx;
                    pt_echo.cluster_count = 1;
                    targets.push_back(pt_echo);
                }
            }

            cluster_idx++;
        }
    }

    return targets;
}

std::vector<scoring::PhonemeTarget> BrahmicG2P::convert_telugu(const std::string& text) const {
    std::vector<scoring::PhonemeTarget> targets;
    auto cps = decode_utf8(text);
    if (cps.empty()) return targets;

    // Telugu Independent Vowels (U+0C05 to U+0C14)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> iv_te = {
        {0x0C05, {"/ɐ/", 1}},  {0x0C06, {"/aː/", 2}}, {0x0C07, {"/i/", 3}},   {0x0C08, {"/iː/", 4}},
        {0x0C09, {"/u/", 5}},  {0x0C0A, {"/uː/", 6}}, {0x0C0B, {"/r̩/", 7}},  {0x0C0E, {"/e/", 8}},
        {0x0C0F, {"/eː/", 9}}, {0x0C10, {"/aɪ/", 10}},{0x0C12, {"/o/", 11}},  {0x0C13, {"/oː/", 12}},
        {0x0C14, {"/aʊ/", 13}}
    };

    // Telugu Consonants (U+0C15 to U+0C39)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> cons_te = {
        {0x0C15, {"/k/", 14}},  {0x0C16, {"/kʰ/", 15}},  {0x0C17, {"/ɡ/", 16}},  {0x0C18, {"/ɡʱ/", 17}}, {0x0C19, {"/ŋ/", 18}},
        {0x0C1A, {"/t͡ʃ/", 19}}, {0x0C1B, {"/t͡ʃʰ/", 20}}, {0x0C1C, {"/d͡ʒ/", 21}}, {0x0C1D, {"/d͡ʒʱ/", 22}},{0x0C1E, {"/ɲ/", 23}},
        {0x0C1F, {"/ʈ/", 24}},  {0x0C20, {"/ʈʰ/", 25}},  {0x0C21, {"/ɖ/", 26}},  {0x0C22, {"/ɖʱ/", 27}}, {0x0C23, {"/ɳ/", 28}},
        {0x0C24, {"/t̪/", 29}},  {0x0C25, {"/t̪ʰ/", 30}},  {0x0C26, {"/d̪/", 31}},  {0x0C27, {"/d̪ʱ/", 32}}, {0x0C28, {"/n/", 33}},
        {0x0C2A, {"/p/", 34}},  {0x0C2B, {"/pʰ/", 35}},  {0x0C2C, {"/b/", 36}},  {0x0C2D, {"/bʱ/", 37}}, {0x0C2E, {"/m/", 38}},
        {0x0C2F, {"/j/", 39}},  {0x0C30, {"/ɾ/", 40}},   {0x0C31, {"/r/", 40}},   {0x0C32, {"/l/", 41}},  {0x0C33, {"/ɭ/", 47}},
        {0x0C35, {"/ʋ/", 42}},  {0x0C36, {"/ʃ/", 43}},   {0x0C37, {"/ʂ/", 44}},   {0x0C38, {"/s/", 45}},  {0x0C39, {"/ɦ/", 46}}
    };

    // Telugu Gunintalu / Matras (U+0C3E to U+0C4C)
    static const std::map<uint32_t, std::pair<std::string, uint32_t>> matra_te = {
        {0x0C3E, {"/aː/", 2}}, {0x0C3F, {"/i/", 3}},   {0x0C40, {"/iː/", 4}},
        {0x0C41, {"/u/", 5}},  {0x0C42, {"/uː/", 6}},  {0x0C43, {"/r̩/", 7}},
        {0x0C46, {"/e/", 8}},  {0x0C47, {"/eː/", 9}},  {0x0C48, {"/aɪ/", 10}},
        {0x0C4A, {"/o/", 11}}, {0x0C4B, {"/oː/", 12}}, {0x0C4C, {"/aʊ/", 13}}
    };

    const uint32_t POLLU = 0x0C4D; // Virama
    const uint32_t SUNNA = 0x0C02; // Anusvara
    const uint32_t VISARGA = 0x0C03;

    uint32_t cluster_idx = 0;
    size_t i = 0;

    while (i < cps.size()) {
        const auto& c = cps[i];

        if (c.cp == ' ' || c.cp == '\t' || c.cp == '\n' || c.cp == '.' || c.cp == ',') {
            i++;
            continue;
        }

        // 1. Independent Vowel
        auto iv_it = iv_te.find(c.cp);
        if (iv_it != iv_te.end()) {
            scoring::PhonemeTarget pt;
            pt.ipa = iv_it->second.first;
            pt.token_id = iv_it->second.second;
            pt.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
            pt.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
            pt.cluster_index = cluster_idx++;
            pt.cluster_count = 1;
            targets.push_back(pt);

            // Check following Sunna or Visarga
            if (i + 1 < cps.size()) {
                if (cps[i + 1].cp == SUNNA) {
                    scoring::PhonemeTarget pt_s;
                    pt_s.ipa = "/m/";
                    pt_s.token_id = 48;
                    pt_s.char_start_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    pt_s.char_end_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    pt_s.cluster_index = cluster_idx++;
                    pt_s.cluster_count = 1;
                    targets.push_back(pt_s);
                    i++;
                } else if (cps[i + 1].cp == VISARGA) {
                    scoring::PhonemeTarget pt_v;
                    pt_v.ipa = "/h/";
                    pt_v.token_id = 46;
                    pt_v.char_start_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    pt_v.char_end_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    pt_v.cluster_index = cluster_idx++;
                    pt_v.cluster_count = 1;
                    targets.push_back(pt_v);
                    i++;
                }
            }
            i++;
            continue;
        }

        // 2. Consonant
        auto cons_it = cons_te.find(c.cp);
        if (cons_it != cons_te.end()) {
            uint32_t start_byte = static_cast<uint32_t>(c.byte_offset);
            uint32_t end_byte = static_cast<uint32_t>(c.byte_offset + c.byte_length);

            bool has_pollu = false;
            std::string matra_ipa;
            uint32_t matra_token = 0;

            if (i + 1 < cps.size()) {
                uint32_t next_cp = cps[i + 1].cp;
                if (next_cp == POLLU) {
                    has_pollu = true;
                    end_byte = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    i++;
                } else {
                    auto mat_it = matra_te.find(next_cp);
                    if (mat_it != matra_te.end()) {
                        matra_ipa = mat_it->second.first;
                        matra_token = mat_it->second.second;
                        end_byte = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                        i++;
                    }
                }
            }

            // Consonant target
            scoring::PhonemeTarget pt_c;
            pt_c.ipa = cons_it->second.first;
            pt_c.token_id = cons_it->second.second;
            pt_c.char_start_utf8 = start_byte;
            pt_c.char_end_utf8 = end_byte;
            pt_c.cluster_index = cluster_idx;
            pt_c.cluster_count = 1;
            targets.push_back(pt_c);

            // Vowel target
            if (has_pollu) {
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

            // Check following Sunna or Visarga
            if (i + 1 < cps.size()) {
                if (cps[i + 1].cp == SUNNA) {
                    scoring::PhonemeTarget pt_s;
                    pt_s.ipa = "/m/";
                    pt_s.token_id = 48;
                    pt_s.char_start_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    pt_s.char_end_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    pt_s.cluster_index = cluster_idx++;
                    pt_s.cluster_count = 1;
                    targets.push_back(pt_s);
                    i++;
                } else if (cps[i + 1].cp == VISARGA) {
                    scoring::PhonemeTarget pt_v;
                    pt_v.ipa = "/h/";
                    pt_v.token_id = 46;
                    pt_v.char_start_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset);
                    pt_v.char_end_utf8 = static_cast<uint32_t>(cps[i + 1].byte_offset + cps[i + 1].byte_length);
                    pt_v.cluster_index = cluster_idx++;
                    pt_v.cluster_count = 1;
                    targets.push_back(pt_v);
                    i++;
                }
            }

            i++;
            continue;
        }

        // Standalone Sunna
        if (c.cp == SUNNA) {
            scoring::PhonemeTarget pt_s;
            pt_s.ipa = "/m/";
            pt_s.token_id = 48;
            pt_s.char_start_utf8 = static_cast<uint32_t>(c.byte_offset);
            pt_s.char_end_utf8 = static_cast<uint32_t>(c.byte_offset + c.byte_length);
            pt_s.cluster_index = cluster_idx++;
            pt_s.cluster_count = 1;
            targets.push_back(pt_s);
            i++;
            continue;
        }

        i++;
    }

    return targets;
}

} // namespace g2p
} // namespace olineri
