import QtQuick
import Ludelo 1.0

Rectangle {
    id: pillRoot

    property string text: ""
    property string iconSource: ""
    property string status: ""
    property color dotColor: LudeloTheme.accentMint
    property color glowColor: LudeloTheme.accentMintGlow
    property bool showDot: true
    property bool pulseDot: false
    property color textColor: LudeloTheme.textPrimary

    onStatusChanged: {
        if (status === "online" || status === "success") {
            dotColor = LudeloTheme.accentMint;
            glowColor = LudeloTheme.accentMintGlow;
        } else if (status === "offline" || status === "dim") {
            dotColor = LudeloTheme.textDim;
            glowColor = "transparent";
        } else if (status === "error" || status === "danger") {
            dotColor = LudeloTheme.error;
            glowColor = Qt.rgba(0xEF/255, 0x47/255, 0x6F/255, 0.35);
        } else if (status === "warning") {
            dotColor = LudeloTheme.warn;
            glowColor = Qt.rgba(0xFF/255, 0xB7/255, 0x03/255, 0.35);
        }
    }

    implicitHeight: 28
    implicitWidth: contentRow.implicitWidth + 20

    radius: LudeloTheme.radiusPill
    color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
    border.color: LudeloTheme.borderSubtle
    border.width: 1

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 8

        // Icon if provided
        Image {
            id: pillIcon
            visible: pillRoot.iconSource !== ""
            source: pillRoot.iconSource
            width: 14
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
        }

        // Status Beacon Dot
        Item {
            id: dotContainer
            visible: pillRoot.showDot
            width: 8
            height: 8
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                id: dotGlow
                anchors.fill: parent
                anchors.margins: -3
                radius: 7
                color: pillRoot.glowColor
                opacity: 0.85

                SequentialAnimation on opacity {
                    running: pillRoot.pulseDot
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.85; to: 0.20; duration: 800; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.20; to: 0.85; duration: 800; easing.type: Easing.InOutQuad }
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 4
                color: pillRoot.dotColor
            }
        }

        // Text
        Text {
            id: pillText
            text: pillRoot.text
            font.family: LudeloTheme.fontFamilyMono
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: pillRoot.textColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
