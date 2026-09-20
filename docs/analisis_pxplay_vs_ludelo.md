# Análisis Comparativo Exhaustivo: PXPlay (Pylux) vs Ludelo

**Fecha:** 20 de Septiembre de 2026  
**Autor:** Antigravity AI (Pair Programming con Bornicho)  
**Objetivo:** Comparativa exhaustiva pantalla por pantalla entre PXPlay (Pylux Remote Play Client v2.10.21) y el build actual de Ludelo (commit `bec3798b`), inventario completo de opciones y defaults, auditoría de defectos visuales en capturas, matriz de paridad técnica y plan de acción priorizado.

---

## 1. Inventario Exhaustivo PXPlay (Pylux v2.10.21)

PXPlay (en su versión de escritorio Pylux) es un cliente maduro para PS4/PS5 Remote Play y Cloud Gaming. Posee una interfaz oscura (fondos azules y púrpuras degradados `#0A0E1A` a `#1E1035`), diseñada para navegación híbrida (ratón, teclado y mando con bumpers `[L1]`/`[R1]` y atajos de pie `(A)`, `(B)`, `(X)`, `(Y)`).

A continuación se detalla cada una de las pantallas y menús examinados en las 54 capturas oficiales:

### 1.1 Pantalla de Login PSN (`084217.png`, `091110.png`)
* **Propósito:** Autenticación con PlayStation Network para vincular la cuenta del usuario.
* **Flujos ofrecidos:**
  1. **Código QR (Dispositivo Móvil):** Muestra un código QR central grande para escanear con la cámara del móvil apuntando al servicio puente `psstream.app`. Debajo indica: *"Scan the QR code with your mobile device or open the following link on any device"*.
  2. **Alternativa en PC (`Sign in on this device`):** Botón que redirige a un navegador web local en la URL oficial de Sony OAuth, permitiendo extraer el código de redirección o pegar el token/URL de respuesta.
* **Valores y controles:**
  - Botón: `Sign in on this device`
  - Enlace de soporte: `Help`
  - Footer de mando: `(A) Confirm`, `(B) Cancel`

### 1.2 Guía de Configuración Inicial (Setup Guide 1/5 a 5/5) (`090908.png` - `091010.png`)
* **Paso 1 (Enable Remote Play):** Instrucciones textuales para habilitar el uso a distancia en la consola (Ruta PS5: *Ajustes > Sistema > Uso a distancia*; Ruta PS4: *Ajustes > Ajustes de conexión del Uso a distancia*).
* **Paso 2 (Network Environment):** Recomendación de conectar la consola por cable Ethernet y el cliente a la banda Wi-Fi de 5 GHz o la misma subred LAN.
* **Paso 3 (Manual IP Configuration):** Explicación de cómo configurar una IP manual fija si el broadcast UDP de descubrimiento automático está bloqueado por el router.
* **Paso 4 (Register Console):** Procedimiento para generar el código PIN de vinculación temporal de 8 dígitos en la consola (*Vincular dispositivo*).
* **Paso 5 (Controller & Steam Deck Mapping):** Diagrama de controles específico para Steam Deck y mandos de juego estándar, detallando la asignación de gatillos traseros `L4/L5/R4/R5`, emulación de clic de Touchpad y botón PS/Guide.
* **Navegación:** Paginador de 5 puntos, botón `Next`, botón `Back`, botón `Finish`.

### 1.3 Pantalla Principal / Home de Consolas (`091027.png`, `091046.png`)
* **Header:**
  - Logo PYLUX a la izquierda con subtítulo "Remote Play Client".
  - Selector superior de pestañas: `Remote Play` (activo) | `Cloud Play`.
  - Iconos de acción en esquina superior derecha: Login de usuario `[->]`, Ajustes `[Gear]`, Cerrar aplicación `[X]`.
* **Cuerpo de Consolas:**
  - Tarjetas de consola descubiertas en la red local.
  - Cada tarjeta incluye:
    - Silueta gráfica de la consola (PS5 / PS4).
    - Nombre del dispositivo (ej. *"PlayStation 5"*).
    - Dirección IP resuelta (ej. `192.168.1.139`).
    - Botón individual `[X]` en la esquina superior de la tarjeta para ocultar la consola de la vista.
  - Botón flotante FAB (esquina inferior derecha): Botón `+` para añadir consola remota o manual.
  - Botón flotante Wi-Fi Discovery: Indicador/botón para refrescar el escaneo broadcast de red.
* **Diálogo contextual al pulsar consola (`091046.png`):**
  - Modal que consulta: *"Choose Action"*.
  - Opciones: `Connect to Console`, `Register Console`, `Cancel`.

### 1.4 Diálogo de Registro / Vinculación de Consola (`091125.png`)
* **Campos y controles:**
  - `Host / IP Address:` Campo de texto para la dirección IPv4 local o pública.
  - `Remote Play PIN:` Campo para los 8 dígitos numéricos obligatorios.
  - `PSN Account ID:`
    - Botón `By Login`: Obtiene el ID mediante el login activo en PSN.
    - Botón `By Lookup`: Utilidad para buscar el Account ID público introduciendo el Online ID (PSN tag) sin iniciar sesión completa.
    - Campo de texto editable para introducir manualmente el Account ID codificado en Base64 (8 bytes).
  - `Console PIN (Optional):` Campo opcional de 4 dígitos numéricos para perfiles de consola protegidos con código de acceso.
  - `Firmware / Target Selection:` Selector radial con 3 opciones:
    - Radio `PS4 Firmware < 7.0`
    - Radio `PS4 Firmware >= 7.0 / 8.0`
    - Radio `PS5`
