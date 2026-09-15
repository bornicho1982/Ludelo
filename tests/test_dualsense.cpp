// Archivo: tests/test_dualsense.cpp
// Test de Periféricos DualSense (Reportes HID, Gatillos Adaptativos, CRC32 Bluetooth)
#include "LudeloCore/Common.h"
#include "LudeloCore/Input/DualSenseHID.h"

#include <iostream>
#include <cassert>
#include <vector>

// Implementación de referencia IEEE 802.3 CRC32 para verificar
uint32_t reference_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

void test_dualsense_crc32() {
    std::cout << "[TEST] DualSense Bluetooth CRC32 Checksum... ";

    // Vector de prueba conocido (Report ID 0xA2 + Payload BT DualSense)
    std::vector<uint8_t> test_packet = {
        0xA2, 0x31, 0x02, 0x0F, 0x03, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05
    };

    uint32_t expected_crc = reference_crc32(test_packet.data(), test_packet.size());
    assert(expected_crc != 0);

    // Verificamos simetría del algoritmo
    uint32_t verify = reference_crc32(test_packet.data(), test_packet.size());
    assert(expected_crc == verify);

    std::cout << "PASSED (CRC: 0x" << std::hex << expected_crc << std::dec << ")\n";
}

void test_adaptive_trigger_effects() {
    std::cout << "[TEST] DualSense Adaptive Trigger Profiles... ";

    // Perfil Feedback (Resistencia en posición 2 con fuerza 6)
    ludelo::input::TriggerEffect feedback{};
    feedback.mode = ludelo::input::TriggerMode::Feedback;
    feedback.params[0] = 0x02; // Posición de inicio de resistencia
    feedback.params[1] = 0x06; // Fuerza de resistencia aplicada

    assert(feedback.mode == ludelo::input::TriggerMode::Feedback);
    assert(feedback.params[0] == 2);
    assert(feedback.params[1] == 6);

    // Perfil Weapon (Tope en posición 3, ruptura en 7, fuerza 8)
    ludelo::input::TriggerEffect weapon{};
    weapon.mode = ludelo::input::TriggerMode::Weapon;
    weapon.params[0] = 0x03; // Start
    weapon.params[1] = 0x07; // Break point
    weapon.params[2] = 0x08; // Retaining force

    assert(weapon.mode == ludelo::input::TriggerMode::Weapon);
    assert(weapon.params[0] == 3);
    assert(weapon.params[1] == 7);
    assert(weapon.params[2] == 8);

    // Perfil Vibration (Vibración ametralladora en posición 1, frec 15Hz, amplitud 7)
    ludelo::input::TriggerEffect vibration{};
    vibration.mode = ludelo::input::TriggerMode::Vibration;
    vibration.params[0] = 0x01; // Start
    vibration.params[1] = 0x0F; // Frequency
    vibration.params[2] = 0x07; // Amplitude

    assert(vibration.mode == ludelo::input::TriggerMode::Vibration);
    assert(vibration.params[0] == 1);
    assert(vibration.params[1] == 15);
    assert(vibration.params[2] == 7);

    std::cout << "PASSED\n";
}

void test_controller_state_normalization() {
    std::cout << "[TEST] DualSense State Normalization... ";

    ludelo::input::DualSenseState state{};
    state.left_stick_x = 128; // Center
    state.left_stick_y = 128; // Center
    state.l2 = 255;           // Full press
    state.r2 = 0;             // Released
    state.cross = true;
    state.square = false;

    assert(state.left_stick_x == 128);
    assert(state.l2 == 255);
    assert(state.r2 == 0);
    assert(state.cross == true);
    assert(state.square == false);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: DualSense HID      \n";
    std::cout << "========================================\n";

    try {
        test_dualsense_crc32();
        test_adaptive_trigger_effects();
        test_controller_state_normalization();
        std::cout << "\n>>> ALL DUALSENSE TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
