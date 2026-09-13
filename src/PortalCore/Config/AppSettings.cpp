// Archivo: src/PortalCore/Config/AppSettings.cpp
#include "AppSettings.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>

namespace portal::config {

// ─── Helper macros for JSON round-trip ────────────────────

#define JS_LOAD(j, key, field) \
    if ((j).contains(#key)) (field) = (j)[#key].get<decltype(field)>()

#define JS_LOAD_CAST(j, key, field, T) \
    if ((j).contains(#key)) (field) = static_cast<decltype(field)>((j)[#key].get<T>())

#define JS_SAVE(j, field) \
    (j)[#field] = (field)

// ─── load ─────────────────────────────────────────────────
Result<void> AppSettings::load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        // No settings file yet — use defaults silently
        return {};
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::warn("[AppSettings] Cannot open settings file: {}", path.string());
        return {};
    }

    try {
        auto j = nlohmann::json::parse(file);

        // Stream Quality
        JS_LOAD_CAST(j, resolution,     resolution,   uint16_t);
        JS_LOAD_CAST(j, fps,            fps,           uint8_t);
        JS_LOAD_CAST(j, codec,          codec,         uint8_t);
        JS_LOAD(j, hdr,                 hdr);
        JS_LOAD(j, bitrate_kbps,        bitrate_kbps);
        JS_LOAD(j, fec_enabled,         fec_enabled);
        JS_LOAD(j, audio_buffer_ms,     audio_buffer_ms);

        // Display
        JS_LOAD(j, vsync,               vsync);
        JS_LOAD(j, fullscreen,          fullscreen);
        JS_LOAD(j, window_width,        window_width);
        JS_LOAD(j, window_height,       window_height);

        // Console
        JS_LOAD(j, preferred_console_host_id, preferred_console_host_id);

        // Input
        JS_LOAD(j, haptics_enabled,     haptics_enabled);
        JS_LOAD(j, adaptive_triggers,   adaptive_triggers);
        JS_LOAD(j, stick_deadzone,      stick_deadzone);
        JS_LOAD(j, enable_gyro,         enable_gyro);
        JS_LOAD(j, remember_login_pin,  remember_login_pin);

        // UI
        JS_LOAD(j, mica_enabled,        mica_enabled);
        JS_LOAD(j, ui_language,         ui_language);

        spdlog::info("[AppSettings] Settings loaded from: {}", path.string());
    } catch (const std::exception& e) {
        spdlog::warn("[AppSettings] Failed to parse settings (using defaults): {}", e.what());
        // Reset to defaults on parse failure
        *this = AppSettings{};
    }

    return {};
}

// ─── save ─────────────────────────────────────────────────
Result<void> AppSettings::save(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        return std::unexpected(Error{ErrorCode::Unknown,
            std::format("Cannot create settings directory: {}", ec.message())});
    }

    try {
        nlohmann::json j;

        // Stream Quality
        j["resolution"]         = static_cast<uint16_t>(resolution);
        j["fps"]                = static_cast<uint8_t>(fps);
        j["codec"]              = static_cast<uint8_t>(codec);
        JS_SAVE(j, hdr);
        JS_SAVE(j, bitrate_kbps);
        JS_SAVE(j, fec_enabled);
        JS_SAVE(j, audio_buffer_ms);

        // Display
        JS_SAVE(j, vsync);
        JS_SAVE(j, fullscreen);
        JS_SAVE(j, window_width);
        JS_SAVE(j, window_height);

        // Console
        JS_SAVE(j, preferred_console_host_id);

        // Input
        JS_SAVE(j, haptics_enabled);
        JS_SAVE(j, adaptive_triggers);
        JS_SAVE(j, stick_deadzone);
        JS_SAVE(j, enable_gyro);
        JS_SAVE(j, remember_login_pin);

        // UI
        JS_SAVE(j, mica_enabled);
        JS_SAVE(j, ui_language);

        std::ofstream file(path);
        if (!file.is_open()) {
            return std::unexpected(Error{ErrorCode::Unknown,
                std::format("Cannot write settings file: {}", path.string())});
        }
        file << j.dump(2);
        spdlog::debug("[AppSettings] Settings saved to: {}", path.string());

    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::Unknown, e.what()});
    }

    return {};
}

} // namespace portal::config
