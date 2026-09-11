// Archivo: src/PortalCore/Config/AppSettings.h
// Ludelo — Persistent application settings with JSON serialization
#pragma once

#include "PortalCore/Common.h"
#include <filesystem>
#include <string>

namespace portal::config {

struct AppSettings {
    // ─── Stream Quality ──────────────────────────────────
    Resolution  resolution    = Resolution::R1080p;
    FrameRate   fps           = FrameRate::FPS60;
    VideoCodec  codec         = VideoCodec::H265;
    bool        hdr           = false;
    uint32_t    bitrate_kbps  = 35000;   // 35 Mbps (LAN default)
    bool        fec_enabled   = true;
    uint32_t    audio_buffer_ms = 20;

    // ─── Display ─────────────────────────────────────────
    bool vsync      = true;
    bool fullscreen = false;
    int  window_width  = 1280;
    int  window_height = 720;

    // ─── Console ─────────────────────────────────────────
    std::string preferred_console_host_id;  // host_id of the last used console

    // ─── Input ───────────────────────────────────────────
    bool    haptics_enabled     = true;
    bool    adaptive_triggers   = true;
    float   stick_deadzone      = 0.08f;  // 8% deadzone

    // ─── PSN Login PIN (convenience) ─────────────────────
    bool    remember_login_pin  = true;

    // ─── UI ──────────────────────────────────────────────
    bool    mica_enabled        = true;   // Windows 11 Mica backdrop
    int     ui_language         = 0;      // 0=ES, 1=EN, 2=FR, 3=DE

    // ─── Serialization ────────────────────────────────────
    /// Load settings from JSON file. Returns {} (void) even if file doesn't exist (uses defaults).
    Result<void> load(const std::filesystem::path& path);

    /// Save settings to JSON file. Creates parent directories if needed.
    Result<void> save(const std::filesystem::path& path) const;

    /// Default settings file location inside app_data_dir
    static std::filesystem::path default_path(const std::filesystem::path& app_data_dir) {
        return app_data_dir / "config" / "settings.json";
    }
};

} // namespace portal::config
