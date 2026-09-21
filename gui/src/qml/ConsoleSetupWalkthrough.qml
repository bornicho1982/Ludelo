import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Ludelo 1.0
import org.streetpea.chiaking
import "components"

Item {
    id: walkthroughRoot

    property int currentStep: 0
    readonly property int totalSteps: 5

    anchors.fill: parent
    focus: true

    function close() {
        if (typeof root !== "undefined" && root && root.closeDialog) {
            root.closeDialog();
        } else if (typeof stack !== "undefined" && stack) {
            stack.pop();
        }
    }

    function nextStep() {
        if (currentStep < totalSteps - 1) {
            currentStep++;
        } else {
            close();
        }
    }

    function previousStep() {
        if (currentStep > 0) {
            currentStep--;
        }
    }

    // Atmospheric dark gamer background
    Rectangle {
        id: bgVoid
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#07090E" }
            GradientStop { position: 0.5; color: "#0F131D" }
            GradientStop { position: 1.0; color: "#0B0E14" }
        }
        z: -10

        // Subtle ambient radial glow
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.8, 1000)
            height: Math.min(parent.height * 0.8, 800)
            radius: width / 2
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.15)
            z: 1
        }
    }

    // Top Navigation Header
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.92)
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        z: 20

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            spacing: 16

            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 12

                Rectangle {
                    width: 34; height: 34; radius: 8
                    color: LudeloTheme.bgElevated
                    border.color: LudeloTheme.accentPrimary
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    Image {
                        anchors.centerIn: parent
                        width: 20; height: 20
                        source: "qrc:/icons/ludelo_logo.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        text: qsTr("GUÍA DE CONFIGURACIÓN LUDELO")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        color: LudeloTheme.textPrimary
                        font.letterSpacing: 1.2
                    }
                    Text {
                        text: qsTr("PASO %1 DE %2").arg(walkthroughRoot.currentStep + 1).arg(walkthroughRoot.totalSteps)
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 10
                        color: LudeloTheme.accentMint
                        font.weight: Font.Bold
                    }
                }
            }

            Item { Layout.fillWidth: true } // Spacer

            // Step Indicator Dots
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                Repeater {
                    model: walkthroughRoot.totalSteps
                    Rectangle {
                        width: index === walkthroughRoot.currentStep ? 32 : 8
                        height: 8
                        radius: 4
                        color: {
                            if (index === walkthroughRoot.currentStep) return LudeloTheme.accentMint;
                            if (index < walkthroughRoot.currentStep) return LudeloTheme.accentPrimary;
                            return Qt.rgba(1.0, 1.0, 1.0, 0.15);
                        }

                        Behavior on width {
                            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                        }
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: walkthroughRoot.currentStep = index
                        }
                    }
                }
            }

            // Close Action
            LButton {
                height: 36
                customRadius: 8
                variant: "ghost"
                text: qsTr("CERRAR")
                keyHint: "[ESC]"
                onClicked: walkthroughRoot.close()
            }
        }
    }

    // Main Interactive Content Area
    Item {
        id: contentContainer
        anchors.top: headerBar.bottom
        anchors.bottom: bottomBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 24

        // Central Elevated Step Card
        LCard {
            id: mainCard
            anchors.centerIn: parent
            width: Math.min(parent.width - 32, 820)
            height: Math.min(parent.height - 32, 540)

            // Step 1: Conecta tu cuenta PSN
            ColumnLayout {
                visible: walkthroughRoot.currentStep === 0
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                RowLayout {
                    spacing: 12
                    LPill {
                        text: qsTr("PASO 1 • AUTENTICACIÓN OFICIAL")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }
                    LPill {
                        text: qsTr("CIFRADO DPAPI")
                        dotColor: LudeloTheme.accentPrimary
                        showDot: true
                    }
                }

                Text {
                    text: qsTr("Conecta tu cuenta de PlayStation Network")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: qsTr("Ludelo se comunica directamente con los servidores oficiales de Sony mediante autenticación OAuth segura (WebView2). No almacenamos tu contraseña ni enviamos tus credenciales a ningún servidor de terceros.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: LudeloTheme.borderSubtle
                }

                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("Almacenamiento Local Seguro (DPAPI)")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Tus tokens de sesión se encriptan exclusivamente para tu cuenta de usuario de Windows mediante CryptProtectData.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textDim
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("Detección Automática de Consolas")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentPrimary
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Al iniciar sesión, Ludelo obtiene tu Account ID necesario para enlazar tus consolas PS5 y PS4 en la red local.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textDim
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    spacing: 12
                    LButton {
                        height: 44
                        implicitWidth: 260
                        customRadius: 8
                        variant: "primary"
                        text: qsTr("INICIAR SESIÓN CON PSN")
                        keyHint: "[A]"
                        onClicked: {
                            if (Qt.platform.os === "windows") {
                                Chiaki.startWebView2Login();
                            } else {
                                if (typeof root !== "undefined" && root.showPSNTokenDialog)
                                    root.showPSNTokenDialog("", false);
                            }
                        }
                    }
                    Text {
                        text: (typeof Chiaki !== "undefined" && Chiaki.settings && Chiaki.settings.psnAccountId) ?
                            qsTr("✓ Sesión PSN activa: %1").arg(Chiaki.settings.psnAccountId) :
                            qsTr("O puedes continuar sin iniciar sesión y vincular por PIN.")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 12
                        color: (typeof Chiaki !== "undefined" && Chiaki.settings && Chiaki.settings.psnAccountId) ?
                            LudeloTheme.accentMint : LudeloTheme.textDim
                    }
                }
            }

            // Step 2: Activa Uso a distancia en tu consola
            ColumnLayout {
                visible: walkthroughRoot.currentStep === 1
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                RowLayout {
                    spacing: 12
                    LPill {
                        text: qsTr("PASO 2 • AJUSTES DE CONSOLA")
                        dotColor: LudeloTheme.accentPrimary
                        glowColor: LudeloTheme.accentGlow
                        showDot: true
                    }
                    LPill {
                        text: qsTr("PS5 & PS4")
                        dotColor: LudeloTheme.accentMint
                        showDot: true
                    }
                }

                Text {
                    text: qsTr("Activa el Uso a distancia en tu consola")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: qsTr("Para permitir que Ludelo reciba la señal de vídeo y envíe tus controles, debes habilitar la opción de Uso a distancia en el menú de tu consola.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: LudeloTheme.borderSubtle
                }

                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    // PS5 Instructions Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.accentPrimary
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8
                            Text {
                                text: qsTr("PlayStation 5")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("1. Ve a Ajustes > Sistema > Uso a distancia.\n2. Activa «Activar Uso a distancia».\n3. En Ahorro de energía > Funciones disponibles en modo de reposo, activa «Mantenerse conectado a Internet» y «Activar encendido de la PS5 desde la red».")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                lineHeight: 1.35
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    // PS4 Instructions Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8
                            Text {
                                text: qsTr("PlayStation 4")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("1. Ve a Ajustes > Ajustes de conexión del Uso a distancia.\n2. Marca «Activar el Uso a distancia».\n3. En Ajustes de ahorro de energía > Establecer funciones en modo de reposo, activa «Permanecer conectado a Internet» y «Habilitar encendido de PS4 desde la red».")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                lineHeight: 1.35
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // Step 3: Vincula tu consola con el PIN de 8 dígitos
            ColumnLayout {
                visible: walkthroughRoot.currentStep === 2
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                RowLayout {
                    spacing: 12
                    LPill {
                        text: qsTr("PASO 3 • VINCULACIÓN PIN")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }
                    LPill {
                        text: qsTr("TEMPORIZADOR 300S")
                        dotColor: LudeloTheme.accentPrimary
                        showDot: true
                    }
                }

                Text {
                    text: qsTr("Vincula tu consola con el PIN de 8 dígitos")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: qsTr("El primer emparejamiento requiere generar un código de seguridad PIN en la consola. Este código valida que tú eres el propietario físico de la PlayStation.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: LudeloTheme.borderSubtle
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            Text {
                                text: qsTr("1. En tu consola:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                text: qsTr("Ve a Ajustes > Sistema > Uso a distancia > Vincular dispositivo.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            Text {
                                text: qsTr("2. En Ludelo:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentPrimary
                            }
                            Text {
                                text: qsTr("Pulsa «PAIR CONSOLE» o «+ Add New Console» e introduce los 8 dígitos.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            Text {
                                text: qsTr("3. Passcode opcional:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.textDim
                            }
                            Text {
                                text: qsTr("Si tu usuario en la consola tiene clave de acceso numérico, ponla en el campo de 4 dígitos.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // Step 4: Tu primer stream
            ColumnLayout {
                visible: walkthroughRoot.currentStep === 3
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                RowLayout {
                    spacing: 12
                    LPill {
                        text: qsTr("PASO 4 • EN JUEGO & HUD")
                        dotColor: LudeloTheme.accentPrimary
                        glowColor: LudeloTheme.accentGlow
                        showDot: true
                    }
                    LPill {
                        text: qsTr("COMBO L1+R1+L3+R3")
                        dotColor: LudeloTheme.accentMint
                        showDot: true
                    }
                }

                Text {
                    text: qsTr("Tu primer stream: Wake-on-LAN y Dock en juego")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: qsTr("Una vez emparejada, puedes iniciar stream con un solo clic. Si la consola está en modo de reposo (Standby), Ludelo enviará un paquete Wake-on-LAN para encenderla automáticamente.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: LudeloTheme.borderSubtle
                }

                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("Dock Flotante en Juego")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Mueve el ratón o pulsa L1+R1+L3+R3 en el mando para mostrar el dock con [PS HOME], silencio [M], pantalla completa [F11] y desconexión rápida [ESC].")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("HUD de Diagnóstico [TAB]")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentPrimary
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Pulsa la tecla [TAB] para alternar la píldora de telemetría con FPS reales, resolución activa, tasa de bitrate dinámico, RTT/latencia y pérdida de paquetes.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // Step 5: Mando y Steam Deck
            ColumnLayout {
                visible: walkthroughRoot.currentStep === 4
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                RowLayout {
                    spacing: 12
                    LPill {
                        text: qsTr("PASO 5 • MANDOS & STEAM DECK")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }
                    LPill {
                        text: qsTr("DUALSENSE 1000HZ")
                        dotColor: LudeloTheme.accentPrimary
                        showDot: true
                    }
                }

                Text {
                    text: qsTr("Mando, DualSense y soporte para Steam Deck")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                    text: qsTr("Ludelo incluye integración SDL avanzada para soportar mandos de PlayStation, Xbox y configuraciones portátiles como Steam Deck y ROG Ally sin drivers adicionales.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: LudeloTheme.borderSubtle
                }

                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("DualSense Nativo por USB")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Conexión a 1000 Hz con soporte directo de gatillos adaptativos, vibración háptica avanzada y emulación del panel táctil con el ratón.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 8
                        color: Qt.rgba(0.04, 0.06, 0.10, 0.6)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Text {
                                text: qsTr("Steam Deck & Mandos Xbox")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.accentPrimary
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("Nomenclatura universal con botones [A] y [B] por posición física, sin confusión de glifos. En Steam Deck puedes crear el acceso directo para Gaming Mode.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    // Bottom Footer Navigation Controls
    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.95)
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        z: 20

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24

            LButton {
                height: 40
                customRadius: 8
                variant: "secondary"
                text: qsTr("ANTERIOR")
                keyHint: "[B]"
                enabled: walkthroughRoot.currentStep > 0
                onClicked: walkthroughRoot.previousStep()
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Paso %1 de %2").arg(walkthroughRoot.currentStep + 1).arg(walkthroughRoot.totalSteps)
                font.family: LudeloTheme.fontFamilyMono
                font.pixelSize: 12
                color: LudeloTheme.textDim
            }

            Item { Layout.fillWidth: true }

            LButton {
                height: 40
                customRadius: 8
                variant: walkthroughRoot.currentStep === walkthroughRoot.totalSteps - 1 ? "mint" : "primary"
                text: walkthroughRoot.currentStep === walkthroughRoot.totalSteps - 1 ? qsTr("FINALIZAR") : qsTr("SIGUIENTE")
                keyHint: "[A]"
                onClicked: walkthroughRoot.nextStep()
            }
        }
    }

    // Keyboard & Gamepad navigation
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            walkthroughRoot.close();
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Right) {
            walkthroughRoot.nextStep();
            event.accepted = true;
        } else if (event.key === Qt.Key_Left) {
            walkthroughRoot.previousStep();
            event.accepted = true;
        }
    }
}