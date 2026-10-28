#include "vaani/internal/package_manager.hpp"
#include "vaani/internal/filesystem.hpp"
#include "vaani/internal/crypto.hpp"
#include <iostream>
#include <cassert>

static const std::string TEST_DIR = "/tmp/vaani_test_pkg_mgr";

void setup_test_sandbox() {
    vaani::fs::remove_all(TEST_DIR);
    vaani::fs::create_directories(TEST_DIR);
}

void cleanup_test_sandbox() {
    vaani::fs::remove_all(TEST_DIR);
}

std::string create_dummy_package(const std::string& pkg_name, const std::string& version, const std::string& dummy_content) {
    std::string pkg_src = TEST_DIR + "/src_" + pkg_name;
    vaani::fs::create_directories(pkg_src + "/model");

    std::string model_file = pkg_src + "/model/acoustic.bin";
    vaani::fs::write_string_to_file(model_file, dummy_content);
    std::string sha = vaani::crypto::sha256_file(model_file);

    std::string manifest_json = R"({
      "schemaVersion": "1.0.0",
      "packageId": ")" + pkg_name + R"(",
      "language": "ta-IN",
      "script": "Taml",
      "version": ")" + version + R"(",
      "minSdkVersion": "0.1.0",
      "displayName": "Tamil Pilot",
      "publisher": { "name": "Vaani Team", "publicKeyId": "test-key" },
      "capabilities": ["pronunciationAssessment"],
      "model": { "id": "mock-model", "license": "MIT" },
      "artifacts": [
        {
          "path": "model/acoustic.bin",
          "sizeBytes": )" + std::to_string(dummy_content.size()) + R"(,
          "sha256": ")" + sha + R"("
        }
      ]
    })";

    vaani::fs::write_string_to_file(pkg_src + "/manifest.json", manifest_json);
    return pkg_src;
}

void test_package_install_and_query() {
    setup_test_sandbox();
    std::string storage = TEST_DIR + "/storage";
    vaani::pkg::PackageManager pm(storage);

    std::string src = create_dummy_package("lang-ta", "1.0.0", "binary_weights_v1");
    std::string installed_id;
    assert(pm.install(src, installed_id) == VAANI_OK);
    assert(installed_id == "lang-ta");

    auto list = pm.list_installed();
    assert(list.size() == 1);
    assert(list[0].package_id == "lang-ta");
    assert(list[0].version == "1.0.0");

    vaani::manifest::PackageManifest found;
    assert(pm.find_by_language("ta-IN", found));
    assert(found.package_id == "lang-ta");

    std::cout << "[PASS] Package install, integrity verification, and querying" << std::endl;
}

void test_tampered_package_rejection() {
    std::string storage = TEST_DIR + "/storage";
    vaani::pkg::PackageManager pm(storage);

    std::string src = create_dummy_package("lang-corrupted", "1.0.0", "legit_data");
    // Maliciously tamper with the file after manifest generation
    vaani::fs::write_string_to_file(src + "/model/acoustic.bin", "tampered_payload");

    std::string installed_id;
    vaani_status_t status = pm.install(src, installed_id);
    assert(status == VAANI_ERR_PACKAGE_CORRUPTED);

    vaani::manifest::PackageManifest dummy;
    assert(!pm.get_installed("lang-corrupted", dummy));

    std::cout << "[PASS] Tampered artifact checksum rejection" << std::endl;
}

void test_atomic_update_and_rollback() {
    std::string storage = TEST_DIR + "/storage";
    vaani::pkg::PackageManager pm(storage);

    // Install v1.0.0
    std::string src_v1 = create_dummy_package("lang-ta", "1.0.0", "weights_v1");
    std::string id;
    assert(pm.install(src_v1, id) == VAANI_OK);

    // Update to v1.1.0
    std::string src_v2 = create_dummy_package("lang-ta", "1.1.0", "weights_v2_optimized");
    assert(pm.install(src_v2, id) == VAANI_OK);

    vaani::manifest::PackageManifest current;
    assert(pm.get_installed("lang-ta", current));
    assert(current.version == "1.1.0");

    // Perform Rollback
    assert(pm.rollback("lang-ta") == VAANI_OK);

    assert(pm.get_installed("lang-ta", current));
    assert(current.version == "1.0.0"); // Restored to v1.0.0!

    std::cout << "[PASS] Atomic update and rollback to previous version" << std::endl;
}

void test_uninstall() {
    std::string storage = TEST_DIR + "/storage";
    vaani::pkg::PackageManager pm(storage);

    assert(pm.uninstall("lang-ta") == VAANI_OK);
    assert(pm.list_installed().empty());
    assert(pm.uninstall("lang-ta") == VAANI_ERR_PACKAGE_NOT_FOUND);

    std::cout << "[PASS] Package uninstallation and registry pruning" << std::endl;
    cleanup_test_sandbox();
}

int main() {
    test_package_install_and_query();
    test_tampered_package_rejection();
    test_atomic_update_and_rollback();
    test_uninstall();
    return 0;
}
