import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Item {
    id: pinRoot
    property var consoleIndex

    readonly property bool canSave: pin.text.trim().length === 4

    function close() {
        if (typeof root !== "undefined" && root && root.closeDialog) {
            root.closeDialog();
        } else if (typeof stack !== "undefined" && stack) {
            stack.pop();
        }
    }

    StackView.onActivated: {
        Qt.callLater(() => {
            pin.forceActiveFocus(Qt.TabFocusReason);
        });
    }

    // Dimmed Backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.03, 0.05, 0.85)
        MouseArea { anchors.fill: parent; onClicked: pinRoot.close() }
    }

    // Modal Card
    Rectangle {
        width: Math.min(parent.width - 48, 500)
        height: Math.min(parent.height - 48, 340)
        anchors.centerIn: parent
        radius: LudeloTheme.radiusCard + 2
        color: LudeloTheme.bgDialog
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        clip: true

        MouseArea { anchors.fill: parent; onClicked: {} }

        // Top Accent Neon Line
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            color: LudeloTheme.accentMint
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                LPill {
                    text: qsTr("CONSOLE SECURITY")
                    dotColor: LudeloTheme.accentMint
                    glowColor: LudeloTheme.accentMintGlow
                    showDot: true
                }

                Label {
                    text: qsTr("USER PASSCODE")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 16
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

            Label {
                text: qsTr("Enter the 4-digit profile login passcode for this console. Ludelo will automatically send it when starting a remote session.")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 11
                color: LudeloTheme.textSecondary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // Monospace Passcode Input
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: qsTr("CONSOLE LOGIN PASSCODE (4 DIGITS)")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46
                    radius: LudeloTheme.radiusCard
                    color: pin.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                    border.color: pin.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        TextInput {
                            id: pin
                            Layout.fillWidth: true
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                            echoMode: Chiaki.settings.streamerMode ? TextInput.Password : TextInput.Password
                            maximumLength: 4
                            validator: RegularExpressionValidator { regularExpression: /[0-9]{4}/ }

                            Text {
                                text: qsTr("•••• (4 digits)")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 14
                                color: LudeloTheme.textMuted
                                visible: !pin.text && !pin.activeFocus
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Actions
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                LButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    variant: canSave ? "mint" : "secondary"
                    keyHint: "[A]"
                    text: qsTr("SAVE PASSCODE")
                    enabled: canSave
                    onClicked: {
                        Chiaki.setConsolePin(pinRoot.consoleIndex, pin.text.trim());
                        pinRoot.close();
                    }
                }

                LButton {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 44
                    variant: "ghost"
                    keyHint: "[B]"
                    text: qsTr("CANCEL")
                    onClicked: pinRoot.close()
                }
            }
        }
    }

    Keys.onEscapePressed: pinRoot.close()
}