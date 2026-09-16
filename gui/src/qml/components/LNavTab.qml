import QtQuick
import Ludelo 1.0

Item {
    id: navTabRoot

    property string text: ""
    property string iconSource: ""
    property bool active: false
    property bool isHovered: tabMouseArea.containsMouse
    property bool isFocused: activeFocus
    signal clicked()

    implicitHeight: 48
    implicitWidth: Math.max(90, tabContentRow.implicitWidth + 24)

    Row {
        id: tabContentRow
        anchors.centerIn: parent
        spacing: 8

        Image {
            id: tabIcon
            visible: navTabRoot.iconSource !== ""
            source: navTabRoot.iconSource
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: tabText
            text: navTabRoot.text
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 14
            font.weight: navTabRoot.active ? Font.DemiBold : Font.Normal
            color: {
                if (navTabRoot.active) return LudeloTheme.textPrimary;
                if (navTabRoot.isHovered || navTabRoot.isFocused) return LudeloTheme.textSecondary;
                return LudeloTheme.textDim;
            }
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color {
                ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
            }
        }
    }

    // Glowing active underline bar
    Rectangle {
        id: activeIndicator
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        radius: 2
        color: LudeloTheme.accentPrimary
        visible: navTabRoot.active

        // Ambient neon drop-glow
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 4
            color: LudeloTheme.accentGlow
            opacity: 0.9
            z: -1
        }
    }

    MouseArea {
        id: tabMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: navTabRoot.clicked()
    }
}
