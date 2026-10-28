#include "olineri/olineri_c_api.h"
#include "olineri/internal/g2p.hpp"
#include "olineri/internal/filesystem.hpp"
#include "olineri/internal/crypto.hpp"
#include "backends/mock/mock_engine.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

static const std::string TEST_SANDBOX = "/tmp/olineri_test_multi_language";

void test_sanskrit_g2p() {
    olineri::g2p::BrahmicG2P g2p({"sa-IN", true, true});

    // Case 1: "नमः" - Visarga echoes preceding inherent vowel /ɐ/
    auto targets1 = g2p.convert("नमः");
    assert(targets1.size() == 6);
    assert(targets1[0].ipa == "/n̪/");
    assert(targets1[0].token_id == 31);
    assert(targets1[1].ipa == "/ɐ/");
    assert(targets1[1].token_id == 1);
    assert(targets1[2].ipa == "/m/");
    assert(targets1[2].token_id == 36);
    assert(targets1[3].ipa == "/ɐ/");
    assert(targets1[3].token_id == 1);
    assert(targets1[4].ipa == "/h/");
    assert(targets1[4].token_id == 45);
    assert(targets1[5].ipa == "/ɐ/");  // Echo vowel!
    assert(targets1[5].token_id == 1);

    // Coordinate verification: Visarga grapheme byte span
    assert(targets1[4].char_start_utf8 == targets1[5].char_start_utf8);
    assert(targets1[4].char_end_utf8 == targets1[5].char_end_utf8);

    // Case 2: "रामः" - Retains inherent vowel, echoes /ɐ/
    auto targets2 = g2p.convert("रामः");
    assert(targets2.size() == 6);
    assert(targets2[0].ipa == "/r/");
    assert(targets2[1].ipa == "/aː/");
    assert(targets2[2].ipa == "/m/");
    assert(targets2[3].ipa == "/ɐ/");  // Inherent vowel NOT deleted in Sanskrit!
    assert(targets2[4].ipa == "/h/");
    assert(targets2[5].ipa == "/ɐ/");

    // Case 3: "शान्तिः" - Visarga echoes preceding matra vowel /i/
    auto targets3 = g2p.convert("शान्तिः");
    assert(targets3.size() == 7);
    assert(targets3[0].ipa == "/ɕ/");
    assert(targets3[1].ipa == "/aː/");
    assert(targets3[2].ipa == "/n̪/");  // Consonant cluster with virama
    assert(targets3[3].ipa == "/t̪/");
    assert(targets3[4].ipa == "/i/");
    assert(targets3[5].ipa == "/h/");
    assert(targets3[6].ipa == "/i/");  // Echo vowel /i/!

    std::cout << "[PASS] Sanskrit G2P: Inherent vowel retention and Visarga vowel echoing" << std::endl;
}

void test_hindi_g2p() {
    olineri::g2p::BrahmicG2P g2p({"hi-IN", true, true});

    // Case 1: "कमल" - Word-final schwa deletion: /k/, /ə/, /m/, /ə/, /l/
    auto targets1 = g2p.convert("कमल");
    assert(targets1.size() == 5);
    assert(targets1[0].ipa == "/k/");
    assert(targets1[0].token_id == 11);
    assert(targets1[1].ipa == "/ə/");
    assert(targets1[1].token_id == 1);
    assert(targets1[2].ipa == "/m/");
    assert(targets1[2].token_id == 32);
    assert(targets1[3].ipa == "/ə/");
    assert(targets1[3].token_id == 1);
    assert(targets1[4].ipa == "/l/");
    assert(targets1[4].token_id == 35);
    // Notice: NO 6th phoneme! Word-final schwa is accurately dropped!

    // Case 2: "फल" - Initial aspirate /pʰ/ + schwa deletion: /pʰ/, /ə/, /l/
    auto targets2 = g2p.convert("फल");
    assert(targets2.size() == 3);
    assert(targets2[0].ipa == "/pʰ/");
    assert(targets2[0].token_id == 29);
    assert(targets2[1].ipa == "/ə/");
    assert(targets2[2].ipa == "/l/");

    // Case 3: "चलना" - Internal schwa deletion in VCəCV environment: /t͡ʃ/, /ə/, /l/, /n/, /aː/
    auto targets3 = g2p.convert("चलना");
    assert(targets3.size() == 5);
    assert(targets3[0].ipa == "/t͡ʃ/");
    assert(targets3[1].ipa == "/ə/");
    assert(targets3[2].ipa == "/l/");   // Schwa on 'ल' deleted!
    assert(targets3[3].ipa == "/n/");
    assert(targets3[4].ipa == "/aː/");

    // Case 4: Nukta consonant: "फ़ल"
    auto targets4 = g2p.convert("फ़ल");
    assert(targets4.size() == 3);
    assert(targets4[0].ipa == "/f/");   // Nukta fricative
    assert(targets4[0].token_id == 44);

    std::cout << "[PASS] Hindi G2P: Final & internal schwa deletion and Nukta consonants" << std::endl;
}

