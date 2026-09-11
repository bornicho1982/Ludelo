// Archivo: src/UI/App.cpp
// Ludelo — Main Application Implementation (ImGui + Vulkan)
#include "UI/App.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <array>
#include <set>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <format>
#include <cmath>
#include <ctime>
#include "PortalCore/Discovery/DDPDiscovery.h"
#include "PortalCore/Crypto/PS5Protocol.h"
#include "Platform/WindowsWindow.h"
#include "PortalCore/Config/AppSettings.h"

namespace portal::ui {

// ─── Stitch Design System: Midnight Portal Color Palette (#0b1021, #0070d1, #00f0ff) ──
namespace colors {
    // Midnight Portal Core (#0b1021, #0070d1, #00f0ff)
    constexpr ImVec4 kBackground       = {0.043f, 0.063f, 0.129f, 1.00f};  // #0b1021 Deep Void Navy
    constexpr ImVec4 kSurface          = {0.055f, 0.082f, 0.165f, 0.85f};  // #0e152a Glass Surface
    constexpr ImVec4 kSurfaceHover     = {0.080f, 0.120f, 0.240f, 0.90f};  // Lighter Glass Hover
    constexpr ImVec4 kSurfaceActive    = {0.100f, 0.150f, 0.300f, 0.98f};  // Glass Active/Focused
    constexpr ImVec4 kGlassCard        = {0.065f, 0.095f, 0.190f, 0.75f};  // Translucent Card
    constexpr ImVec4 kGlassBorder      = {0.180f, 0.250f, 0.450f, 0.40f};  // Subtle Glass Edge
    
    constexpr ImVec4 kPrimary          = {0.000f, 0.439f, 0.820f, 1.00f};  // #0070d1 PlayStation Blue
    constexpr ImVec4 kPrimaryHover     = {0.080f, 0.520f, 0.920f, 1.00f};  // Vibrant PS Blue Hover
    constexpr ImVec4 kAccent           = {0.000f, 0.941f, 1.000f, 1.00f};  // #00f0ff Electric Neon Cyan
    constexpr ImVec4 kAccentHover      = {0.250f, 0.970f, 1.000f, 1.00f};  
    constexpr ImVec4 kAccentGlow       = {0.000f, 0.941f, 1.000f, 0.35f};  // Cyan Glow
    
    constexpr ImVec4 kTextPrimary      = {0.960f, 0.970f, 1.000f, 1.00f};  // Crisp White/Ice
    constexpr ImVec4 kTextSecondary    = {0.550f, 0.620f, 0.730f, 1.00f};  // Slate Grey
    constexpr ImVec4 kTextMuted        = {0.350f, 0.400f, 0.500f, 1.00f};  // Deep Slate
    
    constexpr ImVec4 kSuccess          = {0.000f, 0.902f, 0.463f, 1.00f};  // #00e676 (Awake/Encendida)
    constexpr ImVec4 kWarning          = {1.000f, 0.671f, 0.000f, 1.00f};  // #ffab00 (Standby/Reposo)
    constexpr ImVec4 kError            = {1.000f, 0.200f, 0.400f, 1.00f};  // #ff3366
    
    constexpr ImVec4 kPS5Blue          = {0.000f, 0.439f, 0.820f, 1.00f};  // #0070d1
    constexpr ImVec4 kCloudPurple      = {0.475f, 0.157f, 0.792f, 1.00f};  // #7928ca PS Plus Cloud
    constexpr ImVec4 kCloudPurpleGlow  = {0.475f, 0.157f, 0.792f, 0.35f};

    // PlayStation Controller Button Colors
    constexpr ImVec4 kBtnCross         = {0.000f, 0.850f, 1.000f, 1.00f};  // Cyan
    constexpr ImVec4 kBtnCircle        = {1.000f, 0.280f, 0.340f, 1.00f};  // Coral Red
    constexpr ImVec4 kBtnTriangle      = {0.180f, 0.835f, 0.451f, 1.00f};  // Emerald Green
    constexpr ImVec4 kBtnSquare        = {1.000f, 0.420f, 0.506f, 1.00f};  // Pink/Magenta
}

// ─── Constructor / Destructor ─────────────────────────────

App::App(const AppConfig& config)
    : config_(config)
{
}

App::~App() {
    // Persist settings on exit
    if (!settings_path_.empty()) {
        (void)settings_.save(settings_path_);
    }
    if (audio_stream_) {
        SDL_DestroyAudioStream(audio_stream_);
        audio_stream_ = nullptr;
    }
    cleanup_imgui();
    cleanup_vulkan();
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

// ─── Initialization ──────────────────────────────────────

VoidResult App::init() {
    spdlog::info("Initializing Ludelo application...");
    if (spdlog::default_logger()) spdlog::default_logger()->flush();

    // Create SDL window with Vulkan support
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config_.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    window_ = SDL_CreateWindow(
        config_.window_title.c_str(),
        static_cast<int>(config_.window_width),
        static_cast<int>(config_.window_height),
        flags
    );

    if (!window_) {
        spdlog::critical("Failed to create SDL window: {}", SDL_GetError());
        return std::unexpected(Error{ErrorCode::Unknown,
            std::format("Failed to create SDL window: {}", SDL_GetError())});
    }

    spdlog::info("SDL window created: {}x{}", config_.window_width, config_.window_height);

    // Initialize Vulkan
    auto vk_result = init_vulkan();
    if (!vk_result) {
        spdlog::critical("Vulkan init failed: {}", vk_result.error().to_string());
        return vk_result;
    }

    // Initialize ImGui
    auto imgui_result = init_imgui();
    if (!imgui_result) {
        spdlog::critical("ImGui init failed: {}", imgui_result.error().to_string());
        if (spdlog::default_logger()) spdlog::default_logger()->flush();
        return imgui_result;
    }

    // Initialize Backend Managers
    account_manager_ = std::make_shared<portal::auth::AccountManager>();
    console_registry_ = std::make_shared<portal::discovery::ConsoleRegistry>();
    controller_manager_ = std::make_shared<portal::input::ControllerManager>();
    session_manager_ = std::make_shared<portal::stream::SessionManager>();

    // Load persistent settings
    settings_path_ = portal::config::AppSettings::default_path(config_.app_data_dir);
    (void)settings_.load(settings_path_);
    spdlog::info("[App] Settings loaded: resolution={}, fps={}, bitrate={} kbps",
        static_cast<int>(settings_.resolution),
        static_cast<int>(settings_.fps),
        settings_.bitrate_kbps);

    // Attempt to load saved data
    (void)account_manager_->load_from_disk();
    (void)console_registry_->load_from_disk();
    (void)controller_manager_->init();

    // Hook login PIN requested callback (when console challenges for user passcode)
    session_manager_->on_login_pin_requested = [this]() {
        show_login_pin_modal_ = true;
        active_login_pin_digit_ = 0;
        memset(login_pin_digits_, 0, sizeof(login_pin_digits_));
    };

    // Initial console discovery probe
    probe_consoles_background();

    // High-frequency (250Hz) Gamepad and Keyboard Input Poller
    session_manager_->set_input_poll_callback([this]() -> portal::stream::ControllerState {
        if (!controller_manager_) return {};
        auto in = controller_manager_->poll();
        portal::stream::ControllerState out{};
        if (in.cross)        out.buttons |= (1 << 0);  // CROSS
        if (in.circle)       out.buttons |= (1 << 1);  // MOON / CIRCLE
        if (in.square)       out.buttons |= (1 << 2);  // BOX / SQUARE
        if (in.triangle)     out.buttons |= (1 << 3);  // PYRAMID / TRIANGLE
        if (in.dpad_left)    out.buttons |= (1 << 4);  // DPAD_LEFT
        if (in.dpad_right)   out.buttons |= (1 << 5);  // DPAD_RIGHT
        if (in.dpad_up)      out.buttons |= (1 << 6);  // DPAD_UP
        if (in.dpad_down)    out.buttons |= (1 << 7);  // DPAD_DOWN
        if (in.l1)           out.buttons |= (1 << 8);  // L1
        if (in.r1)           out.buttons |= (1 << 9);  // R1
        if (in.l3)           out.buttons |= (1 << 10); // L3
        if (in.r3)           out.buttons |= (1 << 11); // R3
        if (in.options)      out.buttons |= (1 << 12); // OPTIONS
        if (in.share)        out.buttons |= (1 << 13); // SHARE
        if (in.touchpad_btn) out.buttons |= (1 << 14); // TOUCHPAD
        if (in.ps_btn)       out.buttons |= (1 << 15); // PS

        out.left_stick_x  = in.left_stick_x;
        out.left_stick_y  = in.left_stick_y;
        out.right_stick_x = in.right_stick_x;
        out.right_stick_y = in.right_stick_y;

        out.l2_trigger = in.l2;
        out.r2_trigger = in.r2;

        return out;
    });

    // Video stream frame handler — RGBA directo desde VideoDecoder (FFmpeg swscale, AVX2/SSSE3)
    // El VideoDecoder siempre entrega rgba_data tras la conversión NV12→RGBA en GPU/CPU.
    // No existe fallback scalar: si swscale no produce RGBA el frame se descarta con un warning.
    session_manager_->on_video_frame = [this](portal::stream::DecodedFrame& frame) {
        if (frame.width <= 0 || frame.height <= 0) return;

        const size_t expected_bytes = static_cast<size_t>(frame.width) * frame.height * sizeof(uint32_t);
        if (!frame.rgba_data || frame.rgba_size != expected_bytes) {
            spdlog::warn("[App] Video frame dropped: rgba_data unavailable ({}x{}, rgba_size={})",
                frame.width, frame.height, frame.rgba_size);
            return;
        }

        std::lock_guard<std::mutex> lock(stream_frame_mutex_);
        const size_t pixel_count = static_cast<size_t>(frame.width) * frame.height;
        if (stream_pixel_cache_.size() != pixel_count) {
            stream_pixel_cache_.resize(pixel_count);
        }
        stream_w_ = frame.width;
        stream_h_ = frame.height;

        // Ultra-fast RGBA memcpy — < 0.1ms for 1080p with SIMD optimization
        memcpy(stream_pixel_cache_.data(), frame.rgba_data, expected_bytes);
        stream_has_new_frame_ = true;
    };

    // Low-latency stereo audio output (48kHz 16-bit)
    SDL_AudioSpec audio_spec{};
    audio_spec.format = SDL_AUDIO_S16;
    audio_spec.channels = 2;
    audio_spec.freq = 48000;
    audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, nullptr, nullptr);
    if (audio_stream_) {
        SDL_ResumeAudioStreamDevice(audio_stream_);
        spdlog::info("SDL3 Audio Stream initialized for 48000Hz stereo output");
    } else {
        spdlog::warn("SDL_OpenAudioDeviceStream failed: {}", SDL_GetError());
    }

    session_manager_->on_audio_frame = [this](portal::stream::AudioFrame& audio) {
        if (audio_stream_ && !audio.samples.empty()) {
            // Anti-drift guard: keep queued audio under ~40ms (7680 bytes)
            int queued = SDL_GetAudioStreamQueued(audio_stream_);
            if (queued > 7680) {
                SDL_ClearAudioStream(audio_stream_);
            }
            SDL_PutAudioStreamData(audio_stream_, audio.samples.data(),
                static_cast<int>(audio.samples.size() * sizeof(int16_t)));
        }
    };

    // Center window and show on desktop
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window_);
    SDL_RaiseWindow(window_);
#ifdef _WIN32
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    spdlog::debug("Win32 HWND: 0x{:X}", (uintptr_t)hwnd);
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOWNORMAL);
        ShowWindow(hwnd, SW_RESTORE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        UpdateWindow(hwnd);

        HWND fg = GetForegroundWindow();
        if (fg && fg != hwnd) {
            DWORD fgThread = GetWindowThreadProcessId(fg, NULL);
            DWORD appThread = GetCurrentThreadId();
            if (fgThread != appThread) {
                AttachThreadInput(appThread, fgThread, TRUE);
                BringWindowToTop(hwnd);
                SetForegroundWindow(hwnd);
                SetActiveWindow(hwnd);
                AttachThreadInput(appThread, fgThread, FALSE);
            }
        } else {
            BringWindowToTop(hwnd);
            SetForegroundWindow(hwnd);
            SetActiveWindow(hwnd);
        }

        // ── Apply Windows 11 Mica backdrop ────────────────────
        // Must be called after the window is shown and composited.
        mica_active_ = portal::platform::WindowsWindow::apply_mica_backdrop(hwnd);
        if (mica_active_) {
            spdlog::info("Mica backdrop active — background will be rendered transparently");
            // Re-apply ImGui style now that we know Mica is active
            // (transparent WindowBg vs solid fallback)
            setup_style();
        } else {
            spdlog::info("Mica not available — using solid background color");
        }
    }
#endif

    running_ = true;
    spdlog::info("App::init completed successfully!");
    if (spdlog::default_logger()) spdlog::default_logger()->flush();
    return {};
}

// ─── Main Loop ───────────────────────────────────────────

