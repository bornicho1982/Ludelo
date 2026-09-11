// Archivo: src/PortalCore/Stream/SessionManager.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Stream/VideoDecoder.h"
#include "PortalCore/Stream/AudioDecoder.h"
#include "PortalCore/Discovery/ConsoleRegistry.h"
#include "PortalCore/Auth/PSNAuth.h"
#include "PortalCore/Cloud/GaikaiClient.h"

#include "PortalCore/Stream/TakionConnection.h"

#include <chiaki/session.h>
#include <chiaki/log.h>

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <chrono>

namespace portal::stream {

enum class SessionState {
    Idle,
    Discovering,
    Waking,
    Registering,
    Connecting,
    Negotiating,
    Streaming,
    Disconnecting,
    Error
};

struct StreamStats {
    float video_fps = 0.0f;
    float audio_fps = 0.0f;
    float video_bitrate_kbps = 0.0f;
    float audio_bitrate_kbps = 0.0f;
    float latency_ms = 0.0f;
    float packet_loss_percent = 0.0f;
    int fec_recovered = 0;
};

struct HapticEvent {
    uint8_t motor_left{0};
    uint8_t motor_right{0};
};

enum class TriggerMode {
    Off,
    Feedback,
    Weapon,
    Vibration
};

struct TriggerEffect {
    enum class Side { Left, Right } trigger{Side::Right};
    TriggerMode mode{TriggerMode::Off};
    uint8_t params[10]{0};
};

class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    VoidResult connect_local(const portal::discovery::RegisteredConsole& console, const StreamConfig& config);
    VoidResult connect_cloud(const portal::auth::PSNTokens& tokens, const std::string& game_id, const StreamConfig& config);
    void disconnect();
    bool is_streaming() const;
    StreamStats get_stats() const;
    void send_controller_state(const ControllerState& state);
    void send_login_pin(const std::string& pin);
    void send_ps_button_pulse(uint32_t duration_ms = 150);
    void suspend_console();

    // Callbacks
    std::function<void(DecodedFrame&)> on_video_frame;
    std::function<void(AudioFrame&)> on_audio_frame;
    std::function<void(HapticEvent&)> on_haptic_feedback;
    std::function<void(TriggerEffect&)> on_trigger_effect;
    std::function<void(SessionState)> on_state_change;
    std::function<void(const Error&)> on_error;
    std::function<void()> on_login_pin_requested;
    std::function<ControllerState()> on_poll_input;

    void set_input_poll_callback(std::function<ControllerState()> cb) {
        on_poll_input = std::move(cb);
    }

private:
    void set_state(SessionState state);
    void input_loop(std::stop_token st);

    static void on_chiaki_log(ChiakiLogLevel level, const char *msg, void *user);
    static void on_chiaki_event(ChiakiEvent *event, void *user);
    static bool on_chiaki_video_sample(uint8_t *buf, size_t buf_size, int32_t frames_lost, bool frame_recovered, void *user);
    static void on_chiaki_audio_frame(uint8_t *buf, size_t buf_size, void *user);

    std::atomic<SessionState> m_state{SessionState::Idle};
    mutable std::mutex m_state_mutex;

    // StreamStats guarded by its own mutex (read from UI thread, written from stats calc)
    mutable std::mutex m_stats_mutex;
    mutable StreamStats m_stats{};

    portal::discovery::RegisteredConsole m_console{};
    StreamConfig m_config{};

    std::unique_ptr<VideoDecoder> m_video_decoder;
    std::unique_ptr<AudioDecoder> m_audio_decoder;

    // Local session (Chiaki)
    ChiakiLog m_chiaki_log{};
    ChiakiSession m_chiaki_session{};
    std::atomic<bool> m_session_initialized{false};
    std::atomic<bool> m_session_running{false};

    // Cloud session (Takion direct)
    std::unique_ptr<TakionConnection> m_takion_connection;
    std::string m_cloud_session_id;

    // Input polling thread (250 Hz DualSense)
    std::jthread m_input_thread;

    // PS button pulse thread — jthread so we can safely cancel on disconnect
    std::jthread m_ps_button_thread;

    // Metrics
    mutable std::atomic<uint64_t> m_video_frame_count{0};
    mutable std::atomic<uint64_t> m_audio_frame_count{0};
    mutable std::atomic<uint64_t> m_last_video_frames{0};
    mutable std::atomic<uint64_t> m_last_audio_frames{0};
    mutable std::chrono::steady_clock::time_point m_last_stats_calc{std::chrono::steady_clock::now()};
};

} // namespace portal::stream
