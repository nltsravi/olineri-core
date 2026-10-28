#ifndef VAANI_INTERNAL_FILESYSTEM_HPP
#define VAANI_INTERNAL_FILESYSTEM_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>

namespace vaani {
namespace fs {

inline bool is_safe_relative_path(const std::string& path) {
    if (path.empty()) return false;
    // Reject leading slashes or drive letters
    if (path[0] == '/' || path[0] == '\\') return false;
    if (path.size() >= 2 && path[1] == ':') return false;

    // Reject '..' traversal
    size_t pos = 0;
    while ((pos = path.find("..", pos)) != std::string::npos) {
        bool start_ok = (pos == 0 || path[pos - 1] == '/' || path[pos - 1] == '\\');
        bool end_ok = (pos + 2 == path.size() || path[pos + 2] == '/' || path[pos + 2] == '\\');
        if (start_ok && end_ok) {
            return false;
        }
        pos += 2;
    }
    return true;
}

inline bool file_exists(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

inline bool directory_exists(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

inline bool create_directories(const std::string& path) {
    if (path.empty() || directory_exists(path)) return true;

    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            if (!current.empty() && !directory_exists(current)) {
                mkdir(current.c_str(), 0755);
            }
        }
        current.push_back(c);
    }
    if (!current.empty() && !directory_exists(current)) {
        mkdir(current.c_str(), 0755);
    }
    return directory_exists(path);
}

inline bool remove_all(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return true; // already gone

    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return false;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            std::string subpath = path + "/" + name;
            remove_all(subpath);
        }
        closedir(dir);
        rmdir(path.c_str());
    } else {
        std::remove(path.c_str());
    }
    return !file_exists(path) && !directory_exists(path);
}

inline bool atomic_rename(const std::string& from, const std::string& to) {
    return std::rename(from.c_str(), to.c_str()) == 0;
}

inline bool read_file_to_string(const std::string& path, std::string& out_content) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    out_content = ss.str();
    return true;
}

inline bool write_string_to_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    return file.good();
}

} // namespace fs
} // namespace vaani

#endif // VAANI_INTERNAL_FILESYSTEM_HPP
