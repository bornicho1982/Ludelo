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

- **Pantalla 05: Cuenta PSN & Biblioteca**:
  - Maqueta generada con Stitch MCP y aprobada con correcciones (`docs/ux/mockups/05_account.png`).
  - Creado `gui/src/qml/AccountView.qml` integrado en `Main.qml`, `MainView.qml` (clic en píldora PSN del top bar) y `SettingsDialog.qml` (botón "VIEW FULL ACCOUNT & GAMES"):
    - Hero Card de perfil con avatar circular y halo de estado, ID de cuenta ofuscado (`xxxx••••yyyy`), región de tienda (`ES`), estado de seguridad `CONNECTED TO PSN • ENCRYPTED LOCAL STORAGE (DPAPI)`.
    - Eliminados badges y claims inventados (removido badge falso "PlayStation Plus Premium", sin días de expiración ficticios ni firmware inventado).
    - Acciones de autenticación: `[Y] RE-AUTHENTICATE` (dispara WebView2 nativo), `SYNC GAMES` y `[X] SIGN OUT` (purga credenciales y tokens cifrados de DPAPI).
    - Grid de juegos instalados de consola (`Chiaki.getPsnInstalledGames()` y `ChiakiGames.getGameImage`) con filtros por plataforma (PS5 / PS4).
    - Eliminados badges técnicos no reportados por juego ("60 FPS", "DualSense Haptics", "Fidelity Mode", "PS4 Pro Enhanced"); solo datos reales (carátula real, nombre, plataforma PS5/PS4, Title ID).
    - Botón de cada juego renombrado honestamente a `CONECTAR` (inicia la sesión remota con la consola vinculada, sin prometer lanzamiento directo remoto).
    - Tarjeta colapsable de fallback técnico `ADVANCED AUTHENTICATION & MANUAL NPSSO TOKEN` con campo protegido para entrada manual de token de 64 caracteres.
    - Footer de atajos universales de gamepad (`[A] SELECT GAME`, `[B] BACK`, `[Y] RE-AUTHENTICATE`, `[X] SIGN OUT`).
    - Registrada en `docs/ux/screen-inventory.md` la advertencia legal para capturas de marketing de tienda (usar carátulas genéricas/libres en la tienda pública).
  - Compilación exitosa con CMake + MinGW64 (`Ludelo.exe`, `chiaki-unit`). Tests unitarios 100% pasados en CTest.

- **Pantalla 06: Cloud Play**:
  - Maqueta generada con Stitch MCP y aprobada con correcciones de honestidad y copyright comercial.
  - Maqueta `docs/ux/mockups/06_cloudplay.png` preservada en local e ignorada en `.gitignore` por contener arte comercial con copyright. Prohibido empaquetar carátulas de Sony en el repo.
  - Implementado `gui/src/qml/CloudPlayView.qml` y `gui/src/qml/CloudGameCard.qml` con `LudeloTheme` y suite `L*`:
    - Eliminados claims ficticios ("TIER 1 LOW-LATENCY EDGE", "Instant Edge Launch", nodos falsos "Frankfurt Node 01", "Adaptive Bitrate 45 Mbps" por juego y badges "1080p 60FPS HDR10" inventados).
    - Header superior con breadcrumb, telemetría real (región detectada `es-ES`, total de títulos en catálogo, estado de caché), buscador reactivo neón con atajo `[Y]` y botón de refresco `[F5]`.
    - Fila de filtros con chips interactivos (`[ALL GAMES]`, `[STREAMABLE]`, `[OWNED]`, `[FAVORITES]`) y selector de ordenación cíclico (`[SORT: PLAYABLE FIRST]`, `A → Z`, `Z → A`).
    - Banners honestos de advertencia técnica: aviso claro si falta token NPSSO con acceso directo a ajustes, y aviso de región de fallback si cloud no está soportado nativamente en el país de la cuenta.
    - Hero Spotlight cinemático: destaca el juego seleccionado con imagen atenuada, badges honestos (región resuelta, plataforma, validez de caché), botón primario `[A] LAUNCH CLOUD STREAM` si es jugable o `[A] REQUIRES PS PLUS` si requiere suscripción/compra externa, y alternancia de favoritos `[X] FAVORITE`.
    - Manejo ético y legal de suscripción: los títulos no streamables muestran badge "REQUIRES PS PLUS" y abren la web oficial `https://www.playstation.com/ps-plus` o enlace del store en el navegador externo con `Qt.openUrlExternally` — sin engañar ni cobrar dentro de la app.
    - Grid adaptativo `GridView` con foco por teclado y mando, navegación por flechas y scroll suave.
    - Footer HUD inferior con atajos universales (`[A] PLAY / STORE`, `[B] BACK`, `[X] FAVORITE`, `[Y] SEARCH`, `[START] SORT`) sin glifos geométricos de PlayStation.
  - Compilación exitosa con CMake + MinGW64 (`Ludelo.exe`, `chiaki-unit`). Tests unitarios 100% pasados en CTest.