* **Acciones:** Botón primario `Register`, botón `Cancel`.

---

### 1.5 Ajustes de PXPlay (9 Pestañas en `Settings`)

La barra de navegación de ajustes incluye 9 categorías conmutables mediante ratón o bumpers `[L1]` y `[R1]`:

```
[L1]  General | Video | Stream | Audio/Wifi | Consoles | Keys | Controllers | Config | Cloud  [R1]
```

#### 1.5.1 Pestaña `General` (`095845.png`, `095902.png`, `095925.png`)
* `Action on Disconnect:` Dropdown con opciones:
  - `Ask` *(Default)*
  - `Do Nothing`
  - `Enter Sleep Mode` (pone la consola en modo reposo al cerrar el stream)
* `Action on Suspend:` Dropdown con opciones:
  - `Do Nothing` *(Default)*
  - `Enter Sleep Mode`
* `Steam Deck Haptics:` Checkbox (Default: Unchecked). Activa retroalimentación háptica en trackpads de Steam Deck.
* `Steam Deck Vertical (Gyro Inverted):` Checkbox (Default: Unchecked). Invierte el eje giroscópico al usar el dispositivo en vertical.
* `Audio/Video Mode:` Dropdown: `Both Enabled` *(Default)*, `Video Only`, `Audio Only`.
* `Log Directory:` Botón interactivo que abre la carpeta de registros en el explorador de archivos del sistema.
* `Streamer Mode:` Checkbox (Default: Unchecked). Oculta IPs, MACs y datos identificativos en la interfaz para transmisiones en directo.
* `Stream Menu Shortcut:` Checkbox (Default: Checked). Habilita el atajo de botones de mando para desplegar el HUD en juego.
* `Stream Menu Combo:` 4 Dropdowns encadenados para elegir la combinación de botones físicos (Default: `L1 + R1 + L3 + R3`).
* `Sync Console Game Data:` Checkbox (Default: Checked). Sincroniza títulos instalados e información de juegos desde la consola.
* `Display Game Art While Loading:` Checkbox (Default: Checked). Muestra la carátula oficial del juego durante el inicio de la sesión.

#### 1.5.2 Pestaña `Video` (`095941.png` - `100100.png`)
* `Hardware Decoder:` Dropdown:
  - `auto` *(Default)*
  - `d3d11va` (Direct3D 11 Video Acceleration en Windows)
  - `vulkan` (Vulkan Video Extensions)
  - `none` (decodificación por software CPU)
* `Window Type:` Dropdown:
  - `Selected Resolution` *(Default)*
  - `Custom Resolution`
  - `Adjustable Resolution`
  - `Fullscreen`
  - `Zoom`
  - `Stretch`
* `Toggle Fullscreen on Double-click:` Checkbox (Default: Unchecked).
* `Hide Cursor:` Checkbox (Default: Checked). Oculta el puntero del ratón tras unos segundos de inactividad durante la retransmisión.
* `Render Preset:` Dropdown:
  - `Default` *(Default)*
  - `Fast`
  - `High Quality`
  - `Custom`
* `Display Settings:` Botón que despliega el diálogo modal de calibración de pantalla y pipeline de color.

#### 1.5.3 Submenú `Display Settings` (Calibración libplacebo) (`100136.png` - `100234.png`)
* `Target Primaries:` Dropdown con espacios de color de destino:
  - `Auto` *(Default)*
  - `BT.709` (sRGB / Rec.709 estándar SDR)
  - `BT.2020` (Rec.2020 Wide Color Gamut para HDR)
  - `Apple RGB`
  - `Adobe RGB`
  - `ProPhoto RGB`
  - `DCI-P3`
* `Target Transfer:` Dropdown con funciones de transferencia electro-óptica (EOTF):
  - `Auto` *(Default)*
  - `sRGB`
  - `BT.1886`
  - `Linear`
  - `PQ` (Perceptual Quantizer / SMPTE ST 2084 para HDR10)
  - `HLG` (Hybrid Log-Gamma)
  - `Gamma 1.8`, `Gamma 2.0`, `Gamma 2.2`, `Gamma 2.4`, `Gamma 2.6`, `Gamma 2.8`
* `Target Peak:` Dropdown:
  - `Auto` *(Default)*
  - `Numeric` (Permite especificar nits máximos de luminancia del monitor, ej. 400, 600, 1000 nits)
* `Target Contrast:` Dropdown:
  - `Auto` *(Default)*
  - `Infinity` (Para paneles OLED con negro puro)
  - `Numeric` (Relación de contraste estático, ej. 1000:1 para paneles IPS)

#### 1.5.4 Pestaña `Stream` (`100330.png` - `100638.png`)
* `Settings for:` Dropdown selector: `PS5` | `PS4`.
* **Estructura en dos columnas:** `Local` (LAN) frente a `Remote` (Internet / Fuera de casa).
* **Parámetros para PS5 (`100330.png`):**
  - `Resolution:` Dropdown independiente para Local y Remote:
    - Opciones: `360p`, `540p`, `720p`, `1080p` *(Default: 1080p)*
  - `FPS:` Dropdown independiente:
    - Opciones: `30 fps`, `60 fps` *(Default: 60 fps)*
  - `Bitrate:` Slider independiente:
    - Rango: 5 Mbps a 50 Mbps *(Default Local: 15 Mbps; Default Remote: 10 Mbps)*
  - `Codec:` Dropdown independiente:
    - Opciones: `H264`, `H265`, `H265 HDR` *(Default: H265)*