void App::run() {
    spdlog::info("Entering run() loop...");
    if (spdlog::default_logger()) spdlog::default_logger()->flush();

    int frame_num = 0;
    while (running_) {
        // Process SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Forward events to ControllerManager for hotplugging and haptics
            if (controller_manager_) {
                controller_manager_->handle_sdl_event(event);
            }

            if (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL) {
                last_mouse_activity_time_ = static_cast<float>(ImGui::GetTime());
            }

            // During active streaming, do NOT feed gamepad/keyboard input to ImGui
            // This prevents ImGui from hijacking buttons or focusing the disconnect button
            // UNLESS a modal is open (PIN registration or Login PIN modal), where keyboard/mouse is needed!
            bool modal_open = show_pin_modal_ || show_login_pin_modal_;
            if (current_screen_ != Screen::Streaming || modal_open ||
                (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL) ||
                event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_QUIT) {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }

            switch (event.type) {
                case SDL_EVENT_QUIT:
                    spdlog::warn("SDL_EVENT_QUIT received!");
                    running_ = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_MINIMIZED:
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    swapchain_needs_rebuild_ = true;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    // F11 toggle fullscreen
                    if (event.key.key == SDLK_F11) {
                        bool is_full = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
                        SDL_SetWindowFullscreen(window_, !is_full);
                    }
                    // ESC to go back / exit streaming / close modal
                    if (event.key.key == SDLK_ESCAPE) {
                        if (show_login_pin_modal_) {
                            show_login_pin_modal_ = false;
                        } else if (show_pin_modal_) {
                            show_pin_modal_ = false;
                        } else if (current_screen_ == Screen::Streaming) {
                            float cur_t = static_cast<float>(ImGui::GetTime());
                            if (cur_t - last_mouse_activity_time_ >= 3.0f) {
                                last_mouse_activity_time_ = cur_t; // Wake up HUD first
                            } else {
                                if (session_manager_) session_manager_->disconnect();
                                SDL_SetWindowFullscreen(window_, false);
                                current_screen_ = Screen::Home;
                                current_tab_ = 0;
                            }
                        } else if (current_screen_ != Screen::Home) {
                            current_screen_ = Screen::Home;
                            current_tab_ = 0;
                        }
                    }
                    // Console Tab navigation via Q/E or 1/2/3 (only in menus when NO modal is open)
                    if (current_screen_ != Screen::Streaming && !show_pin_modal_ && !show_login_pin_modal_) {
                        if (event.key.key == SDLK_Q) {
                            current_tab_ = (current_tab_ + 2) % 3;
                            if (current_tab_ == 0) current_screen_ = Screen::Home;
                            else if (current_tab_ == 1) current_screen_ = Screen::CloudGames;
                            else if (current_tab_ == 2) current_screen_ = Screen::Settings;
                        } else if (event.key.key == SDLK_E) {
                            current_tab_ = (current_tab_ + 1) % 3;
                            if (current_tab_ == 0) current_screen_ = Screen::Home;
                            else if (current_tab_ == 1) current_screen_ = Screen::CloudGames;
                            else if (current_tab_ == 2) current_screen_ = Screen::Settings;
                        } else if (event.key.key == SDLK_1) {
                            current_tab_ = 0;
                            current_screen_ = Screen::Home;
                        } else if (event.key.key == SDLK_2) {
                            current_tab_ = 1;
                            current_screen_ = Screen::CloudGames;
                        } else if (event.key.key == SDLK_3) {
                            current_tab_ = 2;
                            current_screen_ = Screen::Settings;
                        }
                    }

                    // 4-digit User Login PIN keyboard handling (strictly non-repeating)
                    if (show_login_pin_modal_ && !event.key.repeat) {
                        char d = 0;
                        if (event.key.key >= SDLK_0 && event.key.key <= SDLK_9) {
                            d = static_cast<char>('0' + (event.key.key - SDLK_0));
                        } else if (event.key.key >= SDLK_KP_1 && event.key.key <= SDLK_KP_9) {
                            d = static_cast<char>('1' + (event.key.key - SDLK_KP_1));
                        } else if (event.key.key == SDLK_KP_0) {
                            d = '0';
                        }

                        if (d != 0) {
                            login_pin_digits_[active_login_pin_digit_] = d;
                            if (active_login_pin_digit_ < 3) active_login_pin_digit_++;
                        } else if (event.key.key == SDLK_BACKSPACE) {
                            if (login_pin_digits_[active_login_pin_digit_] != '\0') {
                                login_pin_digits_[active_login_pin_digit_] = '\0';
                            } else if (active_login_pin_digit_ > 0) {
                                active_login_pin_digit_--;
                                login_pin_digits_[active_login_pin_digit_] = '\0';
                            }
                        } else if (event.key.key == SDLK_LEFT && active_login_pin_digit_ > 0) {
                            active_login_pin_digit_--;
                        } else if (event.key.key == SDLK_RIGHT && active_login_pin_digit_ < 3) {
                            active_login_pin_digit_++;
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            bool pin_ready = true;
                            for (int i = 0; i < 4; i++) {
                                if (login_pin_digits_[i] < '0' || login_pin_digits_[i] > '9') {
                                    pin_ready = false;
                                    break;
                                }
                            }
                            if (pin_ready) {
                                std::string pin_str(login_pin_digits_, 4);
                                if (session_manager_) {
                                    session_manager_->send_login_pin(pin_str);
                                }
                                if (remember_login_pin_) {
                                    if (!target_login_console_host_id_.empty()) {
                                        console_registry_->update_console_login_pin(target_login_console_host_id_, pin_str);
                                    } else {
                                        auto consoles = console_registry_->get_all_consoles();
                                        if (!consoles.empty()) {
                                            console_registry_->update_console_login_pin(consoles[0].host_id, pin_str);
                                        }
                                    }
                                }
                                show_login_pin_modal_ = false;
                                show_toast("PIN de usuario enviado a la consola", 3.0f);
                            }
                        }
                    }

                    // PIN 8-box entry keyboard handling (Registration - strictly non-repeating)
                    if ((show_pin_modal_ || current_screen_ == Screen::Register) && !event.key.repeat) {
                        char d = 0;
                        if (event.key.key >= SDLK_0 && event.key.key <= SDLK_9) {
                            d = static_cast<char>('0' + (event.key.key - SDLK_0));
                        } else if (event.key.key >= SDLK_KP_1 && event.key.key <= SDLK_KP_9) {
                            d = static_cast<char>('1' + (event.key.key - SDLK_KP_1));
                        } else if (event.key.key == SDLK_KP_0) {
                            d = '0';
                        }

                        if (d != 0) {
                            pin_digits_[active_pin_digit_] = d;
                            if (active_pin_digit_ < 7) active_pin_digit_++;
                        } else if (event.key.key == SDLK_BACKSPACE) {
                            if (pin_digits_[active_pin_digit_] != '\0') {
                                pin_digits_[active_pin_digit_] = '\0';
                            } else if (active_pin_digit_ > 0) {
                                active_pin_digit_--;
                                pin_digits_[active_pin_digit_] = '\0';
                            }
                        } else if (event.key.key == SDLK_LEFT && active_pin_digit_ > 0) {
                            active_pin_digit_--;
                        } else if (event.key.key == SDLK_RIGHT && active_pin_digit_ < 7) {
                            active_pin_digit_++;
                        }
                    }

                    // Direct PS Home shortcut during streaming
                    if (current_screen_ == Screen::Streaming && !modal_open) {
                        if (event.key.key == SDLK_F1 || event.key.key == SDLK_HOME) {
                            if (session_manager_) {
                                session_manager_->send_ps_button_pulse(200);
                                show_toast("Boton PS pulsado", 1.5f);
                            }
                        }
                    }
                    break;

                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    // When streaming, NEVER intercept gamepad buttons for UI navigation
                    if (current_screen_ != Screen::Streaming && !modal_open) {
                        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                            current_tab_ = (current_tab_ + 2) % 3;
                            if (current_tab_ == 0) current_screen_ = Screen::Home;
                            else if (current_tab_ == 1) current_screen_ = Screen::CloudGames;
                            else if (current_tab_ == 2) current_screen_ = Screen::Settings;
                        } else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                            current_tab_ = (current_tab_ + 1) % 3;
                            if (current_tab_ == 0) current_screen_ = Screen::Home;
                            else if (current_tab_ == 1) current_screen_ = Screen::CloudGames;
                            else if (current_tab_ == 2) current_screen_ = Screen::Settings;
                        } else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) { // Circle (B)
                            if (show_login_pin_modal_) {
                                show_login_pin_modal_ = false;
                            } else if (show_pin_modal_) {
                                show_pin_modal_ = false;
                            } else if (current_screen_ != Screen::Home) {
                                current_screen_ = Screen::Home;
                                current_tab_ = 0;
                            }
                        }
                    }
                    break;

                case SDL_EVENT_GAMEPAD_ADDED:
                    spdlog::info("[App] Mando añadido: {}", event.gdevice.which);
                    break;

                case SDL_EVENT_GAMEPAD_REMOVED:
                    spdlog::info("[App] Mando retirado: {}", event.gdevice.which);
                    break;

                default:
                    break;
            }
        }

        // Rebuild swapchain if needed
        if (swapchain_needs_rebuild_) {
            int w, h;
            SDL_GetWindowSize(window_, &w, &h);
            if (w > 0 && h > 0) {
                vkDeviceWaitIdle(device_);
                // Cleanup old framebuffers and image views
                for (auto& frame : frames_) {
                    if (frame.framebuffer != VK_NULL_HANDLE) {
                        vkDestroyFramebuffer(device_, frame.framebuffer, nullptr);
                        frame.framebuffer = VK_NULL_HANDLE;
                    }
                }
                for (auto iv : swapchain_image_views_) {
                    vkDestroyImageView(device_, iv, nullptr);
                }
                swapchain_image_views_.clear();

                (void)create_swapchain();
                (void)create_framebuffers();
                swapchain_needs_rebuild_ = false;
            }
        }

        // Render frame
        if (begin_frame()) {
            render_ui();
            end_frame();
            present();

            if (frame_num == 2) {
#ifdef _WIN32
                HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
                if (hwnd) {
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                }
#endif
            }
        }
        frame_num++;
    }

    vkDeviceWaitIdle(device_);
}

void App::request_quit() {
    running_ = false;
}

void App::navigate_to(Screen screen) {
    spdlog::info("Navigating to screen: {}", static_cast<int>(screen));
    current_screen_ = screen;
    if (screen == Screen::Home || screen == Screen::ConsoleList) {
        current_tab_ = 0;
        show_pin_modal_ = false;
    } else if (screen == Screen::CloudGames) {
        current_tab_ = 1;
        show_pin_modal_ = false;
    } else if (screen == Screen::Settings) {
        current_tab_ = 2;
        show_pin_modal_ = false;
    } else if (screen == Screen::Register) {
        show_pin_modal_ = true;
        active_pin_digit_ = 0;
        for (int d = 0; d < 8; d++) pin_digits_[d] = '\0';
    }
}

// ─── Vulkan Initialization ───────────────────────────────

VoidResult App::init_vulkan() {
    auto result = create_instance();
    if (!result) return result;

    result = create_surface();
    if (!result) return result;

    result = select_physical_device();
    if (!result) return result;

    result = create_logical_device();
    if (!result) return result;

    result = create_swapchain();
    if (!result) return result;

    result = create_render_pass();
    if (!result) return result;

    result = create_framebuffers();
    if (!result) return result;

    result = create_command_pool();
    if (!result) return result;

    result = create_sync_objects();
    if (!result) return result;

    spdlog::info("Vulkan initialized successfully");
    return {};
}

VoidResult App::create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Ludelo";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "LudeloEngine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    // Get required extensions from SDL
    uint32_t sdl_ext_count = 0;
    auto sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
    std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_ext_count);

    #ifdef _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    #ifdef _DEBUG
    const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = validation_layers;
    #endif

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create Vulkan instance"});
    }

    spdlog::info("Vulkan instance created (API 1.3)");
    return {};
}

VoidResult App::create_surface() {
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
        return std::unexpected(Error{ErrorCode::Unknown,
            std::format("Failed to create Vulkan surface: {}", SDL_GetError())});
    }
    return {};
}

VoidResult App::select_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0) {
        return std::unexpected(Error{ErrorCode::Unknown, "No Vulkan-capable GPU found"});
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    // Prefer discrete GPU
    for (auto dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical_device_ = dev;
            spdlog::info("Selected GPU: {} (discrete)", props.deviceName);
            break;
        }
    }

    if (physical_device_ == VK_NULL_HANDLE) {
        physical_device_ = devices[0];
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical_device_, &props);
        spdlog::info("Selected GPU: {} (fallback)", props.deviceName);
    }

    // Find queue families
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_count, queue_families.data());

    bool found_graphics = false;
    bool found_present = false;

    for (uint32_t i = 0; i < queue_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family_ = i;
            found_graphics = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i, surface_, &present_support);
        if (present_support) {
            present_queue_family_ = i;
            found_present = true;
        }

        if (found_graphics && found_present) break;
    }

    if (!found_graphics || !found_present) {
        return std::unexpected(Error{ErrorCode::Unknown, "GPU lacks required queue families"});
    }

    return {};
}

VoidResult App::create_logical_device() {
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_families = {graphics_queue_family_, present_queue_family_};

    float priority = 1.0f;
    for (uint32_t family : unique_families) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_create_infos.push_back(queue_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = queue_create_infos.data();
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_info.pEnabledFeatures = &device_features;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;

    if (vkCreateDevice(physical_device_, &device_info, nullptr, &device_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create logical device"});
    }

    vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, present_queue_family_, 0, &present_queue_);

    return {};
}

VoidResult App::create_swapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &capabilities);

    // Choose format
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());

    swapchain_format_ = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
            fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain_format_ = fmt.format;
            color_space = fmt.colorSpace;
            break;
        }
    }

    // Choose extent
    if (capabilities.currentExtent.width != UINT32_MAX) {
        swapchain_extent_ = capabilities.currentExtent;
    } else {
        int w, h;
        SDL_GetWindowSizeInPixels(window_, &w, &h);
        swapchain_extent_.width = std::clamp(static_cast<uint32_t>(w),
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        swapchain_extent_.height = std::clamp(static_cast<uint32_t>(h),
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // Choose present mode (prefer mailbox for low latency)
    uint32_t present_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &present_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &present_count, present_modes.data());

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto mode : present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sc_info{};
    sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_info.surface = surface_;
    sc_info.minImageCount = image_count;
    sc_info.imageFormat = swapchain_format_;
    sc_info.imageColorSpace = color_space;
    sc_info.imageExtent = swapchain_extent_;
    sc_info.imageArrayLayers = 1;
    sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc_info.preTransform = capabilities.currentTransform;
    // Use VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR when available so that Vulkan passes
    // the alpha channel to the DWM compositor — required for Mica to show through.
    // Fall back to OPAQUE if INHERIT is not supported by the surface.
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else {
        sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    sc_info.presentMode = present_mode;
    sc_info.clipped = VK_TRUE;
    sc_info.oldSwapchain = swapchain_;

    uint32_t family_indices[] = {graphics_queue_family_, present_queue_family_};
    if (graphics_queue_family_ != present_queue_family_) {
        sc_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sc_info.queueFamilyIndexCount = 2;
        sc_info.pQueueFamilyIndices = family_indices;
    } else {
        sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkSwapchainKHR old_swapchain = swapchain_;
    if (vkCreateSwapchainKHR(device_, &sc_info, nullptr, &swapchain_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create swapchain"});
    }

    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, old_swapchain, nullptr);
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    // Create image views
    swapchain_image_views_.resize(image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo iv_info{};
        iv_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_info.image = swapchain_images_[i];
        iv_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_info.format = swapchain_format_;
        iv_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        iv_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        if (vkCreateImageView(device_, &iv_info, nullptr, &swapchain_image_views_[i]) != VK_SUCCESS) {
            return std::unexpected(Error{ErrorCode::Unknown, "Failed to create image view"});
        }
    }

    spdlog::info("Swapchain created: {}x{}, {} images, format {}",
        swapchain_extent_.width, swapchain_extent_.height, image_count, static_cast<int>(swapchain_format_));
    return {};
}

VoidResult App::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = 1;
    rp_info.pAttachments = &color_attachment;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 1;
    rp_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &rp_info, nullptr, &render_pass_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create render pass"});
    }

    return {};
}

VoidResult App::create_framebuffers() {
    frames_.resize(swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkFramebufferCreateInfo fb_info{};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass_;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &swapchain_image_views_[i];
        fb_info.width = swapchain_extent_.width;
        fb_info.height = swapchain_extent_.height;
        fb_info.layers = 1;

        if (vkCreateFramebuffer(device_, &fb_info, nullptr, &frames_[i].framebuffer) != VK_SUCCESS) {
            return std::unexpected(Error{ErrorCode::Unknown, "Failed to create framebuffer"});
        }
    }

    return {};
}

VoidResult App::create_command_pool() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = graphics_queue_family_;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create command pool"});
    }

    // Allocate command buffers
    std::vector<VkCommandBuffer> cmd_buffers(frames_.size());
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(frames_.size());

    if (vkAllocateCommandBuffers(device_, &alloc_info, cmd_buffers.data()) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to allocate command buffers"});
    }

    for (size_t i = 0; i < frames_.size(); i++) {
        frames_[i].command_buffer = cmd_buffers[i];
    }

    return {};
}

VoidResult App::create_sync_objects() {
    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame : frames_) {
        if (vkCreateSemaphore(device_, &sem_info, nullptr, &frame.image_available) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &sem_info, nullptr, &frame.render_finished) != VK_SUCCESS ||
            vkCreateFence(device_, &fence_info, nullptr, &frame.fence) != VK_SUCCESS) {
            return std::unexpected(Error{ErrorCode::Unknown, "Failed to create sync objects"});
        }
    }

    return {};
}

// ─── ImGui ───────────────────────────────────────────────

VoidResult App::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;  // No ini file

    setup_style();
    load_fonts();

    // Create descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
    };

    VkDescriptorPoolCreateInfo dp_info{};
    dp_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dp_info.maxSets = 100;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device_, &dp_info, nullptr, &imgui_descriptor_pool_) != VK_SUCCESS) {
        return std::unexpected(Error{ErrorCode::Unknown, "Failed to create ImGui descriptor pool"});
    }

    // Init ImGui backends
    ImGui_ImplSDL3_InitForVulkan(window_);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = instance_;
    init_info.PhysicalDevice = physical_device_;
    init_info.Device = device_;
    init_info.QueueFamily = graphics_queue_family_;
    init_info.Queue = graphics_queue_;
    init_info.DescriptorPool = imgui_descriptor_pool_;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(swapchain_images_.size());
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.RenderPass = render_pass_;

    ImGui_ImplVulkan_Init(&init_info);
    load_all_icons();

    spdlog::info("ImGui initialized with Vulkan backend and icon textures");
    return {};
}

static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

bool App::load_texture(const std::string& name, const std::string& filepath) {
    if (textures_.contains(name)) return true;

    std::string path = filepath;
    if (!std::filesystem::exists(path)) {
        path = "../" + filepath;
        if (!std::filesystem::exists(path)) {
            return false;
        }
    }

    SDL_Surface* src = SDL_LoadPNG(path.c_str());
    if (!src) {
        spdlog::warn("Failed to load PNG: {}", path);
        return false;
    }

    SDL_Surface* surf = SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(src);
    if (!surf) {
        return false;
    }

    VkDeviceSize image_size = surf->w * surf->h * 4;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;

    VkBufferCreateInfo buf_info{};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = image_size;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &buf_info, nullptr, &staging_buffer) != VK_SUCCESS) {
        SDL_DestroySurface(surf);
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device_, staging_buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device_, mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &alloc_info, nullptr, &staging_memory) != VK_SUCCESS) {
        vkDestroyBuffer(device_, staging_buffer, nullptr);
        SDL_DestroySurface(surf);
        return false;
    }
    vkBindBufferMemory(device_, staging_buffer, staging_memory, 0);

    void* mapped = nullptr;
    vkMapMemory(device_, staging_memory, 0, image_size, 0, &mapped);
    memcpy(mapped, surf->pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(device_, staging_memory);

    VulkanTexture tex;
    tex.width = surf->w;
    tex.height = surf->h;

    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = static_cast<uint32_t>(surf->w);
    img_info.extent.height = static_cast<uint32_t>(surf->h);
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device_, &img_info, nullptr, &tex.image) != VK_SUCCESS) {
        vkDestroyBuffer(device_, staging_buffer, nullptr);
        vkFreeMemory(device_, staging_memory, nullptr);
        SDL_DestroySurface(surf);
        return false;
    }

    VkMemoryRequirements img_reqs;
    vkGetImageMemoryRequirements(device_, tex.image, &img_reqs);

    VkMemoryAllocateInfo img_alloc{};
    img_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    img_alloc.allocationSize = img_reqs.size;
    img_alloc.memoryTypeIndex = find_memory_type(physical_device_, img_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device_, &img_alloc, nullptr, &tex.memory) != VK_SUCCESS) {
        vkDestroyImage(device_, tex.image, nullptr);
        vkDestroyBuffer(device_, staging_buffer, nullptr);
        vkFreeMemory(device_, staging_memory, nullptr);
        SDL_DestroySurface(surf);
        return false;
    }
    vkBindImageMemory(device_, tex.image, tex.memory, 0);

    // One-time copy command
    VkCommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc.commandPool = command_pool_;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo cb_begin{};
    cb_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cb_begin);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy_region{};
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageOffset = {0, 0, 0};
    copy_region.imageExtent = {static_cast<uint32_t>(surf->w), static_cast<uint32_t>(surf->h), 1};

    vkCmdCopyBufferToImage(cmd, staging_buffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);

    vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);
    vkDestroyBuffer(device_, staging_buffer, nullptr);
    vkFreeMemory(device_, staging_memory, nullptr);
    SDL_DestroySurface(surf);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = tex.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &view_info, nullptr, &tex.view) != VK_SUCCESS) {
        vkDestroyImage(device_, tex.image, nullptr);
        vkFreeMemory(device_, tex.memory, nullptr);
        return false;
    }

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.maxAnisotropy = 1.0f;
    if (vkCreateSampler(device_, &sampler_info, nullptr, &tex.sampler) != VK_SUCCESS) {
        vkDestroyImageView(device_, tex.view, nullptr);
        vkDestroyImage(device_, tex.image, nullptr);
        vkFreeMemory(device_, tex.memory, nullptr);
        return false;
    }

    tex.descriptor_set = ImGui_ImplVulkan_AddTexture(tex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    textures_[name] = tex;
    spdlog::info("Loaded texture '{}' ({}x{}) successfully into Vulkan", name, tex.width, tex.height);
    return true;
}

