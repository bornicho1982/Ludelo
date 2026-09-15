// Archivo: src/LudeloCore/Stream/SessionManager.cpp
#include "LudeloCore/Stream/SessionManager.h"
#include "LudeloCore/Cloud/GaikaiClient.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace ludelo::stream {

SessionManager::SessionManager() {
    static std::once_flag s_chiaki_init_flag;
    std::call_once(s_chiaki_init_flag, []() {
        chiaki_lib_init();
        spdlog::info("[SessionManager] Chiaki core library initialized.");
    });
    m_video_decoder = std::make_unique<VideoDecoder>();
    m_audio_decoder = std::make_unique<AudioDecoder>();
}

SessionManager::~SessionManager() {
    disconnect();
}

void SessionManager::set_state(SessionState state) {
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_state = state;
    }
    if (on_state_change) {
        on_state_change(state);
    }
}

void SessionManager::on_chiaki_log(ChiakiLogLevel level, const char *msg, void *user) {
    (void)user;
    if (level & CHIAKI_LOG_ERROR) {
        spdlog::error("[ChiakiCore] {}", msg);
    } else if (level & CHIAKI_LOG_WARNING) {
        spdlog::warn("[ChiakiCore] {}", msg);
    } else if (level & CHIAKI_LOG_INFO) {
        spdlog::info("[ChiakiCore] {}", msg);
    } else {
        spdlog::debug("[ChiakiCore] {}", msg);
    }
}

void SessionManager::on_chiaki_event(ChiakiEvent *event, void *user) {
    auto* self = static_cast<SessionManager*>(user);
    if (!self || !event) return;

    switch (event->type) {
        case CHIAKI_EVENT_CONNECTED: {
            spdlog::info("[SessionManager] Received CHIAKI_EVENT_CONNECTED! Transitioning to Streaming.");
            self->set_state(SessionState::Streaming);
            // Automatic PS button pulse to dismiss 'Press PS Button' screen and auto-login
            self->send_ps_button_pulse(200);
            break;
        }
        case CHIAKI_EVENT_RUMBLE: {
            if (self->on_haptic_feedback) {
                HapticEvent haptic{
                    .motor_left = event->rumble.left,
                    .motor_right = event->rumble.right
                };
                self->on_haptic_feedback(haptic);
            }
            break;
        }
        case CHIAKI_EVENT_TRIGGER_EFFECTS: {
            if (self->on_trigger_effect) {
                TriggerEffect left_trig{
                    .trigger = TriggerEffect::Side::Left,
                    .mode = static_cast<TriggerMode>(event->trigger_effects.type_left)
                };
                memcpy(left_trig.params, event->trigger_effects.left, sizeof(left_trig.params));
                self->on_trigger_effect(left_trig);

                TriggerEffect right_trig{
                    .trigger = TriggerEffect::Side::Right,
                    .mode = static_cast<TriggerMode>(event->trigger_effects.type_right)
                };
                memcpy(right_trig.params, event->trigger_effects.right, sizeof(right_trig.params));
                self->on_trigger_effect(right_trig);
            }
            break;
        }
        case CHIAKI_EVENT_LOGIN_PIN_REQUEST: {
            spdlog::info("[SessionManager] Console requested user login PIN (CHIAKI_EVENT_LOGIN_PIN_REQUEST).");
            if (!self->m_console.login_pin.empty()) {
                spdlog::info("[SessionManager] Auto-submitting saved login PIN (length: {})", self->m_console.login_pin.length());
                chiaki_session_set_login_pin(&self->m_chiaki_session,
                    reinterpret_cast<const uint8_t*>(self->m_console.login_pin.data()),
                    self->m_console.login_pin.length());
            } else {
                spdlog::info("[SessionManager] No saved login PIN found; requesting from UI callback...");
                if (self->on_login_pin_requested) {
                    self->on_login_pin_requested();
                }
            }
            break;
        }
        case CHIAKI_EVENT_QUIT: {
            const char* reason_str = chiaki_quit_reason_string(event->quit.reason);
            spdlog::warn("[SessionManager] Session quit: {} (code: {})", reason_str, (int)event->quit.reason);
            if (chiaki_quit_reason_is_error(event->quit.reason)) {
                if (self->on_error) {
                    self->on_error(Error(ErrorCode::SessionDenied, fmt::format("Console disconnected: {}", reason_str)));
                }
                self->set_state(SessionState::Error);
            } else {
                self->set_state(SessionState::Idle);
            }
            break;
        }
        default:
            break;
    }
}

