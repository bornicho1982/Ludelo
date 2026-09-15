#include "LudeloCore/Crypto/SecureRandom.h"
#include <spdlog/spdlog.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace ludelo::crypto {

Result<void> SecureRandom::fill_random(std::span<uint8_t> buffer) {
    if (buffer.empty()) return {};

    NTSTATUS status = BCryptGenRandom(
        nullptr,
        buffer.data(),
        static_cast<ULONG>(buffer.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    if (status != 0) {
        spdlog::get("ludelo")->error("BCryptGenRandom failed with status 0x{:08X}", status);
        return std::unexpected(ErrorCode::CryptoError);
    }

    return {};
}

Result<ByteBuffer> SecureRandom::random_bytes(size_t size) {
    ByteBuffer buffer(size);
    if (auto res = fill_random(buffer); !res) {
        return std::unexpected(res.error());
    }
    return buffer;
}

Result<uint32_t> SecureRandom::random_uint32() {
    uint32_t val = 0;
    if (auto res = fill_random(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&val), sizeof(val))); !res) {
        return std::unexpected(res.error());
    }
    return val;
}

Result<uint64_t> SecureRandom::random_uint64() {
    uint64_t val = 0;
    if (auto res = fill_random(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&val), sizeof(val))); !res) {
        return std::unexpected(res.error());
    }
    return val;
}

} // namespace ludelo::crypto
