// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "munit.h"
#include <string>
#include <vector>
#include <sstream>
#include <map>

// Helper to simulate fallback parsing of INI lines with size=0 corruption
static int simulate_fallback_host_recovery(const std::string& ini_content, std::string& out_nickname) {
    std::istringstream stream(ini_content);
    std::string line;
    bool in_group = false;
    int max_found_index = 0;
    std::map<std::string, std::string> kv;

    while (std::getline(stream, line)) {
        // Strip carriage return if present
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "[registered_hosts]") {
            in_group = true;
            continue;
        } else if (!line.empty() && line.front() == '[') {
            in_group = false;
        }

        if (in_group && !line.empty()) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                kv[key] = val;

                // Check index prefix (e.g., "1\\server_nickname")
                size_t backslash = key.find('\\');
                if (backslash == std::string::npos) backslash = key.find('/');
                if (backslash != std::string::npos) {
                    std::string idx_str = key.substr(0, backslash);
                    try {
                        int idx = std::stoi(idx_str);
                        if (idx > max_found_index) max_found_index = idx;
                    } catch (...) {}
                }
            }
        }
    }

    if (max_found_index > 0) {
        std::string nick_key1 = std::to_string(max_found_index) + "\\server_nickname";
        std::string nick_key2 = std::to_string(max_found_index) + "/server_nickname";
        if (kv.count(nick_key1)) out_nickname = kv[nick_key1];
        else if (kv.count(nick_key2)) out_nickname = kv[nick_key2];
    }
    return max_found_index;
}

static MunitResult test_corrupted_size_zero_recovery(const MunitParameter params[], void* data) {
    std::string corrupted_ini =
        "[General]\n"
        "version=2\n"
        "[registered_hosts]\n"
        "1\\target=1000100\n"
        "1\\ap_name=PS5\n"
        "1\\server_nickname=Bornicho1982 PS5\n"
        "1\\last_host_ip=192.168.18.21\n"
        "size=0\n"
        "[settings]\n"
        "psn_account_id=\"2IFPSJ3ALmE=\"\n";

    std::string nickname;
    int recovered_count = simulate_fallback_host_recovery(corrupted_ini, nickname);
    munit_assert_int(recovered_count, ==, 1);
    munit_assert_string_equal(nickname.c_str(), "Bornicho1982 PS5");
    return MUNIT_OK;
}

static std::string obfuscate_account_id(const std::string& raw) {
    std::string clean;
    for (char c : raw) {
        if (c != '=' && c != '"') clean += c;
    }
    if (clean.length() >= 8) {
        return clean.substr(0, 4) + "••••" + clean.substr(clean.length() - 4);
    }
    return clean;
}

static MunitResult test_account_id_obfuscation(const MunitParameter params[], void* data) {
    std::string raw1 = "2IFPSJ3ALmE=";
    std::string obf1 = obfuscate_account_id(raw1);
    munit_assert_string_equal(obf1.c_str(), "2IFP••••ALmE");

    std::string raw2 = "\"ZFPS3JALmE=\"";
    std::string obf2 = obfuscate_account_id(raw2);
    munit_assert_string_equal(obf2.c_str(), "ZFPS••••ALmE");
    return MUNIT_OK;
}

extern "C" {
    MunitTest registered_hosts_tests[] = {
        { (char*)"/corrupted_size_zero_recovery", test_corrupted_size_zero_recovery, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { (char*)"/account_id_obfuscation", test_account_id_obfuscation, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };
}
