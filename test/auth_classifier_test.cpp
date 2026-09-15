
#include "munit.h"
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

extern "C" {
    MunitTest auth_classifier_tests[] = {
        { (char*)"/success", test_classify_success, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/login_required", test_classify_login_required, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/fatal_error", test_classify_fatal_error, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/dom_error", test_classify_dom, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };
}

