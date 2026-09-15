// Archivo: src/LudeloCore/Crypto/SecureRandom.h
#pragma once
#include "LudeloCore/Common.h"
#include <span>
#include <cstdint>

namespace ludelo::crypto {

class SecureRandom {
public:
    static Result<void> fill_random(std::span<uint8_t> buffer);
    static Result<ByteBuffer> random_bytes(size_t size);
    static Result<uint32_t> random_uint32();
    static Result<uint64_t> random_uint64();
};

} // namespace ludelo::crypto
