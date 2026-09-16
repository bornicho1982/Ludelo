# STATE - Ludelo

## Estado Actual
- **Fase**: Fase 2 (Login Embebido PSN vía Win32 WebView2 nativo).
- **Toolchain**: MinGW64 / MSYS2 + CMake (Target GUI `Ludelo.exe`).
- **Tests**: `chiaki-unit` con tests unitarios de `auth_classifier` (100% passed en CTest).
- **Logging**: `spdlog` file sink en `%APPDATA%\Ludelo\logs\ludelo.log` (INFO global, DEBUG en `qmlbackend`, `auth_classifier`, `psnaccountid`).
  - Higiene de logs activa: tokens y codes enmascarados, `error_code` en claro.
- **Consola Debug**: `run-debug.bat` disponible para ver logs y stderr en vivo.
- **Implementación**:
  1. Portado `WebView2Auth` nativo Win32 desde `ludelo-legacy` a `gui/src/auth/webview2_login_win32.{h,cpp}` con soporte MinGW-w64 (templates `CallbackImpl`/`CallbackTraits` COM).
  2. Almacenamiento de sesión webview2 en `%APPDATA%\Ludelo\webview2`.
  3. Carga dinámica de `WebView2Loader.dll` y fallback a instalador bootstrapper silencioso oficial de Microsoft si el runtime no estuviese instalado.
  4. Intercepción en `NavigationStarting` para capturar `?code=` y cerrar la ventana automáticamente.
  5. Interceptación en `NavigationCompleted` con JavaScript de introspección para detectar errores en el DOM.
  6. Backend QML `startWebView2Login()` ejecuta en worker thread `QThread` y despacha el intercambio OAuth `PSNAccountID::GetPsnAccountId` en el hilo principal de Qt.
  7. Conexiones QML en `QRLoginDialog.qml` ("Login on This Device") y `PSNTokenDialog.qml` actualizadas para disparar WebView2 directamente sin intervención de navegadores externos ni copiar/pegar URLs.

