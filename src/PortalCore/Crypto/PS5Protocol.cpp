// Archivo: src/PortalCore/Crypto/PS5Protocol.cpp
#include "PS5Protocol.h"
#include "PS5CryptoTables.h"
#include "PortalCore/Crypto/SecureRandom.h"
#include <chiaki/common.h>
#include <chiaki/regist.h>
#include <chiaki/base64.h>
#include <chiaki/log.h>
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <format>
#include <sstream>
#include <algorithm>
#include <future>
#include <chrono>
#include <mutex>

namespace portal::crypto {

static std::string base64_encode(std::span<const uint8_t> data) {
    static const char* kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kAlphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(kAlphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::vector<uint8_t> base64_decode(const std::string& in) {
    std::vector<uint8_t> out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    }
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string encode_hex(std::span<const uint8_t> bytes) {
    std::string out;
    for (uint8_t b : bytes) {
        out += std::format("{:02x}", b);
    }
    return out;
}

static portal::ByteBuffer decode_hex(std::string_view hex) {
    portal::ByteBuffer out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        std::string s(hex.substr(i, 2));
        out.push_back(static_cast<uint8_t>(strtol(s.c_str(), nullptr, 16)));
    }
    return out;
}

std::array<uint8_t, 16> PS5Protocol::generate_iv(
    const std::array<uint8_t, 16>& ambassador,
    uint64_t counter
) {
    uint8_t buf[24];
    std::memcpy(buf, ambassador.data(), 16);
    for (int i = 0; i < 8; ++i) {
        buf[16 + i] = static_cast<uint8_t>((counter >> ((7 - i) * 8)) & 0xFF);
    }

    uint8_t hmac[32];
    unsigned int hmac_len = 0;
    HMAC(EVP_sha256(), hmac_key_ps5, 16, buf, sizeof(buf), hmac, &hmac_len);

    std::array<uint8_t, 16> iv{};
    std::memcpy(iv.data(), hmac, 16);
    return iv;
}

Result<portal::ByteBuffer> PS5Protocol::aes_cfb_crypt(
    const std::array<uint8_t, 16>& key,
    const std::array<uint8_t, 16>& iv,
    std::span<const uint8_t> input,
    bool encrypt
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::unexpected(ErrorCode::CryptoError);

    int res = 0;
    if (encrypt) {
        res = EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb128(), nullptr, key.data(), iv.data());
    } else {
        res = EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb128(), nullptr, key.data(), iv.data());
    }

