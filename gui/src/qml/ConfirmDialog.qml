import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import Ludelo 1.0
import "components"

Dialog {
    id: dialog
    property alias text: label.text
    property var callback
    property var rejectCallback
    property bool newDialogOpen: false
    property bool keepDialogOpen: false  // Don't close dialog before callback (e.g., for quit)
    property Item restoreFocusItem

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    width: Math.min(parent ? parent.width - 48 : 520, 520)

    background: Rectangle {
        color: LudeloTheme.bgDialog
        radius: LudeloTheme.radiusCard + 2
        border.color: LudeloTheme.borderSubtle
        border.width: 1

        // Top accent line
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            color: LudeloTheme.accent
        }
    }

    header: Item {
        height: 54
        width: parent.width

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Rectangle {
                width: 24
                height: 24
                radius: 12
                color: Qt.rgba(0.42, 0.36, 0.90, 0.2)
                border.color: LudeloTheme.accent
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "!"
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: LudeloTheme.accentMint
                }
            }

            Label {
                text: dialog.title || qsTr("CONFIRMATION")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 15
                font.weight: Font.Black
                color: LudeloTheme.textPrimary
                Layout.fillWidth: true
            }

            Label {
                text: "[ESC]"
                font.family: LudeloTheme.fontFamilyMono
                font.pixelSize: 10
                font.weight: Font.Bold
                color: LudeloTheme.textMuted
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: LudeloTheme.borderSubtle
        }
    }

    onOpened: {
        confirmBtn.forceActiveFocus(Qt.TabFocusReason);
    }

    onAccepted: {
        if (!keepDialogOpen) {
            newDialogOpen = true;
            restoreFocus();
        }
        if (callback) callback();
    }

    onClosed: {
        if (!newDialogOpen) restoreFocus();
    }

    onRejected: {
        if (rejectCallback) {
            newDialogOpen = true;
            restoreFocus();
            rejectCallback();
        }
    }

    function restoreFocus() {
        if (restoreFocusItem)
            restoreFocusItem.forceActiveFocus(Qt.TabFocusReason);
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 20

        Label {
            id: label
            Layout.fillWidth: true
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 13
            color: LudeloTheme.textSecondary
            wrapMode: Text.Wrap
            lineHeight: 1.3
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            LButton {
                id: confirmBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                variant: "primary"
                keyHint: "[A]"
                text: qsTr("CONFIRM")
                onClicked: dialog.accept()
            }

            LButton {
                id: cancelBtn
                Layout.preferredWidth: 140
                Layout.preferredHeight: 44
                variant: "ghost"
                keyHint: "[B]"
                text: qsTr("CANCEL")
                onClicked: dialog.reject()
            }
        }
    }

    Shortcut { sequence: "Escape"; enabled: dialog.visible; onActivated: dialog.reject() }
    Shortcut { sequence: "Return"; enabled: dialog.visible; onActivated: dialog.accept() }
    Shortcut { sequence: "Enter"; enabled: dialog.visible; onActivated: dialog.accept() }
}
