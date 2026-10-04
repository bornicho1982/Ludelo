import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Item {
    id: detailsRoot

    property var hostData: ({})
    property int hostIndex: 0

    property bool confirmingUnregister: false
    property string statusMessage: ""

    function close() {
        if (typeof root !== "undefined" && root && root.closeDialog) {
            root.closeDialog();
        } else if (typeof stack !== "undefined" && stack) {
            stack.pop();
        }
    }

    StackView.onActivated: {
        Qt.callLater(() => {
            connectButton.forceActiveFocus(Qt.TabFocusReason);
        });
    }

    // Modal Dimmed Backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.03, 0.05, 0.85)

        MouseArea {
            anchors.fill: parent
            onClicked: detailsRoot.close()
        }
    }

    // Center Modal Card
    Rectangle {
        id: modalCard
        width: Math.min(parent.width - 48, 560)
        height: Math.min(parent.height - 48, 560)
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
            anchors.margins: 22
            spacing: 12

            // Header Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                LPill {
                    text: (detailsRoot.hostData && detailsRoot.hostData.ps5) ? "PS5" : "PS4"
                    dotColor: LudeloTheme.accentMint
                    glowColor: LudeloTheme.accentMintGlow
                    showDot: true
                }

                Label {
                    text: qsTr("CONSOLE DETAILS & MANAGEMENT")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 15
                    font.weight: Font.Black
                    color: LudeloTheme.textPrimary
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: closeHoverArea.containsMouse ? LudeloTheme.bgHover : Qt.rgba(1, 1, 1, 0.05)
                    border.color: closeHoverArea.containsMouse ? LudeloTheme.borderMedium : LudeloTheme.borderSubtle
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: "[ESC]"
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: LudeloTheme.textMuted
                    }

                    MouseArea {
                        id: closeHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: detailsRoot.close()
                    }
                }
            }

            // Status feedback message banner (e.g. rename saved / wake sent)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: statusMessage ? 28 : 0
                visible: statusMessage.length > 0
                radius: 6
                color: Qt.rgba(0.0, 0.96, 0.83, 0.15)
                border.color: LudeloTheme.accentMint
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: detailsRoot.statusMessage
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: LudeloTheme.accentMint
                }
            }

            // Rename Section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: qsTr("CUSTOM DISPLAY NAME")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        radius: LudeloTheme.radiusCard
                        color: renameField.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                        border.color: renameField.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10

                            TextInput {
                                id: renameField
                                Layout.fillWidth: true
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                color: LudeloTheme.textPrimary
                                selectByMouse: true
                                text: (detailsRoot.hostData && (detailsRoot.hostData.displayName || detailsRoot.hostData.name)) || ""

                                Keys.onReturnPressed: (event) => {
                                    saveRenameButton.clicked();
                                    event.accepted = true;
                                }
                            }
                        }
                    }

                    LButton {
                        id: saveRenameButton
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 38
                        variant: "secondary"
                        text: qsTr("SAVE")
                        enabled: detailsRoot.hostData && detailsRoot.hostData.mac && renameField.text.trim().length > 0
                        onClicked: {
                            if (detailsRoot.hostData && detailsRoot.hostData.mac) {
                                Chiaki.renameHostByMac(detailsRoot.hostData.mac, renameField.text.trim());
                                detailsRoot.statusMessage = qsTr("✓ Name updated and saved");
                            }
                        }
                        Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                    }
                }
            }

            // Specs / Telemetry Grid
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Qt.rgba(0, 0, 0, 0.35)
                border.color: LudeloTheme.borderSubtle
                border.width: 1

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    columns: 2
                    rowSpacing: 8
                    columnSpacing: 14

                    // Console Model
                    Label {
                        text: qsTr("Model:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: (detailsRoot.hostData && detailsRoot.hostData.ps5) ? qsTr("PlayStation 5") : qsTr("PlayStation 4")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: LudeloTheme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // Power State
                    Label {
                        text: qsTr("Power State:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: {
                            let s = detailsRoot.hostData ? detailsRoot.hostData.state : "";
                            if (s === "ready") return qsTr("Ready (Online)");
                            if (s === "standby") return qsTr("Rest Mode (Standby)");
                            return qsTr("Offline / Standby");
                        }
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: {
                            let s = detailsRoot.hostData ? detailsRoot.hostData.state : "";
                            if (s === "ready") return LudeloTheme.accentMint;
                            if (s === "standby") return LudeloTheme.warn;
                            return LudeloTheme.textSecondary;
                        }
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // IP Address
                    Label {
                        text: qsTr("IP Address:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: (detailsRoot.hostData && detailsRoot.hostData.address) ? (Chiaki.settings.streamerMode ? "hidden" : detailsRoot.hostData.address) : qsTr("Manual / Remote")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        color: LudeloTheme.textSecondary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // MAC Address
                    Label {
                        text: qsTr("MAC Address:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: (detailsRoot.hostData && detailsRoot.hostData.mac) ? (Chiaki.settings.streamerMode ? "hidden" : detailsRoot.hostData.mac) : qsTr("N/A")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        color: LudeloTheme.textSecondary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // Registration
                    Label {
                        text: qsTr("Registration:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: (detailsRoot.hostData && detailsRoot.hostData.registered) ? qsTr("Paired & Registered") : qsTr("Not Registered (PIN Required)")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: (detailsRoot.hostData && detailsRoot.hostData.registered) ? LudeloTheme.accentMint : LudeloTheme.warn
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // Host ID & System Version
                    Label {
                        text: qsTr("Host ID / System:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: {
                            let hid = (detailsRoot.hostData && detailsRoot.hostData.hostId) ? detailsRoot.hostData.hostId : "N/A";
                            let ver = (detailsRoot.hostData && detailsRoot.hostData.systemVersion) ? (" • v" + detailsRoot.hostData.systemVersion) : "";
                            return hid + ver;
                        }
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        color: LudeloTheme.textSecondary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    // Active Title
                    Label {
                        text: qsTr("Active Title:")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textDim
                    }
                    Label {
                        text: (detailsRoot.hostData && detailsRoot.hostData.app) ? detailsRoot.hostData.app : qsTr("None / Home Screen")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        color: (detailsRoot.hostData && detailsRoot.hostData.app) ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }

            // Unregister Confirmation Banner (2-Step Flow)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: detailsRoot.confirmingUnregister ? 70 : 0
                visible: detailsRoot.confirmingUnregister
                radius: 8
                color: Qt.rgba(0.9, 0.1, 0.2, 0.15)
                border.color: LudeloTheme.error
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Label {
                        text: qsTr("⚠️ Remove pairing credentials? You will need an 8-digit PIN to reconnect.")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: LudeloTheme.error
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        LButton {
                            id: finalUnregisterBtn
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            variant: "danger"
                            text: qsTr("CONFIRM UNREGISTER")
                            onClicked: {
                                if (detailsRoot.hostData && detailsRoot.hostData.mac) {
                                    Chiaki.unregisterHostByMac(detailsRoot.hostData.mac);
                                }
                                detailsRoot.close();
                            }
                            Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                        }

                        LButton {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 32
                            variant: "ghost"
                            text: qsTr("CANCEL")
                            onClicked: detailsRoot.confirmingUnregister = false
                            Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                        }
                    }
                }
            }

            // Action Buttons Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Main Connect / Pair Button
                LButton {
                    id: connectButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    variant: (detailsRoot.hostData && detailsRoot.hostData.registered) ? "mint" : "primary"
                    keyHint: "[A]"
                    text: {
                        if (!detailsRoot.hostData || !detailsRoot.hostData.registered)
                            return qsTr("PAIR CONSOLE (PIN)");
                        if (detailsRoot.hostData.state === "standby")
                            return qsTr("CONNECT (WAKE)");
                        return qsTr("CONNECT DIRECT");
                    }
                    onClicked: {
                        if (!detailsRoot.hostData || !detailsRoot.hostData.registered) {
                            let hAddr = detailsRoot.hostData ? detailsRoot.hostData.address : "";
                            let isPs5 = detailsRoot.hostData ? detailsRoot.hostData.ps5 : true;
                            detailsRoot.close();
                            root.showRegistDialog(hAddr, isPs5);
                        } else {
                            Chiaki.connectToHost(detailsRoot.hostIndex, detailsRoot.hostData.name);
                            detailsRoot.close();
                        }
                    }
                    Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                }

                // Wake Button
                LButton {
                    id: wakeButton
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 42
                    variant: "secondary"
                    keyHint: "[Y]"
                    text: qsTr("WAKE")
                    enabled: detailsRoot.hostData && detailsRoot.hostData.mac && detailsRoot.hostData.registered
                    onClicked: {
                        if (detailsRoot.hostData && detailsRoot.hostData.mac) {
                            Chiaki.wakeHostByMac(detailsRoot.hostData.mac);
                            detailsRoot.statusMessage = qsTr("✓ Wake-on-LAN packet sent");
                        }
                    }
                    Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                }

                // Unregister Button (Arms step 1)
                LButton {
                    id: unregisterButton
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 42
                    variant: "danger"
                    keyHint: "[X]"
                    text: qsTr("UNREGISTER")
                    enabled: detailsRoot.hostData && detailsRoot.hostData.registered && !detailsRoot.confirmingUnregister
                    visible: !detailsRoot.confirmingUnregister
                    onClicked: detailsRoot.confirmingUnregister = true
                    Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                }

                // Close Button
                LButton {
                    id: closeBtn
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 42
                    variant: "ghost"
                    keyHint: "[B]"
                    text: qsTr("CLOSE")
                    onClicked: detailsRoot.close()
                    Keys.onReturnPressed: (event) => { clicked(); event.accepted = true; }
                }
            }
        }
    }

    // Root Key Handling for Keyboard & Gamepad
    Keys.onEscapePressed: detailsRoot.close()
    Keys.onPressed: (event) => {
        if (event.modifiers) return;
        switch (event.key) {
        case Qt.Key_Y:
        case Qt.Key_Yes:
            if (wakeButton.enabled) {
                wakeButton.clicked();
                event.accepted = true;
            }
            break;
        case Qt.Key_X:
        case Qt.Key_No:
            if (!detailsRoot.confirmingUnregister && unregisterButton.enabled) {
                unregisterButton.clicked();
                event.accepted = true;
            }
            break;
        }
    }
}
