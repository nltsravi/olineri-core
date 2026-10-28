#include "vaani/vaani_c_api.h"
#include "vaani/internal/filesystem.hpp"
#include "vaani/internal/crypto.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

static const std::string TEST_DIR = "/tmp/vaani_test_runtime_c_api";

std::string setup_mock_package() {
    std::string pkg_src = TEST_DIR + "/src_pkg";
    vaani::fs::create_directories(pkg_src + "/model");

    std::string dummy_model = "mock_model_binary_data";
    std::string model_path = pkg_src + "/model/acoustic.bin";
    vaani::fs::write_string_to_file(model_path, dummy_model);
    std::string sha = vaani::crypto::sha256_file(model_path);

    std::string manifest_json = R"({
      "schemaVersion": "1.0.0",
      "packageId": "lang-ta",
      "language": "ta-IN",
      "script": "Taml",
      "version": "1.0.0",
      "minSdkVersion": "0.1.0",
      "displayName": "Tamil Pilot",
      "publisher": { "name": "Vaani Team", "publicKeyId": "test-key" },
      "capabilities": ["speechRecognition", "pronunciationAssessment"],
      "model": { "id": "mock-model", "license": "MIT" },
      "artifacts": [
        {
          "path": "model/acoustic.bin",
          "sizeBytes": )" + std::to_string(dummy_model.size()) + R"(,
          "sha256": ")" + sha + R"("
        }
      ]
    })";
    vaani::fs::write_string_to_file(pkg_src + "/manifest.json", manifest_json);
    return pkg_src;
}

void test_full_c_abi_lifecycle() {
    vaani::fs::remove_all(TEST_DIR);
    vaani::fs::create_directories(TEST_DIR);

    // 1. Version check
    assert(std::string(vaani_version()) == "0.1.0");
    assert(std::string(vaani_status_message(VAANI_OK)) == "Success");

    // 2. Runtime creation
    std::string storage = TEST_DIR + "/storage";
    vaani_runtime_handle_t runtime = nullptr;
    assert(vaani_runtime_create(storage.c_str(), &runtime) == VAANI_OK);
    assert(runtime != nullptr);

    // 3. Package installation
    std::string pkg_src = setup_mock_package();
    char pkg_id[64] = {0};
    assert(vaani_package_install(runtime, pkg_src.c_str(), pkg_id, sizeof(pkg_id)) == VAANI_OK);
    assert(std::string(pkg_id) == "lang-ta");
    assert(vaani_package_count(runtime) == 1);

    char lang_tag[32] = {0};
    char version[32] = {0};
    assert(vaani_package_get_at(runtime, 0, pkg_id, sizeof(pkg_id), lang_tag, sizeof(lang_tag), version, sizeof(version)) == VAANI_OK);
    assert(std::string(lang_tag) == "ta-IN");

    // 4. Session creation
    vaani_session_handle_t session = nullptr;
    assert(vaani_session_create(runtime, "ta-IN", &session) == VAANI_OK);
    assert(session != nullptr);

    // 5. Audio inputs and assessment
    // Generate 1.0 second of clean sine wave audio (16000 samples)
    std::vector<float> audio(16000);
    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(i) / 16000.0f);
    }

    // Negative test: invalid audio (empty/zero sample rate)
    vaani_result_handle_t result = nullptr;
    assert(vaani_assess_pronunciation(session, "தமிழ்", nullptr, 0, 16000, &result) == VAANI_ERR_INVALID_ARGUMENT);

    // Positive assessment: "தமிழ்" (Tamil)
    assert(vaani_assess_pronunciation(session, "தமிழ்", audio.data(), static_cast<uint32_t>(audio.size()), 16000, &result) == VAANI_OK);
    assert(result != nullptr);

    float overall_score = vaani_result_get_overall_score(result);
    assert(overall_score >= 0.0f && overall_score <= 100.0f);

    uint32_t count = vaani_result_get_phoneme_count(result);
    assert(count > 0);

    for (uint32_t i = 0; i < count; ++i) {
        vaani_phoneme_feedback_t fb;
        assert(vaani_result_get_phoneme_feedback(result, i, &fb) == VAANI_OK);
        assert(fb.expected_phoneme != nullptr);
        assert(fb.score >= 0.0f && fb.score <= 100.0f);
        assert(fb.char_end_utf8 > fb.char_start_utf8);
    }

    std::cout << "[PASS] Full C ABI lifecycle & assessment (Phonemes: " << count 
              << ", Score: " << overall_score << ")" << std::endl;

    // 6. Cleanup
    vaani_result_destroy(result);
    assert(vaani_session_destroy(session) == VAANI_OK);
    assert(vaani_package_uninstall(runtime, "lang-ta") == VAANI_OK);
    assert(vaani_package_count(runtime) == 0);
    assert(vaani_runtime_destroy(runtime) == VAANI_OK);

    vaani::fs::remove_all(TEST_DIR);
}

int main() {
    test_full_c_abi_lifecycle();
    return 0;
}
