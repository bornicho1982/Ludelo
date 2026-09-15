// Archivo: src/LudeloCore/Auth/AccountManager.cpp
#include "AccountManager.h"
#include "LudeloCore/Auth/Keychain.h"
#include "LudeloCore/Net/HttpClient.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <openssl/evp.h>

namespace ludelo::auth {

Result<void> AccountManager::add_account(const std::string& npsso) {
    auto logger = spdlog::get("ludelo");
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

Result<void> AccountManager::add_account_from_code(const std::string& auth_code) {
    spdlog::info("AccountManager::add_account_from_code: starting");

    auto tokens_res = PSNAuth::exchange_code(auth_code);
    if (!tokens_res) {
        spdlog::error("AccountManager::add_account_from_code: exchange_code FAILED: {}", tokens_res.error().message);
        return std::unexpected(tokens_res.error());
    }
    spdlog::info("AccountManager::add_account_from_code: exchange_code OK");

    PSNProfile profile;
    auto profile_res = PSNAuth::fetch_profile(tokens_res.value().access_token, tokens_res->account_id, tokens_res->account_id_b64);
    if (!profile_res) {
        spdlog::warn("AccountManager::add_account_from_code: profile fetch failed ({}), saving account anyway with defaults", profile_res.error().message);
        profile.account_id = tokens_res->account_id;
        profile.account_id_b64 = tokens_res->account_id_b64;
        profile.online_id = "PSN User";
        profile.plus_status = "none";
    } else {
        profile = profile_res.value();
    }
    spdlog::info("AccountManager::add_account_from_code: fetch_profile OK, online_id='{}', account_id={}", 
        profile.online_id, profile.account_id_b64);

    if (!profile.avatar_url.empty()) {
        try {
            const char* appdata = getenv("APPDATA");
            std::filesystem::path av_dir = appdata ? (std::filesystem::path(appdata) / "Ludelo") : std::filesystem::path("Ludelo");
            std::filesystem::create_directories(av_dir);
            std::filesystem::path av_file = av_dir / "avatar.png";

            ludelo::net::HttpClient dl_client;
            auto dl_res = dl_client.get(profile.avatar_url);
            if (dl_res && !dl_res->body.empty()) {
                std::ofstream ofs(av_file, std::ios::binary);
                ofs.write(dl_res->body.data(), dl_res->body.size());
                spdlog::info("AccountManager: avatar downloaded to {} ({} bytes)", av_file.string(), dl_res->body.size());
            }
        } catch (const std::exception& e) {
            spdlog::warn("AccountManager: failed downloading avatar: {}", e.what());
        }
    }

    PSNAccount acc;
    acc.account_id = (profile.account_id != 0) ? profile.account_id : tokens_res->account_id;
    acc.account_id_b64 = (!profile.account_id_b64.empty()) ? profile.account_id_b64 : tokens_res->account_id_b64;
    acc.npsso = ""; // NPSSO not used in this flow
    acc.tokens = tokens_res.value();
    acc.profile = profile;
    if (acc.profile.account_id == 0) acc.profile.account_id = acc.account_id;
    if (acc.profile.account_id_b64.empty()) acc.profile.account_id_b64 = acc.account_id_b64;

    // Check if account already exists to update it, else add
    bool updated = false;
    for (auto& existing : accounts) {
        if (existing.account_id == acc.account_id) {
            existing = acc;
            updated = true;
            break;
        }
    }
    if (!updated) {
        accounts.push_back(acc);
    }
    
    active_account_id = acc.account_id;

    auto save_res = save_to_disk();
    if (!save_res) {
        spdlog::error("AccountManager::add_account_from_code: save_to_disk FAILED: {}", save_res.error().message);
        return std::unexpected(save_res.error());
    }

    spdlog::info("Account saved, online_id {}", acc.profile.online_id);
    return {};
}

void AccountManager::remove_account(uint64_t account_id) {
    auto logger = spdlog::get("ludelo");
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
    auto logger = spdlog::get("ludelo");
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
    spdlog::info("AccountManager::save_to_disk: serializing {} accounts", accounts.size());
    nlohmann::json j;
    j["active_account_id"] = active_account_id.value_or(0);
    
    nlohmann::json acc_array = nlohmann::json::array();
    for (const auto& acc : accounts) {
        nlohmann::json a;
        a["account_id"] = acc.account_id;
        a["account_id_b64"] = !acc.account_id_b64.empty() ? acc.account_id_b64 : acc.profile.account_id_b64;
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
    ludelo::ByteBuffer data(json_str.begin(), json_str.end());
    
    auto enc_res = Keychain::encrypt(data);
    if (!enc_res) {
        spdlog::error("AccountManager::save_to_disk: DPAPI encrypt FAILED: {}", enc_res.error().message);
        return std::unexpected(enc_res.error());
    }
    spdlog::info("AccountManager::save_to_disk: DPAPI encrypt OK ({} bytes)", enc_res->size());
    
    auto store_res = Keychain::store_secret("accounts.json", enc_res.value());
    if (!store_res) {
        spdlog::error("AccountManager::save_to_disk: store_secret FAILED: {}", store_res.error().message);
        return std::unexpected(store_res.error());
    }
    spdlog::info("AccountManager::save_to_disk: store_secret OK");
    return {};
}

Result<void> AccountManager::load_from_disk() {
    auto logger = spdlog::get("ludelo");
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
            acc.account_id_b64 = a.value("account_id_b64", "");
            acc.npsso = a["npsso"];
            acc.tokens.access_token = a["access_token"];
            acc.tokens.refresh_token = a["refresh_token"];
            int64_t exp = a["expires_at"];
            acc.tokens.expires_at = std::chrono::system_clock::time_point(std::chrono::seconds(exp));
            acc.profile.account_id = acc.account_id;
            acc.profile.account_id_b64 = acc.account_id_b64;

            // If account_id_b64 is missing but account_id != 0, generate it
            if (acc.account_id_b64.empty() && acc.account_id != 0) {
                uint8_t le_bytes[8];
                for (int i = 0; i < 8; ++i) {
                    le_bytes[i] = static_cast<uint8_t>((acc.account_id >> (i * 8)) & 0xFF);
                }
                std::string b64(16, '\0');
                int len = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(b64.data()), le_bytes, 8);
                b64.resize(len);
                acc.account_id_b64 = b64;
                acc.profile.account_id_b64 = b64;
            }

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

} // namespace ludelo::auth
