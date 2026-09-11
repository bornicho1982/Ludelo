// Archivo: tests/test_auth.cpp
// Test de Autenticación PSN (JWT decoding, AccountManager y Keychain DPAPI)
#include "PortalCore/Common.h"
#include "PortalCore/Auth/PSNAuth.h"
#include "PortalCore/Auth/Keychain.h"

#include <iostream>
#include <cassert>
#include <string>

// Generador de mock JWT para pruebas: header.payload.signature
std::string create_mock_jwt(const std::string& payload_json) {
    // Header mock: {"alg":"HS256","typ":"JWT"} -> eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9
    std::string header = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    
    // Convertir payload a base64url simplificado para prueba
    // Para la prueba usamos un payload fijo precalculado:
    // {"sub":"1234567890123456","name":"PlayStationUser"} -> eyJzdWIiOiIxMjM0NTY3ODkwMTIzNDU2IiwibmFtZSI6IlBsYXlTdGF0aW9uVXNlciJ9
    return header + ".eyJzdWIiOiIxMjM0NTY3ODkwMTIzNDU2IiwibmFtZSI6IlBsYXlTdGF0aW9uVXNlciJ9.mock_signature_12345";
}

void test_jwt_account_id_decoding() {
    std::cout << "[TEST] PSN JWT Account ID Decoding... ";

    std::string mock_token = create_mock_jwt("");
    auto res = portal::auth::PSNAuth::decode_account_id_from_jwt(mock_token);

    assert(res.has_value());
    assert(res.value() == 1234567890123456ULL);

    // Test de token malformado (sin puntos)
    auto bad_token = portal::auth::PSNAuth::decode_account_id_from_jwt("invalid_token_without_dots");
    assert(!bad_token.has_value());

    std::cout << "PASSED (Decoded Account ID: " << res.value() << ")\n";
}

void test_keychain_dpapi() {
    std::cout << "[TEST] Windows DPAPI Keychain Storage... ";

    portal::auth::Keychain keychain;
    std::string test_secret = "TEST_CREDENTIAL_NOT_REAL";
    portal::ByteBuffer secret_bytes(test_secret.begin(), test_secret.end());

    // Cifrado DPAPI
    auto enc_res = keychain.encrypt(secret_bytes);
    assert(enc_res.has_value());
    assert(enc_res.value() != secret_bytes); // Ciphertext debe diferir del plaintext

    // Descifrado DPAPI
    auto dec_res = keychain.decrypt(enc_res.value());
    assert(dec_res.has_value());
    assert(dec_res.value() == secret_bytes);

    std::string recovered(dec_res.value().begin(), dec_res.value().end());
    assert(recovered == test_secret);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: Auth & Keychain    \n";
    std::cout << "========================================\n";

    try {
        test_jwt_account_id_decoding();
        test_keychain_dpapi();
        std::cout << "\n>>> ALL AUTH TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
