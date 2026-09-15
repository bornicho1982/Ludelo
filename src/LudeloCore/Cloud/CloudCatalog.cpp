// Archivo: src/LudeloCore/Cloud/CloudCatalog.cpp
// CloudCatalog — Cache manager for the PS Plus Cloud game catalog
#include "CloudCatalog.h"

#if defined(LUDELO_CLOUD_EXPERIMENTAL) && (LUDELO_CLOUD_EXPERIMENTAL == 1)

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace ludelo::cloud {

CloudCatalog::CloudCatalog(const std::string& cache_path)
    : m_cache_path(cache_path)
{
    load_cache();
}

CloudCatalog::~CloudCatalog() {
    save_cache();
}

// ─── fetch_and_cache ────────────────────────────────────
ludelo::VoidResult CloudCatalog::fetch_and_cache(const std::string& access_token) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    // Return cached data if still within TTL
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_games.empty() && (current_time - m_last_fetch_time) < CACHE_TTL_SECONDS) {
            spdlog::info("[CloudCatalog] Cache valid ({} games). Skipping fetch.",
                m_games.size());
            return {};
        }
    }

    GaikaiClient client;
    auto result = client.get_catalog(access_token);

    if (!result) {
        spdlog::error("[CloudCatalog] Catalog fetch failed: {}", result.error().message);
        return std::unexpected(result.error());
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_games = std::move(result.value());
        m_last_fetch_time = current_time;
    }

    save_cache();
    spdlog::info("[CloudCatalog] Catalog updated: {} games available.", m_games.size());
    return {};
}

// ─── search ─────────────────────────────────────────────
std::vector<CloudGame> CloudCatalog::search(const std::string& query) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (query.empty()) return m_games;

    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    std::vector<CloudGame> results;
    for (const auto& game : m_games) {
        std::string t = game.title;
        std::transform(t.begin(), t.end(), t.begin(),
            [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (t.find(q) != std::string::npos) {
            results.push_back(game);
        }
    }
    return results;
}

// ─── get_by_id ──────────────────────────────────────────
std::optional<CloudGame> CloudCatalog::get_by_id(const std::string& game_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_games.begin(), m_games.end(),
        [&](const CloudGame& g){ return g.game_id == game_id; });
    if (it != m_games.end()) return *it;
    return std::nullopt;
}

// ─── get_recent ─────────────────────────────────────────
std::vector<CloudGame> CloudCatalog::get_recent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recent_games;
}

// ─── save_cache ─────────────────────────────────────────
void CloudCatalog::save_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_games.empty()) return;

    try {
        // Ensure parent directory exists
        std::filesystem::path p(m_cache_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        nlohmann::json j;
        j["last_fetch_time"] = m_last_fetch_time;
        j["games"] = nlohmann::json::array();

        for (const auto& g : m_games) {
            nlohmann::json item;
            item["game_id"]   = g.game_id;
            item["title"]     = g.title;
            item["cover_url"] = g.cover_url;
            item["platform"]  = g.platform;
            item["genres"]    = g.genres;
            j["games"].push_back(std::move(item));
        }

        std::ofstream file(m_cache_path);
        if (!file.is_open()) {
            spdlog::warn("[CloudCatalog] Could not open cache file for writing: {}", m_cache_path);
            return;
        }
        file << j.dump(2);
        spdlog::debug("[CloudCatalog] Cache saved: {} games to {}", m_games.size(), m_cache_path);

    } catch (const std::exception& e) {
        spdlog::error("[CloudCatalog] Failed to save catalog cache: {}", e.what());
    }
}

// ─── load_cache ─────────────────────────────────────────
void CloudCatalog::load_cache() {
    std::ifstream file(m_cache_path);
    if (!file.is_open()) return;

    try {
        auto j = nlohmann::json::parse(file);
        m_last_fetch_time = j.value("last_fetch_time", uint64_t{0});

        auto& raw_games = j["games"];
        m_games.clear();
        m_games.reserve(raw_games.size());

        for (const auto& item : raw_games) {
            CloudGame g;
            g.game_id   = item.value("game_id",   "");
            g.title     = item.value("title",     "");
            g.cover_url = item.value("cover_url", "");
            g.platform  = item.value("platform",  "");
            if (item.contains("genres") && item["genres"].is_array()) {
                for (const auto& genre : item["genres"]) {
                    if (genre.is_string()) g.genres.push_back(genre.get<std::string>());
                }
            }
            if (!g.game_id.empty()) m_games.push_back(std::move(g));
        }

        spdlog::info("[CloudCatalog] Loaded {} games from disk cache.", m_games.size());

    } catch (const std::exception& e) {
        spdlog::warn("[CloudCatalog] Could not parse catalog cache: {}", e.what());
        m_games.clear();
        m_last_fetch_time = 0;
    }
}

} // namespace ludelo::cloud

#else // !LUDELO_CLOUD_EXPERIMENTAL

namespace ludelo::cloud {

CloudCatalog::CloudCatalog(const std::string& cache_path)
    : m_cache_path(cache_path)
{
}

CloudCatalog::~CloudCatalog() = default;

ludelo::VoidResult CloudCatalog::fetch_and_cache(const std::string&) {
    return std::unexpected(ludelo::Error{ludelo::ErrorCode::CloudNotAvailable, "Cloud streaming is disabled in this build."});
}

std::vector<CloudGame> CloudCatalog::search(const std::string&) const {
    return {};
}

std::optional<CloudGame> CloudCatalog::get_by_id(const std::string&) const {
    return std::nullopt;
}

std::vector<CloudGame> CloudCatalog::get_recent() const {
    return {};
}

void CloudCatalog::load_cache() {}
void CloudCatalog::save_cache() {}

} // namespace ludelo::cloud

#endif // LUDELO_CLOUD_EXPERIMENTAL
