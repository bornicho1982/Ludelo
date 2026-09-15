// Archivo: src/LudeloCore/Auth/Keychain.cpp
#include "LudeloCore/Auth/Keychain.h"
#include <spdlog/spdlog.h>
#include <dpapi.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")

namespace ludelo::auth {

Result<ludelo::ByteBuffer> Keychain::encrypt(const ludelo::ByteBuffer& plaintext) {
    DATA_BLOB in;
    in.pbData = const_cast<BYTE*>(plaintext.data());
    in.cbData = static_cast<DWORD>(plaintext.size());

    DATA_BLOB out;
    if (CryptProtectData(&in, L"Ludelo Keychain", nullptr, nullptr, nullptr, 0, &out)) {
        ludelo::ByteBuffer res(out.pbData, out.pbData + out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return std::unexpected(ludelo::Error{ErrorCode::CryptoError, "CryptProtectData failed"});
}

Result<ludelo::ByteBuffer> Keychain::decrypt(const ludelo::ByteBuffer& ciphertext) {
    DATA_BLOB in;
    in.pbData = const_cast<BYTE*>(ciphertext.data());
    in.cbData = static_cast<DWORD>(ciphertext.size());

    DATA_BLOB out;
    if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
        ludelo::ByteBuffer res(out.pbData, out.pbData + out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return std::unexpected(ludelo::Error{ErrorCode::CryptoError, "CryptUnprotectData failed"});
}

static std::filesystem::path get_storage_path() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::filesystem::path full_path = std::filesystem::path(path) / "Ludelo" / "keychain";
        std::filesystem::create_directories(full_path);
        return full_path;
    }
    return std::filesystem::current_path() / "keychain";
}

Result<void> Keychain::store_secret(const std::string& key_name, const ludelo::ByteBuffer& data) {
    auto file_path = get_storage_path() / key_name;
    
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        spdlog::error("Failed to open file for writing: {}", file_path.string());
        return std::unexpected(ludelo::Error{ErrorCode::Unknown, "File write error"});
    }
    
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    return {};
}

Result<ludelo::ByteBuffer> Keychain::load_secret(const std::string& key_name) {
    auto file_path = get_storage_path() / key_name;
    
    std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
    if (!ifs) {
        return std::unexpected(ludelo::Error{ErrorCode::Unknown, "Secret not found"});
    }
    
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    
    ludelo::ByteBuffer buffer(size);
    if (ifs.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return std::unexpected(ludelo::Error{ErrorCode::Unknown, "File read error"});
}

} // namespace ludelo::auth
