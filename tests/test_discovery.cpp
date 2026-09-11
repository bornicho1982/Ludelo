// Archivo: tests/test_discovery.cpp
// Test del protocolo de descubrimiento PlayStation Device Discovery Protocol (DDP)
#include "PortalCore/Common.h"
#include "PortalCore/Discovery/DDPDiscovery.h"

#include <iostream>
#include <cassert>
#include <string>

void test_parse_ps5_awake_response() {
    std::cout << "[TEST] DDP Parse PS5 Awake Response... ";

    std::string mock_response =
        "HTTP/1.1 200 Ok\r\n"
        "host-id: 001122334455\r\n"
        "host-type: PS5\r\n"
        "host-name: PS5-LivingRoom\r\n"
        "host-request-port: 9303\r\n"
        "device-discovery-protocol-version: 00030010\r\n"
        "system-version: 08000000\r\n\r\n";

    auto console = portal::discovery::DDPDiscovery::parse_response(mock_response, "127.0.0.1", 9302);

    assert(console.state == portal::ConsoleState::Awake);
    assert(console.host_id == "001122334455");
    assert(console.host_type == "PS5");
    assert(console.host_name == "PS5-LivingRoom");
    assert(console.port == 9303);
    assert(console.address == "127.0.0.1");
    assert(console.system_version == "08000000");

    std::cout << "PASSED\n";
}

void test_parse_ps4_standby_response() {
    std::cout << "[TEST] DDP Parse PS4 Standby Response... ";

    std::string mock_response =
        "HTTP/1.1 620 Server Standby\r\n"
        "host-id: AABBCCDDEEFF\r\n"
        "host-type: PS4\r\n"
        "host-name: PS4-Bedroom\r\n"
        "host-request-port: 9295\r\n"
        "device-discovery-protocol-version: 00020020\r\n"
        "system-version: 11000000\r\n\r\n";

    auto console = portal::discovery::DDPDiscovery::parse_response(mock_response, "127.0.0.1", 987);

    assert(console.state == portal::ConsoleState::Standby);
    assert(console.host_id == "AABBCCDDEEFF");
    assert(console.host_type == "PS4");
    assert(console.host_name == "PS4-Bedroom");
    assert(console.port == 9295);
    assert(console.address == "127.0.0.1");

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: DDP Discovery      \n";
    std::cout << "========================================\n";

    try {
        test_parse_ps5_awake_response();
        test_parse_ps4_standby_response();
        std::cout << "\n>>> ALL DISCOVERY TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
