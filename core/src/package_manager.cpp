#include "olineri/internal/package_manager.hpp"
#include "olineri/internal/filesystem.hpp"
#include "olineri/internal/crypto.hpp"
#include <chrono>
#include <dirent.h>

namespace olineri {
namespace pkg {

PackageManager::PackageManager(const std::string& storage_directory)
    : storage_dir_(storage_directory) {
    packages_dir_ = storage_dir_ + "/packages";
    staging_dir_ = storage_dir_ + "/staging";
    backups_dir_ = storage_dir_ + "/backups";

    fs::create_directories(packages_dir_);
    fs::create_directories(staging_dir_);
    fs::create_directories(backups_dir_);

    refresh_registry();
}

PackageManager::~PackageManager() = default;

void PackageManager::add_trusted_key(const std::string& key_id, const std::string& public_key_hex) {
    std::lock_guard<std::mutex> lock(mutex_);
    trusted_keys_[key_id] = public_key_hex;
}

std::string PackageManager::get_package_dir(const std::string& package_id) const {
    return packages_dir_ + "/" + package_id;
}

void PackageManager::refresh_registry() {
    std::lock_guard<std::mutex> lock(mutex_);
    registry_.clear();

    DIR* dir = opendir(packages_dir_.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string pkg_path = packages_dir_ + "/" + name;
        std::string manifest_file = pkg_path + "/manifest.json";

        if (fs::file_exists(manifest_file)) {
            std::string json_text;
            if (fs::read_file_to_string(manifest_file, json_text)) {
                manifest::PackageManifest manifest;
                if (manifest::parse_manifest(json_text, manifest) == OLINERI_OK) {
                    registry_[manifest.package_id] = manifest;
                }
            }
        }
    }
    closedir(dir);
}

olineri_status_t PackageManager::verify_artifacts(const std::string& base_dir, const manifest::PackageManifest& manifest) {
    for (const auto& art : manifest.artifacts) {
        if (!fs::is_safe_relative_path(art.path)) {
            return OLINERI_ERR_PATH_TRAVERSAL;
        }

        std::string full_path = base_dir + "/" + art.path;
        if (!fs::file_exists(full_path)) {
            return OLINERI_ERR_PACKAGE_CORRUPTED;
        }

        std::string actual_hash = crypto::sha256_file(full_path);
        if (!crypto::constant_time_equals(actual_hash, art.sha256)) {
            return OLINERI_ERR_PACKAGE_CORRUPTED;
        }
    }
    return OLINERI_OK;
}

olineri_status_t PackageManager::verify_signature(
    const std::string& manifest_json,
    const std::string& sig_hex,
    const std::string& key_id
) {
    auto it = trusted_keys_.find(key_id);
    if (it == trusted_keys_.end()) {
        if (!trusted_keys_.empty()) {
            return OLINERI_ERR_SIGNATURE_INVALID;
        }
        return OLINERI_OK;
    }

    std::vector<uint8_t> pub_bytes;
    if (!crypto::hex_to_bytes(it->second, pub_bytes) || pub_bytes.size() != 32) {
        return OLINERI_ERR_SIGNATURE_INVALID;
    }

    std::vector<uint8_t> sig_bytes;
    if (!crypto::hex_to_bytes(sig_hex, sig_bytes) || sig_bytes.size() != 64) {
        return OLINERI_ERR_SIGNATURE_INVALID;
    }

    std::vector<uint8_t> msg_bytes(manifest_json.begin(), manifest_json.end());
    if (!crypto::ed25519_verify(msg_bytes, sig_bytes, pub_bytes)) {
        return OLINERI_ERR_SIGNATURE_INVALID;
    }

    return OLINERI_OK;
}

olineri_status_t PackageManager::install(const std::string& source_path, std::string& out_package_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string manifest_path = source_path + "/manifest.json";
    if (!fs::file_exists(manifest_path)) {
        return OLINERI_ERR_PACKAGE_CORRUPTED;
    }

    std::string manifest_json;
    if (!fs::read_file_to_string(manifest_path, manifest_json)) {
        return OLINERI_ERR_PACKAGE_CORRUPTED;
    }

    manifest::PackageManifest manifest;
    olineri_status_t status = manifest::parse_manifest(manifest_json, manifest);
    if (status != OLINERI_OK) return status;

    status = manifest::validate_manifest(manifest, OLINERI_VERSION_STRING);
    if (status != OLINERI_OK) return status;

    // Check signature if present or required
    std::string sig_path = source_path + "/manifest.sig";
    if (fs::file_exists(sig_path)) {
        std::string sig_content;
        if (fs::read_file_to_string(sig_path, sig_content)) {
            while (!sig_content.empty() && (sig_content.back() == '\n' || sig_content.back() == '\r' || sig_content.back() == ' ')) {
                sig_content.pop_back();
            }
            status = verify_signature(manifest_json, sig_content, manifest.publisher.public_key_id);
            if (status != OLINERI_OK) return status;
        }
    } else if (!trusted_keys_.empty()) {
        return OLINERI_ERR_SIGNATURE_INVALID;
    }

    // Verify artifact checksums
    status = verify_artifacts(source_path, manifest);
    if (status != OLINERI_OK) return status;

    // Stage files atomically
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string stage_dir = staging_dir_ + "/" + manifest.package_id + "." + std::to_string(now);
    fs::create_directories(stage_dir);

    // Copy manifest and signature
    fs::write_string_to_file(stage_dir + "/manifest.json", manifest_json);
    if (fs::file_exists(sig_path)) {
        std::string sig_data;
        fs::read_file_to_string(sig_path, sig_data);
        fs::write_string_to_file(stage_dir + "/manifest.sig", sig_data);
    }

    // Copy artifacts
    for (const auto& art : manifest.artifacts) {
        std::string src_file = source_path + "/" + art.path;
        std::string dst_file = stage_dir + "/" + art.path;

        size_t last_slash = dst_file.find_last_of('/');
        if (last_slash != std::string::npos) {
            fs::create_directories(dst_file.substr(0, last_slash));
        }

        std::string content;
        if (!fs::read_file_to_string(src_file, content) || !fs::write_string_to_file(dst_file, content)) {
            fs::remove_all(stage_dir);
            return OLINERI_ERR_STORAGE_UNWRITABLE;
        }
    }

    // Atomic installation swap
    std::string target_dir = packages_dir_ + "/" + manifest.package_id;
    std::string backup_dir = backups_dir_ + "/" + manifest.package_id;

    if (fs::directory_exists(target_dir)) {
        fs::remove_all(backup_dir);
        if (!fs::atomic_rename(target_dir, backup_dir)) {
            fs::remove_all(stage_dir);
            return OLINERI_ERR_STORAGE_UNWRITABLE;
        }
    }

    if (!fs::atomic_rename(stage_dir, target_dir)) {
        if (fs::directory_exists(backup_dir)) {
            fs::atomic_rename(backup_dir, target_dir);
        }
        fs::remove_all(stage_dir);
        return OLINERI_ERR_STORAGE_UNWRITABLE;
    }

    registry_[manifest.package_id] = manifest;
    out_package_id = manifest.package_id;
    return OLINERI_OK;
}

olineri_status_t PackageManager::uninstall(const std::string& package_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = registry_.find(package_id);
    if (it == registry_.end()) {
        return OLINERI_ERR_PACKAGE_NOT_FOUND;
    }

    std::string target_dir = packages_dir_ + "/" + package_id;
    std::string backup_dir = backups_dir_ + "/" + package_id;

    fs::remove_all(target_dir);
    fs::remove_all(backup_dir);
    registry_.erase(it);

    return OLINERI_OK;
}

olineri_status_t PackageManager::rollback(const std::string& package_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string target_dir = packages_dir_ + "/" + package_id;
    std::string backup_dir = backups_dir_ + "/" + package_id;

    if (!fs::directory_exists(backup_dir)) {
        return OLINERI_ERR_PACKAGE_NOT_FOUND;
    }

    fs::remove_all(target_dir);
    if (!fs::atomic_rename(backup_dir, target_dir)) {
        return OLINERI_ERR_STORAGE_UNWRITABLE;
    }

    std::string manifest_path = target_dir + "/manifest.json";
    std::string json_text;
    if (fs::read_file_to_string(manifest_path, json_text)) {
        manifest::PackageManifest manifest;
        if (manifest::parse_manifest(json_text, manifest) == OLINERI_OK) {
            registry_[manifest.package_id] = manifest;
        }
    }

    return OLINERI_OK;
}

std::vector<manifest::PackageManifest> PackageManager::list_installed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<manifest::PackageManifest> result;
    for (const auto& kv : registry_) {
        result.push_back(kv.second);
    }
    return result;
}

bool PackageManager::get_installed(const std::string& package_id, manifest::PackageManifest& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = registry_.find(package_id);
    if (it != registry_.end()) {
        out = it->second;
        return true;
    }
    return false;
}

bool PackageManager::find_by_language(const std::string& language_tag, manifest::PackageManifest& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& kv : registry_) {
        if (kv.second.language_tag == language_tag) {
            out = kv.second;
            return true;
        }
        if (language_tag.size() == 2 && kv.second.language_tag.compare(0, 2, language_tag) == 0) {
            out = kv.second;
            return true;
        }
    }
    return false;
}

} // namespace pkg
} // namespace olineri
