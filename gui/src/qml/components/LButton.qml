import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Ludelo 1.0

Button {
    id: control

    property string variant: "primary" // "primary", "secondary", "ghost", "danger", "mint"
    property string keyHint: ""
    property string iconSource: ""
    property bool glowEnabled: true
    property real customRadius: LudeloTheme.radiusButton
    property bool isGamepadFocused: control.activeFocus

    implicitWidth: Math.max(160, contentRow.implicitWidth + 36)
    implicitHeight: 52

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    scale: control.pressed ? 0.98 : (control.hovered || control.isGamepadFocused ? 1.02 : 1.0)
    Behavior on scale {
        NumberAnimation {
            duration: LudeloTheme.animFast
            easing.type: LudeloTheme.easingCurve
        }
    }

    background: Item {
        id: bgContainer
        anchors.fill: parent

        // Glow Layer for Primary & Mint
        Rectangle {
            id: glowRect
            anchors.fill: parent
            anchors.margins: -4
            radius: control.customRadius + 4
            visible: control.glowEnabled && (control.variant === "primary" || control.variant === "mint" || control.isGamepadFocused || control.hovered)
            color: {
                if (control.variant === "mint") return LudeloTheme.accentMintGlow;
                if (control.variant === "danger") return Qt.rgba(0xEF/255, 0x47/255, 0x6F/255, 0.35);
                return LudeloTheme.accentGlow;
            }
            opacity: (control.hovered || control.isGamepadFocused) ? 0.85 : 0.40
            Behavior on opacity {
                NumberAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
            }
        }

        // Main Button Surface
        Rectangle {
            id: surfaceRect
            anchors.fill: parent
            radius: control.customRadius

            color: {
                if (!control.enabled) return LudeloTheme.bgElevated;
                if (control.variant === "primary") {
                    return control.hovered || control.isGamepadFocused ? LudeloTheme.accentHover : LudeloTheme.accentPrimary;
                }
                if (control.variant === "secondary") {
                    return control.hovered || control.isGamepadFocused ? LudeloTheme.bgCardHover : LudeloTheme.bgElevated;
                }
                if (control.variant === "danger") {
                    return control.hovered || control.isGamepadFocused ? Qt.rgba(0xEF/255, 0x47/255, 0x6F/255, 0.9) : LudeloTheme.error;
                }
                if (control.variant === "mint") {
                    return control.hovered || control.isGamepadFocused ? Qt.lighter(LudeloTheme.accentMint, 1.1) : LudeloTheme.accentMint;
                }
                // Ghost
                return control.hovered || control.isGamepadFocused ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.03);
            }

            border.color: {
                if (control.isGamepadFocused) return (control.variant === "mint" ? LudeloTheme.accentMint : LudeloTheme.borderFocus);
                if (control.hovered) return (control.variant === "primary" ? LudeloTheme.accentHover : LudeloTheme.borderHover);
                if (control.variant === "ghost" || control.variant === "secondary") return LudeloTheme.borderSubtle;
                return "transparent";
            }
            border.width: control.isGamepadFocused ? 2 : 1

            Behavior on color {
                ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
            }
            Behavior on border.color {
                ColorAnimation { duration: LudeloTheme.animFast; easing.type: LudeloTheme.easingCurve }
            }

            // Top-edge light refraction line for glass effect
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 2
                height: 1
                radius: control.customRadius
                color: Qt.rgba(1.0, 1.0, 1.0, control.variant === "primary" ? 0.30 : 0.12)
                visible: control.variant !== "ghost"
            }
        }
    }

    contentItem: Row {
        id: contentRow
        spacing: 12

        Image {
            id: btnIcon
            visible: control.iconSource !== ""
            source: control.iconSource
            width: 20
            height: 20
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: btnText
            text: control.text
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 15
            font.weight: Font.DemiBold
            color: {
                if (!control.enabled) return LudeloTheme.textDim;
                if (control.variant === "mint") return LudeloTheme.bgBase;
                return LudeloTheme.textPrimary;
            }
            anchors.verticalCenter: parent.verticalCenter
        }

        // Keycap badge (e.g. [X] ENTER)
        Rectangle {
            id: keyHintBadge
            visible: control.keyHint !== ""
            anchors.verticalCenter: parent.verticalCenter
            height: 22
            width: hintText.implicitWidth + 12
            radius: 4
            color: Qt.rgba(0, 0, 0, 0.25)
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.15)
            border.width: 1

            Text {
                id: hintText
                anchors.centerIn: parent
                text: control.keyHint
                font.family: LudeloTheme.fontFamilyMono
                font.pixelSize: 11
                font.weight: Font.Bold
                color: LudeloTheme.textSecondary
            }
        }
    }
}
