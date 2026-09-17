import QtQuick
import QtQuick.Controls
import Ludelo 1.0

Slider {
    id: control

    property string unit: ""
    property bool firstInFocusChain: false
    property bool lastInFocusChain: false

    implicitWidth: 280
    implicitHeight: 32
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 6
        width: control.availableWidth
        height: implicitHeight
        radius: 3
        color: Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.9)
        border.color: control.activeFocus ? LudeloTheme.borderFocus : LudeloTheme.borderSubtle
        border.width: 1

        // Active fill
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: LudeloTheme.accentMint
            radius: 3

            // Subtle glow
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: LudeloTheme.accentMintGlow
                opacity: 0.6
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: control.pressed ? "#FFFFFF" : (control.hovered || control.activeFocus ? LudeloTheme.accentMint : "#E1E2EB")
        border.color: control.activeFocus ? "#FFFFFF" : LudeloTheme.accentMint
        border.width: control.activeFocus ? 2 : 1

        Behavior on color { ColorAnimation { duration: LudeloTheme.animFast } }
        Behavior on border.color { ColorAnimation { duration: LudeloTheme.animFast } }

        // Thumb inner dot
        Rectangle {
            anchors.centerIn: parent
            width: 6
            height: 6
            radius: 3
            color: control.pressed ? LudeloTheme.accentMint : "#0B0E14"
        }

        // Thumb Glow
        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: parent.radius + 4
            color: LudeloTheme.accentMintGlow
            opacity: (control.hovered || control.activeFocus) ? 0.75 : 0.0
            visible: opacity > 0
            z: -1
            Behavior on opacity { NumberAnimation { duration: LudeloTheme.animFast } }
        }
    }

    Keys.onLeftPressed: {
        value = Math.max(from, value - stepSize);
    }
    Keys.onRightPressed: {
        value = Math.min(to, value + stepSize);
    }
}
