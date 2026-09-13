// Archivo: src/PortalCore/Common.h
#pragma once

// ─── Standard Library ─────────────────────────────────────
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

// ─── Platform ─────────────────────────────────────────────
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #include <Windows.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// ─── Third Party ──────────────────────────────────────────
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <nlohmann/json.hpp>

// ─── Version ──────────────────────────────────────────────
#ifndef PORTAL_VERSION
#define PORTAL_VERSION "0.1.0"
#endif
#ifndef PORTAL_USER_AGENT
#define PORTAL_USER_AGENT "Ludelo/0.1.0"
#endif

namespace portal {

inline constexpr std::string_view kVersion = PORTAL_VERSION;
inline constexpr std::string_view kUserAgent = PORTAL_USER_AGENT;
inline constexpr std::string_view kAppName = "Ludelo";

// ─── Error Type ───────────────────────────────────────────
enum class ErrorCode : uint32_t {
    // General
    Ok = 0,
    Unknown,
    InvalidArgument,
    InvalidParameter,
    Timeout,
    Cancelled,
    NotInitialized,
    AlreadyInitialized,
    InvalidState,
    JsonParseError,
    OutOfMemory,

    // Network
    NetworkError = 100,
    ConnectionRefused,
    ConnectionReset,
    ConnectionTimeout,
    HostUnreachable,
    DNSResolutionFailed,
    TLSHandshakeFailed,
    WebSocketError,
    NetworkWouldBlock,
    NetworkClosed,
    NetworkTimeout,

    // Auth
    AuthError = 200,
    InvalidCredentials,
    TokenExpired,
    TokenRefreshFailed,
    AccountNotFound,
    NpssoInvalid,
    TwoFactorRequired,
    NoActiveAccount,

    // Discovery
    DiscoveryError = 300,
    ConsoleNotFound,
    ConsoleInStandby,
    ConsoleBusy,
    WakeUpFailed,

    // Registration
    RegistrationError = 400,
    InvalidPIN,
    RegistrationRefused,
    TooManyDevices,
    RegistrationTimeout,

    // Session
    SessionError = 500,
    SessionCreationFailed,
    SessionDenied,
    SessionExpired,
    LoginPINRequired,
    HandshakeFailed,

    // Streaming
    StreamError = 600,
    DecodeError,
    DecoderInitFailed,
    DecoderError,
    FECRecoveryFailed,
    AudioInitFailed,
    TakionError,
    UnsupportedCodec,
    NoFrame,

    // Cloud
    CloudError = 700,
    CloudNotAvailable,
    CloudEntitlementFailed,
    CloudSessionFailed,
    NoPSPlusPremium,

    // Crypto
    CryptoError = 800,
    ECDHFailed,
    AESFailed,
    HMACFailed,
    KeyDerivationFailed,

    // Input
    InputError = 900,
    ControllerNotFound,
    HIDOpenFailed,
    HIDWriteFailed,
};

struct Error {
    ErrorCode code{ErrorCode::Unknown};
    std::string message;

    Error() = default;
    Error(ErrorCode c, std::string msg = "") : code(c), message(std::move(msg)) {}

    [[nodiscard]] bool ok() const { return code == ErrorCode::Ok; }
    [[nodiscard]] explicit operator bool() const { return !ok(); }

    bool operator==(const Error& other) const = default;