void App::load_all_icons() {
    load_texture("ps5", "assets/icons/ic_ps5.png");
    load_texture("ps4", "assets/icons/ic_ps4.png");
    load_texture("cloud", "assets/icons/ic_cloud.png");
    load_texture("gamecontroller", "assets/icons/ic_gamecontroller.png");
    load_texture("gear", "assets/icons/ic_gear.png");
    load_texture("antenna", "assets/icons/ic_antenna.png");
    load_texture("globe", "assets/icons/ic_globe.png");
    load_texture("playstation", "assets/icons/ic_playstation.png");
    load_texture("person", "assets/icons/ic_person.png");
    load_texture("sliders", "assets/icons/ic_sliders.png");
    load_texture("info", "assets/icons/ic_info.png");
}

void App::destroy_textures() {
    for (auto& [name, tex] : textures_) {
        if (tex.descriptor_set) {
            ImGui_ImplVulkan_RemoveTexture(tex.descriptor_set);
        }
        if (tex.sampler) {
            vkDestroySampler(device_, tex.sampler, nullptr);
        }
        if (tex.view) {
            vkDestroyImageView(device_, tex.view, nullptr);
        }
        if (tex.image) {
            vkDestroyImage(device_, tex.image, nullptr);
        }
        if (tex.memory) {
            vkFreeMemory(device_, tex.memory, nullptr);
        }
    }
    textures_.clear();
}

bool App::init_stream_texture(int width, int height) {
    if (stream_texture_initialized_ && stream_w_ == width && stream_h_ == height) {
        return true;
    }
    destroy_stream_texture();

    stream_w_ = width;
    stream_h_ = height;
    VkDeviceSize image_size = static_cast<VkDeviceSize>(width) * height * 4;

    VkBufferCreateInfo buf_info{};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = image_size;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &buf_info, nullptr, &stream_staging_buffer_) != VK_SUCCESS) return false;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device_, stream_staging_buffer_, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device_, mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &alloc_info, nullptr, &stream_staging_memory_) != VK_SUCCESS) return false;
    vkBindBufferMemory(device_, stream_staging_buffer_, stream_staging_memory_, 0);

    // Initial fill: PlayStation dark gradient
    if (vkMapMemory(device_, stream_staging_memory_, 0, image_size, 0, &stream_staging_mapped_) != VK_SUCCESS) {
        return false;
    }
    uint32_t* pix = static_cast<uint32_t*>(stream_staging_mapped_);
    for (int y = 0; y < height; ++y) {
        float v = static_cast<float>(y) / height;
        uint32_t r = static_cast<uint32_t>(10 + 15 * v);
        uint32_t g = static_cast<uint32_t>(18 + 28 * v);
        uint32_t b = static_cast<uint32_t>(40 + 75 * v);
        uint32_t color = 0xFF000000 | (b << 16) | (g << 8) | r;
        for (int x = 0; x < width; ++x) {
            pix[y * width + x] = color;
        }
    }

    VkImageCreateInfo img_info{};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = width;
    img_info.extent.height = height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    if (vkCreateImage(device_, &img_info, nullptr, &stream_texture_.image) != VK_SUCCESS) return false;

    VkMemoryRequirements img_reqs;
    vkGetImageMemoryRequirements(device_, stream_texture_.image, &img_reqs);

    VkMemoryAllocateInfo img_alloc{};
    img_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    img_alloc.allocationSize = img_reqs.size;
    img_alloc.memoryTypeIndex = find_memory_type(physical_device_, img_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device_, &img_alloc, nullptr, &stream_texture_.memory) != VK_SUCCESS) return false;
    vkBindImageMemory(device_, stream_texture_.image, stream_texture_.memory, 0);

    VkCommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc.commandPool = command_pool_;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &cmd_alloc, &cmd);

    VkCommandBufferBeginInfo cb_begin{};
    cb_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cb_begin);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = stream_texture_.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

    vkCmdCopyBufferToImage(cmd, stream_staging_buffer_, stream_texture_.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);
    vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = stream_texture_.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &view_info, nullptr, &stream_texture_.view) != VK_SUCCESS) return false;

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    if (vkCreateSampler(device_, &sampler_info, nullptr, &stream_texture_.sampler) != VK_SUCCESS) return false;

    stream_texture_.descriptor_set = ImGui_ImplVulkan_AddTexture(stream_texture_.sampler, stream_texture_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    stream_texture_.width = width;
    stream_texture_.height = height;

    // Pre-allocate dedicated command buffer for stream texture upload
    VkCommandBufferAllocateInfo up_cmd_alloc{};
    up_cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    up_cmd_alloc.commandPool = command_pool_;
    up_cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    up_cmd_alloc.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device_, &up_cmd_alloc, &stream_upload_cmd_) != VK_SUCCESS) return false;

    // Pre-create upload fence in signaled state
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(device_, &fence_info, nullptr, &stream_upload_fence_) != VK_SUCCESS) return false;

    stream_texture_initialized_ = true;

    spdlog::info("Initialized dynamic Vulkan stream texture ({}x{}) successfully", width, height);
    return true;
}

void App::destroy_stream_texture() {
    if (!stream_texture_initialized_) return;
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        if (stream_upload_fence_ != VK_NULL_HANDLE) {
            vkDestroyFence(device_, stream_upload_fence_, nullptr);
            stream_upload_fence_ = VK_NULL_HANDLE;
        }
        if (stream_upload_cmd_ != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device_, command_pool_, 1, &stream_upload_cmd_);
            stream_upload_cmd_ = VK_NULL_HANDLE;
        }
        if (stream_staging_mapped_ != nullptr) {
            vkUnmapMemory(device_, stream_staging_memory_);
            stream_staging_mapped_ = nullptr;
        }
        if (stream_texture_.descriptor_set) {
            ImGui_ImplVulkan_RemoveTexture(stream_texture_.descriptor_set);
            stream_texture_.descriptor_set = VK_NULL_HANDLE;
        }
        if (stream_texture_.sampler) {
            vkDestroySampler(device_, stream_texture_.sampler, nullptr);
            stream_texture_.sampler = VK_NULL_HANDLE;
        }
        if (stream_texture_.view) {
            vkDestroyImageView(device_, stream_texture_.view, nullptr);
            stream_texture_.view = VK_NULL_HANDLE;
        }
        if (stream_texture_.image) {
            vkDestroyImage(device_, stream_texture_.image, nullptr);
            stream_texture_.image = VK_NULL_HANDLE;
        }
        if (stream_texture_.memory) {
            vkFreeMemory(device_, stream_texture_.memory, nullptr);
            stream_texture_.memory = VK_NULL_HANDLE;
        }
        if (stream_staging_buffer_) {
            vkDestroyBuffer(device_, stream_staging_buffer_, nullptr);
            stream_staging_buffer_ = VK_NULL_HANDLE;
        }
        if (stream_staging_memory_) {
            vkFreeMemory(device_, stream_staging_memory_, nullptr);
            stream_staging_memory_ = VK_NULL_HANDLE;
        }
    }
    stream_texture_initialized_ = false;
}

void App::update_stream_texture(const uint32_t* pixels, int width, int height) {
    if (!stream_texture_initialized_ || stream_w_ != width || stream_h_ != height) {
        if (!init_stream_texture(width, height)) return;
    }
    if (!pixels || !stream_staging_mapped_ || stream_upload_cmd_ == VK_NULL_HANDLE || stream_upload_fence_ == VK_NULL_HANDLE) return;

    // Zero-overhead fence wait (previous GPU DMA copy finished ~16ms ago, returns in 0ms)
    vkWaitForFences(device_, 1, &stream_upload_fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &stream_upload_fence_);

    VkDeviceSize image_size = static_cast<VkDeviceSize>(width) * height * 4;
    memcpy(stream_staging_mapped_, pixels, static_cast<size_t>(image_size));

    VkCommandBufferBeginInfo cb_begin{};
    cb_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(stream_upload_cmd_, &cb_begin);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = stream_texture_.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(stream_upload_cmd_, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

    vkCmdCopyBufferToImage(stream_upload_cmd_, stream_staging_buffer_, stream_texture_.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(stream_upload_cmd_, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(stream_upload_cmd_);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &stream_upload_cmd_;
    // Asynchronous submit with fence; no CPU blocking or vkQueueWaitIdle!
    vkQueueSubmit(graphics_queue_, 1, &submit, stream_upload_fence_);
}

ImTextureID App::get_texture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return (ImTextureID)it->second.descriptor_set;
    }
    return (ImTextureID)0;
}

void App::setup_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    // ── Ultra-premium Midnight Portal console style ──
    style.WindowRounding    = 16.0f;
    style.ChildRounding     = 14.0f;
    style.FrameRounding     = 10.0f;
    style.PopupRounding     = 14.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 8.0f;
    style.TabRounding       = 8.0f;

    style.WindowPadding     = ImVec2(24.0f, 24.0f);
    style.FramePadding      = ImVec2(16.0f, 10.0f);
    style.ItemSpacing       = ImVec2(16.0f, 12.0f);
    style.ItemInnerSpacing  = ImVec2(10.0f, 6.0f);
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 14.0f;

    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    // ── Midnight Portal Theme Palette Colors ──
    ImVec4* c = style.Colors;
    // When Mica is active, make the main window background transparent so the
    // DWM compositor Mica effect shows through. Panels use kSurface (semi-opaque glass).
    c[ImGuiCol_WindowBg] = mica_active_
        ? ImVec4(0.0f, 0.0f, 0.0f, 0.0f)   // fully transparent — Mica shows through
        : colors::kBackground;               // solid deep navy fallback
    c[ImGuiCol_ChildBg]              = {0.0f, 0.0f, 0.0f, 0.0f};
    c[ImGuiCol_PopupBg]              = colors::kSurface;
    c[ImGuiCol_Border]               = colors::kGlassBorder;

    c[ImGuiCol_FrameBg]              = colors::kSurface;
    c[ImGuiCol_FrameBgHovered]       = colors::kSurfaceHover;
    c[ImGuiCol_FrameBgActive]        = colors::kSurfaceActive;

    c[ImGuiCol_TitleBg]              = colors::kBackground;
    c[ImGuiCol_TitleBgActive]        = colors::kSurface;

    c[ImGuiCol_MenuBarBg]            = colors::kSurface;

    c[ImGuiCol_Button]               = colors::kPrimary;
    c[ImGuiCol_ButtonHovered]        = colors::kPrimaryHover;
    c[ImGuiCol_ButtonActive]         = {0.00f, 0.35f, 0.70f, 1.00f};

    c[ImGuiCol_Header]               = colors::kSurface;
    c[ImGuiCol_HeaderHovered]        = colors::kSurfaceHover;
    c[ImGuiCol_HeaderActive]         = colors::kSurfaceActive;

    c[ImGuiCol_Tab]                  = colors::kSurface;
    c[ImGuiCol_TabHovered]           = colors::kPrimary;
    c[ImGuiCol_TabSelected]          = colors::kPrimary;

    c[ImGuiCol_Text]                 = colors::kTextPrimary;
    c[ImGuiCol_TextDisabled]         = colors::kTextSecondary;

    c[ImGuiCol_SliderGrab]           = colors::kAccent;
    c[ImGuiCol_SliderGrabActive]     = colors::kAccentHover;

    c[ImGuiCol_CheckMark]            = colors::kAccent;

    c[ImGuiCol_ScrollbarBg]          = {0.04f, 0.06f, 0.12f, 0.40f};
    c[ImGuiCol_ScrollbarGrab]        = {0.20f, 0.28f, 0.45f, 0.50f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.00f, 0.44f, 0.82f, 0.70f};
    c[ImGuiCol_ScrollbarGrabActive]  = colors::kAccent;

    c[ImGuiCol_NavHighlight]         = colors::kAccent;
}

void App::load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Prioritize SFNS.ttf (PlayStation Portal / San Francisco font)
    std::vector<std::string> candidates = {
        "assets/fonts/SFNS.ttf",
        "../assets/fonts/SFNS.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    std::string font_path;
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            font_path = path;
            break;
        }
    }

    if (!font_path.empty()) {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        font_body_     = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, &cfg);
        font_title_    = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 28.0f, &cfg);
        font_subtitle_ = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 20.0f, &cfg);
        font_small_    = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 13.0f, &cfg);
        font_pin_      = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 36.0f, &cfg);
        spdlog::info("Loaded SFNS font successfully from: {}", font_path);
    } else {
        font_body_ = io.Fonts->AddFontDefault();
        font_title_ = font_body_;
        font_subtitle_ = font_body_;
        font_small_ = font_body_;
        font_pin_ = font_body_;
        spdlog::warn("Font file not found, using ImGui default");
    }
}

// ─── Frame Rendering ─────────────────────────────────────

bool App::begin_frame() {
    auto& frame = frames_[current_frame_];

    vkWaitForFences(device_, 1, &frame.fence, VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
        frame.image_available, VK_NULL_HANDLE, &image_index_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain_needs_rebuild_ = true;
        return false;
    }

    vkResetFences(device_, 1, &frame.fence);
    vkResetCommandBuffer(frame.command_buffer, 0);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    return true;
}

void App::render_ui() {
    // Dynamically toggle ImGui navigation: disabled during streaming to prevent button hijacking
    ImGuiIO& io = ImGui::GetIO();
    if (current_screen_ == Screen::Streaming) {
        io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
    } else {
        io.ConfigFlags |= (ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
    }

    // Full-window ImGui canvas
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##MainCanvas", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    // Periodic console state probe (every 5 seconds) when not streaming
    float cur_time = static_cast<float>(ImGui::GetTime());
    if (current_screen_ != Screen::Streaming && (cur_time - last_probe_time_ > 5.0f) && !is_probing_.load()) {
        last_probe_time_ = cur_time;
        probe_consoles_background();
    }

    if (current_screen_ != Screen::Streaming) {
        draw_ambient_background();
        draw_top_navigation_bar();

        switch (current_screen_) {
            case Screen::Home:
            case Screen::ConsoleList:
                draw_home_screen();
                break;
            case Screen::CloudGames:
                draw_cloud_games();
                break;
            case Screen::Settings:
                draw_settings();
                break;
            case Screen::Register:
                draw_registration_screen();
                break;
            default:
                draw_home_screen();
                break;
        }

        if (show_pin_modal_) {
            draw_pin_modal();
        }

        if (show_login_pin_modal_) {
            draw_login_pin_modal();
        }

        draw_status_bar();
    } else {
        draw_streaming_overlay();

        if (show_login_pin_modal_) {
            draw_login_pin_modal();
        }
    }

    draw_toast_notification();

    ImGui::End();
}

void App::end_frame() {
    ImGui::Render();

    auto& frame = frames_[current_frame_];
    auto cmd = frame.command_buffer;

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    // When Mica is active, render with a fully transparent clear color so DWM
    // compositor shows the desktop wallpaper through the window background.
    // When Mica is not available, use the deep navy color from the design system.
    VkClearValue clear_color;
    if (mica_active_) {
        // Alpha = 0 lets Mica/DWM show through behind the ImGui panels
        clear_color = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
    } else {
        clear_color = {{{
            colors::kBackground.x, colors::kBackground.y,
            colors::kBackground.z, colors::kBackground.w
        }}};
    }

    VkRenderPassBeginInfo rp_begin{};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.renderPass = render_pass_;
    rp_begin.framebuffer = frames_[image_index_].framebuffer;
    rp_begin.renderArea = {{0, 0}, swapchain_extent_};
    rp_begin.clearValueCount = 1;
    rp_begin.pClearValues = &clear_color;

    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

void App::present() {
    auto& frame = frames_[current_frame_];

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.image_available;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame.command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.render_finished;

    vkQueueSubmit(graphics_queue_, 1, &submit, frame.fence);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame.render_finished;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &image_index_;

    VkResult result = vkQueuePresentKHR(present_queue_, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain_needs_rebuild_ = true;
    }

    current_frame_ = (current_frame_ + 1) % frames_.size();
}

// ─── UI Screens ──────────────────────────────────────────

void App::show_toast(const std::string& message, float duration) {
    toast_message_ = message;
    toast_timer_ = duration;
}

void App::draw_toast_notification() {
    if (toast_timer_ <= 0.0f || toast_message_.empty()) return;
    toast_timer_ -= ImGui::GetIO().DeltaTime;

    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float toast_w = 460.0f;
    float toast_h = 44.0f;
    ImVec2 pos((ws.x - toast_w) * 0.5f, 78.0f);

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    float alpha = std::min(1.0f, toast_timer_ * 2.5f);
    ImU32 bg = IM_COL32(11, 20, 42, static_cast<int>(240 * alpha));
    ImU32 border = IM_COL32(0, 240, 255, static_cast<int>(210 * alpha));
    ImU32 glow = IM_COL32(0, 240, 255, static_cast<int>(60 * alpha));

    draw->AddRectFilled(ImVec2(pos.x - 3, pos.y - 3), ImVec2(pos.x + toast_w + 3, pos.y + toast_h + 3), glow, 22.0f);
    draw->AddRectFilled(pos, ImVec2(pos.x + toast_w, pos.y + toast_h), bg, 20.0f);
    draw->AddRect(pos, ImVec2(pos.x + toast_w, pos.y + toast_h), border, 20.0f, 0, 1.5f);

    draw->AddCircleFilled(ImVec2(pos.x + 22.0f, pos.y + toast_h * 0.5f), 5.0f, IM_COL32(0, 240, 255, static_cast<int>(255 * alpha)));

    if (font_small_) ImGui::PushFont(font_small_);
    ImVec2 tsz = ImGui::CalcTextSize(toast_message_.c_str());
    ImVec2 tpos(pos.x + 36.0f, pos.y + (toast_h - tsz.y) * 0.5f);
    draw->AddText(tpos, IM_COL32(245, 250, 255, static_cast<int>(255 * alpha)), toast_message_.c_str());
    if (font_small_) ImGui::PopFont();
}

// ─── Vector PlayStation Controller Button Glyphs ──────────
static void DrawPSCross(ImDrawList* draw, ImVec2 center, float radius) {
    draw->AddCircleFilled(center, radius, IM_COL32(16, 26, 48, 230));
    draw->AddCircle(center, radius, IM_COL32(0, 240, 255, 180), 24, 1.2f);
    float d = radius * 0.44f;
    ImU32 col = IM_COL32(0, 240, 255, 255);
    draw->AddLine(ImVec2(center.x - d, center.y - d), ImVec2(center.x + d, center.y + d), col, 2.0f);
    draw->AddLine(ImVec2(center.x + d, center.y - d), ImVec2(center.x - d, center.y + d), col, 2.0f);
}

static void DrawPSCircle(ImDrawList* draw, ImVec2 center, float radius) {
    draw->AddCircleFilled(center, radius, IM_COL32(40, 20, 25, 230));
    draw->AddCircle(center, radius, IM_COL32(255, 71, 87, 180), 24, 1.2f);
    draw->AddCircle(center, radius * 0.46f, IM_COL32(255, 71, 87, 255), 24, 2.0f);
}

static void DrawPSTriangle(ImDrawList* draw, ImVec2 center, float radius) {
    draw->AddCircleFilled(center, radius, IM_COL32(18, 38, 26, 230));
    draw->AddCircle(center, radius, IM_COL32(46, 213, 115, 180), 24, 1.2f);
    float d = radius * 0.50f;
    ImVec2 p1(center.x, center.y - d);
    ImVec2 p2(center.x - d * 0.86f, center.y + d * 0.55f);
    ImVec2 p3(center.x + d * 0.86f, center.y + d * 0.55f);
    draw->AddTriangle(p1, p2, p3, IM_COL32(46, 213, 115, 255), 2.0f);
}

static void DrawPSSquare(ImDrawList* draw, ImVec2 center, float radius) {
    draw->AddCircleFilled(center, radius, IM_COL32(38, 20, 36, 230));
    draw->AddCircle(center, radius, IM_COL32(255, 107, 129, 180), 24, 1.2f);
    float d = radius * 0.40f;
    draw->AddRect(ImVec2(center.x - d, center.y - d), ImVec2(center.x + d, center.y + d),
        IM_COL32(255, 107, 129, 255), 1.5f, 0, 2.0f);
}

static void DrawPSBumper(ImDrawList* draw, ImVec2 pos, const char* label) {
    ImVec2 size(34.0f, 20.0f);
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(28, 38, 58, 240), 5.0f);
    draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 130, 175, 180), 5.0f, 0, 1.0f);
    ImVec2 tsz = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(pos.x + (size.x - tsz.x) * 0.5f, pos.y + (size.y - tsz.y) * 0.5f),
        IM_COL32(220, 235, 255, 255), label);
}

