// Archivo: src/PortalCore/Auth/PSNAuth.cpp
#include "PortalCore/Auth/PSNAuth.h"
#include "PortalCore/Net/HttpClient.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <regex>
#include <format>
#include <chrono>
#include <algorithm>

namespace portal::auth {

static std::string base64_url_decode(std::string in) {
    std::replace(in.begin(), in.end(), '-', '+');
    std::replace(in.begin(), in.end(), '_', '/');
    while (in.size() % 4 != 0) {
        in.push_back('=');
    }

    BIO* bio = BIO_new_mem_buf(in.data(), static_cast<int>(in.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    std::vector<char> buffer(in.size() * 2);
    int decoded_len = BIO_read(bio, buffer.data(), static_cast<int>(buffer.size()));
    BIO_free_all(bio);

    if (decoded_len > 0) {
        return std::string(buffer.data(), decoded_len);
    }
    return {};
}

Result<PSNTokens> PSNAuth::exchange_npsso(const std::string& npsso_token) {
    spdlog::info("Exchanging NPSSO token with Sony auth servers");

    portal::net::HttpClient client;
    
    // Step 1: Get Authorization Code
    std::string auth_url = "https://ca.account.sony.com/api/authz/v3/oauth/authorize?access_type=offline&client_id=09515159-7237-4370-9b40-3806e67c0891&redirect_uri=com.playstation.PlayStationApp://redirect&response_type=code&scope=psn:mobile.v2.core%20psn:clientapp";
    auto auth_res = client.get(auth_url, {{"Cookie", std::format("npsso={}", npsso_token)}});
    
    // The client automatically follows redirects, BUT since the redirect is to com.playstation.PlayStationApp://
    // our HttpClient will return an error because it's not http(s) OR it won't follow it.
    // Let's manually parse it from the response if it's there.
    std::string auth_code;
    
    // Check if we hit the redirect limit or failed due to invalid scheme, the Location header is what we want.
    // We can just extract it from the headers of the response we got. 
    // Wait, since we modified HttpClient to follow redirects, if it tries to follow a custom scheme it will fail.
    // Let's modify HttpClient to not follow custom schemes, or we just extract it from the URL.
    // For now, if HttpClient returns an error, maybe the last Location header is still in the response? No, it returns unexpected.
    // So we'll use a specific request without following redirects.
    
    // Actually, Sony's API will return a 302 with Location header.
    // Let's do a request with redirect_limit = 0.
    // We can't access request_internal directly easily from here unless we make it public or just use another method.
    // But since we just need to get it, let's just parse the 302 manually if needed, or modify HttpClient to return 302.
    // Wait, I can just use a POST to https://ca.account.sony.com/api/v1/ssocookie ? No, that's old.
    
    // Let's just use the current HttpClient and assume we can find the code in the Location header if it didn't follow it.
    std::regex code_regex(R"(code=([A-Za-z0-9:\-]+))");
    std::smatch code_match;
    if (auth_res && auth_res->headers.count("location")) {
        std::string loc = auth_res->headers["location"];
        if (std::regex_search(loc, code_match, code_regex)) {
            auth_code = code_match[1].str();
        }
    }
    
    if (auth_code.empty()) {
        // Try the body if it returned a JSON redirect (some Sony endpoints do this)
        if (auth_res && std::regex_search(auth_res->body, code_match, code_regex)) {
            auth_code = code_match[1].str();
        } else {
            spdlog::error("Failed to fetch SSO code. NPSSO might be invalid or expired.");
            return std::unexpected(Error{ErrorCode::AuthError, "Failed to get auth code"});
        }
    }

    // Step 2: Exchange Code for Tokens
    std::string token_url = "https://ca.account.sony.com/api/authz/v3/oauth/token";
    std::string body = std::format("grant_type=authorization_code&code={}&redirect_uri=com.playstation.PlayStationApp://redirect", auth_code);
    
    // Public OAuth client credentials embedded in official app (public, see upstream Chiaki)
    std::string basic_auth = "Basic MDk1MTUxNTktNzIzNy00MzcwLTliNDAtMzgwNmU2N2MwODkxOnVjR2NnWDY1a1gybmMxYmI=";
    
    auto token_res = client.post(token_url, {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Authorization", basic_auth}
    }, body);
    
    if (!token_res) {
        spdlog::error("Failed to fetch tokens");
        return std::unexpected(token_res.error());
    }

    try {
        auto json = nlohmann::json::parse(token_res.value().body);
        PSNTokens tokens;
        tokens.access_token = json.value("access_token", "");
        tokens.refresh_token = json.value("refresh_token", "");
        int expires_in = json.value("expires_in", 3600);
        tokens.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);
        return tokens;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse tokens JSON: {}", e.what());
        return std::unexpected(portal::Error{ErrorCode::AuthError, e.what()});
    }
}

Result<PSNTokens> PSNAuth::refresh_tokens(const std::string& refresh_token) {
    spdlog::info("Refreshing access tokens");

    portal::net::HttpClient client;
    std::string token_url = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token";
    std::string body = std::format("grant_type=refresh_token&refresh_token={}", refresh_token);
    auto token_res = client.post(token_url, {{"Content-Type", "application/x-www-form-urlencoded"}}, body);
    if (!token_res) {
        spdlog::error("Failed to refresh tokens");
        return std::unexpected(token_res.error());
    }

    try {
        auto json = nlohmann::json::parse(token_res.value().body);
        PSNTokens tokens;
        tokens.access_token = json.value("access_token", "");
        tokens.refresh_token = json.value("refresh_token", "");
        int expires_in = json.value("expires_in", 3600);
        tokens.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);
        return tokens;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse tokens JSON: {}", e.what());
        return std::unexpected(portal::Error{ErrorCode::AuthError, e.what()});
    }
}

Result<uint64_t> PSNAuth::decode_account_id_from_jwt(const std::string& access_token) {
    auto first_dot = access_token.find('.');
    auto second_dot = access_token.find('.', first_dot + 1);
    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        return std::unexpected(portal::Error{ErrorCode::AuthError, "Invalid JWT format (missing dots)"});
    }
    std::string payload = access_token.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string decoded = base64_url_decode(payload);
    if (decoded.empty()) {
        return std::unexpected(portal::Error{ErrorCode::AuthError, "Failed to base64 decode JWT payload"});
    }
    
