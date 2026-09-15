# Guía de Sincronización con Upstream (Pylux / chiaki-ng)

Este documento describe el procedimiento estándar para incorporar actualizaciones, correcciones de errores y mejoras de protocolo desde los proyectos upstream ([Pylux](https://github.com/ForWard-Technologies-LLC/Pylux) y [chiaki-ng](https://github.com/streetpea/chiaki-ng)) hacia **Ludelo**, protegiendo al mismo tiempo las personalizaciones exclusivas de nuestra arquitectura (interfaz QML propia, branding, login embebido y configuraciones de Windows).

---

## 1. Configuración de Remotos Git

El repositorio local debe mantener dos remotos principales:

```bash
# Repositorio oficial propio de Ludelo
git remote add origin https://github.com/bornicho1982/Ludelo.git

# Repositorio upstream de Pylux (referencia primaria de protocolo y cloud)
git remote add upstream https://github.com/ForWard-Technologies-LLC/Pylux.git

# Opcional: Upstream chiaki-ng directo (si se requiere cherry-pick específico de streaming base)
git remote add chiaki-ng https://github.com/streetpea/chiaki-ng.git
```

---

## 2. Árbol de Responsabilidad: Qué carpetas se actualizan

| Directorio | Origen / Política | Comportamiento en sincronización |
|---|---|---|
| `lib/` | **Upstream (Pylux / chiaki-ng)** | **Se sincroniza.** Contiene el protocolo de streaming, Takion, criptografía, descubrimiento, códecs y FEC. Toda lógica compartida debe mantenerse idéntica a upstream. |
| `third-party/` | **Upstream / Submódulos** | **Se sincroniza.** Actualizaciones de nanopb, jerasure, gf-complete, curl o cpp-steam-tools. |
| `psn-api/` | **Upstream (Pylux)** | **Se sincroniza.** Rutas de endpoints de PSN y estructuras de catálogo/Apollo. |
| `gui/` (o `qt-quick/`) | **Propio (Ludelo)** | **NO se pisa.** La UI visual de Ludelo (QML, diseño índigo, diálogo de login embebido) es propia. Se analizan diffs de `qmlbackend.cpp` para incorporar nuevas llamadas a `lib/` si se añaden métodos, pero nunca se sobrescribe la UI. |
| `docs/` | **Propio (Ludelo)** | **NO se pisa.** Documentación, especificaciones y maquetas de Ludelo. |
| `cmake/` | **Mixto** | Se sincronizan módulos de detección de librerías (`FindFFMPEG.cmake`, etc.), revisando que no rompan las directivas de Windows. |
| `CMakeLists.txt` | **Propio (Ludelo)** | Se adaptan manualmente las variables nuevas necesarias sin alterar el recorte a Desktop ni el nombre de target `Ludelo.exe`. |

---

## 3. Flujo de Trabajo para Traer Actualizaciones

### Método A: Cherry-pick puntual (Recomendado para fixes críticos)

Cuando upstream publica un fix específico (por ejemplo, corrección de audio, nuevo opcode de PS5 o adaptación de PSN):

1. **Obtener últimos cambios:**
   ```bash
   git fetch upstream
   ```
2. **Crear rama de integración temporal:**
   ```bash
   git checkout -b sync/patch-<nombre-del-arreglo>
   ```
3. **Aplicar el commit:**
   ```bash
   git cherry-pick -x <commit-hash>
   ```
4. **Resolver conflictos si existen:**
   - Si el conflicto está en `lib/`: aceptar los cambios de upstream.
   - Si el conflicto toca archivos de GUI/QML: adaptar la llamada a la interfaz de Ludelo preservando nuestro estilo.
5. **Compilar y verificar tests:**
   ```bash
   # Compilar
   cmake --build build --config Release --target Ludelo -j8
   # Ejecutar tests
   ctest --test-dir build --output-on-failure
   ```
6. **Merge a `main` y push a `origin`:**
   ```bash
   git checkout main
   git merge --ff-only sync/patch-<nombre-del-arreglo>
   git push origin main
   ```

---

## 4. Submódulos: Mantenimiento

Tras cualquier rebase o cherry-pick que toque referencias a submódulos:
```bash
git submodule update --init third-party/nanopb third-party/jerasure third-party/gf-complete third-party/curl third-party/cpp-steam-tools test/munit
```
*(Nota: No actualizar submódulos de Android ni Switch ya que están fuera del alcance desktop de Ludelo).*

---

## 5. Regla de Oro de Integridad

Cualquier mejora en el motor de vídeo, audio o conexión **debe vivir en `lib/`** para que las futuras actualizaciones de upstream encajen limpiamente sin divergencias insalvables.