// ─── Ambient Shaders & Atmosphere ─────────────────────────
void App::draw_ambient_background() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float time = static_cast<float>(ImGui::GetTime());

    // Deep Void Navy Background (#0b1021)
    draw->AddRectFilled(ImVec2(0, 0), ws, IM_COL32(11, 16, 33, 255));

    // Ambient Radial Light Spill (PlayStation Blue glow on top-left)
    ImVec2 c1(ws.x * 0.20f, ws.y * 0.15f);
    draw->AddCircleFilled(c1, ws.x * 0.45f, IM_COL32(0, 112, 209, 24), 64);
    draw->AddCircleFilled(c1, ws.x * 0.25f, IM_COL32(0, 112, 209, 38), 64);

    // Neon Cyan glow on bottom-right
    ImVec2 c2(ws.x * 0.85f, ws.y * 0.80f);
    draw->AddCircleFilled(c2, ws.x * 0.40f, IM_COL32(0, 240, 255, 18), 64);
    draw->AddCircleFilled(c2, ws.x * 0.20f, IM_COL32(0, 240, 255, 30), 64);

    // Subtle PS Plus Purple glow on top-right
    ImVec2 c3(ws.x * 0.80f, ws.y * 0.10f);
    draw->AddCircleFilled(c3, ws.x * 0.35f, IM_COL32(121, 40, 202, 22), 64);

    // Animated PlayStation Portal Wave ribbons
    for (int w = 0; w < 3; w++) {
        float speed = 0.35f + w * 0.15f;
        float amp = 24.0f + w * 14.0f;
        float y_offset = ws.y * 0.48f + w * 45.0f;
        float phase = time * speed + w * 1.8f;
        ImU32 wave_col = (w == 0) ? IM_COL32(0, 240, 255, 30) :
                         (w == 1) ? IM_COL32(0, 112, 209, 36) :
                                    IM_COL32(121, 40, 202, 26);

        int steps = 40;
        float step_x = ws.x / static_cast<float>(steps);
        for (int i = 0; i < steps; i++) {
            float x1 = i * step_x;
            float x2 = (i + 1) * step_x;
            float y1 = y_offset + sinf(x1 * 0.0035f + phase) * amp + cosf(x1 * 0.0018f - phase * 0.5f) * (amp * 0.5f);
            float y2 = y_offset + sinf(x2 * 0.0035f + phase) * amp + cosf(x2 * 0.0018f - phase * 0.5f) * (amp * 0.5f);
            draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), wave_col, 2.5f - w * 0.5f);
        }
    }
}

// ─── 10-Foot Console Top Navigation Bar ───────────────────
void App::draw_top_navigation_bar() {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float bar_height = 68.0f;

    // Glass bar background
    draw->AddRectFilled(ImVec2(0, 0), ImVec2(ws.x, bar_height), IM_COL32(8, 12, 26, 215));
    draw->AddLine(ImVec2(0, bar_height), ImVec2(ws.x, bar_height), IM_COL32(255, 255, 255, 18), 1.0f);

    // Left: Logo + Branding
    float cur_x = 28.0f;
    ImTextureID ps_tex = get_texture("playstation");
    if (ps_tex) {
        draw->AddImage(ps_tex, ImVec2(cur_x, 19.0f), ImVec2(cur_x + 30.0f, 49.0f));
        cur_x += 38.0f;
    } else {
        // Fallback PlayStation logo vector glyph
        DrawPSCross(draw, ImVec2(cur_x + 15.0f, 34.0f), 12.0f);
        cur_x += 38.0f;
    }

    if (font_title_) ImGui::PushFont(font_title_);
    draw->AddText(ImVec2(cur_x, 18.0f), IM_COL32(255, 255, 255, 255), "PORTAL");
    cur_x += ImGui::CalcTextSize("PORTAL").x + 8.0f;
    if (font_title_) ImGui::PopFont();

    // "PC" / "PRO" cyan badge
    ImVec2 badge_size(36.0f, 18.0f);
    ImVec2 badge_pos(cur_x, 26.0f);
    draw->AddRectFilled(badge_pos, ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
        IM_COL32(0, 240, 255, 45), 5.0f);
    draw->AddRect(badge_pos, ImVec2(badge_pos.x + badge_size.x, badge_pos.y + badge_size.y),
        IM_COL32(0, 240, 255, 200), 5.0f, 0, 1.0f);
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(badge_pos.x + 7.0f, badge_pos.y + 1.0f), IM_COL32(0, 240, 255, 255), "PC");
    if (font_small_) ImGui::PopFont();

    // Center: 10-Foot Console Tabs (Consolas, PS Plus Cloud, Ajustes)
    const char* tabs[] = {"CONSOLAS", "PS PLUS CLOUD", "AJUSTES"};
    float tab_widths[] = {120.0f, 150.0f, 110.0f};
    float total_tabs_w = 120.0f + 150.0f + 110.0f + 20.0f * 2.0f + 80.0f;
    float tab_start_x = (ws.x - total_tabs_w) * 0.5f;

    // L1 Bumper hint
    DrawPSBumper(draw, ImVec2(tab_start_x, 24.0f), "L1");
    tab_start_x += 44.0f;

    for (int i = 0; i < 3; i++) {
        float tw = tab_widths[i];
        ImVec2 t_min(tab_start_x, 15.0f);
        ImVec2 t_max(tab_start_x + tw, 53.0f);
        bool active = (current_tab_ == i);

        // Tab click detection
        ImGui::SetCursorScreenPos(t_min);
        if (ImGui::InvisibleButton(std::format("##tab_{}", i).c_str(), ImVec2(tw, 38.0f))) {
            current_tab_ = i;
            if (i == 0) current_screen_ = Screen::Home;
            else if (i == 1) current_screen_ = Screen::CloudGames;
            else if (i == 2) current_screen_ = Screen::Settings;
        }
        bool hovered = ImGui::IsItemHovered();

        if (active) {
            draw->AddRectFilled(t_min, t_max, IM_COL32(0, 112, 209, 50), 8.0f);
            // Glowing cyan underline
            draw->AddLine(ImVec2(t_min.x + 8.0f, bar_height - 2.0f),
                          ImVec2(t_max.x - 8.0f, bar_height - 2.0f),
                          IM_COL32(0, 240, 255, 255), 3.0f);
        } else if (hovered) {
            draw->AddRectFilled(t_min, t_max, IM_COL32(255, 255, 255, 15), 8.0f);
        }

        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        ImVec2 label_sz = ImGui::CalcTextSize(tabs[i]);
        ImVec2 label_pos(t_min.x + (tw - label_sz.x) * 0.5f, t_min.y + (38.0f - label_sz.y) * 0.5f);
        ImU32 text_col = active ? IM_COL32(255, 255, 255, 255) :
                         hovered ? IM_COL32(220, 235, 255, 220) :
                                   IM_COL32(140, 160, 185, 190);
        draw->AddText(label_pos, text_col, tabs[i]);
        if (font_subtitle_) ImGui::PopFont();

        tab_start_x += tw + 18.0f;
    }

    // R1 Bumper hint
    DrawPSBumper(draw, ImVec2(tab_start_x, 24.0f), "R1");

    // Right: Status Badges (LAN, PSN Profile, Clock)
    float right_x = ws.x - 30.0f;

    // Clock
    std::time_t now = std::time(nullptr);
    std::tm* ltm = std::localtime(&now);
    char time_str[16];
    std::snprintf(time_str, sizeof(time_str), "%02d:%02d", ltm->tm_hour, ltm->tm_min);
    if (font_body_) ImGui::PushFont(font_body_);
    ImVec2 csz = ImGui::CalcTextSize(time_str);
    right_x -= csz.x;
    draw->AddText(ImVec2(right_x, 24.0f), IM_COL32(230, 240, 255, 240), time_str);
    if (font_body_) ImGui::PopFont();
    right_x -= 24.0f;

    // PSN Profile capsule
    float psn_w = 175.0f;
    right_x -= psn_w;
    ImVec2 psn_min(right_x, 16.0f);
    ImVec2 psn_max(right_x + psn_w, 52.0f);
    draw->AddRectFilled(psn_min, psn_max, IM_COL32(20, 30, 54, 200), 18.0f);
    draw->AddRect(psn_min, psn_max, IM_COL32(0, 112, 209, 120), 18.0f, 0, 1.0f);

    // Profile avatar circle
    ImVec2 avatar_c(psn_min.x + 18.0f, psn_min.y + 18.0f);
    draw->AddCircleFilled(avatar_c, 12.0f, IM_COL32(0, 112, 209, 255));
    DrawPSCross(draw, avatar_c, 8.0f);
    // Online green dot
    draw->AddCircleFilled(ImVec2(avatar_c.x + 8.0f, avatar_c.y + 8.0f), 3.5f, IM_COL32(0, 230, 118, 255));

    if (font_small_) ImGui::PushFont(font_small_);
    std::string profile_name = "PSN User";
    if (account_manager_) {
        auto act = account_manager_->get_active_account();
        if (act && !act->profile.online_id.empty()) {
            profile_name = act->profile.online_id;
        }
    }
    draw->AddText(ImVec2(psn_min.x + 36.0f, psn_min.y + 6.0f), IM_COL32(245, 250, 255, 255), profile_name.c_str());
    // "PS Plus" gold tag
    draw->AddText(ImVec2(psn_min.x + 36.0f, psn_min.y + 20.0f), IM_COL32(255, 205, 50, 220), "PS+ Premium");
    if (font_small_) ImGui::PopFont();
    right_x -= 16.0f;

    // LAN / WiFi Signal indicator
    float net_w = 135.0f;
    right_x -= net_w;
    ImVec2 net_min(right_x, 18.0f);
    ImVec2 net_max(right_x + net_w, 50.0f);
    draw->AddRectFilled(net_min, net_max, IM_COL32(16, 24, 44, 180), 16.0f);
    draw->AddRect(net_min, net_max, IM_COL32(60, 85, 130, 100), 16.0f, 0, 1.0f);

    // Signal green dot
    draw->AddCircleFilled(ImVec2(net_min.x + 14.0f, net_min.y + 16.0f), 4.0f, IM_COL32(0, 230, 118, 255));
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(net_min.x + 24.0f, net_min.y + 8.0f), IM_COL32(180, 205, 235, 230), "LAN 1 Gbps • 2ms");
    if (font_small_) ImGui::PopFont();
}

void App::probe_consoles_background() {
    if (is_probing_.exchange(true)) return;

    auto reg = console_registry_;
    probe_thread_ = std::jthread([this, reg](std::stop_token st) {
        if (!reg) {
            is_probing_ = false;
            return;
        }
        auto list = reg->get_all_consoles();
        for (const auto& c : list) {
            if (st.stop_requested()) break;
            if (c.address.empty()) continue;
            auto state = portal::discovery::DDPDiscovery::probe_console(c.address, c.port, 350);
            reg->update_console_state(c.host_id, state, c.address);
        }
        is_probing_ = false;
    });
}

void App::wake_and_connect(const portal::discovery::RegisteredConsole& console) {
    if (is_waking_.load()) return;
    is_waking_ = true;
    waking_status_text_ = std::format("Despertando {}...", console.host_name);
    show_toast(std::format("Enviando señal de encendido (Wake-on-LAN) a {}...", console.host_name), 5.0f);

    // Save preferred console for next startup
    if (!console.host_id.empty()) {
        settings_.preferred_console_host_id = console.host_id;
        (void)settings_.save(settings_path_);
    }

    auto reg = console_registry_;
    auto sm = session_manager_;
    auto console_copy = console;

    connect_thread_ = std::jthread([this, reg, sm, console_copy](std::stop_token st) {
        bool is_ps5 = (console_copy.host_type == "PS5" || console_copy.host_type.find("5") != std::string::npos);
        
        // Step 1: Send initial wake packet
        (void)portal::discovery::DDPDiscovery::wake(console_copy.address, console_copy.rp_auth, is_ps5);

        // Step 2: Probe console up to 10 seconds (every 500ms) waiting for it to become Awake
        auto start = std::chrono::steady_clock::now();
        bool awake = false;
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // Send another wake pulse
            (void)portal::discovery::DDPDiscovery::wake(console_copy.address, console_copy.rp_auth, is_ps5);
            auto state = portal::discovery::DDPDiscovery::probe_console(console_copy.address, console_copy.port, 400);
            if (state == portal::ConsoleState::Awake) {
                awake = true;
                reg->update_console_state(console_copy.host_id, state, console_copy.address);
                break;
            }
        }

        is_waking_ = false;

        // Step 3: Connect
        portal::StreamConfig cfg;
        cfg.resolution = portal::Resolution::R1080p;
        cfg.fps = portal::FrameRate::FPS60;
        cfg.codec = is_ps5 ? portal::VideoCodec::H265 : portal::VideoCodec::H264;
        cfg.bitrate_kbps = settings_.bitrate_kbps;
        cfg.hdr = false;

        current_screen_ = Screen::Streaming;
        SDL_SetWindowFullscreen(window_, true);
        show_toast("PlayStation 5 lista. Iniciando sesion...", 3.0f);

        auto res = sm->connect_local(console_copy, cfg);
        if (!res.has_value()) {
            spdlog::error("connect_local failed after wake: {}", res.error().message);
            SDL_SetWindowFullscreen(window_, false);
            current_screen_ = Screen::Home;
            show_toast(std::format("Error al conectar: {}", res.error().message), 6.0f);
        }
    });
}