    [[nodiscard]] std::string to_string() const {
        return std::format("[Error {}] {}", static_cast<uint32_t>(code), message);
    }
};

template<typename T>
using Result = std::expected<T, Error>;

using VoidResult = std::expected<void, Error>;

// ─── Byte Buffer ──────────────────────────────────────────
using ByteBuffer = std::vector<uint8_t>;

// ─── Time Helpers ─────────────────────────────────────────
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Microseconds = std::chrono::microseconds;

// ─── Console Types ────────────────────────────────────────
enum class ConsoleType : uint8_t {
    Unknown = 0,
    PS4 = 4,
    PS5 = 5,
};

enum class ConsoleState : uint8_t {
    Unknown = 0,
    Standby,
    Awake,
    Busy,
};

// ─── Stream Configuration ─────────────────────────────────
enum class VideoCodec : uint8_t {
    H264 = 0,
    H265 = 1,
};

enum class Resolution : uint16_t {
    R360p  = 360,
    R540p  = 540,
    R720p  = 720,
    R1080p = 1080,
    R1440p = 1440,
    R2160p = 2160,
};

enum class FrameRate : uint8_t {
    FPS30 = 30,
    FPS60 = 60,
    FPS120 = 120,
};

struct StreamConfig {
    Resolution resolution = Resolution::R1080p;
    FrameRate  fps = FrameRate::FPS60;
    VideoCodec codec = VideoCodec::H265;
    bool       hdr = false;
    uint32_t   bitrate_kbps = 15000;  // 15 Mbps default
    bool       fec_enabled = true;
    uint32_t   audio_buffer_ms = 20;
};

// ─── PSN Constants ────────────────────────────────────────
namespace psn {

inline constexpr std::string_view kAccountHost = "ca.account.sony.com";
inline constexpr std::string_view kAuthHost = "auth.api.sonyentertainmentnetwork.com";
inline constexpr std::string_view kNpssoCapturePath = "/npsso-capture";
inline constexpr std::string_view kSSOCookiePath = "/api/v1/ssocookie";
inline constexpr std::string_view kTokenEndpoint = "/2.0/oauth/token";

// PS Remote Play discovery
inline constexpr uint16_t kPS4DiscoveryPort = 987;
inline constexpr uint16_t kPS5DiscoveryPort = 9302;
inline constexpr uint16_t kPS4SessionPort = 9295;
inline constexpr uint16_t kPS5SessionPort = 9303;

// Protocol versions
inline constexpr std::string_view kPS4DiscoveryVersion = "00020020";
inline constexpr std::string_view kPS5DiscoveryVersion = "00030010";
inline constexpr std::string_view kPS4RPVersion = "9.0";
inline constexpr std::string_view kPS5RPVersion = "10.0";

// Cloud (Gaikai/Kamaji)
inline constexpr std::string_view kCloudPlatformId = "pc:psnow";
inline constexpr std::string_view kKamajiBasePath = "/kamaji/api/pcnow/00_09_000";
inline constexpr std::string_view kCloudOrigin = "https://psnow.playstation.com";

}  // namespace psn

// ─── Logging Initialization ──────────────────────────────
inline void init_logging(spdlog::level::level_enum level = spdlog::level::info, const std::string& log_file = "ludelo.log") {
    if (auto existing = spdlog::get("portal")) {
        existing->set_level(level);
        return;
    }
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, 5 * 1024 * 1024, 3);
    file_sink->set_level(spdlog::level::debug);

    auto logger = std::make_shared<spdlog::logger>(
        "portal", spdlog::sinks_init_list{console_sink, file_sink});
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    spdlog::set_default_logger(logger);
}

// ─── Log Hygiene / Obfuscation ───────────────────────────
/// Obfuscates sensitive strings (keys, tokens, nonces, MACs, BSSIDs)
/// Keeps first 4 and last 4 chars, replacing the middle with "****".
/// If length <= 8, returns "****".
inline std::string mask_secret(std::string_view s) {
    if (s.size() <= 8) return "****";
    return std::string(s.substr(0, 4)) + "****" + std::string(s.substr(s.size() - 4));
}

inline std::string mask_secret(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return "<empty>";
    std::string hex;
    for (uint8_t b : bytes) {
        hex += std::format("{:02x}", b);
    }
    return mask_secret(hex);
}

inline std::string mask_secret(const uint8_t* data, size_t len) {
    if (!data || len == 0) return "<empty>";
    std::string hex;
    for (size_t i = 0; i < len; ++i) {
        hex += std::format("{:02x}", data[i]);
    }
    return mask_secret(hex);
}

namespace auth {}
namespace discovery {}
namespace stream {}
namespace input {}
namespace crypto {}
namespace net {}
namespace cloud {}

}  // namespace portal
