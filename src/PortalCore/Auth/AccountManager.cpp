// Archivo: src/PortalCore/Auth/AccountManager.cpp
#include "AccountManager.h"
#include "PortalCore/Auth/Keychain.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace portal::auth {

Result<void> AccountManager::add_account(const std::string& npsso) {
    auto logger = spdlog::get("portal");
    if (logger) logger->info("Adding new account");

    auto tokens_res = PSNAuth::exchange_npsso(npsso);
    if (!tokens_res) return std::unexpected(tokens_res.error());

    auto profile_res = PSNAuth::fetch_profile(tokens_res.value().access_token);
    if (!profile_res) return std::unexpected(profile_res.error());

    PSNAccount acc;
    acc.account_id = profile_res.value().account_id;
    acc.npsso = npsso;
    acc.tokens = tokens_res.value();
    acc.profile = profile_res.value();

    accounts.push_back(acc);
    active_account_id = acc.account_id;
    
    return save_to_disk();
}

void AccountManager::remove_account(uint64_t account_id) {
    auto logger = spdlog::get("portal");
    std::erase_if(accounts, [account_id](const PSNAccount& acc) {
        return acc.account_id == account_id;
    });
    if (active_account_id == account_id) {
        active_account_id = std::nullopt;
        if (!accounts.empty()) {
            active_account_id = accounts.front().account_id;
        }
    }
    (void)save_to_disk();
}

std::optional<PSNAccount> AccountManager::get_active_account() const {
    if (!active_account_id) return std::nullopt;
    for (const auto& acc : accounts) {
        if (acc.account_id == *active_account_id) return acc;
    }
    return std::nullopt;
}

Result<std::string> AccountManager::get_access_token() {
    auto logger = spdlog::get("portal");
    if (!active_account_id) return std::unexpected(ErrorCode::NoActiveAccount);

    for (auto& acc : accounts) {
        if (acc.account_id == *active_account_id) {
            auto now = std::chrono::system_clock::now();
            if (acc.tokens.expires_at < now) {
                if (logger) logger->info("Access token expired, refreshing...");
                auto new_tokens_res = PSNAuth::refresh_tokens(acc.tokens.refresh_token);
                if (new_tokens_res) {
                    acc.tokens = new_tokens_res.value();
                    (void)save_to_disk();
                    return acc.tokens.access_token;
                } else {
                    return std::unexpected(new_tokens_res.error());
                }
            }
            return acc.tokens.access_token;
        }
    }
    return std::unexpected(ErrorCode::NoActiveAccount);
}

Result<void> AccountManager::save_to_disk() {
    auto logger = spdlog::get("portal");
    nlohmann::json j;
    j["active_account_id"] = active_account_id.value_or(0);
    
    nlohmann::json acc_array = nlohmann::json::array();
    for (const auto& acc : accounts) {
        nlohmann::json a;
        a["account_id"] = acc.account_id;
        a["npsso"] = acc.npsso;
        a["access_token"] = acc.tokens.access_token;
        a["refresh_token"] = acc.tokens.refresh_token;
        a["expires_at"] = std::chrono::duration_cast<std::chrono::seconds>(acc.tokens.expires_at.time_since_epoch()).count();
        a["online_id"] = acc.profile.online_id;
        a["avatar_url"] = acc.profile.avatar_url;
        a["plus_status"] = acc.profile.plus_status;
        acc_array.push_back(a);
    }
    j["accounts"] = acc_array;

    std::string json_str = j.dump();
    portal::ByteBuffer data(json_str.begin(), json_str.end());
    
    auto enc_res = Keychain::encrypt(data);
    if (!enc_res) return std::unexpected(enc_res.error());
    
    return Keychain::store_secret("accounts.json", enc_res.value());
}

Result<void> AccountManager::load_from_disk() {
    auto logger = spdlog::get("portal");
    auto read_res = Keychain::load_secret("accounts.json");
    if (!read_res) return std::unexpected(read_res.error());

    auto dec_res = Keychain::decrypt(read_res.value());
    if (!dec_res) return std::unexpected(dec_res.error());

    std::string json_str(dec_res.value().begin(), dec_res.value().end());
    try {
        auto j = nlohmann::json::parse(json_str);
        if (j.contains("active_account_id") && j["active_account_id"] != 0) {
            active_account_id = j["active_account_id"];
        } else {
            active_account_id = std::nullopt;
        }

        accounts.clear();
        for (const auto& a : j["accounts"]) {
            PSNAccount acc;
            acc.account_id = a["account_id"];
            acc.npsso = a["npsso"];
            acc.tokens.access_token = a["access_token"];
            acc.tokens.refresh_token = a["refresh_token"];
            int64_t exp = a["expires_at"];
            acc.tokens.expires_at = std::chrono::system_clock::time_point(std::chrono::seconds(exp));
            acc.profile.account_id = acc.account_id;
            acc.profile.online_id = a["online_id"];
            acc.profile.avatar_url = a["avatar_url"];
            acc.profile.plus_status = a["plus_status"];
            accounts.push_back(acc);
        }
    } catch (const std::exception& e) {
        if (logger) logger->error("Failed to parse accounts JSON: {}", e.what());
        return std::unexpected(ErrorCode::JsonParseError);
    }
    
    return {};
}

} // namespace portal::auth