bool SessionManager::on_chiaki_video_sample(uint8_t *buf, size_t buf_size, int32_t frames_lost, bool frame_recovered, void *user) {
    auto* self = static_cast<SessionManager*>(user);
    if (!self || !buf || buf_size == 0) return false;

#ifdef _WIN32
    static thread_local bool s_thread_priority_set = false;
    if (!s_thread_priority_set) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        HMODULE hAvrt = LoadLibraryA("avrt.dll");
        if (hAvrt) {
            typedef HANDLE (WINAPI *AvSetMmThreadCharacteristicsW_fn)(LPCWSTR, LPDWORD);
            auto pfnAvSet = reinterpret_cast<AvSetMmThreadCharacteristicsW_fn>(GetProcAddress(hAvrt, "AvSetMmThreadCharacteristicsW"));
            if (pfnAvSet) {
                DWORD task_idx = 0;
                HANDLE mm_h = pfnAvSet(L"Games", &task_idx);
                if (mm_h) {
                    spdlog::info("[SessionManager] Video receiver thread elevated to MMCSS 'Games' & THREAD_PRIORITY_HIGHEST");
                }
            }
        }
        s_thread_priority_set = true;
    }
#endif

    if (frames_lost > 0) {
        spdlog::debug("[SessionManager] Video sample: {} frames lost, recovered={}", frames_lost, frame_recovered);
    }

    if (self->m_video_decoder) {
        auto dec_res = self->m_video_decoder->decode(std::span<const uint8_t>(buf, buf_size));
        if (dec_res.has_value()) {
            self->m_video_frame_count++;
            if (self->on_video_frame) {
                self->on_video_frame(dec_res.value());
            }
            return true;
        }
    }
    return true;
}

void SessionManager::on_chiaki_audio_frame(uint8_t *buf, size_t buf_size, void *user) {
    auto* self = static_cast<SessionManager*>(user);
    if (!self || !buf || buf_size == 0) return;

    if (self->m_audio_decoder) {
        auto dec_res = self->m_audio_decoder->decode(std::span<const uint8_t>(buf, buf_size));
        if (dec_res.has_value()) {
            self->m_audio_frame_count++;
            if (self->on_audio_frame) {
                self->on_audio_frame(dec_res.value());
            }
        }
    }
}

