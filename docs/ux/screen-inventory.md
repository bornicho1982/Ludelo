# Inventario de Pantallas — Ludelo UI (Fase 3)

| ID | Pantalla | Estado Maqueta | Archivo Maqueta | Implementación QML |
|---|---|---|---|---|
| 01 | Onboarding | ✅ Aprobada | `docs/ux/mockups/01_onboarding.png` | ✅ Implementada |
| 02 | Home / Consolas | ✅ Aprobada | `docs/ux/mockups/02_home.png` | ✅ Implementada |
| 03 | Sesión / HUD | ✅ Aprobada | `docs/ux/mockups/03_hud.png` | ✅ Implementada |
| 04 | Ajustes | ✅ Aprobada | `docs/ux/mockups/04_settings.png` | ✅ Implementada |
| 05 | Cuenta PSN | ✅ Aprobada | `docs/ux/mockups/05_account.png` | ✅ Implementada |
| 06 | Cloud Play | ✅ Aprobada (con correcciones) | `docs/ux/mockups/06_cloudplay.png` (privada, no commitear) | ✅ Implementada |
| 07 | RegistDialog / PIN | ✅ Aprobada (con correcciones) | `docs/ux/mockups/07_regist_pin.png` | ✅ Implementada |
| 08 | Diálogos menores + Toasts | ⏳ Pendiente | `docs/ux/mockups/08_dialogs_toasts.png` | Pendiente |

---

## 01. Onboarding
- **Prompt Stitch**: `"Design a premium gamer desktop app onboarding for 'Ludelo', a PlayStation remote play client. Dark #0B0E14, glass panel, indigo accent #6C5CE7 with glow, mint #00F5D4 for success. Center card: logo, welcome title, subtitle about secure PSN sign-in, large glowing 'Sign in with PlayStation Network' button, ghost link 'Skip for now'. Steam Big Picture meets Razer Synapse. 1920x1080, gamepad-friendly large targets, no PlayStation trademarks."`
- **Fondo**: `#0B0E14` con viñetas radiales indigo sutiles.
- **Tarjeta Central**: Vidrio translúcido `rgba(21, 25, 35, 0.85)` con borde 1px y glow `#6C5CE7`.
- **Acciones**:
  - Primaria: Botón `LButton` Indigo con glow y badge `[X] ENTER` -> dispara `Chiaki.startWebView2Login()`.
  - Secundaria: Enlace ghost "Skip for now (Local Network Scan)" -> navega a Home/Consolas sin cuenta vinculada.

---

## 06. Cloud Play (PlayStation Cloud Gaming Portal)
- **Prompt Stitch**: `"Design 'Ludelo - Screen 06: PlayStation Cloud Gaming Portal (Cloud Play)'. Aesthetic: Cyber-minimalist luxury, blending Steam Big Picture with GeForce NOW and Razer Synapse precision. Deep obsidian dark canvas #0B0E14, frosted glass card panels #151923 with crisp 1px borders, electric indigo #6C5CE7 ambient glows, and vibrant mint #00F5D4 accents for playable cloud indicators and status pills. Desktop 1920x1080 layout. Header & Sub-Bar: Brandmark 'LUDELO' with pill badge 'CLOUD STREAMING v2.4'. Search bar. Telemetry pills: Datacenter & PS Plus Premium cloud notice. Filter bar: [ALL GAMES], [STREAMABLE NOW], [OWNED], [FAVORITES], Sort dropdown. Hero Cloud Spotlight: Wide cinematic banner for featured title with '[A] LAUNCH CLOUD STREAM'. Cloud Games Catalog Grid: Multi-column grid of cloud game cover cards with tags and '[A] PLAY' action. Bottom Controller Footer: Universal controller buttons [A], [B], [X], [Y], [START]."`
- **Maqueta Generada**: `docs/ux/mockups/06_cloudplay.png` (mantenida en local/privado, ignorada en git por contener arte con copyright comercial).
- **Estado**: ✅ Aprobada con correcciones de honestidad e Implementada (`gui/src/qml/CloudPlayView.qml`, `gui/src/qml/CloudGameCard.qml`).
- **Pautas y Correcciones Aplicadas**:
  - Cero claims ficticios: eliminados "TIER 1 LOW-LATENCY EDGE", "Instant Edge Launch", nodos falsos ("Frankfurt Node 01"), "Adaptive Bitrate 45 Mbps" por juego y badges "1080p 60FPS HDR10" ficticios.
  - Telemetría real en Top Bar y Hero: Región resuelta de catálogo (`es-ES`), validez de caché de catálogo, y total de títulos reales de `Chiaki.cloudCatalog`.
  - Manejo honesto de falta de suscripción PS Plus Premium: títulos no reproducibles muestran badge "REQUIRES PS PLUS" y botón `[A] PS PLUS` que abre el navegador externo a la web oficial de PlayStation (`https://www.playstation.com/ps-plus`), sin cobrar ni engañar dentro de la app.
  - Carátulas dinámicas cargadas únicamente desde la cuenta y catálogo PSN del usuario autenticado vía `getImageUrl()`. Prohibido empaquetar arte comercial en el repo.
  - Cero glifos PlayStation: controles universales y nomenclatura Xbox (`[A]`, `[B]`, `[X]`, `[Y]`, `[START]`).

---

## 07. RegistDialog / PIN (Vinculación y Registro de Consola por PIN)
- **Prompt Stitch**: `"Design 'Ludelo - Screen 07: Console Registration & 8-Digit PIN Pairing Modal Dialog'. Aesthetic: Cyber-minimalist gaming luxury, matching Steam Big Picture and Razer Synapse dark UI. Dark obsidian background #0B0E14 with soft ambient radial glow in electric indigo #6C5CE7. Center Modal Card: Frosted glass panel #151923, 1px subtle border, rounded corners. Form content: 1. Instructions banner with console navigation path. 2. Target console selector [PlayStation 5] (PS5 • HDR) and [PlayStation 4]. 3. Console IP address with [ONLINE / DETECTED] badge. 4. Segmented 8-box digit input with 300s countdown timer. 5. PSN Account ID section with DPAPI verification badge and [RE-AUTHENTICATE] button. 6. Optional 4-digit console user passcode. 7. Actions [A] REGISTER & PAIR CONSOLE and [B] CANCEL."`
- **Maqueta Generada**: `docs/ux/mockups/07_regist_pin.png`
- **Estado**: ✅ Aprobada con correcciones e Implementada (`gui/src/qml/RegistDialog.qml`).
- **Pautas y Correcciones Aplicadas**:
  - PIN de vinculación de 8 dígitos estrictamente numéricos (0-9) repartidos en 8 casillas con avance de foco y gestión fluida de backspace.
  - Temporizador de 300 segundos activo (expiración real de PIN de Remote Play de Sony).
  - Passcode opcional de usuario de consola de 4 dígitos numéricos para perfiles con código de bloqueo de inicio de sesión.
  - Selector de consola con etiqueta honesta "PS5 • HDR" (sin claim falso de "4K", ya que Remote Play es hasta 1080p).
  - Eliminado claim publicitario "Ultra Low Latency" del footer.
  - Cero glifos PlayStation: solo controles universales (`[A]`, `[B]`, `[ESC]`).



