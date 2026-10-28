#ifndef VAANI_INTERNAL_PACKAGE_MANAGER_HPP
#define VAANI_INTERNAL_PACKAGE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include "vaani/vaani_c_api.h"
#include "vaani/internal/manifest.hpp"

namespace vaani {
namespace pkg {

class PackageManager {
public:
    explicit PackageManager(const std::string& storage_directory);
    ~PackageManager();

    void add_trusted_key(const std::string& key_id, const std::string& public_key_hex);

    vaani_status_t install(const std::string& source_path, std::string& out_package_id);
    vaani_status_t uninstall(const std::string& package_id);
    vaani_status_t rollback(const std::string& package_id);

    std::vector<manifest::PackageManifest> list_installed() const;
    bool get_installed(const std::string& package_id, manifest::PackageManifest& out) const;
    bool find_by_language(const std::string& language_tag, manifest::PackageManifest& out) const;

    std::string get_package_dir(const std::string& package_id) const;

    void refresh_registry();

private:
    std::string storage_dir_;
    std::string packages_dir_;
    std::string staging_dir_;
    std::string backups_dir_;

    mutable std::mutex mutex_;
    std::map<std::string, std::string> trusted_keys_; // key_id -> hex public key
    std::map<std::string, manifest::PackageManifest> registry_; // package_id -> manifest

    vaani_status_t verify_artifacts(const std::string& base_dir, const manifest::PackageManifest& manifest);
    vaani_status_t verify_signature(const std::string& manifest_json, const std::string& sig_hex, const std::string& key_id);
};

} // namespace pkg
} // namespace vaani

#endif // VAANI_INTERNAL_PACKAGE_MANAGER_HPP
