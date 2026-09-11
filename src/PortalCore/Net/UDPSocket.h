// Archivo: src/PortalCore/Net/UDPSocket.h
#pragma once
#include "PortalCore/Common.h"
#include <span>
#include <string>
#include <tuple>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace portal::net {

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;

    Result<void> bind(uint16_t port);
    Result<void> send_to(const std::string& address, uint16_t port, std::span<const uint8_t> data);
    Result<std::tuple<ByteBuffer, std::string, uint16_t>> receive_from();
    
    Result<void> set_recv_buffer(int size);
    Result<void> set_send_buffer(int size);
    Result<void> set_broadcast(bool enable);
    Result<void> set_nonblocking(bool enable);
    
    void close();

    SOCKET get_handle() const { return sock_; }

private:
    SOCKET sock_ = INVALID_SOCKET;
};

} // namespace portal::net
