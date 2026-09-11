// Archivo: src/PortalCore/Stream/VideoDecoder.h
#pragma once

#include "PortalCore/Common.h"
#include <span>
#include <string>
#include <memory>
#include <vector>

struct AVCodecContext;
struct AVFrame;
struct AVBufferRef;
struct AVPacket;
struct SwsContext;

namespace portal::stream {

struct DecodedFrame {
    int width = 0;
    int height = 0;
    int format = 0; // FFmpeg format
    int64_t pts = 0;
    std::vector<const uint8_t*> data;
    std::vector<int> strides;
    bool is_hw = false;
    void* hw_texture = nullptr;
    const uint8_t* rgba_data = nullptr;
    size_t rgba_size = 0;
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    Result<void> init(VideoCodec codec, bool hw_preferred);
    Result<DecodedFrame> decode(std::span<const uint8_t> nalu_data);
    void flush();
    void reset();
    std::string get_hw_device_type() const;

private:
    void cleanup();

    AVCodecContext* m_codec_ctx = nullptr;
    AVFrame* m_frame = nullptr;
    AVFrame* m_temp_frame = nullptr;
    AVFrame* m_sw_frame = nullptr;
    AVPacket* m_packet = nullptr;
    AVBufferRef* m_hw_device_ctx = nullptr;
    
    struct SwsContext* m_sws_ctx = nullptr;
    int m_sws_src_w = 0;
    int m_sws_src_h = 0;
    int m_sws_src_fmt = -1;
    std::vector<uint8_t> m_rgba_buffer;

    VideoCodec m_codec = VideoCodec::H264;
    bool m_is_hw = false;
    std::string m_hw_type = "software";
};

} // namespace portal::stream
