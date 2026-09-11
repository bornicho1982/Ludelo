# Ludelo

**Ludelo** es un cliente de PlayStation Remote Play de alto rendimiento para PC con Windows 11, diseñado para ofrecer una experiencia visual fluida, baja latencia y soporte completo de hardware para el mando DualSense.

> [!IMPORTANT]
> **Aviso Legal / Legal Disclaimer:**  
> **ES:** Ludelo es un proyecto de software independiente y de código abierto. No está afiliado, respaldado, asociado ni certificado de ninguna manera por Sony Interactive Entertainment Inc., PlayStation u otras empresas filiales. "PlayStation", "PS4", "PS5", "DualSense" y "PSN" son marcas registradas de sus respectivos propietarios.  
> **EN:** Ludelo is an independent open-source software project. It is not affiliated with, endorsed by, or in any way associated with Sony Interactive Entertainment Inc., PlayStation, or any of its subsidiaries. "PlayStation", "PS4", "PS5", "DualSense", and "PSN" are registered trademarks of their respective owners.

---

## Características / Features

- **Pipeline Gráfico Moderno (Vulkan 1.3):** Renderizado acelerado por hardware con shaders GLSL compilados a SPIR-V para conversión y presentación NV12 y YCbCr.
- **Soporte Nativo DualSense HID:** Acceso directo a actuadores hápticos, resistencia mecánica en gatillos adaptativos (Adaptive Triggers) y barra de luz LED RGB via USB y Bluetooth.
- **Descubrimiento Local (DDP):** Detección automática en red local de consolas PlayStation 4 y PlayStation 5 mediante PlayStation Device Discovery Protocol.
- **Seguridad en Reposo (Windows DPAPI):** Credenciales de consola (`rp_key`, `rp_auth`) y tokens de sesión cifrados localmente en Windows Credential Storage / DPAPI.
- **Baja Latencia y Resiliencia de Red:** Decodificación acelerada con FFmpeg, buffer dinámico y reconstrucción de paquetes mediante Forward Error Correction (FEC).

---

## Requisitos del Sistema / System Requirements

- **Sistema Operativo:** Windows 11 / Windows 10 (x64)
- **Compilador / SDK:** Visual Studio 2022 (MSVC v143+ con soporte C++23) y Windows 10/11 SDK
- **Dependencias:**
  - Vulkan SDK o Vulkan Runtime compatible con Vulkan 1.3
  - OpenSSL 3.0+ (Win64)
  - Bibliotecas vendored / incluidas en `third_party/` (SDL3, FFmpeg, ImGui, spdlog, nlohmann_json)

---

## Compilación / Building

Ludelo incluye un sistema de compilación maestro automatizado mediante `build.bat`:

### 1. Compilar la aplicación principal
```bat
build.bat app
```
El ejecutable binario se generará en `bin\Ludelo.exe`, junto con las dependencias DLL y los recursos gráficos necesarios.

### 2. Compilar y ejecutar la suite de pruebas unitarias
```bat
build.bat tests
```
Compila y valida los 6 componentes críticos:
- `test_fec`: Reconstrucción de paquetes con paridad FEC XOR
- `test_discovery`: Descubrimiento de consolas en red local (DDP)
- `test_dualsense`: Reportes de entrada HID y gatillos adaptativos
- `test_takion`: Estructuras y encabezados del protocolo de transporte Takion
- `test_auth`: Decodificación JWT y almacenamiento DPAPI
- `test_crypto`: Criptografía Diffie-Hellman y protocolo de cifrado PS5

### 3. Compilar la herramienta de diagnóstico de hardware
```bat
build.bat diag
```
Genera `bin\Ludelo_LiveDiagnostic.exe` para verificar de forma interactiva la conectividad de red con la consola, las luces LED, vibración y resistencia de gatillos en mandos DualSense.

### 4. Limpieza de artefactos
```bat
build.bat clean
```

---

## Uso / Usage

1. Asegúrate de tener activada la opción **Uso a distancia (Remote Play)** en los ajustes de tu consola PlayStation:
   - *Ajustes > Sistema > Uso a distancia > Activar Uso a distancia*
2. Ejecuta `bin\Ludelo.exe` o haz doble clic en `INICIAR_LUDELO.bat`.
3. Inicia sesión con tu cuenta de PSN o vincula tu consola directamente usando el PIN de 8 dígitos proporcionado en la pantalla de la consola.
4. Conecta tu mando DualSense mediante cable USB o Bluetooth para disfrutar de soporte nativo completo.

---

## Reconocimientos y Licencias / Credits & Licensing

- Código base del proyecto: **GNU Affero General Public License v3.0 or later (AGPL-3.0-or-later)**. Consulta [LICENSE](LICENSE).
- Incorpora arquitectura de protocolo y código derivado de los proyectos de código abierto **Chiaki** y **chiaki-ng**. Consulta [NOTICE](NOTICE).
- Bibliotecas de terceros: Consulta [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) para detalles completos de SDL3, FFmpeg, Dear ImGui, OpenSSL, Vulkan Headers, spdlog y nlohmann_json.
