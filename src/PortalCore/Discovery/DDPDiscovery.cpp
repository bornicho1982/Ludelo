// Archivo: src/PortalCore/Discovery/DDPDiscovery.cpp
#include "PortalCore/Discovery/DDPDiscovery.h"
#include "PortalCore/Net/UDPSocket.h"
#include <spdlog/spdlog.h>
#include <format>
#include <sstream>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace portal::discovery {

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

    portal::net::UDPSocket sock;
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
                            console.host_name, console.host_type, console.host_id,
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
    portal::net::UDPSocket sock;
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
    spdlog::info("[DDPDiscovery] Sending Wake-on-LAN to {} (is_ps5: {})", host, is_ps5);

    uint64_t credential = 0;
    std::string auth_clean = rp_auth;
    if (auth_clean.size() == 16) {
        std::string ascii_str;
        for (size_t i = 0; i < 16; i += 2) {
            std::string byte_hex = auth_clean.substr(i, 2);
            ascii_str.push_back(static_cast<char>(std::stoul(byte_hex, nullptr, 16)));
        }
        try {
            credential = std::stoull(ascii_str, nullptr, 16);
        } catch (...) {
            credential = 0;
        }
    } else if (!auth_clean.empty()) {
        try {
            credential = std::stoull(auth_clean, nullptr, 16);
        } catch (...) {
            credential = 0;
        }
    }

    if (credential == 0) {
        credential = 2062813029ULL; // Default 7af40765 in decimal
    }

    const char* proto_ver = is_ps5 ? "00030010" : "00020020";
    std::string payload = std::format(
        "WAKEUP * HTTP/1.1\n"
        "client-type:vr\n"
        "auth-type:R\n"
        "model:w\n"
        "app-type:r\n"
        "user-credential:{}\n"
        "device-discovery-protocol-version:{}\n\n",
        credential, proto_ver
    );

    portal::net::UDPSocket sock;
    (void)sock.bind(0);
    (void)sock.set_broadcast(true);
    std::span<const uint8_t> span_payload(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

    uint16_t primary_port = is_ps5 ? 9302 : 987;
    // Unicast to target host
    (void)sock.send_to(host, primary_port, span_payload);
    (void)sock.send_to(host, 987, span_payload);
    (void)sock.send_to(host, 997, span_payload);
    (void)sock.send_to(host, 9295, span_payload);

    // Broadcast on local subnet
    (void)sock.send_to("255.255.255.255", primary_port, span_payload);
    (void)sock.send_to("255.255.255.255", 987, span_payload);

    return {};
}

VoidResult DDPDiscovery::wake(const DiscoveredConsole& console) {
    bool is_ps5 = (console.host_type == "PS5" || console.host_type.find("5") != std::string::npos);
    return wake(console.address, "", is_ps5);
}

} // namespace portal::discovery
