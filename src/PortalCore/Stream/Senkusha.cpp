// Archivo: src/PortalCore/Stream/Senkusha.cpp
#include "Senkusha.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace portal::stream {

    Result<SenkushaResult> Senkusha::run(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port) {
        SenkushaResult result;
        result.mtu_in = 1454;
        result.mtu_out = 1454;
        result.rtt_us = 1000; // default 1ms (aligned with Chiaki LAN session fallback)
        
        spdlog::get("portal")->info("Running Senkusha QoS negotiation to {}:{}", remote_addr, remote_port);

        // A basic RTT measurement
        ByteBuffer ping(32, 0);
        ping[0] = 0xFF; // Dummy ping type
        
        auto start = std::chrono::high_resolution_clock::now();
        socket->send_to(remote_addr, remote_port, ping);
        
        // Wait for pong with short timeout
        for (int i = 0; i < 10; ++i) {
            auto recv_res = socket->receive_from();
            if (recv_res.has_value()) {
                auto end = std::chrono::high_resolution_clock::now();
                result.rtt_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        spdlog::get("portal")->info("Senkusha finished: MTU={}/{}, RTT={}us", result.mtu_in, result.mtu_out, result.rtt_us);
        
        return result;
    }
}
