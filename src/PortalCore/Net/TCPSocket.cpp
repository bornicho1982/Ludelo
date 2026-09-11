#include "PortalCore/Net/TCPSocket.h"
#include <spdlog/spdlog.h>

namespace portal::net {

TCPSocket::TCPSocket() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

TCPSocket::~TCPSocket() {
    close();
    WSACleanup();
}

void TCPSocket::close() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
}

Result<void> TCPSocket::connect(const std::string& host, uint16_t port, std::chrono::milliseconds timeout) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        spdlog::get("portal")->error("TCP getaddrinfo failed for {}", host);
        return std::unexpected(Error{ErrorCode::NetworkError});
    }

    // Set non-blocking for connect timeout
    u_long mode = 1;
    ioctlsocket(sock_, FIONBIO, &mode);

    ::connect(sock_, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);

    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock_, &set);

    timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    if (select(0, nullptr, &set, nullptr, &tv) <= 0) {
        spdlog::get("portal")->error("TCP connect timeout to {}:{}", host, port);
        return std::unexpected(Error{ErrorCode::NetworkTimeout});
    }

    // Restore blocking
    mode = 0;
    ioctlsocket(sock_, FIONBIO, &mode);

    return {};
}

Result<void> TCPSocket::send_all(std::span<const uint8_t> data) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        int sent = ::send(sock_, reinterpret_cast<const char*>(data.data() + total_sent), 
                          static_cast<int>(data.size() - total_sent), 0);
        if (sent == SOCKET_ERROR) {
            spdlog::get("portal")->error("TCP send failed: {}", WSAGetLastError());
            return std::unexpected(Error{ErrorCode::NetworkError});
        }
        total_sent += sent;
    }
    return {};
}

Result<ByteBuffer> TCPSocket::receive(size_t max_bytes) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    ByteBuffer buffer(max_bytes);
    int received = ::recv(sock_, reinterpret_cast<char*>(buffer.data()), static_cast<int>(max_bytes), 0);
    
    if (received == 0) {
        return std::unexpected(ErrorCode::NetworkClosed);
    } else if (received == SOCKET_ERROR) {
        spdlog::get("portal")->error("TCP recv failed: {}", WSAGetLastError());
        return std::unexpected(Error{ErrorCode::NetworkError});
    }

    buffer.resize(received);
    return buffer;
}

Result<ByteBuffer> TCPSocket::receive_exact(size_t n_bytes) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    ByteBuffer buffer(n_bytes);
    size_t total_received = 0;

    while (total_received < n_bytes) {
        int received = ::recv(sock_, reinterpret_cast<char*>(buffer.data() + total_received), 
                              static_cast<int>(n_bytes - total_received), 0);
        
        if (received == 0) {
            return std::unexpected(ErrorCode::NetworkClosed);
        } else if (received == SOCKET_ERROR) {
            spdlog::get("portal")->error("TCP recv_exact failed: {}", WSAGetLastError());
            return std::unexpected(Error{ErrorCode::NetworkError});
        }
        total_received += received;
    }
    
    return buffer;
}

} // namespace portal::net
