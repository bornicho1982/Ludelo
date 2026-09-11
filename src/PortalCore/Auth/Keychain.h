// Archivo: src/PortalCore/Auth/Keychain.h
#pragma once

#include "PortalCore/Common.h"
#include <string>

namespace portal::auth {

class Keychain {
public:
    static Result<portal::ByteBuffer> encrypt(const portal::ByteBuffer& plaintext);
    static Result<portal::ByteBuffer> decrypt(const portal::ByteBuffer& ciphertext);
    
    static Result<void> store_secret(const std::string& key_name, const portal::ByteBuffer& data);
    static Result<portal::ByteBuffer> load_secret(const std::string& key_name);
};

} // namespace portal::auth
