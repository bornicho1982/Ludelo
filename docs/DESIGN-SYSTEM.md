# Ludelo Design System (V1 2026)

## 1. Visión y Principios
Ludelo es un cliente nativo para Windows 11 de PlayStation Remote Play enfocado en ultra baja latencia, estética moderna 10-foot console-first y respeto absoluto a la propiedad intelectual de Sony.
- **Identidad propia**: No somos un clon de PlayStation. Usamos una estética inspirada en consolas de nueva generación con nuestra propia paleta (*Electric Indigo* + *Cyber Mint*).
- **Cero activos de terceros**: Ningún logo registrado ni glifos propietarios.
- **Elevación y fluidez**: Transiciones suaves, fondos con material Mica DWM y jerarquía visual de tres niveles.

---

## 2. Paleta de Tokens (Theme Tokens)

| Token | Hex | RGB (Normalizado) | Propósito |
|---|---|---|---|
| `kBase` | `#0E1117` | `(0.055, 0.067, 0.090)` | Fondo global y scrim profundo |
| `kPanel` | `#171B24` | `(0.090, 0.106, 0.141)` | Tarjetas, barras, paneles de configuración |
| `kPrimary` | `#6C5CE7` | `(0.424, 0.361, 0.906)` | **Electric Indigo Ludelo** — Botones primarios, acento de marca |
| `kPrimaryHover` | `#7D6FF0` | `(0.490, 0.435, 0.941)` | Hover en botones primarios |
| `kAccent` | `#00F5D4` | `(0.000, 0.961, 0.831)` | **Cyber Mint** — Consola Awake, métricas activas, focus |
| `kAlert` | `#FFB703` | `(1.000, 0.718, 0.012)` | **Amber Gold** — Consola en Modo Reposo (Standby), advertencias |
| `kError` | `#EF476F` | `(0.937, 0.278, 0.435)` | **Coral Pink** — Desconexión, errores de red |
| `kTextPrimary` | `#F8F9FA` | `(0.973, 0.976, 0.980)` | Texto principal y encabezados |
| `kTextSecondary` | `#94A3B8` | `(0.580, 0.639, 0.722)` | Etiquetas secundarias, descripciones |
| `kTextMuted` | `#64748B` | `(0.392, 0.455, 0.545)` | Separadores, estados inactivos |

---

## 3. Radios y Métricas de Forma
- **Componentes (`12px`)**: Botones de acción, campos de entrada, botones de selector (pills).
- **Tarjetas (`16px`)**: Tarjetas de consola, paneles de ajustes, tarjetas de cloud games.
- **Modales (`20px`)**: Diálogos modales (vincular consola, fallback de navegador, test de mando).

---

## 4. Elevación e Iluminación
1. **Nivel 1 (Reposo / Estático)**: Borde sutil `kGlassBorder` (`#333B51`) con grosor 1.0px.
2. **Nivel 2 (Hover / Seleccionado)**: Borde `kPrimary` (`#6C5CE7`) con resplandor suave de 2.0px.
3. **Nivel 3 (Foco / Streaming Activo)**: Borde `kAccent` (`#00F5D4`) con brillo de 3.0px.

---

## 5. Tipografía y Jerarquía
- **Primaria**: *Segoe UI Variable Display* (Windows 11) / *Inter* (OFL) / *Segoe UI* (Windows 10).
- **Fallback CJK**: *Noto Sans CJK* cargado estrictamente como font-merge para glifos orientales.
- **Escala tipográfica**:
  - `Title`: 24px, peso 600 (Semibold/Bold).
  - `Subtitle`: 16px, peso 600.
  - `Body`: 14px, peso 400 (Regular).
  - `Caption`: 12px, peso 400.
  - `Metrics / PIN`: 18–32px, Monospace / Tabular numbers.

---

## 6. Iconografía y Glifos de Mando
- Conjunto outline de 2px con caja base de 24x24 px (estilo Lucide / Phosphor).
- **Mando**:
  - Botones de acción nombrados neutralmente: *Sur (A)*, *Este (B)*, *Oeste (X)*, *Norte (Y)*.
  - Botones frontales: *L1*, *R1*, *L2*, *R2*, *L3*, *R3*.
  - Navegación del sistema: *Menú*, *Vista*, *Guía*, *Touchpad*.
  - Prohibido el uso de símbolos de marca registrados.
