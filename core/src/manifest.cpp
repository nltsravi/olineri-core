#include "olineri/internal/manifest.hpp"
#include "olineri/internal/json.hpp"
#include "olineri/internal/filesystem.hpp"
#include <sstream>
#include <vector>

namespace olineri {
namespace manifest {

namespace {

std::vector<int> parse_version_triplet(const std::string& ver) {
    std::vector<int> parts;
    std::stringstream ss(ver);
    std::string item;
    while (std::getline(ss, item, '.')) {
        try {
            parts.push_back(std::stoi(item));
        } catch (...) {
            parts.push_back(0);
        }
    }
    while (parts.size() < 3) parts.push_back(0);
    return parts;
}

} // anonymous namespace

bool is_version_compatible(const std::string& required, const std::string& current) {
    auto req_parts = parse_version_triplet(required);
    auto cur_parts = parse_version_triplet(current);

    for (size_t i = 0; i < 3; ++i) {
        if (cur_parts[i] > req_parts[i]) return true;
        if (cur_parts[i] < req_parts[i]) return false;
    }
    return true; // Exactly equal
}

olineri_status_t parse_manifest(const std::string& json_text, PackageManifest& out) {
    auto root = json::parse(json_text);
    if (!root.is_object()) {
        return OLINERI_ERR_PACKAGE_CORRUPTED;
    }

    out.schema_version = root["schemaVersion"].as_string();
    out.package_id = root["packageId"].as_string();
    out.language_tag = root["language"].as_string();
    out.script = root["script"].as_string();
    out.version = root["version"].as_string();
    out.min_sdk_version = root["minSdkVersion"].as_string();
    out.display_name = root["displayName"].as_string();

    auto pub = root["publisher"];
    if (pub.is_object()) {
        out.publisher.name = pub["name"].as_string();
        out.publisher.url = pub["url"].as_string();
        out.publisher.public_key_id = pub["publicKeyId"].as_string();
    }

    auto caps = root["capabilities"];
    if (caps.is_array()) {
        for (const auto& c : caps.as_array()) {
            if (c.is_string()) {
                out.capabilities.push_back(c.as_string());
            }
        }
    }

    auto mdl = root["model"];
    if (mdl.is_object()) {
        out.model.id = mdl["id"].as_string();
        out.model.architecture = mdl["architecture"].as_string();
        out.model.quantization = mdl["quantization"].as_string();
        out.model.sample_rate = static_cast<uint32_t>(mdl["sampleRate"].as_int());
        if (out.model.sample_rate == 0) out.model.sample_rate = 16000;
        out.model.feature_dimension = static_cast<uint32_t>(mdl["featureDimension"].as_int());
        if (out.model.feature_dimension == 0) out.model.feature_dimension = 80;
        out.model.license = mdl["license"].as_string();
        out.model.provenance = mdl["provenance"].as_string();
    }

    auto arts = root["artifacts"];
    if (arts.is_array()) {
        for (const auto& a : arts.as_array()) {
            if (a.is_object()) {
                ArtifactEntry entry;
                entry.path = a["path"].as_string();
                entry.size_bytes = static_cast<uint64_t>(a["sizeBytes"].as_int());
                entry.sha256 = a["sha256"].as_string();
                out.artifacts.push_back(entry);
            }
        }
    }

    out.total_size_bytes = static_cast<uint64_t>(root["totalSizeBytes"].as_int());
    out.installed_size_bytes = static_cast<uint64_t>(root["installedSizeBytes"].as_int());

    auto lims = root["knownLimitations"];
    if (lims.is_array()) {
        for (const auto& l : lims.as_array()) {
            if (l.is_string()) {
                out.known_limitations.push_back(l.as_string());
            }
        }
    }

    return OLINERI_OK;
}

olineri_status_t validate_manifest(const PackageManifest& manifest, const std::string& current_sdk_version) {
    if (manifest.package_id.empty() || manifest.language_tag.empty() || manifest.version.empty()) {
        return OLINERI_ERR_PACKAGE_CORRUPTED;
    }

    // Check schema version (must be 1.x)
    if (manifest.schema_version.rfind("1.", 0) != 0 && manifest.schema_version != "1.0.0") {
        return OLINERI_ERR_INCOMPATIBLE_VERSION;
    }

    // Check minimum SDK version compatibility
    if (!manifest.min_sdk_version.empty()) {
        if (!is_version_compatible(manifest.min_sdk_version, current_sdk_version)) {
            return OLINERI_ERR_INCOMPATIBLE_VERSION;
        }
    }

    // Artifacts must exist and paths must be strictly safe relative paths
    if (manifest.artifacts.empty()) {
        return OLINERI_ERR_PACKAGE_CORRUPTED;
    }

    for (const auto& art : manifest.artifacts) {
        if (art.path.empty() || !fs::is_safe_relative_path(art.path)) {
            return OLINERI_ERR_PATH_TRAVERSAL;
        }
        if (art.sha256.length() != 64) {
            return OLINERI_ERR_PACKAGE_CORRUPTED;
        }
    }

    return OLINERI_OK;
}

} // namespace manifest
} // namespace olineri