void test_telugu_g2p() {
    olineri::g2p::BrahmicG2P g2p({"te-IN", true, true});

    // Case 1: "అమ్మ" - Geminate bilabial nasal: /ɐ/, /m/, /m/, /ɐ/
    auto targets1 = g2p.convert("అమ్మ");
    assert(targets1.size() == 4);
    assert(targets1[0].ipa == "/ɐ/");
    assert(targets1[0].token_id == 1);
    assert(targets1[1].ipa == "/m/");
    assert(targets1[1].token_id == 38);
    assert(targets1[2].ipa == "/m/");
    assert(targets1[2].token_id == 38);
    assert(targets1[3].ipa == "/ɐ/");
    assert(targets1[3].token_id == 1);

    // Case 2: "ఎండ" - Short vowel /e/ vs long /eː/, and Sunna /m/
    auto targets2 = g2p.convert("ఎండ");
    assert(targets2.size() == 4);
    assert(targets2[0].ipa == "/e/");
    assert(targets2[0].token_id == 8);
    assert(targets2[1].ipa == "/m/");  // Sunna Anusvara
    assert(targets2[1].token_id == 48);
    assert(targets2[2].ipa == "/ɖ/");
    assert(targets2[2].token_id == 26);
    assert(targets2[3].ipa == "/ɐ/");

    // Case 3: "తెలుగు" - Polysyllabic vowel-terminating structure
    auto targets3 = g2p.convert("తెలుగు");
    assert(targets3.size() == 6);
    assert(targets3[0].ipa == "/t̪/");
    assert(targets3[1].ipa == "/e/");
    assert(targets3[2].ipa == "/l/");
    assert(targets3[3].ipa == "/u/");
    assert(targets3[4].ipa == "/ɡ/");
    assert(targets3[5].ipa == "/u/");

    std::cout << "[PASS] Telugu G2P: Gemination, short/long vowels, and Sunna" << std::endl;
}

static std::string create_test_package(const std::string& pkg_id, const std::string& lang_tag, const std::string& script) {
    std::string pkg_dir = TEST_SANDBOX + "/" + pkg_id;
    olineri::fs::create_directories(pkg_dir + "/model");

    std::string dummy_onnx = "dummy_conformer_ctc_" + lang_tag + "_weights";
    std::string model_file = pkg_dir + "/model/acoustic_model.onnx";
    olineri::fs::write_string_to_file(model_file, dummy_onnx);
    std::string sha = olineri::crypto::sha256_file(model_file);

    std::string manifest_json = R"RAW({
      "schemaVersion": "1.0.0",
      "packageId": ")RAW" + pkg_id + R"RAW(",
      "language": ")RAW" + lang_tag + R"RAW(",
      "script": ")RAW" + script + R"RAW(",
      "version": "1.0.0",
      "minSdkVersion": "0.1.0",
      "displayName": ")RAW" + pkg_id + R"RAW(",
      "publisher": { "name": "Olineri Core Team", "publicKeyId": "olineri-prod-key" },
      "capabilities": ["speechRecognition", "pronunciationAssessment", "phonemeFeedback", "characterMapping"],
      "model": {
        "id": "indicconformer-ctc-)RAW" + lang_tag + R"RAW(-int8",
        "architecture": "Conformer-CTC",
        "quantization": "int8",
        "sampleRate": 16000,
        "featureDimension": 80,
        "license": "MIT"
      },
      "artifacts": [
        {
          "path": "model/acoustic_model.onnx",
          "sizeBytes": )RAW" + std::to_string(dummy_onnx.size()) + R"RAW(,
          "sha256": ")RAW" + sha + R"RAW("
        }
      ],
      "totalSizeBytes": )RAW" + std::to_string(dummy_onnx.size()) + R"RAW(,
      "installedSizeBytes": )RAW" + std::to_string(dummy_onnx.size() * 2) + R"RAW(
    })RAW";

    olineri::fs::write_string_to_file(pkg_dir + "/manifest.json", manifest_json);
    return pkg_dir;
}

