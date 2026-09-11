// Archivo: src/PortalCore/Discovery/ConsoleRegistry.cpp
#include "ConsoleRegistry.h"
#include "PortalCore/Auth/Keychain.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>

namespace portal::discovery {

static bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

Result<void> ConsoleRegistry::register_console(const DiscoveredConsole& console, const portal::ByteBuffer& rp_key, const std::string& credentials) {
    auto logger = spdlog::get("portal");
    if (logger) logger->info("Registering console {} at {}", console.host_id, console.address);

    RegisteredConsole reg;
    reg.host_name = console.host_name.empty() ? "PlayStation 5" : console.host_name;
    reg.host_id = console.host_id;
    reg.host_type = console.host_type.empty() ? "PS5" : console.host_type;
    reg.address = console.address;
    reg.port = console.port;
    if (reg.port == 997 || reg.port == 9302 || reg.port == 987 || reg.port == 0) {
        reg.port = 9295;
    }
    reg.state = console.state;
    reg.system_version = console.system_version.empty() ? "13600007" : console.system_version;
    
    reg.rp_key = rp_key;
    reg.rp_auth = credentials;
    reg.registration_timestamp = std::chrono::system_clock::now();
    reg.nickname = reg.host_name;

    // Check if already registered by host_id (case-insensitive) OR by IP address
    auto it = std::find_if(consoles.begin(), consoles.end(), [&](const RegisteredConsole& c) {
        bool match_id = !c.host_id.empty() && !console.host_id.empty() && iequals(c.host_id, console.host_id);
        bool match_ip = !c.address.empty() && !console.address.empty() && c.address == console.address;
        return match_id || match_ip;
    });

    if (it != consoles.end()) {
        if (reg.login_pin.empty() && !it->login_pin.empty()) {
            reg.login_pin = it->login_pin;
        }
        *it = reg;
    } else {
        consoles.push_back(reg);
    }

    return save_to_disk();
}

void ConsoleRegistry::unregister_console(const std::string& host_id) {
    std::erase_if(consoles, [&](const RegisteredConsole& c) {
        return iequals(c.host_id, host_id);
    });
    (void)save_to_disk();
}

void ConsoleRegistry::update_console_state(const std::string& host_id, ConsoleState state, const std::string& ip) {
    for (auto& c : consoles) {
        bool match_id = !host_id.empty() && !c.host_id.empty() && iequals(c.host_id, host_id);
        bool match_ip = !ip.empty() && !c.address.empty() && c.address == ip;
        if (match_id || match_ip) {
            c.state = state;
            if (!ip.empty()) c.address = ip;
            break;
        }
    }
}

void ConsoleRegistry::update_console_login_pin(const std::string& host_id, const std::string& pin) {
    for (auto& c : consoles) {
        if (iequals(c.host_id, host_id)) {
            c.login_pin = pin;
            break;
        }
    }
    (void)save_to_disk();
}

std::optional<RegisteredConsole> ConsoleRegistry::get_console(const std::string& host_id) const {
    for (const auto& c : consoles) {
        if (iequals(c.host_id, host_id)) return c;
    }
    return std::nullopt;
}

std::vector<RegisteredConsole> ConsoleRegistry::get_all_consoles() const {
    return consoles;
}

static std::string encode_hex(const portal::ByteBuffer& buf) {
    std::string res;
    for (uint8_t b : buf) {
        res += std::format("{:02x}", b);
    }
    return res;
}

static portal::ByteBuffer decode_hex(const std::string& hex) {
    portal::ByteBuffer res;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        res.push_back(byte);
    }
    return res;
}

