// Archivo: src/LudeloCore/Crypto/RPCrypt.h
#pragma once
#include "LudeloCore/Common.h"
#include <span>
#include <openssl/evp.h>

namespace ludelo::crypto {

class RPCrypt {
public:
    RPCrypt();
    ~RPCrypt();

    RPCrypt(const RPCrypt&) = delete;
    RPCrypt& operator=(const RPCrypt&) = delete;

    Result<void> set_keys(std::span<const uint8_t> key, std::span<const uint8_t> iv);

    // AES-128-GCM for PS5
    Result<ByteBuffer> encrypt_packet_gcm(std::span<const uint8_t> plaintext, std::span<const uint8_t> aad);
    Result<ByteBuffer> decrypt_packet_gcm(std::span<const uint8_t> ciphertext, std::span<const uint8_t> tag, std::span<const uint8_t> aad);

    // AES-128-CBC for PS4
    Result<ByteBuffer> encrypt_packet_cbc(std::span<const uint8_t> plaintext);
    Result<ByteBuffer> decrypt_packet_cbc(std::span<const uint8_t> ciphertext);

    // GMAC for authentication
    Result<ByteBuffer> compute_gmac(std::span<const uint8_t> data);

private:
    void increment_iv();

    ByteBuffer key_;
    ByteBuffer iv_;
};

} // namespace ludelo::crypto
