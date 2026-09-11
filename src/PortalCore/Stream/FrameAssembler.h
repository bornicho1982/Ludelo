// Archivo: src/PortalCore/Stream/FrameAssembler.h
#pragma once

#include "PortalCore/Common.h"
#include <vector>
#include <optional>
#include <cstdint>
#include <functional>
#include <map>

namespace portal::stream {

    struct TakionAVPacket {
        uint8_t stream_type; // 0=Video, 1=Audio
        uint32_t seq;
        uint32_t unit_index;
        uint32_t units_in_frame;
        uint32_t frame_index;
        bool is_fec;
        ByteBuffer data;
        uint64_t pts;
        uint8_t codec{0};
    };

    struct AssembledFrame {
        uint32_t frame_number;
        bool is_idr;
        VideoCodec codec;
        ByteBuffer data;
        uint64_t pts;
    };

    class FrameAssembler {
    public:
        FrameAssembler();
        
        using RequestIDRCallback = std::function<void()>;
        void set_request_idr_callback(RequestIDRCallback cb);
        
        std::optional<AssembledFrame> add_packet(const TakionAVPacket& packet);

    private:
        struct FrameBuffer {
            uint32_t frame_index;
            uint32_t expected_units;
            uint64_t pts;
            std::map<uint32_t, ByteBuffer> units;
            bool is_idr;
        };

        std::map<uint32_t, FrameBuffer> m_reorder_buffer;
        uint32_t m_last_completed_frame{0};
        RequestIDRCallback m_on_request_idr;
    };
}