VoidResult SessionManager::connect_local(const ludelo::discovery::RegisteredConsole& console, const StreamConfig& config) {
    if (m_state.load() != SessionState::Idle) {
        spdlog::info("[SessionManager] Cleaning up prior session state before connecting...");
        disconnect();
    }

    m_console = console;
    m_config = config;

    set_state(SessionState::Discovering);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Decoders initialization
    VideoCodec codec = config.codec;
    auto vdec_res = m_video_decoder->init(codec, true);
    if (!vdec_res.has_value()) {
        spdlog::warn("[SessionManager] Hardware video decoder unavailable, switching to software decoder: {}", vdec_res.error().message);
        auto sw_res = m_video_decoder->init(codec, false);
        if (!sw_res.has_value()) {
            spdlog::error("[SessionManager] Video decoder init failed: {}", sw_res.error().message);
            set_state(SessionState::Error);
            return std::unexpected(sw_res.error());
        }
    }
    (void)m_audio_decoder->init(48000, 2);

    spdlog::info("[SessionManager] Video stream pipeline ready: codec={}, hw_decoder='{}', threads=4, target_fps={}",
        (codec == VideoCodec::H265 ? "H.265 (HEVC)" : "H.264 (AVC)"),
        m_video_decoder->get_hw_device_type(),
        (config.fps == FrameRate::FPS60 ? 60 : 30));

    // Setup Chiaki Logging
    chiaki_log_init(&m_chiaki_log, CHIAKI_LOG_ALL, on_chiaki_log, this);

    // Populate ChiakiConnectInfo
    ChiakiConnectInfo connect_info{};
    memset(&connect_info, 0, sizeof(connect_info));

    std::string host = console.address;
    if (host.empty()) {
        spdlog::error("[SessionManager] Cannot connect: console has no IP address. Please register first.");
        set_state(SessionState::Error);
        return std::unexpected(Error(ErrorCode::ConsoleNotFound, "Console has no IP address. Please register the console first."));
    }
    connect_info.host = host.c_str();
    connect_info.ps5 = (console.host_type.empty() || console.host_type == "PS5" || console.host_type.find("PS5") != std::string::npos);

    // Registration key handling
    if (console.rp_auth.empty()) {
        spdlog::error("[SessionManager] Cannot connect: console has no rp_auth key. Please register first.");
        set_state(SessionState::Error);
        return std::unexpected(Error(ErrorCode::RegistrationError, "Console has no rp_auth key. Please register the console first."));
    }
    std::string auth_str = console.rp_auth;
    bool all_hex = !auth_str.empty() && std::all_of(auth_str.begin(), auth_str.end(), [](unsigned char c) {
        return std::isxdigit(c);
    });
    if (auth_str.size() == 32 && all_hex) {
        for (size_t i = 0; i < 16; ++i) {
            std::string byte_str = auth_str.substr(i * 2, 2);
            connect_info.regist_key[i] = static_cast<char>(std::stoul(byte_str, nullptr, 16));
        }
    } else if (auth_str.size() == 16 && all_hex) {
        for (size_t i = 0; i < 8; ++i) {
            std::string byte_str = auth_str.substr(i * 2, 2);
            connect_info.regist_key[i] = static_cast<char>(std::stoul(byte_str, nullptr, 16));
        }
    } else {
        memcpy(connect_info.regist_key, auth_str.data(), std::min(auth_str.size(), sizeof(connect_info.regist_key)));
    }

    // Morning key (rp_key) — must be present from registration, never use a hardcoded fallback
    if (!console.rp_key.empty() && console.rp_key.size() >= 16) {
        memcpy(connect_info.morning, console.rp_key.data(), 16);
    } else {
        spdlog::error("[SessionManager] Cannot connect: console has no rp_key (morning key). Please re-register the console.");
        set_state(SessionState::Error);
        return std::unexpected(Error(ErrorCode::RegistrationError, "Console has no rp_key. Please re-register the console."));
    }

    spdlog::info("[SessionManager] Connecting to {} (rp_auth={}, rp_key={})",
        host, ludelo::mask_secret(auth_str), ludelo::mask_secret(console.rp_key));

    // Video Profile
    ChiakiVideoResolutionPreset res_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
    if (config.resolution == Resolution::R720p) {
        res_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
    } else if (config.resolution == Resolution::R540p) {
        res_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_540p;
    } else if (config.resolution == Resolution::R360p) {
        res_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_360p;
    }
    ChiakiVideoFPSPreset fps_preset = (config.fps == FrameRate::FPS30) ? CHIAKI_VIDEO_FPS_PRESET_30 : CHIAKI_VIDEO_FPS_PRESET_60;
    chiaki_connect_video_profile_preset(&connect_info.video_profile, res_preset, fps_preset);
    connect_info.video_profile.codec = (codec == VideoCodec::H265) ? CHIAKI_CODEC_H265 : CHIAKI_CODEC_H264;
    if (config.bitrate_kbps > 0) {
        connect_info.video_profile.bitrate = config.bitrate_kbps;
    }
    connect_info.video_profile_auto_downgrade = true;
    connect_info.enable_dualsense = true;
    connect_info.enable_keyboard = true;
    connect_info.packet_loss_max = 1.0; // Aligned with chiaki-ng ChiakiSettings (allow full congestion reporting)

    spdlog::info("[SessionManager] Connecting to {} ({}) with resolution {}x{}, bitrate {} kbps, codec {}",
        host, connect_info.ps5 ? "PS5" : "PS4",
        connect_info.video_profile.width, connect_info.video_profile.height,
        connect_info.video_profile.bitrate,
        chiaki_codec_name(connect_info.video_profile.codec));

    set_state(SessionState::Connecting);

    ChiakiErrorCode err = chiaki_session_init(&m_chiaki_session, &connect_info, &m_chiaki_log);
    if (err != CHIAKI_ERR_SUCCESS) {
        spdlog::error("[SessionManager] chiaki_session_init failed: {}", chiaki_error_string(err));
        set_state(SessionState::Error);
        return std::unexpected(Error(ErrorCode::SessionCreationFailed, chiaki_error_string(err)));
    }
    m_session_initialized = true;

    // Register Callbacks
    chiaki_session_set_event_cb(&m_chiaki_session, on_chiaki_event, this);
    chiaki_session_set_video_sample_cb(&m_chiaki_session, on_chiaki_video_sample, this);

    ChiakiAudioSink audio_sink{};
    audio_sink.user = this;
    audio_sink.header_cb = nullptr;
    audio_sink.frame_cb = on_chiaki_audio_frame;
    chiaki_session_set_audio_sink(&m_chiaki_session, &audio_sink);

    set_state(SessionState::Negotiating);

    err = chiaki_session_start(&m_chiaki_session);
    if (err != CHIAKI_ERR_SUCCESS) {
        spdlog::error("[SessionManager] chiaki_session_start failed: {}", chiaki_error_string(err));
        disconnect();
        return std::unexpected(Error(ErrorCode::HandshakeFailed, chiaki_error_string(err)));
    }
    m_session_running = true;

    // Start 250Hz DualSense low-latency input thread
    m_input_thread = std::jthread([this](std::stop_token st) {
        input_loop(st);
    });

    spdlog::info("[SessionManager] Chiaki session initiated successfully in background.");
    return {};
}

