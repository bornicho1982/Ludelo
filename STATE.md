# STATE - Ludelo

## Estado Actual
- **Fase**: Fase 2 (Login Embebido PSN vía Win32 WebView2 nativo).
- **Toolchain**: MinGW64 / MSYS2 + CMake (Target GUI `Ludelo.exe`).
- **Tests**: `chiaki-unit` con tests unitarios de `auth_classifier` (100% passed en CTest).
- **Logging**: `spdlog` file sink en `%APPDATA%\Ludelo\logs\ludelo.log` (INFO global, DEBUG en `qmlbackend`, `auth_classifier`, `psnaccountid`).
- **Seguridad de Logs**:
  - Redacción estricta en logs de red (`JsonRequester`, `psnaccountid`, `qmlbackend`, `qmlgamesbackend`).
  - Prohibido y suprimido volcado de "Response Body" y "Request Body".
  - Ofuscación automática en URLs (query params y paths `/2.0/oauth/token/<token>`) y JSON: `access_token`, `refresh_token`, `code`, `userid`/`user_id`, `useruuid`/`user_uuid` (primeros 4 caracteres + `****`).
  - Cabecera Authorization Basic pública preservada; cuerpos con tokens eliminados de los logs.
  - Tests de seguridad (`test_log_security_simulated_login`, `test_log_redaction_*`) en `chiaki-unit` con escaneo de fugas (100% pasando).
- **Consola Debug**: `run-debug.bat` disponible para ver logs y stderr en vivo.
- **Implementación**:
  1. Portado `WebView2Auth` nativo Win32 desde `ludelo-legacy` a `gui/src/auth/webview2_login_win32.{h,cpp}` con soporte MinGW-w64 (templates `CallbackImpl`/`CallbackTraits` COM).
  2. Almacenamiento de sesión webview2 en `%APPDATA%\Ludelo\webview2`.
  3. Carga dinámica de `WebView2Loader.dll` y fallback a instalador bootstrapper silencioso oficial de Microsoft si el runtime no estuviese instalado.
  4. Intercepción en `NavigationStarting` para capturar `?code=` y cerrar la ventana automáticamente.
  5. Interceptación en `NavigationCompleted` con JavaScript de introspección para detectar errores en el DOM.
  6. Backend QML `startWebView2Login()` ejecuta en worker thread `QThread` y despacha el intercambio OAuth `PSNAccountID::GetPsnAccountId` en el hilo principal de Qt.
  7. Conexiones QML en `QRLoginDialog.qml` ("Login on This Device") y `PSNTokenDialog.qml` actualizadas para disparar WebView2 directamente sin intervención de navegadores externos ni copiar/pegar URLs.
- **Diagnóstico y Recuperación Holepunch (HTTP 403)**:
  1. **Instrumentación de error body**: Se deshabilitó `CURLOPT_FAILONERROR` en las llamadas curl de holepunch (`lib/src/remote/holepunch.c`) para capturar el cuerpo de respuesta devuelto por Sony (JSON legible con códigos de error como `unauthorized_client`) antes de retornar error.
  2. **Refresco proactivo de token**: En `autoRegister` y `connectToHost` (`gui/src/qmlbackend.cpp`), se verifica la antigüedad del `access_token`; si tiene más de 50 minutos de antigüedad (<10 min restantes sobre los 3599s de validez), se refresca automáticamente antes de iniciar la sesión de holepunch, y la lambda mutable vincula el nuevo token a `info.psn_token`.
  3. **UX Fallback PIN en error 403**: Al detectarse `CHIAKI_ERR_HTTP_NONOK` (HTTP 403/Forbidden), la UI transiciona a `ConnectFailedForbidden` en `PsnView.qml`. Muestra el mensaje ejecutivo `"Sony ha rechazado la conexión remota. Prueba más tarde o registra la consola por PIN (más rápido en casa)"` con botones directos para registrar vía PIN con host resuelto (`root.showRegistDialog`) o volver al menú principal.
- **Correcciones Menores (16/09)**:
  1. **RegistDialog QQuickItem binding**: Corregido `RegistDialog.qml` y `DialogView.qml` asociando `okButton` vía `id: registDialog`, eliminando el warning de QML `Unable to assign [undefined] to QQuickItem*`.
  2. **Build de target ejecutable**: Identificado que el target CMake es `chiaki` (genera `build/gui/Ludelo.exe`). Corregida duplicación de cierre en `jsonrequester.cpp` y re-compilado `Ludelo.exe`, asegurando que la build en disco incorpora todas las directivas de redacción de logs de `cd04ae40`.
- **Alineación QR Login y Branding QML (Pylux -> Ludelo)**:
  1. **Compatibilidad QML<->C++**: En `gui/include/qmlbackend.h`, añadidos alias `Q_INVOKABLE` de compatibilidad (`getPyluxURL()`, `createPyluxCode()`, `checkPyluxStatus()`, `ensurePyluxSteamShortcut()`) redirigiendo a los métodos canónicos de Ludelo, previniendo crashes por bindings antiguos.
  2. **QRLoginDialog**: Migrado para invocar canónicamente `createLudeloCode`, `checkLudeloStatus` y `getLudeloURL` en `gui/src/qml/QRLoginDialog.qml`.
  3. **Auditoría global de branding**: Reemplazo exhaustivo de referencias "Pylux" a "Ludelo" en interfaces y diálogos QML (`MainView.qml`, `DialogView.qml`, `PsnView.qml`, `GamingModeAddedDialog.qml`, `ConsoleSetupWalkthrough.qml`, `ProfileDialog.qml`, `SteamShortcutDialog.qml`, `SettingsDialog.qml`, `DonationPromptDialog.qml`).
  4. **Tests Unitarios**: Añadido `test_qr_login_url_generation` en `test/auth_classifier_test.cpp`, verificando la URL del código QR y los endpoints de creación/estado de tokens. `munit` configurado con C11 en `test/CMakeLists.txt`. 136/136 tests (100%) pasando en CTest.


