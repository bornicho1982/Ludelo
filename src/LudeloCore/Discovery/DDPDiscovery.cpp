// Archivo: src/LudeloCore/Discovery/DDPDiscovery.cpp
#include "LudeloCore/Discovery/DDPDiscovery.h"
#include "LudeloCore/Net/UDPSocket.h"
#include <spdlog/spdlog.h>
#include <format>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstdlib>

extern "C" {
#include <chiaki/discovery.h>
#include <chiaki/log.h>
}

#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace ludelo::discovery {

DiscoveredConsole DDPDiscovery::parse_response(const std::string& response, const std::string& sender_ip, uint16_t sender_port) {
    DiscoveredConsole console;
    console.address = sender_ip;
    console.port = sender_port;
    console.state = ConsoleState::Unknown;
    
    std::istringstream iss(response);
    std::string line;
    if (std::getline(iss, line)) {
        if (line.find(" 200 ") != std::string::npos || line.find("200 Ok") != std::string::npos || line.find("200 OK") != std::string::npos) {
            console.state = ConsoleState::Awake;
        } else if (line.find(" 620 ") != std::string::npos || line.find("620") != std::string::npos || line.find("Standby") != std::string::npos || line.find("standby") != std::string::npos) {
            console.state = ConsoleState::Standby;
        }
    }

    auto trim = [](std::string s) {
        auto start = s.find_first_not_of(" \n\r\t");
        if (start == std::string::npos) return std::string{};
        auto end = s.find_last_not_of(" \n\r\t");
        return s.substr(start, end - start + 1);
    };

    while (std::getline(iss, line)) {
        auto colon = line.find(":");
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string val = trim(line.substr(colon + 1));

        if (key.find("host-id") != std::string::npos) {
            console.host_id = val;
        } else if (key.find("host-name") != std::string::npos) {
            console.host_name = val;
        } else if (key.find("host-type") != std::string::npos) {
            console.host_type = val;
        } else if (key.find("host-request-port") != std::string::npos) {
            try {
                console.port = static_cast<uint16_t>(std::stoi(val));
            } catch (...) {}
        } else if (key.find("system-version") != std::string::npos) {
            console.system_version = val;
        }
    }

    if (console.port == 997 || console.port == 9302 || console.port == 987 || console.port == 0) {
        console.port = 9295;
    }

    return console;
}

Result<std::vector<DiscoveredConsole>> DDPDiscovery::search(int timeout_ms, const std::vector<std::string>& known_ips) {
    spdlog::info("Starting DDP Discovery search (timeout: {}ms)", timeout_ms);

    ludelo::net::UDPSocket sock;
    (void)sock.bind(0);
    if (auto res = sock.set_broadcast(true); !res) {
        return std::unexpected(res.error());
    }
    sock.set_nonblocking(true);

    std::string payload = "SRCH * HTTP/1.1\r\ndevice-discovery-protocol-version: 00030010\r\n\r\n";
    std::span<const uint8_t> span_payload(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

    // Broadcast a puertos PS4 (987) y PS5 (9302)
    sock.send_to("255.255.255.255", 987, span_payload);
    sock.send_to("255.255.255.255", 9302, span_payload);

    // Enviar sondeo directo unicast a IPs conocidas para saltarse bloqueos de aislamiento WiFi en routers
    for (const auto& ip : known_ips) {
        if (!ip.empty()) {
            sock.send_to(ip, 9302, span_payload);
            sock.send_to(ip, 987, span_payload);
        }
    }

    // Enviar tambien a subredes locales activas (broadcast)
#ifdef _WIN32
    ULONG outBufLen = 16384;
    std::vector<uint8_t> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen) == NO_ERROR) {
        for (auto pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next) {
            if (pCurr->OperStatus != IfOperStatusUp) continue;
            for (auto pUni = pCurr->FirstUnicastAddress; pUni != nullptr; pUni = pUni->Next) {
                if (pUni->Address.lpSockaddr && pUni->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(pUni->Address.lpSockaddr);
                    uint32_t ip = ntohl(sin->sin_addr.s_addr);
                    uint8_t prefix = pUni->OnLinkPrefixLength;
                    if (prefix > 0 && prefix < 32 && prefix >= 8) {
                        uint32_t mask = (0xFFFFFFFF << (32 - prefix));
                        uint32_t bcast = (ip & mask) | (~mask);
                        in_addr baddr{};
                        baddr.s_addr = htonl(bcast);
                        char bcast_str[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &baddr, bcast_str, sizeof(bcast_str));
                        sock.send_to(bcast_str, 987, span_payload);
                        sock.send_to(bcast_str, 9302, span_payload);
                    }
                }
            }
        }
    }
