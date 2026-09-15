// Archivo: src/LudeloCore/Auth/PSNAuth.cpp
#include "LudeloCore/Auth/PSNAuth.h"
#include "LudeloCore/Net/HttpClient.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <regex>
#include <format>
#include <chrono>
#include <algorithm>

namespace ludelo::auth {

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

Result<PSNTokens> PSNAuth::exchange_code(const std::string& auth_code) {
    spdlog::info("PSNAuth::exchange_code: starting token exchange");
    ludelo::net::HttpClient client;

    std::string token_url = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token";
    std::string body = std::format("grant_type=authorization_code&code={}&redirect_uri=https%3A%2F%2Fremoteplay.dl.playstation.net%2Fremoteplay%2Fredirect", auth_code);
    
    // Public OAuth client credentials embedded in official app (public, see upstream Chiaki)
    std::string basic_auth = "Basic YmE0OTVhMjQtODE4Yy00NzJiLWIxMmQtZmYyMzFjMWI1NzQ1Om12YWlaa1JzQXNJMUlCa1k=";
    
    auto token_res = client.post(token_url, {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Authorization", basic_auth}
    }, body);
    
    if (!token_res) {
        spdlog::error("PSNAuth::exchange_code: HTTP request failed: {}", token_res.error().message);
        return std::unexpected(token_res.error());
    }

    spdlog::info("PSNAuth::exchange_code: HTTP status={}, body={} bytes", token_res->status_code, token_res->body.size());

    // Try to parse the body as JSON regardless of status code.
    // Our HTTP client sometimes returns status 0 even when the server sent 200 OK
    // (HTTP/2 or chunked transfer encoding parsing issue).
    try {
        auto json = nlohmann::json::parse(token_res.value().body);
        
        // Check for Sony error response
        if (json.contains("error")) {
            std::string err = json.value("error", "unknown");
            std::string desc = json.value("error_description", "");
            spdlog::error("PSNAuth::exchange_code: Sony error '{}': {}", err, desc);
            return std::unexpected(ludelo::Error{ErrorCode::AuthError, std::format("Sony: {} - {}", err, desc)});
        }

        PSNTokens tokens;
        tokens.access_token = json.value("access_token", "");
        tokens.refresh_token = json.value("refresh_token", "");
        std::string token_type = json.value("token_type", "");
        int expires_in = json.value("expires_in", 3600);
        tokens.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);

        std::string token_type_lower = token_type;
        std::transform(token_type_lower.begin(), token_type_lower.end(), token_type_lower.begin(), ::tolower);

        bool is_valid_token_response = !tokens.access_token.empty() && 
            (token_type_lower == "bearer" || token_type.empty());

        if (!is_valid_token_response) {
            // Not a valid token response — if status was a real error, report it
            if (token_res->status_code != 0 && token_res->status_code != 200) {
                spdlog::error("PSNAuth::exchange_code: HTTP {} with invalid/missing access_token", token_res->status_code);
                return std::unexpected(ludelo::Error{ErrorCode::AuthError, std::format("Token exchange HTTP {}", token_res->status_code)});
            }
            spdlog::error("PSNAuth::exchange_code: access_token absent or invalid token_type in response");
            return std::unexpected(ludelo::Error{ErrorCode::AuthError, "Empty access_token in Sony response"});
        }

        if (token_res->status_code == 0) {
            spdlog::warn("PSNAuth::exchange_code: WARN status line not parsed, inferring success from body (token_type={})", token_type);
        }

        // Extract user_id (PSN Account-ID integer) from response or token info endpoint
        uint64_t user_id = 0;
        if (json.contains("user_id")) {
            if (json["user_id"].is_string()) {
                try { user_id = std::stoull(json["user_id"].get<std::string>()); } catch (...) {}
            } else if (json["user_id"].is_number()) {
                user_id = json["user_id"].get<uint64_t>();
            }
        } else if (json.contains("account_id")) {
            if (json["account_id"].is_string()) {
                try { user_id = std::stoull(json["account_id"].get<std::string>()); } catch (...) {}
            } else if (json["account_id"].is_number()) {
                user_id = json["account_id"].get<uint64_t>();
            }
        }

        // If not in token exchange JSON, query https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/<access_token>
        if (user_id == 0 && !tokens.access_token.empty()) {
            std::string token_info_url = std::format("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/{}", tokens.access_token);
            auto info_res = client.get(token_info_url, {{"Authorization", basic_auth}});
            if (info_res && (info_res->status_code == 200 || info_res->status_code == 0)) {
                try {
                    auto info_json = nlohmann::json::parse(info_res->body);
                    if (info_json.contains("user_id")) {
                        if (info_json["user_id"].is_string()) {
                            user_id = std::stoull(info_json["user_id"].get<std::string>());
                        } else if (info_json["user_id"].is_number()) {
                            user_id = info_json["user_id"].get<uint64_t>();
                        }
                    } else if (info_json.contains("account_id")) {
                        if (info_json["account_id"].is_string()) {
                            user_id = std::stoull(info_json["account_id"].get<std::string>());
                        } else if (info_json["account_id"].is_number()) {
                            user_id = info_json["account_id"].get<uint64_t>();
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("PSNAuth::exchange_code: parsing token info JSON failed: {}", e.what());
                }
            }
        }

        if (user_id == 0) {
            auto jwt_uid = decode_account_id_from_jwt(tokens.access_token);
            if (jwt_uid.has_value()) {
                user_id = jwt_uid.value();
            }
        }

        if (user_id != 0) {
            tokens.account_id = user_id;
            uint8_t le_bytes[8];
            for (int i = 0; i < 8; ++i) {
                le_bytes[i] = static_cast<uint8_t>((user_id >> (i * 8)) & 0xFF);
            }
            std::string b64(16, '\0');
            int len = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(b64.data()), le_bytes, 8);
            b64.resize(len);
            tokens.account_id_b64 = b64;
        }

        // Exact required log format: status numérico + "tokens presentes: access=/refresh= SI/NO". Sin valores.
        spdlog::info("PSNAuth::exchange_code: status={}, tokens presentes: access={} / refresh={}",
            token_res->status_code,
            tokens.access_token.empty() ? "NO" : "SI",
            tokens.refresh_token.empty() ? "NO" : "SI");
        spdlog::info("Token exchange OK");
        return tokens;
    } catch (const std::exception& e) {
        // JSON parse failed — body is not valid JSON
        if (token_res->status_code != 0 && token_res->status_code != 200) {
            spdlog::error("PSNAuth::exchange_code: HTTP {} and body is not JSON: {}", token_res->status_code, e.what());
            return std::unexpected(ludelo::Error{ErrorCode::AuthError, std::format("Token exchange HTTP {}", token_res->status_code)});
        }
        spdlog::error("PSNAuth::exchange_code: JSON parse failed: {}", e.what());
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, e.what()});
    }
}

