#ifndef OLINERI_INTERNAL_CRYPTO_HPP
#define OLINERI_INTERNAL_CRYPTO_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace olineri {
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
 * @brief Verifies an Ed25519 digital signature against a public key.
 * @param message Data that was signed.
 * @param signature 64-byte raw signature.
 * @param public_key 32-byte raw Ed25519 public key.
 * @return true if valid, false otherwise.
 */
bool ed25519_verify(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& public_key
);

} // namespace crypto
} // namespace olineri

#endif // OLINERI_INTERNAL_CRYPTO_HPP
