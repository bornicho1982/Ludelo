// Archivo: src/main.cpp
// Ludelo — Entry Point (Windows WinMain)

#include "PortalCore/Common.h"
#include "UI/App.h"
#include "Platform/WindowsWindow.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include "PortalCore/Stream/SessionManager.h"
#include "PortalCore/Discovery/ConsoleRegistry.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <shellapi.h>
#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")
#endif

namespace {

std::filesystem::path get_app_data_dir() {
    #ifdef _WIN32
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result / "Ludelo";
    }
    #endif
    return std::filesystem::current_path() / "data";
}

bool ensure_directories(const std::filesystem::path& app_data) {
    std::error_code ec;
    std::filesystem::create_directories(app_data / "keychain", ec);
    std::filesystem::create_directories(app_data / "cache" / "avatars", ec);
    std::filesystem::create_directories(app_data / "cache" / "covers", ec);
    std::filesystem::create_directories(app_data / "logs", ec);
    std::filesystem::create_directories(app_data / "config", ec);
    return !ec;
}

#ifdef _WIN32
struct WinsockInit {
    WinsockInit() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    ~WinsockInit() {
        WSACleanup();
    }
};

LONG WINAPI UnhandledCrashFilter(EXCEPTION_POINTERS* ep) {
    if (spdlog::default_logger()) {
        spdlog::critical("FATAL CRASH! ExceptionCode: 0x{:08X}, Address: {}", 
            ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
        spdlog::default_logger()->flush();
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetUnhandledExceptionFilter(UnhandledCrashFilter);
    #endif

    // ── Initialize Winsock ────────────────────────────────
    #ifdef _WIN32
    WinsockInit winsock;
    #endif

    // ── Setup data directories ────────────────────────────
    auto app_data = get_app_data_dir();
    ensure_directories(app_data);

    // ── Initialize logging ────────────────────────────────
    portal::init_logging(spdlog::level::debug, (app_data / "logs" / "ludelo.log").string());
    if (!std::filesystem::exists(app_data / "logs" / "ludelo.log")) {
        // Will show error in UI later, for now just try our best.
    }
    spdlog::info("╔══════════════════════════════════════════╗");
    spdlog::info("║  Ludelo v{}                         ║", portal::kVersion);
    spdlog::info("║  PlayStation Remote Play Client          ║");
    spdlog::info("╚══════════════════════════════════════════╝");
    spdlog::info("Data directory: {}", app_data.string());

    // ── Initialize SDL3 ───────────────────────────────────
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        spdlog::critical("SDL_Init failed: {}", SDL_GetError());
        return EXIT_FAILURE;
    }
    spdlog::info("SDL3 initialized successfully");

    // ── Check CLI test arguments ─────────────────────────
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-stream" || std::string(argv[i]) == "--stream") {
            spdlog::info("Running in headless stream test mode...");
            auto sm = std::make_shared<portal::stream::SessionManager>();

            // Load the first registered console from disk — no hardcoded IPs
            auto reg = std::make_shared<portal::discovery::ConsoleRegistry>();
            portal::discovery::RegisteredConsole console;
            bool console_found = false;
            if (reg->load_from_disk()) {
                auto all = reg->get_all_consoles();
                if (!all.empty()) {
                    console = all.front();
                    console_found = true;
                    spdlog::info("Loaded registered console: {} @ {} (rp_auth len={}, rp_key len={})",
                        console.host_name, console.address, console.rp_auth.size(), console.rp_key.size());
                }
            }
            if (!console_found) {
                spdlog::error("[TEST] No registered console found. Please register a console first.");
                SDL_Quit();
                return EXIT_FAILURE;
            }

            std::atomic<bool> connected{false};
            std::atomic<int> video_frames{0};
            std::atomic<int> audio_frames{0};

            sm->on_state_change = [&](portal::stream::SessionState state) {
                spdlog::info("[TEST] SessionState changed: state={}", static_cast<int>(state));
                if (state == portal::stream::SessionState::Streaming) {
                    connected = true;
                }
            };
            sm->on_video_frame = [&](portal::stream::DecodedFrame& frame) {
                int count = ++video_frames;
                if (count == 1 || count % 30 == 0) {
                    spdlog::info("[TEST] Received Decoded Video Frame #{}: {}x{}", 
                        count, frame.width, frame.height);
                }
            };
            sm->on_audio_frame = [&](portal::stream::AudioFrame& audio) {
                int count = ++audio_frames;
                if (count == 1 || count % 100 == 0) {
                    spdlog::info("[TEST] Received Decoded Audio Frame #{}: {} samples, {} Hz, {} ch", 
                        count, audio.samples.size(), audio.sample_rate, audio.channels);
                }
            };

            portal::StreamConfig cfg;
            cfg.resolution = portal::Resolution::R1080p;
            cfg.fps = portal::FrameRate::FPS60;
            cfg.codec = portal::VideoCodec::H265;
            cfg.hdr = false;

            auto res = sm->connect_local(console, cfg);
            if (!res) {
                spdlog::error("[TEST] connect_local failed immediately: {}", res.error().message);
                SDL_Quit();
                return EXIT_FAILURE;
            }

            spdlog::info("[TEST] Session started. Waiting for connection and video frames (15 seconds)...");
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < 15) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (connected.load() && video_frames.load() >= 30) {
                    spdlog::info("[TEST] Successfully validated live streaming pipeline! {} video frames, {} audio frames received.",
                        video_frames.load(), audio_frames.load());
                    break;
                }
            }

            spdlog::info("[TEST] Disconnecting test session...");
            sm->disconnect();
            spdlog::info("[TEST] Done. Total video frames: {}, audio frames: {}", video_frames.load(), audio_frames.load());
            SDL_Quit();
            return (connected.load() && video_frames.load() > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    // ── Create and run the application ────────────────────
    try {
        portal::ui::AppConfig config{
            .app_data_dir = app_data,
            .window_title = "Ludelo — PlayStation Remote Play",
            .window_width = 1280,
            .window_height = 720,
            .fullscreen = false,
            .vsync = true,
        };

        portal::ui::App app(config);

        auto init_result = app.init();
        if (!init_result) {
            spdlog::critical("App initialization failed: {}", init_result.error().to_string());
            SDL_Quit();
            return EXIT_FAILURE;
        }

        spdlog::info("Application initialized, entering main loop");
        app.run();

        spdlog::info("Application exiting normally");
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Quit();
    spdlog::info("Goodbye!");
    return EXIT_SUCCESS;
}