// ─── Dashboard de Consolas (10-Foot Console First) ─────────
void App::draw_home_screen() {
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float start_y = 95.0f;

    // Section Header
    ImGui::SetCursorPos(ImVec2(40.0f, start_y));
    if (font_title_) ImGui::PushFont(font_title_);
    ImGui::TextColored(colors::kTextPrimary, "MIS CONSOLAS PLAYSTATION");
    if (font_title_) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(40.0f, start_y + 36.0f));
    if (font_small_) ImGui::PushFont(font_small_);
    ImGui::TextColored(colors::kTextSecondary, "Selecciona una consola vinculada para iniciar Remote Play en ultra baja latencia");
    if (font_small_) ImGui::PopFont();

    // ── Console Cards Row ──
    float card_w = 340.0f;
    float card_h = 245.0f;
    float card_spacing = 24.0f;
    float cards_y = start_y + 70.0f;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    auto consoles = console_registry_->get_all_consoles();

    // ── Dynamic Console Cards ──
    int card_idx = 0;
    for (const auto& c : consoles) {
        ImVec2 pos(40.0f + card_idx * (card_w + card_spacing), cards_y);
        ImVec2 pos_end(pos.x + card_w, pos.y + card_h);

        ImGui::SetCursorScreenPos(pos);
        bool is_hovered = ImGui::IsMouseHoveringRect(pos, pos_end);

        // Card Glass Background
        ImU32 bg_color = is_hovered ? IM_COL32(18, 28, 56, 240) : IM_COL32(12, 19, 38, 220);
        draw->AddRectFilled(pos, pos_end, bg_color, 16.0f);

        // Glowing border (Cyan neon on focus/hover)
        if (is_hovered) {
            draw->AddRect(ImVec2(pos.x - 2, pos.y - 2), ImVec2(pos_end.x + 2, pos_end.y + 2),
                IM_COL32(0, 240, 255, 60), 18.0f, 0, 4.0f);
            draw->AddRect(pos, pos_end, IM_COL32(0, 240, 255, 255), 16.0f, 0, 2.0f);
        } else {
            draw->AddRect(pos, pos_end, IM_COL32(0, 112, 209, 130), 16.0f, 0, 1.2f);
        }

        // Top specular reflection line
        draw->AddLine(ImVec2(pos.x + 16.0f, pos.y + 1.0f), ImVec2(pos_end.x - 16.0f, pos.y + 1.0f),
            IM_COL32(255, 255, 255, 45), 1.0f);

        // Header: Badge + Icon
        bool is_ps5 = (c.host_type == "PS5" || c.host_type.find("5") != std::string::npos);
        ImTextureID c_icon = get_texture(is_ps5 ? "ps5" : "ps4");
        if (c_icon) {
            draw->AddImage(c_icon, ImVec2(pos.x + 20.0f, pos.y + 18.0f), ImVec2(pos.x + 50.0f, pos.y + 48.0f));
        } else {
            draw->AddRectFilled(ImVec2(pos.x + 20.0f, pos.y + 18.0f), ImVec2(pos.x + 52.0f, pos.y + 44.0f),
                is_ps5 ? IM_COL32(0, 112, 209, 255) : IM_COL32(60, 40, 110, 255), 6.0f);
            draw->AddText(ImVec2(pos.x + 24.0f, pos.y + 22.0f), IM_COL32(255, 255, 255, 255), is_ps5 ? "PS5" : "PS4");
        }

        // Console Name
        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        draw->AddText(ImVec2(pos.x + 60.0f, pos.y + 18.0f), IM_COL32(255, 255, 255, 255), c.host_name.c_str());
        if (font_subtitle_) ImGui::PopFont();

        // Dynamic Status Pill: Online / Awake (Green) vs Standby (Amber) vs Unknown (Gray)
        float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 4.0f);
        ImVec2 dot_pos(pos.x + 66.0f, pos.y + 48.0f);

        bool is_awake = (c.state == portal::ConsoleState::Awake);
        bool is_standby = (c.state == portal::ConsoleState::Standby);

        if (is_awake) {
            draw->AddCircleFilled(dot_pos, 7.0f + pulse * 2.0f, IM_COL32(0, 230, 118, static_cast<int>(60 * pulse)));
            draw->AddCircleFilled(dot_pos, 5.0f, IM_COL32(0, 230, 118, 255));
            if (font_small_) ImGui::PushFont(font_small_);
            draw->AddText(ImVec2(dot_pos.x + 12.0f, dot_pos.y - 7.0f), IM_COL32(0, 230, 118, 255), "Encendida • Lista para streaming");
            if (font_small_) ImGui::PopFont();
        } else if (is_standby) {
            draw->AddCircleFilled(dot_pos, 7.0f + pulse * 2.0f, IM_COL32(255, 171, 0, static_cast<int>(70 * pulse)));
            draw->AddCircleFilled(dot_pos, 5.0f, IM_COL32(255, 171, 0, 255));
            if (font_small_) ImGui::PushFont(font_small_);
            draw->AddText(ImVec2(dot_pos.x + 12.0f, dot_pos.y - 7.0f), IM_COL32(255, 171, 0, 255), "En Modo Reposo (Standby)");
            if (font_small_) ImGui::PopFont();
        } else {
            draw->AddCircleFilled(dot_pos, 5.0f, IM_COL32(140, 155, 180, 180));
            if (font_small_) ImGui::PushFont(font_small_);
            draw->AddText(ImVec2(dot_pos.x + 12.0f, dot_pos.y - 7.0f), IM_COL32(140, 155, 180, 220), "En Reposo / Fuera de Red");
            if (font_small_) ImGui::PopFont();
        }

        // Specs Grid
        float spec_y = pos.y + 75.0f;
        draw->AddLine(ImVec2(pos.x + 20.0f, spec_y), ImVec2(pos_end.x - 20.0f, spec_y), IM_COL32(255, 255, 255, 18), 1.0f);

        if (font_small_) ImGui::PushFont(font_small_);
        draw->AddText(ImVec2(pos.x + 20.0f, spec_y + 8.0f), IM_COL32(160, 175, 205, 240), "Direccion IP:");
        std::string ip_str = c.address + " : " + std::to_string(c.port) + " (LAN)";
        draw->AddText(ImVec2(pos.x + 140.0f, spec_y + 8.0f), IM_COL32(235, 245, 255, 255), ip_str.c_str());

        draw->AddText(ImVec2(pos.x + 20.0f, spec_y + 25.0f), IM_COL32(160, 175, 205, 240), "Resolucion/FPS:");
        draw->AddText(ImVec2(pos.x + 140.0f, spec_y + 25.0f), IM_COL32(0, 240, 255, 255), is_ps5 ? "1080p @ 60 FPS HDR" : "1080p @ 60 FPS SDR");

        draw->AddText(ImVec2(pos.x + 20.0f, spec_y + 42.0f), IM_COL32(160, 175, 205, 240), "PIN de Usuario:");
        std::string pin_label = c.login_pin.empty() ? "Sin configurar (Configurar)" : "•••• (Guardado)";
        ImU32 pin_color = c.login_pin.empty() ? IM_COL32(255, 171, 0, 230) : IM_COL32(0, 230, 118, 255);
        draw->AddText(ImVec2(pos.x + 140.0f, spec_y + 42.0f), pin_color, pin_label.c_str());
        if (font_small_) ImGui::PopFont();

        // Invisible button over PIN row to edit/set PIN
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 20.0f, spec_y + 38.0f));
        if (ImGui::InvisibleButton(std::format("##edit_pin_{}", card_idx).c_str(), ImVec2(card_w - 40.0f, 20.0f))) {
            target_login_console_host_id_ = c.host_id;
            active_login_pin_digit_ = 0;
            memset(login_pin_digits_, 0, sizeof(login_pin_digits_));
            if (c.login_pin.size() == 4) {
                for (int p = 0; p < 4; ++p) login_pin_digits_[p] = c.login_pin[p];
            }
            show_login_pin_modal_ = true;
        }

        // Action Button: Glowing PlayStation Blue (Awake) or Amber / Gold (Wake & Connect)
        ImVec2 btn_pos(pos.x + 20.0f, pos.y + card_h - 52.0f);
        ImVec2 btn_size(card_w - 40.0f, 38.0f);
        ImGui::SetCursorScreenPos(btn_pos);

        if (is_awake) {
            ImGui::PushStyleColor(ImGuiCol_Button, colors::kPrimary);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kPrimaryHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.00f, 0.35f, 0.70f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.48f, 0.00f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.58f, 0.05f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.38f, 0.00f, 1.00f));
        }

        std::string btn_id = "##connect_" + std::to_string(card_idx);
        if (ImGui::Button(btn_id.c_str(), btn_size)) {
            target_login_console_host_id_ = c.host_id;
            if (is_awake) {
                show_toast(std::format("Iniciando conexion Remote Play con {}...", c.host_name), 4.0f);
                
                portal::StreamConfig cfg;
                cfg.resolution = portal::Resolution::R1080p;
                cfg.fps = portal::FrameRate::FPS60;
                cfg.codec = is_ps5 ? portal::VideoCodec::H265 : portal::VideoCodec::H264;
                cfg.bitrate_kbps = settings_.bitrate_kbps;
                cfg.hdr = false;
                
                current_screen_ = Screen::Streaming;
                SDL_SetWindowFullscreen(window_, true);
                auto sm = session_manager_;
                auto console_copy = c;
                connect_thread_ = std::jthread([this, sm, console_copy, cfg](std::stop_token) {
                    auto res = sm->connect_local(console_copy, cfg);
                    if (!res.has_value()) {
                        spdlog::error("connect_local failed: {}", res.error().message);
                        SDL_SetWindowFullscreen(window_, false);
                        current_screen_ = Screen::Home;
                        show_toast(std::format("Error al conectar: {}", res.error().message), 6.0f);
                    }
                });
            } else {
                wake_and_connect(c);
            }
        }
        ImGui::PopStyleColor(3);

        // Vector Circle Icon and Button Text inside button
        const char* btn_text = is_awake ? "CONECTAR AHORA" : "DESPERTAR Y CONECTAR";
        DrawPSCircle(draw, ImVec2(btn_pos.x + 24.0f, btn_pos.y + 19.0f), 9.0f);
        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        ImVec2 btxt_sz = ImGui::CalcTextSize(btn_text);
        draw->AddText(ImVec2(btn_pos.x + (btn_size.x - btxt_sz.x) * 0.5f + 10.0f, btn_pos.y + (btn_size.y - btxt_sz.y) * 0.5f),
            IM_COL32(255, 255, 255, 255), btn_text);
        if (font_subtitle_) ImGui::PopFont();

        card_idx++;
    }

    // Card: + Vincular Nueva Consola
    {
        ImVec2 pos(40.0f + card_idx * (card_w + card_spacing), cards_y);
        ImVec2 pos_end(pos.x + card_w, pos.y + card_h);

        ImGui::SetCursorScreenPos(pos);
        bool is_hovered = ImGui::IsMouseHoveringRect(pos, pos_end);

        ImU32 bg_color = is_hovered ? IM_COL32(14, 24, 50, 180) : IM_COL32(9, 14, 28, 150);
        draw->AddRectFilled(pos, pos_end, bg_color, 16.0f);

        // Dashed glowing border
        ImU32 border_col = is_hovered ? IM_COL32(0, 240, 255, 240) : IM_COL32(0, 112, 209, 140);
        draw->AddRect(pos, pos_end, border_col, 16.0f, 0, is_hovered ? 2.0f : 1.5f);

        // Big '+' Icon in Center
        ImVec2 icon_c(pos.x + card_w * 0.5f, pos.y + card_h * 0.38f);
        draw->AddCircleFilled(icon_c, 26.0f, is_hovered ? IM_COL32(0, 240, 255, 45) : IM_COL32(0, 112, 209, 35));
        draw->AddCircle(icon_c, 26.0f, border_col, 32, 1.5f);

        // '+' lines
        draw->AddLine(ImVec2(icon_c.x - 12.0f, icon_c.y), ImVec2(icon_c.x + 12.0f, icon_c.y), IM_COL32(0, 240, 255, 255), 2.5f);
        draw->AddLine(ImVec2(icon_c.x, icon_c.y - 12.0f), ImVec2(icon_c.x, icon_c.y + 12.0f), IM_COL32(0, 240, 255, 255), 2.5f);

        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        const char* title = "Vincular Consola";
        ImVec2 tsz = ImGui::CalcTextSize(title);
        draw->AddText(ImVec2(pos.x + (card_w - tsz.x) * 0.5f, pos.y + card_h * 0.62f),
            IM_COL32(245, 250, 255, 255), title);
        if (font_subtitle_) ImGui::PopFont();

        if (font_small_) ImGui::PushFont(font_small_);
        const char* sub = "PIN de 8 casillas o DDP Broadcast";
        ImVec2 ssz = ImGui::CalcTextSize(sub);
        draw->AddText(ImVec2(pos.x + (card_w - ssz.x) * 0.5f, pos.y + card_h * 0.76f),
            IM_COL32(140, 165, 195, 220), sub);
        if (font_small_) ImGui::PopFont();

        // Invisible button over whole card
        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton("##add_console_card", ImVec2(card_w, card_h))) {
            show_pin_modal_ = true;
            active_pin_digit_ = 0;
            for (int d = 0; d < 8; d++) pin_digits_[d] = '\0';
        }
    }

    // ── Quick Actions Row ──
    float actions_y = cards_y + card_h + 36.0f;
    ImGui::SetCursorPos(ImVec2(40.0f, actions_y));
    if (font_subtitle_) ImGui::PushFont(font_subtitle_);
    ImGui::TextColored(colors::kTextSecondary, "ACCIONES RAPIDAS");
    if (font_subtitle_) ImGui::PopFont();

    float action_btn_w = 260.0f;
    float action_btn_h = 44.0f;

    // Button 1: Escanear Red (DDP)
    ImGui::SetCursorPos(ImVec2(40.0f, actions_y + 30.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kSurfaceHover);
    if (ImGui::Button("##scan_btn", ImVec2(action_btn_w, action_btn_h))) {
        show_toast("Escaneando red local (DDP Broadcast 9295)...", 3.0f);
        auto result = portal::discovery::DDPDiscovery::search(1200);
        if (result) {
            show_toast(std::format("Escaneo finalizado: {} consolas detectadas", result->size()), 3.5f);
        }
    }
    ImGui::PopStyleColor(2);

    ImVec2 b1_pos(40.0f, actions_y + 30.0f);
    DrawPSTriangle(draw, ImVec2(b1_pos.x + 22.0f, b1_pos.y + 22.0f), 9.0f);
    if (font_body_) ImGui::PushFont(font_body_);
    draw->AddText(ImVec2(b1_pos.x + 40.0f, b1_pos.y + 12.0f), IM_COL32(235, 245, 255, 255), "Buscar Consolas (DDP)");
    if (font_body_) ImGui::PopFont();

    // Button 2: Diagnostico LAN
    ImGui::SetCursorPos(ImVec2(40.0f + action_btn_w + 20.0f, actions_y + 30.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kSurfaceHover);
    if (ImGui::Button("##diag_btn", ImVec2(action_btn_w, action_btn_h))) {
        std::string diag_str = "Diagnostico LAN: Red local OK • MTU 1450";
        if (console_registry_) {
            auto all = console_registry_->get_all_consoles();
            if (!all.empty() && !all.front().address.empty()) {
                diag_str = std::format("Diagnostico LAN: {} OK • MTU 1450", all.front().address);
            }
        }
        show_toast(diag_str, 4.0f);
    }
    ImGui::PopStyleColor(2);

    ImVec2 b2_pos(40.0f + action_btn_w + 20.0f, actions_y + 30.0f);
    DrawPSSquare(draw, ImVec2(b2_pos.x + 22.0f, b2_pos.y + 22.0f), 9.0f);
    if (font_body_) ImGui::PushFont(font_body_);
    draw->AddText(ImVec2(b2_pos.x + 40.0f, b2_pos.y + 12.0f), IM_COL32(235, 245, 255, 255), "Diagnostico de Red LAN");
    if (font_body_) ImGui::PopFont();

    // Button 3: Ajustes
    ImGui::SetCursorPos(ImVec2(40.0f + (action_btn_w + 20.0f) * 2.0f, actions_y + 30.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kSurfaceHover);
    if (ImGui::Button("##settings_btn", ImVec2(action_btn_w, action_btn_h))) {
        current_tab_ = 2;
    }
    ImGui::PopStyleColor(2);

    ImVec2 b3_pos(40.0f + (action_btn_w + 20.0f) * 2.0f, actions_y + 30.0f);
    DrawPSCross(draw, ImVec2(b3_pos.x + 22.0f, b3_pos.y + 22.0f), 9.0f);
    if (font_body_) ImGui::PushFont(font_body_);
    draw->AddText(ImVec2(b3_pos.x + 40.0f, b3_pos.y + 12.0f), IM_COL32(235, 245, 255, 255), "Ajustes de Streaming");
    if (font_body_) ImGui::PopFont();
}

void App::draw_console_list() {
    draw_home_screen();
}

// ─── Catalogo Cloud de PlayStation Plus ───────────────────
struct CloudGameDef {
    const char* title;
    const char* studio;
    const char* genre;
    const char* platform;
    const char* rating;
    const char* stream_spec;
    ImU32 color_top;
    ImU32 color_bottom;
};

static const CloudGameDef kCloudGamesCatalog[] = {
    {"Marvel's Spider-Man 2", "Insomniac Games", "Accion / Superheroes", "PS5", "★ 4.9", "4K 60FPS • Takion HDR", IM_COL32(190, 25, 35, 255), IM_COL32(35, 12, 22, 255)},
    {"God of War Ragnarok", "Santa Monica Studio", "Aventura Mitica", "PS5", "★ 5.0", "1080p 60FPS • 3D Audio", IM_COL32(25, 80, 160, 255), IM_COL32(12, 28, 55, 255)},
    {"Horizon Forbidden West", "Guerrilla Games", "Mundo Abierto Sci-Fi", "PS5", "★ 4.8", "1080p 60FPS • HDR10", IM_COL32(0, 150, 130, 255), IM_COL32(12, 45, 40, 255)},
    {"Returnal", "Housemarque", "Roguelike Sci-Fi", "PS5", "★ 4.7", "1080p 60FPS • Low-Latency", IM_COL32(45, 130, 85, 255), IM_COL32(18, 40, 28, 255)},
    {"Ghost of Tsushima: DC", "Sucker Punch", "Accion Samurai", "PS5", "★ 4.9", "1080p 60FPS • HDR", IM_COL32(170, 65, 25, 255), IM_COL32(48, 18, 12, 255)},
    {"The Last of Us Part I", "Naughty Dog", "Supervivencia Narrativa", "PS5", "★ 4.9", "1080p 60FPS • Tempest 3D", IM_COL32(75, 95, 45, 255), IM_COL32(22, 32, 18, 255)},
    {"Demon's Souls", "Bluepoint Games", "Action RPG", "PS5", "★ 4.8", "1080p 60FPS • HDR", IM_COL32(65, 70, 85, 255), IM_COL32(22, 22, 32, 255)},
    {"Gran Turismo 7", "Polyphony Digital", "Simulacion Carreras", "PS5", "★ 4.7", "1080p 60FPS • Ray Tracing", IM_COL32(15, 90, 180, 255), IM_COL32(12, 28, 65, 255)},
    {"Ratchet & Clank: Rift Apart", "Insomniac Games", "Plataformas / Aventura", "PS5", "★ 4.8", "1080p 60FPS • DualSense", IM_COL32(140, 35, 150, 255), IM_COL32(38, 12, 48, 255)},
    {"Astro's Playroom", "Team Asobi", "Plataformas DualSense", "PS5", "★ 4.9", "1080p 60FPS • Haptics", IM_COL32(0, 130, 225, 255), IM_COL32(12, 45, 85, 255)},
    {"Death Stranding DC", "Kojima Productions", "Sci-Fi Cinematico", "PS5", "★ 4.8", "1080p 60FPS • Ultrawide", IM_COL32(75, 80, 90, 255), IM_COL32(22, 25, 30, 255)},
    {"Uncharted: Legado de Ladrones", "Naughty Dog", "Aventura / Accion", "PS5", "★ 4.8", "1080p 60FPS", IM_COL32(150, 95, 35, 255), IM_COL32(42, 28, 12, 255)},
    {"Final Fantasy VII Rebirth", "Square Enix", "JRPG Fantasia", "PS5", "★ 4.9", "1080p 60FPS • Takion", IM_COL32(45, 85, 130, 255), IM_COL32(18, 32, 48, 255)},
    {"Bloodborne: Definitive", "FromSoftware", "Action RPG Gotico", "PS4", "★ 4.9", "1080p 30FPS", IM_COL32(95, 28, 38, 255), IM_COL32(30, 12, 18, 255)},
    {"The Witcher 3: Wild Hunt", "CD Projekt Red", "RPG Mundo Abierto", "PS5", "★ 4.9", "1080p 60FPS • HDR", IM_COL32(120, 40, 30, 255), IM_COL32(38, 14, 12, 255)},
    {"Cyberpunk 2077: Phantom Liberty", "CD Projekt Red", "RPG Sci-Fi", "PS5", "★ 4.8", "1080p 60FPS • Ray Tracing", IM_COL32(190, 160, 15, 255), IM_COL32(50, 40, 8, 255)}
};

void App::draw_cloud_games() {
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float start_y = 88.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Header & Section Title
    ImGui::SetCursorPos(ImVec2(40.0f, start_y));
    if (font_title_) ImGui::PushFont(font_title_);
    ImGui::TextColored(colors::kTextPrimary, "PLAYSTATION PLUS — CATALOGO CLOUD");
    if (font_title_) ImGui::PopFont();

    // Datacenter status pill on the right
    float pill_w = 460.0f;
    ImVec2 pill_pos(ws.x - pill_w - 40.0f, start_y + 4.0f);
    draw->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + 30.0f), IM_COL32(18, 26, 48, 220), 15.0f);
    draw->AddRect(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + 30.0f), IM_COL32(121, 40, 202, 140), 15.0f, 0, 1.0f);
    draw->AddCircleFilled(ImVec2(pill_pos.x + 15.0f, pill_pos.y + 15.0f), 4.5f, IM_COL32(0, 240, 255, 255));
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(pill_pos.x + 28.0f, pill_pos.y + 7.0f), IM_COL32(230, 240, 255, 240),
        "Servidor Cloud: EU-Madrid (14 ms) • Protocolo Takion • 1080p60 HDR");
    if (font_small_) ImGui::PopFont();

    // ── Filter Chips Row ──
    const char* categories[] = {
        "Todos (842)", "Exclusivos PS5", "Accion & Aventura", "RPGs", "Multijugador", "Clasicos PS"
    };
    float chip_x = 40.0f;
    float chip_y = start_y + 45.0f;

    for (int c = 0; c < 6; c++) {
        ImVec2 chip_sz(c == 0 ? 100.0f : c == 1 ? 135.0f : c == 2 ? 150.0f : 110.0f, 32.0f);
        ImVec2 c_min(chip_x, chip_y);
        ImVec2 c_max(chip_x + chip_sz.x, chip_y + chip_sz.y);
        bool selected = (cloud_selected_category_ == c);

        ImGui::SetCursorScreenPos(c_min);
        if (ImGui::InvisibleButton(std::format("##chip_{}", c).c_str(), chip_sz)) {
            cloud_selected_category_ = c;
        }
        bool hovered = ImGui::IsItemHovered();

        ImU32 chip_bg = selected ? IM_COL32(0, 112, 209, 180) :
                        hovered ? IM_COL32(25, 38, 65, 180) :
                                  IM_COL32(14, 20, 38, 160);
        draw->AddRectFilled(c_min, c_max, chip_bg, 16.0f);
        draw->AddRect(c_min, c_max, selected ? IM_COL32(0, 240, 255, 255) : IM_COL32(60, 85, 125, 120), 16.0f, 0, selected ? 1.5f : 1.0f);

        if (font_small_) ImGui::PushFont(font_small_);
        ImVec2 tsz = ImGui::CalcTextSize(categories[c]);
        draw->AddText(ImVec2(c_min.x + (chip_sz.x - tsz.x) * 0.5f, c_min.y + (chip_sz.y - tsz.y) * 0.5f),
            selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 195, 220, 240), categories[c]);
        if (font_small_) ImGui::PopFont();

        chip_x += chip_sz.x + 12.0f;
    }

    // Search bar
    float search_w = 280.0f;
    ImGui::SetCursorPos(ImVec2(ws.x - search_w - 40.0f, chip_y));
    ImGui::SetNextItemWidth(search_w);
    ImGui::InputTextWithHint("##cloud_search", "Buscar en 800+ juegos...", cloud_search_, sizeof(cloud_search_));

    // ── Game Grid / Carousel ──
    float grid_y = chip_y + 48.0f;
    float card_w = 195.0f;
    float card_h = 280.0f;
    float spacing_x = 20.0f;
    float spacing_y = 22.0f;

    int cols = static_cast<int>((ws.x - 80.0f + spacing_x) / (card_w + spacing_x));
    if (cols < 1) cols = 1;

    ImGui::SetCursorPos(ImVec2(40.0f, grid_y));
    ImGui::BeginChild("##cloud_grid_scroll", ImVec2(ws.x - 80.0f, ws.y - grid_y - 56.0f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar);

    ImVec2 scroll_origin = ImGui::GetCursorScreenPos();
    int visible_idx = 0;

    for (int i = 0; i < 16; i++) {
        const auto& game = kCloudGamesCatalog[i];

        // Filter search term
        if (cloud_search_[0] != '\0') {
            std::string st = cloud_search_;
            std::string gt = game.title;
            std::transform(st.begin(), st.end(), st.begin(), ::tolower);
            std::transform(gt.begin(), gt.end(), gt.begin(), ::tolower);
            if (gt.find(st) == std::string::npos) continue;
        }

        int col = visible_idx % cols;
        int row = visible_idx / cols;
        ImVec2 pos(scroll_origin.x + col * (card_w + spacing_x), scroll_origin.y + row * (card_h + spacing_y));
        ImVec2 pos_end(pos.x + card_w, pos.y + card_h);

        ImGui::SetCursorScreenPos(pos);
        bool is_hovered = ImGui::IsMouseHoveringRect(pos, pos_end);

        // Glass Poster Card with Gradient Cover
        draw->AddRectFilled(pos, pos_end, IM_COL32(12, 17, 34, 230), 14.0f);

        // Poster Artwork Gradient simulation
        float art_h = 165.0f;
        draw->AddRectFilledMultiColor(
            pos, ImVec2(pos_end.x, pos.y + art_h),
            game.color_top, game.color_top,
            game.color_bottom, game.color_bottom
        );

        // Neon Glow border on Hover
        if (is_hovered) {
            draw->AddRect(ImVec2(pos.x - 2, pos.y - 2), ImVec2(pos_end.x + 2, pos_end.y + 2),
                IM_COL32(0, 240, 255, 60), 16.0f, 0, 3.5f);
            draw->AddRect(pos, pos_end, IM_COL32(0, 240, 255, 255), 14.0f, 0, 2.0f);
        } else {
            draw->AddRect(pos, pos_end, IM_COL32(40, 60, 95, 110), 14.0f, 0, 1.0f);
        }

        // Platform Badge (PS5 or PS4)
        ImVec2 ptag_pos(pos.x + 12.0f, pos.y + 12.0f);
        draw->AddRectFilled(ptag_pos, ImVec2(ptag_pos.x + 40.0f, ptag_pos.y + 20.0f), IM_COL32(0, 112, 209, 220), 4.0f);
        if (font_small_) ImGui::PushFont(font_small_);
        draw->AddText(ImVec2(ptag_pos.x + 8.0f, ptag_pos.y + 2.0f), IM_COL32(255, 255, 255, 255), game.platform);

        // Rating tag (★ 4.9)
        ImVec2 rtag_pos(pos_end.x - 52.0f, pos.y + 12.0f);
        draw->AddRectFilled(rtag_pos, ImVec2(rtag_pos.x + 40.0f, rtag_pos.y + 20.0f), IM_COL32(15, 20, 35, 210), 4.0f);
        draw->AddText(ImVec2(rtag_pos.x + 6.0f, rtag_pos.y + 2.0f), IM_COL32(255, 210, 60, 255), game.rating);
        if (font_small_) ImGui::PopFont();

        // Card Content Details (Bottom half)
        float text_y = pos.y + art_h + 10.0f;
        if (font_small_) ImGui::PushFont(font_small_);
        draw->AddText(ImVec2(pos.x + 12.0f, text_y), IM_COL32(140, 160, 190, 220), game.genre);
        if (font_small_) ImGui::PopFont();

        if (font_body_) ImGui::PushFont(font_body_);
        draw->AddText(ImVec2(pos.x + 12.0f, text_y + 16.0f), IM_COL32(255, 255, 255, 255), game.title);
        if (font_body_) ImGui::PopFont();

        if (font_small_) ImGui::PushFont(font_small_);
        draw->AddText(ImVec2(pos.x + 12.0f, text_y + 36.0f), IM_COL32(0, 240, 255, 240), game.stream_spec);
        draw->AddText(ImVec2(pos.x + 12.0f, text_y + 54.0f), IM_COL32(120, 140, 165, 200), game.studio);
        if (font_small_) ImGui::PopFont();

        // Hover Overlay: Stream Prompt
        if (is_hovered) {
            ImVec2 play_btn_pos(pos.x + 12.0f, pos_end.y - 38.0f);
            ImVec2 play_btn_sz(card_w - 24.0f, 28.0f);
            draw->AddRectFilled(play_btn_pos, ImVec2(play_btn_pos.x + play_btn_sz.x, play_btn_pos.y + play_btn_sz.y),
                IM_COL32(0, 112, 209, 230), 6.0f);
            DrawPSCircle(draw, ImVec2(play_btn_pos.x + 16.0f, play_btn_pos.y + 14.0f), 7.0f);
            if (font_small_) ImGui::PushFont(font_small_);
            draw->AddText(ImVec2(play_btn_pos.x + 30.0f, play_btn_pos.y + 6.0f), IM_COL32(255, 255, 255, 255), "TRANSMITIR AHORA");
            if (font_small_) ImGui::PopFont();
        }

        // Clickable Button
        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton(std::format("##cloud_game_{}", i).c_str(), ImVec2(card_w, card_h))) {
            cloud_selected_game_ = i;
            // Launch cloud streaming session for the selected game
            auto acct = account_manager_ ? account_manager_->get_active_account() : std::nullopt;
            if (!acct.has_value()) {
                show_toast("Inicia sesión con tu cuenta PSN primero", 4.0f);
            } else {
                // Refresh token if needed, then start cloud session
                auto token_res = account_manager_->get_access_token();
                if (!token_res) {
                    show_toast("Error al obtener token PSN. Vuelve a iniciar sesión.", 4.0f);
                } else {
                    portal::auth::PSNTokens tokens;
                    tokens.access_token = token_res.value();

                    portal::StreamConfig cfg;
                    cfg.resolution = portal::Resolution::R1080p;
                    cfg.fps        = portal::FrameRate::FPS60;
                    cfg.codec      = portal::VideoCodec::H265;
                    cfg.hdr        = false;
                    cfg.bitrate_kbps = settings_.bitrate_kbps;

                    // Use game_id from catalog or the static list as fallback
                    std::string gid = std::string(game.title); // catalog id when real catalog is loaded

                    show_toast(std::format("Iniciando Cloud: {}...", game.title), 3.5f);
                    navigate_to(Screen::Streaming);

                    // Connect cloud in a background thread to avoid blocking the UI
                    cloud_thread_ = std::jthread([this, tokens, gid, cfg](std::stop_token) {
                        auto res = session_manager_->connect_cloud(tokens, gid, cfg);
                        if (!res) {
                            spdlog::error("[App] Cloud connect failed: {}", res.error().message);
                            show_toast(std::format("Error cloud: {}", res.error().message), 5.0f);
                            current_screen_ = Screen::CloudGames;
                        }
                    });
                }
            }
        }

        visible_idx++;
    }

    ImGui::EndChild();
}

