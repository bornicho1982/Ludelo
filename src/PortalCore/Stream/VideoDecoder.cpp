// Archivo: src/PortalCore/Stream/VideoDecoder.cpp
#include "VideoDecoder.h"
#include <spdlog/spdlog.h>
#include <d3d11.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace portal::stream {

VideoDecoder::VideoDecoder() {
    m_frame = av_frame_alloc();
    m_temp_frame = av_frame_alloc();
    m_sw_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
}

VideoDecoder::~VideoDecoder() {
    cleanup();
    av_frame_free(&m_frame);
    av_frame_free(&m_temp_frame);
    av_frame_free(&m_sw_frame);
    av_packet_free(&m_packet);
}

void VideoDecoder::cleanup() {
    if (m_codec_ctx) {
        avcodec_free_context(&m_codec_ctx);
    }
    if (m_hw_device_ctx) {
        av_buffer_unref(&m_hw_device_ctx);
    }
    if (m_sws_ctx) {
        sws_freeContext(m_sws_ctx);
        m_sws_ctx = nullptr;
    }
    m_rgba_buffer.clear();
    m_sws_src_w = 0;
    m_sws_src_h = 0;
    m_sws_src_fmt = -1;
    m_is_hw = false;
    m_hw_type = "software";
}

Result<void> VideoDecoder::init(VideoCodec codec, bool hw_preferred) {
    cleanup();
    m_codec = codec;
    
    const AVCodec* av_codec = nullptr;
    if (codec == VideoCodec::H264) {
        av_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    } else if (codec == VideoCodec::H265) {
        av_codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    }

    if (!av_codec) {
        spdlog::get("portal")->error("Unsupported video codec");
        return std::unexpected(ErrorCode::UnsupportedCodec);
    }

    m_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!m_codec_ctx) {
        return std::unexpected(ErrorCode::OutOfMemory);
    }

    // Configure for ultra-low latency interactive streaming
    m_codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codec_ctx->flags2 |= AV_CODEC_FLAG2_FAST;
    m_codec_ctx->thread_type = FF_THREAD_SLICE;
    m_codec_ctx->thread_count = 4;
    m_codec_ctx->has_b_frames = 0;
    m_codec_ctx->max_b_frames = 0;
    m_codec_ctx->delay = 0;

    if (hw_preferred) {
        // Try D3D11VA
        if (av_hwdevice_ctx_create(&m_hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) >= 0) {
            m_codec_ctx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
            m_is_hw = true;
            m_hw_type = "d3d11va";
        } 
        // Try CUDA (NVDEC)
        else if (av_hwdevice_ctx_create(&m_hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) >= 0) {
            m_codec_ctx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
            m_is_hw = true;
            m_hw_type = "cuda";
        }
    }

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "tune", "zerolatency", 0);
    av_dict_set(&opts, "flags", "low_delay", 0);
    int open_ret = avcodec_open2(m_codec_ctx, av_codec, &opts);
    av_dict_free(&opts);

    if (open_ret < 0) {
        spdlog::get("portal")->error("Failed to open codec");
        return std::unexpected(ErrorCode::DecoderInitFailed);
    }

    spdlog::get("portal")->info("VideoDecoder initialized with HW type: {}", m_hw_type);
    return {};
}

