// Archivo: tests/test_takion.cpp
// Test del protocolo de transporte propietario Takion (Sony PS Remote Play & Cloud)
#include "PortalCore/Common.h"
#include "PortalCore/Stream/TakionConnection.h"

#include <iostream>
#include <cassert>
#include <vector>

void test_takion_packet_header() {
    std::cout << "[TEST] Takion Binary Header Layout... ";

    // Estructura de cabecera Takion (11 bytes big-endian):
    // [0]    : Type (0=Init, 1=InitAck, 2=Cookie, 3=CookieAck, 4=Bang, 5=Data, 6=Control, 7=Heartbeat)
    // [1-2]  : Flags (uint16 big-endian)
    // [3-6]  : Sequence number (uint32 big-endian)
    // [7-10] : Timestamp (uint32 big-endian)
    // [11+]  : Payload

    uint8_t packet_type = 5; // Data
    uint16_t flags = 0x0001;
    uint32_t sequence = 0x12345678;
    uint32_t timestamp = 0x00ABCDEF;

    std::vector<uint8_t> packet(11 + 4);
    packet[0] = packet_type;
    packet[1] = (flags >> 8) & 0xFF;
    packet[2] = flags & 0xFF;

    packet[3] = (sequence >> 24) & 0xFF;
    packet[4] = (sequence >> 16) & 0xFF;
    packet[5] = (sequence >> 8) & 0xFF;
    packet[6] = sequence & 0xFF;

    packet[7] = (timestamp >> 24) & 0xFF;
    packet[8] = (timestamp >> 16) & 0xFF;
    packet[9] = (timestamp >> 8) & 0xFF;
    packet[10] = timestamp & 0xFF;

    // Payload de 4 bytes
    packet[11] = 0xDE; packet[12] = 0xAD; packet[13] = 0xBE; packet[14] = 0xEF;

    // Decodificar y verificar
    assert(packet[0] == 5);
    uint16_t dec_flags = (packet[1] << 8) | packet[2];
    assert(dec_flags == flags);

    uint32_t dec_seq = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | packet[6];
    assert(dec_seq == sequence);

    uint32_t dec_ts = (packet[7] << 24) | (packet[8] << 16) | (packet[9] << 8) | packet[10];
    assert(dec_ts == timestamp);

    assert(packet[11] == 0xDE && packet[12] == 0xAD && packet[13] == 0xBE && packet[14] == 0xEF);

    std::cout << "PASSED\n";
}

void test_controller_state_struct() {
    std::cout << "[TEST] Controller State Bitmask and Inputs... ";

    portal::stream::ControllerState ctrl{};
    ctrl.buttons = 0x01 | 0x10; // Cross + L1
    ctrl.left_stick_x = 128;
    ctrl.left_stick_y = 128;
    ctrl.l2_trigger = 200;
    ctrl.r2_trigger = 0;
    ctrl.touchpad[0] = {1, 50, 60, true};
    ctrl.touchpad[1] = {0, 0, 0, false};
    ctrl.sequence = 1001;

    assert((ctrl.buttons & 0x01) != 0);
    assert((ctrl.buttons & 0x02) == 0);
    assert(ctrl.touchpad[0].active == true);
    assert(ctrl.touchpad[1].active == false);
    assert(ctrl.sequence == 1001);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: Takion Protocol    \n";
    std::cout << "========================================\n";

    try {
        test_takion_packet_header();
        test_controller_state_struct();
        std::cout << "\n>>> ALL TAKION TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
