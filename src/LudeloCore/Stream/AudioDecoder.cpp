// Archivo: src/LudeloCore/Stream/AudioDecoder.cpp
#include "AudioDecoder.h"
#include <spdlog/spdlog.h>
#include <algorithm>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

namespace ludelo::stream {

AudioDecoder::AudioDecoder() = default;

AudioDecoder::~AudioDecoder() {
    if (m_decoder) {
        avcodec_free_context(&m_decoder);
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
}

Result<void> AudioDecoder::init(int sample_rate, int channels) {
    if (m_decoder) {
        avcodec_free_context(&m_decoder);
    }
    if (!m_frame) m_frame = av_frame_alloc();
    if (!m_packet) m_packet = av_packet_alloc();

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    if (!codec) {
        spdlog::get("ludelo")->error("Opus decoder not found in FFmpeg");
        return std::unexpected(ErrorCode::DecoderInitFailed);
    }

    m_decoder = avcodec_alloc_context3(codec);
    if (!m_decoder) {
        return std::unexpected(ErrorCode::DecoderInitFailed);
    }

    m_decoder->sample_rate = sample_rate;
#if LIBAVCODEC_VERSION_MAJOR >= 59
    av_channel_layout_default(&m_decoder->ch_layout, channels);
#else
    m_decoder->channels = channels;
#endif

    if (avcodec_open2(m_decoder, codec, nullptr) < 0) {
        spdlog::get("ludelo")->error("Failed to open Opus codec");
        return std::unexpected(ErrorCode::DecoderInitFailed);
    }

    m_sample_rate = sample_rate;
    m_channels = channels;
    
    spdlog::get("ludelo")->info("AudioDecoder initialized with FFmpeg Opus, {} Hz, {} channels", sample_rate, channels);
    return {};
}

Result<AudioFrame> AudioDecoder::decode(std::span<const uint8_t> opus_packet) {
    if (!m_decoder) return std::unexpected(ErrorCode::InvalidState);

    m_packet->data = (uint8_t*)opus_packet.data();
    m_packet->size = static_cast<int>(opus_packet.size());

    int ret = avcodec_send_packet(m_decoder, m_packet);
    if (ret < 0) {
        return std::unexpected(ErrorCode::DecodeError);
    }

    ret = avcodec_receive_frame(m_decoder, m_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return std::unexpected(ErrorCode::DecodeError);
    } else if (ret < 0) {
        return std::unexpected(ErrorCode::DecodeError);
    }

    AudioFrame frame;
    frame.num_samples = m_frame->nb_samples;
    frame.channels = m_channels;
    frame.sample_rate = m_sample_rate;

    frame.samples.resize(frame.num_samples * frame.channels);
    
    if (m_decoder->sample_fmt == AV_SAMPLE_FMT_FLT || m_decoder->sample_fmt == AV_SAMPLE_FMT_FLTP) {
        float* ch0 = (float*)m_frame->extended_data[0];
        float* ch1 = (float*)m_frame->extended_data[m_channels > 1 ? 1 : 0];
        for (int i = 0; i < frame.num_samples; ++i) {
            float s0 = ch0[i] * 32767.0f;
            float s1 = ch1[i] * 32767.0f;
            frame.samples[i * 2 + 0] = static_cast<int16_t>(std::clamp(s0, -32768.0f, 32767.0f));
            if (m_channels > 1) {
                frame.samples[i * 2 + 1] = static_cast<int16_t>(std::clamp(s1, -32768.0f, 32767.0f));
            }
        }
    } else if (m_decoder->sample_fmt == AV_SAMPLE_FMT_S16) {
        memcpy(frame.samples.data(), m_frame->data[0], frame.num_samples * m_channels * sizeof(int16_t));
    }

    return frame;
}

uint32_t AudioDecoder::get_buffered_ms() const {
    return 0;
}

} // namespace ludelo::stream
