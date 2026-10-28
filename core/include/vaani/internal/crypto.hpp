#ifndef VAANI_INTERNAL_CRYPTO_HPP
#define VAANI_INTERNAL_CRYPTO_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace vaani {
namespace crypto {

/**
 * @brief Computes SHA-256 hash of a memory buffer.
 * @return 64-character lowercase hex string.
 */
std::string sha256_buffer(const uint8_t* data, size_t length);
std::string sha256_string(const std::string& input);

/**
 * @brief Computes SHA-256 hash of a file on disk.
 * @return 64-character lowercase hex string, or empty string on read error.
 */
std::string sha256_file(const std::string& file_path);

/**
 * @brief Constant-time string comparison to prevent timing attacks.
 */
bool constant_time_equals(const std::string& a, const std::string& b);

/**
 * @brief Converts hex string to byte vector.
 */
bool hex_to_bytes(const std::string& hex, std::vector<uint8_t>& out_bytes);

/**
 * @brief Converts byte vector to hex string.
 */
std::string bytes_to_hex(const uint8_t* data, size_t length);

/**
 * @brief Verifies an Ed25519 detached signature.
 * @param message_bytes The raw message content.
 * @param signature_bytes The 64-byte signature.
 * @param public_key_bytes The 32-byte public key.
 * @return true if signature is mathematically valid, false otherwise.
 */
bool ed25519_verify(
    const std::vector<uint8_t>& message_bytes,
    const std::vector<uint8_t>& signature_bytes,
    const std::vector<uint8_t>& public_key_bytes
);

} // namespace crypto
} // namespace vaani

#endif // VAANI_INTERNAL_CRYPTO_HPP