// ─── Modal de Registro PIN de 8 Casillas ──────────────────
void App::draw_pin_modal() {
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float modal_w = 660.0f;
    float modal_h = 480.0f;
    ImVec2 modal_pos((ws.x - modal_w) * 0.5f, (ws.y - modal_h) * 0.5f);
    ImVec2 modal_end(modal_pos.x + modal_w, modal_pos.y + modal_h);

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    // Dark glass backdrop scrim over whole screen
    draw->AddRectFilled(ImVec2(0, 0), ws, IM_COL32(4, 7, 16, 215));

    // Modal Glass Container (#0b1021 with glowing border)
    draw->AddRectFilled(modal_pos, modal_end, IM_COL32(11, 16, 33, 250), 20.0f);
    // Cyan glow around modal
    draw->AddRect(ImVec2(modal_pos.x - 2, modal_pos.y - 2), ImVec2(modal_end.x + 2, modal_end.y + 2),
        IM_COL32(0, 240, 255, 45), 22.0f, 0, 3.0f);
    draw->AddRect(modal_pos, modal_end, IM_COL32(0, 240, 255, 180), 20.0f, 0, 1.5f);

    // Top reflection
    draw->AddLine(ImVec2(modal_pos.x + 20.0f, modal_pos.y + 1.0f),
                  ImVec2(modal_end.x - 20.0f, modal_pos.y + 1.0f),
                  IM_COL32(255, 255, 255, 40), 1.0f);

    // Header
    float cur_y = modal_pos.y + 24.0f;
    DrawPSCross(draw, ImVec2(modal_pos.x + 36.0f, cur_y + 12.0f), 12.0f);

    if (font_title_) ImGui::PushFont(font_title_);
    draw->AddText(ImVec2(modal_pos.x + 58.0f, cur_y - 2.0f), IM_COL32(255, 255, 255, 255), "Vincular Consola PlayStation 5");
    if (font_title_) ImGui::PopFont();

    cur_y += 38.0f;
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(modal_pos.x + 36.0f, cur_y), IM_COL32(160, 180, 210, 240),
        "1. En tu PS5: Ajustes > Sistema > Uso a distancia > Vincular dispositivo");
    draw->AddText(ImVec2(modal_pos.x + 36.0f, cur_y + 18.0f), IM_COL32(160, 180, 210, 240),
        "2. Introduce a continuacion el codigo temporal de 8 digitos que aparece en tu pantalla:");
    if (font_small_) ImGui::PopFont();

    // ── 8 INDIVIDUAL PIN CASILLAS (CASILLAS 0 a 7) ──
    cur_y += 48.0f;
    float box_w = 54.0f;
    float box_h = 66.0f;
    float box_spacing = 10.0f;
    float group_gap = 26.0f; // Gap between digit 3 and 4
    float total_boxes_w = 8 * box_w + 6 * box_spacing + group_gap;
    float boxes_start_x = modal_pos.x + (modal_w - total_boxes_w) * 0.5f;

    float bx = boxes_start_x;
    for (int i = 0; i < 8; i++) {
        if (i == 4) {
            // Draw central divider dash '—'
            float dash_x = bx + 4.0f;
            float dash_y = cur_y + box_h * 0.5f;
            draw->AddLine(ImVec2(dash_x, dash_y), ImVec2(dash_x + 14.0f, dash_y),
                IM_COL32(0, 240, 255, 180), 2.5f);
            bx += group_gap;
        }

        ImVec2 b_min(bx, cur_y);
        ImVec2 b_max(bx + box_w, cur_y + box_h);
        bool is_active = (active_pin_digit_ == i);
        bool has_digit = (pin_digits_[i] >= '0' && pin_digits_[i] <= '9');

        // Click to focus box
        ImGui::SetCursorScreenPos(b_min);
        if (ImGui::InvisibleButton(std::format("##pin_box_{}", i).c_str(), ImVec2(box_w, box_h))) {
            active_pin_digit_ = i;
        }
        bool is_hovered = ImGui::IsItemHovered();

        // Box background
        ImU32 box_bg = is_active ? IM_COL32(16, 28, 56, 250) :
                       is_hovered ? IM_COL32(18, 26, 48, 230) :
                                    IM_COL32(11, 19, 38, 210);
        draw->AddRectFilled(b_min, b_max, box_bg, 10.0f);

        // Box border
        if (is_active) {
            float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 6.0f);
            draw->AddRect(ImVec2(b_min.x - 2, b_min.y - 2), ImVec2(b_max.x + 2, b_max.y + 2),
                IM_COL32(0, 240, 255, static_cast<int>(80 + 80 * pulse)), 12.0f, 0, 2.5f);
            draw->AddRect(b_min, b_max, IM_COL32(0, 240, 255, 255), 10.0f, 0, 2.0f);
            // Underline cursor
            draw->AddLine(ImVec2(b_min.x + 10.0f, b_max.y - 8.0f),
                          ImVec2(b_max.x - 10.0f, b_max.y - 8.0f),
                          IM_COL32(0, 240, 255, static_cast<int>(200 + 55 * pulse)), 2.5f);
        } else if (has_digit) {
            draw->AddRect(b_min, b_max, IM_COL32(0, 112, 209, 200), 10.0f, 0, 1.5f);
        } else {
            draw->AddRect(b_min, b_max, IM_COL32(50, 70, 105, 120), 10.0f, 0, 1.0f);
        }

        // Draw digit or placeholder dot
        if (has_digit) {
            char d_str[2] = {pin_digits_[i], '\0'};
            if (font_pin_) ImGui::PushFont(font_pin_);
            ImVec2 dsz = ImGui::CalcTextSize(d_str);
            draw->AddText(ImVec2(b_min.x + (box_w - dsz.x) * 0.5f, b_min.y + (box_h - dsz.y) * 0.5f),
                IM_COL32(255, 255, 255, 255), d_str);
            if (font_pin_) ImGui::PopFont();
        } else {
            draw->AddCircleFilled(ImVec2(b_min.x + box_w * 0.5f, b_min.y + box_h * 0.5f), 3.5f,
                IM_COL32(70, 90, 125, 160));
        }

        bx += box_w + box_spacing;
    }

    // ── On-Screen Keypad for Gamepad & Mouse ──
    cur_y += box_h + 18.0f;
    float kp_btn_w = 46.0f;
    float kp_btn_h = 36.0f;
    float kp_spacing = 8.0f;
    float kp_total_w = 10 * kp_btn_w + 9 * kp_spacing + 80.0f;
    float kp_start_x = modal_pos.x + (modal_w - kp_total_w) * 0.5f;

    for (int d = 0; d <= 9; d++) {
        ImVec2 kp_pos(kp_start_x + d * (kp_btn_w + kp_spacing), cur_y);
        ImGui::SetCursorScreenPos(kp_pos);
        ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kPrimary);
        if (ImGui::Button(std::format("{}", d).c_str(), ImVec2(kp_btn_w, kp_btn_h))) {
            pin_digits_[active_pin_digit_] = static_cast<char>('0' + d);
            if (active_pin_digit_ < 7) active_pin_digit_++;
        }
        ImGui::PopStyleColor(2);
    }

    ImVec2 bsp_pos(kp_start_x + 10 * (kp_btn_w + kp_spacing), cur_y);
    ImGui::SetCursorScreenPos(bsp_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 25, 35, 220));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 30, 45, 255));
    if (ImGui::Button("Borrar", ImVec2(72.0f, kp_btn_h))) {
        if (pin_digits_[active_pin_digit_] != '\0') {
            pin_digits_[active_pin_digit_] = '\0';
        } else if (active_pin_digit_ > 0) {
            active_pin_digit_--;
            pin_digits_[active_pin_digit_] = '\0';
        }
    }
    ImGui::PopStyleColor(2);

    // ── IP Address Input Row ──
    cur_y += kp_btn_h + 18.0f;
    ImGui::SetCursorPos(ImVec2(modal_pos.x + 36.0f, cur_y));
    if (font_small_) ImGui::PushFont(font_small_);
    ImGui::TextColored(colors::kTextSecondary, "IP de la PS5:");
    ImGui::SameLine(modal_pos.x + 125.0f);
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##register_ip", register_ip_, sizeof(register_ip_));

    ImGui::SameLine(modal_pos.x + 330.0f);
    ImGui::TextColored(colors::kTextSecondary, "Nombre:");
    ImGui::SameLine(modal_pos.x + 395.0f);
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##register_name", register_name_, sizeof(register_name_));
    if (font_small_) ImGui::PopFont();

    // ── Modal Action Buttons ──
    cur_y += 42.0f;
    float btn_w = 220.0f;
    float btn_h = 44.0f;
    float btns_start_x = modal_pos.x + (modal_w - (btn_w * 2 + 24.0f)) * 0.5f;

    bool pin_ready = true;
    for (int i = 0; i < 8; i++) {
        if (pin_digits_[i] < '0' || pin_digits_[i] > '9') {
            pin_ready = false;
            break;
        }
    }

    // Button 1: Vincular Consola
    ImVec2 b1_pos(btns_start_x, cur_y);
    ImGui::SetCursorScreenPos(b1_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, pin_ready ? colors::kPrimary : ImVec4(0.08f, 0.14f, 0.25f, 0.70f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kPrimaryHover);
    if (ImGui::Button("##do_pair", ImVec2(btn_w, btn_h)) && pin_ready) {
        std::string pin_str(pin_digits_, 8);
        try {
            uint32_t pin_num = static_cast<uint32_t>(std::stoul(pin_str));
            show_toast(std::format("Vinculando con PS5 ({}) usando PIN {}...", register_ip_, pin_str), 5.0f);
            std::string ip = register_ip_;
            std::string name = register_name_;
            register_thread_ = std::jthread([this, ip, name, pin_num](std::stop_token) {
                auto res = portal::crypto::PS5Protocol::register_with_pin(ip, 9295, pin_num);
                if (res.has_value()) {
                    if (res->host_id.empty()) {
                        show_toast("Error al vincular: ID de consola vacio recibido", 6.0f);
                        return;
                    }
                    portal::discovery::DiscoveredConsole dc {
                        name.empty() ? res->host_name : name,
                        res->host_id,
                        "PS5",
                        ip,
                        9295,
                        portal::ConsoleState::Awake,
                        "13600007"
                    };
                    (void)console_registry_->register_console(dc, res->rp_key, res->regist_key);
                    show_toast("¡Consola PlayStation 5 vinculada con éxito!", 5.0f);
                    show_pin_modal_ = false;
                    current_screen_ = Screen::Home;
                } else {
                    show_toast("Error al vincular: " + res.error().message, 6.0f);
                }
            });
        } catch (...) {
            show_toast("PIN inválido. Introduce 8 dígitos numéricos.", 4.0f);
        }
    }
    ImGui::PopStyleColor(2);

    DrawPSCross(draw, ImVec2(b1_pos.x + 24.0f, b1_pos.y + btn_h * 0.5f), 10.0f);
    if (font_subtitle_) ImGui::PushFont(font_subtitle_);
    draw->AddText(ImVec2(b1_pos.x + 44.0f, b1_pos.y + 11.0f), IM_COL32(255, 255, 255, 255), "Vincular Consola");
    if (font_subtitle_) ImGui::PopFont();

    // Button 2: Cancelar
    ImVec2 b2_pos(btns_start_x + btn_w + 24.0f, cur_y);
    ImGui::SetCursorScreenPos(b2_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kSurfaceHover);
    if (ImGui::Button("##do_cancel", ImVec2(btn_w, btn_h))) {
        show_pin_modal_ = false;
        if (current_screen_ == Screen::Register) current_screen_ = Screen::Home;
    }
    ImGui::PopStyleColor(2);

    DrawPSCircle(draw, ImVec2(b2_pos.x + 24.0f, b2_pos.y + btn_h * 0.5f), 10.0f);
    if (font_subtitle_) ImGui::PushFont(font_subtitle_);
    draw->AddText(ImVec2(b2_pos.x + 48.0f, b2_pos.y + 11.0f), IM_COL32(235, 245, 255, 255), "Cancelar");
    if (font_subtitle_) ImGui::PopFont();
}

