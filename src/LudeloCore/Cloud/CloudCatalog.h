// Archivo: src/LudeloCore/Cloud/CloudCatalog.h
#pragma once

#include "LudeloCore/Cloud/GaikaiClient.h"
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <expected>

namespace ludelo::cloud {

class CloudCatalog {
public:
    explicit CloudCatalog(const std::string& cache_path = "cloud_catalog_cache.json");
    ~CloudCatalog();

    ludelo::VoidResult fetch_and_cache(const std::string& access_token);
    std::vector<CloudGame> search(const std::string& query) const;
    std::optional<CloudGame> get_by_id(const std::string& game_id) const;
    std::vector<CloudGame> get_recent() const;

private:
    void load_cache();
    void save_cache();

    std::vector<CloudGame> m_games;
    std::vector<CloudGame> m_recent_games;
    mutable std::mutex m_mutex;
    
    std::string m_cache_path;
    uint64_t m_last_fetch_time{0};
    const uint64_t CACHE_TTL_SECONDS = 24 * 60 * 60; // 24 hours
};

} // namespace ludelo::cloud
