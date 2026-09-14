# ROADMAP — Ludelo (ruta comercial Store/Steam)

**FASE ACTIVA: 0**
(Mover solo cuando el criterio de aceptación de la fase esté verificado por el usuario, no por el agente.)

---

## FASE 0 — Seguridad y fundación legal (AHORA, antes de cualquier otra cosa)
- Rotar NPSSO: cerrar sesiones en ajustes PSN y generar token nuevo (el anterior estuvo hardcodeado en el código).
- `git init` + primer commit del estado actual.
- Copia fuera del disco de todo lo que Kiro ha tocado hoy.
- Limpiar historial: cualquier IP privada real, NPSSO, nick de usuario real, credencial de consola real, clave fallback debe desaparecer de TODO el historial antes de hacer el repo público (push forzado en repo nuevo vacío).
- Crear `LICENSE` (AGPL-3.0), `NOTICE` (Chiaki/chiaki-ng), `THIRD-PARTY-LICENSES.md`, `README.md` con nombre **Ludelo** y disclaimer "software independiente, no afiliado ni avalado por Sony Interactive Entertainment".
- Renombrar carpetas/proyecto/solución PortalPC → Ludelo en todo el árbol de código.
**Aceptación:** Verificación de secretos con 0 hallazgos en fuentes e historial. Repo público `github.com/<usuario>/ludelo` creado.

## FASE 1 — Saneamiento y build (Tasks 1–3 de Kiro)
- Cero hardcodes personales en fuentes.
- `build.bat` reproducible en máquina limpia con VS2022: protobuf generado, libs completas, detección VS correcta.
- `send_ps_button_pulse` con `jthread`; `get_stats()` con mutex; `disconnect()` con join previo.
**Aceptación:** `build.bat clean && build.bat app` verde en máquina limpia; 10 conectar/desconectar sin crash; `--test-stream` sin consola registrada da error claro.

## FASE 2 — Motor vídeo/audio (Task 4 corregido)
- VideoDecoder: swscale NV12→RGBA ya implementado; verificar contra stream H264 y H265 real.
- Eliminar fallback scalar YUV de App.cpp (ya hecho por Kiro; validar visualmente).
- Audio WASAPI/Opus sin crackling a 1080p60.
**Aceptación:** 30 min de sesión estable en LAN, <80 ms latencia visual, CPU UI <3%, cero warnings nuevos.

## FASE 3 — Cloud detrás de flag (Tasks 5–6 SOLO como experimental)
- GaikaiClient + connect_cloud compilando bajo `LUDELO_CLOUD_EXPERIMENTAL=1`.
- Probing manual con cuenta propia Premium; documentar en `docs/cloud-research.md` sin incluir datos de cuenta.
**Aceptación:** build de Store con flag OFF no contiene símbolos ni strings de Kamaji. (Verificación: `strings` sobre el exe no muestra "kamaji.cloud.playstation.com".)

## FASE 4 — UI y configuración (Tasks 7–10)
- Mica backdrop (ya hecho; verificar en W11 limpio).
- Cards + navegación por mando completa, branding Ludelo, SIN glifos ni iconografía PS propietaria.
- Streaming fullscreen + HUD auto-oculto + stats.
- AppSettings + ConsoleRegistry round-trip JSON; NPSSO nunca persistido en claro.
**Aceptación:** flujo completo descubrir→registrar→jugar→cerrar navegable solo con DualSense; cierre/reapertura conserva ajustes y consola.

## FASE 5 — Tests (Tasks 11–12)
- Catch2 + CTest: 11+ tests verdes (`test_auth` incluye round-trip sin NPSSO en JSON).
- Shaders GLSL fuente + `compile_shaders.bat` integrado.
**Aceptación:** ctest verde; compilación de shaders reproducible; ninguna cadena "PortalPC" en strings del binario.

## FASE 6 — MSIX y Partner Center (Tasks 13–14)
- `packaging/` con AppxManifest: identity `Ludelo`, capabilities mínimas (internetClient, privateNetworkClientServer, bluetooth), assets 100% propios, `makeappx`+`signtool` para sideload.
- Cuenta Partner Center individual; reservar nombre **Ludelo**; pricing 9,99€; IARC "Todos"; privacy policy + página soporte; listing ES/EN según `STORE-LISTING.md`, sin mencionar cloud.
**Aceptación:** .msix instala/arranca/desinstala limpio en W11 limpio; validador de Partner Center sin errores.

## FASE 7 — Store y fallback Steam (Task 15)
- Submission Store con 4+ capturas propias (sin UI oficial Sony) y trailer 30 s.
- Si certificación rechaza citando políticas de terceros: documentar motivo y publicar en Steam (mismo build, sin MSIX) manteniendo AGPL.
**Aceptación:** app publicada en al menos una tienda de pago, o rechazo documentado con apelación redactada.

## FASE 8 — Cloud opcional (POST-LANZAMIENTO, revisión legal previa)
Solo si las Fases 0–7 están cerradas y hay producto vendiendo. Decisión del usuario, nunca del agente.

---

## CHECKPOINTS ENTRE FASES
- Build limpio en máquina limpia, ctest verde, STATE.md actualizado.
- `strings` del binario: sin datos personales, sin Kamaji (flag off), sin rutas de desarrollo, sin "PortalPC".
- docs/notes/ y cualquier archivo de diseño comercial está PROHIBIDO en el repo; el agente verifica `git status` antes de cada push.
- Usuario verifica el criterio; el agente nunca marca su propia fase como completa.