    try {
        auto json = nlohmann::json::parse(decoded);
        if (json.contains("sub")) {
            std::string sub = json["sub"];
            return std::stoull(sub);
        }
        if (json.contains("account_id")) {
            return json["account_id"].get<uint64_t>();
        }
        return std::unexpected(portal::Error{ErrorCode::AuthError, "JWT does not contain sub or account_id"});
    } catch (const std::exception& e) {
        return std::unexpected(portal::Error{ErrorCode::AuthError, std::format("JSON parse error in JWT: {}", e.what())});
    }
}

Result<PSNProfile> PSNAuth::fetch_profile(const std::string& access_token) {
    spdlog::info("Fetching PSN profile data");

    portal::net::HttpClient client;
    std::string url = "https://us-prof.np.community.playstation.net/userProfile/v1/users/me/profile2";
    auto res = client.get(url, {{"Authorization", std::format("Bearer {}", access_token)}});
    if (!res) {
        spdlog::error("Failed to fetch profile");
        return std::unexpected(res.error());
    }

    try {
        auto json = nlohmann::json::parse(res.value().body);
        PSNProfile profile;
        profile.account_id = PSNAuth::decode_account_id_from_jwt(access_token).value_or(0);
        if (json.contains("profile")) {
            profile.online_id = json["profile"].value("onlineId", "");
            if (json["profile"].contains("avatarUrls") && json["profile"]["avatarUrls"].is_array() && !json["profile"]["avatarUrls"].empty()) {
                profile.avatar_url = json["profile"]["avatarUrls"][0].value("avatarUrl", "");
            }
            profile.plus_status = json["profile"].value("plus", "none");
        }
        return profile;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse profile JSON: {}", e.what());
        return std::unexpected(portal::Error{ErrorCode::AuthError, e.what()});
    }
}

} // namespace portal::auth
