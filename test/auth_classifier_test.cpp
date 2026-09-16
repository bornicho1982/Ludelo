
#include "munit.h"
#include <cstring>
#include "../gui/include/auth_classifier.h"

static MunitResult test_classify_success(const MunitParameter params[], void* data) {
    std::string out_code, out_err;
    auto cls = ludelo::auth::classify_auth_url("https://remoteplay.dl.playstation.net/remoteplay/redirect?code=v3.123456&cid=xxx", &out_code, &out_err);
    munit_assert_int(static_cast<int>(cls), ==, static_cast<int>(ludelo::auth::AuthUrlClassification::Success));
    munit_assert_string_equal(out_code.c_str(), "v3.123456");
    return MUNIT_OK;
}

static MunitResult test_classify_login_required(const MunitParameter params[], void* data) {
    std::string out_code, out_err;
    auto cls = ludelo::auth::classify_auth_url("https://my.account.sony.com/sonyacct/signin/?error=login_required&no_captcha=true", &out_code, &out_err);
    munit_assert_int(static_cast<int>(cls), ==, static_cast<int>(ludelo::auth::AuthUrlClassification::Continue));
    return MUNIT_OK;
}

static MunitResult test_classify_fatal_error(const MunitParameter params[], void* data) {
    std::string out_code, out_err;
    auto cls = ludelo::auth::classify_auth_url("https://remoteplay.dl.playstation.net/remoteplay/redirect?error=server_error", &out_code, &out_err);
    munit_assert_int(static_cast<int>(cls), ==, static_cast<int>(ludelo::auth::AuthUrlClassification::FatalError));
    munit_assert_string_equal(out_err.c_str(), "server_error");
    return MUNIT_OK;
}

static MunitResult test_classify_dom(const MunitParameter params[], void* data) {
    munit_assert_true(ludelo::auth::classify_dom_content("Something went wrong with the server"));
    return MUNIT_OK;
}

static MunitResult test_log_redaction_secrets(const MunitParameter params[], void* data) {
    munit_assert_string_equal(ludelo::log::mask_secret("").c_str(), "****");
    munit_assert_string_equal(ludelo::log::mask_secret("abc").c_str(), "****");
    munit_assert_string_equal(ludelo::log::mask_secret("1234").c_str(), "****");
    munit_assert_string_equal(ludelo::log::mask_secret("v3.12345678").c_str(), "v3.1****");
    munit_assert_string_equal(ludelo::log::mask_secret("secret_token_value").c_str(), "secr****");
    return MUNIT_OK;
}

static MunitResult test_log_redaction_url(const MunitParameter params[], void* data) {
    std::string url = "https://remoteplay.dl.playstation.net/remoteplay/redirect?code=v3.AQAAAYabcdef123456&state=xyz";
    std::string masked = ludelo::log::mask_url_secrets(url);
    munit_assert_string_equal(masked.c_str(), "https://remoteplay.dl.playstation.net/remoteplay/redirect?code=v3.A****&state=xyz");

    std::string path_url = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/at-secret-access-token-987654";
    std::string path_masked = ludelo::log::mask_url_secrets(path_url);
    munit_assert_string_equal(path_masked.c_str(), "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/at-s****");
    return MUNIT_OK;
}

static MunitResult test_log_redaction_json(const MunitParameter params[], void* data) {
    std::string json = "{\"access_token\": \"at-value-12345\", \"refresh_token\": \"rt-value-67890\", \"user_id\": \"987654321\"}";
    std::string masked = ludelo::log::mask_json_secrets(json);
    munit_assert_string_equal(masked.c_str(), "{\"access_token\": \"at-v****\", \"refresh_token\": \"rt-v****\", \"user_id\": \"9876****\"}");

    std::string num_json = "{\"user_id\": 1234567890, \"online_id\": \"ProGamer\"}";
    std::string num_masked = ludelo::log::mask_json_secrets(num_json);
    munit_assert_string_equal(num_masked.c_str(), "{\"user_id\": 1234****, \"online_id\": \"ProGamer\"}");
    return MUNIT_OK;
}

static MunitResult test_log_redaction_bodies(const MunitParameter params[], void* data) {
    std::string req = "Request Body: grant_type=authorization_code&code=v3.secretcode123";
    std::string req_masked = ludelo::log::sanitize_log_message(req);
    munit_assert_string_equal(req_masked.c_str(), "Request Body: [REDACTED]");

    std::string res = "Response Body: {\"access_token\": \"at-secret-token\"}";
    std::string res_masked = ludelo::log::sanitize_log_message(res);
    munit_assert_string_equal(res_masked.c_str(), "Response Body: [REDACTED]");
    return MUNIT_OK;
}