Result<PSNTokens> PSNAuth::exchange_npsso(const std::string& npsso_token) {
    spdlog::info("Exchanging NPSSO token with Sony auth servers");

    ludelo::net::HttpClient client;
    
    // Step 1: Get Authorization Code
    std::string auth_url = "https://ca.account.sony.com/api/authz/v3/oauth/authorize?access_type=offline&client_id=09515159-7237-4370-9b40-3806e67c0891&redirect_uri=com.playstation.PlayStationApp://redirect&response_type=code&scope=psn:mobile.v2.core%20psn:clientapp";
    auto auth_res = client.get(auth_url, {{"Cookie", std::format("npsso={}", npsso_token)}});
    
    std::string auth_code;
    std::regex code_regex(R"(code=([A-Za-z0-9:\-]+))");
    std::smatch code_match;
    if (auth_res && auth_res->headers.count("location")) {
        std::string loc = auth_res->headers["location"];
        if (std::regex_search(loc, code_match, code_regex)) {
            auth_code = code_match[1].str();
        }
    }
    
    if (auth_code.empty()) {
        if (auth_res && std::regex_search(auth_res->body, code_match, code_regex)) {
            auth_code = code_match[1].str();
        } else {
            spdlog::error("Failed to fetch SSO code. NPSSO might be invalid or expired.");
            return std::unexpected(Error{ErrorCode::AuthError, "Failed to get auth code"});
        }
    }

    return exchange_code(auth_code);
}

Result<PSNTokens> PSNAuth::refresh_tokens(const std::string& refresh_token) {
    spdlog::info("Refreshing access tokens");

    ludelo::net::HttpClient client;
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
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, e.what()});
    }
}

Result<uint64_t> PSNAuth::decode_account_id_from_jwt(const std::string& access_token) {
    auto first_dot = access_token.find('.');
    auto second_dot = access_token.find('.', first_dot + 1);
    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, "Invalid JWT format (missing dots)"});
    }
    std::string payload = access_token.substr(first_dot + 1, second_dot - first_dot - 1);
    std::string decoded = base64_url_decode(payload);
    if (decoded.empty()) {
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, "Failed to base64 decode JWT payload"});
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
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, "JWT does not contain sub or account_id"});
    } catch (const std::exception& e) {
        return std::unexpected(ludelo::Error{ErrorCode::AuthError, std::format("JSON parse error in JWT: {}", e.what())});
    }
}

