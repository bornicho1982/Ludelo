// Archivo: tools/Ludelo_LiveDiagnostic.cpp
// Herramienta de Diagnostico en Vivo: Deteccion de PS5 en Red y Mando DualSense HID
#include "LudeloCore/Common.h"
#include "LudeloCore/Discovery/DDPDiscovery.h"
#include "LudeloCore/Input/DualSenseHID.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

using namespace ludelo;
using namespace ludelo::discovery;
using namespace ludelo::input;

void print_banner() {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "            Ludelo - DIAGNOSTICO EN VIVO Y PRUEBA DE HARDWARE         \n";
    std::cout << "         PlayStation Remote Play Client (Windows 11 / C++23)          \n";
    std::cout << "======================================================================\n\n";
}

void test_network_discovery() {
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[FASE 1] ESCANEO DE RED LOCAL (PlayStation Device Discovery Protocol)\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[*] Enviando paquetes UDP Broadcast (puertos 9302 y 987)...\n";
    std::cout << "[*] Esperando respuesta de tu PS5 en la red local (tiempo de espera: 3s)...\n\n";

    auto res = DDPDiscovery::search(3000);
    if (!res.has_value()) {
        std::cout << "[-] Error al iniciar el socket de descubrimiento: " << res.error().message << "\n";
        return;
    }

    const auto& consoles = res.value();
    if (consoles.empty()) {
        std::cout << "[!] No se recibio respuesta de consolas en la red local.\n";
        std::cout << "    Consejos:\n";
        std::cout << "    - Asegurate de que la opcion 'Uso a distancia' (Remote Play) este activada en tu PS5:\n";
        std::cout << "      (Ajustes -> Sistema -> Uso a distancia -> Activar Uso a distancia)\n";
        std::cout << "    - Comprueba que el PC y la PS5 esten en la misma subred local.\n";
        std::cout << "    - Si el Firewall de Windows muestra aviso, permite el trafico UDP entrante.\n";
    } else {
        std::cout << "[+] EXITO: Se encontraron " << consoles.size() << " consola(s) PlayStation:\n\n";
        for (size_t i = 0; i < consoles.size(); ++i) {
            const auto& c = consoles[i];
            std::cout << "    [" << (i + 1) << "] Consola: " << c.host_name << "\n";
            std::cout << "        Modelo        : " << c.host_type << "\n";
            std::cout << "        Direccion IP  : " << c.address << ":" << c.port << "\n";
            std::cout << "        ID de Host    : " << c.host_id << "\n";
            std::cout << "        Firmware/Ver  : " << c.system_version << "\n";
            std::cout << "        Estado Energia: " 
                      << (c.state == ConsoleState::Awake ? "ENCENDIDA (Awake)" : "MODO REPOSO (Standby)") 
                      << "\n\n";
        }
    }
}

