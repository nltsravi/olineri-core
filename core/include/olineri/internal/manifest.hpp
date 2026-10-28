#ifndef OLINERI_INTERNAL_MANIFEST_HPP
#define OLINERI_INTERNAL_MANIFEST_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "olineri/olineri_c_api.h"

namespace olineri {
namespace manifest {

struct ArtifactEntry {
    std::string path;
    uint64_t size_bytes = 0;
    std::string sha256;
};

struct ModelSpec {
    std::string id;
    std::string architecture;
    std::string quantization;
    uint32_t sample_rate = 16000;
    uint32_t feature_dimension = 80;
    std::string license;
    std::string provenance;
};

struct PublisherInfo {
    std::string name;
    std::string url;
    std::string public_key_id;
};

struct PackageManifest {
    std::string schema_version;
    std::string package_id;
    std::string language_tag;
    std::string script;
    std::string version;
    std::string min_sdk_version;
    std::string display_name;
    PublisherInfo publisher;
    std::vector<std::string> capabilities;
    ModelSpec model;
    std::vector<ArtifactEntry> artifacts;
    uint64_t total_size_bytes = 0;
    uint64_t installed_size_bytes = 0;
    std::vector<std::string> known_limitations;

    bool supports_capability(const std::string& cap) const {
        for (const auto& c : capabilities) {
            if (c == cap) return true;
        }
        return false;
    }
};

/**
 * @brief Parses JSON string into a PackageManifest struct.
 */
olineri_status_t parse_manifest(const std::string& json_text, PackageManifest& out_manifest);

/**
 * @brief Validates manifest compliance against current SDK version and schema requirements.
 */
olineri_status_t validate_manifest(const PackageManifest& manifest, const std::string& current_sdk_version);

/**
 * @brief Checks if version string 'required' is <= 'current'.
 */
bool is_version_compatible(const std::string& required, const std::string& current);

} // namespace manifest
} // namespace olineri

#endif // OLINERI_INTERNAL_MANIFEST_HPP
