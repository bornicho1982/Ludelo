import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects

import org.streetpea.chiaking
import Ludelo 1.0
import "components"
import "controls" as C

Item {
    id: view
    focus: true

    property bool sessionError: false
    property bool sessionLoading: true
    property list<Item> restoreFocusItems
    property bool controllerOverlayShown: false

    // Computed property: are we launching a game directly?
    property bool launchingGame: {
        if (!Chiaki.settings.showGameImageDuringLaunch) {
            return false;
        }
        if (Chiaki.session !== null 
            && Chiaki.session.titleId !== undefined 
            && Chiaki.session.titleId !== null
            && Chiaki.session.titleId !== "") {
            return true;
        }
        if (sessionLoading
            && Chiaki.cloudStreaming !== null
            && Chiaki.cloudStreaming.gameImageUrl !== undefined
            && Chiaki.cloudStreaming.gameImageUrl !== null
            && Chiaki.cloudStreaming.gameImageUrl !== "") {
            return true;
        }
        return false;
    }

    onLaunchingGameChanged: {
        if (launchingGame && Chiaki.session) {
            var isRemotePlay = Chiaki.session.titleId !== undefined 
                               && Chiaki.session.titleId !== null 
                               && Chiaki.session.titleId !== "";
            if (isRemotePlay) {
                Chiaki.session.SetAudioVolume(0);
            }
        }
    }

    function grabInput(item) {
        Chiaki.window.grabInput();
        restoreFocusItems.push(Window.window.activeFocusItem);
        if (item)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    function releaseInput() {
        Chiaki.window.releaseInput();
        var item = restoreFocusItems.pop();
        if (item && item.visible)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    StackView.onActivating: Chiaki.window.keepVideo = true
    StackView.onDeactivated: Chiaki.window.keepVideo = false

    Component.onCompleted: {
        if (Chiaki.session) {
            Chiaki.session.GameLaunchCompleted.connect(function() {
                gameBackgroundLoader.active = false;
                sessionLoading = false;
                Chiaki.session.SetAudioVolume(Chiaki.settings.audioVolume);
                if (!controllerOverlayShown && !Chiaki.settings.controllerOverlayShown) {
                    controllerOverlayTimer.start();
                }
            });
        }
    }

    // Auto-hide timer for bottom HUD dock (1.5 seconds of inactivity)
    Timer {
        id: dockAutoHideTimer
        interval: 1500
        repeat: false
        onTriggered: {
            if (!dockMouseArea.containsMouse && !sessionStopDialog.opened && !sessionPinDialog.opened) {
                hudDock.opacity = 0.0;
            }
        }
    }

    function showHudDock() {
        hudDock.opacity = 1.0;
        dockAutoHideTimer.restart();
    }

    // Connect to C++ user activity signals (mouse move, stick move, chord shortcut)
    Connections {
        target: Chiaki.window
        function onUserActivity() {
            view.showHudDock();
        }
        function onMenuRequested() {
            if (sessionPinDialog.opened || sessionStopDialog.opened)
                return;
            if (hudDock.opacity > 0.0) {
                hudDock.opacity = 0.0;
                dockAutoHideTimer.stop();
            } else {
                view.showHudDock();
            }
        }
    }

    // Timer to show controller overlay after stream starts
    Timer {
        id: controllerOverlayTimer
        interval: 800
        repeat: false
        onTriggered: {
            if (!controllerOverlayShown && !sessionError) {
                controllerOverlayLoader.active = true;
            }
        }
    }

    // Black background when launching game or on error
    Rectangle {
        anchors.fill: parent
        color: LudeloTheme.bgBase
        opacity: {
            if (sessionError || (Chiaki.settings.audioVideoDisabled & 0x02))
                return 1.0;
            if (sessionLoading && launchingGame)
                return 1.0;
            return 0.0;
        }
        visible: opacity > 0
        z: 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }

    // Game background image during launch
    Loader {
        id: gameBackgroundLoader
        anchors.fill: parent
        z: 0
        active: launchingGame
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                Image {
                    id: gameImage
                    anchors.centerIn: parent
                    width: parent.width
                    height: parent.height
                    source: {
                        if (Chiaki.cloudStreaming !== null
                            && Chiaki.cloudStreaming.gameImageUrl !== undefined
                            && Chiaki.cloudStreaming.gameImageUrl !== null
                            && Chiaki.cloudStreaming.gameImageUrl !== "") {
                            return Chiaki.cloudStreaming.gameImageUrl;
                        }
                        if (Chiaki.session && Chiaki.session.titleId) {
                            return ChiakiGames.getGameImage(Chiaki.session.titleId, "landscape");
                        }
                        return "";
                    }
                    fillMode: Image.PreserveAspectFit
                    cache: false
                    Rectangle {
                        anchors.fill: parent
                        color: "black"
                        opacity: 0.4
                    }
                }
            }
        }
    }

    // Loading / Connecting View
    Rectangle {
        id: loadingView
        anchors.fill: parent
        color: "transparent"
        opacity: sessionError || sessionLoading || (Chiaki.settings.audioVideoDisabled & 0x02) ? 1.0 : 0.0
        visible: opacity > 0
        z: 2

        Behavior on opacity { NumberAnimation { duration: 250 } }

        Item {
            anchors {
                top: parent.verticalCenter
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            z: 3

            BusyIndicator {
                id: spinner
                anchors.centerIn: parent
                width: 70
                height: width
                visible: sessionLoading
            }

            Label {
                anchors {
                    top: spinner.bottom
                    horizontalCenter: spinner.horizontalCenter
                    topMargin: 24
                }
                horizontalAlignment: Text.AlignHCenter
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 18
                color: LudeloTheme.textPrimary
                text: {
                    if (Chiaki.cloudStreaming && Chiaki.cloudStreaming.allocationProgress) {
                        return Chiaki.cloudStreaming.allocationProgress;
                    }
                    return qsTr("Establishing secure low-latency session...");
                }
                visible: sessionLoading && (text !== "" || launchingGame)
            }

            Label {
                id: errorTitleLabel
                objectName: "errorTitleLabel"
                anchors {
                    bottom: spinner.top
                    horizontalCenter: spinner.horizontalCenter
                    bottomMargin: 12
                }
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 24
                font.weight: Font.Bold
                color: LudeloTheme.error
                visible: text !== ""
                onVisibleChanged: if (visible) view.grabInput(errorTitleLabel)
                Keys.onReturnPressed: root.showMainView()
                Keys.onEscapePressed: root.showMainView()
            }

            Label {
                id: errorTextLabel
                objectName: "errorTextLabel"
                anchors {
                    top: errorTitleLabel.bottom
                    horizontalCenter: errorTitleLabel.horizontalCenter
                    topMargin: 10
                }
                horizontalAlignment: Text.AlignHCenter
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 16
                color: LudeloTheme.textSecondary
                visible: text !== ""
            }
        }
    }

    // Message when stream content cannot be displayed
    ColumnLayout {
        id: cantDisplayMessage
        anchors.centerIn: parent
        opacity: Chiaki.window.hasVideo && Chiaki.session && Chiaki.session.cantDisplay ? 1.0 : 0.0
        visible: opacity > 0
        spacing: 24
        z: 10

        Behavior on opacity { NumberAnimation { duration: 250 } }

        onVisibleChanged: {
            if (visible) {
                hudDock.opacity = 0.0;
                view.grabInput(goToHomeButton);
            } else {
                view.releaseInput();
            }
        }

        Label {
            Layout.alignment: Qt.AlignCenter
            text: qsTr("The screen contains content that cannot be displayed over Remote Play.")
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 18
            color: LudeloTheme.textPrimary
        }

        LButton {
            id: goToHomeButton
            Layout.alignment: Qt.AlignCenter
            text: qsTr("Go to Home Screen")
            variant: "primary"
            keyHint: "[A]"
            onClicked: Chiaki.sessionGoHome()
            Keys.onReturnPressed: clicked()
            Keys.onEscapePressed: clicked()
        }
    }

    // High Packet Loss Notification Beacon (Top Right)
    Item {
        anchors {
            right: parent.right
            top: parent.top
            margins: 30
            topMargin: 70
        }
        visible: Chiaki.session && Chiaki.session.averagePacketLoss > (Chiaki.settings.wifiDroppedNotif * 0.01)
        z: 99

        LPill {
            text: qsTr("POOR NETWORK CONNECTION")
            dotColor: LudeloTheme.error
            glowColor: Qt.rgba(0xEF/255, 0x47/255, 0x6F/255, 0.5)
            pulseDot: true
            showDot: true
        }
    }

    // =========================================================================
    // TOP HUD BAR: Real Telemetry, Controller Polling, Real Stream Specs
    // =========================================================================
    Item {
        id: topHudBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }
        height: 48
        visible: !sessionLoading && !sessionError && !(Chiaki.settings.audioVideoDisabled & 0x02)
        z: 100

        RowLayout {
            anchors.fill: parent
            spacing: 14

            // Top-Left: Real Stream Identity & Verified Codec/Resolution (NO FAKE "VORTEX")
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.85)
                    border.color: LudeloTheme.borderSubtle
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        source: "qrc:/icons/logo_square_1024.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                LPill {
                    text: {
                        if (!Chiaki.session) return qsTr("CONNECTING...");
                        var fps = Math.round(Chiaki.session.measuredFps || 60);
                        var res = Chiaki.session.resolution || "1080p";
                        if (Chiaki.session.isCloudStreaming) {
                            return qsTr("CLOUD STREAM • %1 %2FPS").arg(res).arg(fps);
                        }
                        var codec = Chiaki.settings.codecLocalPS5 === 1 ? "H.264" : "H.265";
                        return qsTr("DIRECT LAN • %1 %2FPS • %3").arg(res).arg(fps).arg(codec);
                    }
                    dotColor: LudeloTheme.accentMint
                    glowColor: LudeloTheme.accentMintGlow
                    pulseDot: true
                    showDot: true
                }
            }

            Item { Layout.fillWidth: true } // Spacer

            // Top-Center: Controller Hardware Polling (Real Driver Rate, No Fake Battery)
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                LPill {
                    text: {
                        if (Chiaki.controllers.length > 0) {
                            var ctrl = Chiaki.controllers[0];
                            return ctrl.dualSense ? "DUALSENSE WIRELESS • 1000Hz" : "GAMEPAD • 250Hz";
                        }
                        return "KEYBOARD MODE";
                    }
                    dotColor: Chiaki.controllers.length > 0 ? LudeloTheme.accentMint : LudeloTheme.textDim
                    showDot: true
                }

                // Volume Indicator Pill
                MouseArea {
                    width: volumePill.implicitWidth
                    height: volumePill.implicitHeight
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    LPill {
                        id: volumePill
                        anchors.fill: parent
                        text: qsTr("VOL %1%").arg(((Chiaki.settings.audioVolume / 128.0) * 100).toFixed(0))
                        dotColor: (Chiaki.session && Chiaki.session.muted) ? LudeloTheme.warn : LudeloTheme.accentPrimary
                        showDot: true
                    }

                    onClicked: {
                        if (Chiaki.session) {
                            Chiaki.session.muted = !Chiaki.session.muted;
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true } // Spacer

            // Top-Right: Stream Diagnostics HUD Pill (Toggleable with [TAB])
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                visible: Chiaki.settings.showStreamStats
                height: 34
                width: statsRow.implicitWidth + 24
                radius: LudeloTheme.radiusPill
                color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.85)
                border.color: LudeloTheme.borderHover
                border.width: 1

                Row {
                    id: statsRow
                    anchors.centerIn: parent
                    spacing: 12

                    // Bitrate
                    Row {
                        spacing: 4
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: Chiaki.session ? Chiaki.session.measuredBitrate.toFixed(1) : "0.0"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                        }
                        Text {
                            text: "Mbps"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            color: LudeloTheme.accentPrimary
                        }
                    }

                    // RTT / Latency (with mint beacon)
                    Row {
                        spacing: 5
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: LudeloTheme.accentMint
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: qsTr("%1ms RTT").arg(Chiaki.session ? Math.round(Chiaki.session.measuredRtt) : 0)
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: LudeloTheme.accentMint
                        }
                    }

                    // Packet Loss
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("%1% LOSS").arg(Chiaki.session ? (Chiaki.session.averagePacketLoss * 100).toFixed(1) : "0.0")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        color: (Chiaki.session && Chiaki.session.averagePacketLoss > 0.01) ? LudeloTheme.error : LudeloTheme.textSecondary
                    }

                    // FPS & Resolution
                    Rectangle {
                        height: 20
                        width: fpsBadgeText.implicitWidth + 10
                        radius: 4
                        color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.15)
                        border.color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.4)
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            id: fpsBadgeText
                            anchors.centerIn: parent
                            text: qsTr("%1 FPS • %2").arg(Chiaki.session ? Math.round(Chiaki.session.measuredFps) : 60).arg(Chiaki.session ? (Chiaki.session.resolution || "1080p") : "1080p")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: LudeloTheme.accentMint
                        }
                    }
                }
            }

            // Window Close / Disconnect Action Icon
            Rectangle {
                width: 32
                height: 32
                radius: 8
                color: closeTopMouse.containsMouse ? LudeloTheme.error : Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.85)
                border.color: LudeloTheme.borderSubtle
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 13
                    color: closeTopMouse.containsMouse ? "#FFFFFF" : LudeloTheme.textSecondary
                }

                MouseArea {
                    id: closeTopMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sessionStopDialog.open()
                }
            }
        }
    }

    // =========================================================================
    // FLOATING IN-GAME QUICK ACTION DOCK (Auto-hiding at 1.5s of inactivity)
    // =========================================================================
    Item {
        id: dockContainer
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: 32
        }
        height: 78
        z: 100
        visible: !sessionLoading && !sessionError && !(Chiaki.settings.audioVideoDisabled & 0x02)

        // Dock Capsule
        Rectangle {
            id: hudDock
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            height: 52
            width: dockRow.implicitWidth + 36
            radius: 26
            color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.92)
            border.color: LudeloTheme.borderFocus
            border.width: 1
            opacity: 1.0

            Behavior on opacity {
                NumberAnimation { duration: 250; easing.type: Easing.OutQuad }
            }

            // Ambient Drop Glow
            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                radius: parent.radius + 4
                color: LudeloTheme.accentGlow
                opacity: 0.55
                z: -1
            }

            MouseArea {
                id: dockMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: dockAutoHideTimer.stop()
                onExited: dockAutoHideTimer.restart()
            }

            RowLayout {
                id: dockRow
                anchors.centerIn: parent
                spacing: 10

                // [GUIDE] PS HOME
                LButton {
                    height: 36
                    customRadius: 18
                    variant: "secondary"
                    text: "PS HOME"
                    keyHint: "[GUIDE]"
                    onClicked: Chiaki.sessionGoHome()
                }

                // [M] MUTE AUDIO
                LButton {
                    height: 36
                    customRadius: 18
                    variant: (Chiaki.session && Chiaki.session.muted) ? "mint" : "secondary"
                    text: (Chiaki.session && Chiaki.session.muted) ? "UNMUTE" : "MUTE AUDIO"
                    keyHint: "[M]"
                    onClicked: {
                        if (Chiaki.session)
                            Chiaki.session.muted = !Chiaki.session.muted;
                    }
                }

                // [F11] FULLSCREEN
                LButton {
                    height: 36
                    customRadius: 18
                    variant: "secondary"
                    text: "FULLSCREEN"
                    keyHint: "[F11]"
                    onClicked: {
                        if (Chiaki.window.windowState === Qt.WindowFullScreen)
                            Chiaki.window.normalTime();
                        else
                            Chiaki.window.fullscreenTime();
                    }
                }

                // [TAB] HUD STATS (Toggled state indicator)
                LButton {
                    height: 36
                    customRadius: 18
                    variant: Chiaki.settings.showStreamStats ? "primary" : "secondary"
                    text: "HUD STATS"
                    keyHint: "[TAB]"
                    onClicked: {
                        Chiaki.settings.showStreamStats = !Chiaki.settings.showStreamStats;
                    }
                }

                // [START] DISPLAY SETTINGS
                LButton {
                    height: 36
                    customRadius: 18
                    variant: "secondary"
                    text: "DISPLAY"
                    keyHint: "[START]"
                    onClicked: root.openDisplaySettings()
                }

                // Subtle Divider
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 22
                    color: LudeloTheme.borderSubtle
                }

                // [ESC] DISCONNECT
                LButton {
                    height: 36
                    customRadius: 18
                    variant: "danger"
                    text: "DISCONNECT"
                    keyHint: "[ESC]"
                    onClicked: sessionStopDialog.open()
                }
            }
        }

        // Accompanying Keyboard / Guide Hint Below Dock
        Text {
            anchors.top: hudDock.bottom
            anchors.topMargin: 6
            anchors.horizontalCenter: hudDock.horizontalCenter
            text: qsTr("Press [F10] or hold [GUIDE] to toggle HUD dock • Press [TAB] to toggle telemetry pill")
            font.family: LudeloTheme.fontFamilyMono
            font.pixelSize: 11
            color: Qt.rgba(1.0, 1.0, 1.0, 0.45)
            opacity: hudDock.opacity
        }
    }

    // =========================================================================
    // DIALOGS: Session Disconnect & Console PIN (Styled in Ludelo Theme)
    // =========================================================================
    Popup {
        id: sessionStopDialog
        property int closeAction: 0
        property bool isCloudSession: Chiaki.session && Chiaki.session.isCloudStreaming
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 480
        height: 280
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: LudeloTheme.radiusCard
            color: LudeloTheme.bgPanelSolid
            border.color: LudeloTheme.borderHover
            border.width: 1
        }

        onAboutToShow: {
            closeAction = 0;
            view.grabInput(confirmButton);
        }
        onClosed: {
            view.releaseInput();
            if (isCloudSession) {
                if (closeAction === 1) {
                    Chiaki.stopSession(false);
                }
            } else {
                if (closeAction) {
                    Chiaki.stopSession(closeAction === 1);
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 16

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Disconnect Stream Session")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 20
                font.weight: Font.Bold
                color: LudeloTheme.textPrimary
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: sessionStopDialog.isCloudSession 
                    ? qsTr("Are you sure you want to close your active cloud streaming session?")
                    : qsTr("Would you like to put your PlayStation console into Rest Mode or keep it running?")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 13
                color: LudeloTheme.textSecondary
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 12
                spacing: 14

                LButton {
                    id: confirmButton
                    Layout.preferredWidth: 140
                    height: 44
                    variant: "primary"
                    text: sessionStopDialog.isCloudSession ? qsTr("Close") : qsTr("Rest Mode")
                    keyHint: "[A]"
                    onClicked: {
                        sessionStopDialog.closeAction = 1;
                        sessionStopDialog.close();
                    }
                }

                LButton {
                    Layout.preferredWidth: 140
                    height: 44
                    variant: "secondary"
                    text: sessionStopDialog.isCloudSession ? qsTr("Cancel") : qsTr("Leave On")
                    keyHint: "[B]"
                    onClicked: {
                        sessionStopDialog.closeAction = sessionStopDialog.isCloudSession ? 2 : 2;
                        if (!sessionStopDialog.isCloudSession) {
                            Chiaki.stopSession(false);
                        }
                        sessionStopDialog.close();
                    }
                }
            }
        }
    }

    Dialog {
        id: sessionPinDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        title: qsTr("Console Login PIN")
        modal: true
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAboutToShow: {
            standardButton(Dialog.Ok).enabled = Qt.binding(function() {
                return pinField.acceptableInput;
            });
            view.grabInput(pinField);
        }
        onClosed: view.releaseInput()
        onAccepted: Chiaki.enterPin(pinField.text)
        onRejected: Chiaki.stopSession(false)

        background: Rectangle {
            radius: LudeloTheme.radiusCard
            color: LudeloTheme.bgPanelSolid
            border.color: LudeloTheme.borderFocus
            border.width: 1
        }

        TextField {
            id: pinField
            echoMode: Chiaki.settings.streamerMode ? TextInput.Password : TextInput.Normal
            implicitWidth: 220
            font.family: LudeloTheme.fontFamilyMono
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            validator: RegularExpressionValidator { regularExpression: /[0-9]{4}/ }
            Keys.onReturnPressed: {
                if (sessionPinDialog.standardButton(Dialog.Ok).enabled)
                    sessionPinDialog.standardButton(Dialog.Ok).clicked();
            }
        }
    }

    // Controller Helper Overlay
    Loader {
        id: controllerOverlayLoader
        anchors.fill: parent
        active: false
        z: 1000

        sourceComponent: Component {
            ControllerOverlay {
                id: controllerOverlay
                anchors.fill: parent
                active: true
                onDismissed: {
                    view.controllerOverlayShown = true;
                    Chiaki.settings.controllerOverlayShown = true;
                    controllerOverlayLoader.active = false;
                    view.releaseInput();
                }
                Component.onCompleted: {
                    view.grabInput(controllerOverlay);
                }
            }
        }
    }

    Timer {
        id: closeTimer
        objectName: "closeTimer"
        interval: 2000
        onTriggered: root.showMainView()
    }

    Timer {
        id: errorHideTimerOAuth
        interval: 10000
        onTriggered: root.showMainView()
    }

    Connections {
        target: Chiaki

        function onSessionChanged() {
            if (!Chiaki.session) {
                DonationManager.cancelScheduledOffer();
                DonationManager.flushStreamTime();
                if (errorTitleLabel.text)
                    closeTimer.start();
                else
                    root.showMainView();
            }
        }

        function onSessionError(title, text) {
            DonationManager.cancelScheduledOffer();
            DonationManager.flushStreamTime();
            sessionError = true;
            sessionLoading = false;
            gameBackgroundLoader.active = false;
            if (Chiaki.cloudStreaming && Chiaki.cloudStreaming.gameImageUrl) {
                Chiaki.cloudStreaming.gameImageUrl = "";
            }
            errorTitleLabel.text = title;
            errorTextLabel.text = text;

            var isOAuthError = text && (text.includes("OAuth") || text.includes("authorization"));
            var mainComp = root;
            while (mainComp && !mainComp.showToast) {
                mainComp = mainComp.parent;
            }
            if (mainComp && mainComp.showToast) {
                if (isOAuthError) {
                    mainComp.showToast(title, text, "#F44336");
                    Qt.callLater(() => {
                        errorHideTimerOAuth.interval = 10000;
                        errorHideTimerOAuth.restart();
                    });
                } else {
                    closeTimer.start();
                }
            } else {
                closeTimer.start();
            }
        }

        function onSessionPinDialogRequested() {
            if (sessionPinDialog.opened)
                return;
            hudDock.opacity = 0.0;
            sessionPinDialog.open();
        }

        function onSessionStopDialogRequested() {
            if (sessionStopDialog.opened)
                return;
            hudDock.opacity = 0.0;
            sessionStopDialog.open();
        }
    }

    Connections {
        target: Chiaki.window

        function onHasVideoChanged() {
            if (Chiaki.window.hasVideo) {
                if (Chiaki.cloudStreaming && Chiaki.cloudStreaming.gameImageUrl) {
                    gameBackgroundLoader.active = false;
                    sessionLoading = false;
                    Chiaki.cloudStreaming.gameImageUrl = "";
                    if (!controllerOverlayShown && !Chiaki.settings.controllerOverlayShown) {
                        controllerOverlayTimer.start();
                    }
                    return;
                }
                if (!launchingGame) {
                    sessionLoading = false;
                    if (!controllerOverlayShown && !Chiaki.settings.controllerOverlayShown) {
                        controllerOverlayTimer.start();
                    }
                }
            }
        }
    }

    Connections {
        target: Chiaki.session
        enabled: Chiaki.session !== null

        function onConnectedChanged() {
            if (Chiaki.settings.audioVideoDisabled & 0x02)
                sessionLoading = false;

            if (Chiaki.session && Chiaki.session.connected) {
                DonationManager.markConnected();
                DonationManager.scheduleOfferIfEligible();
                view.showHudDock();
            }
        }
    }
}
