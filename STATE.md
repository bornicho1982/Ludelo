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



