// Archivo: src/LudeloCore/Stream/RUDPConnection.h
#pragma once

#include "LudeloCore/Common.h"
#include "LudeloCore/Net/UDPSocket.h"
#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <condition_variable>

namespace ludelo::stream {

    enum class RUDPMessageType : uint8_t {
        REGIST = 1,
        SESSION_INIT = 2,
        CTRL_HTTP = 3
    };

    class RUDPConnection {
    public:
        RUDPConnection(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port);
        ~RUDPConnection();

        Result<ByteBuffer> handshake(RUDPMessageType type, const ByteBuffer& payload);
        Result<void> send_message(RUDPMessageType type, const ByteBuffer& payload);

    private:
        void listen_loop(std::stop_token stoken);
        
        std::shared_ptr<net::UDPSocket> m_socket;
        std::string m_remote_addr;
        uint16_t m_remote_port;
        
        std::jthread m_listener_thread;
        
        std::atomic<uint16_t> m_sequence{1};
        
        std::mutex m_mutex;
        std::condition_variable m_cv;
        
        bool m_has_response{false};
        ByteBuffer m_response_data;
        uint16_t m_expected_ack{0};
    };
}
