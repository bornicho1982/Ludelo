import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Item {
    id: registRoot

    property bool ps5: true
    property string host: ""

    // 8-digit PIN string computed from the 8 segmented input boxes
    readonly property string pinText: digit0.text + digit1.text + digit2.text + digit3.text +
                                      digit4.text + digit5.text + digit6.text + digit7.text

    // Check if registration action is ready
    readonly property bool canRegister: {
        if (!hostField.text.trim()) return false;
        if (pinText.length !== 8) return false;
        let pId = Chiaki.settings.psnAccountId || manualAccountIdField.text.trim();
        if (!pId) return false;
        return !isRegistering;
    }

    property bool isRegistering: false
    property bool registrationSuccess: false
    property string logOutputText: ""
    property int countdownSeconds: 300 // Sony Remote Play PIN expires in 5 minutes (300s)

    function close() {
        if (typeof root !== "undefined" && root && root.closeDialog) {
            root.closeDialog();
        } else if (typeof stack !== "undefined" && stack) {
            stack.pop();
        }
    }

    Component.onCompleted: {
        if (host) {
            hostField.text = host;
        }
        if (Chiaki.settings.psnAccountId) {
            manualAccountIdField.text = Chiaki.settings.psnAccountId;
        }
        countdownTimer.restart();
        digit0.forceActiveFocus();
    }

    // 300s Countdown timer for the PIN
    Timer {
        id: countdownTimer
        interval: 1000
        repeat: true
        running: true
        onTriggered: {
            if (countdownSeconds > 0) {
                countdownSeconds--;
            } else {
                running = false;
            }
        }
    }

    function formatCountdown(sec) {
        let m = Math.floor(sec / 60);
        let s = sec % 60;
        return (m < 10 ? "0" + m : m) + ":" + (s < 10 ? "0" + s : s);
    }

    Connections {
        target: Chiaki
        function onPsnLoginAccountIdDone(id) {
            if (id) {
                manualAccountIdField.text = id;
            }
        }
    }

    // Modal Dimmed Backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.03, 0.05, 0.85)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (!isRegistering) registRoot.close();
            }
        }
    }

    // Center Modal Dialog
    Rectangle {
        id: modalContainer
        width: Math.min(parent.width - 48, 720)
        height: Math.min(parent.height - 48, 760)
        anchors.centerIn: parent
        radius: LudeloTheme.radiusCard + 4
        color: LudeloTheme.bgDialog
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        clip: true

        // Stop mouse clicks from bubbling to the backdrop
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        // Top Accent Neon Bar
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 3
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: LudeloTheme.accent }
                GradientStop { position: 0.5; color: LudeloTheme.accentMint }
                GradientStop { position: 1.0; color: LudeloTheme.accent }
            }
        }

        ScrollView {
            id: modalScroll
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: modalScroll.availableWidth
                spacing: 14

                // 1. HEADER ROW
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    LPill {
                        text: qsTr("PAIRING MODE")
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }

                    Label {
                        text: qsTr("REGISTER CONSOLE")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 18
                        font.weight: Font.Black
                        color: LudeloTheme.textPrimary
                    }

                    Item { Layout.fillWidth: true }

                    // Close Button
                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: 16
                        color: closeMouseArea.containsMouse ? LudeloTheme.bgHover : Qt.rgba(1, 1, 1, 0.05)
                        border.color: closeMouseArea.containsMouse ? LudeloTheme.borderMedium : LudeloTheme.borderSubtle
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "[ESC]"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.textSecondary
                        }

                        MouseArea {
                            id: closeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: registRoot.close()
                        }
                    }
                }

                Label {
                    text: qsTr("Pair your PlayStation 5 or PlayStation 4 via local 8-digit Remote Play PIN")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 12
                    color: LudeloTheme.textMuted
                }

                // 2. STEP-BY-STEP INSTRUCTIONS BANNER
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    radius: LudeloTheme.radiusCard
                    color: Qt.rgba(0.04, 0.05, 0.08, 0.7)
                    border.color: Qt.rgba(0.0, 0.96, 0.83, 0.3)
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 8
                            color: Qt.rgba(0.0, 0.96, 0.83, 0.15)
                            border.color: LudeloTheme.accentMint
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "🎮"
                                font.pixelSize: 16
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: qsTr("HOW TO RETRIEVE YOUR 8-DIGIT PAIRING CODE")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: LudeloTheme.accentMint
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("On your console, navigate to: Settings > System > Remote Play > Link Device (PS5) or Settings > Remote Play Connection Settings > Add Device (PS4).")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 11
                                color: LudeloTheme.textSecondary
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }

                // 3. CONSOLE TARGET SELECTOR
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: qsTr("CONSOLE TARGET ARCHITECTURE")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: LudeloTheme.textSecondary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // PS5 Button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: LudeloTheme.radiusCard
                            color: registRoot.ps5 ? LudeloTheme.accent : LudeloTheme.bgElevated
                            border.color: registRoot.ps5 ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                            border.width: registRoot.ps5 ? 2 : 1

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    text: registRoot.ps5 ? "●" : "○"
                                    font.pixelSize: 14
                                    color: registRoot.ps5 ? LudeloTheme.accentMint : LudeloTheme.textMuted
                                }

                                Label {
                                    text: qsTr("PlayStation 5")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 13
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }

                                LPill {
                                    text: qsTr("PS5 • HDR")
                                    showDot: false
                                    textColor: LudeloTheme.accentMint
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: registRoot.ps5 = true
                            }
                        }

                        // PS4 Button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: LudeloTheme.radiusCard
                            color: !registRoot.ps5 ? LudeloTheme.accent : LudeloTheme.bgElevated
                            border.color: !registRoot.ps5 ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                            border.width: !registRoot.ps5 ? 2 : 1

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    text: !registRoot.ps5 ? "●" : "○"
                                    font.pixelSize: 14
                                    color: !registRoot.ps5 ? LudeloTheme.accentMint : LudeloTheme.textMuted
                                }

                                Label {
                                    text: qsTr("PlayStation 4")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 13
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }

                                LPill {
                                    text: qsTr("LEGACY DDP")
                                    showDot: false
                                    textColor: LudeloTheme.textMuted
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: registRoot.ps5 = false
                            }
                        }
                    }
                }

                // 4. CONSOLE IP ADDRESS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("CONSOLE IP ADDRESS (LOCAL NETWORK)")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.textSecondary
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: hostField.text.trim() ? qsTr("• Local LAN Target") : qsTr("• Required")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            color: hostField.text.trim() ? LudeloTheme.accentMint : LudeloTheme.colorWarning
                        }
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
                            spacing: 8

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

                            LPill {
                                text: hostField.text.trim() ? qsTr("ONLINE / DETECTED") : qsTr("NO IP")
                                showDot: true
                                dotColor: hostField.text.trim() ? LudeloTheme.accentMint : LudeloTheme.colorWarning
                                glowColor: hostField.text.trim() ? LudeloTheme.accentMintGlow : Qt.rgba(0.96, 0.62, 0.04, 0.4)
                            }
                        }
                    }
                }

                // 5. 8-DIGIT REMOTE PLAY PIN INPUT (EXACTLY 8 NUMERIC DIGITS)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("8-DIGIT REMOTE PLAY PIN")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.textSecondary
                        }

                        Item { Layout.fillWidth: true }

                        // Active 300s Countdown Badge
                        Rectangle {
                            Layout.preferredHeight: 22
                            Layout.preferredWidth: timerRow.implicitWidth + 14
                            radius: 4
                            color: countdownSeconds > 30 ? Qt.rgba(0.0, 0.96, 0.83, 0.15) : Qt.rgba(0.94, 0.28, 0.44, 0.2)
                            border.color: countdownSeconds > 30 ? LudeloTheme.accentMint : LudeloTheme.colorDanger
                            border.width: 1

                            Row {
                                id: timerRow
                                anchors.centerIn: parent
                                spacing: 4

                                Text {
                                    text: "⏱"
                                    font.pixelSize: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Label {
                                    text: qsTr("%1 remaining").arg(formatCountdown(countdownSeconds))
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: countdownSeconds > 30 ? LudeloTheme.accentMint : LudeloTheme.colorDanger
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    // 8 Segmented Digit Boxes
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 8

                        // First Group of 4 Digits
                        RowLayout {
                            spacing: 6

                            // Box 0
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit0.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit0.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit0.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit0
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit1.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 0)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit0.forceActiveFocus(); }
                            }

                            // Box 1
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit1.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit1.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit1.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit1
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit2.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 1)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit1.forceActiveFocus(); }
                            }

                            // Box 2
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit2.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit2.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit2.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit2
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit3.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 2)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit2.forceActiveFocus(); }
                            }

                            // Box 3
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit3.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit3.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit3.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit3
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit4.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 3)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit3.forceActiveFocus(); }
                            }
                        }

                        // Divider Dash
                        Rectangle {
                            width: 14
                            height: 3
                            radius: 1.5
                            color: LudeloTheme.accentMint
                        }

                        // Second Group of 4 Digits
                        RowLayout {
                            spacing: 6

                            // Box 4
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit4.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit4.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit4.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit4
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit5.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 4)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit4.forceActiveFocus(); }
                            }

                            // Box 5
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit5.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit5.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit5.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit5
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit6.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 5)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit5.forceActiveFocus(); }
                            }

                            // Box 6
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit6.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit6.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit6.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit6
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    onTextChanged: { if (text.length === 1) digit7.forceActiveFocus(); }
                                    Keys.onPressed: (event) => handleDigitKey(event, 6)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit6.forceActiveFocus(); }
                            }

                            // Box 7
                            Rectangle {
                                width: 44; height: 50; radius: 8
                                color: digit7.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                                border.color: digit7.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: digit7.activeFocus ? 2 : 1

                                TextInput {
                                    id: digit7
                                    anchors.centerIn: parent
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                    maximumLength: 1
                                    validator: RegularExpressionValidator { regularExpression: /[0-9]/ }
                                    Keys.onPressed: (event) => handleDigitKey(event, 7)
                                }
                                MouseArea { anchors.fill: parent; onClicked: digit7.forceActiveFocus(); }
                            }
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("PIN expires in 300 seconds on your console screen. Enter digits consecutively without dashes.")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        color: LudeloTheme.textMuted
                    }
                }

                // 6. PSN ACCOUNT ID SECTION
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("PSN ACCOUNT ID (BASE64)")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.textSecondary
                        }

                        Item { Layout.fillWidth: true }

                        LPill {
                            text: Chiaki.settings.psnAccountId ? qsTr("AUTHENTICATED") : qsTr("NOT SIGNED IN")
                            showDot: true
                            dotColor: Chiaki.settings.psnAccountId ? LudeloTheme.accentMint : LudeloTheme.colorWarning
                            glowColor: Chiaki.settings.psnAccountId ? LudeloTheme.accentMintGlow : Qt.rgba(0.96, 0.62, 0.04, 0.4)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: LudeloTheme.radiusCard
                        color: Qt.rgba(0.04, 0.05, 0.08, 0.8)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 8
                            spacing: 8

                            Text {
                                text: "🔒"
                                font.pixelSize: 12
                            }

                            // Show masked Account ID from session or manual entry
                            TextInput {
                                id: manualAccountIdField
                                Layout.fillWidth: true
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 12
                                color: LudeloTheme.textPrimary
                                selectByMouse: true
                                text: Chiaki.settings.psnAccountId || ""

                                Text {
                                    text: qsTr("PSN Account ID (base64) required...")
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    color: LudeloTheme.textMuted
                                    visible: !manualAccountIdField.text && !manualAccountIdField.activeFocus
                                }
                            }

                            LButton {
                                Layout.preferredHeight: 28
                                Layout.preferredWidth: 140
                                variant: "ghost"
                                text: qsTr("RE-AUTHENTICATE")
                                onClicked: {
                                    if (Qt.platform.os === "windows") {
                                        Chiaki.startWebView2Login();
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        text: qsTr("Verified via Encrypted Local Storage (DPAPI). Automatically linked during setup.")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 10
                        color: LudeloTheme.textMuted
                    }
                }

                // 7. CONSOLE USER PASSCODE (OPTIONAL - 4 DIGITS)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("CONSOLE USER PASSCODE (OPTIONAL - 4 DIGITS)")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.textSecondary
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: qsTr("Optional")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            color: LudeloTheme.textMuted
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: LudeloTheme.radiusCard
                        color: cpinField.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(0.04, 0.05, 0.08, 0.8)
                        border.color: cpinField.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            TextInput {
                                id: cpinField
                                Layout.fillWidth: true
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 13
                                color: LudeloTheme.textPrimary
                                echoMode: TextInput.Password
                                maximumLength: 4
                                validator: RegularExpressionValidator { regularExpression: /^$|[0-9]{4}/ }

                                Text {
                                    text: qsTr("•••• (Leave blank if disabled on console profile)")
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    color: LudeloTheme.textMuted
                                    visible: !cpinField.text && !cpinField.activeFocus
                                }
                            }
                        }
                    }

                    Label {
                        text: qsTr("Required only if your console user profile has a login passcode set. Auto-submitted when starting session.")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 10
                        color: LudeloTheme.textMuted
                    }
                }

                // 8. LOG OUTPUT AREA (When registering or after)
                Rectangle {
                    id: logContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: logOutputText ? 100 : 0
                    visible: logOutputText.length > 0
                    radius: LudeloTheme.radiusCard
                    color: Qt.rgba(0.04, 0.05, 0.08, 0.95)
                    border.color: registrationSuccess ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1
                    clip: true

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 10

                        TextArea {
                            id: logTextArea
                            text: logOutputText
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            color: registrationSuccess ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                            readOnly: true
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // 9. MODAL ACTIONS & CTA
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    spacing: 12

                    LButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        variant: canRegister ? "mint" : "secondary"
                        keyHint: "[A]"
                        text: isRegistering ? qsTr("PAIRING CONSOLE...") : qsTr("REGISTER & PAIR CONSOLE")
                        enabled: canRegister
                        onClicked: executeRegistration()
                    }

                    LButton {
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 46
                        variant: "ghost"
                        keyHint: "[B]"
                        text: qsTr("CANCEL")
                        enabled: !isRegistering
                        onClicked: registRoot.close()
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Ludelo uses local DDP peer-to-peer pairing. Your PIN is sent directly to your console on LAN.")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    color: LudeloTheme.textMuted
                }
            }
        }
    }

    // Helper for key navigation and backspace across the 8 digit inputs
    function handleDigitKey(event, index) {
        if (event.key === Qt.Key_Backspace) {
            let current = getDigitItem(index);
            if (current.text.length === 0 && index > 0) {
                let prev = getDigitItem(index - 1);
                prev.text = "";
                prev.forceActiveFocus();
                event.accepted = true;
            }
        } else if (event.key === Qt.Key_Left && index > 0) {
            getDigitItem(index - 1).forceActiveFocus();
            event.accepted = true;
        } else if (event.key === Qt.Key_Right && index < 7) {
            getDigitItem(index + 1).forceActiveFocus();
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (canRegister) {
                executeRegistration();
                event.accepted = true;
            }
        }
    }

    function getDigitItem(idx) {
        switch (idx) {
            case 0: return digit0;
            case 1: return digit1;
            case 2: return digit2;
            case 3: return digit3;
            case 4: return digit4;
            case 5: return digit5;
            case 6: return digit6;
            case 7: return digit7;
            default: return digit0;
        }
    }

    function executeRegistration() {
        if (!canRegister) return;

        let hostAddr = hostField.text.trim();
        let psnId = Chiaki.settings.psnAccountId || manualAccountIdField.text.trim();
        let pin = pinText;
        let cpin = cpinField.text.trim();
        let isBroadcast = hostAddr === "255.255.255.255";
        let targetVersion = registRoot.ps5 ? 1000100 : 1000;

        isRegistering = true;
        registrationSuccess = false;
        logOutputText = qsTr("Initiating DDP pairing handshake with %1...\n").arg(hostAddr);

        let registerOk = Chiaki.registerHost(hostAddr, psnId, pin, cpin, isBroadcast, targetVersion, function(msg, ok, done) {
            if (!done) {
                logOutputText += msg + "\n";
            } else {
                isRegistering = false;
                if (ok) {
                    registrationSuccess = true;
                    logOutputText += qsTr("\n✓ Console registered successfully! Closing dialog...\n");
                    autoCloseTimer.restart();
                } else {
                    logOutputText += qsTr("\n✗ Registration failed. Please verify the PIN and IP address.\n");
                }
            }
        });

        if (!registerOk) {
            isRegistering = false;
            logOutputText += qsTr("Failed to initiate host registration.\n");
        }
    }

    Timer {
        id: autoCloseTimer
        interval: 1500
        repeat: false
        onTriggered: registRoot.close()
    }

    // Global Key Handling for modal
    Keys.onEscapePressed: {
        if (!isRegistering) registRoot.close();
    }
}
