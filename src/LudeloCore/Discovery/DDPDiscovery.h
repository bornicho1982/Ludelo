// Archivo: src/LudeloCore/Discovery/DDPDiscovery.h
#pragma once

#include "LudeloCore/Common.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ludelo::discovery {

struct DiscoveredConsole {
    std::string host_name;
    std::string host_id;
    std::string host_type; // "PS4" or "PS5"
    std::string address;
    uint16_t port{0};
    ConsoleState state{ConsoleState::Unknown};
    std::string system_version;
};

class DDPDiscovery {
public:
    static Result<std::vector<DiscoveredConsole>> search(int timeout_ms, const std::vector<std::string>& known_ips = {});
    static ConsoleState probe_console(const std::string& ip, uint16_t port = 9302, int timeout_ms = 450);
    static VoidResult wake(const std::string& host, const std::string& rp_auth, bool is_ps5);
    static VoidResult wake(const DiscoveredConsole& console);
    
    // Parser helper exposed for testing
    static DiscoveredConsole parse_response(const std::string& response_text, const std::string& sender_ip, uint16_t sender_port);
};

} // namespace ludelo::discovery
