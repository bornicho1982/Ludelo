import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Ludelo 1.0

Rectangle {
    id: topBarRoot

    property bool showTelemetry: true
    property bool showWindowControls: true
    property string dualSenseText: qsTr("DUALSENSE WIRELESS DETECTED • 0.8ms")
    property string streamReadyText: qsTr("STREAM READY • 4K HDR 60FPS")
    signal settingsClicked()
    signal minimizeClicked()
    signal maximizeClicked()
    signal closeClicked()

    height: 60
    color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.90)
    border.color: LudeloTheme.borderSubtle
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 16
        spacing: 16

        // Brand Emblem & Title
        Row {
            Layout.alignment: Qt.AlignVCenter
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
                    width: 20
                    height: 20
                    source: "qrc:/res/chiaki.svg"
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

        Item { Layout.fillWidth: true } // Spacer

        // Telemetry Badges
        Row {
            visible: topBarRoot.showTelemetry
            Layout.alignment: Qt.AlignVCenter
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

        // Settings Icon Button
        Rectangle {
            width: 36
            height: 36
            radius: 8
            color: settingsMouse.containsMouse ? LudeloTheme.bgElevated : "transparent"
            border.color: settingsMouse.containsMouse ? LudeloTheme.borderHover : "transparent"
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: 18
                height: 18
                source: "qrc:/res/settings-20px.svg"
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
            Layout.alignment: Qt.AlignVCenter
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
                    onClicked: topBarRoot.minimizeClicked()
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
                    text: "□"
                    font.pixelSize: 14
                    color: LudeloTheme.textSecondary
                }
                MouseArea {
                    id: maxMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: topBarRoot.maximizeClicked()
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
                    onClicked: topBarRoot.closeClicked()
                }
            }
        }
    }
}
