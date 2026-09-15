// Archivo: src/LudeloCore/Auth/PSNAuth.h
#pragma once

#include "LudeloCore/Common.h"
#include <string>
#include <cstdint>
#include <chrono>

namespace ludelo::auth {

struct PSNTokens {
    std::string access_token;
    std::string refresh_token;
    std::chrono::system_clock::time_point expires_at;
    uint64_t account_id = 0;
    std::string account_id_b64;
};

struct PSNProfile {
    uint64_t account_id = 0;
    std::string account_id_b64;
    std::string online_id;
    std::string avatar_url;
    std::string plus_status;
};

class PSNAuth {
public:
    static Result<PSNTokens> exchange_code(const std::string& auth_code);
    static Result<PSNTokens> exchange_npsso(const std::string& npsso_token);
    static Result<PSNTokens> refresh_tokens(const std::string& refresh_token);
    static Result<uint64_t> decode_account_id_from_jwt(const std::string& access_token);
    static Result<PSNProfile> fetch_profile(const std::string& access_token, uint64_t known_account_id = 0, const std::string& known_account_id_b64 = "");
    static PSNProfile parse_profile_json(const std::string& json_str, uint64_t known_account_id = 0, const std::string& known_account_id_b64 = "");
};

} // namespace ludelo::auth