* **Parámetros para PS4 (`100541.png` - `100638.png`):**
  - `Resolution:` `360p`, `540p`, `720p (Default)`, `1080p (PS5 and PS4 Pro)`
  - `FPS:` `30 fps`, `60 fps (Default: 60 fps)`
  - `Bitrate:` Slider *(Default Local: 10 Mbps; Default Remote: 10 Mbps)*
  - *Nota:* Sin selector de códec para PS4 (fijado internamente a AVC / H.264).

#### 1.5.5 Pestaña `Audio/Wifi` (`100022.png` - `100100.png`)
* `Output Device:` Dropdown dinámico enumerando dispositivos de audio del SO (ej. *Auto*, *Realtek Digital Output*, *NVIDIA High Definition Audio*) *(Default: Auto)*.
* `Input Device:` Dropdown dinámico enumerando micrófonos del SO (ej. *Auto*, *Micrófono WEB CAM*, *Micrófono Q9-1*) *(Default: Auto)*.
* `Audio Buffer Size:` Slider de 10 ms a 200 ms *(Default: 50 ms)*.
* `Audio Volume:` Slider de 0% a 100% *(Default: 100%)*.
* `Start Mic Unmuted:` Checkbox *(Default: Unchecked)*.
* `Speech Processing:` Checkbox *"Noise suppression + echo cancellation"* *(Default: Unchecked)* (procesamiento Speex DSP).
* `Weak Wifi Notification:` Slider con umbral en `%` de paquetes perdidos *(Default: >= 3% dropped packets)*.
* `Packet Loss Reported Max:` Slider con tope de telemetría reportada *(Default: 5% packet loss)*.
* `Show Stream Stats During Gameplay:` Checkbox *(Default: Unchecked)*.

#### 1.5.6 Pestaña `Consoles` (`100148.png`)
* Botón: `Register New` (inicia el asistente de vinculación).
* Lista: `Registered Consoles` (consolas emparejadas con sus MACs, nicknames, IP y opciones de eliminación/edición).
* Lista: `Hidden Consoles` (consolas detectadas en red que el usuario ha decidido ocultar, con opción de desocultar).

