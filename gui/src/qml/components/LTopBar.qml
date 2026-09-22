import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.streetpea.chiaking
import Ludelo 1.0

Rectangle {
    id: topBarRoot

    property bool showTelemetry: true
    property bool showWindowControls: true
    property string dualSenseText: (typeof Chiaki !== "undefined" && Chiaki.controllers && Chiaki.controllers.length > 0) ?
        (Chiaki.controllers[0].dualSense ? qsTr("WIRELESS GAMEPAD • 1000Hz") : qsTr("GAMEPAD CONNECTED")) :
        qsTr("DIRECT P2P LAN")
    property string streamReadyText: qsTr("DIRECT P2P STREAM READY")
    signal settingsClicked()
    signal minimizeClicked()
    signal maximizeClicked()
    signal closeClicked()

    height: 60
    color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.90)
    border.color: LudeloTheme.borderSubtle
    border.width: 1

    // Background drag and double-click maximize area covering entire topbar
    MouseArea {
        id: bgDragArea
        anchors.fill: parent
        z: 10
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            console.log("[window] LTopBar bgDragArea onPressed triggered");
            if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                var ok = Chiaki.window.startDrag();
                console.log("[window] LTopBar startDrag returned:", ok);
            } else {
                console.warn("[window] Chiaki.window.startDrag not available in LTopBar");
            }
        }
        onDoubleClicked: (mouse) => {
            console.log("[window] LTopBar bgDragArea onDoubleClicked triggered");
            if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                Chiaki.window.toggleMaximize();
            }
        }
    }

    // Left visual items: Brand emblem, title and optional telemetry badges (under drag MouseArea)
    Row {
        id: leftVisuals
        z: 1
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.verticalCenter: parent.verticalCenter
        spacing: 16

        // Brand Emblem & Title
        Row {
            id: brandRow
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            // Hexagon Icon / Logo Emblem
            Rectangle {
                width: 34
                height: 34
                radius: 8
                color: LudeloTheme.bgElevated
                border.color: LudeloTheme.borderHover
                border.width: 1
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/icons/ludelo_logo.svg"
                    fillMode: Image.PreserveAspectFit
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                Text {
                    text: "LUDELO"
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                    font.letterSpacing: 1.5
                }

                Text {
                    text: "REMOTE PLAY CLIENT v2.4"
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 9
                    color: LudeloTheme.textDim
                    font.letterSpacing: 0.8
                }
            }
        }

        // Telemetry Badges
        Row {
            visible: topBarRoot.showTelemetry
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            LPill {
                text: topBarRoot.dualSenseText
                dotColor: LudeloTheme.accentMint
                glowColor: LudeloTheme.accentMintGlow
                showDot: true
                pulseDot: true
            }

            LPill {
                text: topBarRoot.streamReadyText
                dotColor: LudeloTheme.accentPrimary
                glowColor: LudeloTheme.accentGlow
                showDot: true
            }
        }
    }

    // Right Interactive Controls (Settings, Window Controls) with high z-order (above drag area)
    Row {
        id: rightControls
        z: 20
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // Settings Icon Button
        Rectangle {
            width: 36
            height: 36
            radius: 8
            color: settingsMouse.containsMouse ? LudeloTheme.bgElevated : "transparent"
            border.color: settingsMouse.containsMouse ? LudeloTheme.borderHover : "transparent"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Image {
                anchors.centerIn: parent
                width: 18
                height: 18
                source: "qrc:/icons/settings-20px.svg"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: settingsMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: topBarRoot.settingsClicked()
            }
        }

        // Window Controls (Minimize, Maximize, Close)
        Row {
            visible: topBarRoot.showWindowControls
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            // Minimize
            Rectangle {
                width: 32
                height: 32
                radius: 6
                color: minMouse.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "—"
                    font.pixelSize: 12
                    color: LudeloTheme.textSecondary
                }
                MouseArea {
                    id: minMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.showMinimized();
                    }
                }
            }

            // Maximize / Restore
            Rectangle {
                width: 32
                height: 32
                radius: 6
                color: maxMouse.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: (typeof Chiaki !== "undefined" && Chiaki.window && Chiaki.window.visibility === Window.Maximized) ? "❐" : "□"
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }
                MouseArea {
                    id: maxMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        console.log("[window] LTopBar maxButton clicked, visibility:", (typeof Chiaki !== "undefined" && Chiaki.window) ? Chiaki.window.visibility : "unknown");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                            Chiaki.window.toggleMaximize();
                        }
                    }
                }
            }

            // Close
            Rectangle {
                width: 32
                height: 32
                radius: 6
                color: closeMouse.containsMouse ? LudeloTheme.error : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 13
                    color: closeMouse.containsMouse ? "#FFFFFF" : LudeloTheme.textSecondary
                }
                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.close();
                    }
                }
            }
        }
    }
}