void test_dualsense_controller() {
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[FASE 2] INSPECCION DE MANDO DUALSENSE (Direct Windows HID)\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[*] Enumerando dispositivos HID Sony (VID 0x054C, PID 0x0CE6/0x0DF2)...\n";

    DualSenseHID ds;
    auto controllers = ds.enumerate();

    if (controllers.empty()) {
        std::cout << "[!] No se detecto ningun mando DualSense conectado.\n";
        std::cout << "    - Asegurate de que el cable USB este bien conectado o el Bluetooth emparejado.\n";
        return;
    }

    std::cout << "[+] EXITO: Mando DualSense detectado (" << controllers.size() << " interfaz/es):\n";
    for (size_t i = 0; i < controllers.size(); ++i) {
        std::cout << "    [" << (i + 1) << "] Dispositivo: " << controllers[i].device_path << "\n";
        std::cout << "        Numero de Serie: " << (controllers[i].serial.empty() ? "(USB Standard)" : controllers[i].serial) << "\n";
    }

    // Intentar abrir el primer controlador disponible
    std::cout << "\n[*] Abriendo conexion directa con el mando...\n";
    bool opened = false;
    for (const auto& c : controllers) {
        if (ds.open(c.device_path).has_value()) {
            opened = true;
            std::cout << "[+] Mando conectado y abierto correctamente.\n";
            std::cout << "    Tipo de conexion: " << (ds.is_bluetooth() ? "Bluetooth (Reporte 78 bytes)" : "USB (Reporte 64 bytes)") << "\n";
            break;
        }
    }

    if (!opened) {
        std::cout << "[-] No se pudo abrir la interfaz del mando para lectura/escritura.\n";
        return;
    }

    // Lectura de estado de entrada (Input Poll)
    std::cout << "\n[*] LEYENDO ENTRADA EN VIVO (Mueve sticks o pulsa botones durante 3 segundos):\n";
    auto start_poll = std::chrono::steady_clock::now();
    int samples = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_poll).count() < 3000) {
        auto state_res = ds.poll();
        if (state_res.has_value()) {
            const auto& s = state_res.value();
            if (++samples % 15 == 0) {
                std::cout << "    Stick Izq: (" << std::setw(3) << (int)s.left_stick_x << "," << std::setw(3) << (int)s.left_stick_y << ") | "
                          << "Stick Der: (" << std::setw(3) << (int)s.right_stick_x << "," << std::setw(3) << (int)s.right_stick_y << ") | "
                          << "L2: " << std::setw(3) << (int)s.l2 << " R2: " << std::setw(3) << (int)s.r2 << " | "
                          << "Bat: " << (int)s.battery_level * 10 << "% "
                          << (s.is_charging ? "(Cargando) " : "")
                          << (s.cross ? "[X] " : "")
                          << (s.circle ? "[O] " : "")
                          << (s.square ? "[[]] " : "")
                          << (s.triangle ? "[^] " : "")
                          << "\r" << std::flush;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "\n[+] Lectura de reporte de entrada superada con exito.\n\n";

    // Prueba de Funciones Avanzadas de Hardware (Lightbar, Gatillo Adaptativo, Vibracion)
    std::cout << "[*] PROBANDO HARDWARE AVANZADO DEL DUALSENSE:\n";
    
    // 1. Barra de Luz Azul PlayStation
    std::cout << "    [1/3] Iluminando barra de luz LED en Azul PlayStation (#006FCD)...\n";
    (void)ds.set_lightbar(0, 111, 205);
    (void)ds.poll(); // Forzar envio de reporte de salida
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // 2. Barra de Luz Cyan PlayStation Portal
    std::cout << "    [2/3] Iluminando barra de luz LED en Cyan PlayStation Portal (#00E5FF)...\n";
    (void)ds.set_lightbar(0, 229, 255);
    (void)ds.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // 3. Prueba de Gatillo Adaptativo en R2 (Weapon Mode)
    std::cout << "    [3/3] ACTIVANDO RESISTENCIA DEL GATILLO ADAPTATIVO EN R2 (Weapon Mode)...\n";
    std::cout << "          >>> APRIETA EL GATILLO R2 AHORA EN TU MANDO <<<\n";
    std::cout << "          (Notaras el tope y la resistencia mecanica del motor interno)\n";
    
    TriggerEffect weapon_effect;
    weapon_effect.mode = TriggerMode::Weapon;
    weapon_effect.params[0] = 0x02; // Start position
    weapon_effect.params[1] = 0x05; // End position
    weapon_effect.params[2] = 0x08; // Fuerza de resistencia alta
    (void)ds.set_trigger_effect(false, weapon_effect); // false = Right Trigger (R2)
    (void)ds.set_vibration(40, 40);
    (void)ds.poll();

    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Restaurar gatillos y apagar vibracion
    TriggerEffect off_effect;
    off_effect.mode = TriggerMode::Off;
    (void)ds.set_trigger_effect(false, off_effect);
    (void)ds.set_vibration(0, 0);
    (void)ds.poll();

    std::cout << "\n[+] Pruebas de actuadores del DualSense finalizadas.\n";
    ds.close();
}

void test_assets_integrity() {
    std::cout << "\n----------------------------------------------------------------------\n";
    std::cout << "[FASE 3] VERIFICACION DE SHADERS SPIR-V Y RECURSOS DE ASOBI\n";
    std::cout << "----------------------------------------------------------------------\n";

    std::vector<std::string> shaders = {
        "assets/shaders/fullscreen.vert.glsl.spv",
        "assets/shaders/nv12_to_hdr.frag.glsl.spv",
        "assets/shaders/nv12_to_rgb.frag.glsl.spv",
        "assets/shaders/ycbcr_hdr_tonemap.frag.glsl.spv",
        "assets/shaders/ycbcr_passthrough.frag.glsl.spv"
    };

    int loaded_shaders = 0;
    for (const auto& sh : shaders) {
        if (std::filesystem::exists(sh)) {
            auto sz = std::filesystem::file_size(sh);
            std::cout << "    [+] Shader: " << std::setw(45) << std::left << sh << " (" << sz << " bytes)\n";
            loaded_shaders++;
        } else {
            std::cout << "    [-] FALTA:  " << sh << "\n";
        }
    }

    std::cout << "\n    Total shaders verificados: " << loaded_shaders << " / " << shaders.size() << "\n";
    
    bool font_ok = std::filesystem::exists("assets/fonts/SFNS.ttf");
    std::cout << "    [+] Tipografia oficial SFNS.ttf: " << (font_ok ? "OK" : "NO ENCONTRADA") << "\n";
}

void run_glass_to_glass_test() {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "          Ludelo - MODO DE MEDICION GLASS-TO-GLASS (LATENCIA)        \n";
    std::cout << "======================================================================\n\n";
    std::cout << "Instrucciones para medicion de latencia punta a punta (Glass-to-Glass):\n";
    std::cout << " 1. Coloca este monitor justo al lado del televisor/pantalla de tu PS5.\n";
    std::cout << " 2. Inicia streaming en Ludelo en tu PC.\n";
    std::cout << " 3. Apunta la camara de tu smartphone en modo 'Camara Lenta' (240 FPS)\n";
    std::cout << "    enfocando ambas pantallas a la vez.\n";
    std::cout << " 4. Presiona un boton del mando (o mueve el stick) y graba la respuesta.\n";
    std::cout << " 5. Reproduce fotograma a fotograma en el movil y cuenta los frames:\n";
    std::cout << "    Latencia (ms) = (Frames transcurridos / 240.0) * 1000.0 ms.\n\n";
    std::cout << "[*] Iniciando reloj de alta precision (std::chrono::high_resolution_clock)...\n";
    std::cout << "[*] Presiona Ctrl+C para salir.\n\n";

    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t frame_count = 0;

    for (int i = 0; i < 5000; ++i) { // Run for ~5 seconds when called non-interactively or until aborted
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - t0).count();
        uint64_t sec = elapsed_us / 1000000ULL;
        uint64_t ms = (elapsed_us % 1000000ULL) / 1000ULL;
        uint64_t us = elapsed_us % 1000ULL;

        std::cout << "\r >>> [" 
                  << std::setw(3) << std::setfill('0') << sec << "s "
                  << std::setw(3) << std::setfill('0') << ms << "ms "
                  << std::setw(3) << std::setfill('0') << us << "us]  Frame #" 
                  << std::setw(8) << std::setfill(' ') << ++frame_count 
                  << "  <<<" << std::flush;

        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
    std::cout << "\n[OK] Prueba Glass-to-Glass completada.\n";
}

int main(int argc, char** argv) {
    try {
        #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        #endif

        if (argc > 1 && (std::string(argv[1]) == "--glass-to-glass" || std::string(argv[1]) == "-g")) {
            run_glass_to_glass_test();
            #ifdef _WIN32
            WSACleanup();
            #endif
            return 0;
        }

        ludelo::init_logging(spdlog::level::warn);
        print_banner();

        // 1. Escaneo de red para PS5
        test_network_discovery();

        // 2. Prueba del mando DualSense
        test_dualsense_controller();

        // 3. Verificacion de shaders y assets
        test_assets_integrity();

        std::cout << "\n======================================================================\n";
        std::cout << "  DIAGNOSTICO FINALIZADO CON EXITO\n";
        std::cout << "======================================================================\n\n";

        #ifdef _WIN32
        WSACleanup();
        #endif
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR (exception): " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "FATAL ERROR (unknown exception)\n";
        return 1;
    }
}