Result<DecodedFrame> VideoDecoder::decode(std::span<const uint8_t> nalu_data) {
    if (!m_codec_ctx || nalu_data.empty()) {
        return std::unexpected(ErrorCode::InvalidParameter);
    }

    m_packet->data = const_cast<uint8_t*>(nalu_data.data());
    m_packet->size = static_cast<int>(nalu_data.size());

    int ret = avcodec_send_packet(m_codec_ctx, m_packet);
    if (ret == AVERROR(EAGAIN)) {
        // Buffer full: drain a frame and retry packet push
        av_frame_unref(m_frame);
        (void)avcodec_receive_frame(m_codec_ctx, m_frame);
        ret = avcodec_send_packet(m_codec_ctx, m_packet);
    }
    if (ret < 0) {
        spdlog::get("portal")->error("Error sending packet for decoding: {}", ret);
        return std::unexpected(ErrorCode::DecodeError);
    }

    // Drain all available frames from decoder and keep ONLY the latest frame
    AVFrame* cur = m_frame;
    AVFrame* next = m_temp_frame;
    bool has_frame = false;

    while (true) {
        av_frame_unref(next);
        int r = avcodec_receive_frame(m_codec_ctx, next);
        if (r == 0) {
            has_frame = true;
            std::swap(cur, next);
        } else {
            if (r != AVERROR(EAGAIN) && r != AVERROR_EOF) {
                spdlog::get("portal")->warn("avcodec_receive_frame error: {}", r);
            }
            break;
        }
    }

    if (!has_frame) {
        return std::unexpected(ErrorCode::NoFrame);
    }

    AVFrame* output_frame = cur;

    DecodedFrame decoded_frame{};
    decoded_frame.width = output_frame->width;
    decoded_frame.height = output_frame->height;
    decoded_frame.format = output_frame->format;
    decoded_frame.pts = output_frame->pts;
    decoded_frame.is_hw = m_is_hw;

    if (m_is_hw && output_frame->format == AV_PIX_FMT_D3D11) {
        decoded_frame.hw_texture = output_frame->data[0];
        if (av_hwframe_transfer_data(m_sw_frame, cur, 0) >= 0) {
            output_frame = m_sw_frame;
        }
    } else {
        decoded_frame.hw_texture = nullptr;
    }

    for (int i = 0; i < 8 && output_frame->data[i] != nullptr; ++i) {
        decoded_frame.data.push_back(output_frame->data[i]);
        decoded_frame.strides.push_back(output_frame->linesize[i]);
    }

    // Ultra-fast SIMD color conversion to RGBA using libswscale (AVX2/SSSE3)
    int width = output_frame->width;
    int height = output_frame->height;
    AVPixelFormat src_fmt = static_cast<AVPixelFormat>(output_frame->format);

    if (width > 0 && height > 0 && src_fmt != AV_PIX_FMT_NONE) {
        if (!m_sws_ctx || m_sws_src_w != width || m_sws_src_h != height || m_sws_src_fmt != src_fmt) {
            if (m_sws_ctx) {
                sws_freeContext(m_sws_ctx);
            }
            m_sws_ctx = sws_getContext(
                width, height, src_fmt,
                width, height, AV_PIX_FMT_RGBA,
                SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
            );
            if (m_sws_ctx) {
                int src_range = (output_frame->color_range == AVCOL_RANGE_JPEG) ? 1 : 0;
                int dst_range = 1; // RGBA full range [0-255]
                const int* inv_table = sws_getCoefficients(SWS_CS_ITU709);
                if (output_frame->colorspace == AVCOL_SPC_BT470BG || output_frame->colorspace == AVCOL_SPC_SMPTE170M) {
                    inv_table = sws_getCoefficients(SWS_CS_ITU601);
                }
                const int* table = sws_getCoefficients(SWS_CS_DEFAULT);
                sws_setColorspaceDetails(m_sws_ctx, inv_table, src_range, table, dst_range, 0, 1 << 16, 1 << 16);
            }
            m_sws_src_w = width;
            m_sws_src_h = height;
            m_sws_src_fmt = src_fmt;
        }

        if (m_sws_ctx) {
            size_t needed = static_cast<size_t>(width) * height * 4;
            if (m_rgba_buffer.size() != needed) {
                m_rgba_buffer.resize(needed);
            }

            uint8_t* dst_data[4] = { m_rgba_buffer.data(), nullptr, nullptr, nullptr };
            int dst_linesize[4] = { width * 4, 0, 0, 0 };

            int scaled = sws_scale(
                m_sws_ctx,
                output_frame->data,
                output_frame->linesize,
                0,
                height,
                dst_data,
                dst_linesize
            );

            if (scaled > 0) {
                decoded_frame.rgba_data = m_rgba_buffer.data();
                decoded_frame.rgba_size = needed;
            }
        }
    }

    return decoded_frame;
}

void VideoDecoder::flush() {
    if (m_codec_ctx) {
        avcodec_flush_buffers(m_codec_ctx);
    }
}

void VideoDecoder::reset() {
    flush();
}

std::string VideoDecoder::get_hw_device_type() const {
    return m_hw_type;
}

} // namespace portal::stream
