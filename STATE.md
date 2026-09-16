# STATE - Ludelo

## Estado Actual
- **Fase**: Fase 2 (Instrumentación y Diagnóstico de Login Embebido).
- **Toolchain**: MinGW64 / MSYS2 + CMake (Target GUI `Ludelo.exe`).
- **Tests**: `chiaki-unit` con 4 tests unitarios de `auth_classifier` (100% passed en CTest).
- **Logging**: `spdlog` file sink en `%APPDATA%\Ludelo\logs\ludelo.log` (INFO global, DEBUG en `qmlbackend`, `auth_classifier`, `psnaccountid`).
- **Consola Debug**: `run-debug.bat` disponible para ver logs y stderr en vivo.
- **Diagnóstico clave identificado**:
  1. `mingw-w64-x86_64-qt6-webview` en MSYS2 fue compilado sin plugins de backend (`QT_FEATURE_webview_webview2_plugin = -1`, `plugins/webview` no existe en MSYS2).
  2. Al pulsar "Añadir cuenta", el flujo de UI de Pylux llama a `QRLoginDialog` -> `PSNTokenDialog` (flujo manual legacy con 3 pasos / navegador externo) en lugar de `PSNLoginDialog`.
  3. Añadido banner visual de prueba `"WebView cargando..."` en `PSNLoginDialog` para comprobar renderizado.

