#include "olineri/internal/scoring_engine.hpp"
#include <iostream>
#include <cassert>

void test_perfect_alignment_scoring() {
    olineri::scoring::ScoringEngine scorer;

    // 2 target phonemes: token 1 and token 2
    std::vector<olineri::scoring::PhonemeTarget> targets = {
        {"/p/", 1, 0, 1, 0, 1},
        {"/a/", 2, 1, 2, 1, 1}
    };

    // 20 frames, vocab size 10, blank = 0
    olineri::scoring::AcousticEmission emission;
    emission.num_frames = 20;
    emission.vocab_size = 10;
    emission.blank_id = 0;
    emission.data.assign(20 * 10, -5.0f); // background log prob

    // Frames 0..2: blank
    for (int t = 0; t < 3; ++t) emission.data[t * 10 + 0] = -0.05f;
    // Frames 3..9: token 1 (/p/) dominant
    for (int t = 3; t < 10; ++t) emission.data[t * 10 + 1] = -0.02f;
    // Frames 10..12: blank
    for (int t = 10; t < 13; ++t) emission.data[t * 10 + 0] = -0.05f;
    // Frames 13..18: token 2 (/a/) dominant
    for (int t = 13; t < 19; ++t) emission.data[t * 10 + 2] = -0.02f;
    // Frame 19: blank
    emission.data[19 * 10 + 0] = -0.05f;

    auto result = scorer.assess(targets, emission);
    assert(result.phonemes.size() == 2);
    assert(result.phonemes[0].status == OLINERI_PHONEME_CORRECT);
    assert(result.phonemes[0].score >= 70.0f);
    assert(result.phonemes[1].status == OLINERI_PHONEME_CORRECT);
    assert(result.phonemes[1].score >= 70.0f);
    assert(result.overall_score >= 70.0f);

    std::cout << "[PASS] Perfect pronunciation GOP assessment (Score: " << result.overall_score << ")" << std::endl;
}

void test_substitution_detection() {
    olineri::scoring::ScoringEngine scorer;

    // Target expects token 1 (/ɻ/ e.g. Tamil ழ)
    std::vector<olineri::scoring::PhonemeTarget> targets = {
        {"/ɻ/", 1, 0, 3, 0, 1}
    };

    olineri::scoring::AcousticEmission emission;
    emission.num_frames = 15;
    emission.vocab_size = 10;
    emission.blank_id = 0;
    emission.data.assign(15 * 10, -5.0f);

    // Audio instead produces strong evidence for competitor token 3 (/l/ e.g. Tamil ல)
    for (int t = 2; t < 12; ++t) {
        emission.data[t * 10 + 3] = -0.02f; // Competitor /l/
        emission.data[t * 10 + 1] = -4.5f;  // Expected /ɻ/ suppressed
    }

    auto result = scorer.assess(targets, emission);
    assert(result.phonemes.size() == 1);
    assert(result.phonemes[0].status == OLINERI_PHONEME_SUBSTITUTION);
    assert(result.phonemes[0].score < 50.0f);
    assert(result.phonemes[0].detected_phoneme == "token_3");

    std::cout << "[PASS] Substitution detection (Target: /ɻ/, Competitor: token_3, Score: " 
              << result.phonemes[0].score << ")" << std::endl;
}

void test_omission_detection() {
    olineri::scoring::ScoringEngine scorer;

    // 2 target phonemes, second one receives zero audio frames (omitted)
    std::vector<olineri::scoring::PhonemeTarget> targets = {
        {"/k/", 1, 0, 1, 0, 1},
        {"/m/", 2, 1, 2, 1, 1}
    };

    olineri::scoring::AcousticEmission emission;
    emission.num_frames = 10;
    emission.vocab_size = 10;
    emission.blank_id = 0;
    emission.data.assign(10 * 10, -5.0f);

    // All frames are either token 1 or blanks
    for (int t = 0; t < 10; ++t) {
        emission.data[t * 10 + 1] = -0.05f;
    }

    auto result = scorer.assess(targets, emission);
    assert(result.phonemes.size() == 2);
    // Token 2 was never aligned
    assert(result.phonemes[1].status == OLINERI_PHONEME_OMISSION);
    assert(result.phonemes[1].score == 0.0f);

    std::cout << "[PASS] Phoneme omission / deletion detection" << std::endl;
}

int main() {
    test_perfect_alignment_scoring();
    test_substitution_detection();
    test_omission_detection();
    return 0;
}
