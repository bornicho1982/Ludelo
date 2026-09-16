# Inventario de Pantallas — Ludelo UI (Fase 3)

| ID | Pantalla | Estado Maqueta | Archivo Maqueta | Implementación QML |
|---|---|---|---|---|
| 01 | Onboarding | ✅ Aprobada | `docs/ux/mockups/01_onboarding.png` | ✅ Implementada |
| 02 | Home / Consolas | ✅ Aprobada | `docs/ux/mockups/02_home.png` | ✅ Implementada |
| 03 | Sesión / HUD | ⏳ Pendiente | `docs/ux/mockups/03_hud.png` | Pendiente |
| 04 | Ajustes | ⏳ Pendiente | `docs/ux/mockups/04_settings.png` | Pendiente |
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