- **Pantalla 07: RegistDialog / PIN (Vinculación y Registro de Consola)**:
  - Maqueta generada con Stitch MCP y aprobada con correcciones (`docs/ux/mockups/07_regist_pin.png`).
  - Implementado `gui/src/qml/RegistDialog.qml` sobre `LudeloTheme` y suite de componentes `L*`:
    - Banner de instrucciones con rutas exactas en menús de PS5 y PS4 para generar el código de vinculación.
    - Selector de arquitectura de consola objetivo (`PlayStation 5` con badge honesto "PS5 • HDR" sin claim falso de 4K, y `PlayStation 4` con "LEGACY DDP").
    - Campo de dirección IP local de la consola con badge `[ONLINE / DETECTED]` y verificación en tiempo real.
    - Entrada de PIN de 8 dígitos segmentada en 8 casillas rectangulares (`[ 5 ] [ 8 ] [ 2 ] [ 1 ] — [ 9 ] [ 4 ] [ 0 ] [ 3 ]`) con foco activo neón cyan `#00F5D4`, soporte para pegado de 8 dígitos, navegación fluida por flechas y borrado por backspace.
    - Temporizador activo de cuenta atrás de 300 segundos (expiración real de PIN de Remote Play de Sony).
    - Sección de Account ID de PSN mostrando el ID ofuscado de la sesión cifrada local (DPAPI) con botón `[RE-AUTHENTICATE]`.
    - Campo de passcode opcional de usuario de consola de 4 dígitos numéricos para perfiles con código de bloqueo.
    - Botón primario de registro `[A] REGISTER & PAIR CONSOLE` conectado a `Chiaki.registerHost` con terminal de log integrado en tiempo real y auto-cierre tras éxito.
    - Footer de atajos universales de gamepad (`[A] REGISTER`, `[B] CANCEL`, `[ESC] BACK`) sin glifos geométricos de PlayStation y sin claims publicitarios ("Ultra Low Latency").
  - Compilación exitosa con CMake + MinGW64 (`Ludelo.exe`, `chiaki-unit`). Tests unitarios 100% pasados en CTest.

- **Pantalla 08: Diálogos Menores + Toasts / Overlays**:
  - Maqueta generada con Stitch MCP (`docs/ux/mockups/08_dialogs_toasts.png`) y aprobada con correcciones.
  - Implementación completa de la suite de modales, alertas y notificaciones sobre `LudeloTheme` y componentes `L*`:
    - `ConfirmDialog.qml`: Modal de confirmación de acción/destructivo con tarjeta de vidrio `#151923`, botones `LButton` (`[A] CONFIRM`, `[B] CANCEL`, `[ESC]`) eliminando iconos propietarios.
    - `MessageDialog.qml`: Modal de información general alineado con `LudeloTheme` y botón `LButton` `[A] OK`.
    - `RemindDialog.qml`: Diálogo de prompt con botones `[A] YES`, `[B] NO` y checkbox interactivo con estilo gaming para recordar decisión.
    - `Main.qml` (`errorToast`): Transformado en stack de notificaciones flotantes premium gamer con 3 estados de acento (`success` mint, `info` indigo, `error/warning` coral) y barra de progreso animada.
    - `ManualHostDialog.qml`: Rediseñado a modal cyber-minimalista sobre `LudeloTheme` con atajos de teclado y gamepad.
    - `ConsolePinDialog.qml`: Rediseñado sobre `LudeloTheme` para solicitud en vivo de passcode de usuario de 4 dígitos.
    - `AccountPrivacyDialog.qml` y `AutoConnectView.qml`: Purgados glifos PlayStation (`△`, `cross`, `Circle`), unificados a nomenclatura Xbox/universal (`[A]`, `[B]`, `[Y]`).
  - **Pasada Global de Consistencia en las 8 Pantallas**:
    - Purgados todos los claims publicitarios o técnicos no verificables ("Ultra Low Latency", "Vortex Engine", "Lossless F2P", "10 GbE P2P Hardware Verified", "Sony Magic Negotiation Stalled").
    - Purgados todos los glifos geométricos de PlayStation (`✕`, `○`, `□`, `△`) de los botones y hints de control en toda la UI, reemplazados por nomenclatura universal/Xbox (`[A]`, `[B]`, `[X]`, `[Y]`, `[LB]`, `[RB]`, `[ESC]`, `[START]`).
    - Eliminada marca hardcodeada "DUALSENSE WIRELESS", sustituida por "Wireless Gamepad" / genérica (`AccountView.qml`, `MainView.qml`, `StreamView.qml`, `LTopBar.qml`, `ControllerMappingDialog.qml`).
    - Eliminado claim ficticio de "4K HDR 60FPS" de `LTopBar.qml`, sustituido por `DIRECT P2P STREAM READY`.
  - Compilación exitosa con CMake + MinGW64 (`Ludelo.exe`, `chiaki-unit`). Tests unitarios 100% pasados en CTest.

