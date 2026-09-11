// Archivo: src/PortalCore/Stream/AudioRingBuffer.h
#pragma once

#include <vector>
#include <span>
#include <atomic>
#include <cstdint>

namespace portal::stream {

class AudioRingBuffer {
public:
    AudioRingBuffer();
    ~AudioRingBuffer();

    void configure(size_t capacity_samples, int channels);
    size_t write(std::span<const int16_t> samples);
    size_t read(std::span<int16_t> out);
    size_t available_samples() const;
    size_t available_space() const;
    void clear();

private:
    std::vector<int16_t> m_buffer;
    size_t m_capacity = 0;
    int m_channels = 2;
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
};

} // namespace portal::stream
