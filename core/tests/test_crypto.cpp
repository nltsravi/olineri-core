#include "vaani/internal/crypto.hpp"
#include <iostream>
#include <cassert>

void test_sha256_known_vectors() {
    // RFC 6234 test vector 1: "" (empty string)
    assert(vaani::crypto::sha256_string("") == 
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // RFC 6234 test vector 2: "abc"
    assert(vaani::crypto::sha256_string("abc") == 
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // RFC 6234 test vector 3: "The quick brown fox jumps over the lazy dog"
    assert(vaani::crypto::sha256_string("The quick brown fox jumps over the lazy dog") == 
           "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");

    std::cout << "[PASS] SHA-256 standard vectors" << std::endl;
}

void test_hex_conversion() {
    std::string hex = "0123456789abcdef";
    std::vector<uint8_t> bytes;
    assert(vaani::crypto::hex_to_bytes(hex, bytes));
    assert(bytes.size() == 8);
    assert(vaani::crypto::bytes_to_hex(bytes.data(), bytes.size()) == hex);

    // Invalid hex length
    assert(!vaani::crypto::hex_to_bytes("abc", bytes));
    // Invalid characters
    assert(!vaani::crypto::hex_to_bytes("01zz", bytes));

    std::cout << "[PASS] Hex conversion" << std::endl;
}

void test_constant_time_equals() {
    assert(vaani::crypto::constant_time_equals("hello", "hello"));
    assert(!vaani::crypto::constant_time_equals("hello", "world"));
    assert(!vaani::crypto::constant_time_equals("hello", "hell"));
    std::cout << "[PASS] Constant time comparison" << std::endl;
}

void test_ed25519_verification() {
    // Known RFC 8032 test vector 1
    // Public key: d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a
    // Message: "" (empty)
    // Signature: e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b

    std::vector<uint8_t> pubkey, sig, msg;
    vaani::crypto::hex_to_bytes("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", pubkey);
    vaani::crypto::hex_to_bytes("e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b", sig);

    assert(vaani::crypto::ed25519_verify(msg, sig, pubkey));

    // Tamper with signature
    sig[0] ^= 0xFF;
    assert(!vaani::crypto::ed25519_verify(msg, sig, pubkey));

    std::cout << "[PASS] Ed25519 signature verification & tamper rejection" << std::endl;
}

int main() {
    test_sha256_known_vectors();
    test_hex_conversion();
    test_constant_time_equals();
    test_ed25519_verification();
    return 0;
}
