# STATE - Ludelo

## Estado Actual
- **Fase**: Fase 3 (Diseño Premium Gamer & Sistema QML nativo).
- **Toolchain**: MinGW64 / MSYS2 + CMake (Target GUI `Ludelo.exe`).
- **Tests**: `chiaki-unit` (100% passed en CTest).
- **Logging**: `spdlog` file sink en `%APPDATA%\Ludelo\logs\ludelo.log` con redacción estricta de credenciales.
- **Identidad Git**: `bornicho1982 <149513609+bornicho1982@users.noreply.github.com>`.
- **Eliminación Total de QR Login**:
  - `QRLoginDialog.qml` respaldado en `docs/notes/QRLoginDialog.qml.bak` (ignorado por `.gitignore`).
  - Endpoints `/psstream/*` y métodos de generación/verificación QR eliminados del backend C++.
  - Flujo de login unificado: Win32 WebView2 nativo embebido directo. Fallback manual NPSSO (`PSNTokenDialog`) preservado para ajustes avanzados.
- **Sistema de Diseño Propio (`LudeloTheme` + Componentes `L*`)**:
  - Singleton `gui/src/qml/LudeloTheme.qml` registrado en `org.streetpea.chiaking` y módulo `Ludelo 1.0` con paleta gaming (`#0B0E14`, `#151923`, `#6C5CE7`, `#00F5D4`), radios, glows y curvas de animación.
  - Suite de componentes `L*` en `gui/src/qml/components/`: `LButton.qml`, `LCard.qml`, `LPill.qml`, `LNavTab.qml`, `LTopBar.qml`.
- **Pantalla 01: Onboarding**:
  - Maqueta generada con Stitch MCP y aprobada por el usuario (`docs/ux/mockups/01_onboarding.png`).
  - Implementado `gui/src/qml/OnboardingView.qml` con fidelidad exacta a la maqueta: tarjeta central translúcida, glow índigo, botón CTA primario, enlace ghost y badge honesto "Encrypted Local Storage (DPAPI)".
  - Integración en `Main.qml` como vista de bienvenida inicial para usuarios sin cuenta PSN y soporte de invocación de prueba con argumento `--onboarding`.
  - Corregido contenedor de `Connections` en `RegistDialog.qml` dentro de elemento `Item`.
- **Pantalla 02: Home / Consolas**:
  - Maqueta generada con Stitch MCP y aprobada por el usuario (`docs/ux/mockups/02_home.png`).
  - Implementado `gui/src/qml/MainView.qml` con diseño gamer premium sobre `LudeloTheme` y componentes `L*`:
    - Header superior con navegación de pestañas `[Consoles]`, `[Cloud Play]`, `[Settings]`, estado PSN en tiempo real con indicador beacon y telemetría de stream.
    - Subheader con estado de topología de red (`NETWORK TOPOLOGY DETECTED • DIRECT P2P READY`), botón de escaneo de subred y chips de filtro reales (`All`, `PlayStation 5`, `PlayStation 4`).
    - Grid adaptativo con tarjetas `LCard`:
      - Tarjetas de consola con solo datos reales verificables (estado online/standby, IP de conexión directa, tipo de hardware PS5/PS4, estado de emparejamiento, título activo en ejecución, botones de acción `CONNECT DIRECT` / `WAKE CONSOLE` / `PAIR CONSOLE`).
      - Tarjeta integrada "+ Add New Console / Manual IP" para vincular por PIN de 8 dígitos o IP manual.
      - Tarjeta integrada "Hardware Acceleration HUD" con telemetría de decodificador HW activo (D3D11VA/NVDEC), tasa de polling de mando DualSense/gamepad y bitrate dinámico real.
    - Footer HUD inferior con atajos de mando usando nomenclatura Xbox y texto legible (`[A] SELECT`, `[Y] WAKE`, `[X] DETAILS`, `[B] BACK`, `[START] SETTINGS`) sin glifos geométricos de PlayStation.
  - Verificado y compilado con CMake + MinGW64. Tests unitarios 100% pasados en CTest.