void SessionManager::input_loop(std::stop_token st) {
    spdlog::info("[SessionManager] DualSense 250Hz input polling loop active.");
    using clock = std::chrono::steady_clock;
    constexpr auto interval = std::chrono::microseconds(4000); // 250Hz

    while (!st.stop_requested()) {
        auto next_tick = clock::now() + interval;

        if (m_session_running.load() && on_poll_input) {
            auto in = on_poll_input();
            ChiakiControllerState c_state{};
            chiaki_controller_state_set_idle(&c_state);

            c_state.buttons = in.buttons;
            c_state.l2_state = in.l2_trigger;
            c_state.r2_state = in.r2_trigger;

            // Sticks directos 16-bit (-32768 a 32767) sin perdida de precision
            c_state.left_x  = in.left_stick_x;
            c_state.left_y  = in.left_stick_y;
            c_state.right_x = in.right_stick_x;
            c_state.right_y = in.right_stick_y;

            // Touchpad
            for (size_t i = 0; i < 2; ++i) {
                if (in.touchpad[i].active) {
                    c_state.touches[i].id = static_cast<int8_t>(in.touchpad[i].id);
                    c_state.touches[i].x  = static_cast<uint16_t>(in.touchpad[i].x) * 7;
                    c_state.touches[i].y  = static_cast<uint16_t>(in.touchpad[i].y) * 7;
                } else {
                    c_state.touches[i].id = -1;
                }
            }

            // Motion
            c_state.gyro_x = in.gyro.x;
            c_state.gyro_y = in.gyro.y;
            c_state.gyro_z = in.gyro.z;
            c_state.accel_x = in.accel.x;
            c_state.accel_y = in.accel.y;
            c_state.accel_z = in.accel.z;

            chiaki_session_set_controller_state(&m_chiaki_session, &c_state);
        }

        std::this_thread::sleep_until(next_tick);
    }
    spdlog::info("[SessionManager] DualSense 250Hz input polling loop stopped.");
}

