// Archivo: src/PortalCore/Net/TCPSocket.h
#pragma once
#include "PortalCore/Common.h"
#include <span>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>

namespace portal::net {

class TCPSocket {
public:
    TCPSocket();
    ~TCPSocket();

    TCPSocket(const TCPSocket&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;

    Result<void> connect(const std::string& host, uint16_t port, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    Result<void> send_all(std::span<const uint8_t> data);
    Result<ByteBuffer> receive(size_t max_bytes);
    Result<ByteBuffer> receive_exact(size_t n_bytes);
    
    void close();

    SOCKET get_handle() const { return sock_; }

private:
    SOCKET sock_ = INVALID_SOCKET;
};

} // namespace portal::net
