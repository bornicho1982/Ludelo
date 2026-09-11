// Archivo: src/PortalCore/Net/TLSSocket.h
#pragma once
#include "PortalCore/Net/TCPSocket.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace portal::net {

class TLSSocket {
public:
    TLSSocket();
    ~TLSSocket();

    TLSSocket(const TLSSocket&) = delete;
    TLSSocket& operator=(const TLSSocket&) = delete;

    Result<void> connect(const std::string& host, uint16_t port, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    Result<void> handshake();
    Result<void> send_all(std::span<const uint8_t> data);
    Result<ByteBuffer> receive(size_t max_bytes);
    
    void close();

private:
    TCPSocket tcp_socket_;
    SSL_CTX* ctx_ = nullptr;
    SSL* ssl_ = nullptr;
    std::string host_;
};

} // namespace portal::net