    if (res != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::unexpected(ErrorCode::CryptoError);
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    portal::ByteBuffer output(input.size());
    int out_len = 0;

    if (encrypt) {
        res = EVP_EncryptUpdate(ctx, output.data(), &out_len, input.data(), static_cast<int>(input.size()));
    } else {
        res = EVP_DecryptUpdate(ctx, output.data(), &out_len, input.data(), static_cast<int>(input.size()));
    }

    EVP_CIPHER_CTX_free(ctx);

    if (res != 1 || out_len != static_cast<int>(input.size())) {
        return std::unexpected(ErrorCode::AESFailed);
    }

    return output;
}

Result<PS5RegistrationResult> PS5Protocol::register_with_pin(
    const std::string& host,
    uint16_t port,
    uint32_t pin,
    const std::string& account_id_b64
) {
    (void)port;
    auto logger = spdlog::get("portal");
    if (logger) logger->info("[PS5Protocol] Registering with PS5 at {} using Chiaki Core regist (PIN: {})", host, pin);

    static std::once_flag s_chiaki_init_flag;
    std::call_once(s_chiaki_init_flag, []() {
        chiaki_lib_init();
    });

    ChiakiRegistInfo info{};
    info.target = CHIAKI_TARGET_PS5_1;
    info.host = host.c_str();
    info.broadcast = false;
    info.pin = pin;
    info.console_pin = 0;
    info.holepunch_info = nullptr;
    info.rudp = nullptr;
    info.psn_online_id = nullptr;

    // Decode 8-byte PSN Account-ID from base64
    size_t out_len = sizeof(info.psn_account_id);
    ChiakiErrorCode b64_err = chiaki_base64_decode(account_id_b64.c_str(), account_id_b64.size(), info.psn_account_id, &out_len);
    if (b64_err != CHIAKI_ERR_SUCCESS || out_len != CHIAKI_PSN_ACCOUNT_ID_SIZE) {
        if (logger) logger->error("[PS5Protocol] Failed to decode 8-byte PSN Account-ID from base64: '{}'", account_id_b64);
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Invalid PSN Account-ID (expected 8 bytes base64)"});
    }

    ChiakiLog chiaki_log;
    chiaki_log_init(&chiaki_log, CHIAKI_LOG_ALL, [](ChiakiLogLevel level, const char* msg, void*) {
        auto log = spdlog::get("portal");
        if (!log) return;
        if (level & CHIAKI_LOG_ERROR) log->error("[ChiakiRegist] {}", msg);
        else if (level & CHIAKI_LOG_WARNING) log->warn("[ChiakiRegist] {}", msg);
        else if (level & CHIAKI_LOG_INFO) log->info("[ChiakiRegist] {}", msg);
        else log->debug("[ChiakiRegist] {}", msg);
    }, nullptr);

    struct RegistResult {
        ChiakiRegistEventType type = CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED;
        ChiakiRegisteredHost registered_host{};
    };

    std::promise<RegistResult> promise;
    auto future = promise.get_future();

    auto cb = [](ChiakiRegistEvent* event, void* user) {
        auto* prom = static_cast<std::promise<RegistResult>*>(user);
        RegistResult res;
        if (event) {
            res.type = event->type;
            if (event->type == CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS && event->registered_host) {
                res.registered_host = *event->registered_host;
            }
        }
        prom->set_value(res);
    };

    ChiakiRegist regist{};
    ChiakiErrorCode start_err = chiaki_regist_start(&regist, &chiaki_log, &info, cb, &promise);
    if (start_err != CHIAKI_ERR_SUCCESS) {
        if (logger) logger->error("[PS5Protocol] chiaki_regist_start failed with code: {}", static_cast<int>(start_err));
        return std::unexpected(Error{ErrorCode::RegistrationError, "Failed to start Chiaki regist"});
    }

    // Wait for registration to complete (timeout after 25 seconds)
    if (future.wait_for(std::chrono::seconds(25)) == std::future_status::timeout) {
        chiaki_regist_stop(&regist);
        chiaki_regist_fini(&regist);
        if (logger) logger->error("[PS5Protocol] Chiaki regist timed out after 25s");
        return std::unexpected(Error{ErrorCode::NetworkTimeout, "Regist timed out waiting for console response"});
    }

    RegistResult result = future.get();
    chiaki_regist_fini(&regist);

    if (result.type != CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS) {
        if (logger) logger->error("[PS5Protocol] Chiaki regist failed (event type: {})", static_cast<int>(result.type));
        return std::unexpected(Error{ErrorCode::RegistrationRefused, "Registration failed or rejected by console"});
    }

    PS5RegistrationResult reg_res;
    if (result.registered_host.server_nickname[0] != '\0') {
        reg_res.host_name = result.registered_host.server_nickname;
    } else {
        reg_res.host_name = "PlayStation 5";
    }

    reg_res.mac = std::format("{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
        result.registered_host.server_mac[0], result.registered_host.server_mac[1],
        result.registered_host.server_mac[2], result.registered_host.server_mac[3],
        result.registered_host.server_mac[4], result.registered_host.server_mac[5]);
    reg_res.host_id = reg_res.mac;

    reg_res.regist_key = std::string(result.registered_host.rp_regist_key, 16);
    reg_res.rp_key.assign(result.registered_host.rp_key, result.registered_host.rp_key + 16);

    if (logger) {
        logger->info("[PS5Protocol] Registration succeeded via Chiaki Core! Host: {}, MAC: {}",
            reg_res.host_name, portal::mask_secret(reg_res.mac));
    }

    return reg_res;
}

Result<PS5SessionInitResult> PS5Protocol::init_session(
    const std::string& host,
    uint16_t port,
    const std::string& regist_key
) {
    auto logger = spdlog::get("portal");
    if (logger) logger->info("PS5Protocol: Initiating session with {}:{} using regist_key {}", host, port, portal::mask_secret(regist_key));

    net::TCPSocket tcp;
    auto conn_res = tcp.connect(host, port, std::chrono::milliseconds(5000));
    if (!conn_res) return std::unexpected(conn_res.error());

    std::string req = std::format(
        "GET /sie/ps5/rp/sess/init HTTP/1.1\r\n"
        "Host: {}:{}\r\n"
        "User-Agent: remoteplay Windows\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n"
        "RP-Registkey: {}\r\n"
        "Rp-Version: 1.0\r\n\r\n",
        host, port, regist_key
    );

    auto send_res = tcp.send_all(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(req.data()), req.size()));
    if (!send_res) return std::unexpected(send_res.error());