#### 1.5.7 Pestaña `Keys` (Mapeo de Teclado y Ratón) (`100209.png`)
* Botón: `Reset All Keys` (restaura la configuración de fábrica).
* `Enable Keyboard mapping:` Checkbox *(Default: Checked)*.
* `Enable Mouse Touchpad:` Checkbox *(Default: Checked)* (control del cursor del touchpad mediante el ratón).
* **Distribución en 3 columnas para remapear teclas:**
  - *Columna 1:* Cross (`Return`), Pyramid (`C`), D-Pad Up (`Up`), R1 (`3`), Options (`O`), PS (`Esc`), Left Stick Right (`]`), Left Stick Down (`Del`), Right Stick Up (`PgUp`).
  - *Columna 2:* Moon (`Backspace`), D-Pad Left (`Left`), D-Pad Down (`Down`), L3 (`5`), Share (`F`), L2 (`1`), Left Stick Left (`[`), Right Stick Right (`=`), Right Stick Down (`PgDown`).
  - *Columna 3:* Box (`\`), D-Pad Right (`Right`), L1 (`2`), R3 (`6`), Touchpad (`T`), R2 (`4`), Left Stick Up (`Ins`), Right Stick Left (`-`).
  - *Nota legal notable:* PXPlay evita las marcas registradas de Sony renombrando los botones geométricos a `Cross`, `Moon` (Círculo), `Box` (Cuadrado) y `Pyramid` (Triángulo).

#### 1.5.8 Pestaña `Controllers` (`100311.png` - `100542.png`)
* Botones principales: `Change Controller Mapping`, `Reset Controller Mapping`, `Apply Base Layout`.
* `Controller Capture:` Al pulsar *Change Controller Mapping*, se abre un modal interactivo: *"Choose the controller by pressing any button on the controller"*.
* Pantalla de mapeo de mando detectado (ej. *"Xbox 360 Controller"*):
  - Checkbox: `Allow mapping analog sticks and L2/R2` *(Default: Unchecked)*.
  - Tabla de 3 columnas con slots primario y secundario por cada botón físico del gamepad (b0 a b10, h0.1 a h0.8, ejes a0 a a5).
  - Botón: `Update Controller Mapping`.
* `Background Controller Events:` Checkbox *"Process controller input when application is in background"* *(Default: Checked)*.
* `Dpad Touchpad Emulation:` Checkbox *(Default: Checked)*.
* `Dpad Touch Increment:` Slider *(Default: 0.3 mm)*.
* `Dpad Regular/Touch Combo:` 4 Dropdowns para activar la superficie táctil con el Dpad *(Default: L1 + R1 + Dpad Up + Not Used)*.
* `Buttons By Position:` Checkbox *"Use buttons by position instead of by label"* *(Default: Unchecked)* (crucial para mandos de Nintendo Switch / Xbox con disposición invertida A/B y X/Y).
* `Rumble Haptics:` Dropdown:
  - `Off`
  - `Very Weak`
  - `Weak`
  - `Normal` *(Default)*
  - `Strong`
  - `Very Strong`
* `True Haptics Intensity:` Slider *(Default: console setting)*.

#### 1.5.9 Pestaña `Config` (`100625.png`)
* Indicador de perfil: `Current Profile: default`.
* Botón: `Manage Profiles` (creación de perfiles independientes para distintas resoluciones, redes o usuarios).
* Botón: `Login` (gestión de cuenta PSN).
* Botón: `Clear Saved Game Data` (limpia cachés de carátulas e información de catálogo).
* Botón: `Export settings to file` (copia de seguridad de configuración en JSON/INI).
* Botón: `Import settings from file` (restauración de configuración externa).
* Botón: `Console Setup Guide` (reabre el asistente de 5 pasos).
* Botón: `Support Pylux`
* Botón: `About` (información de versión y licencias).
* `Verbose Logging:` Checkbox *(Default: Unchecked)*.

#### 1.5.10 Pestaña `Cloud` (Ajustes de PS Cloud / Apollo) (`100635.png` - `100711.png`)
* `Settings for:` Dropdown: `Game Library` | `Game Catalog`.
* `Resolution:` Dropdown:
  - `720p`
  - `1080p` *(Default)*
  - `1440p`
  - `2160p` (4K UHD)
* `Datacenter:` Dropdown: `Auto` *(Default)*.
* `Bitrate:` Slider *(Default: 20 Mbps)*.

---

### 1.6 Módulo de Cloud Play en PXPlay (`091416.png`, `091503.png`)
* **Header:** Pestaña `Cloud Play` seleccionada en el menú superior principal.
* **Sub-navegación:** Píldoras conmutables: `Game Catalog` | `Game Library`.
* **Filtros y Contadores:**
  - Filtro: `All`
  - Botón: `Refresh`
  - Contador: `4102 games` (en el catálogo unificado)
* **Gestión de Sesión / Advertencias:**
  - Banner rojo/amarillo destacado si no hay autenticación válida:
    *"NPSSO token is required for PS5 cloud play. Please login to PSN and enter a valid NPSSO token. You also need a valid PS Plus subscription. Owned games cannot be identified."*
* **Cuadrícula de Juegos (Grid 9x3):**
  - Cada tarjeta contiene:
    - Estrella en esquina superior izquierda para marcar como favorito.
    - Badge naranja en esquina superior derecha: `NOT OWNED` (o estado de suscripción).
    - Arte de carátula oficial de Sony PSN.
    - Franja inferior integrada en la imagen con el título recortado y una píldora con el número `5` (PS5) o `4` (PS4).
    - Botón de acción: `Add Game` o `Stream`.
  - Tarjetas ofuscadas: Tarjeta especial con título `[REDACTED]` y arte de Spider-Man modificado para evitar disputas de licencia directa.

---

## 2. Inventario Exhaustivo de Ludelo (Build Actual `bec3798b`)

Ludelo cuenta con un sistema de diseño propio muy pulido estéticamente (`LudeloTheme.qml`, paleta índigo `#0B0E14`/`#151923`/`#6C5CE7` con acentos en verde menta `#00F5D4`, fuentes Inter y JetBrains Mono). Sin embargo, una auditoría técnica visual píxel a píxel sobre las 19 capturas del build actual revela **defectos bloqueantes de composición, bindings rotos y omisión de datos**.

A continuación se detalla el estado funcional y los fallos exactos detectados en cada pantalla:

### 2.1 Setup Guide (1/5 a 5/5) (`105809.png` - `105900.png`)
* **Estado:** **Funcional y visualmente correcto**.
* **Contenido:** 5 pasos con diseño Ludelo (`LCard`, píldoras de paso, botones `BACK` y `NEXT`, botón `FINISH SETUP`).
* **Defectos detectados:**
  - Ningún solape ni fallo de renderizado.
  - En el paso 5 (Steam Deck) se mencionan glifos PlayStation en el texto descriptivo, aunque los botones del diagrama son neutros.

### 2.2 Onboarding (`105913.png`)
* **Estado:** **Funcional**.
* **Contenido:** Tarjeta flotante con glow índigo, botón primario `[A] SIGN IN WITH PLAYSTATION`, enlace `Skip For Now`, badge `Encrypted Local Storage (DPAPI)`.
* **Defectos detectados:**
  - Ninguno visible. La ventana renderiza limpia a 1080p.

### 2.3 MainView / Home de Consolas (`105934.png`, `110041.png`, `110053.png`)
* **Estado:** **CRÍTICAMENTE ROTO EN TELEMETRÍA Y PRESENTACIÓN DE DATOS**.
* **Defectos visuales y de código detectados:**
  1. **Tarjeta de Consola Vacía (Causa raíz: Ámbito léxico de QML):** En `hostsView`, el delegado `Loader` referencia un `Component` externo (`consoleCardComponent`) definido a nivel de raíz en `MainView.qml`. Debido a esto, `modelData` no está en el ámbito léxico del componente hijo y evalúa a `undefined`. Como resultado:
     - El recuadro del icono de consola queda como un cuadro gris oscuro sin imagen.
     - El badge de estado queda como un anillo hueco sin texto ni color de estado.
     - El nombre de la consola (`modelData.name`) no se renderiza (espacio vacío).
     - La línea de red e IP (`modelData.address`) no se renderiza.
     - La tabla de especificaciones muestra las etiquetas `Power State:`, `Registration:` y `Active Title:`, pero los valores a su derecha están completamente en blanco.
     - El botón de acción inferior queda como un rectángulo morado uniforme sin ningún texto interior.
  2. **Footer con valor erróneo de Bitrate:** En la barra inferior se lee textualmente:  
     `BITRATE: nan Mbps • DECODER: AUTO`.  
     El valor `nan` (Not a Number) se genera por divisiones aritméticas sobre `Chiaki.settings.bitrateLocalPS5`, cuyo valor por defecto en `settings.cpp` es `0` (en chiaki original 0 representaba "automático").
  3. **Claims publicitarios en tarjeta HUD:** La tarjeta derecha muestra `Audio Buffer: 0 frames` (sin telemetría en frío) y el claim publicitario `Stream Engine: Ludelo Low-Latency Core` (violación expresa de la regla de no inventar nombres comerciales ni tecnología ficticia).
  4. **Filtros de plataforma:** Los botones `All (1)`, `PS5 (1)`, `PS4 (0)` contabilizan bien las consolas pero filtran tarjetas con los datos en blanco.

### 2.4 ManualHostDialog (`105951.png`)
* **Estado:** **Funcional**.
* **Contenido:** Diálogo modal limpio para introducir dirección IP manual y conmutar arquitectura PS5 / PS4.

### 2.5 CloudPlayView (`110112.png`)
* **Estado:** **GRAVEMENTE SOLAPADO EN FOOTER**.
* **Defectos detectados:**
  1. **Doble Footer Solapado (Bug Visual Crítico):** El footer interno de navegación de `CloudPlayView` (`[A] PLAY / STORE`, `[B] BACK`, `[X] FAVORITE`, `[Y] SEARCH`, `[START] SORT`) se renderiza exactamente en las mismas coordenadas Y que el footer global persistente de `Main.qml` (`[A] SELECT`, `[Y] WAKE`, `[X] DETAILS`, `[B] BACK`, `BITRATE: nan Mbps`, versión). Los textos de ambos footers quedan superpuestos, ilegibles y con artefactos visuales.
  2. **Banner de Re-autenticación Desalineado:** El botón `RE-AUTHENTICATE` queda pegado contra el borde derecho del banner sin padding de seguridad.
  3. **Hero Spotlight:** Título visible `[REDACTED]` y carátula atenuada. Botón funcional abriendo la tienda oficial externa de PlayStation mediante `Qt.openUrlExternally`.

### 2.6 AccountView (`110142.png`)
* **Estado:** **INCOHERENTE EN ESTADO Y CON TEXTOS CORTADOS**.
* **Defectos detectados:**
  1. **Incoherencia Grave de Estado:** El avatar y título principal indican `Guest Player (Not Connected)` y muestran un badge amarillo `OFFLINE`, pero inmediatamente a la derecha se muestra un chip verde menta que dice: `CONNECTED TO PSN • ENCRYPTED LOCAL STORAGE (DPAPI)`. El usuario no sabe si está conectado o no.
  2. **Account ID Vacío:** La fila `Account ID:` no muestra ningún identificador ofuscado.
  3. **Texto Cortado (Clipping):** El chip de plataforma `PlayStation 5` está cortado en el margen derecho (`PlayStation 5...` o borde de píldora truncado) por no tener `implicitWidth` elástico.

### 2.7 SettingsDialog (Ajustes) (`110235.png` - `110407.png`)
* **Estado:** **CRÍTICAMENTE ROTO EN CONTROLES `LToggle` Y DROPDOWNS**.
* **Defectos detectados:**
  1. **`LToggle.qml` SIN TEXTO NI DESCRIPCIÓN:** En las 6 pestañas de ajustes (`Video & Stream`, `Audio`, `Network`, `Controller`, `Account`, `General`), todos los toggles aparecen únicamente como el switch gráfico (pastilla de 46x24 px). No se visualiza ni una sola palabra del título ni de la descripción.  
     *Causa raíz identificada:* En `gui/src/qml/components/LToggle.qml`, no está definido `implicitWidth`. Al integrarse dentro de un `ColumnLayout` sin `Layout.fillWidth: true`, el ancho del control colapsa al ancho del indicador (46 px). Además, la propiedad `anchors.verticalCenter: parent.verticalCenter` dentro del `contentItem` genera un conflicto de geometría en Qt 6 que fuerza el ancho del bloque de texto a 0.
  2. **Slider de Bitrate a Cero (`0 Mbps`):** El slider muestra `0 Mbps (Standard)` con el cursor anclado a la izquierda. La clave `settings/bitrate_local_ps5` devuelve `0` en instalaciones limpias, provocando que la división `/ 1000` dé `0`, violando el rango visual del slider (`from: 5000, to: 30000`).
  3. **Dropdowns de Dispositivos de Audio Vacíos (`110310.png`):** Tanto `Audio Output Device` como `Microphone Input Device` están completamente en blanco. Al hacer clic no despliegan ningún dispositivo.  
     *Causa raíz identificada:* El método `QmlSettings::refreshAudioDevices()` nunca se invoca automáticamente en el arranque de la aplicación ni al abrir la vista de ajustes. Las listas `audio_in_devices` y `audio_out_devices` permanecen vacías.
  4. **Audio Buffer Size a 0 frames:** El slider de buffer de audio marca `0 frames`.
  5. **Panel lateral duplicado en Ajustes:** El bloque "Stream Session Diagnostics" aparece idéntico en las pestañas Network y Video mostrando marcas `--` en reposo.
  6. **Registered Consoles (0):** En la pestaña Account se reporta `Registered Consoles (0)` a pesar de que hay hardware PS5 detectado en la subred.
  7. **Falta de configuración para Remote Play por Internet:** En la pestaña Video & Stream solo se ofrece ajuste para el stream local (LAN), omitiendo la configuración de Remote que sí tiene PXPlay.

---

## 3. Matriz Comparativa de Funcionalidades: PXPlay vs Ludelo

| Categoría | Funcionalidad | PXPlay (Pylux) | Ludelo (Build Actual) | Estado Técnico en Ludelo |
| :--- | :--- | :--- | :--- | :--- |
| **Autenticación** | Login Web nativo embebido | No (usa browser externo / móvil) | Sí (Win32 WebView2 nativo) | **La tenemos (Superior)** |
| | Login manual por token NPSSO | Sí (enlace manual) | Sí (`PSNTokenDialog` fallback) | **La tenemos** |
| | Almacenamiento seguro DPAPI | No (texto plano / obfuscado) | Sí (DPAPI cifrado en Windows) | **La tenemos (Superior)** |
| | Login por código QR | Sí (`psstream.app`) | Retirado deliberadamente | **Descartado por diseño** |
| **Consolas / Red** | Descubrimiento local DDP | Sí (UDP broadcast) | Sí (`DiscoveryManager`) | **La tenemos** |
| | Tarjeta de consola con telemetría | Sí (Nombre, IP, Hide) | Sí (Diseño gamer, pero datos vacíos) | **Parcial (Bug de QML Scope)** |
| | Ocultar consolas de la red | Sí (botón `[X]` en tarjeta) | Sí (soportado en backend y dialog) | **La tenemos** |
| | Encendido remoto (Wake-on-LAN) | Sí | Sí (`Chiaki.wakeUpHost`) | **La tenemos** |
| | Vinculación PIN 8 dígitos | Sí (8 dígitos + firmware radio) | Sí (`RegistDialog` segmentado) | **La tenemos** |
| | Passcode de usuario (4 dígitos) | Sí | Sí (campo opcional en dialog) | **La tenemos** |
| | Búsqueda de Account ID pública | Sí (Lookup por PSN Online ID) | No (requiere login o base64 directo) | **Falta (P1)** |
| **Streaming Video** | Doble perfil Local vs Remote | Sí (Columnas separadas LAN/Internet) | No en UI (Backend sí lo soporta) | **Backend listo, falta UI (P1)** |
| | Resolución PS5 (1080p, 720p, 540p) | Sí | Sí (botones en Video & Stream) | **La tenemos** |
| | Resolución PS4 (1080p Pro, 720p, 540p, 360p) | Sí | Sí (botones en Video & Stream) | **La tenemos** |
| | Selección de FPS (30 / 60 fps) | Sí | Sí | **La tenemos** |
| | Bitrate configurable | Sí (Sliders independientes) | Sí en backend (Roto a 0 en UI) | **Parcial (Bug en UI P0)** |
| | Códecs (H.264, H.265 / HEVC) | Sí | Sí | **La tenemos** |
| | HDR10 (10-bit Rec.2020) | Sí (opción `H265 HDR` directa) | Sí (Toggle HDR validado para PS5) | **La tenemos** |
| | Decodificador Hardware (D3D11VA, Vulkan) | Sí (Auto, D3D11VA, Vulkan, None) | Sí (Combo enlazado a backend) | **La tenemos** |
| | Modos de ventana (Stretch, Zoom, Fullscreen) | Sí (6 opciones de ventana) | No en UI (Backend sí lo soporta) | **Backend listo, falta UI (P1)** |
| | Render Preset (Fast, Default, HQ, Custom) | Sí | Parcial (Backend listo, UI pendiente) | **Backend listo, falta UI (P1)** |
| **Color y Pantalla** | Calibración libplacebo Display Targets | Sí (Primaries, Transfer, Peak, Contrast) | No en UI (Backend `settings.h` lo tiene) | **Backend listo, falta UI (P1)** |
| | Ajuste OLED (Contraste Infinito) | Sí | Backend listo (`displayTargetContrast`) | **Backend listo, falta UI (P1)** |
| | Peak Nits numérico para HDR | Sí | Backend listo (`displayTargetPeak`) | **Backend listo, falta UI (P1)** |
| | Escalador FSR (FidelityFX) | No explícito (usa EwaLanczos libplacebo) | No en UI | **[VERIFICAR BACKEND] (P2)** |
| **Audio y Micrófono** | Dispositivos de Salida (Speakers) | Sí (Dropdown con enumeración SO) | En blanco (Falta llamar a refresh) | **Parcial (Bug C++ P0)** |
| | Dispositivos de Entrada (Micrófono) | Sí (Dropdown con enumeración SO) | En blanco (Falta llamar a refresh) | **Parcial (Bug C++ P0)** |
| | Buffer de Audio dinámico (ms) | Sí (Slider 10-200 ms) | Slider a 0 (Falta mapping) | **Parcial (Bug UI P0)** |
| | Reducción de ruido / Eco (Speex) | Sí (Checkbox) | Backend compilado (`CHIAKI_GUI_ENABLE_SPEEX`) | **Backend listo, falta UI (P1)** |
| | Iniciar micrófono desmuteado | Sí | Sí en backend (`startMicUnmuted`) | **Backend listo, falta UI (P1)** |
| **Controladores** | Mapeo interactivo por hardware | Sí (Captura botón por botón de mando) | No en UI moderna (Backend sí lo tiene) | **Backend listo, falta UI (P1)** |
| | Soporte háptico DualSense (PC) | Sí | Sí (`enable_dualsense`, SDL audio sink) | **La tenemos** |
| | Intensidad de vibración (Rumble) | Sí (Off, Weak, Normal, Strong...) | Slider genérico en UI | **La tenemos** |
| | Mapeo completo de teclado/ratón | Sí (Pestaña `Keys` con 3 columnas) | Parcial en C++ (Falta editor visual) | **Falta UI (P1)** |
| | Emulación Touchpad con D-Pad | Sí (Combo modificador + D-pad) | Sí en backend (`dpadTouchEnabled`) | **Backend listo, falta UI (P1)** |
| | Botones por posición (Switch/Xbox layout) | Sí (Checkbox) | Sí en backend (`buttonsByPosition`) | **Backend listo, falta UI (P1)** |
| | Entradas en segundo plano (Background input) | Sí (Checkbox) | Sí en backend (`allowJoystickBackgroundEvents`) | **Backend listo, falta UI (P1)** |
| **Cloud Play** | Catálogo unificado PS5 Cloud + PS Now | Sí (4102 juegos con carátula) | Sí (Catálogo Apollo integrado) | **La tenemos** |
| | Selector de Datacenter y Ping | Sí (Dropdown con ping automático) | Parcial en C++ (JSON datacenters) | **Parcial (P1)** |
| | Resolución Cloud hasta 4K (2160p) | Sí (720p a 2160p) | Backend listo (`cloudResolutionPSCloud`) | **Backend listo, falta UI (P1)** |
| | Bitrate Cloud dedicado | Sí (Slider 20 Mbps) | Backend listo (`cloudBitratePSCloud`) | **Backend listo, falta UI (P1)** |
| | Doble footer solapado | No | Sí (Bug crítico en pantalla Cloud) | **Bug UI Ludelo (P0)** |
| **Gestión y Sistema** | Sistema de Perfiles múltiples | Sí (`Manage Profiles`) | Backend listo (`profiles` en settings) | **Backend listo, falta UI (P1)** |
| | Importar / Exportar configuración | Sí (Archivos externos) | Backend listo (`ExportSettings`) | **Backend listo, falta UI (P1)** |
| | Acción al desconectar (Ask/Sleep) | Sí | Backend listo (`disconnectAction`) | **Backend listo, falta UI (P1)** |
| | Auto-ocultar cursor de ratón | Sí | Sí (Implementado en `QmlMainWindow`) | **La tenemos** |
| | HUD en juego con telemetría RTT/Loss | Sí (`[TAB]` overlay) | Sí (Overlay cinemático conmutado) | **La tenemos** |
| | Atajos de mando estilo Xbox en UI | No (usa texto genérico y símbolos) | Sí (`[A]`, `[B]`, `[X]`, `[Y]`, `[LB]`, `[RB]`) | **La tenemos (Superior)** |
| | Overlay táctil en pantalla | Sí (Móviles / Tablets) | No (Exclusivo escritorio / handheld) | **No aplica a PC** |

---

## 4. Hallazgos Clave

### A. Lo que PXPlay tiene y nosotros ya soportamos en backend pero la UI no expone
Una inspección profunda de `gui/include/settings.h` y `gui/include/qmlsettings.h` revela que el backend de Ludelo (heredado de chiaki-ng/Pylux) **ya implementa en C++ el 90% de las opciones avanzadas de PXPlay**, pero nuestra interfaz QML actual no las tiene enlazadas o las oculta:
1. **Doble columna Local vs Remote:** En C++ existen `resolutionRemotePS5`, `fpsRemotePS5`, `bitrateRemotePS5`, `codecRemotePS5`, `resolutionRemotePS4`, etc., pero `SettingsDialog.qml` solo dibuja los controles locales.
2. **Display Settings de libplacebo:** En C++ existen `displayTargetPrim`, `displayTargetTrc`, `displayTargetPeak`, `displayTargetContrast` y presets de upscaler/downscaler. Solo falta el diálogo QML para seleccionarlos.
3. **Acciones de desconexión y suspensión:** En C++ existen `disconnectAction` (`Ask`, `AlwaysNothing`, `AlwaysSleep`) y `suspendAction`.
4. **Perfiles de configuración:** En C++ existen `profiles`, `currentProfile`, `LoadProfiles()`, `SaveProfiles()`, `ExportSettings()` e `ImportSettings()`.
5. **Ajustes de Cloud Play:** En C++ existen `cloudResolutionPSCloud` (hasta 4K), `cloudBitratePSCloud`, `cloudDatacenterPSCloud` y `cloudDatacentersJsonPSCloud`.
6. **Mapeo de mandos y teclado:** En C++ existen las tablas completas de remapeo SDL y atajos de touchpad.

### B. Lo que está ROTO hoy en Ludelo según las capturas
1. **`LToggle.qml` ciego:** No renderiza texto ni descripción en ninguna pantalla. Causa: anclajes conflictivos en `contentItem` y falta de `implicitWidth`.
2. **Tarjeta de consola en blanco:** Causa: ámbito léxico de QML. `consoleCardComponent` está fuera del `Loader` y busca `modelData` en vez de recibir los datos inyectados, dejando todos los textos en blanco.
3. **Footer con `nan Mbps`:** Causado por división aritmética sobre bitrate 0 en instalaciones limpias.
4. **Dispositivos de audio en blanco:** Las listas de dispositivos de entrada/salida nunca se refrescan al abrir ajustes.
5. **Doble footer superpuesto en Cloud Play:** El footer interno de `CloudPlayView` colisiona directamente sobre la barra de estado de `Main.qml`.
6. **Incoherencia en AccountView:** Se muestra simultáneamente "Guest Player OFFLINE" y "CONNECTED TO PSN". Chip de PS5 cortado en el margen derecho.
7. **Claims falsos:** Presencia de "Stream Engine: Ludelo Low-Latency Core" en la tarjeta HUD.

---

## 5. Plan de Acción Priorizado

### Fase P0: Corrección Inmediata de Defectos Visuales y de Datos (Bugs Bloqueantes)
* **P0.1 · Corregir Scoping en Tarjeta de Consola (`MainView.qml`):**
  - Mover `consoleCardComponent` para que reciba directamente el objeto `hostData` del `Loader` o definir el delegate de forma inline para que `modelData` resuelva correctamente.
  - Asegurar que se muestre el nombre real de la consola, la IP, el estado de energía, el título activo y el texto del botón de conexión.
* **P0.2 · Reparar `LToggle.qml` (Visibilidad de Textos y Descripciones):**
  - Añadir `Layout.fillWidth: true` e `implicitWidth: control.availableWidth`.
  - Eliminar `anchors.verticalCenter` y anclajes directos dentro de `contentItem: Column`.
  - Garantizar que los títulos y descripciones sean 100% legibles en todas las pestañas de ajustes.
* **P0.3 · Reparar `LButton.qml` (Anclajes de Contenido):**
  - Eliminar `anchors.centerIn: parent` de `contentItem: Row` y dejar que el layout interno de Qt Quick Controls centre el contenido sin generar warnings de geometría.
* **P0.4 · Resolver Doble Footer en `CloudPlayView.qml`:**
  - Ocultar la barra inferior global de `Main.qml` cuando la pestaña activa sea `Cloud Play`, o suprimir el footer duplicado de `CloudPlayView` unificando los atajos de gamepad en la barra inferior persistente.
* **P0.5 · Corregir Bitrate 0 Mbps y `nan Mbps` en Footer:**
  - Establecer valores por defecto saludables en `settings.cpp` (ej. 15000 kbps para PS5 Local, 10000 kbps para Remote y PS4).
  - Proteger las funciones de formateo en QML contra valores `0`, `undefined` o `NaN`.
* **P0.6 · Inicializar Dispositivos de Audio:**
  - Invocar `Chiaki.settings.refreshAudioDevices()` en la carga de la aplicación y al abrir la pestaña `Audio` en `SettingsDialog.qml`.
  - Conectar el slider de Audio Buffer al valor real del backend (`Chiaki.settings.audioBufferSize`).
* **P0.7 · Resolver Incoherencia de Estado en `AccountView.qml`:**
  - Sincronizar el avatar y texto de cabecera con el estado real de autenticación. Si el token está activo, mostrar el Online ID real en lugar de "Guest Player".
  - Ajustar el ancho del chip "PlayStation 5" para evitar cortes de texto en el margen derecho.
* **P0.8 · Erradicación de Claims Falsos:**
  - Eliminar el string "Stream Engine: Ludelo Low-Latency Core" de la tarjeta de aceleración hardware y reemplazarlo por datos reales verificables (ej. decodificador activo: D3D11VA).

---

### Fase P1: Integración de Funcionalidades de PXPlay con Backend Existente
* **P1.1 · Doble Columna Local vs Remote en Ajustes de Stream:**
  - Rediseñar la pestaña `Video & Stream` para ofrecer dos columnas paralelas: ajustes para red local (LAN) y ajustes para juego remoto fuera de casa (Internet), enlazados a `resolutionRemotePS5`, `fpsRemotePS5`, `bitrateRemotePS5` y `codecRemotePS5`.
* **P1.2 · Diálogo Display Settings Avanzado (libplacebo):**
  - Implementar el diálogo modal "Advanced Renderer & Display Settings" con los selectores de `Target Primaries` (Auto, BT.709, BT.2020, DCI-P3), `Target Transfer` (sRGB, BT.1886, PQ, HLG, Gamma 2.2), `Target Peak` (nits numéricos) y `Target Contrast` (Auto, Infinity para OLED).
* **P1.3 · Selector de Modos de Ventana:**
  - Añadir en la pestaña Video el selector de tipo de ventana enlazado a `Chiaki.settings.windowType` (`Selected Resolution`, `Fullscreen`, `Zoom`, `Stretch`).
* **P1.4 · Acciones de Desconexión y Suspensión:**
  - Añadir dropdowns en la pestaña General para `Action on Disconnect` (`Ask`, `Do Nothing`, `Enter Sleep Mode`) y `Action on Suspend`.
* **P1.5 · Selector y Gestor de Perfiles:**
  - Habilitar en la UI la selección y creación de perfiles (`default`, `SteamDeck`, `4K_TV`, `Mobile_Hotspot`) enlazados a `Chiaki.settings.profiles`.
* **P1.6 · Controles de Red y Pérdida de Paquetes:**
  - Enlazar sliders para notificación de Wi-Fi débil (`wifiDroppedNotif`) y límite de paquetes perdidos reportados (`packetLossMax`).
* **P1.7 · Pestaña de Ajustes Cloud Dedicada:**
  - Incorporar en Ajustes la configuración de resolución de Cloud (720p hasta 4K 2160p) y bitrate específico de Cloud Stream.

---

### Fase P2: Mejoras a Medio Plazo
* **P2.1 · Mapeo Visual e Interactivo de Mandos:**
  - Pantalla interactiva que detecte la pulsación de botones físicos del gamepad y permita reasignar entradas con slots primario y secundario.
* **P2.2 · Búsqueda de Account ID por Online ID (`By Lookup`):**
  - Implementar consulta pública segura para resolver el Account ID de un usuario a partir de su ID de PSN sin requerir OAuth web.
* **P2.3 · Escaladores Avanzados (FSR / EwaLanczos) `[VERIFICAR BACKEND]`:**
  - Verificar si el pipeline Vulkan de libplacebo admite FSR 1.0 integrado o si se cubre mediante los filtros `ewa_lanczos4sharpest` ya compilados en libplacebo.

---

### Descartes Explícitos (No Aplica a Versión PC)
* **Overlay táctil en pantalla:** No procede en versión de escritorio ni en PC Handhelds (Steam Deck / ROG Ally) que ya cuentan con sticks y cruceta físicos.
* **Picture-in-Picture (PiP) móvil:** Exclusivo de sistemas operativos móviles (Android/iOS).
