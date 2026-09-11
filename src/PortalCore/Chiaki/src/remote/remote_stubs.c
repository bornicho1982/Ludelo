// Archivo: src/PortalCore/Chiaki/src/remote/remote_stubs.c
#include <chiaki/remote/holepunch.h>
#include <chiaki/remote/rudp.h>
#include <string.h>

ChiakiHolepunchRegistInfo chiaki_get_regist_info(ChiakiHolepunchSession session) {
    (void)session;
    ChiakiHolepunchRegistInfo info;
    memset(&info, 0, sizeof(info));
    return info;
}

void chiaki_get_ps_selected_addr(ChiakiHolepunchSession session, char *ps_ip) {
    (void)session;
    if (ps_ip) ps_ip[0] = '\0';
}

uint16_t chiaki_get_ps_ctrl_port(ChiakiHolepunchSession session) {
    (void)session;
    return 9295;
}

chiaki_socket_t *chiaki_get_holepunch_sock(ChiakiHolepunchSession session, ChiakiHolepunchPortType type) {
    (void)session; (void)type;
    return NULL;
}

ChiakiErrorCode holepunch_session_create_offer(ChiakiHolepunchSession session) {
    (void)session;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_holepunch_session_punch_hole(ChiakiHolepunchSession session, ChiakiHolepunchPortType port_type) {
    (void)session; (void)port_type;
    return CHIAKI_ERR_UNINITIALIZED;
}

void chiaki_holepunch_session_fini(ChiakiHolepunchSession session) {
    (void)session;
}

ChiakiRudp chiaki_rudp_init(chiaki_socket_t *sock, ChiakiLog *log) {
    (void)sock; (void)log;
    return NULL;
}

void chiaki_rudp_reset_counter_header(ChiakiRudp rudp) {
    (void)rudp;
}

ChiakiErrorCode chiaki_rudp_send_init_message(ChiakiRudp rudp) {
    (void)rudp;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_cookie_message(ChiakiRudp rudp, uint8_t *response_buf, size_t response_size) {
    (void)rudp; (void)response_buf; (void)response_size;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_session_message(ChiakiRudp rudp, uint16_t remote_counter, uint8_t *session_msg, size_t session_msg_size) {
    (void)rudp; (void)remote_counter; (void)session_msg; (void)session_msg_size;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_ack_message(ChiakiRudp rudp, uint16_t remote_counter) {
    (void)rudp; (void)remote_counter;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_ctrl_message(ChiakiRudp rudp, uint8_t *ctrl_message, size_t ctrl_message_size) {
    (void)rudp; (void)ctrl_message; (void)ctrl_message_size;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_switch_to_stream_connection_message(ChiakiRudp rudp) {
    (void)rudp;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_raw(ChiakiRudp rudp, uint8_t *buf, size_t buf_size) {
    (void)rudp; (void)buf; (void)buf_size;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_send_recv(ChiakiRudp rudp, RudpMessage *message, uint8_t *buf, size_t buf_size,
    uint16_t remote_counter, RudpPacketType send_type, RudpPacketType recv_type, size_t min_data_size, size_t tries) {
    (void)rudp; (void)message; (void)buf; (void)buf_size; (void)remote_counter;
    (void)send_type; (void)recv_type; (void)min_data_size; (void)tries;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_select_recv(ChiakiRudp rudp, size_t buf_size, RudpMessage *message) {
    (void)rudp; (void)buf_size; (void)message;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_recv_only(ChiakiRudp rudp, size_t buf_size, RudpMessage *message) {
    (void)rudp; (void)buf_size; (void)message;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_stop_pipe_select_single(ChiakiRudp rudp, ChiakiStopPipe *stop_pipe, uint64_t timeout) {
    (void)rudp; (void)stop_pipe; (void)timeout;
    return CHIAKI_ERR_UNINITIALIZED;
}

ChiakiErrorCode chiaki_rudp_ack_packet(ChiakiRudp rudp, uint16_t counter_to_ack) {
    (void)rudp; (void)counter_to_ack;
    return CHIAKI_ERR_UNINITIALIZED;
}

void chiaki_rudp_print_message(ChiakiRudp rudp, RudpMessage *message) {
    (void)rudp; (void)message;
}

void chiaki_rudp_message_pointers_free(RudpMessage *message) {
    (void)message;
}

ChiakiErrorCode chiaki_rudp_fini(ChiakiRudp rudp) {
    (void)rudp;
    return CHIAKI_ERR_SUCCESS;
}
