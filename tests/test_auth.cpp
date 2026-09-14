// Archivo: tests/test_auth.cpp
// Test de Autenticación PSN (JWT decoding, AccountManager y Keychain DPAPI)
#include "PortalCore/Common.h"
#include "PortalCore/Auth/PSNAuth.h"
#include "PortalCore/Auth/Keychain.h"
#include "PortalCore/Auth/WebView2Auth.h"

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

void test_mixed_profile_json_parsing() {
    std::cout << "[TEST] PSN Mixed Profile JSON Parsing (Type-Safe)... ";

    // Mixed JSON matching Sony's actual response:
    // - onlineId: string
    // - plus: integer 0 (not string or boolean)
    // - accountId: uint64 number
    // - avatarUrls: array of objects with avatarUrl
    std::string sony_json = R"({
        "profile": {
            "onlineId": "player_one",
            "accountId": 123456789012345678,
            "plus": 0,
            "avatarUrls": [
                {
                    "size": "m",
                    "avatarUrl": "https://static-resource.np.community.playstation.net/avatar_m/user.png"
                }
            ]
        }
    })";

    auto prof = portal::auth::PSNAuth::parse_profile_json(sony_json);
    assert(prof.online_id == "player_one");
    assert(prof.account_id == 123456789012345678ULL);
    assert(!prof.account_id_b64.empty());
    assert(prof.plus_status == "none");
    assert(prof.avatar_url == "https://static-resource.np.community.playstation.net/avatar_m/user.png");

    // Another variant: plus=1 (int), string accountId, direct avatarUrl
    std::string sony_json2 = R"({
        "profile": {
            "onlineId": "player_two",
            "accountId": "987654321098765432",
            "plus": 1,
            "avatarUrl": "https://example.com/direct_avatar.png"
        }
    })";
    auto prof2 = portal::auth::PSNAuth::parse_profile_json(sony_json2);
    assert(prof2.online_id == "player_two");
    assert(prof2.account_id == 987654321098765432ULL);
    assert(!prof2.account_id_b64.empty());
    assert(prof2.plus_status == "active");
    assert(prof2.avatar_url == "https://example.com/direct_avatar.png");

    // Flat root JSON format (no "profile" wrapper):
    std::string flat_json = R"({
        "onlineId": "player_three",
        "accountId": "1122334455667788",
        "plus": 1,
        "avatarUrls": [
            { "avatarUrl": "https://example.com/flat_avatar.png" }
        ]
    })";
    auto prof_flat = portal::auth::PSNAuth::parse_profile_json(flat_json);
    assert(prof_flat.online_id == "player_three");
    assert(prof_flat.account_id == 1122334455667788ULL);
    assert(!prof_flat.account_id_b64.empty());
    assert(prof_flat.plus_status == "active");
    assert(prof_flat.avatar_url == "https://example.com/flat_avatar.png");

    // Robustness test: completely malformed or missing fields should NOT crash
    std::string malformed = "{ \"profile\": \"not_an_object\" }";
    auto prof3 = portal::auth::PSNAuth::parse_profile_json(malformed, 55555ULL, "b64test==");
    assert(prof3.account_id == 55555ULL);
    assert(prof3.account_id_b64 == "b64test==");

    std::cout << "PASSED\n";
}

void test_classify_auth_url() {
    std::cout << "[TEST] PSN Auth URL Classification (classify_auth_url)... ";

    // 1. signin con error=login_required => CONTINUE
    std::string signin_url = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize"
                             "?service_entity=urn:service-entity:psn&response_type=code"
                             "&client_id=ba495a24-818c-472b-b12d-ff231c1b5745"
                             "&redirect_uri=https%3A%2F%2Fremoteplay.dl.playstation.net%2Fremoteplay%2Fredirect"
                             "&error=login_required&no_captcha=true";
    std::string extracted_code;
    std::string extracted_error;
    auto res1 = portal::auth::classify_auth_url(signin_url, &extracted_code, &extracted_error);
    assert(res1 == portal::auth::AuthUrlClassification::Continue);
    assert(extracted_code.empty());
    assert(extracted_error.empty());

    // 2. redirect con ?code= => SUCCESS, extrae code
    std::string redirect_success_url = "https://remoteplay.dl.playstation.net/remoteplay/redirect?code=v1.mock_auth_code_98765&state=xyz";
    auto res2 = portal::auth::classify_auth_url(redirect_success_url, &extracted_code, &extracted_error);
    assert(res2 == portal::auth::AuthUrlClassification::Success);
    assert(extracted_code == "v1.mock_auth_code_98765");

    // 3. redirect con ?error=... => FAIL real
    std::string redirect_fail_url = "https://remoteplay.dl.playstation.net/remoteplay/redirect?error=access_denied&error_description=User+rejected+login";
    auto res3 = portal::auth::classify_auth_url(redirect_fail_url, &extracted_code, &extracted_error);
    assert(res3 == portal::auth::AuthUrlClassification::FatalError);
    assert(extracted_error == "access_denied");

    // 4. body/title con "Something went wrong" => FAIL real
    std::string dom_fail_body = "<html><head><title>Error</title></head><body>Something went wrong. Please try again.</body></html>";
    auto res4 = portal::auth::classify_auth_url(dom_fail_body, &extracted_code, &extracted_error);
    assert(res4 == portal::auth::AuthUrlClassification::FatalError);
    assert(portal::auth::classify_dom_content(dom_fail_body) == true);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: Auth & Keychain    \n";
    std::cout << "========================================\n";

    try {
        test_jwt_account_id_decoding();
        test_keychain_dpapi();
        test_mixed_profile_json_parsing();
        test_classify_auth_url();
        std::cout << "\n>>> ALL AUTH TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
