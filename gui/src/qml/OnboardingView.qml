import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import Ludelo 1.0
import "components"

Item {
    id: onboardingRoot

    signal loginCompleted(string accountId)
    signal skipRequested()

    anchors.fill: parent
    focus: true

    // Background Void #0B0E14
    Rectangle {
        id: bgVoid
        anchors.fill: parent
        color: LudeloTheme.bgBase
        z: -10

        // Central Atmospheric Radial Indigo Glow
        Rectangle {
            id: centerGlow
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.7, 900)
            height: Math.min(parent.height * 0.7, 750)
            radius: width / 2
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.12)
            z: 1
        }

        // Corner Subtle Mint Cyan Aura
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 450
            height: 450
            radius: 225
            color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.04)
            z: 1
        }
    }

    // Top Bar
    LTopBar {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        onSettingsClicked: {
            if (typeof root !== "undefined" && root.showSettingsDialog) {
                root.showSettingsDialog();
            }
        }
        onMinimizeClicked: {
            if (Chiaki && Chiaki.window) Chiaki.window.showMinimized();
        }
        onMaximizeClicked: {
            if (Chiaki && Chiaki.window) {
                if (Chiaki.window.visibility === Window.Maximized)
                    Chiaki.window.showNormal();
                else
                    Chiaki.window.showMaximized();
            }
        }
        onCloseClicked: {
            if (Chiaki && Chiaki.window) Chiaki.window.close();
        }
    }

    // Center Hero Onboarding Card
    Item {
        id: cardWrapper
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -8
        width: Math.min(parent.width - 48, 640)
        height: Math.min(parent.height - topBar.height - footerHUD.height - 40, 600)

        // Outer Glow Aura
        Rectangle {
            anchors.fill: parent
            anchors.margins: -12
            radius: LudeloTheme.radiusDialog + 12
            color: LudeloTheme.accentGlow
            opacity: 0.55
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

                // Hexagonal Logo Emblem with Halo
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -6
                        radius: 18
                        color: LudeloTheme.accentGlow
                        opacity: 0.65
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 14
                        color: LudeloTheme.bgElevated
                        border.color: LudeloTheme.accentPrimary
                        border.width: 1.5

                        Image {
                            anchors.centerIn: parent
                            width: 32
                            height: 32
                            source: "qrc:/res/chiaki.svg"
                            fillMode: Image.PreserveAspectFit
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
                    text: qsTr("Connect your console for ultra-low latency remote play anywhere. Sign in securely via official PlayStation Network authentication.")
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
                    keyHint: "[X] ENTER"
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

    // Bottom Gamepad Telemetry & Navigation HUD Footer
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

            // Left: Controller Quick Navigation Prompts
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 16

                Row {
                    spacing: 6
                    Rectangle {
                        width: 20; height: 20; radius: 4
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                        border.color: LudeloTheme.accentMint
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("SELECT"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                }

                Row {
                    spacing: 6
                    Rectangle {
                        width: 20; height: 20; radius: 4
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                        Text { anchors.centerIn: parent; text: "○"; font.pixelSize: 11; font.weight: Font.Bold; color: LudeloTheme.textSecondary }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("BACK"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                }

                Row {
                    spacing: 6
                    Rectangle {
                        width: 20; height: 20; radius: 4
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                        Text { anchors.centerIn: parent; text: "△"; font.pixelSize: 11; font.weight: Font.Bold; color: LudeloTheme.textSecondary }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("MANUAL IP"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                }

                Row {
                    spacing: 6
                    Rectangle {
                        width: 52; height: 20; radius: 4
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                        Text { anchors.centerIn: parent; text: "OPTIONS"; font.pixelSize: 9; font.weight: Font.Bold; color: LudeloTheme.textSecondary }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: qsTr("SETTINGS"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                }
            }

            Item { Layout.fillWidth: true } // Spacer

            // Right: Streaming Engine Telemetry Beacon
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                Text {
                    text: qsTr("Ludelo Remote Play Client • Ultra Low Latency |")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 11
                    color: LudeloTheme.textDim
                    anchors.verticalCenter: parent.verticalCenter
                }

                LPill {
                    text: qsTr("VORTEX ENGINE ACTIVE")
                    dotColor: LudeloTheme.accentMint
                    glowColor: LudeloTheme.accentMintGlow
                    showDot: true
                    pulseDot: true
                }
            }
        }
    }

    // Connect to Chiaki Login Signal
    Connections {
        target: Chiaki
        function onPsnLoginAccountIdDone(accountId) {
            if (accountId && accountId.length > 0) {
                onboardingRoot.loginCompleted(accountId);
            }
        }
    }

    // Gamepad Key Navigation Handling
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            signInButton.clicked();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            onboardingRoot.skipRequested();
            event.accepted = true;
        }
    }
}
