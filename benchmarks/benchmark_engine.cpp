#include "olineri/olineri_c_api.h"
#include "olineri/internal/g2p.hpp"
#include "olineri/internal/filesystem.hpp"
#include "olineri/internal/crypto.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>

static const std::string BENCHMARK_SANDBOX = "/tmp/olineri_benchmark_sandbox";

static void setup_dummy_package(const std::string& pkg_id, const std::string& lang_tag) {
    std::string pkg_dir = BENCHMARK_SANDBOX + "/" + pkg_id;
    olineri::fs::create_directories(pkg_dir + "/model");

    std::string dummy_onnx = "indicconformer_ctc_" + lang_tag + "_int8_weights";
    std::string model_file = pkg_dir + "/model/acoustic_model.onnx";
    olineri::fs::write_string_to_file(model_file, dummy_onnx);
    std::string sha = olineri::crypto::sha256_file(model_file);

    std::string manifest = R"RAW({
      "schemaVersion": "1.0.0",
      "packageId": ")RAW" + pkg_id + R"RAW(",
      "language": ")RAW" + lang_tag + R"RAW(",
      "script": "Brahmic",
      "version": "1.0.0",
      "minSdkVersion": "0.1.0",
      "displayName": ")RAW" + pkg_id + R"RAW(",
      "publisher": { "name": "Olineri Benchmark", "publicKeyId": "bench-key" },
      "capabilities": ["pronunciationAssessment", "phonemeFeedback"],
      "model": {
        "id": "indicconformer-ctc-)RAW" + lang_tag + R"RAW(-int8",
        "architecture": "Conformer-CTC",
        "quantization": "int8",
        "sampleRate": 16000,
        "featureDimension": 80,
        "license": "MIT"
      },
      "artifacts": [
        { "path": "model/acoustic_model.onnx", "sizeBytes": )RAW" + std::to_string(dummy_onnx.size()) + R"RAW(, "sha256": ")RAW" + sha + R"RAW(" }
      ],
      "totalSizeBytes": )RAW" + std::to_string(dummy_onnx.size()) + R"RAW(,
      "installedSizeBytes": )RAW" + std::to_string(dummy_onnx.size() * 2) + R"RAW(
    })RAW";

    olineri::fs::write_string_to_file(pkg_dir + "/manifest.json", manifest);
}

void run_g2p_benchmark() {
    std::cout << "\n--- 1. Brahmic G2P Transformation Throughput ---" << std::endl;

    struct TestCase {
        std::string lang;
        std::string text;
    };

    std::vector<TestCase> cases = {
        {"ta-IN", "பழம் பலம் அறம் அரம்"},
        {"sa-IN", "नमः शिवाय शान्तिः रामः"},
        {"hi-IN", "कमल फल चलना अपना"},
        {"te-IN", "అమ్మ ఎండ తెలుగు అన్న"}
    };

    const int ITERATIONS = 20000;

    for (const auto& tc : cases) {
        olineri::g2p::BrahmicG2P g2p({tc.lang, true, true});

        auto start = std::chrono::high_resolution_clock::now();
        size_t total_phonemes = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto targets = g2p.convert(tc.text);
            total_phonemes += targets.size();
        }
        (void)total_phonemes;
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        double words_per_sec = (ITERATIONS * 4.0) / (elapsed_ms / 1000.0);

        std::cout << "Language: " << tc.lang << " | Phrase: '" << tc.text << "'"
                  << " | " << std::fixed << std::setprecision(0) << words_per_sec << " words/sec"
                  << " (" << std::setprecision(3) << (elapsed_ms / ITERATIONS) << " ms/phrase)" << std::endl;
    }
}

void run_session_latency_benchmark() {
    std::cout << "\n--- 2. Native C ABI Latency Benchmark ---" << std::endl;

    olineri::fs::remove_all(BENCHMARK_SANDBOX);
    std::string storage = BENCHMARK_SANDBOX + "/storage";
    olineri::fs::create_directories(storage);

    olineri_runtime_handle_t runtime = nullptr;
    olineri_runtime_create(storage.c_str(), &runtime);

    setup_dummy_package("lang-ta", "ta-IN");
    setup_dummy_package("lang-sa", "sa-IN");
    setup_dummy_package("lang-hi", "hi-IN");
    setup_dummy_package("lang-te", "te-IN");

    char out_id[64] = {0};
    olineri_package_install(runtime, (BENCHMARK_SANDBOX + "/lang-ta").c_str(), out_id, sizeof(out_id));
    olineri_package_install(runtime, (BENCHMARK_SANDBOX + "/lang-sa").c_str(), out_id, sizeof(out_id));
    olineri_package_install(runtime, (BENCHMARK_SANDBOX + "/lang-hi").c_str(), out_id, sizeof(out_id));
    olineri_package_install(runtime, (BENCHMARK_SANDBOX + "/lang-te").c_str(), out_id, sizeof(out_id));

    // 16 kHz 1.0s audio
    std::vector<float> audio(16000, 0.1f);

    struct LangBench {
        std::string tag;
        std::string phrase;
    };

    std::vector<LangBench> langs = {
        {"ta-IN", "பழம்"},
        {"sa-IN", "नमः"},
        {"hi-IN", "कमल"},
        {"te-IN", "అమ్మ"}
    };

    for (const auto& lb : langs) {
        // Cold start latency
        auto start_cold = std::chrono::high_resolution_clock::now();
        olineri_session_handle_t session = nullptr;
        olineri_session_create(runtime, lb.tag.c_str(), &session);
        auto end_cold = std::chrono::high_resolution_clock::now();
        double cold_ms = std::chrono::duration<double, std::milli>(end_cold - start_cold).count();

        // Warm assessment latency
        const int WARM_ITERS = 1000;
        auto start_warm = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < WARM_ITERS; ++i) {
            olineri_result_handle_t res = nullptr;
            olineri_assess_pronunciation(session, lb.phrase.c_str(), audio.data(), audio.size(), 16000, &res);
            olineri_result_destroy(res);
        }
        auto end_warm = std::chrono::high_resolution_clock::now();
        double warm_avg_ms = std::chrono::duration<double, std::milli>(end_warm - start_warm).count() / WARM_ITERS;

        olineri_session_destroy(session);

        std::cout << "Language: " << lb.tag
                  << " | Cold Session Create: " << std::fixed << std::setprecision(4) << cold_ms << " ms"
                  << " | Warm Assessment: " << std::setprecision(4) << warm_avg_ms << " ms" << std::endl;
    }

    olineri_runtime_destroy(runtime);
    olineri::fs::remove_all(BENCHMARK_SANDBOX);
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "        Olineri Core SDK — Performance Benchmarking       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    run_g2p_benchmark();
    run_session_latency_benchmark();

    std::cout << "\n[PASS] Benchmarking harness completed successfully." << std::endl;
    return 0;
}