- **Pantalla 03: Sesión / HUD**:
  - Maqueta generada con Stitch MCP y aprobada con correcciones (`docs/ux/mockups/03_hud.png`).
  - Implementado en `gui/src/qml/StreamView.qml`, `gui/src/qmlmainwindow.cpp` y `gui/include/qmlmainwindow.h`:
    - Eliminado cualquier claim ficticio ("VORTEX"). Barra superior muestra estado real verificable: `DIRECT LAN • 1080p 60FPS • H.265/H.264` o `CLOUD STREAM`.
    - Telemetría de mando en vivo con tasa de polling real (1000Hz DualSense / 250Hz Gamepad / Keyboard Mode) y control de volumen integrado. Batería omitida honestamente al no reportarse por SDL.
    - Píldora de diagnóstico en tiempo real (Bitrate medido, RTT real, Loss % real, FPS y resolución) conmutada persistentemente con tecla `[TAB]`.
    - Dock flotante inferior auto-ocultable a los 1.5s sin interacción (`PS HOME`, `MUTE AUDIO`, `FULLSCREEN`, `HUD STATS`, `DISPLAY`, `DISCONNECT`).
    - Atajos de teclado en `QmlMainWindow`: `[TAB]` conmuta stats, `[F10]` conmuta dock HUD, `[F11]` conmuta pantalla completa.
    - Auto-ocultación del cursor del ratón a los 3 segundos de inactividad durante el streaming.
- **Pantalla 04: Ajustes (Settings)**:
  - Maqueta generada con Stitch MCP y aprobada con correcciones (`docs/ux/mockups/04_settings.png`).
  - Creados nuevos componentes reutilizables `LToggle.qml` y `LSlider.qml` en `gui/src/qml/components/` registrados en `qml.qrc`.
  - Rediseñado completamente `gui/src/qml/SettingsDialog.qml` sobre `LudeloTheme` y componentes `L*`:
    - Eliminados textos publicitarios o claims inventados ("Ultra Low Latency", "Lossless F2P", "AUTOTUNE STREAM", "Firmware 9.00").
    - Navegación superior por categorías (`Video & Stream`, `Audio`, `Network`, `Controller`, `Account`, `General`) con atajos de navegación por botones bumper `[LB]` y `[RB]` y teclas `PageUp`/`PageDown`.
    - Selector de perfil y consolas objetivo (PS5 / PS4) con configuración granular de resoluciones (1080p/720p/540p), tasa de refresco (60 FPS/30 FPS), códec (H.265/H.264) y decodificador por hardware (D3D11VA/NVDEC).
    - Toggle de HDR con validación real: solo activo cuando se selecciona PS5 con códec H.265 (HEVC), desactivado con nota "HDR disponible en streams H.265 compatibles" en H.264.
    - Slider de bitrate de 5 a 30 Mbps con marcas claras (`Eco`, `Standard`, `High Quality / Recommended for LAN`).
    - Panel lateral de telemetría y diagnóstico honesto: valores de RTT, Decoder Time, Jitter y Packet Loss leídos en vivo de `Chiaki.session` si hay sesión activa; en reposo sin conexión muestran `"--"` en gris tenue.
    - Botón y atajo `[X] RESET DEFAULTS` funcional conectado a restablecimiento directo de parámetros de fábrica en `Chiaki.settings` con feedback mediante toast.
    - Footer de atajos de gamepad universales (`[A] SELECT`, `[B] BACK / CLOSE`, `[X] RESET DEFAULTS`).
    - Tipografía exclusivamente `Inter` y `JetBrains Mono`.
  - Backup de versión anterior en `docs/notes/SettingsDialog.qml.bak`.
  - Compilación exitosa con CMake + MinGW64 (`Ludelo.exe`, `chiaki-unit`). Tests unitarios 100% pasados en CTest.

- **Siguiente**:
  - Pantalla 05: Cuenta PSN (`docs/ux/mockups/05_account.png` vía Stitch MCP).





