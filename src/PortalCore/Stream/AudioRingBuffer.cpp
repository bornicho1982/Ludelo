// Archivo: src/PortalCore/Stream/AudioRingBuffer.cpp
#include "AudioRingBuffer.h"
#include <algorithm>
#include <cstring>

namespace portal::stream {

AudioRingBuffer::AudioRingBuffer() = default;

AudioRingBuffer::~AudioRingBuffer() = default;

void AudioRingBuffer::configure(size_t capacity_samples, int channels) {
    m_capacity = capacity_samples * channels;
    m_buffer.resize(m_capacity + 1); // +1 to distinguish full from empty
    m_channels = channels;
    clear();
}

size_t AudioRingBuffer::write(std::span<const int16_t> samples) {
    if (m_capacity == 0 || samples.empty()) return 0;

    size_t head = m_head.load(std::memory_order_relaxed);
    size_t tail = m_tail.load(std::memory_order_acquire);
    size_t size = m_buffer.size();
    
    size_t available = (tail + size - head - 1) % size;
    size_t to_write = std::min(available, samples.size());

    if (to_write == 0) return 0;

    size_t first_part = std::min(to_write, size - head);
    std::memcpy(m_buffer.data() + head, samples.data(), first_part * sizeof(int16_t));

    if (first_part < to_write) {
        std::memcpy(m_buffer.data(), samples.data() + first_part, (to_write - first_part) * sizeof(int16_t));
    }

    m_head.store((head + to_write) % size, std::memory_order_release);
    return to_write;
}

size_t AudioRingBuffer::read(std::span<int16_t> out) {
    if (m_capacity == 0 || out.empty()) return 0;

    size_t tail = m_tail.load(std::memory_order_relaxed);
    size_t head = m_head.load(std::memory_order_acquire);
    size_t size = m_buffer.size();

    size_t available = (head + size - tail) % size;
    size_t to_read = std::min(available, out.size());

    if (to_read == 0) return 0;

    size_t first_part = std::min(to_read, size - tail);
    std::memcpy(out.data(), m_buffer.data() + tail, first_part * sizeof(int16_t));

    if (first_part < to_read) {
        std::memcpy(out.data() + first_part, m_buffer.data(), (to_read - first_part) * sizeof(int16_t));
    }

    m_tail.store((tail + to_read) % size, std::memory_order_release);
    return to_read;
}

size_t AudioRingBuffer::available_samples() const {
    if (m_capacity == 0) return 0;
    size_t head = m_head.load(std::memory_order_acquire);
    size_t tail = m_tail.load(std::memory_order_acquire);
    return (head + m_buffer.size() - tail) % m_buffer.size();
}

size_t AudioRingBuffer::available_space() const {
    if (m_capacity == 0) return 0;
    return m_capacity - available_samples();
}

void AudioRingBuffer::clear() {
    m_head.store(0, std::memory_order_release);
    m_tail.store(0, std::memory_order_release);
}

} // namespace portal::stream
