// Archivo: src/PortalCore/Crypto/PS5Protocol.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Net/TCPSocket.h"
#include <string>
#include <vector>
#include <array>
#include <span>

namespace portal::crypto {

struct PS5RegistrationResult {
    std::string host_name;
    std::string host_id;
    std::string regist_key;
    portal::ByteBuffer rp_key;
    std::string mac;
};

struct PS5SessionInitResult {
    portal::ByteBuffer nonce;
    std::string rp_version;
};

struct PS5SessionKeys {
    std::array<uint8_t, 16> bright{};
    std::array<uint8_t, 16> ambassador{};
};

class PS5Protocol {
public:
    // 8-digit PIN registration against PS5 (/sie/ps5/rp/sess/rgst on TCP port 9295)
    static Result<PS5RegistrationResult> register_with_pin(
        const std::string& host,
        uint16_t port,
        uint32_t pin,
        const std::string& account_id_b64 = "2IFPSJ3ALmE="
    );

    // Session init (/sie/ps5/rp/sess/init on TCP port 9295)
    static Result<PS5SessionInitResult> init_session(
        const std::string& host,
        uint16_t port,
        const std::string& regist_key
    );

    // Key derivation from session nonce and paired rp_key
    static PS5SessionKeys derive_session_keys(
        const portal::ByteBuffer& nonce,
        const portal::ByteBuffer& rp_key
    );

    // Session control handshake (/sie/ps5/rp/sess/ctrl on TCP port 9295)
    static Result<void> ctrl_session(
        net::TCPSocket& tcp,
        const std::string& host,
        uint16_t port,
        const std::string& regist_key,
        const PS5SessionKeys& keys,
        VideoCodec codec,
        bool hdr
    );

    // AES-128-CFB helper methods
    static std::array<uint8_t, 16> generate_iv(
        const std::array<uint8_t, 16>& ambassador,
        uint64_t counter
    );

    static Result<portal::ByteBuffer> aes_cfb_crypt(
        const std::array<uint8_t, 16>& key,
        const std::array<uint8_t, 16>& iv,
        std::span<const uint8_t> input,
        bool encrypt
    );
};

} // namespace portal::crypto
