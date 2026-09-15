#include "LudeloCore/Crypto/ECDHKeyExchange.h"
#include <openssl/ec.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/err.h>
#include <spdlog/spdlog.h>

namespace ludelo::crypto {

ECDHKeyExchange::ECDHKeyExchange() = default;

ECDHKeyExchange::~ECDHKeyExchange() {
    if (keypair_) {
        EVP_PKEY_free(keypair_);
        keypair_ = nullptr;
    }
}

Result<void> ECDHKeyExchange::generate_keypair() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (!ctx) {
        spdlog::get("ludelo")->error("Failed to create EVP_PKEY_CTX");
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_PKEY_keygen(ctx, &keypair_) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    EVP_PKEY_CTX_free(ctx);
    return {};
}

Result<ByteBuffer> ECDHKeyExchange::get_public_key() const {
    if (!keypair_) return std::unexpected(Error{ErrorCode::CryptoError});

    size_t len = 0;
    if (EVP_PKEY_get_octet_string_param(keypair_, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &len) <= 0) {
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    ByteBuffer pub_key(len);
    if (EVP_PKEY_get_octet_string_param(keypair_, OSSL_PKEY_PARAM_PUB_KEY, pub_key.data(), len, &len) <= 0) {
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    return pub_key;
}

Result<ByteBuffer> ECDHKeyExchange::compute_shared_secret(std::span<const uint8_t> peer_public_key) {
    if (!keypair_) return std::unexpected(Error{ErrorCode::CryptoError});

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    if (EVP_PKEY_fromdata_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, const_cast<char*>("prime256v1"), 0),
        OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY, const_cast<uint8_t*>(peer_public_key.data()), peer_public_key.size()),
        OSSL_PARAM_construct_end()
    };

    EVP_PKEY* peer_key = nullptr;
    if (EVP_PKEY_fromdata(ctx, &peer_key, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    EVP_PKEY_CTX_free(ctx);

    EVP_PKEY_CTX* derive_ctx = EVP_PKEY_CTX_new(keypair_, nullptr);
    if (!derive_ctx) {
        EVP_PKEY_free(peer_key);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_PKEY_derive_init(derive_ctx) <= 0 || EVP_PKEY_derive_set_peer(derive_ctx, peer_key) <= 0) {
        EVP_PKEY_CTX_free(derive_ctx);
        EVP_PKEY_free(peer_key);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    size_t secret_len = 0;
    if (EVP_PKEY_derive(derive_ctx, nullptr, &secret_len) <= 0) {
        EVP_PKEY_CTX_free(derive_ctx);
        EVP_PKEY_free(peer_key);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    ByteBuffer secret(secret_len);
    if (EVP_PKEY_derive(derive_ctx, secret.data(), &secret_len) <= 0) {
        EVP_PKEY_CTX_free(derive_ctx);
        EVP_PKEY_free(peer_key);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    EVP_PKEY_CTX_free(derive_ctx);
    EVP_PKEY_free(peer_key);
    return secret;
}

Result<ByteBuffer> ECDHKeyExchange::derive_session_keys(std::span<const uint8_t> shared_secret, std::span<const uint8_t> salt, std::span<const uint8_t> info, size_t out_len) {
    EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "HKDF", nullptr);
    if (!kdf) return std::unexpected(Error{ErrorCode::CryptoError});

    EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (!kctx) return std::unexpected(Error{ErrorCode::CryptoError});

    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, const_cast<char*>("SHA256"), 0),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, const_cast<uint8_t*>(shared_secret.data()), shared_secret.size()),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, const_cast<uint8_t*>(salt.data()), salt.size()),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, const_cast<uint8_t*>(info.data()), info.size()),
        OSSL_PARAM_construct_end()
    };

    ByteBuffer out_key(out_len);
    if (EVP_KDF_derive(kctx, out_key.data(), out_len, params) <= 0) {
        EVP_KDF_CTX_free(kctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    EVP_KDF_CTX_free(kctx);
    return out_key;
}

} // namespace ludelo::crypto
