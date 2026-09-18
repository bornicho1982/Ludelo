import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Item {
    id: manualHostRoot

    readonly property bool canAdd: hostField.text.trim().length > 0 &&
                                  consoleCombo.currentIndex >= 0 &&
                                  consoleCombo.model[consoleCombo.currentIndex].index !== -1

    function close() {
        if (typeof root !== "undefined" && root && root.closeDialog) {
            root.closeDialog();
        } else if (typeof stack !== "undefined" && stack) {
            stack.pop();
        }
    }

    StackView.onActivated: {
        Qt.callLater(() => {
            hostField.forceActiveFocus(Qt.TabFocusReason);
        });
    }

    // Modal Dimmed Backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.03, 0.05, 0.85)

        MouseArea {
            anchors.fill: parent
            onClicked: manualHostRoot.close()
        }
    }

    // Center Modal Card
    Rectangle {
        id: modalCard
        width: Math.min(parent.width - 48, 540)
        height: Math.min(parent.height - 48, 400)
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
                    text: qsTr("MANUAL IP")
                    dotColor: LudeloTheme.accentMint
                    glowColor: LudeloTheme.accentMintGlow
                    showDot: true
                }

                Label {
                    text: qsTr("ADD MANUAL CONSOLE")
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
                text: qsTr("Directly connect to a console by specifying its local IP address if mDNS discovery is blocked on your LAN.")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 11
                color: LudeloTheme.textSecondary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // IP Field
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: qsTr("CONSOLE IP ADDRESS")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: LudeloTheme.radiusCard
                    color: hostField.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                    border.color: hostField.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        TextInput {
                            id: hostField
                            Layout.fillWidth: true
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 13
                            color: LudeloTheme.textPrimary
                            echoMode: Chiaki.settings.streamerMode ? TextInput.Password : TextInput.Normal
                            selectByMouse: true

                            Text {
                                text: qsTr("192.168.1.xxx")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 13
                                color: LudeloTheme.textMuted
                                visible: !hostField.text && !hostField.activeFocus
                            }
                        }
                    }
                }
            }

            // Registered Consoles Combo
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: qsTr("REGISTERED HOST TARGET")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }

                ComboBox {
                    id: consoleCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    textRole: "name"
                    model: {
                        let m = [];
                        if (Chiaki.settings.registeredHosts.length > 0) {
                            m.push({ name: qsTr("Select an Option"), index: -1 });
                        }
                        let i = 0;
                        for (; i < Chiaki.settings.registeredHosts.length; ++i) {
                            let host = Chiaki.settings.registeredHosts[i];
                            m.push({
                                name: "%1 (%2)".arg(Chiaki.settings.streamerMode ? "hidden" : host.mac).arg(host.name),
                                index: i,
                            });
                        }
                        m.push({
                            name: qsTr("Register on first Connection"),
                            index: i,
                        });
                        return m;
                    }

                    background: Rectangle {
                        color: LudeloTheme.bgElevated
                        radius: LudeloTheme.radiusCard
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                LButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    variant: canAdd ? "mint" : "secondary"
                    keyHint: "[A]"
                    text: qsTr("ADD CONSOLE")
                    enabled: canAdd
                    onClicked: {
                        Chiaki.addManualHost(consoleCombo.model[consoleCombo.currentIndex].index, hostField.text.trim());
                        manualHostRoot.close();
                    }
                }

                LButton {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 44
                    variant: "ghost"
                    keyHint: "[B]"
                    text: qsTr("CANCEL")
                    onClicked: manualHostRoot.close()
                }
            }
        }
    }

    Keys.onEscapePressed: manualHostRoot.close()
}
