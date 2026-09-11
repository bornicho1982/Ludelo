// Archivo: tests/test_fec.cpp
// Test de Forward Error Correction (FEC XOR Recovery)
#include "PortalCore/Common.h"
#include "PortalCore/Stream/FECDecoder.h"

#include <iostream>
#include <cassert>
#include <vector>

void test_fec_recovery() {
    std::cout << "[TEST] FEC XOR Packet Recovery (K=3, M=1)... ";

    const size_t k = 3;
    const size_t m = 1;
    portal::stream::FECDecoder decoder(k, m);

    // 3 paquetes de datos de prueba (payloads de video simulados)
    std::vector<uint8_t> d0 = {0x10, 0x20, 0x30, 0x40, 0x50};
    std::vector<uint8_t> d1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    std::vector<uint8_t> d2 = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Paridad FEC calculada: P = D0 ^ D1 ^ D2
    std::vector<uint8_t> parity(d0.size());
    for (size_t i = 0; i < parity.size(); ++i) {
        parity[i] = d0[i] ^ d1[i] ^ d2[i];
    }

    // Simulamos que el paquete 1 (d1) se pierde en la red
    // Entregamos d0 (seq 0), d2 (seq 2) y parity (seq 0, is_fec=true)
    decoder.add_packet(0, d0, false);
    decoder.add_packet(2, d2, false);
    decoder.add_packet(0, parity, true);

    assert(!decoder.is_complete()); // Aún falta d1

    // Ejecutamos recuperación
    auto recovered = decoder.try_recover();
    assert(recovered.has_value());
    assert(decoder.is_complete());

    const auto& packets = recovered.value();
    assert(packets.size() == 3);
    assert(packets[0] == d0);
    assert(packets[1] == d1); // ¡El paquete perdido fue reconstruido bit a bit!
    assert(packets[2] == d2);

    std::cout << "PASSED\n";
}

void test_fec_already_complete() {
    std::cout << "[TEST] FEC Already Complete (No Loss)... ";

    portal::stream::FECDecoder decoder(2, 1);
    std::vector<uint8_t> d0 = {0x01, 0x02};
    std::vector<uint8_t> d1 = {0x03, 0x04};

    decoder.add_packet(0, d0, false);
    decoder.add_packet(1, d1, false);

    assert(decoder.is_complete());
    auto res = decoder.try_recover();
    assert(res.has_value());
    assert(res.value()[0] == d0);
    assert(res.value()[1] == d1);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Ludelo Unit Tests: FEC Recovery       \n";
    std::cout << "========================================\n";

    try {
        test_fec_recovery();
        test_fec_already_complete();
        std::cout << "\n>>> ALL FEC TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
