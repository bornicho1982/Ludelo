#include "PortalCore/Stream/FECDecoder.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace portal::stream {

    FECDecoder::FECDecoder(size_t k, size_t m) : m_k(k), m_m(m) {
        m_data_packets.resize(k);
        m_fec_packets.resize(m);
    }

    void FECDecoder::add_packet(uint32_t seq, const ByteBuffer& data, bool is_fec) {
        if (is_fec) {
            uint32_t index = seq % m_m;
            if (!m_fec_packets[index]) {
                m_fec_packets[index] = data;
                m_received_fec_count++;
            }
        } else {
            uint32_t index = seq % m_k;
            if (!m_data_packets[index]) {
                m_data_packets[index] = data;
                m_received_data_count++;
            }
        }
    }

    std::optional<std::vector<ByteBuffer>> FECDecoder::try_recover() {
        if (is_complete()) {
            std::vector<ByteBuffer> result;
            for (const auto& pkt : m_data_packets) {
                result.push_back(*pkt);
            }
            return result;
        }

        if (m_received_data_count + m_received_fec_count < m_k) {
            return std::nullopt; // Not enough packets to recover
        }

        // Basic XOR recovery (assuming M=1 for simplicity in this implementation)
        // A full implementation would use Reed-Solomon or similar.
        if (m_m == 1 && m_received_fec_count == 1 && m_received_data_count == m_k - 1) {
            ByteBuffer recovered = *m_fec_packets[0];
            size_t missing_idx = 0;
            
            for (size_t i = 0; i < m_k; ++i) {
                if (m_data_packets[i]) {
                    const auto& data = *m_data_packets[i];
                    for (size_t j = 0; j < recovered.size() && j < data.size(); ++j) {
                        recovered[j] ^= data[j];
                    }
                } else {
                    missing_idx = i;
                }
            }
            
            m_data_packets[missing_idx] = recovered;
            m_received_data_count++;
            
            std::vector<ByteBuffer> result;
            for (const auto& pkt : m_data_packets) {
                result.push_back(*pkt);
            }
            return result;
        }

        return std::nullopt;
    }

    bool FECDecoder::is_complete() const {
        return m_received_data_count == m_k;
    }

    void FECDecoder::reset() {
        std::fill(m_data_packets.begin(), m_data_packets.end(), std::nullopt);
        std::fill(m_fec_packets.begin(), m_fec_packets.end(), std::nullopt);
        m_received_data_count = 0;
        m_received_fec_count = 0;
    }
}