void SessionManager::disconnect() {
    auto current_state = m_state.load();
    if (current_state == SessionState::Idle || current_state == SessionState::Disconnecting) {
        return;
    }

    spdlog::info("[SessionManager] Disconnecting session...");
    set_state(SessionState::Disconnecting);

    // Stop input polling thread first
    if (m_input_thread.joinable()) {
        m_input_thread.request_stop();
        m_input_thread.join();
    }

    // Stop PS button pulse thread before touching Chiaki session — prevents UAF
    if (m_ps_button_thread.joinable()) {
        m_ps_button_thread.request_stop();
        m_ps_button_thread.join();
    }

    // ── Cloud session path (TakionConnection) ───────────────
    if (m_takion_connection) {
        m_takion_connection->disconnect();
        m_takion_connection.reset();
        // Best-effort notify Kamaji server that session ended
        if (!m_cloud_session_id.empty()) {
            ludelo::cloud::GaikaiClient gaikai;
            (void)gaikai.end_session(m_cloud_session_id);
            m_cloud_session_id.clear();
        }
    }

    // ── Local session path (Chiaki) ──────────────────────────
    if (m_session_running.load()) {
        chiaki_session_stop(&m_chiaki_session);
        chiaki_session_join(&m_chiaki_session);
        m_session_running = false;
    }

    if (m_session_initialized.load()) {
        chiaki_session_fini(&m_chiaki_session);
        m_session_initialized = false;
    }

    if (m_video_decoder) {
        m_video_decoder->reset();
    }

    set_state(SessionState::Idle);
    spdlog::info("[SessionManager] Session disconnected and reset.");
}

void SessionManager::send_controller_state(const ControllerState& state) {
    if (m_session_running.load()) {
        ChiakiControllerState c_state{};
        chiaki_controller_state_set_idle(&c_state);
        c_state.buttons = state.buttons;
        c_state.l2_state = state.l2_trigger;
        c_state.r2_state = state.r2_trigger;
        c_state.left_x  = state.left_stick_x;
        c_state.left_y  = state.left_stick_y;
        c_state.right_x = state.right_stick_x;
        c_state.right_y = state.right_stick_y;
        chiaki_session_set_controller_state(&m_chiaki_session, &c_state);
    }
}

void SessionManager::send_login_pin(const std::string& pin) {
    if (m_session_initialized.load()) {
        spdlog::info("[SessionManager] Sending login PIN (length: {}) to console", pin.length());
        chiaki_session_set_login_pin(&m_chiaki_session,
            reinterpret_cast<const uint8_t*>(pin.data()),
            pin.length());
        // Pulse PS button after entering PIN to confirm and dismiss login screen
        send_ps_button_pulse(150);
    } else {
        spdlog::warn("[SessionManager] Cannot send login PIN: session not initialized");
    }
}

void SessionManager::send_ps_button_pulse(uint32_t duration_ms) {
    if (!m_session_initialized.load()) return;

    // Cancel any previous pulse that might still be running
    if (m_ps_button_thread.joinable()) {
        m_ps_button_thread.request_stop();
        m_ps_button_thread.join();
    }

    m_ps_button_thread = std::jthread([this, duration_ms](std::stop_token st) {
        // Short pause to ensure session control channel is active
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Bail out if session was torn down while we waited
        if (st.stop_requested() || !m_session_initialized.load()) return;

        spdlog::info("[SessionManager] Sending PS button pulse ({} ms)...", duration_ms);

        // 1. Controller state with PS button held
        ChiakiControllerState ps_down{};
        chiaki_controller_state_set_idle(&ps_down);
        ps_down.buttons |= CHIAKI_CONTROLLER_BUTTON_PS;
        chiaki_session_set_controller_state(&m_chiaki_session, &ps_down);

        // Sleep in short intervals so stop_token is checked promptly
        const auto total = std::chrono::milliseconds(duration_ms);
        const auto tick  = std::chrono::milliseconds(10);
        for (auto elapsed = std::chrono::milliseconds(0);
             elapsed < total && !st.stop_requested();
             elapsed += tick) {
            std::this_thread::sleep_for(tick);
        }
        if (st.stop_requested() || !m_session_initialized.load()) return;

        // 2. Controller state with PS button released
        ChiakiControllerState ps_up{};
        chiaki_controller_state_set_idle(&ps_up);
        chiaki_session_set_controller_state(&m_chiaki_session, &ps_up);

        // 3. Dispatch go_home command over control stream to dismiss lockscreen
        chiaki_session_go_home(&m_chiaki_session);
        spdlog::info("[SessionManager] PS button pulse sent successfully.");
    });
}

