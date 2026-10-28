#include "vaani/internal/manifest.hpp"
#include <iostream>
#include <cassert>

static const char* VALID_MANIFEST_JSON = R"RAW({
  "schemaVersion": "1.0.0",
  "packageId": "lang-ta",
  "language": "ta-IN",
  "script": "Taml",
  "version": "1.0.0",
  "minSdkVersion": "0.1.0",
  "displayName": "Tamil (தமிழ்)",
  "publisher": {
    "name": "Vaani Core Team",
    "publicKeyId": "vaani-key-01"
  },
  "capabilities": ["speechRecognition", "pronunciationAssessment"],
  "model": {
    "id": "indicconformer-ta",
    "architecture": "Conformer-CTC",
    "quantization": "int8",
    "sampleRate": 16000,
    "featureDimension": 80,
    "license": "MIT"
  },
  "artifacts": [
    {
      "path": "model/acoustic_model.onnx",
      "sizeBytes": 1024,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    }
  ],
  "totalSizeBytes": 1024,
  "installedSizeBytes": 2048
})RAW";

void test_valid_manifest() {
    vaani::manifest::PackageManifest manifest;
    assert(vaani::manifest::parse_manifest(VALID_MANIFEST_JSON, manifest) == VAANI_OK);
    assert(manifest.package_id == "lang-ta");
    assert(manifest.language_tag == "ta-IN");
    assert(manifest.supports_capability("pronunciationAssessment"));
    assert(!manifest.supports_capability("tts"));
    assert(manifest.artifacts.size() == 1);
    assert(vaani::manifest::validate_manifest(manifest, "0.1.0") == VAANI_OK);
    std::cout << "[PASS] Valid manifest parsing and validation" << std::endl;
}

void test_version_compatibility() {
    assert(vaani::manifest::is_version_compatible("0.1.0", "0.1.0"));
    assert(vaani::manifest::is_version_compatible("0.1.0", "0.2.0"));
    assert(vaani::manifest::is_version_compatible("0.1.0", "1.0.0"));
    assert(!vaani::manifest::is_version_compatible("0.2.0", "0.1.0"));
    assert(!vaani::manifest::is_version_compatible("1.0.0", "0.9.9"));
    std::cout << "[PASS] Version compatibility logic" << std::endl;
}

void test_incompatible_sdk_version() {
    vaani::manifest::PackageManifest manifest;
    assert(vaani::manifest::parse_manifest(VALID_MANIFEST_JSON, manifest) == VAANI_OK);
    manifest.min_sdk_version = "2.0.0";
    // Current SDK is 0.1.0, should reject
    assert(vaani::manifest::validate_manifest(manifest, "0.1.0") == VAANI_ERR_INCOMPATIBLE_VERSION);
    std::cout << "[PASS] Incompatible SDK version rejection" << std::endl;
}

void test_path_traversal_rejection() {
    vaani::manifest::PackageManifest manifest;
    assert(vaani::manifest::parse_manifest(VALID_MANIFEST_JSON, manifest) == VAANI_OK);
    // Inject malicious path traversal artifact
    manifest.artifacts[0].path = "../../../lib/system.so";
    assert(vaani::manifest::validate_manifest(manifest, "0.1.0") == VAANI_ERR_PATH_TRAVERSAL);
    std::cout << "[PASS] Path traversal artifact rejection" << std::endl;
}

int main() {
    test_valid_manifest();
    test_version_compatibility();
    test_incompatible_sdk_version();
    test_path_traversal_rejection();
    return 0;
}