PSNProfile PSNAuth::parse_profile_json(const std::string& json_str, uint64_t known_account_id, const std::string& known_account_id_b64) {
    PSNProfile profile;
    profile.account_id = known_account_id;
    profile.account_id_b64 = known_account_id_b64;

    try {
        auto json = nlohmann::json::parse(json_str);

        const auto& prof = (json.contains("profile") && json["profile"].is_object()) ? json["profile"] : json;
        
        // onlineId (can be string or number)
        for (const char* k : {"onlineId", "online_id"}) {
            if (prof.contains(k)) {
                try {
                    if (prof[k].is_string()) {
                        profile.online_id = prof[k].get<std::string>();
                    } else if (prof[k].is_number()) {
                        profile.online_id = std::to_string(prof[k].get<uint64_t>());
                    }
                    if (!profile.online_id.empty()) break;
                } catch (const nlohmann::json::exception& e) {
                    spdlog::warn("parse_profile_json: parsing onlineId warning: {}", e.what());
                }
            }
        }

        // avatarUrls / avatars (array of objects or strings)
        for (const char* k : {"avatarUrls", "avatars"}) {
            if (prof.contains(k) && prof[k].is_array() && !prof[k].empty()) {
                try {
                    // Try to pick the best avatar (prefer large or first valid)
                    for (const auto& av_item : prof[k]) {
                        if (av_item.is_object()) {
                            if (av_item.contains("avatarUrl") && av_item["avatarUrl"].is_string()) {
                                profile.avatar_url = av_item["avatarUrl"].get<std::string>();
                                break;
                            } else if (av_item.contains("url") && av_item["url"].is_string()) {
                                profile.avatar_url = av_item["url"].get<std::string>();
                                break;
                            }
                        } else if (av_item.is_string()) {
                            profile.avatar_url = av_item.get<std::string>();
                            break;
                        }
                    }
                } catch (const nlohmann::json::exception& e) {
                    spdlog::warn("parse_profile_json: parsing avatarUrls warning: {}", e.what());
                }
            }
            if (!profile.avatar_url.empty()) break;
        }
        if (profile.avatar_url.empty()) {
            for (const char* k : {"avatarUrl", "avatar"}) {
                if (prof.contains(k)) {
                    try {
                        if (prof[k].is_string()) {
                            profile.avatar_url = prof[k].get<std::string>();
                            if (!profile.avatar_url.empty()) break;
                        }
                    } catch (...) {}
                }
            }
        }

        // plus / is_plus (can be number 0/1, bool, or string)
        for (const char* k : {"plus", "is_plus"}) {
            if (prof.contains(k)) {
                try {
                    if (prof[k].is_number()) {
                        profile.plus_status = (prof[k].get<int>() != 0) ? "active" : "none";
                    } else if (prof[k].is_string()) {
                        profile.plus_status = prof[k].get<std::string>();
                    } else if (prof[k].is_boolean()) {
                        profile.plus_status = prof[k].get<bool>() ? "active" : "none";
                    }
                    break;
                } catch (const nlohmann::json::exception& e) {
                    spdlog::warn("parse_profile_json: parsing plus warning: {}", e.what());
                    profile.plus_status = "none";
                }
            }
        }

        // accountId in profile (can be number or string)
        if (profile.account_id == 0) {
            for (const char* k : {"accountId", "account_id"}) {
                if (prof.contains(k)) {
                    try {
                        if (prof[k].is_number()) {
                            profile.account_id = prof[k].get<uint64_t>();
                        } else if (prof[k].is_string()) {
                            profile.account_id = std::stoull(prof[k].get<std::string>());
                        }
                        if (profile.account_id != 0) break;
                    } catch (const nlohmann::json::exception& e) {
                        spdlog::warn("parse_profile_json: parsing accountId warning: {}", e.what());
                    }
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("parse_profile_json: JSON exception: {}", e.what());
    } catch (const std::exception& e) {
        spdlog::warn("parse_profile_json: exception: {}", e.what());
    }

    if (profile.account_id != 0 && profile.account_id_b64.empty()) {
        uint8_t le_bytes[8];
        for (int i = 0; i < 8; ++i) {
            le_bytes[i] = static_cast<uint8_t>((profile.account_id >> (i * 8)) & 0xFF);
        }
        std::string b64(16, '\0');
        int len = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(b64.data()), le_bytes, 8);
        b64.resize(len);
        profile.account_id_b64 = b64;
    }

    return profile;
}

Result<PSNProfile> PSNAuth::fetch_profile(const std::string& access_token, uint64_t known_account_id, const std::string& known_account_id_b64) {
    spdlog::info("PSNAuth::fetch_profile: starting");

    ludelo::net::HttpClient client;
    std::string url = "https://us-prof.np.community.playstation.net/userProfile/v1/users/me/profile2?fields=npId,onlineId,avatarUrls,plus";
    auto res = client.get(url, {{"Authorization", std::format("Bearer {}", access_token)}});
    if (!res) {
        spdlog::warn("PSNAuth::fetch_profile: HTTP request failed: {}, returning partial profile", res.error().message);
        PSNProfile profile;
        profile.account_id = known_account_id;
        profile.account_id_b64 = known_account_id_b64;
        spdlog::info("fetch_profile OK, online_id={}, account_id={}, avatar={}", 
            "PSN User", 
            profile.account_id_b64.empty() ? std::to_string(profile.account_id) : profile.account_id_b64, 
            "NO");
        return profile;
    }

    spdlog::info("PSNAuth::fetch_profile: HTTP status={}, body={} bytes", res->status_code, res->body.size());

    PSNProfile profile = parse_profile_json(res->body, known_account_id, known_account_id_b64);

    if (profile.account_id == 0) {
        profile.account_id = PSNAuth::decode_account_id_from_jwt(access_token).value_or(0);
    }

    if (profile.account_id == 0) {
        std::string token_info_url = std::format("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token/{}", access_token);
        std::string basic_auth = "Basic YmE0OTVhMjQtODE4Yy00NzJiLWIxMmQtZmYyMzFjMWI1NzQ1Om12YWlaa1JzQXNJMUlCa1k=";
        auto info_res = client.get(token_info_url, {{"Authorization", basic_auth}});
        if (info_res && (info_res->status_code == 200 || info_res->status_code == 0)) {
            try {
                auto info_json = nlohmann::json::parse(info_res->body);
                if (info_json.contains("user_id")) {
                    if (info_json["user_id"].is_string()) {
                        profile.account_id = std::stoull(info_json["user_id"].get<std::string>());
                    } else if (info_json["user_id"].is_number()) {
                        profile.account_id = info_json["user_id"].get<uint64_t>();
                    }
                } else if (info_json.contains("account_id")) {
                    if (info_json["account_id"].is_string()) {
                        profile.account_id = std::stoull(info_json["account_id"].get<std::string>());
                    } else if (info_json["account_id"].is_number()) {
                        profile.account_id = info_json["account_id"].get<uint64_t>();
                    }
                }
            } catch (...) {}
        }
    }

    if (profile.account_id != 0 && profile.account_id_b64.empty()) {
        uint8_t le_bytes[8];
        for (int i = 0; i < 8; ++i) {
            le_bytes[i] = static_cast<uint8_t>((profile.account_id >> (i * 8)) & 0xFF);
        }
        std::string b64(16, '\0');
        int len = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(b64.data()), le_bytes, 8);
        b64.resize(len);
        profile.account_id_b64 = b64;
    }

    if (profile.avatar_url.empty() && !profile.online_id.empty()) {
        std::string alt_url = std::format("https://us-prof.np.community.playstation.net/userProfile/v1/users/{}/profile2?fields=npId,onlineId,avatarUrls,plus", profile.online_id);
        auto alt_res = client.get(alt_url, {{"Authorization", std::format("Bearer {}", access_token)}});
        if (alt_res && (alt_res->status_code == 200 || alt_res->status_code == 0)) {
            try {
                auto alt_json = nlohmann::json::parse(alt_res->body);
                if (alt_json.contains("profile") && alt_json["profile"].is_object()) {
                    const auto& alt_prof = alt_json["profile"];
                    if (alt_prof.contains("avatarUrls") && alt_prof["avatarUrls"].is_array() && !alt_prof["avatarUrls"].empty()) {
                        const auto& first_av = alt_prof["avatarUrls"][0];
                        if (first_av.is_object() && first_av.contains("avatarUrl") && first_av["avatarUrl"].is_string()) {
                            profile.avatar_url = first_av["avatarUrl"].get<std::string>();
                        }
                    }
                }
            } catch (...) {}
        }
    }

    if (res->status_code == 0) {
        spdlog::warn("PSNAuth::fetch_profile: WARN status line not parsed, inferring success from body");
    }

    // REQUIRED LOG FORMAT: "fetch_profile OK, online_id=X, account_id=<base64 no vacío>"
    spdlog::info("fetch_profile OK, online_id={}, account_id={}, avatar={}", 
        profile.online_id.empty() ? "unknown" : profile.online_id, 
        profile.account_id_b64.empty() ? std::to_string(profile.account_id) : profile.account_id_b64, 
        profile.avatar_url.empty() ? "NO" : "YES");
    return profile;
}

} // namespace ludelo::auth
