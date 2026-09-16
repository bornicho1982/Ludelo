import QtQuick
import Ludelo 1.0

Item {
    id: cardRoot

    default property alias content: contentContainer.data
    property real customRadius: LudeloTheme.radiusCard
    property color cardColor: LudeloTheme.bgCard
    property color borderColor: isFocused ? LudeloTheme.borderFocus : (isHovered ? LudeloTheme.borderHover : LudeloTheme.borderSubtle)
    property bool hoverLift: false
    property bool glowEnabled: true
    property color glowColor: LudeloTheme.accentGlow
    property bool isHovered: mouseArea.containsMouse
    property bool isFocused: activeFocus
    property alias mouseEnabled: mouseArea.enabled
    signal clicked()

    implicitWidth: 320
    implicitHeight: 200

    y: (hoverLift && (isHovered || isFocused)) ? -4 : 0
    Behavior on y {
        NumberAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
    }

    // Atmospheric Glow Behind Card
    Rectangle {
        id: ambientGlow
        anchors.fill: parent
        anchors.margins: -6
        radius: cardRoot.customRadius + 6
        visible: cardRoot.glowEnabled && (cardRoot.isHovered || cardRoot.isFocused)
        color: cardRoot.glowColor
        opacity: cardRoot.isFocused ? 0.70 : (cardRoot.isHovered ? 0.45 : 0.0)
        Behavior on opacity {
            NumberAnimation { duration: LudeloTheme.animNormal; easing.type: LudeloTheme.easingCurve }
        }
    }

    // Main Card Surface
    Rectangle {
        id: surface
        anchors.fill: parent
        radius: cardRoot.customRadius
        color: cardRoot.cardColor
        border.color: cardRoot.borderColor
        border.width: cardRoot.isFocused ? 2 : 1

        Behavior on border.color {
            ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
        }
        Behavior on color {
            ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
        }

        // Subtle Glass Highlight on Top Border
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: 1
            radius: cardRoot.customRadius
            color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
        }

        Item {
            id: contentContainer
            anchors.fill: parent
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: cardRoot.hoverLift ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: cardRoot.clicked()
    }
}
