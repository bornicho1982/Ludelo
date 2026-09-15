#include "LudeloCore/Net/TLSSocket.h"
#include <spdlog/spdlog.h>

namespace ludelo::net {

TLSSocket::TLSSocket() {
    ctx_ = SSL_CTX_new(TLS_client_method());
    if (!ctx_) {
        spdlog::get("ludelo")->error("Failed to create SSL context");
    }
    SSL_CTX_set_default_verify_paths(ctx_);
}

TLSSocket::~TLSSocket() {
    close();
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
}

void TLSSocket::close() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    tcp_socket_.close();
}

Result<void> TLSSocket::connect(const std::string& host, uint16_t port, std::chrono::milliseconds timeout) {
    host_ = host;
    if (auto res = tcp_socket_.connect(host, port, timeout); !res) {
        return res;
    }

    ssl_ = SSL_new(ctx_);
    if (!ssl_) {
        return std::unexpected(Error{ErrorCode::NetworkError});
    }

    SSL_set_fd(ssl_, static_cast<int>(tcp_socket_.get_handle()));
    SSL_set_tlsext_host_name(ssl_, host.c_str());

    return handshake();
}

Result<void> TLSSocket::handshake() {
    if (!ssl_) return std::unexpected(Error{ErrorCode::NetworkError});

    if (SSL_connect(ssl_) != 1) {
        spdlog::get("ludelo")->error("SSL connect failed for {}", host_);
        return std::unexpected(Error{ErrorCode::NetworkError});
    }
    return {};
}

Result<void> TLSSocket::send_all(std::span<const uint8_t> data) {
    if (!ssl_) return std::unexpected(Error{ErrorCode::NetworkError});

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        int sent = SSL_write(ssl_, data.data() + total_sent, static_cast<int>(data.size() - total_sent));
        if (sent <= 0) {
            spdlog::get("ludelo")->error("SSL write failed");
            return std::unexpected(Error{ErrorCode::NetworkError});
        }
        total_sent += sent;
    }
    return {};
}

Result<ByteBuffer> TLSSocket::receive(size_t max_bytes) {
    if (!ssl_) return std::unexpected(Error{ErrorCode::NetworkError});

    ByteBuffer buffer(max_bytes);
    int received = SSL_read(ssl_, buffer.data(), static_cast<int>(max_bytes));
    
    if (received <= 0) {
        int err = SSL_get_error(ssl_, received);
        if (err == SSL_ERROR_ZERO_RETURN) {
            return std::unexpected(Error{ErrorCode::NetworkClosed});
        }
        spdlog::get("ludelo")->error("SSL read failed");
        return std::unexpected(Error{ErrorCode::NetworkError});
    }

    buffer.resize(received);
    return buffer;
}

} // namespace ludelo::net
