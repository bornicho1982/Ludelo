// Archivo: src/LudeloCore/Auth/Keychain.h
#pragma once

#include "LudeloCore/Common.h"
#include <string>

namespace ludelo::auth {

class Keychain {
public:
    static Result<ludelo::ByteBuffer> encrypt(const ludelo::ByteBuffer& plaintext);
    static Result<ludelo::ByteBuffer> decrypt(const ludelo::ByteBuffer& ciphertext);
    
    static Result<void> store_secret(const std::string& key_name, const ludelo::ByteBuffer& data);
    static Result<ludelo::ByteBuffer> load_secret(const std::string& key_name);
};

} // namespace ludelo::auth
