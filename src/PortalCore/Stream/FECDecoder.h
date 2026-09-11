// Archivo: src/PortalCore/Stream/FECDecoder.h
#pragma once

#include "PortalCore/Common.h"
#include <vector>
#include <optional>
#include <cstdint>

namespace portal::stream {

    class FECDecoder {
    public:
        FECDecoder(size_t k, size_t m);
        
        void add_packet(uint32_t seq, const ByteBuffer& data, bool is_fec);
        std::optional<std::vector<ByteBuffer>> try_recover();
        bool is_complete() const;
        void reset();

    private:
        size_t m_k; // Data packets
        size_t m_m; // Parity packets
        
        std::vector<std::optional<ByteBuffer>> m_data_packets;
        std::vector<std::optional<ByteBuffer>> m_fec_packets;
        
        size_t m_received_data_count{0};
        size_t m_received_fec_count{0};
    };
}
