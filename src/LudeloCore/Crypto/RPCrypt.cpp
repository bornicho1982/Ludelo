#include "LudeloCore/Crypto/RPCrypt.h"
#include <openssl/err.h>
#include <spdlog/spdlog.h>

namespace ludelo::crypto {

RPCrypt::RPCrypt() = default;

RPCrypt::~RPCrypt() = default;

Result<void> RPCrypt::set_keys(std::span<const uint8_t> key, std::span<const uint8_t> iv) {
    if (key.size() != 16 || iv.size() != 16) {
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    key_.assign(key.begin(), key.end());
    iv_.assign(iv.begin(), iv.end());
    return {};
}

void RPCrypt::increment_iv() {
    for (int i = 15; i >= 0; --i) {
        if (++iv_[i] != 0) break;
    }
}

Result<ByteBuffer> RPCrypt::encrypt_packet_gcm(std::span<const uint8_t> plaintext, std::span<const uint8_t> aad) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    int len;
    int ciphertext_len;
    ByteBuffer out(plaintext.size() + 16); // Reserve space for tag at the end

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (!aad.empty()) {
        if (EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), aad.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::unexpected(Error{ErrorCode::CryptoError});
        }
    }

    if (EVP_EncryptUpdate(ctx, out.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, out.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    ciphertext_len += len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, out.data() + ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    
    out.resize(ciphertext_len + 16);
    EVP_CIPHER_CTX_free(ctx);
    increment_iv();

    return out;
}

Result<ByteBuffer> RPCrypt::decrypt_packet_gcm(std::span<const uint8_t> ciphertext, std::span<const uint8_t> tag, std::span<const uint8_t> aad) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    int len;
    int plaintext_len;
    ByteBuffer out(ciphertext.size());

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (!aad.empty()) {
        if (EVP_DecryptUpdate(ctx, nullptr, &len, aad.data(), aad.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::unexpected(Error{ErrorCode::CryptoError});
        }
    }

    if (EVP_DecryptUpdate(ctx, out.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    plaintext_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    int ret = EVP_DecryptFinal_ex(ctx, out.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);
    increment_iv();

    if (ret > 0) {
        plaintext_len += len;
        out.resize(plaintext_len);
        return out;
    } else {
        char err_buf[256]{};
        ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
        spdlog::error("GCM Decrypt failed authentication: {}", err_buf);
        return std::unexpected(Error{ErrorCode::CryptoError, err_buf});
    }
}

Result<ByteBuffer> RPCrypt::encrypt_packet_cbc(std::span<const uint8_t> plaintext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    int len;
    int ciphertext_len;
    ByteBuffer out(plaintext.size() + 16); // padding

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_EncryptUpdate(ctx, out.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, out.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    ciphertext_len += len;

    out.resize(ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);
    increment_iv();
    return out;
}

Result<ByteBuffer> RPCrypt::decrypt_packet_cbc(std::span<const uint8_t> ciphertext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    int len;
    int plaintext_len;
    ByteBuffer out(ciphertext.size());

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_DecryptUpdate(ctx, out.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, out.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }
    plaintext_len += len;

    out.resize(plaintext_len);
    EVP_CIPHER_CTX_free(ctx);
    increment_iv();
    return out;
}

Result<ByteBuffer> RPCrypt::compute_gmac(std::span<const uint8_t> data) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(Error{ErrorCode::CryptoError});

    int len;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_EncryptUpdate(ctx, nullptr, &len, data.data(), data.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    if (EVP_EncryptFinal_ex(ctx, nullptr, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    ByteBuffer tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(Error{ErrorCode::CryptoError});
    }

    EVP_CIPHER_CTX_free(ctx);
    increment_iv();
    return tag;
}

} // namespace ludelo::crypto