void App::draw_registration_screen() {
    draw_pin_modal();
}

// ─── Modal de PIN de 4 Digitos para Perfil de Usuario ───────
void App::draw_login_pin_modal() {
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float modal_w = 540.0f;
    float modal_h = 420.0f;
    ImVec2 modal_pos((ws.x - modal_w) * 0.5f, (ws.y - modal_h) * 0.5f);
    ImVec2 modal_end(modal_pos.x + modal_w, modal_pos.y + modal_h);

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    // Dark glass backdrop scrim over whole screen
    draw->AddRectFilled(ImVec2(0, 0), ws, IM_COL32(4, 7, 16, 220));

    // Modal Container (#0b1021 with glowing border)
    draw->AddRectFilled(modal_pos, modal_end, IM_COL32(11, 16, 33, 252), 20.0f);
    draw->AddRect(ImVec2(modal_pos.x - 2, modal_pos.y - 2), ImVec2(modal_end.x + 2, modal_end.y + 2),
        IM_COL32(0, 240, 255, 50), 22.0f, 0, 3.0f);
    draw->AddRect(modal_pos, modal_end, IM_COL32(0, 240, 255, 180), 20.0f, 0, 1.5f);

    // Top reflection
    draw->AddLine(ImVec2(modal_pos.x + 20.0f, modal_pos.y + 1.0f),
                  ImVec2(modal_end.x - 20.0f, modal_pos.y + 1.0f),
                  IM_COL32(255, 255, 255, 40), 1.0f);

    // Header
    float cur_y = modal_pos.y + 24.0f;
    DrawPSCross(draw, ImVec2(modal_pos.x + 36.0f, cur_y + 12.0f), 12.0f);

    if (font_title_) ImGui::PushFont(font_title_);
    draw->AddText(ImVec2(modal_pos.x + 58.0f, cur_y - 2.0f), IM_COL32(255, 255, 255, 255), "PIN de Usuario de PS5");
    if (font_title_) ImGui::PopFont();

    cur_y += 38.0f;
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(modal_pos.x + 36.0f, cur_y), IM_COL32(160, 180, 210, 240),
        "Introduce el codigo de 4 digitos de tu perfil para iniciar sesion en la consola:");
    if (font_small_) ImGui::PopFont();

    // ── 4 INDIVIDUAL PIN CASILLAS (CASILLAS 0 a 3) ──
    cur_y += 44.0f;
    float box_w = 64.0f;
    float box_h = 72.0f;
    float box_spacing = 16.0f;
    float total_boxes_w = 4 * box_w + 3 * box_spacing;
    float boxes_start_x = modal_pos.x + (modal_w - total_boxes_w) * 0.5f;

    float bx = boxes_start_x;
    for (int i = 0; i < 4; i++) {
        ImVec2 b_min(bx, cur_y);
        ImVec2 b_max(bx + box_w, cur_y + box_h);
        bool is_active = (active_login_pin_digit_ == i);
        bool has_digit = (login_pin_digits_[i] >= '0' && login_pin_digits_[i] <= '9');

        ImGui::SetCursorScreenPos(b_min);
        if (ImGui::InvisibleButton(std::format("##lpin_box_{}", i).c_str(), ImVec2(box_w, box_h))) {
            active_login_pin_digit_ = i;
        }
        bool is_hovered = ImGui::IsItemHovered();

        ImU32 box_bg = is_active ? IM_COL32(16, 28, 56, 250) :
                       is_hovered ? IM_COL32(18, 26, 48, 230) :
                                    IM_COL32(11, 19, 38, 210);
        draw->AddRectFilled(b_min, b_max, box_bg, 12.0f);

        if (is_active) {
            float pulse = 0.5f + 0.5f * sinf(static_cast<float>(ImGui::GetTime()) * 6.0f);
            draw->AddRect(ImVec2(b_min.x - 2, b_min.y - 2), ImVec2(b_max.x + 2, b_max.y + 2),
                IM_COL32(0, 240, 255, static_cast<int>(80 + 80 * pulse)), 14.0f, 0, 2.5f);
            draw->AddRect(b_min, b_max, IM_COL32(0, 240, 255, 255), 12.0f, 0, 2.0f);
            draw->AddLine(ImVec2(b_min.x + 12.0f, b_max.y - 8.0f),
                          ImVec2(b_max.x - 12.0f, b_max.y - 8.0f),
                          IM_COL32(0, 240, 255, static_cast<int>(200 + 55 * pulse)), 2.5f);
        } else if (has_digit) {
            draw->AddRect(b_min, b_max, IM_COL32(0, 112, 209, 200), 12.0f, 0, 1.5f);
        } else {
            draw->AddRect(b_min, b_max, IM_COL32(50, 70, 105, 120), 12.0f, 0, 1.0f);
        }

        if (has_digit) {
            char d_str[2] = {login_pin_digits_[i], '\0'};
            if (font_pin_) ImGui::PushFont(font_pin_);
            ImVec2 dsz = ImGui::CalcTextSize(d_str);
            draw->AddText(ImVec2(b_min.x + (box_w - dsz.x) * 0.5f, b_min.y + (box_h - dsz.y) * 0.5f),
                IM_COL32(255, 255, 255, 255), d_str);
            if (font_pin_) ImGui::PopFont();
        } else {
            draw->AddCircleFilled(ImVec2(b_min.x + box_w * 0.5f, b_min.y + box_h * 0.5f), 4.0f,
                IM_COL32(70, 90, 125, 160));
        }

        bx += box_w + box_spacing;
    }

    // ── On-Screen Keypad for Gamepad & Mouse ──
    cur_y += box_h + 18.0f;
    float kp_btn_w = 40.0f;
    float kp_btn_h = 34.0f;
    float kp_spacing = 8.0f;
    float kp_total_w = 10 * kp_btn_w + 9 * kp_spacing + 76.0f;
    float kp_start_x = modal_pos.x + (modal_w - kp_total_w) * 0.5f;

    for (int d = 0; d <= 9; d++) {
        ImVec2 kp_pos(kp_start_x + d * (kp_btn_w + kp_spacing), cur_y);
        ImGui::SetCursorScreenPos(kp_pos);
        ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kPrimary);
        if (ImGui::Button(std::format("{}", d).c_str(), ImVec2(kp_btn_w, kp_btn_h))) {
            login_pin_digits_[active_login_pin_digit_] = static_cast<char>('0' + d);
            if (active_login_pin_digit_ < 3) active_login_pin_digit_++;
        }
        ImGui::PopStyleColor(2);
    }

    ImVec2 bsp_pos(kp_start_x + 10 * (kp_btn_w + kp_spacing), cur_y);
    ImGui::SetCursorScreenPos(bsp_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 25, 35, 220));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 30, 45, 255));
    if (ImGui::Button("Borrar##lpin", ImVec2(68.0f, kp_btn_h))) {
        if (login_pin_digits_[active_login_pin_digit_] != '\0') {
            login_pin_digits_[active_login_pin_digit_] = '\0';
        } else if (active_login_pin_digit_ > 0) {
            active_login_pin_digit_--;
            login_pin_digits_[active_login_pin_digit_] = '\0';
        }
    }
    ImGui::PopStyleColor(2);

    // ── Checkbox: Recordar PIN ──
    cur_y += kp_btn_h + 16.0f;
    ImGui::SetCursorPos(ImVec2(modal_pos.x + 36.0f, cur_y));
    ImGui::Checkbox("Recordar este PIN para futuros inicios de sesion", &remember_login_pin_);

    // ── Action Buttons ──
    cur_y += 36.0f;
    float btn_w = 200.0f;
    float btn_h = 42.0f;
    float btns_start_x = modal_pos.x + (modal_w - (btn_w * 2 + 20.0f)) * 0.5f;

    bool pin_ready = true;
    for (int i = 0; i < 4; i++) {
        if (login_pin_digits_[i] < '0' || login_pin_digits_[i] > '9') {
            pin_ready = false;
            break;
        }
    }

    // Button 1: Confirmar
    ImVec2 b1_pos(btns_start_x, cur_y);
    ImGui::SetCursorScreenPos(b1_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, pin_ready ? colors::kPrimary : ImVec4(0.08f, 0.14f, 0.25f, 0.70f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kPrimaryHover);
    if ((ImGui::Button("##do_confirm_lpin", ImVec2(btn_w, btn_h)) || (pin_ready && ImGui::IsKeyPressed(ImGuiKey_Enter))) && pin_ready) {
        std::string pin_str(login_pin_digits_, 4);
        if (session_manager_) {
            session_manager_->send_login_pin(pin_str);
        }
        if (remember_login_pin_) {
            if (!target_login_console_host_id_.empty()) {
                console_registry_->update_console_login_pin(target_login_console_host_id_, pin_str);
            } else {
                auto consoles = console_registry_->get_all_consoles();
                if (!consoles.empty()) {
                    console_registry_->update_console_login_pin(consoles[0].host_id, pin_str);
                }
            }
        }
        show_login_pin_modal_ = false;
        show_toast("PIN de usuario enviado a la consola", 3.0f);
    }
    ImGui::PopStyleColor(2);

    DrawPSCross(draw, ImVec2(b1_pos.x + 24.0f, b1_pos.y + btn_h * 0.5f), 9.0f);
    if (font_subtitle_) ImGui::PushFont(font_subtitle_);
    draw->AddText(ImVec2(b1_pos.x + 44.0f, b1_pos.y + 11.0f), IM_COL32(255, 255, 255, 255), "Confirmar PIN");
    if (font_subtitle_) ImGui::PopFont();

    // Button 2: Cancelar
    ImVec2 b2_pos(btns_start_x + btn_w + 20.0f, cur_y);
    ImGui::SetCursorScreenPos(b2_pos);
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kSurface);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kSurfaceHover);
    if (ImGui::Button("##do_cancel_lpin", ImVec2(btn_w, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        show_login_pin_modal_ = false;
    }
    ImGui::PopStyleColor(2);

    DrawPSCircle(draw, ImVec2(b2_pos.x + 24.0f, b2_pos.y + btn_h * 0.5f), 9.0f);
    if (font_subtitle_) ImGui::PushFont(font_subtitle_);
    draw->AddText(ImVec2(b2_pos.x + 48.0f, b2_pos.y + 11.0f), IM_COL32(235, 245, 255, 255), "Cancelar");
    if (font_subtitle_) ImGui::PopFont();
}

// ─── 10-Foot Console Settings ─────────────────────────────
void App::draw_settings() {
    ImVec2 ws = ImGui::GetIO().DisplaySize;
    float start_y = 92.0f;

    ImGui::SetCursorPos(ImVec2(40.0f, start_y));
    if (font_title_) ImGui::PushFont(font_title_);
    ImGui::TextColored(colors::kTextPrimary, "CONFIGURACION DEL SISTEMA");
    if (font_title_) ImGui::PopFont();

    float panel_w = (ws.x - 120.0f) / 3.0f;
    float panel_h = ws.y - start_y - 140.0f;

    // Panel 1: Video & Streaming
    ImGui::SetCursorPos(ImVec2(40.0f, start_y + 50.0f));
    ImGui::BeginChild("##settings_video", ImVec2(panel_w, panel_h), ImGuiChildFlags_None);
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(12, 18, 36, 210), 16.0f);
        draw->AddRect(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(0, 112, 209, 100), 16.0f, 0, 1.0f);

        ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));
        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        ImGui::TextColored(colors::kAccent, "STREAMING & VIDEO");
        if (font_subtitle_) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 10));
        static int res_idx = 2;
        const char* resolutions[] = {"720p HD", "1080p Full HD", "1440p 2K", "2160p 4K UHD"};
        ImGui::TextColored(colors::kTextSecondary, "Resolucion de Transmision:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        ImGui::Combo("##res", &res_idx, resolutions, 4);

        ImGui::Dummy(ImVec2(0, 10));
        static int fps_idx = 1;
        const char* f_rates[] = {"30 FPS", "60 FPS (Recomendado)", "120 FPS (Baja Latencia)"};
        ImGui::TextColored(colors::kTextSecondary, "Tasa de Cuadros:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        ImGui::Combo("##fps", &fps_idx, f_rates, 3);

        ImGui::Dummy(ImVec2(0, 10));
        static int codec_idx = 1;
        const char* codecs[] = {"H.264 (AVC Universal)", "H.265 (HEVC 10-bit)", "AV1"};
        ImGui::TextColored(colors::kTextSecondary, "Codec de Descompresion:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        ImGui::Combo("##codec", &codec_idx, codecs, 3);

        ImGui::Dummy(ImVec2(0, 15));
        static bool enable_hdr = true;
        ImGui::Checkbox("Activar HDR10 (Colores 10-bit)", &enable_hdr);
    }
    ImGui::EndChild();

    // Panel 2: Red & Rendimiento
    ImGui::SetCursorPos(ImVec2(40.0f + panel_w + 20.0f, start_y + 50.0f));
    ImGui::BeginChild("##settings_net", ImVec2(panel_w, panel_h), ImGuiChildFlags_None);
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(12, 18, 36, 210), 16.0f);
        draw->AddRect(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(0, 112, 209, 100), 16.0f, 0, 1.0f);

        ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));
        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        ImGui::TextColored(colors::kAccent, "RED & LATENCIA");
        if (font_subtitle_) ImGui::PopFont();

        int cur_bitrate = static_cast<int>(settings_.bitrate_kbps);
        ImGui::TextColored(colors::kTextSecondary, "Bitrate Objetivo:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        if (ImGui::SliderInt("##bitrate", &cur_bitrate, 5000, 50000, "%d kbps")) {
            settings_.bitrate_kbps = static_cast<uint32_t>(cur_bitrate);
            // Persist immediately so setting survives restart
            (void)settings_.save(settings_path_);
        }

        ImGui::Dummy(ImVec2(0, 10));
        static bool fec_enabled = true;
        ImGui::Checkbox("Recuperacion de Paquetes FEC", &fec_enabled);

        ImGui::Dummy(ImVec2(0, 10));
        static bool takion_mode = true;
        ImGui::Checkbox("Gaikai Takion Ultra-Low Latency", &takion_mode);

        ImGui::Dummy(ImVec2(0, 10));
        static bool udp_pacing = true;
        ImGui::Checkbox("Optimizacion UDP Pacing (Sin micro-stutter)", &udp_pacing);

        ImGui::Dummy(ImVec2(0, 14));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextColored(colors::kAccent, "SEGURIDAD & ENCENDIDO");
        ImGui::Dummy(ImVec2(0, 6));

        if (ImGui::Button("Configurar PIN de Usuario de PS5", ImVec2(panel_w - 40.0f, 34.0f))) {
            auto list = console_registry_->get_all_consoles();
            if (!list.empty()) {
                target_login_console_host_id_ = list[0].host_id;
                memset(login_pin_digits_, 0, sizeof(login_pin_digits_));
                if (list[0].login_pin.size() == 4) {
                    for (int p = 0; p < 4; ++p) login_pin_digits_[p] = list[0].login_pin[p];
                }
            }
            active_login_pin_digit_ = 0;
            show_login_pin_modal_ = true;
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (ImGui::Button("Escanear Consolas en Red Ahora", ImVec2(panel_w - 40.0f, 34.0f))) {
            probe_consoles_background();
            show_toast("Buscando consolas en red local...", 3.0f);
        }
    }
    ImGui::EndChild();

    // Panel 3: Audio & DualSense
    ImGui::SetCursorPos(ImVec2(40.0f + (panel_w + 20.0f) * 2.0f, start_y + 50.0f));
    ImGui::BeginChild("##settings_audio", ImVec2(panel_w, panel_h), ImGuiChildFlags_None);
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(12, 18, 36, 210), 16.0f);
        draw->AddRect(pos, ImVec2(pos.x + panel_w, pos.y + panel_h), IM_COL32(0, 112, 209, 100), 16.0f, 0, 1.0f);

        ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));
        if (font_subtitle_) ImGui::PushFont(font_subtitle_);
        ImGui::TextColored(colors::kAccent, "AUDIO & DUALSENSE");
        if (font_subtitle_) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 10));
        static int audio_buf = 20;
        ImGui::TextColored(colors::kTextSecondary, "Buffer de Audio:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        ImGui::SliderInt("##audio_buf", &audio_buf, 5, 100, "%d ms");

        ImGui::Dummy(ImVec2(0, 10));
        static bool tempest_3d = true;
        ImGui::Checkbox("Emulacion Audio 3D Tempest", &tempest_3d);

        ImGui::Dummy(ImVec2(0, 10));
        static bool haptics = true;
        ImGui::Checkbox("Vibracion Haptica DualSense (USB)", &haptics);

        ImGui::Dummy(ImVec2(0, 10));
        static bool triggers = true;
        ImGui::Checkbox("Gatillos Adaptativos (L2 / R2)", &triggers);

        ImGui::Dummy(ImVec2(0, 10));
        static int led_color = 0;
        const char* leds[] = {"Cyan Neon (#00f0ff)", "PlayStation Blue (#0070d1)", "Violeta Cloud (#7928ca)", "Desactivado"};
        ImGui::TextColored(colors::kTextSecondary, "Color LED DualSense:");
        ImGui::SetNextItemWidth(panel_w - 40.0f);
        ImGui::Combo("##led_col", &led_color, leds, 4);
    }
    ImGui::EndChild();

    // Bottom Action Buttons
    float b_y = ws.y - 78.0f;
    ImGui::SetCursorPos(ImVec2(40.0f, b_y));
    if (ImGui::Button("Guardar Configuracion", ImVec2(220.0f, 40.0f))) {
        show_toast("Ajustes guardados correctamente", 3.0f);
        current_screen_ = Screen::Home;
        current_tab_ = 0;
    }
    ImGui::SameLine(0, 20.0f);
    if (ImGui::Button("Volver al Inicio", ImVec2(180.0f, 40.0f))) {
        current_screen_ = Screen::Home;
        current_tab_ = 0;
    }
}