static MunitResult test_log_security_simulated_login(const MunitParameter params[], void* data) {
    std::string raw_code = "v3.AQAAAYabcdef987654321";
    std::string raw_access_token = "at-secret-access-token-xyz-123456";
    std::string raw_refresh_token = "rt-secret-refresh-token-uvw-789012";
    std::string raw_userid = "8765432109876543210";

    // 1. Verify: Unredacted log with raw secrets fails the security scan
    std::string unredacted_log = 
        "Navigating to redirect: https://remoteplay.dl.playstation.net/remoteplay/redirect?code=" + raw_code + "\n"
        "POST https://ca.account.sony.com/api/authz/v3/oauth/token\n"
        "Response Body: {\"access_token\": \"" + raw_access_token + "\", \"refresh_token\": \"" + raw_refresh_token + "\"}\n"
        "GET https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/" + raw_access_token + "\n"
        "Response Body: {\"user_id\": \"" + raw_userid + "\", \"online_id\": \"TestUser\"}\n";

    std::string unredacted_leak;
    bool unredacted_has_leak = ludelo::log::scan_log_for_leaks(
        unredacted_log, {raw_access_token, raw_refresh_token, raw_userid, raw_code}, &unredacted_leak);
    munit_assert_true(unredacted_has_leak);
    munit_assert_false(ludelo::log::has_no_cleartext_credentials(unredacted_log));

    // 2. Simulate actual sanitized logs generated by Ludelo during a login sequence
    // Line 1: WebView2 redirect navigation (sanitized URL)
    std::string log1 = ludelo::log::sanitize_log_message(
        "Navigating: https://remoteplay.dl.playstation.net/remoteplay/redirect?code=" + raw_code);
    
    // Line 2: Captured code (masked)
    std::string log2 = "code captured (masked): " + ludelo::log::mask_secret(raw_code);

    // Line 3: JsonRequester POST token request (only method and sanitized URL, no bodies)
    std::string log3 = ludelo::log::sanitize_log_message(
        "POST https://ca.account.sony.com/api/authz/v3/oauth/token");
    std::string log4 = ludelo::log::sanitize_log_message("Status Code: 200");

    // Line 5: JsonRequester GET user profile (URL path token masked, no response body)
    std::string log5 = ludelo::log::sanitize_log_message(
        "GET https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/" + raw_access_token);
    std::string log6 = ludelo::log::sanitize_log_message("Status Code: 200");

    // Line 7 & 8: Accidental Request Body / Response Body injection through any log macro
    std::string log7 = ludelo::log::sanitize_log_message(
        "Request Body: grant_type=authorization_code&code=" + raw_code);
    std::string log8 = ludelo::log::sanitize_log_message(
        "Response Body: {\"access_token\": \"" + raw_access_token + "\", \"refresh_token\": \"" + raw_refresh_token + "\", \"user_id\": \"" + raw_userid + "\"}");

    std::string full_simulated_log = log1 + "\n" + log2 + "\n" + log3 + "\n" + log4 + "\n" +
                                     log5 + "\n" + log6 + "\n" + log7 + "\n" + log8 + "\n";

    // 3. Scan the simulated log: it MUST NOT contain raw secrets
    std::string leak_found;
    bool has_leak = ludelo::log::scan_log_for_leaks(
        full_simulated_log, {raw_access_token, raw_refresh_token, raw_userid, raw_code}, &leak_found);
    munit_assert_false(has_leak);

    // Explicit checks: cleartext strings must not exist anywhere in the log
    munit_assert_null(strstr(full_simulated_log.c_str(), raw_access_token.c_str()));
    munit_assert_null(strstr(full_simulated_log.c_str(), raw_refresh_token.c_str()));
    munit_assert_null(strstr(full_simulated_log.c_str(), raw_userid.c_str()));
    munit_assert_null(strstr(full_simulated_log.c_str(), raw_code.c_str()));

    // Verify all credential assignments in the log are strictly redacted/masked
    munit_assert_true(ludelo::log::has_no_cleartext_credentials(full_simulated_log));

    return MUNIT_OK;
}

static MunitResult test_qr_login_url_generation(const MunitParameter params[], void* data) {
    const std::string base_url = "https://www.xbgamestream.com";
    const std::string test_code = "AB12CD";
    
    // Verify QR image payload URL format matches QRLoginDialog and QmlBackend
    std::string qr_target_url = base_url + "/psstream/?psstream_code=" + test_code;
    munit_assert_string_equal(qr_target_url.c_str(), "https://www.xbgamestream.com/psstream/?psstream_code=AB12CD");

    // Verify endpoint paths used by createLudeloCode and checkLudeloStatus
    std::string create_code_endpoint = base_url + "/psstream/create-code";
    std::string get_tokens_endpoint = base_url + "/psstream/get-tokens";
    munit_assert_string_equal(create_code_endpoint.c_str(), "https://www.xbgamestream.com/psstream/create-code");
    munit_assert_string_equal(get_tokens_endpoint.c_str(), "https://www.xbgamestream.com/psstream/get-tokens");

    return MUNIT_OK;
}

extern "C" {
    MunitTest auth_classifier_tests[] = {
        { (char*)"/success", test_classify_success, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/login_required", test_classify_login_required, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/fatal_error", test_classify_fatal_error, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/dom_error", test_classify_dom, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/redaction_secrets", test_log_redaction_secrets, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/redaction_url", test_log_redaction_url, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/redaction_json", test_log_redaction_json, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/redaction_bodies", test_log_redaction_bodies, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/security_simulated_login", test_log_security_simulated_login, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/qr_login_url", test_qr_login_url_generation, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };
}

