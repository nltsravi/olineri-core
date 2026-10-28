#include "olineri/internal/crypto.hpp"
#include <iostream>
#include <cassert>

void test_sha256_known_vectors() {
    // RFC 6234 test vector 1: "" (empty string)
    assert(olineri::crypto::sha256_string("") == 
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // RFC 6234 test vector 2: "abc"
    assert(olineri::crypto::sha256_string("abc") == 
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // RFC 6234 test vector 3: "The quick brown fox jumps over the lazy dog"
    assert(olineri::crypto::sha256_string("The quick brown fox jumps over the lazy dog") == 
           "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");

    std::cout << "[PASS] SHA-256 standard vectors" << std::endl;
}

void test_hex_conversion() {
    std::string hex = "0123456789abcdef";
    std::vector<uint8_t> bytes;
    assert(olineri::crypto::hex_to_bytes(hex, bytes));
    assert(bytes.size() == 8);
    assert(olineri::crypto::bytes_to_hex(bytes.data(), bytes.size()) == hex);

    // Invalid hex length
    assert(!olineri::crypto::hex_to_bytes("abc", bytes));
    // Invalid characters
    assert(!olineri::crypto::hex_to_bytes("01zz", bytes));

    std::cout << "[PASS] Hex conversion" << std::endl;
}

void test_constant_time_equals() {
    assert(olineri::crypto::constant_time_equals("hello", "hello"));
    assert(!olineri::crypto::constant_time_equals("hello", "world"));
    assert(!olineri::crypto::constant_time_equals("hello", "hell"));
    std::cout << "[PASS] Constant time comparison" << std::endl;
}

void test_ed25519_verification() {
    // Known RFC 8032 test vector 1
    // Public key: d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a
    std::string pubkey_hex = "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a";
    // Signature: e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b
    std::string sig_hex = "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b";
    // Message: ""
    std::vector<uint8_t> msg;
    std::vector<uint8_t> pub;
    std::vector<uint8_t> sig;

    assert(olineri::crypto::hex_to_bytes(pubkey_hex, pub));
    assert(olineri::crypto::hex_to_bytes(sig_hex, sig));

    bool valid = olineri::crypto::ed25519_verify(msg, sig, pub);
    assert(valid);

    // Tampered signature should fail
    sig[0] ^= 0x01;
    assert(!olineri::crypto::ed25519_verify(msg, sig, pub));

    std::cout << "[PASS] Ed25519 signature verification" << std::endl;
}

int main() {
    std::cout << "=== Running Olineri Cryptographic Tests ===" << std::endl;
    test_sha256_known_vectors();
    test_hex_conversion();
    test_constant_time_equals();
    test_ed25519_verification();
    std::cout << "All crypto tests passed successfully!" << std::endl;
    return 0;
}
