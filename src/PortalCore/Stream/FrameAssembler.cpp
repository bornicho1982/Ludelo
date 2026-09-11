// Archivo: src/PortalCore/Stream/FrameAssembler.cpp
#include "FrameAssembler.h"
#include <spdlog/spdlog.h>

namespace portal::stream {

    FrameAssembler::FrameAssembler() = default;

    void FrameAssembler::set_request_idr_callback(RequestIDRCallback cb) {
        m_on_request_idr = std::move(cb);
    }

    std::optional<AssembledFrame> FrameAssembler::add_packet(const TakionAVPacket& packet) {
        if (packet.stream_type != 0) return std::nullopt; // Audio not implemented in this snippet
        
        if (packet.frame_index <= m_last_completed_frame && m_last_completed_frame > 0) {
            return std::nullopt; // Old frame
        }

        auto& frame = m_reorder_buffer[packet.frame_index];
        frame.frame_index = packet.frame_index;
        frame.expected_units = packet.units_in_frame;
        frame.pts = packet.pts;
        
        // IDR check - check both H.264 and H.265 NAL unit types
        if (packet.unit_index == 0 && packet.data.size() > 6) {
            // Check Annex B start code after 2-byte unit header
            size_t nalu_offset = 2;
            if (packet.data.size() > 6 && packet.data[2] == 0 && packet.data[3] == 0) {
                if (packet.data[4] == 1) nalu_offset = 5;
                else if (packet.data[4] == 0 && packet.data[5] == 1) nalu_offset = 6;
            }
            if (nalu_offset < packet.data.size()) {
                uint8_t nal_byte = packet.data[nalu_offset];
                uint8_t h264_type = nal_byte & 0x1F;
                uint8_t h265_type = (nal_byte >> 1) & 0x3F;
                // H.264 IDR=5, SPS=7, PPS=8 | H.265 IDR_W_RADL=19, IDR_N_LP=20, CRA=21, VPS=32, SPS=33, PPS=34
                frame.is_idr = (h264_type == 5 || h264_type == 7 || h264_type == 8 ||
                                h265_type == 19 || h265_type == 20 || h265_type == 21 ||
                                h265_type == 32 || h265_type == 33 || h265_type == 34);
            }
        }

        frame.units[packet.unit_index] = packet.data;

        if (frame.units.size() == frame.expected_units) {
            // Frame is complete
            AssembledFrame assembled;
            assembled.frame_number = frame.frame_index;
            assembled.is_idr = frame.is_idr;
            assembled.codec = (packet.codec == 1) ? VideoCodec::H264 : VideoCodec::H265;
            assembled.pts = frame.pts;

            size_t total_size = 0;
            for (const auto& [_, unit] : frame.units) {
                if (unit.size() > 2) {
                    total_size += (unit.size() - 2);
                }
            }

            assembled.data.reserve(total_size);
            for (const auto& [_, unit] : frame.units) {
                if (unit.size() > 2) {
                    assembled.data.insert(assembled.data.end(), unit.begin() + 2, unit.end());
                }
            }

            m_last_completed_frame = frame.frame_index;
            m_reorder_buffer.erase(m_reorder_buffer.begin(), m_reorder_buffer.upper_bound(packet.frame_index));
            
            return assembled;
        }

        // Prune old frames and request IDR if we get too far behind
        if (m_reorder_buffer.size() > 10) {
            m_reorder_buffer.clear();
            if (m_on_request_idr) {
                m_on_request_idr();
            }
        }

        return std::nullopt;
    }
}
