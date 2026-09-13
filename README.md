# Ludelo

**Ludelo** es un cliente de PlayStation Remote Play de alto rendimiento para PC con Windows 11, diseñado para ofrecer una experiencia visual fluida, baja latencia y soporte completo de hardware para el mando DualSense.

> [!IMPORTANT]
> **Aviso Legal / Legal Disclaimer:**  
> **ES:** Ludelo es un proyecto de software independiente y de código abierto. No está afiliado, respaldado, asociado ni certificado de ninguna manera por Sony Interactive Entertainment Inc., PlayStation u otras empresas filiales. "PlayStation", "PS4", "PS5", "DualSense" y "PSN" son marcas registradas de sus respectivos propietarios.  
> **EN:** Ludelo is an independent open-source software project. It is not affiliated with, endorsed by, or in any way associated with Sony Interactive Entertainment Inc., PlayStation, or any of its subsidiaries. "PlayStation", "PS4", "PS5", "DualSense", and "PSN" are registered trademarks of their respective owners.

---

## ¿Por qué elegir Ludelo?

- 🎮 **Experiencia 10-Foot Console First:** Interfaz moderna e inmersiva navegable tanto con mando como con ratón, diseñada para pantallas de TV, monitores gaming y portátiles.
- ⚡ **Latencia Ultra Baja (Glass-to-Glass):** Decodificación acelerada por hardware (Vulkan 1.3 + FFmpeg) con soporte para H.265 (HEVC 10-bit HDR) y H.264 hasta 1080p a 60 FPS y 120 FPS.
- 🎯 **Soporte Nativo DualSense:** Acceso directo por USB a los motores hápticos de alta definición, resistencia dinámica en los gatillos adaptativos (L2/R2), sensor de movimiento (giroscopio de 6 ejes) y barra de luz LED personalizable.
- 🕹️ **Filtro Anti-Drift Configurable:** Zona muerta radial personalizable en sticks analógicos para eliminar cualquier deriva mecánica por desgaste.
- 📡 **Despertar en 1 Clic (Wake-on-LAN):** Enciende tu consola directamente desde el modo de reposo a través de la red local sin tocar físicamente el botón de encendido.
- 🔒 **Login Seguro y Transparente:** Inicio de sesión web oficial de PlayStation Network con WebView2 seguro (sin necesidad de buscar cookies `npsso` manualmente en las herramientas de desarrollador). Tus credenciales se cifran de forma estricta en tu equipo mediante Windows DPAPI.

---

## Guía de Inicio Rápido

### 1. Prepara tu consola
1. Ve a **Ajustes > Sistema > Uso a distancia** y activa **Activar Uso a distancia**.
2. Ve a **Ajustes > Sistema > Ahorro de energía > Funciones disponibles en modo de reposo** y activa:
   - *Permanecer conectado a Internet*
   - *Activar encendido de la PS5/PS4 desde la red*

### 2. Ejecuta Ludelo
- Descarga la versión oficial o ejecuta `bin\Ludelo.exe`.
- Conecta tu PC a la misma red local (cable Ethernet o WiFi de 5 GHz recomendado) que tu consola.

### 3. Inicia sesión
- Al abrir la aplicación por primera vez, pulsa **Iniciar sesión con PlayStation Network**.
- Se abrirá la ventana oficial y segura de Sony. Introduce tus datos y autoriza el acceso. Ludelo vinculará tu cuenta de forma automática y descargará tu avatar de perfil.

### 4. Vincula tu consola
- Ludelo detectará automáticamente las consolas de tu red local.
- Si es la primera vez que te conectas, pulsa **Vincular Consola**, acude a tu consola en **Ajustes > Sistema > Uso a distancia > Vincular dispositivo**, e introduce en Ludelo el PIN de 8 casillas mostrado en la pantalla de la TV.

### 5. ¡A jugar!
- Conecta tu mando (DualSense recomendado mediante cable USB para funciones hápticas y gatillos) y haz clic en **CONECTAR AHORA**.

---

## Preguntas Frecuentes (FAQ)

#### ¿Mis datos de cuenta y contraseñas están seguros?
**Sí, al 100%.** Ludelo nunca tiene acceso a tu contraseña. La autenticación se realiza directamente a través de los servidores oficiales de Sony mediante WebView2. Los tokens de autorización y las claves de conexión con tu consola se almacenan localmente cifrados mediante el sistema **Windows Data Protection API (DPAPI)**. Nunca se sube ningún dato a servidores externos.

#### ¿Qué mandos son compatibles?
- **Sony DualSense (PS5):** Soporte total de vibración háptica, gatillos adaptativos y giroscopio (se recomienda conexión USB para efectos hápticos avanzados).
- **Sony DualShock 4 (PS4):** Vibración y giroscopio compatibles.
- **Mandos Xbox, Nintendo Switch Pro y genéricos:** Compatibles mediante la base de datos SDL3 GameControllerDB integrada.

#### ¿Por qué mi consola no despierta del modo de reposo?
Verifica que la opción *Activar encendido de la PS5/PS4 desde la red* esté activada en los ajustes de ahorro de energía de tu consola, y que tanto el PC como la consola estén conectados al mismo router sin subredes o aislamiento de clientes WiFi activado.

#### ¿Puedo jugar fuera de casa a través de Internet?
Ludelo está diseñado para ultra baja latencia en red local (LAN). Si dispones de una conexión VPN a tu hogar (por ejemplo WireGuard o Tailscale) o reenvío de puertos UDP configurado en tu router, podrás conectarte remotamente a tu consola desde cualquier lugar.

---

## Para Desarrolladores (Compilación desde fuentes)

Ludelo está escrito en **C++23 moderno** y cuenta con un script maestro de compilación automatizado para Visual Studio 2022 (MSVC x64):

```bat
# Compilar la aplicación completa y desplegar assets
build.bat app

# Compilar y ejecutar la suite de 6 pruebas unitarias (100% verde)
build.bat tests

# Compilar la herramienta de diagnóstico de hardware
build.bat diag
```

---

## Licencia y Reconocimientos

- **Licencia principal:** [GNU Affero General Public License v3.0 or later (AGPL-3.0-or-later)](LICENSE).
- **Motor de protocolo:** Basado y derivado de las arquitecturas de ingeniería inversa de [Chiaki](https://github.com/thestr4ng3r/chiaki) y [chiaki-ng](https://github.com/streetpea/chiaki-ng). Consulta [NOTICE](NOTICE).
- **Bibliotecas de terceros:** Agradecemos el trabajo de los proyectos [SDL3](https://libsdl.org/), [FFmpeg](https://ffmpeg.org/), [Dear ImGui](https://github.com/ocornut/imgui), [OpenSSL](https://www.openssl.org/), [Inter Font](https://rsms.me/inter/) y [Lucide Icons](https://lucide.dev/). Consulta [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) para los textos íntegros de licencia.