void SessionManager::suspend_console() {
    if (m_session_initialized.load()) {
        spdlog::info("[SessionManager] Sending suspend (goto bed) command to console...");
        chiaki_session_goto_bed(&m_chiaki_session);
    } else {
        spdlog::warn("[SessionManager] Cannot suspend console: session not initialized");
    }
}

bool SessionManager::is_streaming() const {
    return m_state.load() == SessionState::Streaming;
}

StreamStats SessionManager::get_stats() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_stats_calc).count();
    if (elapsed_ms >= 500) {
        uint64_t cur_vf = m_video_frame_count.load();
        uint64_t cur_af = m_audio_frame_count.load();
        float dt = elapsed_ms / 1000.0f;

        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.video_fps = (cur_vf - m_last_video_frames.load()) / dt;
        m_stats.audio_fps = (cur_af - m_last_audio_frames.load()) / dt;
        m_last_video_frames = cur_vf;
        m_last_audio_frames = cur_af;
        m_last_stats_calc = now;
        if (m_session_running.load()) {
            m_stats.latency_ms = static_cast<float>(m_chiaki_session.rtt_us) / 1000.0f;
        }
    }
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    return m_stats;
}

VoidResult SessionManager::connect_cloud(const ludelo::auth::PSNTokens& tokens, const std::string& game_id, const StreamConfig& config) {
#if defined(LUDELO_CLOUD_EXPERIMENTAL) && (LUDELO_CLOUD_EXPERIMENTAL == 1)
    if (tokens.access_token.empty()) {
        return std::unexpected(Error(ErrorCode::NoActiveAccount, "No PSN access token available"));
    }
    if (game_id.empty()) {
        return std::unexpected(Error(ErrorCode::InvalidParameter, "game_id cannot be empty"));
    }
    if (m_state.load() != SessionState::Idle) {
        spdlog::info("[SessionManager] Cleaning up prior session before cloud connect...");
        disconnect();
    }

    m_config = config;
    set_state(SessionState::Discovering);

    // ── Step 1: Create Kamaji cloud session ──────────────
    ludelo::cloud::GaikaiClient gaikai;
    ludelo::cloud::CloudStreamConfig cloud_cfg{
        .video_bitrate = static_cast<int>(config.bitrate_kbps),
        .fps           = static_cast<int>(config.fps == FrameRate::FPS60 ? 60 : 30),
        .resolution    = config.resolution == Resolution::R1080p ? "1080p" :
                         config.resolution == Resolution::R720p  ? "720p"  : "1080p",
    };

    spdlog::info("[SessionManager] Creating Kamaji cloud session for game: {}", game_id);
    auto session_res = gaikai.create_session(tokens.access_token, game_id, cloud_cfg);
    if (!session_res) {
        spdlog::error("[SessionManager] Kamaji create_session failed: {}", session_res.error().message);
        set_state(SessionState::Error);
        return std::unexpected(session_res.error());
    }
    auto& cloud_session = session_res.value();
    spdlog::info("[SessionManager] Cloud session obtained: {} -> {}", 
        cloud_session.session_id, cloud_session.streaming_endpoint);

    // ── Step 2: Parse streaming endpoint (udp://host:port or wss://...) ──
    std::string remote_addr;
    uint16_t    remote_port = 9296;

    const std::string& ep = cloud_session.streaming_endpoint;
    std::string ep_stripped = ep;
    for (const auto* scheme : {"udp://", "tcp://", "wss://", "ws://"}) {
        if (ep_stripped.rfind(scheme, 0) == 0) {
            ep_stripped = ep_stripped.substr(std::strlen(scheme));
            break;
        }
    }
    auto colon = ep_stripped.rfind(':');
    if (colon != std::string::npos) {
        remote_addr = ep_stripped.substr(0, colon);
        try {
            remote_port = static_cast<uint16_t>(std::stoi(ep_stripped.substr(colon + 1)));
        } catch (...) {
            remote_port = 9296;
        }
    } else {
        remote_addr = ep_stripped;
    }

    // ── Step 3: Initialize Video & Audio decoders ────────
    auto codec = (config.codec == VideoCodec::H265) ? VideoCodec::H265 : VideoCodec::H264;
    m_video_decoder = std::make_unique<VideoDecoder>();
    auto dec_res = m_video_decoder->init(codec, false);
    if (!dec_res) {
        spdlog::error("[SessionManager] Cloud video decoder init failed: {}", dec_res.error().message);
        set_state(SessionState::Error);
        return std::unexpected(dec_res.error());
    }

    m_audio_decoder = std::make_unique<AudioDecoder>();
    (void)m_audio_decoder->init(48000, 2);

    // ── Step 4: Build TakionConfig for cloud session ──────
    TakionConfig takion_cfg{};
    takion_cfg.remote_addr   = remote_addr;
    takion_cfg.remote_port   = remote_port;
    takion_cfg.session_id    = cloud_session.session_id;
    takion_cfg.codec         = codec;
    takion_cfg.hdr           = config.hdr;
    takion_cfg.width         = config.resolution == Resolution::R1080p ? 1920 :
                               config.resolution == Resolution::R720p  ? 1280 : 1920;
    takion_cfg.height        = config.resolution == Resolution::R1080p ? 1080 :
                               config.resolution == Resolution::R720p  ?  720 : 1080;
    takion_cfg.fps           = static_cast<int>(config.fps == FrameRate::FPS60 ? 60 : 30);
    takion_cfg.bitrate_kbps  = config.bitrate_kbps;
    takion_cfg.bright_key    = {};
    takion_cfg.ambassador_key= {};

    // ── Step 5: Connect via TakionConnection ─────────────
    set_state(SessionState::Connecting);
    m_takion_connection = std::make_unique<TakionConnection>();

    m_takion_connection->set_on_video_frame([this](DecodedFrame& frame) {
        m_video_frame_count++;
        if (on_video_frame) on_video_frame(frame);
    });
    m_takion_connection->set_on_audio_frame([this](AudioFrame& audio) {
        m_audio_frame_count++;
        if (on_audio_frame) on_audio_frame(audio);
    });
    m_takion_connection->set_cloud_mode(true);

    auto conn_res = m_takion_connection->connect(takion_cfg);
    if (!conn_res) {
        spdlog::error("[SessionManager] Cloud Takion connect failed: {}", conn_res.error().message);
        (void)gaikai.end_session(cloud_session.session_id);
        m_takion_connection.reset();
        set_state(SessionState::Error);
        return std::unexpected(conn_res.error());
    }

    m_cloud_session_id = cloud_session.session_id;
    m_session_running  = true;

    // ── Step 6: Start 250Hz input thread ─────────────────
    m_input_thread = std::jthread([this](std::stop_token st) {
        input_loop(st);
    });

    set_state(SessionState::Streaming);
    spdlog::info("[SessionManager] Cloud streaming session active: {} @ {}:{}", 
        cloud_session.session_id, remote_addr, remote_port);
    return {};
#else
    (void)tokens;
    (void)game_id;
    (void)config;
    return std::unexpected(Error(ErrorCode::CloudNotAvailable, "Cloud streaming is experimental and disabled in this build."));
#endif
}

} // namespace ludelo::stream
