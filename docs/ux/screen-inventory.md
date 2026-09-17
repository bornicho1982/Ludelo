# Inventario de Pantallas — Ludelo UI (Fase 3)

| ID | Pantalla | Estado Maqueta | Archivo Maqueta | Implementación QML |
|---|---|---|---|---|
| 01 | Onboarding | ✅ Aprobada | `docs/ux/mockups/01_onboarding.png` | ✅ Implementada |
| 02 | Home / Consolas | ✅ Aprobada | `docs/ux/mockups/02_home.png` | ✅ Implementada |
| 03 | Sesión / HUD | ✅ Aprobada | `docs/ux/mockups/03_hud.png` | ✅ Implementada |
| 04 | Ajustes | ✅ Aprobada | `docs/ux/mockups/04_settings.png` | ✅ Implementada |
| 05 | Cuenta PSN | ⏳ Pendiente | `docs/ux/mockups/05_account.png` | Pendiente |
| 06 | Cloud Play | ⏳ Pendiente | `docs/ux/mockups/06_cloudplay.png` | Pendiente |
| 07 | RegistDialog / PIN | ⏳ Pendiente | `docs/ux/mockups/07_regist_pin.png` | Pendiente |
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

## 04. Ajustes (Settings)
- **Prompt Stitch**: `"Design a premium gamer desktop settings screen for 'Ludelo', a PlayStation remote play client. Aesthetic: Cyber-minimalist luxury, Steam Big Picture meets Razer Synapse. Deep obsidian dark theme #0B0E14, frosted glass card panels #151923 with subtle 1px border and indigo #6C5CE7 glow, vibrant mint #00F5D4 accents for active toggles and badges. Layout: Fullscreen 1920x1080 desktop interface. Header: Title 'SETTINGS' in bold Sora font, with category tabs: 'Video', 'Audio', 'Network', 'Controller', 'Account', 'General'. Quick bumper hints '[LB]' and '[RB]' for tab switching. Active tab content (Video & Stream): Clean grouped setting cards with section titles: 'Display & Resolution': Resolution dropdown (1080p, 720p, 540p), Target Framerate toggle (60 FPS / 30 FPS), Video Codec selector (H.265 / H.264), Hardware Decoder selection (D3D11VA, DXVA2, Software). 'Bitrate & Network': Streaming Bitrate slider (range 5 - 30 Mbps, current 15 Mbps), Stream Diagnostics HUD switch (toggle ON with mint #00F5D4 indicator). 'Gamepad & Feedback': Controller Haptics toggle, Trigger Resistance slider, Polling Rate readout (1000Hz DualSense). Bottom action footer: Controller navigation hints '[A] SELECT' '[B] BACK / CLOSE' '[X] RESET DEFAULTS'. Strict rules: No PlayStation trademark symbols or geometric glyphs (no triangle, circle, cross, square). No fake marketing engine claims (no 'Vortex'). Only real, clean, technical gaming parameters."`
- **Maqueta Generada**: `docs/ux/mockups/04_settings.png`
- **Estado**: Generada en Stitch MCP (Screen `ef5d78282b2a4bc4901c35955e9ba2e4`), pendiente de revisión del usuario.

