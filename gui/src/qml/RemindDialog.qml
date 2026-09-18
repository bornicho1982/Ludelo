import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Dialog {
    id: dialog
    property alias text: label.text
    property var remotePlay
    property var callback
    property bool newDialogOpen: false
    property Item restoreFocusItem

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    width: Math.min(parent ? parent.width - 48 : 500, 500)

    background: Rectangle {
        color: LudeloTheme.bgDialog
        radius: LudeloTheme.radiusCard + 2
        border.color: LudeloTheme.borderSubtle
        border.width: 1

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            color: LudeloTheme.accent
        }
    }

    header: Item {
        height: 50
        width: parent.width

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Label {
                text: dialog.title || qsTr("REMINDER")
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
        yesBtn.forceActiveFocus(Qt.TabFocusReason);
    }

    onAccepted: {
        newDialogOpen = true;
        restoreFocus();
        if (callback) callback();
    }

    onRejected: {
        if (dialog.remotePlay)
            Chiaki.settings.remotePlayAsk = false;
        else
            Chiaki.settings.addSteamShortcutAsk = false;
    }

    onClosed: {
        if (!newDialogOpen) restoreFocus();
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
                id: yesBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                variant: "primary"
                keyHint: "[A]"
                text: qsTr("YES")
                onClicked: dialog.accept()
            }

            LButton {
                id: noBtn
                Layout.preferredWidth: 140
                Layout.preferredHeight: 44
                variant: "ghost"
                keyHint: "[B]"
                text: qsTr("NO")
                onClicked: dialog.reject()
            }
        }
    }

    Keys.onEscapePressed: dialog.reject()
    Keys.onReturnPressed: dialog.accept()
}