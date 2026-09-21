import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import Ludelo 1.0
import org.streetpea.chiaking
import "components"

Item {
    id: onboardingRoot

    signal loginCompleted(string accountId)
    signal skipRequested()

    anchors.fill: parent
    focus: true

    // Living Ambient Indigo & Cyan Atmosphere
    Rectangle {
        id: bgVoid
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#07090E" }
            GradientStop { position: 0.45; color: "#0F131D" }
            GradientStop { position: 1.0; color: "#0B0E14" }
        }
        z: -10

        // Central Atmospheric Radial Indigo Glow (Breathing Animation)
        Rectangle {
            id: centerGlow
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.75, 960)
            height: Math.min(parent.height * 0.75, 800)
            radius: width / 2
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.22)
            z: 1

            SequentialAnimation on scale {
                loops: Animation.Infinite
                NumberAnimation { from: 1.0; to: 1.14; duration: 6500; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.14; to: 1.0; duration: 6500; easing.type: Easing.InOutSine }
            }

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.16; to: 0.28; duration: 6500; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.28; to: 0.16; duration: 6500; easing.type: Easing.InOutSine }
            }
        }

        // Top-Right Subtle Cyan Aura (Breathing Animation)
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: -80
            anchors.rightMargin: -80
            width: 520
            height: 520
            radius: 260
            color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.06)
            z: 1

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.03; to: 0.07; duration: 8000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.07; to: 0.03; duration: 8000; easing.type: Easing.InOutSine }
            }
        }

        // Bottom-Left Deep Violet Aura (Breathing Animation)
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.bottomMargin: -60
            anchors.leftMargin: -60
            width: 480
            height: 480
            radius: 240
            color: Qt.rgba(0x48/255, 0x34/255, 0xD4/255, 0.08)
            z: 1

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.04; to: 0.10; duration: 7500; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.10; to: 0.04; duration: 7500; easing.type: Easing.InOutSine }
            }
        }
    }

    // Top Bar (Telemetry pills hidden in Onboarding)
    LTopBar {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        showTelemetry: false
        onSettingsClicked: {
            if (typeof root !== "undefined" && root.showSettingsDialog) {
                root.showSettingsDialog();
            }
        }
        onMinimizeClicked: {
            if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.showMinimized();
        }
        onMaximizeClicked: {
            if (typeof Chiaki !== "undefined" && Chiaki.window) {
                if (typeof Chiaki.window.toggleMaximize === "function")
                    Chiaki.window.toggleMaximize();
                else if (Chiaki.window.visibility === Window.Maximized)
                    Chiaki.window.showNormal();
                else
                    Chiaki.window.showMaximized();
            }
        }
        onCloseClicked: {
            if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.close();
        }
    }

    // Center Hero Onboarding Card
    Item {
        id: cardWrapper
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -8
        width: Math.min(parent.width - 48, 640)
        height: Math.min(parent.height - topBar.height - footerHUD.height - 40, 620)

        // Outer Glow Aura
        Rectangle {
            anchors.fill: parent
            anchors.margins: -14
            radius: LudeloTheme.radiusDialog + 14
            color: LudeloTheme.accentGlow
            opacity: 0.60
            z: -1
        }

        // Frosted Glass Card Surface
        Rectangle {
            id: heroCard
            anchors.fill: parent
            radius: LudeloTheme.radiusDialog
            color: LudeloTheme.bgPanel
            border.color: LudeloTheme.borderHover
            border.width: 1

            // Top-edge light refraction line
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 2
                height: 1
                radius: LudeloTheme.radiusDialog
                color: Qt.rgba(1.0, 1.0, 1.0, 0.20)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 36
                spacing: 16

                // High-Presence Brand Logo Emblem with Glowing Halo
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 76

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -8
                        radius: 24
                        color: LudeloTheme.accentGlow
                        opacity: 0.70
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 18
                        color: LudeloTheme.bgElevated
                        border.color: LudeloTheme.accentPrimary
                        border.width: 1.5

                        Image {
                            anchors.centerIn: parent
                            width: 46
                            height: 46
                            source: "qrc:/icons/ludelo_logo.svg"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }
                }

                // Title
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Welcome to Ludelo")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 32
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                    font.letterSpacing: -0.5
                }

                // Subtitle
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width - 40, 480)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Connect your console for high-performance direct remote play anywhere. Sign in securely via official PlayStation Network authentication.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 14
                    lineHeight: 1.4
                    color: LudeloTheme.textSecondary
                }

                Item { Layout.preferredHeight: 4 } // Small spacer

                // Trust & Security Badges Row
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    LPill {
                        text: qsTr("Official Token Auth")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }

                    LPill {
                        text: qsTr("Encrypted Local Storage (DPAPI)")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }

                    LPill {
                        text: qsTr("Direct P2P Encrypted")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }
                }

                Item { Layout.preferredHeight: 8 } // Spacer

                // Primary CTA Button
                LButton {
                    id: signInButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width - 40, 480)
                    Layout.preferredHeight: 52
                    variant: "primary"
                    text: qsTr("Sign in with PlayStation Network")
                    keyHint: LudeloTheme.hint("select")
                    glowEnabled: true

                    onClicked: {
                        if (Qt.platform.os === "windows") {
                            Chiaki.startWebView2Login();
                        } else {
                            if (typeof root !== "undefined" && root.showPSNTokenDialog) {
                                root.showPSNTokenDialog("", false);
                            }
                        }
                    }
                }

                // Ghost Secondary Button (Skip for now)
                LButton {
                    id: skipButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width - 40, 480)
                    Layout.preferredHeight: 42
                    variant: "ghost"
                    text: qsTr("Skip for now (Local Network Scan)")

                    onClicked: {
                        onboardingRoot.skipRequested();
                    }
                }

                Item { Layout.fillHeight: true } // Bottom pusher

                // Security & Independence Disclaimer Micro-copy
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(parent.width - 40, 480)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Protected by AES-256 stream encryption. Ludelo is an independent remote client.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 11
                    color: LudeloTheme.textDim
                }
            }
        }
    }

    // Contextual Footer HUD (Dynamic hints: [A] SELECT / [B] SKIP vs ENTER SELECT / ESC SKIP)
    Rectangle {
        id: footerHUD
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.95)
        border.color: LudeloTheme.borderSubtle
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24

            // Left: Dynamic Contextual Navigation Prompts
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 16

                MouseArea {
                    width: selectHintRow.implicitWidth
                    height: 24
                    cursorShape: Qt.PointingHandCursor
                    onClicked: signInButton.clicked()

                    Row {
                        id: selectHintRow
                        spacing: 6
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            width: Math.max(20, selectKeyText.implicitWidth + 8)
                            height: 20
                            radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                            border.color: LudeloTheme.accentMint
                            border.width: 1
                            Text {
                                id: selectKeyText
                                anchors.centerIn: parent
                                text: LudeloTheme.hintKey("select")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("SELECT")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                        }
                    }
                }

                MouseArea {
                    width: skipHintRow.implicitWidth
                    height: 24
                    cursorShape: Qt.PointingHandCursor
                    onClicked: onboardingRoot.skipRequested()

                    Row {
                        id: skipHintRow
                        spacing: 6
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            width: Math.max(20, skipKeyText.implicitWidth + 8)
                            height: 20
                            radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                            border.color: LudeloTheme.borderSubtle
                            border.width: 1
                            Text {
                                id: skipKeyText
                                anchors.centerIn: parent
                                text: LudeloTheme.hintKey("skip")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                color: LudeloTheme.textSecondary
                            }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("SKIP")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            color: LudeloTheme.textSecondary
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true } // Spacer

            // Right: Subtle Client Metadata (No fake telemetry pills)
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                Text {
                    text: qsTr("Ludelo Remote Play Client v2.4 • Direct P2P Protocol")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 11
                    color: LudeloTheme.textDim
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // Connect to Chiaki Login Signals
    Connections {
        target: Chiaki
        function onPsnLoginAccountIdDone(accountId) {
            if (accountId && accountId.length > 0) {
                onboardingRoot.loginCompleted(accountId);
            }
        }
        function onPsnLoginAccountIdError(error) {
            if (typeof root !== "undefined" && root.showToast) {
                root.showToast(qsTr("Login Notice"), error, "#F44336");
            }
        }
    }

    // Direct Shortcuts for Dead-Proof Key Support
    Shortcut { sequence: "Return"; onActivated: signInButton.clicked() }
    Shortcut { sequence: "Enter"; onActivated: signInButton.clicked() }
    Shortcut { sequence: "Space"; onActivated: signInButton.clicked() }
    Shortcut { sequence: "A"; onActivated: signInButton.clicked() }
    Shortcut { sequence: "Esc"; onActivated: onboardingRoot.skipRequested() }
    Shortcut { sequence: "Escape"; onActivated: onboardingRoot.skipRequested() }
    Shortcut { sequence: "B"; onActivated: onboardingRoot.skipRequested() }
    Shortcut { sequence: "Back"; onActivated: onboardingRoot.skipRequested() }

    Component.onCompleted: {
        onboardingRoot.forceActiveFocus();
    }

    // Gamepad / Keyboard Navigation Handling fallback
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_A) {
            signInButton.clicked();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back || event.key === Qt.Key_B || event.key === Qt.Key_Backspace) {
            onboardingRoot.skipRequested();
            event.accepted = true;
        }
    }
}
