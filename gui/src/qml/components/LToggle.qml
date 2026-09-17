import QtQuick
import QtQuick.Controls
import Ludelo 1.0

Switch {
    id: control

    property string label: ""
    property string description: ""

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    indicator: Rectangle {
        id: track
        implicitWidth: 46
        implicitHeight: 24
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 12
        color: control.checked ? LudeloTheme.accentMint : Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.9)
        border.color: {
            if (control.activeFocus) return LudeloTheme.accentMint;
            if (control.checked) return LudeloTheme.accentMint;
            if (control.hovered) return LudeloTheme.borderHover;
            return LudeloTheme.borderSubtle;
        }
        border.width: control.activeFocus ? 2 : 1

        Behavior on color {
            ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
        }
        Behavior on border.color {
            ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
        }

        // Soft glow when ON
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: LudeloTheme.accentMintGlow
            opacity: control.checked ? 0.7 : 0.0
            visible: opacity > 0
            z: -1

            Behavior on opacity {
                NumberAnimation { duration: LudeloTheme.animFast }
            }
        }

        // Thumb
        Rectangle {
            id: thumb
            x: control.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            radius: 9
            color: control.checked ? "#0B0E14" : LudeloTheme.textSecondary

            Behavior on x {
                NumberAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
            }
            Behavior on color {
                ColorAnimation { duration: LudeloTheme.animFast }
            }
        }
    }

    contentItem: Column {
        leftPadding: control.indicator.width + 12
        spacing: 2
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: control.label.length > 0 ? control.label : control.text
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 14
            font.weight: Font.Medium
            color: control.enabled ? LudeloTheme.textPrimary : LudeloTheme.textDim
        }

        Text {
            visible: control.description.length > 0
            text: control.description
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 11
            color: LudeloTheme.textSecondary
        }
    }

    Keys.onReturnPressed: toggle()
    Keys.onSpacePressed: toggle()
}
