// Archivo: tests/test_crypto.cpp
// Test de Criptografía (ECDH P-256, AES-128-GCM, HKDF, SecureRandom)
#include "PortalCore/Common.h"
#include "PortalCore/Crypto/ECDHKeyExchange.h"
#include "PortalCore/Crypto/RPCrypt.h"
#include "PortalCore/Crypto/SecureRandom.h"

#include <iostream>
#include <cstdio>
#include <cassert>
#include <vector>

void test_secure_random() {
    printf("[TEST] SecureRandom... "); fflush(stdout);
    auto res1 = portal::crypto::SecureRandom::random_bytes(32);
    auto res2 = portal::crypto::SecureRandom::random_bytes(32);

    assert(res1.has_value());
    assert(res2.has_value());
    auto bytes1 = res1.value();
    auto bytes2 = res2.value();

    assert(bytes1.size() == 32);
    assert(bytes2.size() == 32);
    assert(bytes1 != bytes2);

    auto u32_1_res = portal::crypto::SecureRandom::random_uint32();
    auto u32_2_res = portal::crypto::SecureRandom::random_uint32();
    assert(u32_1_res.has_value());
    assert(u32_2_res.has_value());
    uint32_t u32_1 = u32_1_res.value();
    uint32_t u32_2 = u32_2_res.value();
    assert(u32_1 != u32_2 || u32_1 != 0);

    printf("PASSED\n"); fflush(stdout);
}

void test_ecdh_key_exchange() {
    printf("[TEST] ECDH P-256 Key Exchange... "); fflush(stdout);
    
    portal::crypto::ECDHKeyExchange alice;
    auto alice_init = alice.generate_keypair();
    assert(alice_init.has_value());
    auto alice_pub_res = alice.get_public_key();
    assert(alice_pub_res.has_value());
    auto alice_pub = alice_pub_res.value();
    assert(!alice_pub.empty());

    portal::crypto::ECDHKeyExchange bob;
    auto bob_init = bob.generate_keypair();
    assert(bob_init.has_value());
    auto bob_pub_res = bob.get_public_key();
    assert(bob_pub_res.has_value());
    auto bob_pub = bob_pub_res.value();
    assert(!bob_pub.empty());

    auto alice_secret = alice.compute_shared_secret(bob_pub);
    assert(alice_secret.has_value());

    auto bob_secret = bob.compute_shared_secret(alice_pub);
    assert(bob_secret.has_value());

    assert(alice_secret.value() == bob_secret.value());
    assert(alice_secret.value().size() == 32);

    std::vector<uint8_t> salt = {0x01, 0x02, 0x03, 0x04};
    std::string info_str = "PS5-RemotePlay-Session";
    std::vector<uint8_t> info(info_str.begin(), info_str.end());

    auto alice_session_key = portal::crypto::ECDHKeyExchange::derive_session_keys(alice_secret.value(), salt, info, 16);
    auto bob_session_key = portal::crypto::ECDHKeyExchange::derive_session_keys(bob_secret.value(), salt, info, 16);

    assert(alice_session_key.has_value());
    assert(bob_session_key.has_value());
    assert(alice_session_key.value() == bob_session_key.value());
    assert(alice_session_key.value().size() == 16);

    printf("PASSED\n"); fflush(stdout);
}

void test_aes_gcm_rpcrypt() {
    printf("[TEST] RPCrypt (AES-128-GCM)... "); fflush(stdout);

    portal::crypto::RPCrypt crypt;
    std::vector<uint8_t> key(16, 0x5A);
    std::vector<uint8_t> iv(16, 0x1F);
    auto set_res = crypt.set_keys(key, iv);
    assert(set_res.has_value());

    std::string plaintext_str = "PlayStation-Portal-Takion-Packet-Payload-1234567890";
    std::vector<uint8_t> plaintext(plaintext_str.begin(), plaintext_str.end());
    std::vector<uint8_t> aad = {0x01, 0x02, 0x03};

    // Cifrado GCM
    auto enc_res = crypt.encrypt_packet_gcm(plaintext, aad);
    assert(enc_res.has_value());
    auto out = enc_res.value();
    assert(out.size() == plaintext.size() + 16); // Ciphertext + 16-byte tag

    // Separar ciphertext y tag
    std::vector<uint8_t> ciphertext(out.begin(), out.end() - 16);
    std::vector<uint8_t> tag(out.end() - 16, out.end());

    // Descifrado GCM
    crypt.set_keys(key, iv);
    auto dec_res = crypt.decrypt_packet_gcm(ciphertext, tag, aad);
    if (!dec_res.has_value()) {
        printf("FAILED! dec_res error: %s\n", dec_res.error().message.c_str());
        fflush(stdout);
        assert(false);
    }
    assert(dec_res.value() == plaintext);

    // Detección de manipulación (Tag alterado)
    tag[0] ^= 0xFF;
    crypt.set_keys(key, iv);
    auto tampered_res = crypt.decrypt_packet_gcm(ciphertext, tag, aad);
    assert(!tampered_res.has_value());

    printf("PASSED\n"); fflush(stdout);
}

void test_log_masking() {
    printf("[TEST] Testing Log Obfuscation (mask_secret)... ");
    fflush(stdout);

    // Short secret (<= 8 chars) -> "****"
    assert(portal::mask_secret("1234") == "****");
    assert(portal::mask_secret("12345678") == "****");

    // Normal secret (> 8 chars) -> first 4 + "****" + last 4
    assert(portal::mask_secret("123456789") == "1234****6789");
    assert(portal::mask_secret("0123456789abcdef") == "0123****cdef");
    assert(portal::mask_secret("AA:BB:CC:DD:EE:FF") == "AA:B****E:FF");
    assert(portal::mask_secret("AABBCCDDEEFF") == "AABB****EEFF");

    // Vector of bytes (e.g. 16-byte rp_key = 32 hex chars)
    portal::ByteBuffer sample_key = {0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0xab, 0xcd, 0xef, 0x99};
    std::string masked_hex = portal::mask_secret(sample_key);
    assert(masked_hex == "1234****ef99");

    printf("PASSED\n"); fflush(stdout);
}

int main() {
    printf("========================================\n");
    printf("  Ludelo Unit Tests: Cryptography       \n");
    printf("========================================\n");
    fflush(stdout);

    try {
        test_secure_random();
        test_ecdh_key_exchange();
        test_aes_gcm_rpcrypt();
        test_log_masking();
        printf("\n>>> ALL CRYPTO TESTS PASSED SUCCESSFULLY! <<<\n");
        fflush(stdout);
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "\n[FAIL] Exception: %s\n", e.what());
        fflush(stderr);
        return 1;
    }
}
