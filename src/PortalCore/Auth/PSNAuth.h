// Archivo: src/PortalCore/Auth/PSNAuth.h
#pragma once

#include "PortalCore/Common.h"
#include <string>
#include <cstdint>
#include <chrono>

namespace portal::auth {

struct PSNTokens {
    std::string access_token;
    std::string refresh_token;
    std::chrono::system_clock::time_point expires_at;
};

struct PSNProfile {
    uint64_t account_id;
    std::string online_id;
    std::string avatar_url;
    std::string plus_status;
};

class PSNAuth {
public:
    static Result<PSNTokens> exchange_npsso(const std::string& npsso_token);
    static Result<PSNTokens> refresh_tokens(const std::string& refresh_token);
    static Result<uint64_t> decode_account_id_from_jwt(const std::string& access_token);
    static Result<PSNProfile> fetch_profile(const std::string& access_token);
};

} // namespace portal::auth
