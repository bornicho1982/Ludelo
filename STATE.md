# STATE.md — Ludelo

> Este archivo es la memoria del proyecto. El agente lo actualiza tras cada tarea.
> Reglas: una línea por item, sin relatos. El usuario verifica criterios de fase, el agente nunca se autocertifica.

## Estado global
- Proyecto: **Ludelo** (renombrado completado en código, CMake, build.bat, tools y tests).
- FASE ACTIVA: **Fase 0 CERRADA — Publicación completada con éxito** (Siguiente: Fase 1)
- Repo público: https://github.com/bornicho1982/Ludelo.git
- Fecha publicación: 11/09/2026
- Commit hash inicial: `9f076ea1ecbe530fe1714b47539d08c1a12ff0ab` (`9f076ea`)
- Build actual: **VERDE** (`bin\Ludelo.exe` y `bin\Ludelo_LiveDiagnostic.exe` compilados con éxito)
- Tests: **6/6 PASADOS (VERDE)** (`test_fec`, `test_discovery`, `test_dualsense`, `test_takion`, `test_auth`, `test_crypto`)
- Rama de seguridad / backup: `c:\proyectos antigravity\backup_play_portal_pre_b`

## Herencia de la sesión Kiro (VERIFICADA)
| Tarea | Estado declarado | Verificación realizada |
|---|---|---|
| Saneamiento hardcodes (IPs privadas, nick de usuario, fallback host_id) | Hecho | 0 coincidencias en src/, tests/, tools/, docs/ |
| build.bat maestro (Vulkan, Chiaki Core, Ludelo GUI, Tests, Diag) | Hecho | Build verde y 6/6 tests pasando en MSVC x64 |
| Eliminación de std::thread().detach() | Hecho | 0 llamadas en todo el código fuente (migradas a std::jthread) |
| Cloud streaming experimental flag-gated | Hecho | Protegido con LUDELO_CLOUD_EXPERIMENTAL; seguro para Store |
| Mica backdrop DWM integrado | Hecho | enum redefinition guardada (#ifndef DWMWA_COLOR_NONE) |
| ConsoleRegistry rp_auth protegido con DPAPI | Hecho | Cifrado y descifrado transparente con Windows DPAPI |
| AppSettings JSON + persistencia | Hecho | Compilado e integrado en build.bat |

## Bugs/riesgos conocidos resueltos
- [x] `std::thread().detach()` erradicado por completo (0 llamadas en el código).
- [x] Fallo LNK2019 AppSettings resuelto (añadido Config/*.cpp a build.bat).
- [x] Conflicto de símbolos con diagnostic tool resuelto (obj_diag separado).
- [x] Rutas OpenSSL centralizadas a nivel global en build.bat.
- [x] Suite de pruebas 100% verde (6/6 ejecutables compilados y validados).
- [x] Documentación legal generada: LICENSE (AGPL-3.0), NOTICE, THIRD-PARTY-LICENSES.md, README.md.
- [x] Historial limpio y 0 secretos en el repositorio público.

## Hitos completados (Fase 0)
- [x] git init, higiene y primer commit limpio ("Initial commit: Ludelo")
- [x] Generar PUBLISH-REPORT.md
- [x] Configurar origin remoto y push a main en GitHub (público)
- [x] Verificación de acceso HTTP 200 al repositorio y commit remoto

## Decisiones cerradas (no reabrir)
- Licencia: AGPL-3.0-or-later, repo público GitHub
- Marca: Ludelo. Sin activos ni glifos Sony.
- Cloud: solo experimental tras flag; nunca en listing.
- Store: pago único 9,99€, sin suscripción.
- Stack: C++23 + CMake/vcpkg, FFmpeg, SDL3, Vulkan/ImGui, OpenSSL, nlohmann, cpr, Catch2.
- Datos de app: `%APPDATA%\Roaming\Ludelo\` (logs, config, keychain, cache). Se eligió Roaming sobre LocalAppData para permitir sincronización de perfil entre equipos del mismo dominio.
- Fuente UI: NotoSansCJK-Regular.ttc (libre) + Inter.ttf (OFL). SFNS.ttf (Apple) eliminada.

## Decisión Comercial: Modelo de Distribución y Monetización (Store / DLC)
- **Modelo Base (GitHub):** 100% código abierto bajo AGPL-3.0-or-later. Los usuarios pueden clonar, compilar y ejecutar libremente sin coste ni restricciones funcionales.
- **Canal Microsoft Store:** Empaquetado MSIX a precio único de 9,99€ (sin modelo de suscripción periódica ni anuncios).
- **Justificación comercial y legal:**
  1. *Valor para el usuario de Store:* Instalación segura en un solo clic, actualizaciones automáticas en segundo plano gestionadas por Microsoft Store, binarios firmados digitalmente de confianza (cero alertas de Windows SmartScreen) y soporte directo para el desarrollo continuo.
  2. *Cumplimiento AGPL-3.0:* Totalmente compatible. El código fuente completo permanece público en GitHub y se enlaza de forma transparente tanto en la ficha de la tienda como en los créditos de la aplicación.
  3. *Alternativa Store DLC (Supporter Pack):* Si las directivas de Microsoft Store exigen listing base gratuito para la categoría, se publicará Ludelo como descarga gratuita con "Ludelo Supporter Pass" (DLC opcional de 9,99€) que habilita cosméticos de interfaz (temas de color adicionales y personalización estética) manteniendo todas las capacidades de streaming en la versión base sin barreras.

## Fase 1 — Progreso
- [x] PSN Account-ID incluido en regist (campo Np-AccountId, obligatorio para PS5).
- [x] Propagación IP DDP→registro restaurada (fallback host_name en lugar de host_id).
- [x] SFNS.ttf y texturas PlayStation eliminadas del proyecto.
- [x] Modal de vinculación: sección PSN visible ANTES del PIN, botón deshabilitado sin cuenta.
- [x] Onboarding: pantalla de bienvenida con login NPSSO al primer arranque sin cuenta.
- [x] Registro PS5 integrado con Chiaki Core (chiaki_regist_start / stop / fini), sustituyendo implementación manual (fix 403 80108bff).
- [x] Persistencia automática en ConsoleRegistry con rp_key y rp_regist_key vía DPAPI y transición inmediata a tarjeta de consola activa ("CONECTAR AHORA").
- [x] Limitador de FPS y VSync (present mode FIFO) en render loop: 60 FPS en menús activos, 20 FPS en reposo/minimizado, y cap al framerate de stream durante sesión (GPU 100% resuelto).
- [x] Deadzone radial configurable de sticks (default 0.15) en ControllerManager antes de enviar state a Chiaki, slider en Ajustes > Mando y log de confirmación (drift resuelto).
- [x] Higiene de logs de seguridad: ofuscación obligatoria (primeros/últimos 4 caracteres + ****) en rp_key, rp_auth, RP-Nonce y MAC/host_id de la consola en todo log output y suite de tests.
- [x] Login PSN automático: reemplazo completo del flujo manual (eliminadas instrucciones de F12/cookies/NPSSO) por WebView2 embebido modal con captura de redirect code, descarga silenciosa oficial del bootstrapper Evergreen si falta runtime, fallback modal de navegador ("copia la URL completa") sin cookies, descarga inmediata de avatar PNG y visualización de online_id real en la tarjeta de perfil del dashboard.
- [x] A1: Wake-on-LAN vía chiaki_discovery_wakeup() con reintentos a 10s/20s, timeout 30s, botón 'Despertar' separado y banner de ayuda en fallo.
- [x] A2: Modal de PIN flexible (1-8 dígitos), toggle 'Mi consola no pide PIN' (PIN 0), textos de ayuda y feedback de PIN rechazado.
- [x] A3: Paridad de streaming con fast_bilinear, cola de frames cero lag, prioridad MMCSS (THREAD_PRIORITY_HIGHEST / Games) y modo --glass-to-glass en diagnóstico.
- [x] A4: Paridad de mando con carga de gamecontrollerdb.txt, soporte de giroscopio configurable, fallback de touchpad y modal de test interactivo de sticks/botones.
- [x] A5: UX de errores WebView2 con detección DOM de 'Something went wrong', redirección automática a fallback y toast interactivo de cancelación [Reintentar]/[Usar navegador] (regresión corregida: no tratar error=login_required como fatal; auto-lectura de portapapeles y guard contra reapertura en fallback).
- [x] B1: Tokens y paleta definitiva centralizados en src/UI/Theme.h y documentados en docs/DESIGN-SYSTEM.md (Base #0E1117, Panel #171B24, Indigo #6C5CE7, Mint #00F5D4, radios 12/16/20px, elevación 3 niveles).
- [x] B2: Tipografía Segoe UI Variable e Inter (OFL) con CJK estrictamente en MergeMode fallback y escala 24/16/14/12/30px sin artefactos.
- [x] B3: Iconografía outline vectorial limpia 2px (base 24px/48px retina) sin marcas ni glifos propietarios de PlayStation/Sony.
- [x] B4: Branding propio Ludelo con monograma 'L', wordmark y app.ico multi-resolución (16-256px) asignado a ventana HWND y barra de tareas.
- [x] B5: Rediseño de componentes clave: tarjetas de consola con elevación, píldoras Mint/Amber/Slate, métricas en tiempo real (ping, codec, bitrate), topbar con estado LAN/avatar y sliders con track primario, thumb menta y tooltips técnicos.
- [x] C1: Decisión comercial documentada en STATE.md: modelo Store 9,99€ pago único para conveniencia/actualizaciones con base AGPL libre en GitHub y alternativa DLC Supporter Pack.
- [x] C2: README.md reescrito con enfoque 100% usuario final (guía paso a paso, características clave, FAQ de seguridad/mandos y notas de compilación secundarias).
- [x] C3: THIRD-PARTY-LICENSES.md actualizado con licencias de Inter Font (OFL-1.1), SDL GameControllerDB (zlib), Lucide Icons (ISC) y Noto Sans CJK (OFL-1.1).
- [x] C4: Documento de diseño arquitectónico de streaming IPC preservado de forma privada en notas externas fuera del repo; docs/notes/ completamente ignorado y protegido.
- [x] Higiene de repo: retirada de docs/notes/design-streaming-service.md del repo y del historial git, archivo resguardado en notes_privados/ y regla de exclusión estricta en DIRECTIVAS.md.
- [x] Hotfix Sesión B1: Salidas múltiples en modal de test de mando (botón X visible, tecla Esc, botón B/Circle del mando y clic en dimmer).
- [x] Hotfix Sesión B2: Focus-gating total de gamepad e ImGui Nav en SDL_EVENT_WINDOW_FOCUS_GAINED/LOST (verificado: sin procesos Steam Input/mapeo global activos).
- [x] Hotfix Sesión B3: Parámetros de streaming alineados con chiaki-ng (a_rwnd 4MB, reorder buffer 256, RTT LAN 1ms, drenado de audio progresivo y pacing VSync sin doble sleep).
- [x] Branding forense (1/2): Eliminación de restos PortalPC en vcpkg.json, tools/capture_ui.ps1, tools/cleanup.bat y cabeceras de shaders.