- **FASE 3 (Diseño Premium Gamer, QML Nativo & Stitch MCP): REVISIÓN Y RETOQUES COMPLETADOS (18/09/2026)**.
  - Implementación completa de todos los puntos P0 (Bloqueantes) y P1 (Importantes) del Informe de Retoques:
    - **P0.1**: Panel derecho de telemetría de Ajustes restringido exclusivamente a la pestaña Network (`dialog.activeCategoryIndex === 2`). En estado sin sesión, muestra una única tarjeta de estado vacío sobria.
    - **P0.2**: Corregidos solapes y cortes en Ajustes (`LCard.qml` y `LToggle.qml` con altura dinámica calculada y ajuste de texto; cálculo exacto de altura de scroll).
    - **P0.3**: Persistencia y auto-sanación de consolas vinculadas ante corrupción `size=0` en `Ludelo.conf` (`Settings::LoadRegisteredHosts`, `Settings::SaveRegisteredHosts`). `QmlBackend::hosts()` muestra consolas vinculadas registradas en reposo/standby. Añadido test unitario de regresión en `test/registered_hosts_test.cpp`.
    - **P0.4**: Ofuscación unificada y homogénea de Account ID (`XXXX••••YYYY`, sin padding ni sufijos `=`) en `LudeloTheme.formatObfuscatedAccountId()`, aplicado en `MainView`, `SettingsDialog` y `AccountView`.
    - **P0.5**: Corregida telemetría de decodificador (latencia real en ms separada de motor HW `D3D11VA`), erradicado `NaN Mbps` en HUD de MainView, tipografía mínima garantizada >= 11px.
    - **P1.1 / P1.2**: Un solo botón `RESET DEFAULTS` funcional en footer de Ajustes; footer depurado para atajos globales.
    - **P1.3**: Contadores de Cloud Play unificados y precisos: `Catálogo: X • Mostrando: Y`.
    - **P1.4**: Banner de autenticación Cloud Play con acción `[RE-AUTHENTICATE]`; Hero spotlight prioriza títulos jugables/instalados sobre conceptos de tienda; botón contextual de favoritos `FAVORITE`/`UNFAVORITE`.
    - **P1.5**: Eliminado selector ficticio "Cloud Data Center Location: Auto" de Ajustes, sustituido por tarjeta informativa de arquitectura de transporte (Kamaji / WebRTC).
    - **P1.6**: Base de internacionalización preparada con `qsTr()` y literales centralizados (traducción completa diferida a Fase 5).
    - **P1.7**: Tarjetas de consola en Home con estados protegidos (sin campos en blanco); chips de filtro concisos `All (X)`, `PS5 (X)`, `PS4 (X)`; botón renombrado a `STREAM PREFERENCES`.
  - Validación 100% exitosa de tests unitarios (`ctest`) y verificación completa de componentes QML y smoke test con `scripts/deploy-windows.ps1`.







