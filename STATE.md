# STATE.md — Ludelo

> Este archivo es la memoria del proyecto. El agente lo actualiza tras cada tarea.
> Reglas: una línea por item, sin relatos. El usuario verifica criterios de fase, el agente nunca se autocertifica.

## Estado global
- Proyecto: **Ludelo** (renombrado completado en código, CMake, build.bat, tools y tests).
- FASE ACTIVA: **Etapa B finalizada — Preparado para publicación (Etapa C)**
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

## Pendiente inmediato (Etapa C)
- [ ] git init, higiene y primer commit limpio ("Initial commit: Ludelo")
- [ ] Generar PUBLISH-REPORT.md
- [ ] Configurar origin remoto y esperar confirmación explícita del usuario ("PUBLICA")

## Decisiones cerradas (no reabrir)
- Licencia: AGPL-3.0-or-later, repo público GitHub
- Marca: Ludelo. Sin activos ni glifos Sony.
- Cloud: solo experimental tras flag; nunca en listing.
- Store: pago único 9,99€, sin suscripción.
- Stack: C++23 + CMake/vcpkg, FFmpeg, SDL3, Vulkan/ImGui, OpenSSL, nlohmann, cpr, Catch2.
