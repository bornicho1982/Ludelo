// Archivo: src/PortalCore/Stream/TakionConnection.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Net/UDPSocket.h"
#include "PortalCore/Stream/VideoDecoder.h"
#include "PortalCore/Stream/AudioDecoder.h"
#include "PortalCore/Stream/FrameAssembler.h"
#include <functional>
#include <thread>
#include <atomic>
#include <array>
#include <optional>
#include <span>
#include <mutex>
#include <memory>
#include <string>

namespace portal::stream {

    struct TouchPoint {
        uint8_t id{0};
        uint8_t x{0}, y{0};
        bool active{false};
    };

    struct ControllerState {
        uint32_t buttons{0};
        int16_t left_stick_x{0}, left_stick_y{0};
        int16_t right_stick_x{0}, right_stick_y{0};
        uint8_t l2_trigger{0}, r2_trigger{0};
        std::array<TouchPoint, 2> touchpad{};
        struct { float x{0}, y{0}, z{0}; } gyro;
        struct { float x{0}, y{0}, z{0}; } accel;
        uint32_t sequence{0};
    };

    enum class TakionState {
        Disconnected,
        Connecting,
        Connected,
        Streaming,
        Error
    };

    struct TakionConfig {
        std::string remote_addr;
        uint16_t remote_port{9296};
        std::string session_id;
        std::array<uint8_t, 16> bright_key{};
        std::array<uint8_t, 16> ambassador_key{};
        VideoCodec codec{VideoCodec::H265};
        bool hdr{false};
        int width{1920};
        int height{1080};
        int fps{60};
        int bitrate_kbps{15000};
    };

    class TakionConnection {
    public:
        TakionConnection();
        ~TakionConnection();

        Result<void> connect(const TakionConfig& config);
        Result<void> connect(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port, const ByteBuffer& session_key);
        void disconnect();

        void send_controller_state(const ControllerState& state);
        
        using VideoFrameCallback = std::function<void(DecodedFrame&)>;
        using AudioFrameCallback = std::function<void(AudioFrame&)>;
        using AVPacketCallback = std::function<void(const ByteBuffer&)>;
        using ControlPacketCallback = std::function<void(const ByteBuffer&)>;
        
        void set_on_video_frame(VideoFrameCallback cb);
        void set_on_audio_frame(AudioFrameCallback cb);
        void set_on_av_packet(AVPacketCallback cb);
        void set_on_control_packet(ControlPacketCallback cb);
        
        void set_cloud_mode(bool cloud_mode);
        TakionState get_state() const;
        bool is_streaming() const;

    private:
        bool do_handshake();
        void heartbeat_loop(std::stop_token stoken);
        void receive_loop(std::stop_token stoken);
        void handle_packet(std::span<const uint8_t> data);
        void handle_control_packet(std::span<const uint8_t> data);
        void handle_video_packet(std::span<const uint8_t> data);

        void send_raw(std::span<const uint8_t> data);
        void send_data_ack(uint32_t seq_num);
        void send_streaminfo_ack();
        void send_controller_connection();

        std::shared_ptr<net::UDPSocket> m_socket;
        TakionConfig m_config{};
        
        std::atomic<TakionState> m_state{TakionState::Disconnected};
        std::atomic<bool> m_cloud_mode{false};
        
        std::unique_ptr<VideoDecoder> m_video_decoder;
        std::unique_ptr<AudioDecoder> m_audio_decoder;
        FrameAssembler m_frame_assembler;

        std::jthread m_heartbeat_thread;
        std::jthread m_receive_thread;

        VideoFrameCallback m_on_video_frame;
        AudioFrameCallback m_on_audio_frame;
        AVPacketCallback m_on_av_packet;
        ControlPacketCallback m_on_control_packet;
        
        uint32_t m_tag_local{0};
        uint32_t m_tag_remote{0};
        uint32_t m_seq_local{0};
        uint32_t m_a_rwnd{0x400000}; // 4 MB aligned with Chiaki TAKION_A_RWND
        
        std::array<uint8_t, 16> m_handshake_key{};
        std::array<uint8_t, 16> m_key_remote{};
        std::array<uint8_t, 16> m_iv_remote{};
        std::array<uint8_t, 16> m_key_local{};
        std::array<uint8_t, 16> m_iv_local{};
        bool m_crypt_initialized{false};

        std::mutex m_send_mutex;
        std::atomic<uint64_t> m_frames_received{0};
        std::atomic<uint64_t> m_frames_decoded{0};
        void* m_aes_ecb_ctx{nullptr};
        std::vector<uint8_t> m_keystream_buffer;
    };
}