void test_multilingual_assessment_sessions() {
    olineri::fs::remove_all(TEST_SANDBOX);
    olineri::fs::create_directories(TEST_SANDBOX);
    std::string storage_dir = TEST_SANDBOX + "/storage";

    // Initialize runtime
    olineri_runtime_handle_t runtime = nullptr;
    assert(olineri_runtime_create(storage_dir.c_str(), &runtime) == OLINERI_OK);

    // Install Sanskrit, Hindi, Telugu packages
    std::string sa_pkg = create_test_package("lang-sa", "sa-IN", "Deva");
    std::string hi_pkg = create_test_package("lang-hi", "hi-IN", "Deva");
    std::string te_pkg = create_test_package("lang-te", "te-IN", "Telu");

    char out_id[64] = {0};
    assert(olineri_package_install(runtime, sa_pkg.c_str(), out_id, sizeof(out_id)) == OLINERI_OK);
    assert(olineri_package_install(runtime, hi_pkg.c_str(), out_id, sizeof(out_id)) == OLINERI_OK);
    assert(olineri_package_install(runtime, te_pkg.c_str(), out_id, sizeof(out_id)) == OLINERI_OK);

    std::vector<float> audio(16000, 0.1f);
    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] = 0.4f * std::sin(2.0f * 3.14159f * 300.0f * static_cast<float>(i) / 16000.0f);
    }

    // 1. Sanskrit Assessment Session: "नमः"
    {
        olineri_session_handle_t sa_session = nullptr;
        assert(olineri_session_create(runtime, "sa-IN", &sa_session) == OLINERI_OK);

        olineri_result_handle_t result = nullptr;
        assert(olineri_assess_pronunciation(sa_session, "नमः", audio.data(), audio.size(), 16000, &result) == OLINERI_OK);
        assert(result != nullptr);
        assert(olineri_result_get_phoneme_count(result) == 6); // /n̪/, /ɐ/, /m/, /ɐ/, /h/, /ɐ/
        assert(olineri_result_get_overall_score(result) >= 70.0f);

        olineri_phoneme_feedback_t fb;
        assert(olineri_result_get_phoneme_feedback(result, 4, &fb) == OLINERI_OK);
        assert(std::string(fb.expected_phoneme) == "/h/");
        assert(fb.status == OLINERI_PHONEME_CORRECT);

        olineri_result_destroy(result);
        olineri_session_destroy(sa_session);
    }

    // 2. Hindi Assessment Session: "कमल"
    {
        olineri_session_handle_t hi_session = nullptr;
        assert(olineri_session_create(runtime, "hi-IN", &hi_session) == OLINERI_OK);

        olineri_result_handle_t result = nullptr;
        assert(olineri_assess_pronunciation(hi_session, "कमल", audio.data(), audio.size(), 16000, &result) == OLINERI_OK);
        assert(result != nullptr);
        assert(olineri_result_get_phoneme_count(result) == 5); // /k/, /ə/, /m/, /ə/, /l/ (schwa deleted!)
        assert(olineri_result_get_overall_score(result) >= 70.0f);

        olineri_phoneme_feedback_t fb;
        assert(olineri_result_get_phoneme_feedback(result, 4, &fb) == OLINERI_OK);
        assert(std::string(fb.expected_phoneme) == "/l/");
        assert(fb.status == OLINERI_PHONEME_CORRECT);

        olineri_result_destroy(result);
        olineri_session_destroy(hi_session);
    }

    // 3. Telugu Assessment Session: "అమ్మ"
    {
        olineri_session_handle_t te_session = nullptr;
        assert(olineri_session_create(runtime, "te-IN", &te_session) == OLINERI_OK);

        olineri_result_handle_t result = nullptr;
        assert(olineri_assess_pronunciation(te_session, "అమ్మ", audio.data(), audio.size(), 16000, &result) == OLINERI_OK);
        assert(result != nullptr);
        assert(olineri_result_get_phoneme_count(result) == 4); // /ɐ/, /m/, /m/, /ɐ/
        assert(olineri_result_get_overall_score(result) >= 70.0f);

        olineri_phoneme_feedback_t fb;
        assert(olineri_result_get_phoneme_feedback(result, 1, &fb) == OLINERI_OK);
        assert(std::string(fb.expected_phoneme) == "/m/");
        assert(fb.status == OLINERI_PHONEME_CORRECT);

        olineri_result_destroy(result);
        olineri_session_destroy(te_session);
    }

    olineri_runtime_destroy(runtime);
    olineri::fs::remove_all(TEST_SANDBOX);
    std::cout << "[PASS] Sanskrit, Hindi, and Telugu assessment sessions via C ABI" << std::endl;
}

int main() {
    std::cout << "=== Running Olineri Multi-Language Expansion Suite (sa, hi, te) ===" << std::endl;
    test_sanskrit_g2p();
    test_hindi_g2p();
    test_telugu_g2p();
    test_multilingual_assessment_sessions();
    std::cout << "All Sanskrit, Hindi, and Telugu tests passed successfully!" << std::endl;
    return 0;
}