Result<void> ConsoleRegistry::save_to_disk() {
    auto logger = spdlog::get("portal");
    nlohmann::json j = nlohmann::json::array();
    
    for (const auto& c : consoles) {
        nlohmann::json obj;
        obj["host_name"] = c.host_name;
        obj["host_id"] = c.host_id;
        obj["host_type"] = c.host_type;
        obj["address"] = c.address;
        obj["port"] = c.port;
        obj["state"] = static_cast<int>(c.state);
        obj["system_version"] = c.system_version;
        
        auto enc_rp_key = portal::auth::Keychain::encrypt(c.rp_key);
        if (enc_rp_key) {
            obj["rp_key_enc"] = encode_hex(enc_rp_key.value());
        }
        
        if (!c.rp_auth.empty()) {
            portal::ByteBuffer auth_bytes(c.rp_auth.begin(), c.rp_auth.end());
            auto enc_rp_auth = portal::auth::Keychain::encrypt(auth_bytes);
            if (enc_rp_auth) {
                obj["rp_auth_enc"] = encode_hex(enc_rp_auth.value());
            }
        }
        obj["registration_timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(c.registration_timestamp.time_since_epoch()).count();
        obj["nickname"] = c.nickname;
        obj["login_pin"] = c.login_pin;
        
        j.push_back(obj);
    }

    std::string json_str = j.dump();
    portal::ByteBuffer data(json_str.begin(), json_str.end());
    return portal::auth::Keychain::store_secret("consoles.json", data);
}

Result<void> ConsoleRegistry::load_from_disk() {
    auto logger = spdlog::get("portal");
    auto read_res = portal::auth::Keychain::load_secret("consoles.json");
    if (!read_res) {
        return std::unexpected(read_res.error());
    }

    std::string json_str(read_res.value().begin(), read_res.value().end());
    try {
        auto j = nlohmann::json::parse(json_str);
        consoles.clear();
        for (const auto& obj : j) {
            RegisteredConsole c;
            c.host_name = obj.value("host_name", "PlayStation 5");
            c.host_id = obj.value("host_id", "");
            c.host_type = obj.value("host_type", "PS5");
            c.address = obj.value("address", "");
            c.port = obj.value("port", 9295);
            if (c.port == 997 || c.port == 9302 || c.port == 987 || c.port == 0) {
                c.port = 9295;
            }
            c.state = static_cast<ConsoleState>(obj.value("state", 0));
            c.system_version = obj.value("system_version", "13600007");
            
            if (obj.contains("rp_key_enc")) {
                std::string hex_enc = obj["rp_key_enc"];
                auto dec = portal::auth::Keychain::decrypt(decode_hex(hex_enc));
                if (dec) {
                    c.rp_key = dec.value();
                }
            }
            
            if (obj.contains("rp_auth_enc")) {
                std::string hex_enc = obj["rp_auth_enc"];
                auto dec = portal::auth::Keychain::decrypt(decode_hex(hex_enc));
                if (dec) {
                    c.rp_auth = std::string(dec.value().begin(), dec.value().end());
                }
            } else {
                c.rp_auth = obj.value("rp_auth", "");
            }
            int64_t ts = obj.value("registration_timestamp", 0LL);
            c.registration_timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(ts));
            c.nickname = obj.value("nickname", c.host_name);
            c.login_pin = obj.value("login_pin", "");
            
            // Deduplicate against already loaded items
            auto it = std::find_if(consoles.begin(), consoles.end(), [&](const RegisteredConsole& existing) {
                bool match_id = !existing.host_id.empty() && !c.host_id.empty() && iequals(existing.host_id, c.host_id);
                bool match_ip = !existing.address.empty() && !c.address.empty() && existing.address == c.address;
                return match_id || match_ip;
            });
            if (it != consoles.end()) {
                if (it->rp_key.empty() && !c.rp_key.empty()) {
                    *it = c;
                } else if (!c.rp_auth.empty() && it->rp_auth.empty()) {
                    it->rp_auth = c.rp_auth;
                }
                if (it->login_pin.empty() && !c.login_pin.empty()) {
                    it->login_pin = c.login_pin;
                }
            } else {
                consoles.push_back(c);
            }
        }
    } catch (const std::exception& e) {
        if (logger) logger->error("Failed to parse consoles JSON: {}", e.what());
        return std::unexpected(ErrorCode::JsonParseError);
    }
    
    return {};
}

} // namespace portal::discovery
