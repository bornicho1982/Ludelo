// Archivo: src/LudeloCore/Stream/RUDPConnection.cpp
#include "RUDPConnection.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace ludelo::stream {

    RUDPConnection::RUDPConnection(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port)
        : m_socket(std::move(socket)), m_remote_addr(remote_addr), m_remote_port(remote_port) {
        m_listener_thread = std::jthread([this](std::stop_token stoken) { listen_loop(std::move(stoken)); });
    }

    RUDPConnection::~RUDPConnection() {
        if (m_listener_thread.joinable()) {
            m_listener_thread.request_stop();
        }
    }

    Result<ByteBuffer> RUDPConnection::handshake(RUDPMessageType type, const ByteBuffer& payload) {
        uint16_t seq = m_sequence++;
        m_expected_ack = seq;
        
        ByteBuffer packet(4 + payload.size());
        packet[0] = static_cast<uint8_t>(type);
        packet[1] = 0; // Flags
        packet[2] = (seq >> 8) & 0xFF;
        packet[3] = seq & 0xFF;
        
        std::copy(payload.begin(), payload.end(), packet.begin() + 4);

        int max_retries = 5;
        int timeout_ms = 100;
        
        for (int i = 0; i < max_retries; ++i) {
            m_socket->send_to(m_remote_addr, m_remote_port, packet);
            
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return m_has_response; })) {
                m_has_response = false;
                return m_response_data;
            }
            
            timeout_ms *= 2; // Exponential backoff
        }
        
        return std::unexpected(ErrorCode::Timeout);
    }

    Result<void> RUDPConnection::send_message(RUDPMessageType type, const ByteBuffer& payload) {
        uint16_t seq = m_sequence++;
        ByteBuffer packet(4 + payload.size());
        packet[0] = static_cast<uint8_t>(type);
        packet[1] = 0; // Flags
        packet[2] = (seq >> 8) & 0xFF;
        packet[3] = seq & 0xFF;
        std::copy(payload.begin(), payload.end(), packet.begin() + 4);
        
        m_socket->send_to(m_remote_addr, m_remote_port, packet);
        return {};
    }

    void RUDPConnection::listen_loop(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            auto result = m_socket->receive_from();
            if (result.has_value()) {
                const auto& [buffer, ip, port] = result.value();
                if (buffer.size() >= 4) {
                    uint16_t ack = (buffer[2] << 8) | buffer[3];
                    
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (ack == m_expected_ack) {
                        m_response_data = ByteBuffer(buffer.begin() + 4, buffer.end());
                        m_has_response = true;
                        m_cv.notify_one();
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }
}
