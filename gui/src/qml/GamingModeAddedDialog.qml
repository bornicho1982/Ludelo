import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "controls" as C

Dialog {
    id: dialog
    parent: Overlay.overlay
    x: Math.round((root.width - width) / 2)
    y: Math.round((root.height - height) / 2)
    modal: true
    width: Math.min(root.width * 0.9, 560)
    standardButtons: Dialog.NoButton
    closePolicy: Popup.CloseOnEscape
    Material.roundedScale: Material.MediumScale
    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    background: Rectangle {
        color: Material.dialogColor
        radius: 12
        border.color: Material.accent
        border.width: 2
        clip: true
    }

    Component.onCompleted: header.visible = false

    onOpened: continueButton.forceActiveFocus(Qt.TabFocusReason)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Title bar inside the bordered panel (matches DialogView title size)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Qt.rgba(0, 212/255, 255/255, 0.1)

            Label {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 20
                font.bold: true
                color: Material.accent
                text: qsTr("Added to Steam")
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 24
            spacing: 24

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Ludelo has been added to your Steam library with artwork and a Steam Deck controller layout.\n\nOn Steam Deck, switch to Gaming Mode to play. If Ludelo does not appear in your library, restart Steam first.")
            }

            RowLayout {
                Layout.alignment: Qt.AlignCenter
                spacing: 20

                C.Button {
                    id: continueButton
                    text: qsTr("Continue")
                    Material.roundedScale: Material.SmallScale
                    onClicked: dialog.close()
                }

                C.Button {
                    text: qsTr("Close App")
                    Material.background: Material.accent
                    Material.roundedScale: Material.SmallScale
                    onClicked: Qt.quit()
                }
            }
        }
    }
}