// ─── Streaming HUD Overlay ────────────────────────────────
void App::draw_streaming_overlay() {
    ImVec2 ws = ImGui::GetContentRegionAvail();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Pure black OLED background (no navy blue borders)
    draw->AddRectFilled(pos, ImVec2(pos.x + ws.x, pos.y + ws.y), IM_COL32(0, 0, 0, 255));

    // Ensure stream texture is initialized
    if (!stream_texture_initialized_) {
        init_stream_texture(1920, 1080);
    }

    // Process new frame if available
    {
        std::lock_guard<std::mutex> lock(stream_frame_mutex_);
        if (stream_has_new_frame_ && !stream_pixel_cache_.empty()) {
            update_stream_texture(stream_pixel_cache_.data(), stream_w_, stream_h_);
            stream_has_new_frame_ = false;
        }
    }

    // Render video texture centered at 16:9
    float aspect = 16.0f / 9.0f;
    float render_w = ws.x;
    float render_h = ws.x / aspect;
    if (render_h > ws.y) {
        render_h = ws.y;
        render_w = ws.y * aspect;
    }
    ImVec2 stream_min(pos.x + (ws.x - render_w) * 0.5f, pos.y + (ws.y - render_h) * 0.5f);
    ImVec2 stream_max(stream_min.x + render_w, stream_min.y + render_h);

    if (stream_texture_.descriptor_set != VK_NULL_HANDLE) {
        draw->AddImage((ImTextureID)stream_texture_.descriptor_set, stream_min, stream_max);
    } else {
        draw->AddRectFilled(stream_min, stream_max, IM_COL32(0, 0, 0, 255));
    }

    // Double-click on streaming surface to toggle fullscreen
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        bool is_full = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
        SDL_SetWindowFullscreen(window_, !is_full);
    }

    // Auto-hide HUD and cursor after 3 seconds of mouse inactivity
    float cur_time = static_cast<float>(ImGui::GetTime());
    bool show_hud = (cur_time - last_mouse_activity_time_ < 3.0f) || show_login_pin_modal_;

    if (!show_hud) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        return; // Pure clean fullscreen video
    }

    // Top HUD Stats Pill
    float hud_w = 640.0f;
    float hud_h = 42.0f;
    ImVec2 hud_pos(pos.x + (ws.x - hud_w) * 0.5f, pos.y + 20.0f);
    draw->AddRectFilled(hud_pos, ImVec2(hud_pos.x + hud_w, hud_pos.y + hud_h), IM_COL32(11, 16, 33, 210), 20.0f);
    draw->AddRect(hud_pos, ImVec2(hud_pos.x + hud_w, hud_pos.y + hud_h), IM_COL32(0, 240, 255, 160), 20.0f, 0, 1.2f);

    bool is_streaming = session_manager_ && session_manager_->is_streaming();
    draw->AddCircleFilled(ImVec2(hud_pos.x + 20.0f, hud_pos.y + 21.0f), 5.0f,
        is_streaming ? IM_COL32(0, 230, 118, 255) : IM_COL32(255, 171, 0, 255));

    if (font_body_) ImGui::PushFont(font_body_);
    std::string hud_text;
    if (is_streaming) {
        auto stats = session_manager_->get_stats();
        hud_text = std::format("PS5 | 1080p @ {:.0f} FPS | {:.1f} Mbps | Latencia: {:.0f} ms",
            stats.video_fps > 0 ? stats.video_fps : 60.0f,
            stats.video_bitrate_kbps > 0 ? (stats.video_bitrate_kbps / 1000.0f) : (settings_.bitrate_kbps / 1000.0f),
            stats.latency_ms > 0 ? stats.latency_ms : 1.0f);
    } else {
        hud_text = "Conectando con PlayStation...";
    }
    draw->AddText(ImVec2(hud_pos.x + 34.0f, hud_pos.y + 11.0f), IM_COL32(245, 250, 255, 255), hud_text.c_str());
    if (font_body_) ImGui::PopFont();

    // Action Buttons: "Botón PS (Home) [F1]", "Poner en Reposo (Suspender)", and "Desconectar (ESC)"
    float btn_w = 210.0f;
    float gap = 16.0f;
    float total_w = btn_w * 3.0f + gap * 2.0f;
    float start_x = (ws.x - total_w) * 0.5f;
    float btn_y = ws.y - 70.0f;

    // Button 1: Botón PS (Home) [F1]
    ImGui::SetCursorPos(ImVec2(start_x, btn_y));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.44f, 0.88f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.55f, 1.00f, 1.00f));
    if (ImGui::Button("Boton PS (Home) [F1]", ImVec2(btn_w, 42.0f))) {
        if (session_manager_) {
            session_manager_->send_ps_button_pulse(200);
            show_toast("Boton PS pulsado", 1.5f);
        }
    }
    ImGui::PopStyleColor(2);

    // Button 2: Poner en Reposo (Suspender consola)
    ImGui::SetCursorPos(ImVec2(start_x + btn_w + gap, btn_y));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.48f, 0.00f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.58f, 0.05f, 1.00f));
    if (ImGui::Button("Poner en Reposo (Suspender)", ImVec2(btn_w, 42.0f))) {
        if (session_manager_) {
            session_manager_->suspend_console();
            show_toast("Poniendo PlayStation 5 en Modo Reposo...", 4.0f);
            suspend_thread_ = std::jthread([sm = session_manager_](std::stop_token) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sm->disconnect();
            });
        }
        SDL_SetWindowFullscreen(window_, false);
        current_screen_ = Screen::Home;
        current_tab_ = 0;
    }
    ImGui::PopStyleColor(2);

    // Button 3: Desconectar (ESC)
    ImGui::SetCursorPos(ImVec2(start_x + (btn_w + gap) * 2.0f, btn_y));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(colors::kError.x, colors::kError.y, colors::kError.z, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kError);
    if (ImGui::Button("Desconectar (ESC)", ImVec2(btn_w, 42.0f))) {
        if (session_manager_) session_manager_->disconnect();
        SDL_SetWindowFullscreen(window_, false);
        current_screen_ = Screen::Home;
        current_tab_ = 0;
    }
    ImGui::PopStyleColor(2);
}

// ─── Bottom PlayStation Controller Legend ─────────────────
void App::draw_status_bar() {
    float bar_height = 46.0f;
    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, display_size.y - bar_height));
    ImGui::SetNextWindowSize(ImVec2(display_size.x, bar_height));
    ImGui::Begin("##StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 bar_min(0, display_size.y - bar_height);
    ImVec2 bar_max(display_size.x, display_size.y);

    // Glass bar background
    draw->AddRectFilled(bar_min, bar_max, IM_COL32(7, 11, 22, 245));
    draw->AddLine(bar_min, ImVec2(display_size.x, display_size.y - bar_height),
        IM_COL32(0, 240, 255, 45), 1.0f);

    float cur_x = 24.0f;
    float center_y = display_size.y - bar_height * 0.5f;

    // Button: [X] Conectar / Seleccionar
    DrawPSCross(draw, ImVec2(cur_x, center_y), 9.0f);
    cur_x += 16.0f;
    if (font_small_) ImGui::PushFont(font_small_);
    draw->AddText(ImVec2(cur_x, center_y - 8.0f), IM_COL32(220, 235, 255, 240), "Seleccionar");
    cur_x += ImGui::CalcTextSize("Seleccionar").x + 28.0f;

    // Button: [O] Volver
    DrawPSCircle(draw, ImVec2(cur_x, center_y), 9.0f);
    cur_x += 16.0f;
    draw->AddText(ImVec2(cur_x, center_y - 8.0f), IM_COL32(220, 235, 255, 240), "Volver");
    cur_x += ImGui::CalcTextSize("Volver").x + 28.0f;

    // Button: [▢] Opciones / Despertar
    DrawPSSquare(draw, ImVec2(cur_x, center_y), 9.0f);
    cur_x += 16.0f;
    draw->AddText(ImVec2(cur_x, center_y - 8.0f), IM_COL32(220, 235, 255, 240), "Opciones / Despertar");
    cur_x += ImGui::CalcTextSize("Opciones / Despertar").x + 28.0f;

    // Button: [△] Escanear
    DrawPSTriangle(draw, ImVec2(cur_x, center_y), 9.0f);
    cur_x += 16.0f;
    draw->AddText(ImVec2(cur_x, center_y - 8.0f), IM_COL32(220, 235, 255, 240), "Buscar Consolas");
    cur_x += ImGui::CalcTextSize("Buscar Consolas").x + 28.0f;

    // Button: [L1] [R1] Pestañas
    DrawPSBumper(draw, ImVec2(cur_x, center_y - 10.0f), "L1");
    cur_x += 38.0f;
    DrawPSBumper(draw, ImVec2(cur_x, center_y - 10.0f), "R1");
    cur_x += 42.0f;
    draw->AddText(ImVec2(cur_x, center_y - 8.0f), IM_COL32(220, 235, 255, 240), "Cambiar Pestaña");
    if (font_small_) ImGui::PopFont();

    // Right Telemetry Indicators
    float right_x = display_size.x - 24.0f;

    // PSN Status
    if (font_small_) ImGui::PushFont(font_small_);
    std::string psn_status_str = "PSN: Conectado";
    if (account_manager_) {
        auto act = account_manager_->get_active_account();
        if (act && !act->profile.online_id.empty()) {
            psn_status_str = "PSN: " + act->profile.online_id;
        }
    }
    const char* psn_txt = psn_status_str.c_str();
    float psn_w = ImGui::CalcTextSize(psn_txt).x;
    right_x -= psn_w;
    draw->AddText(ImVec2(right_x, center_y - 8.0f), IM_COL32(0, 230, 118, 255), psn_txt);
    right_x -= 20.0f;

    // Controller Status (Dynamic from ControllerManager)
    std::string ctrl_txt = (controller_manager_ && controller_manager_->is_connected())
        ? controller_manager_->get_controller_name()
        : "Sin mando conectado";
    float ctrl_w = ImGui::CalcTextSize(ctrl_txt.c_str()).x;
    right_x -= ctrl_w;
    ImU32 ctrl_col = (controller_manager_ && controller_manager_->is_connected())
        ? IM_COL32(0, 240, 255, 240)
        : IM_COL32(160, 175, 205, 180);
    draw->AddText(ImVec2(right_x, center_y - 8.0f), ctrl_col, ctrl_txt.c_str());
    if (font_small_) ImGui::PopFont();

    ImGui::End();
}

// ─── Cleanup ─────────────────────────────────────────────

void App::cleanup_vulkan() {
    if (device_ == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device_);

    for (auto& frame : frames_) {
        if (frame.fence) vkDestroyFence(device_, frame.fence, nullptr);
        if (frame.image_available) vkDestroySemaphore(device_, frame.image_available, nullptr);
        if (frame.render_finished) vkDestroySemaphore(device_, frame.render_finished, nullptr);
        if (frame.framebuffer) vkDestroyFramebuffer(device_, frame.framebuffer, nullptr);
    }

    if (command_pool_) vkDestroyCommandPool(device_, command_pool_, nullptr);
    if (render_pass_) vkDestroyRenderPass(device_, render_pass_, nullptr);

    for (auto iv : swapchain_image_views_) {
        vkDestroyImageView(device_, iv, nullptr);
    }

    if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    if (imgui_descriptor_pool_) vkDestroyDescriptorPool(device_, imgui_descriptor_pool_, nullptr);
    if (device_) vkDestroyDevice(device_, nullptr);
    if (surface_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (instance_) vkDestroyInstance(instance_, nullptr);
}

void App::cleanup_imgui() {
    if (device_ == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(device_);
    destroy_stream_texture();
    destroy_textures();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

}  // namespace portal::ui
