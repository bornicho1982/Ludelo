// Archivo: src/PortalCore/Crypto/ECDHKeyExchange.h
#pragma once
#include "PortalCore/Common.h"
#include <span>
#include <openssl/evp.h>

namespace portal::crypto {

class ECDHKeyExchange {
public:
    ECDHKeyExchange();
    ~ECDHKeyExchange();

    ECDHKeyExchange(const ECDHKeyExchange&) = delete;
    ECDHKeyExchange& operator=(const ECDHKeyExchange&) = delete;

    Result<void> generate_keypair();
    Result<ByteBuffer> get_public_key() const;
    Result<ByteBuffer> compute_shared_secret(std::span<const uint8_t> peer_public_key);
    
    static Result<ByteBuffer> derive_session_keys(std::span<const uint8_t> shared_secret, std::span<const uint8_t> salt, std::span<const uint8_t> info, size_t out_len);

private:
    EVP_PKEY* keypair_ = nullptr;
};

} // namespace portal::crypto