    auto resp_res = tcp.receive(4096);
    if (!resp_res || resp_res->empty()) return std::unexpected(ErrorCode::NetworkTimeout);

    std::string resp(reinterpret_cast<const char*>(resp_res->data()), resp_res->size());
    if (resp.find("200") == std::string::npos) {
        if (logger) logger->error("PS5 session init failed:\n{}", resp);
        std::string err_msg = "Error al iniciar sesion en PS5";
        if (resp.find("80108b15") != std::string::npos) {
            err_msg = "PS5 reporta error 80108b15 (Servicio colapsado). Reinicia Uso a distancia en Ajustes > Sistema de la PS5.";
        } else if (resp.find("80108b10") != std::string::npos) {
            err_msg = "La consola ya esta en uso por otro usuario (80108b10).";
        } else if (resp.find("403") != std::string::npos) {
            err_msg = "Acceso denegado por la consola PS5 (403 Forbidden).";
        }
        return std::unexpected(Error{ErrorCode::SessionError, err_msg});
    }

    std::string nonce_b64;
    std::string rp_version = "1.0";
    std::istringstream iss(resp);
    std::string line;
    while (std::getline(iss, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        auto k = line.substr(0, colon);
        auto v = line.substr(colon + 1);
        while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(0, 1);
        while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();

        if (_stricmp(k.c_str(), "RP-Nonce") == 0) {
            nonce_b64 = v;
        } else if (_stricmp(k.c_str(), "RP-Version") == 0) {
            rp_version = v;
        }
    }

    if (nonce_b64.empty()) return std::unexpected(ErrorCode::HandshakeFailed);

    PS5SessionInitResult init_res;
    init_res.nonce = base64_decode(nonce_b64);
    init_res.rp_version = rp_version;
    return init_res;
}

PS5SessionKeys PS5Protocol::derive_session_keys(
    const portal::ByteBuffer& nonce,
    const portal::ByteBuffer& rp_key
) {
    PS5SessionKeys keys{};
    const uint8_t* key_a = &keys_a_ps5[(nonce[0] >> 3) * 0x70];
    for (size_t i = 0; i < 16; ++i) {
        uint8_t v = static_cast<uint8_t>(nonce[i] - 0x2d - i);
        keys.ambassador[i] = v ^ key_a[i];
    }

    const uint8_t* key_b = &keys_b_ps5[(nonce[7] >> 3) * 0x70];
    for (size_t i = 0; i < 16; ++i) {
        uint8_t v = static_cast<uint8_t>(rp_key[i] + 0x18 + i);
        keys.bright[i] = v ^ nonce[i] ^ key_b[i];
    }

    return keys;
}

Result<void> PS5Protocol::ctrl_session(
    net::TCPSocket& tcp,
    const std::string& host,
    uint16_t port,
    const std::string& regist_key,
    const PS5SessionKeys& keys,
    VideoCodec codec,
    bool hdr
) {
    auto logger = spdlog::get("portal");
    if (logger) logger->info("PS5Protocol: Sending GET /sie/ps5/rp/sess/ctrl to {}:{}", host, port);

    uint64_t counter_local = 0;

    // 1. RP-Auth
    auto raw_key = decode_hex(regist_key);
    std::array<uint8_t, 16> auth_plain{};
    std::memcpy(auth_plain.data(), raw_key.data(), std::min<size_t>(16, raw_key.size()));
    auto iv_auth = generate_iv(keys.ambassador, counter_local++);
    auto enc_auth = aes_cfb_crypt(keys.bright, iv_auth, auth_plain, true);
    if (!enc_auth) return std::unexpected(enc_auth.error());
    std::string auth_b64 = base64_encode(*enc_auth);

    // 2. RP-Did (32 bytes)
    uint8_t did_raw[32] = {
        0x00, 0x18, 0x00, 0x00, 0x00, 0x07, 0x00, 0x40, 0x00, 0x80,
        0x73, 0x9f, 0x6b, 0x04, 0x18, 0x25, 0x90, 0xb4, 0x73, 0x7b, 0x43, 0x39, 0x1a, 0x39, 0xec, 0xa5,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    auto iv_did = generate_iv(keys.ambassador, counter_local++);
    auto enc_did = aes_cfb_crypt(keys.bright, iv_did, did_raw, true);
    if (!enc_did) return std::unexpected(enc_did.error());
    std::string did_b64 = base64_encode(*enc_did);

    // 3. RP-OSType ("Win10.0.0\0")
    std::string ostype_str = "Win10.0.0";
    std::vector<uint8_t> ostype_raw(ostype_str.begin(), ostype_str.end());
    ostype_raw.push_back(0);
    auto iv_os = generate_iv(keys.ambassador, counter_local++);
    auto enc_os = aes_cfb_crypt(keys.bright, iv_os, ostype_raw, true);
    if (!enc_os) return std::unexpected(enc_os.error());
    std::string ostype_b64 = base64_encode(*enc_os);

    // 4. RP-StartBitrate (4 bytes: 0)
    uint8_t bitrate_raw[4] = {0, 0, 0, 0};
    auto iv_br = generate_iv(keys.ambassador, counter_local++);
    auto enc_br = aes_cfb_crypt(keys.bright, iv_br, bitrate_raw, true);
    if (!enc_br) return std::unexpected(enc_br.error());
    std::string bitrate_b64 = base64_encode(*enc_br);

    // 5. RP-StreamingType: 2 for H265, 3 for H265 HDR, 1 for H264
    uint32_t st_val = (codec == VideoCodec::H265) ? (hdr ? 3 : 2) : 1;
    uint8_t st_raw[4] = {
        static_cast<uint8_t>(st_val & 0xFF),
        static_cast<uint8_t>((st_val >> 8) & 0xFF),
        static_cast<uint8_t>((st_val >> 16) & 0xFF),
        static_cast<uint8_t>((st_val >> 24) & 0xFF)
    };
    auto iv_st = generate_iv(keys.ambassador, counter_local++);
    auto enc_st = aes_cfb_crypt(keys.bright, iv_st, st_raw, true);
    if (!enc_st) return std::unexpected(enc_st.error());
    std::string st_b64 = base64_encode(*enc_st);

    std::string req = std::format(
        "GET /sie/ps5/rp/sess/ctrl HTTP/1.1\r\n"
        "Host: {}:{}\r\n"
        "User-Agent: remoteplay Windows\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 0\r\n"
        "RP-Auth: {}\r\n"
        "RP-Version: 1.0\r\n"
        "RP-Did: {}\r\n"
        "RP-ControllerType: 3\r\n"
        "RP-ClientType: 11\r\n"
        "RP-OSType: {}\r\n"
        "RP-ConPath: 1\r\n"
        "RP-StartBitrate: {}\r\n"
        "RP-StreamingType: {}\r\n\r\n",
        host, port, auth_b64, did_b64, ostype_b64, bitrate_b64, st_b64
    );

    auto send_res = tcp.send_all(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(req.data()), req.size()));
    if (!send_res) return std::unexpected(send_res.error());

    auto resp_res = tcp.receive(2048);
    if (!resp_res || resp_res->empty()) return std::unexpected(ErrorCode::NetworkTimeout);

    std::string resp(reinterpret_cast<const char*>(resp_res->data()), resp_res->size());
    if (resp.find("200 OK") == std::string::npos) {
        if (logger) logger->error("PS5 ctrl handshake failed:\n{}", resp);
        return std::unexpected(ErrorCode::SessionError);
    }

    if (logger) logger->info("PS5 ctrl handshake succeeded with HTTP 200 OK!");
    return {};
}

} // namespace portal::crypto
