// Archivo: src/PortalCore/Discovery/ConsoleRegistry.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Discovery/DDPDiscovery.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace portal::discovery {

struct RegisteredConsole {
    std::string host_name;
    std::string host_id;
    std::string host_type;
    std::string address;
    uint16_t port;
    ConsoleState state;
    std::string system_version;
    
    portal::ByteBuffer rp_key;
    std::string rp_auth;
    std::chrono::system_clock::time_point registration_timestamp;
    std::string nickname;
    std::string login_pin; // 4-digit PS5 user profile passcode
};

class ConsoleRegistry {
public:
    Result<void> register_console(const DiscoveredConsole& console, const portal::ByteBuffer& rp_key, const std::string& credentials);
    void unregister_console(const std::string& host_id);
    void update_console_state(const std::string& host_id, ConsoleState state, const std::string& ip = "");
    void update_console_login_pin(const std::string& host_id, const std::string& pin);
    std::optional<RegisteredConsole> get_console(const std::string& host_id) const;
    std::vector<RegisteredConsole> get_all_consoles() const;

    Result<void> save_to_disk();
    Result<void> load_from_disk();

private:
    std::vector<RegisteredConsole> consoles;
};

} // namespace portal::discovery

