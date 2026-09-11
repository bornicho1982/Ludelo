// Archivo: src/PortalCore/Stream/AudioDecoder.h
#pragma once

#include "PortalCore/Common.h"
#include <span>
#include <vector>
#include <cstdint>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace portal::stream {

struct AudioFrame {
    std::vector<int16_t> samples;
    int num_samples;
    int channels;
    int sample_rate;
};

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    Result<void> init(int sample_rate = 48000, int channels = 2);
    Result<AudioFrame> decode(std::span<const uint8_t> opus_packet);
    uint32_t get_buffered_ms() const;

private:
    AVCodecContext* m_decoder = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_sample_rate = 48000;
    int m_channels = 2;
};

} // namespace portal::stream
