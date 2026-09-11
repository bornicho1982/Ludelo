// Archivo: src/PortalCore/Stream/Senkusha.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Net/UDPSocket.h"
#include <memory>
#include <string>
#include <cstdint>

namespace portal::stream {

    struct SenkushaResult {
        uint32_t mtu_in;
        uint32_t mtu_out;
        uint32_t rtt_us;
    };

    class Senkusha {
    public:
        Senkusha() = default;
        
        Result<SenkushaResult> run(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port);
    };
}
