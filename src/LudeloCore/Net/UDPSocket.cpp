// Archivo: src/LudeloCore/Net/UDPSocket.cpp
#include "UDPSocket.h"
#include <spdlog/spdlog.h>

namespace ludelo::net {

UDPSocket::UDPSocket() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == INVALID_SOCKET) {
        spdlog::get("ludelo")->error("Failed to create UDP socket: {}", WSAGetLastError());
    }
}

UDPSocket::~UDPSocket() {
    close();
    WSACleanup();
}

void UDPSocket::close() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
}

Result<void> UDPSocket::bind(uint16_t port) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(ErrorCode::NetworkError);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        spdlog::get("ludelo")->error("UDP bind failed: {}", WSAGetLastError());
        return std::unexpected(ErrorCode::NetworkError);
    }
    return {};
}

Result<void> UDPSocket::send_to(const std::string& address, uint16_t port, std::span<const uint8_t> data) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &dest.sin_addr);

    int sent = sendto(sock_, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()), 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    if (sent == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return std::unexpected(Error{ErrorCode::NetworkWouldBlock});
        }
        spdlog::get("ludelo")->error("UDP sendto failed: {}", WSAGetLastError());
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

Result<std::tuple<ByteBuffer, std::string, uint16_t>> UDPSocket::receive_from() {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});

    ByteBuffer buffer(65536);
    sockaddr_in sender{};
    int sender_len = sizeof(sender);

    int received = recvfrom(sock_, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&sender), &sender_len);
    
    if (received == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return std::unexpected(Error{ErrorCode::NetworkWouldBlock});
        }
        spdlog::get("ludelo")->error("UDP recvfrom failed: {}", WSAGetLastError());
        return std::unexpected(Error{ErrorCode::NetworkError});
    }

    buffer.resize(received);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sender.sin_addr, ip_str, INET_ADDRSTRLEN);
    
    return std::make_tuple(std::move(buffer), std::string(ip_str), ntohs(sender.sin_port));
}

Result<void> UDPSocket::set_recv_buffer(int size) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof(size)) == SOCKET_ERROR) {
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

Result<void> UDPSocket::set_send_buffer(int size) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&size), sizeof(size)) == SOCKET_ERROR) {
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

Result<void> UDPSocket::set_broadcast(bool enable) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});
    int opt = enable ? 1 : 0;
    if (setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&opt), sizeof(opt)) == SOCKET_ERROR) {
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

Result<void> UDPSocket::set_nonblocking(bool enable) {
    if (sock_ == INVALID_SOCKET) return std::unexpected(Error{ErrorCode::NetworkError});
    u_long mode = enable ? 1 : 0;
    if (ioctlsocket(sock_, FIONBIO, &mode) == SOCKET_ERROR) {
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

} // namespace ludelo::net
