// Archivo: src/PortalCore/Cloud/GaikaiClient.cpp
// GaikaiClient — REST client for Sony Kamaji Cloud Streaming API
// Endpoints reverse-engineered from PlayStation Plus Cloud Streaming client
#include "GaikaiClient.h"

#if defined(LUDELO_CLOUD_EXPERIMENTAL) && (LUDELO_CLOUD_EXPERIMENTAL == 1)

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <format>
#include <chrono>

namespace portal::cloud {

// ─── Kamaji API constants ─────────────────────────────────
namespace {

constexpr std::string_view kKamajiBase    = "https://kamaji.cloud.playstation.com";
constexpr std::string_view kKamajiApiPath = "/kamaji/api/pcnow/00_09_000";
constexpr std::string_view kOrigin        = "https://psnow.playstation.com";
constexpr std::string_view kUserAgent     = "Ludelo/1.0.0 (Windows NT 10.0; Win64; x64)";

// Timeout for all Kamaji API calls
constexpr int kTimeoutMs = 10000;

/// Build standard headers for all Kamaji requests
cpr::Header make_auth_headers(const std::string& access_token) {
    return cpr::Header{
        {"Authorization",  std::format("Bearer {}", access_token)},
        {"Origin",         std::string(kOrigin)},
        {"User-Agent",     std::string(kUserAgent)},
        {"Accept",         "application/json"},
        {"Content-Type",   "application/json"},
        {"X-Platform",     "pc:psnow"},
    };
}

/// Map HTTP status codes to portal::Error
portal::Error http_error(long status, const std::string& url, const std::string& body) {
    spdlog::error("[GaikaiClient] HTTP {} from {}: {}", status, url, body.substr(0, 256));
    switch (status) {
        case 401: return {portal::ErrorCode::TokenExpired,          "PSN access token expired or invalid"};
        case 403: return {portal::ErrorCode::NoPSPlusPremium,       "PS Plus Premium entitlement required"};
        case 404: return {portal::ErrorCode::CloudSessionFailed,    "Resource not found on Kamaji"};
        case 429: return {portal::ErrorCode::NetworkError,          "Rate limited by Kamaji API"};
        default:  return {portal::ErrorCode::CloudError,
                          std::format("Kamaji HTTP {} error", status)};
    }
}

} // namespace

// ─── Constructor / Destructor ─────────────────────────────
GaikaiClient::GaikaiClient() = default;
GaikaiClient::~GaikaiClient() = default;

// ─── check_entitlement ────────────────────────────────────
// GET /kamaji/api/pcnow/00_09_000/user/entitlement
// Returns true if the account has an active PS Plus Premium subscription.
portal::Result<bool> GaikaiClient::check_entitlement(const std::string& access_token) {
    if (access_token.empty()) {
        return std::unexpected(portal::Error{portal::ErrorCode::NoActiveAccount,
            "No access token provided"});
    }

    const std::string url = std::format("{}{}/user/entitlement", kKamajiBase, kKamajiApiPath);
    spdlog::info("[GaikaiClient] Checking PS Plus Premium entitlement...");

    auto response = cpr::Get(
        cpr::Url{url},
        make_auth_headers(access_token),
        cpr::Timeout{kTimeoutMs}
    );

    if (response.status_code == 0) {
        return std::unexpected(portal::Error{portal::ErrorCode::NetworkError,
            std::format("Network error: {}", response.error.message)});
    }
    if (response.status_code != 200) {
        return std::unexpected(http_error(response.status_code, url, response.text));
    }

    try {
        auto j = nlohmann::json::parse(response.text);
        // Kamaji returns {"entitled": true, "tier": "premium", ...}
        bool entitled = j.value("entitled", false);
        std::string tier = j.value("tier", "none");
        spdlog::info("[GaikaiClient] Entitlement check: entitled={}, tier={}", entitled, tier);

        if (!entitled || (tier != "premium" && tier != "ps-plus-premium")) {
            return std::unexpected(portal::Error{portal::ErrorCode::NoPSPlusPremium,
                std::format("Account not entitled for cloud streaming (tier: {})", tier)});
        }
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[GaikaiClient] Failed to parse entitlement response: {}", e.what());
        return std::unexpected(portal::Error{portal::ErrorCode::JsonParseError, e.what()});
    }
}

// ─── get_catalog ─────────────────────────────────────────
// GET /kamaji/api/pcnow/00_09_000/catalog/games?platform=pc:psnow&limit=500
// Returns full list of cloud-streamable games for this account.
portal::Result<std::vector<CloudGame>> GaikaiClient::get_catalog(const std::string& access_token) {
    if (access_token.empty()) {
        return std::unexpected(portal::Error{portal::ErrorCode::NoActiveAccount,
            "No access token provided"});
    }

    const std::string url = std::format("{}{}/catalog/games", kKamajiBase, kKamajiApiPath);
    spdlog::info("[GaikaiClient] Fetching cloud game catalog...");

    auto response = cpr::Get(
        cpr::Url{url},
        make_auth_headers(access_token),
        cpr::Parameters{
            {"platform", "pc:psnow"},
            {"limit",    "500"},
            {"offset",   "0"},
        },
        cpr::Timeout{kTimeoutMs}
    );

    if (response.status_code == 0) {
        return std::unexpected(portal::Error{portal::ErrorCode::NetworkError,
            std::format("Network error: {}", response.error.message)});
    }
    if (response.status_code != 200) {
        return std::unexpected(http_error(response.status_code, url, response.text));
    }

    try {
        auto j = nlohmann::json::parse(response.text);
        std::vector<CloudGame> games;

        // Kamaji catalog response:
        // {"data": [{"titleId": "CUSA00001", "name": "...", "imageUrl": "...",
        //            "platform": "PS5", "genres": ["Action"]}]}
        const auto& data = j.value("data", nlohmann::json::array());
        games.reserve(data.size());

        for (const auto& item : data) {
            CloudGame g;
            g.game_id   = item.value("titleId",  item.value("gameId", ""));
            g.title     = item.value("name",     item.value("title", ""));
            g.cover_url = item.value("imageUrl", item.value("thumbnailUrl", ""));
            g.platform  = item.value("platform", "PS4");

            if (item.contains("genres") && item["genres"].is_array()) {
                for (const auto& genre : item["genres"]) {
                    if (genre.is_string()) g.genres.push_back(genre.get<std::string>());
                }
            }

            if (!g.game_id.empty() && !g.title.empty()) {
                games.push_back(std::move(g));
            }
        }

        spdlog::info("[GaikaiClient] Catalog fetched: {} games available for streaming", games.size());
        return games;

    } catch (const std::exception& e) {
        spdlog::error("[GaikaiClient] Failed to parse catalog response: {}", e.what());
        return std::unexpected(portal::Error{portal::ErrorCode::JsonParseError, e.what()});
    }
}

// ─── create_session ──────────────────────────────────────
// POST /kamaji/api/pcnow/00_09_000/user/session
// Creates a new cloud streaming session for a given game.
// Returns session ID, streaming endpoint, and TURN server credentials.
portal::Result<CloudSession> GaikaiClient::create_session(
    const std::string& access_token,
    const std::string& game_id,
    const CloudStreamConfig& config)
{
    if (access_token.empty()) {
        return std::unexpected(portal::Error{portal::ErrorCode::NoActiveAccount,
            "No access token provided"});
    }
    if (game_id.empty()) {
        return std::unexpected(portal::Error{portal::ErrorCode::InvalidParameter,
            "game_id cannot be empty"});
    }

    // First verify entitlement — fail fast before requesting a session
    auto entitlement = check_entitlement(access_token);
    if (!entitlement) {
        return std::unexpected(entitlement.error());
    }

    const std::string url = std::format("{}{}/user/session", kKamajiBase, kKamajiApiPath);
    spdlog::info("[GaikaiClient] Creating cloud session for game: {}", game_id);

    // Build request body
    nlohmann::json body;
    body["titleId"]    = game_id;
    body["platform"]   = "pc:psnow";
    body["resolution"] = config.resolution.empty() ? "1080p" : config.resolution;
    body["fps"]        = config.fps > 0 ? config.fps : 60;
    body["bitrate"]    = config.video_bitrate > 0 ? config.video_bitrate : 15000;
    body["deviceType"] = "PC";
    body["clientVersion"] = "Ludelo/1.0.0";

    auto response = cpr::Post(
        cpr::Url{url},
        make_auth_headers(access_token),
        cpr::Body{body.dump()},
        cpr::Timeout{kTimeoutMs}
    );

    if (response.status_code == 0) {
        return std::unexpected(portal::Error{portal::ErrorCode::NetworkError,
            std::format("Network error: {}", response.error.message)});
    }
    if (response.status_code != 200 && response.status_code != 201) {
        return std::unexpected(http_error(response.status_code, url, response.text));
    }

    try {
        auto j = nlohmann::json::parse(response.text);

        CloudSession session;
        session.session_id         = j.value("sessionId",         "");
        session.streaming_endpoint = j.value("streamingEndpoint", "");

        // TURN server credentials for NAT traversal
        if (j.contains("turnServer") && j["turnServer"].is_object()) {
            auto& turn = j["turnServer"];
            session.turn_server   = turn.value("url",      "");
            session.turn_username = turn.value("username", "");
            session.turn_password = turn.value("password", "");
        }

        // Populate game info from the response (Kamaji echoes title metadata)
        session.game_info.game_id = game_id;
        if (j.contains("gameInfo") && j["gameInfo"].is_object()) {
            auto& gi = j["gameInfo"];
            session.game_info.title     = gi.value("name",      "");
            session.game_info.cover_url = gi.value("imageUrl",  "");
            session.game_info.platform  = gi.value("platform",  "PS5");
        }

        if (session.session_id.empty() || session.streaming_endpoint.empty()) {
            spdlog::error("[GaikaiClient] Kamaji response missing sessionId or streamingEndpoint");
            return std::unexpected(portal::Error{portal::ErrorCode::CloudSessionFailed,
                "Incomplete session response from Kamaji"});
        }

        spdlog::info("[GaikaiClient] Cloud session created: {} -> {}", 
            session.session_id, session.streaming_endpoint);
        return session;

    } catch (const std::exception& e) {
        spdlog::error("[GaikaiClient] Failed to parse session response: {}", e.what());
        return std::unexpected(portal::Error{portal::ErrorCode::JsonParseError, e.what()});
    }
}

// ─── end_session ─────────────────────────────────────────
// DELETE /kamaji/api/pcnow/00_09_000/user/session/<session_id>
portal::VoidResult GaikaiClient::end_session(const std::string& session_id) {
    if (session_id.empty()) {
        return std::unexpected(portal::Error{portal::ErrorCode::InvalidParameter,
            "session_id cannot be empty"});
    }

    const std::string url = std::format("{}{}/user/session/{}", kKamajiBase, kKamajiApiPath, session_id);
    spdlog::info("[GaikaiClient] Ending cloud session: {}", session_id);

    // Note: end_session is best-effort; we don't have the token here so we use
    // a fire-and-forget DELETE. Token should be passed in future refactor.
    auto response = cpr::Delete(
        cpr::Url{url},
        cpr::Header{
            {"User-Agent", std::string(kUserAgent)},
            {"Origin",     std::string(kOrigin)},
        },
        cpr::Timeout{5000}
    );

    if (response.status_code != 200 && response.status_code != 204 && response.status_code != 0) {
        spdlog::warn("[GaikaiClient] end_session returned HTTP {} (non-fatal)", response.status_code);
    }

    spdlog::info("[GaikaiClient] Cloud session ended.");
    return {};
}

} // namespace portal::cloud

#else // !LUDELO_CLOUD_EXPERIMENTAL

namespace portal::cloud {

GaikaiClient::GaikaiClient() = default;
GaikaiClient::~GaikaiClient() = default;

portal::Result<CloudSession> GaikaiClient::create_session(const std::string&, const std::string&, const CloudStreamConfig&) {
    return std::unexpected(portal::Error{portal::ErrorCode::CloudNotAvailable, "Cloud streaming is disabled in this build."});
}

portal::Result<std::vector<CloudGame>> GaikaiClient::get_catalog(const std::string&) {
    return std::unexpected(portal::Error{portal::ErrorCode::CloudNotAvailable, "Cloud streaming is disabled in this build."});
}

portal::Result<bool> GaikaiClient::check_entitlement(const std::string&) {
    return std::unexpected(portal::Error{portal::ErrorCode::CloudNotAvailable, "Cloud streaming is disabled in this build."});
}

portal::VoidResult GaikaiClient::end_session(const std::string&) {
    return {};
}

} // namespace portal::cloud

#endif // LUDELO_CLOUD_EXPERIMENTAL