#endif

    std::vector<DiscoveredConsole> consoles;
    
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < timeout_ms) {
        auto res = sock.receive_from();
        if (res.has_value()) {
            auto& [buf, sender_ip, sender_port] = res.value();
            if (!buf.empty()) {
                std::string response(reinterpret_cast<const char*>(buf.data()), buf.size());
                auto console = parse_response(response, sender_ip, sender_port);
                
                if (!console.host_name.empty()) {
                    auto it = std::find_if(consoles.begin(), consoles.end(), [&](const auto& c) {
                        return (!console.host_id.empty() && c.host_id == console.host_id) || c.address == console.address;
                    });
                    if (it == consoles.end()) {
                        spdlog::info("Discovered console: {} ({}, ID: {}, State: {}) at {}:{}",
                            console.host_name, console.host_type, ludelo::mask_secret(console.host_id),
                            (int)console.state, console.address, console.port);
                        consoles.push_back(console);
                    } else {
                        it->state = console.state;
                        it->address = console.address;
                        it->port = console.port;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return consoles;
}

ConsoleState DDPDiscovery::probe_console(const std::string& ip, uint16_t port, int timeout_ms) {
    if (ip.empty()) return ConsoleState::Unknown;
    ludelo::net::UDPSocket sock;
    if (auto res = sock.bind(0); !res) {
        return ConsoleState::Unknown;
    }
    sock.set_nonblocking(true);

    std::string payload = "SRCH * HTTP/1.1\r\ndevice-discovery-protocol-version: 00030010\r\n\r\n";
    std::span<const uint8_t> span_payload(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

    // Send probe directly to both 9302 (PS5) and 987 (PS4)
    sock.send_to(ip, 9302, span_payload);
    sock.send_to(ip, 987, span_payload);

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < timeout_ms) {
        auto res = sock.receive_from();
        if (res.has_value()) {
            auto& [buf, sender_ip, sender_port] = res.value();
            if (!buf.empty()) {
                std::string response(reinterpret_cast<const char*>(buf.data()), buf.size());
                auto console = parse_response(response, sender_ip, sender_port);
                if (console.state != ConsoleState::Unknown) {
                    return console.state;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    return ConsoleState::Unknown;
}

VoidResult DDPDiscovery::wake(const std::string& host, const std::string& rp_auth, bool is_ps5) {
    uint64_t credential = 0;
    if (!rp_auth.empty()) {
        credential = static_cast<uint64_t>(std::strtoull(rp_auth.c_str(), nullptr, 16));
    }
    spdlog::info("[DDPDiscovery] Sending Chiaki Wake-on-LAN to {} (credential: {}, is_ps5: {})",
        host, ludelo::mask_secret(std::to_string(credential)), is_ps5);

    // 1. Direct Unicast to target host via Chiaki Core
    ChiakiErrorCode err1 = chiaki_discovery_wakeup(nullptr, nullptr, host.c_str(), credential, is_ps5);
    if (err1 != CHIAKI_ERR_SUCCESS) {
        spdlog::warn("[DDPDiscovery] chiaki_discovery_wakeup to host {} failed: {}", host, chiaki_error_string(err1));
    }

    // 2. Broadcast to 255.255.255.255 as backup via Chiaki Core
    (void)chiaki_discovery_wakeup(nullptr, nullptr, "255.255.255.255", credential, is_ps5);

    return {};
}

VoidResult DDPDiscovery::wake(const DiscoveredConsole& console) {
    bool is_ps5 = (console.host_type == "PS5" || console.host_type.find("5") != std::string::npos);
    return wake(console.address, "", is_ps5);
}

} // namespace ludelo::discovery
