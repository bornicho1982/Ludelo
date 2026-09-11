// Archivo: src/PortalCore/Stream/TakionConnection.cpp
#include "TakionConnection.h"
#include "PortalCore/Crypto/PS5Protocol.h"
#include "PortalCore/Crypto/SecureRandom.h"
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/hmac.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <format>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace portal::stream {

static std::string b64_encode(std::span<const uint8_t> data) {
    static const char* kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kAlphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(kAlphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static void encode_varint(ByteBuffer& buf, uint64_t val) {
    while (val > 0x7F) {
        buf.push_back(static_cast<uint8_t>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    buf.push_back(static_cast<uint8_t>(val & 0x7F));
}

static void encode_field_varint(ByteBuffer& buf, uint32_t field_no, uint64_t val) {
    encode_varint(buf, (field_no << 3) | 0);
    encode_varint(buf, val);
}

static void encode_field_bytes(ByteBuffer& buf, uint32_t field_no, std::span<const uint8_t> data) {
    encode_varint(buf, (field_no << 3) | 2);
    encode_varint(buf, data.size());
    buf.insert(buf.end(), data.begin(), data.end());
}

static void encode_field_string(ByteBuffer& buf, uint32_t field_no, const std::string& str) {
    encode_field_bytes(buf, field_no, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
}

struct ProtoField {
    uint32_t field_number;
    uint32_t wire_type;
    uint64_t varint_val{0};
    std::span<const uint8_t> bytes_val{};
};

static std::vector<ProtoField> parse_protobuf(std::span<const uint8_t> data) {
    std::vector<ProtoField> fields;
    size_t p = 0;
    while (p < data.size()) {
        uint64_t tag = 0;
        int shift = 0;
        while (p < data.size()) {
            uint8_t b = data[p++];
            tag |= static_cast<uint64_t>(b & 0x7F) << shift;
            if (!(b & 0x80)) break;
            shift += 7;
        }
        uint32_t field_no = static_cast<uint32_t>(tag >> 3);
        uint32_t wire_type = static_cast<uint32_t>(tag & 7);
        ProtoField f;
        f.field_number = field_no;
        f.wire_type = wire_type;
        if (wire_type == 0) {
            uint64_t val = 0;
            int s = 0;
            while (p < data.size()) {
                uint8_t b = data[p++];
                val |= static_cast<uint64_t>(b & 0x7F) << s;
                if (!(b & 0x80)) break;
                s += 7;
            }
            f.varint_val = val;
            fields.push_back(f);
        } else if (wire_type == 2) {
            uint64_t len = 0;
            int s = 0;
            while (p < data.size()) {
                uint8_t b = data[p++];
                len |= static_cast<uint64_t>(b & 0x7F) << s;
                if (!(b & 0x80)) break;
                s += 7;
            }
            if (p + len <= data.size()) {
                f.bytes_val = data.subspan(p, len);
                p += len;
                fields.push_back(f);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return fields;
}

static inline void counter_add(uint8_t* out, const uint8_t* base, uint64_t v) {
    size_t i = 0;
    do {
        uint64_t r = static_cast<uint64_t>(base[i]) + (v & 0xff);
        out[i] = static_cast<uint8_t>(r & 0xff);
        v = (v >> 8) + (r >> 8);
        i++;
    } while (i < 16 && v);

    if (i < 16) {
        std::memcpy(out + i, base + i, 16 - i);
    }
}

static ByteBuffer make_message_header(uint32_t tag, uint32_t key_pos, uint8_t chunk_type, uint8_t chunk_flags, size_t data_size) {
    ByteBuffer hdr(16);
    uint32_t tag_be = htonl(tag);
    std::memcpy(hdr.data() + 0, &tag_be, 4);
    std::memset(hdr.data() + 4, 0, 4); // GMAC zeros
    uint32_t kp_be = htonl(key_pos);
    std::memcpy(hdr.data() + 8, &kp_be, 4);
    hdr[12] = chunk_type;
    hdr[13] = chunk_flags;
    uint16_t ps_be = htons(static_cast<uint16_t>(4 + data_size));
    std::memcpy(hdr.data() + 14, &ps_be, 2);
    return hdr;
}

TakionConnection::TakionConnection() = default;

TakionConnection::~TakionConnection() {
    disconnect();
}

Result<void> TakionConnection::connect(const TakionConfig& config) {
    m_config = config;
    m_state = TakionState::Connecting;
    spdlog::info("TakionConnection connecting to {}:{}", m_config.remote_addr, m_config.remote_port);

    m_socket = std::make_shared<net::UDPSocket>();
    auto bind_res = m_socket->bind(0);
    if (!bind_res.has_value()) {
        m_state = TakionState::Error;
        spdlog::error("Failed to bind UDP socket for Takion");
        return std::unexpected(Error{ErrorCode::NetworkError, "Failed to bind UDP socket"});
    }
    (void)m_socket->set_recv_buffer(4 * 1024 * 1024);
    (void)m_socket->set_send_buffer(1024 * 1024);
    (void)m_socket->set_nonblocking(true);

    m_video_decoder = std::make_unique<VideoDecoder>();
    auto dec_init = m_video_decoder->init(m_config.codec, true);
    if (!dec_init.has_value()) {
        spdlog::warn("VideoDecoder hardware init failed, falling back to software");
        (void)m_video_decoder->init(m_config.codec, false);
    }

    if (!do_handshake()) {
        m_state = TakionState::Error;
        spdlog::error("Takion v12 handshake with PS5 failed");
        return std::unexpected(Error{ErrorCode::HandshakeFailed, "Takion handshake failed"});
    }

    m_state = TakionState::Streaming;
    spdlog::info("Takion v12 handshake SUCCESSFUL! Streaming established on port {}", m_config.remote_port);

    m_receive_thread = std::jthread([this](std::stop_token st) { receive_loop(st); });
    m_heartbeat_thread = std::jthread([this](std::stop_token st) { heartbeat_loop(st); });

    return {};
}

Result<void> TakionConnection::connect(std::shared_ptr<net::UDPSocket> socket, const std::string& remote_addr, uint16_t remote_port, const ByteBuffer& session_key) {
    TakionConfig cfg{};
    cfg.remote_addr = remote_addr;
    cfg.remote_port = remote_port;
    cfg.session_id = std::string(session_key.begin(), session_key.end());
    return connect(cfg);
}

void TakionConnection::disconnect() {
    if (m_state.load() == TakionState::Disconnected) return;
    m_state = TakionState::Disconnected;

    if (m_heartbeat_thread.joinable()) {
        m_heartbeat_thread.request_stop();
        m_heartbeat_thread.join();
    }
    if (m_receive_thread.joinable()) {
        m_receive_thread.request_stop();
        m_receive_thread.join();
    }

    if (m_aes_ecb_ctx) {
        EVP_CIPHER_CTX_free(reinterpret_cast<EVP_CIPHER_CTX*>(m_aes_ecb_ctx));
        m_aes_ecb_ctx = nullptr;
    }

    if (m_video_decoder) {
        m_video_decoder->reset();
    }

    if (m_socket) {
        m_socket->close();
        m_socket.reset();
    }

    spdlog::info("TakionConnection disconnected");
}

void TakionConnection::send_raw(std::span<const uint8_t> data) {
    if (!m_socket || data.empty()) return;
    std::lock_guard<std::mutex> lock(m_send_mutex);
    (void)m_socket->send_to(m_config.remote_addr, m_config.remote_port, ByteBuffer(data.begin(), data.end()));
}

void TakionConnection::send_data_ack(uint32_t seq_num) {
    ByteBuffer pl(12);
    uint32_t s_be = htonl(seq_num);
    uint32_t rw_be = htonl(m_a_rwnd);
    uint32_t z_be = 0;
    std::memcpy(pl.data(), &s_be, 4);
    std::memcpy(pl.data() + 4, &rw_be, 4);
    std::memcpy(pl.data() + 8, &z_be, 4);

    ByteBuffer hdr = make_message_header(m_tag_remote, 0, 3, 0, 12);
    ByteBuffer pkt;
    pkt.push_back(0);
    pkt.insert(pkt.end(), hdr.begin(), hdr.end());
    pkt.insert(pkt.end(), pl.begin(), pl.end());
    send_raw(pkt);
}

void TakionConnection::send_streaminfo_ack() {
    ByteBuffer msg;
    encode_field_varint(msg, 1, 14); // STREAMINFOACK

    uint32_t seq = m_seq_local++;
    ByteBuffer prefix(9);
    uint32_t s_be = htonl(seq);
    uint16_t ch_be = htons(9);
    uint16_t st_be = htons(0);
    std::memcpy(prefix.data(), &s_be, 4);
    std::memcpy(prefix.data() + 4, &ch_be, 2);
    std::memcpy(prefix.data() + 6, &st_be, 2);
    prefix[8] = 0;

    ByteBuffer data;
    data.insert(data.end(), prefix.begin(), prefix.end());
    data.insert(data.end(), msg.begin(), msg.end());

    ByteBuffer hdr = make_message_header(m_tag_remote, 0, 0, 1, data.size());
    ByteBuffer pkt;
    pkt.push_back(0);
    pkt.insert(pkt.end(), hdr.begin(), hdr.end());
    pkt.insert(pkt.end(), data.begin(), data.end());
    send_raw(pkt);
}

void TakionConnection::send_controller_connection() {
    ByteBuffer pl;
    encode_field_varint(pl, 1, 1); // connected = true
    encode_field_varint(pl, 3, 1); // controller_type = 1 (DualSense)

    ByteBuffer msg;
    encode_field_varint(msg, 1, 21); // CONTROLLERCONNECTION
    encode_field_bytes(msg, 22, pl);

    uint32_t seq = m_seq_local++;
    ByteBuffer prefix(9);
    uint32_t s_be = htonl(seq);
    uint16_t ch_be = htons(1);
    uint16_t st_be = htons(0);
    std::memcpy(prefix.data(), &s_be, 4);
    std::memcpy(prefix.data() + 4, &ch_be, 2);
    std::memcpy(prefix.data() + 6, &st_be, 2);
    prefix[8] = 0;

    ByteBuffer data;
    data.insert(data.end(), prefix.begin(), prefix.end());
    data.insert(data.end(), msg.begin(), msg.end());

    ByteBuffer hdr = make_message_header(m_tag_remote, 0, 0, 1, data.size());
    ByteBuffer pkt;
    pkt.push_back(0);
    pkt.insert(pkt.end(), hdr.begin(), hdr.end());
    pkt.insert(pkt.end(), data.begin(), data.end());
    send_raw(pkt);
}

bool TakionConnection::do_handshake() {
    // 1. Initial parameters
    auto rnd_tag = crypto::SecureRandom::random_uint32();
    m_tag_local = rnd_tag.has_value() ? rnd_tag.value() : 0x48231982;
    m_seq_local = m_tag_local;
    m_tag_remote = 0;

    // 2. Send INIT (33 bytes)
    ByteBuffer init_pl(16);
    uint32_t tl = htonl(m_tag_local);
    uint32_t ar = htonl(m_a_rwnd);
    uint16_t ob = htons(100);
    uint16_t ib = htons(100);
    uint32_t sq = htonl(m_seq_local);
    std::memcpy(init_pl.data() + 0, &tl, 4);
    std::memcpy(init_pl.data() + 4, &ar, 4);
    std::memcpy(init_pl.data() + 8, &ob, 2);
    std::memcpy(init_pl.data() + 10, &ib, 2);
    std::memcpy(init_pl.data() + 12, &sq, 4);

    ByteBuffer init_hdr = make_message_header(0, 0, 1, 0, init_pl.size());
    ByteBuffer init_pkt;
    init_pkt.push_back(0);
    init_pkt.insert(init_pkt.end(), init_hdr.begin(), init_hdr.end());
    init_pkt.insert(init_pkt.end(), init_pl.begin(), init_pl.end());

    spdlog::info("Takion: Sending INIT ({} bytes)...", init_pkt.size());
    send_raw(init_pkt);

    // 3. Receive INIT_ACK
    ByteBuffer cookie(32);
    auto start_time = std::chrono::steady_clock::now();
    bool init_ack_received = false;

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 3000) {
        auto recv_res = m_socket->receive_from();
        if (recv_res.has_value()) {
            const auto& [buf, ip, port] = recv_res.value();
            if (buf.size() >= 49 && buf[0] == 0 && buf[13] == 2) { // InitAck
                uint32_t remote_tag_be = 0;
                std::memcpy(&remote_tag_be, buf.data() + 17, 4);
                m_tag_remote = ntohl(remote_tag_be);
                if (buf.size() >= 65) {
                    std::memcpy(cookie.data(), buf.data() + 33, 32);
                }
                spdlog::info("Takion: INIT_ACK received! Remote Tag: 0x{:X}", m_tag_remote);
                init_ack_received = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (!init_ack_received) {
        spdlog::error("Takion: Timeout waiting for INIT_ACK from PS5");
        return false;
    }

    // 4. Send COOKIE (49 bytes)
    ByteBuffer cookie_hdr = make_message_header(m_tag_remote, 0, 0x0a, 0, cookie.size());
    ByteBuffer cookie_pkt;
    cookie_pkt.push_back(0);
    cookie_pkt.insert(cookie_pkt.end(), cookie_hdr.begin(), cookie_hdr.end());
    cookie_pkt.insert(cookie_pkt.end(), cookie.begin(), cookie.end());

    spdlog::info("Takion: Sending COOKIE ({} bytes)...", cookie_pkt.size());
    send_raw(cookie_pkt);

    // 5. Receive COOKIE_ACK
    start_time = std::chrono::steady_clock::now();
    bool cookie_ack_received = false;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 3000) {
        auto recv_res = m_socket->receive_from();
        if (recv_res.has_value()) {
            const auto& [buf, ip, port] = recv_res.value();
            if (buf.size() >= 17 && buf[0] == 0 && buf[13] == 0x0b) { // CookieAck
                spdlog::info("Takion: COOKIE_ACK received!");
                cookie_ack_received = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (!cookie_ack_received) {
        spdlog::error("Takion: Timeout waiting for COOKIE_ACK");
        return false;
    }

    // 6. Generate secp256k1 EC Key & Sign
    EC_KEY* local_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!local_key || !EC_KEY_generate_key(local_key)) {
        spdlog::error("Takion: Failed to generate secp256k1 EC_KEY");
        if (local_key) EC_KEY_free(local_key);
        return false;
    }

    const EC_POINT* local_pub_point = EC_KEY_get0_public_key(local_key);
    const EC_GROUP* group = EC_KEY_get0_group(local_key);
    uint8_t local_pub[65];
    size_t pub_len = EC_POINT_point2oct(group, local_pub_point, POINT_CONVERSION_UNCOMPRESSED, local_pub, sizeof(local_pub), nullptr);
    if (pub_len != 65) {
        spdlog::error("Takion: Failed to export secp256k1 public key (len {})", pub_len);
        EC_KEY_free(local_key);
        return false;
    }

    auto rnd_hk = crypto::SecureRandom::random_bytes(16);
    if (rnd_hk.has_value()) {
        std::memcpy(m_handshake_key.data(), rnd_hk->data(), 16);
    } else {
        std::fill(m_handshake_key.begin(), m_handshake_key.end(), 0x33);
    }

    uint8_t local_sig[32];
    unsigned int sig_len = 32;
    HMAC(EVP_sha256(), m_handshake_key.data(), 16, local_pub, 65, local_sig, &sig_len);

    std::string hk_b64 = b64_encode(m_handshake_key);
    std::string launch_spec_str = std::format(
        "{{\"sessionId\":\"{}\",\"streamResolutions\":[{{\"resolution\":{{\"width\":{},\"height\":{}}},"
        "\"maxFps\":{},\"score\":10}}],\"network\":{{\"bwKbpsSent\":{},\"bwLoss\":0.001000,\"mtu\":1454,\"rtt\":10,"
        "\"ports\":[53,2053]}},\"slotId\":1,\"appSpecification\":{{\"minFps\":30,\"minBandwidth\":0,\"extTitleId\":\"ps3\","
        "\"version\":1,\"timeLimit\":1,\"startTimeout\":100,\"afkTimeout\":100,\"afkTimeoutDisconnect\":100}},"
        "\"konan\":{{\"ps3AccessToken\":\"accessToken\",\"ps3RefreshToken\":\"refreshToken\"}},"
        "\"requestGameSpecification\":{{\"model\":\"bravia_tv\",\"platform\":\"android\",\"audioChannels\":\"5.1\",\"language\":\"sp\","
        "\"acceptButton\":\"X\",\"connectedControllers\":[\"xinput\",\"ds3\",\"ds4\"],\"yuvCoefficient\":\"bt601\","
        "\"videoEncoderProfile\":\"hw4.1\",\"audioEncoderProfile\":\"audio1\",\"adaptiveStreamMode\":\"resize\"}},"
        "\"userProfile\":{{\"onlineId\":\"psnId\",\"npId\":\"npId\",\"region\":\"US\",\"languagesUsed\":[\"en\",\"jp\"]}},"
        "\"videoCodec\":\"{}\",\"dynamicRange\":\"SDR\",\"handshakeKey\":\"{}\"}}",
        m_config.session_id.empty() ? "sessionId4321" : m_config.session_id,
        m_config.width, m_config.height,
        m_config.fps,
        m_config.bitrate_kbps,
        m_config.codec == VideoCodec::H264 ? "avc" : "hevc",
        hk_b64
    );

    ByteBuffer launch_spec_raw(launch_spec_str.begin(), launch_spec_str.end());
    launch_spec_raw.push_back(0); // Null terminator

    auto iv0 = crypto::PS5Protocol::generate_iv(m_config.ambassador_key, 0);
    auto enc_spec_res = crypto::PS5Protocol::aes_cfb_crypt(m_config.bright_key, iv0, launch_spec_raw, true);
    if (!enc_spec_res.has_value()) {
        spdlog::error("Takion: Failed to encrypt launch spec");
        EC_KEY_free(local_key);
        return false;
    }
    std::string launch_spec_b64 = b64_encode(enc_spec_res.value());

    // Build BigPayload
    ByteBuffer big_pl;
    encode_field_varint(big_pl, 1, 12);
    encode_field_string(big_pl, 2, m_config.session_id.empty() ? "sessionId4321" : m_config.session_id);
    encode_field_string(big_pl, 3, launch_spec_b64);
    std::array<uint8_t, 16> zero_key{};
    encode_field_bytes(big_pl, 4, zero_key);
    encode_field_bytes(big_pl, 5, std::span<const uint8_t>(local_pub, 65));
    encode_field_bytes(big_pl, 6, std::span<const uint8_t>(local_sig, 32));

    // Wrap in TakionMessage
    ByteBuffer takion_msg;
    encode_field_varint(takion_msg, 1, 0); // type = 0 (BIG)
    encode_field_bytes(takion_msg, 2, big_pl);

    uint32_t my_seq = m_seq_local++;
    ByteBuffer prefix(9);
    uint32_t my_seq_be = htonl(my_seq);
    std::memcpy(prefix.data(), &my_seq_be, 4);
    uint16_t ch_be = htons(1);
    std::memcpy(prefix.data() + 4, &ch_be, 2);
    uint16_t st_be = htons(0);
    std::memcpy(prefix.data() + 6, &st_be, 2);
    prefix[8] = 0;

    ByteBuffer data_chunk;
    data_chunk.insert(data_chunk.end(), prefix.begin(), prefix.end());
    data_chunk.insert(data_chunk.end(), takion_msg.begin(), takion_msg.end());

    ByteBuffer data_hdr = make_message_header(m_tag_remote, 0, 0, 1, data_chunk.size());
    ByteBuffer data_pkt;
    data_pkt.push_back(0);
    data_pkt.insert(data_pkt.end(), data_hdr.begin(), data_hdr.end());
    data_pkt.insert(data_pkt.end(), data_chunk.begin(), data_chunk.end());

    spdlog::info("Takion: Sending BIG protobuf ({} bytes)...", data_pkt.size());
    send_raw(data_pkt);

    // 7. Receive BANG Protobuf
    start_time = std::chrono::steady_clock::now();
    bool bang_received = false;

    spdlog::info("Takion: Waiting for BANG from PS5 (timeout: 6s)...");
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 6000) {
        auto recv_res = m_socket->receive_from();
        if (recv_res.has_value()) {
            const auto& [buf, ip, port] = recv_res.value();
            if (buf.size() >= 14 && buf[0] == 0) {
                uint8_t chunk_type = buf[13];
                spdlog::info("Takion: UDP packet received (size: {} bytes, chunk_type: {})", buf.size(), chunk_type);
                if (chunk_type == 3) {
                    spdlog::info("Takion: Received DATA_ACK for BIG from PS5");
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    continue;
                } else if (chunk_type == 0 && buf.size() > 17) { // DATA chunk
                    std::span<const uint8_t> msg_pl(buf.data() + 17, buf.size() - 17);
                    if (msg_pl.size() > 9) {
                        uint32_t remote_seq_be = 0;
                        std::memcpy(&remote_seq_be, msg_pl.data(), 4);
                        uint32_t remote_seq = ntohl(remote_seq_be);

                        std::span<const uint8_t> proto_data = msg_pl.subspan(9);
                        auto root_fields = parse_protobuf(proto_data);
                        spdlog::info("Takion: DATA chunk seq={}, proto_len={}, root_fields={}", remote_seq, proto_data.size(), root_fields.size());

                        for (const auto& rf : root_fields) {
                            if (rf.field_number == 1) {
                                spdlog::info("Takion: Message payload type = {}", rf.varint_val);
                            }
                            if (rf.field_number == 3 && !rf.bytes_val.empty()) { // bang_payload
                                auto bang_fields = parse_protobuf(rf.bytes_val);
                                spdlog::info("Takion: BANG payload fields count = {}", bang_fields.size());
                                std::span<const uint8_t> remote_pub_bytes{};
                                for (const auto& bf : bang_fields) {
                                    if (bf.field_number == 8) { // ecdh_pub_key
                                        remote_pub_bytes = bf.bytes_val;
                                        break;
                                    }
                                }

                                if (remote_pub_bytes.size() == 65) {
                                    spdlog::info("Takion: BANG received! Deriving secp256k1 shared secret...");
                                    EC_POINT* remote_point = EC_POINT_new(group);
                                    if (remote_point && EC_POINT_oct2point(group, remote_point, remote_pub_bytes.data(), 65, nullptr)) {
                                        uint8_t shared_secret[32];
                                        int secret_len = ECDH_compute_key(shared_secret, 32, remote_point, local_key, nullptr);
                                        EC_POINT_free(remote_point);

                                        if (secret_len == 32) {
                                             // Derive Remote GKCrypt keys (index 3)
                                             uint8_t d3[3 + 16 + 2] = {1, 3, 0};
                                             std::memcpy(d3 + 3, m_handshake_key.data(), 16);
                                             d3[19] = 1; d3[20] = 0;
                                             uint8_t h3[32];
                                             unsigned int h3_l = 32;
                                             HMAC(EVP_sha256(), shared_secret, 32, d3, sizeof(d3), h3, &h3_l);
                                             std::memcpy(m_key_remote.data(), h3, 16);
                                             std::memcpy(m_iv_remote.data(), h3 + 16, 16);

                                             // Derive Local GKCrypt keys (index 2)
                                             uint8_t d2[3 + 16 + 2] = {1, 2, 0};
                                             std::memcpy(d2 + 3, m_handshake_key.data(), 16);
                                             d2[19] = 1; d2[20] = 0;
                                             uint8_t h2[32];
                                             HMAC(EVP_sha256(), shared_secret, 32, d2, sizeof(d2), h2, &h3_l);
                                             std::memcpy(m_key_local.data(), h2, 16);
                                             std::memcpy(m_iv_local.data(), h2 + 16, 16);

                                             m_crypt_initialized = true;
                                             spdlog::info("Takion: GKCrypt initialized! Remote key/iv and Local key/iv derived.");

                                             // Send DATA_ACK for BANG
                                             send_data_ack(remote_seq);
                                             spdlog::info("Takion: Sent DATA_ACK for BANG (seq {})", remote_seq);

                                             // Send StreamInfoAck
                                             send_streaminfo_ack();
                                             spdlog::info("Takion: Sent StreamInfoAck");

                                             // Send ControllerConnection
                                             send_controller_connection();
                                             spdlog::info("Takion: Sent ControllerConnection");

                                             bang_received = true;
                                             break;
                                        } else {
                                            spdlog::error("Takion: ECDH_compute_key returned {} (expected 32)", secret_len);
                                        }
                                    } else {
                                        if (remote_point) EC_POINT_free(remote_point);
                                        spdlog::error("Takion: EC_POINT_oct2point failed for remote pub key");
                                    }
                                } else {
                                    spdlog::error("Takion: Remote pub key size mismatch: {} (expected 65)", remote_pub_bytes.size());
                                }
                            }
                        }
                    }
                }
            }
        }
        if (bang_received) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    EC_KEY_free(local_key);

    if (!bang_received) {
        spdlog::error("Takion: Timeout waiting for BANG from PS5");
        return false;
    }

    return true;
}

void TakionConnection::handle_packet(std::span<const uint8_t> data) {
    if (data.empty()) return;
    uint8_t pkt_type = data[0] & 0x0f;
    if (pkt_type == 2) {
        handle_video_packet(data);
    } else if (pkt_type == 0) {
        handle_control_packet(data);
    }
}

void TakionConnection::handle_video_packet(std::span<const uint8_t> data) {
    if (data.size() < 24 || !m_crypt_initialized) return;

    // Header parsing (v12: 24 bytes)
    uint16_t packet_index = (static_cast<uint16_t>(data[1]) << 8) | data[2];
    uint16_t frame_index = (static_cast<uint16_t>(data[3]) << 8) | data[4];
    uint32_t dword_2 = (static_cast<uint32_t>(data[5]) << 24) |
                       (static_cast<uint32_t>(data[6]) << 16) |
                       (static_cast<uint32_t>(data[7]) << 8)  |
                       data[8];

    uint16_t unit_index = (dword_2 >> 0x15) & 0x7FF;
    uint16_t units_total = ((dword_2 >> 0xA) & 0x7FF) + 1;
    uint16_t units_fec = dword_2 & 0x3FF;
    uint16_t units_source = (units_total > units_fec) ? (units_total - units_fec) : units_total;
    uint8_t codec = data[9]; // 1 = H.264, 2 = H.265

    uint32_t key_pos = (static_cast<uint32_t>(data[14]) << 24) |
                       (static_cast<uint32_t>(data[15]) << 16) |
                       (static_cast<uint32_t>(data[16]) << 8)  |
                       data[17];

    const uint8_t* payload_data = data.data() + 24;
    size_t payload_size = data.size() - 24;

    // GKCrypt AES-128-CTR decryption
    uint64_t full_key_pos = static_cast<uint64_t>(key_pos) + 16;
    uint64_t padding_pre = full_key_pos % 16;
    size_t full_size = ((padding_pre + payload_size + 15) / 16) * 16;

    if (m_keystream_buffer.size() < full_size) {
        m_keystream_buffer.resize(full_size);
    }

    uint64_t counter_offset = (full_key_pos - padding_pre) / 16;
    for (size_t offset = 0; offset < full_size; offset += 16) {
        counter_add(m_keystream_buffer.data() + offset, m_iv_remote.data(), counter_offset++);
    }

    if (!m_aes_ecb_ctx) {
        m_aes_ecb_ctx = EVP_CIPHER_CTX_new();
    }
    auto* ctx = reinterpret_cast<EVP_CIPHER_CTX*>(m_aes_ecb_ctx);
    int outl = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, m_key_remote.data(), nullptr);
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    EVP_EncryptUpdate(ctx, m_keystream_buffer.data(), &outl, m_keystream_buffer.data(), static_cast<int>(full_size));

    ByteBuffer decrypted(payload_size);
    const uint8_t* ks = m_keystream_buffer.data() + padding_pre;
    for (size_t i = 0; i < payload_size; ++i) {
        decrypted[i] = payload_data[i] ^ ks[i];
    }

    m_frames_received++;

    if (unit_index < units_source) {
        TakionAVPacket av_pkt{};
        av_pkt.stream_type = 0; // Video
        av_pkt.seq = packet_index;
        av_pkt.unit_index = unit_index;
        av_pkt.units_in_frame = units_source;
        av_pkt.frame_index = frame_index;
        av_pkt.is_fec = false;
        av_pkt.data = std::move(decrypted);
        av_pkt.pts = frame_index;
        av_pkt.codec = codec;

        auto assembled = m_frame_assembler.add_packet(av_pkt);
        if (assembled.has_value()) {
            if (m_video_decoder) {
                auto dec_res = m_video_decoder->decode(assembled->data);
                if (dec_res.has_value()) {
                    m_frames_decoded++;
                    if (m_on_video_frame) {
                        m_on_video_frame(dec_res.value());
                    }
                }
            }
        }
    }
}

void TakionConnection::handle_control_packet(std::span<const uint8_t> data) {
    if (data.size() < 17) return;
    uint8_t chunk_type = data[13];
    if (chunk_type == 0) { // DATA chunk
        std::span<const uint8_t> msg_pl = data.subspan(17);
        if (msg_pl.size() > 9) {
            std::span<const uint8_t> proto_bytes = msg_pl.subspan(9);
            auto fields = parse_protobuf(proto_bytes);
            for (const auto& f : fields) {
                if (f.field_number == 1 && f.varint_val == 13) { // STREAMINFO
                    for (const auto& f2 : fields) {
                        if (f2.field_number == 15) { // stream_info_payload
                            auto si_fields = parse_protobuf(f2.bytes_val);
                            for (const auto& sif : si_fields) {
                                if (sif.field_number == 1) { // resolution
                                    auto res_fields = parse_protobuf(sif.bytes_val);
                                    for (const auto& rf : res_fields) {
                                        if (rf.field_number == 3 && !rf.bytes_val.empty()) { // video_header
                                            spdlog::info("Takion: Received PS5 video header VPS/SPS/PPS ({} bytes)", rf.bytes_val.size());
                                            if (m_video_decoder) {
                                                (void)m_video_decoder->decode(rf.bytes_val);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TakionConnection::heartbeat_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (m_state.load() == TakionState::Streaming) {
            ByteBuffer msg;
            encode_field_varint(msg, 1, 3); // HEARTBEAT

            uint32_t seq = m_seq_local++;
            ByteBuffer prefix(9);
            uint32_t s_be = htonl(seq);
            uint16_t ch_be = htons(1);
            uint16_t st_be = htons(0);
            std::memcpy(prefix.data(), &s_be, 4);
            std::memcpy(prefix.data() + 4, &ch_be, 2);
            std::memcpy(prefix.data() + 6, &st_be, 2);
            prefix[8] = 0;

            ByteBuffer data;
            data.insert(data.end(), prefix.begin(), prefix.end());
            data.insert(data.end(), msg.begin(), msg.end());

            ByteBuffer hdr = make_message_header(m_tag_remote, 0, 0, 1, data.size());
            ByteBuffer pkt;
            pkt.push_back(0);
            pkt.insert(pkt.end(), hdr.begin(), hdr.end());
            pkt.insert(pkt.end(), data.begin(), data.end());
            send_raw(pkt);
        }
    }
}

void TakionConnection::receive_loop(std::stop_token stoken) {
    spdlog::info("Takion receive loop started on port {}", m_config.remote_port);
    while (!stoken.stop_requested()) {
        auto res = m_socket->receive_from();
        if (res.has_value()) {
            const auto& [buffer, sender_ip, sender_port] = res.value();
            if (!buffer.empty()) {
                handle_packet(std::span<const uint8_t>(buffer.data(), buffer.size()));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
    spdlog::info("Takion receive loop stopped");
}

void TakionConnection::send_controller_state(const ControllerState& state) {
    if (m_state.load() != TakionState::Streaming) return;

    // Send FEEDBACK_STATE (type 6, 40 bytes)
    uint8_t buf[12 + 28];
    buf[0] = 6;
    uint16_t seq = static_cast<uint16_t>(m_seq_local++);
    uint16_t seq_be = htons(seq);
    std::memcpy(buf + 1, &seq_be, 2);
    buf[3] = 0;
    std::memset(buf + 4, 0, 8);

    uint8_t* p = buf + 12;
    p[0] = 0xa0;

    constexpr float GYRO_MIN = -30.0f;
    constexpr float GYRO_MAX = 30.0f;
    constexpr float ACCEL_MIN = -5.0f;
    constexpr float ACCEL_MAX = 5.0f;

    uint16_t gx = static_cast<uint16_t>(0xffff * (state.gyro.x - GYRO_MIN) / (GYRO_MAX - GYRO_MIN));
    uint16_t gy = static_cast<uint16_t>(0xffff * (state.gyro.y - GYRO_MIN) / (GYRO_MAX - GYRO_MIN));
    uint16_t gz = static_cast<uint16_t>(0xffff * (state.gyro.z - GYRO_MIN) / (GYRO_MAX - GYRO_MIN));
    p[1] = gx & 0xff; p[2] = (gx >> 8) & 0xff;
    p[3] = gy & 0xff; p[4] = (gy >> 8) & 0xff;
    p[5] = gz & 0xff; p[6] = (gz >> 8) & 0xff;

    uint16_t ax = static_cast<uint16_t>(0xffff * (state.accel.x - ACCEL_MIN) / (ACCEL_MAX - ACCEL_MIN));
    uint16_t ay = static_cast<uint16_t>(0xffff * (state.accel.y - ACCEL_MIN) / (ACCEL_MAX - ACCEL_MIN));
    uint16_t az = static_cast<uint16_t>(0xffff * (state.accel.z - ACCEL_MIN) / (ACCEL_MAX - ACCEL_MIN));
    p[7] = ax & 0xff; p[8] = (ax >> 8) & 0xff;
    p[9] = ay & 0xff; p[10] = (ay >> 8) & 0xff;
    p[11] = az & 0xff; p[12] = (az >> 8) & 0xff;

    p[13] = 0; p[14] = 0; p[15] = 0; p[16] = 0;

    int16_t lx = state.left_stick_x;
    int16_t ly = state.left_stick_y;
    int16_t rx = state.right_stick_x;
    int16_t ry = state.right_stick_y;

    uint16_t lx_be = htons(static_cast<uint16_t>(lx));
    uint16_t ly_be = htons(static_cast<uint16_t>(ly));
    uint16_t rx_be = htons(static_cast<uint16_t>(rx));
    uint16_t ry_be = htons(static_cast<uint16_t>(ry));
    std::memcpy(p + 17, &lx_be, 2);
    std::memcpy(p + 19, &ly_be, 2);
    std::memcpy(p + 21, &rx_be, 2);
    std::memcpy(p + 23, &ry_be, 2);

    p[25] = 0; p[26] = 0; p[27] = 1;

    send_raw(std::span<const uint8_t>(buf, sizeof(buf)));

    // Send FEEDBACK_HISTORY (type 1)
    ByteBuffer hist_pl;
    hist_pl.push_back(0x80); hist_pl.push_back(0x88); hist_pl.push_back((state.buttons & 0x01) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x89); hist_pl.push_back((state.buttons & 0x02) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x8A); hist_pl.push_back((state.buttons & 0x04) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x8B); hist_pl.push_back((state.buttons & 0x08) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x84); hist_pl.push_back((state.buttons & 0x10) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x85); hist_pl.push_back((state.buttons & 0x20) ? 0xFF : 0x00);
    hist_pl.push_back(0x80); hist_pl.push_back(0x86); hist_pl.push_back(state.l2_trigger);
    hist_pl.push_back(0x80); hist_pl.push_back(0x87); hist_pl.push_back(state.r2_trigger);
    hist_pl.push_back(0x80); hist_pl.push_back((state.buttons & 0x100) ? 0xAF : 0x8F);
    hist_pl.push_back(0x80); hist_pl.push_back((state.buttons & 0x200) ? 0xB0 : 0x90);
    hist_pl.push_back(0x80); hist_pl.push_back((state.buttons & 0x400) ? 0xAC : 0x8C);
    hist_pl.push_back(0x80); hist_pl.push_back((state.buttons & 0x800) ? 0xAD : 0x8D);
    hist_pl.push_back(0x80); hist_pl.push_back((state.buttons & 0x1000) ? 0xAE : 0x8E);

    ByteBuffer hist_pkt(12 + hist_pl.size());
    hist_pkt[0] = 1;
    uint16_t h_seq_be = htons(static_cast<uint16_t>(m_seq_local++));
    std::memcpy(hist_pkt.data() + 1, &h_seq_be, 2);
    hist_pkt[3] = 0;
    std::memset(hist_pkt.data() + 4, 0, 8);
    std::memcpy(hist_pkt.data() + 12, hist_pl.data(), hist_pl.size());
    send_raw(hist_pkt);
}

void TakionConnection::set_on_video_frame(VideoFrameCallback cb) {
    m_on_video_frame = std::move(cb);
}

void TakionConnection::set_on_audio_frame(AudioFrameCallback cb) {
    m_on_audio_frame = std::move(cb);
}

void TakionConnection::set_on_av_packet(AVPacketCallback cb) {
    m_on_av_packet = std::move(cb);
}

void TakionConnection::set_on_control_packet(ControlPacketCallback cb) {
    m_on_control_packet = std::move(cb);
}

void TakionConnection::set_cloud_mode(bool cloud_mode) {
    m_cloud_mode = cloud_mode;
}

TakionState TakionConnection::get_state() const {
    return m_state.load();
}

bool TakionConnection::is_streaming() const {
    return m_state.load() == TakionState::Streaming;
}

} // namespace portal::stream
