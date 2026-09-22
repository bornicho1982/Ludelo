import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Item {
    id: root
    property list<Item> restoreFocusItems
    property bool steamShortcutChecked: false
    Material.theme: Material.Dark
    Material.accent: "#00d4ff"

    function controllerButton(name) {
        let type = "deck";
        for (let i = 0; i < Chiaki.controllers.length; ++i) {
            if (Chiaki.controllers[i].playStation) {
                type = "ps";
                break;
            }
        }
        return "image://svg/button-%1#%2".arg(type).arg(name);
    }
    function grabInput(item) {
        Chiaki.window.grabInput();
        restoreFocusItems.push(Window.window.activeFocusItem);
        if (item)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    function releaseInput() {
        Chiaki.window.releaseInput();
        let item = restoreFocusItems.pop();
        if (item && item.visible)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    function autoRegister(auto, host, ps5) {
        if(auto)
            Chiaki.autoRegister()
        else
            showRegistDialog(host, ps5);
    }

    function openDisplaySettings() {
        if(displaySettingsLoader.item.status == Loader.ready)
        {
            if(placeboSettingsRect.opacity || displaySettingsRect.opacity || colorMappingSettingsRect.opacity)
                closeDialog();
            displaySettingsRect.opacity = 0.8;
            grabInput(displaySettingsLoader);
            if (!displaySettingsLoader.item.restoreFocusItem) {
                let item = displaySettingsLoader.item.mainItem.nextItemInFocusChain();
                if (item)
                    item.forceActiveFocus(Qt.TabFocusReason);
            } else {
                displaySettingsLoader.item.restoreFocusItem.forceActiveFocus(Qt.TabFocusReason);
                displaySettingsLoader.item.restoreFocusItem = null;
            }
        }
    }
    function openPlaceboSettings() {
        if(placeboSettingsLoader.item.status == Loader.ready)
        {
            if(placeboSettingsRect.opacity || displaySettingsRect.opacity || colorMappingSettingsRect.opacity)
                closeDialog();
            placeboSettingsRect.opacity = 0.8;
            grabInput(placeboSettingsLoader);
            if (!placeboSettingsLoader.item.restoreFocusItem) {
                let item = placeboSettingsLoader.item.mainItem.nextItemInFocusChain();
                if (item)
                    item.forceActiveFocus(Qt.TabFocusReason);
            } else {
                placeboSettingsLoader.item.restoreFocusItem.forceActiveFocus(Qt.TabFocusReason);
                placeboSettingsLoader.item.restoreFocusItem = null;
            }
        }
    }

    function closeDialog() {
       if(displaySettingsRect.opacity) {
        displaySettingsRect.opacity = 0.0;
        displaySettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
        releaseInput();
       }
       else if(placeboSettingsRect.opacity) {
        placeboSettingsRect.opacity = 0.0;
        placeboSettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
        releaseInput();
       }
       else if(colorMappingSettingsRect.opacity) {
        releaseInput();
        colorMappingSettingsRect.opacity = 0.0;
        colorMappingSettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
        root.openPlaceboSettings();
       }
       else
        stack.pop();
    }

    function showMainView() {
        if(displaySettingsRect.opacity) {
            displaySettingsRect.opacity = 0.0;
            displaySettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
            releaseInput();
        }
        else if(placeboSettingsRect.opacity) {
            placeboSettingsRect.opacity = 0.0;
            placeboSettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
            releaseInput();
        }
        else if(colorMappingSettingsRect.opacity) {
            colorMappingSettingsRect.opacity = 0.0;
            colorMappingSettingsLoader.item.restoreFocusItem = Window.window.activeFocusItem;
            releaseInput();
        }
        if (stack.depth > 1)
            stack.pop(stack.get(0));
        else
            stack.replace(stack.get(0), mainViewComponent);
        
        // Check if ping timeout dialog should be shown
        if (Chiaki.showPingTimeoutDialog) {
            Qt.callLater(() => {
                root.showMessageDialog(
                    qsTr("Ping Too High"),
                    qsTr("Ping must be less than 80ms to start a cloud session.\n\nTo continue anyway, go to Settings → Cloud and manually select a datacenter for your service (Owned Games or Streamable Games)."),
                    () => {
                        Chiaki.showPingTimeoutDialog = false;
                    }
                );
            });
        }
        
        // Check if authorization failed dialog should be shown
        if (Chiaki.showAuthorizationFailedDialog) {
            Qt.callLater(() => {
                root.showConfirmDialog(
                    qsTr("Authentication Required"),
                    qsTr("Your NPSSO token is likely expired. Please re-login to continue using cloud streaming."),
                    () => {
                        // A button: Open QR login screen
                        Chiaki.showAuthorizationFailedDialog = false;
                        root.showPSNTokenDialog("", true);
                    },
                    () => {
                        // B button: Just close the dialog
                        Chiaki.showAuthorizationFailedDialog = false;
                    }
                );
            });
        }
        
        // Check if PS Plus subscription dialog should be shown
        if (Chiaki.showPSPlusSubscriptionDialog) {
            Qt.callLater(() => {
                root.showMessageDialog(
                    qsTr("PS Plus Subscription Required"),
                    qsTr("You may not have an active PS Plus subscription. Please check your subscription status and try again."),
                    () => {
                        Chiaki.showPSPlusSubscriptionDialog = false;
                    }
                );
            });
        }
        
        // Check if account privacy settings dialog should be shown
        if (Chiaki.showAccountPrivacySettingsDialog) {
            Qt.callLater(() => {
                accountPrivacyDialog.upgradeUrl = Chiaki.accountPrivacyUpgradeUrl || "";
                accountPrivacyDialog.callback = () => {
                    // Ignore Forever - set setting to skip future checks
                    Chiaki.settings.accountAttributesCheckPassed = true;
                    Chiaki.showAccountPrivacySettingsDialog = false;
                    Chiaki.accountPrivacyUpgradeUrl = "";
                };
                accountPrivacyDialog.rejectCallback = () => {
                    // Cancel - just close dialog
                    Chiaki.showAccountPrivacySettingsDialog = false;
                    Chiaki.accountPrivacyUpgradeUrl = "";
                };
                accountPrivacyDialog.restoreFocusItem = Window.window.activeFocusItem;
                accountPrivacyDialog.open();
            });
        }
    }

    function showStreamView() {
        stack.replace(stack.get(0), streamViewComponent);
    }

    function showPsnView() {
        stack.replace(stack.get(0), psnViewComponent, {}, StackView.Immediate)
    }

    function showGamesView(deviceId, deviceName, serverIndex) {
        stack.push(gamesViewComponent, {deviceId: deviceId, deviceName: deviceName, serverIndex: serverIndex})
    }

    function showManualHostDialog() {
        stack.push(manualHostDialogComponent);
    }

    function showConfirmDialog(title, text, callback, rejectCallback = null, keepDialogOpen = false) {
        confirmDialog.title = title;
        confirmDialog.text = text;
        confirmDialog.callback = callback;
        confirmDialog.rejectCallback = rejectCallback;
        confirmDialog.keepDialogOpen = keepDialogOpen;
        confirmDialog.restoreFocusItem = Window.window.activeFocusItem;
        confirmDialog.open();
    }

    function showMessageDialog(title, text, callback) {
        messageDialog.title = title;
        messageDialog.text = text;
        messageDialog.callback = callback;
        messageDialog.restoreFocusItem = Window.window.activeFocusItem;
        messageDialog.open();
    }

    function showToast(title, text, color = "#2196F3") {
        errorTitleLabel.text = title;
        errorTextLabel.text = text;
        if (color === "#4CAF50" || color === "green" || color === "#00F5D4") {
            errorToast.accentColor = LudeloTheme.accentMint;
        } else if (color === "#F44336" || color === "red" || color === "#EF476F") {
            errorToast.accentColor = LudeloTheme.colorDanger;
        } else {
            errorToast.accentColor = LudeloTheme.accent;
        }
        errorHideTimer.restart();
    }



    function showRemindDialog(title, text, remotePlay, callback) {
        remindDialog.title = title;
        remindDialog.text = text;
        remindDialog.remotePlay = remotePlay;
        remindDialog.callback = callback;
        remindDialog.restoreFocusItem = Window.window.activeFocusItem;
        remindDialog.open();
    }

    function showOnboarding() {
        stack.push(onboardingViewComponent);
    }

    function showRegistDialog(host, ps5) {
        stack.push(registDialogComponent, {host: host, ps5: ps5});
    }

    function showSettingsDialog() {
        stack.push(settingsDialogComponent);
    }

    function showDisplaySettingsDialog() {
        stack.push(displaySettingsDialogComponent);
    }

    function showPlaceboSettingsDialog() {
        stack.push(placeboSettingsDialogComponent);
    }

    function showPlaceboColorMappingDialog() {
        if(placeboSettingsRect.opacity)
        {
            root.closeDialog();
            if(colorMappingSettingsLoader.status == Loader.Ready)
            {
                grabInput(colorMappingSettingsLoader);
                colorMappingSettingsRect.opacity = 0.8;
                if (!colorMappingSettingsLoader.item.restoreFocusItem) {
                    let item = colorMappingSettingsLoader.item.mainItem.nextItemInFocusChain();
                    if (item)
                        item.forceActiveFocus(Qt.TabFocusReason);
                } else {
                    colorMappingSettingsLoader.item.restoreFocusItem.forceActiveFocus(Qt.TabFocusReason);
                    colorMappingSettingsLoader.item.restoreFocusItem = null;
                }
            }
        }
        else
            stack.push(placeboColorMappingDialogComponent);
    }

    function showProfileDialog() {
        stack.push(profileDialogComponent)
    }

    function showAccountView() {
        stack.push(accountViewComponent);
    }

    function showConsolePinDialog(consoleIndex) {
        stack.push(consolePinDialogComponent, {consoleIndex: consoleIndex});
    }

    function showSteamShortcutDialog(fromReminder) {
        stack.push(steamShortcutDialogComponent, {fromReminder: fromReminder});
    }

    function showPSNTokenDialog(psnurl, expired) {
        if (Qt.platform.os === "windows") {
            Chiaki.startWebView2Login();
        } else {
            stack.push(psnTokenDialogComponent, {psnurl: psnurl, expired: expired});
        }
    }

    function showControllerMappingDialog() {
        stack.push(controllerMappingDialogComponent)
    }

    function showConsoleSetupWalkthrough() {
        stack.push(consoleSetupWalkthroughComponent)
    }

    function openDonationPrompt() {
        stack.push(donationPromptDialogComponent)
    }

    Component.onCompleted: {
        if (Chiaki.session)
            stack.replace(stack.get(0), streamViewComponent, {}, StackView.Immediate);
        else if (Chiaki.window.directStream)
            stack.replace(stack.get(0), streamViewComponent, {}, StackView.Immediate);
        else if (Chiaki.autoConnect)
            stack.replace(stack.get(0), autoConnectViewComponent, {}, StackView.Immediate);
        else if (Qt.application.arguments.indexOf("--onboarding") !== -1 || !Chiaki.settings.psnAccountId)
            stack.replace(stack.get(0), onboardingViewComponent, {}, StackView.Immediate);
    }

    StackView {
        id: stack
        anchors.fill: parent
        hoverEnabled: false
        initialItem: mainViewComponent
        font.pixelSize: 20

        replaceEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0.01
                to: 1.0
                duration: 200
            }
        }

        replaceExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 200
            }
        }
    }

    Rectangle {
        id: placeboSettingsRect
        opacity: 0.0
        visible: opacity
        height: 650
        width: 1200
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: "transparent"
        
        Loader {
            anchors.fill: parent
            id: placeboSettingsLoader
            sourceComponent: placeboSettingsDialogComponent
        }
    }
    Rectangle {
        id: displaySettingsRect
        opacity: 0.0
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        height: 500
        width: 1200
        visible: opacity
        color: "transparent"
        
        Loader {
            anchors.fill: parent
            id: displaySettingsLoader
            sourceComponent: displaySettingsDialogComponent
        }
    }
    Rectangle {
        id: colorMappingSettingsRect
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: 0.0
        height: 600
        width: 1200
        visible: opacity
        color: "transparent"
        
        Loader {
            anchors.fill: parent
            id: colorMappingSettingsLoader
            sourceComponent: placeboColorMappingDialogComponent
        }
    }
    // Premium Gamer Global Toast Notification
    Rectangle {
        id: errorToast
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 60
        }
        property color accentColor: LudeloTheme.accent
        color: Qt.rgba(0.08, 0.10, 0.16, 0.95)
        border.color: accentColor
        border.width: 1
        width: Math.min(Math.max(errorLayout.implicitWidth + 48, 320), 540)
        height: errorLayout.implicitHeight + 24
        radius: LudeloTheme.radiusCard
        opacity: errorHideTimer.running ? 1.0 : 0.0
        visible: opacity > 0
        z: 9999

        Behavior on opacity { NumberAnimation { duration: LudeloTheme.animNormal; easing.type: LudeloTheme.easingCurve } }

        // Glow
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: errorToast.accentColor
            opacity: 0.25
            z: -1
        }

        RowLayout {
            id: errorLayout
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Status Beacon Dot
            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 5
                color: errorToast.accentColor
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: 8
                    color: parent.color
                    opacity: 0.5
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                Label {
                    id: errorTitleLabel
                    Layout.fillWidth: true
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                    elide: Text.ElideRight
                }

                Label {
                    id: errorTextLabel
                    Layout.fillWidth: true
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 11
                    color: LudeloTheme.textSecondary
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }

        // Animated progress bar at bottom
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            height: 2
            color: errorToast.accentColor
            width: errorHideTimer.running ? parent.width : 0

            Behavior on width {
                enabled: errorHideTimer.running
                NumberAnimation {
                    from: errorToast.width
                    to: 0
                    duration: errorHideTimer.interval
                }
            }
        }

        Timer {
            id: errorHideTimer
            interval: 3500
        }
    }

    ConfirmDialog {
        id: confirmDialog
    }

    MessageDialog {
        id: messageDialog
    }

    AccountPrivacyDialog {
        id: accountPrivacyDialog
    }

    GamingModeAddedDialog {
        id: gamingModeAddedDialog
    }

    RemindDialog {
        id: remindDialog
    }

    Connections {
        target: Chiaki

        function onSessionChanged() {
            if (Chiaki.session)
                root.showStreamView();
        }

        function onShowPsnView() {
            root.showPsnView();
        }

        function onPsnCredsExpired() {
            Chiaki.settings.psnRefreshToken = ""
            Chiaki.settings.psnAuthToken = ""
            Chiaki.settings.psnAuthTokenExpiry = ""
            Chiaki.settings.psnAccountId = ""
            root.showPSNTokenDialog(true);
        }

        function onError(title, text, durationMs) {
            root.showToast(title, text, "#F44336");
            if (durationMs !== undefined)
                errorHideTimer.interval = durationMs;
        }

        function onPsnLoginAccountIdError(error) {
            root.showToast(qsTr("Login Notice"), error, "#F44336");
        }

        function onPsnGamesSynced(newGamesCount) {
            var msg = newGamesCount === 1 
                ? qsTr("1 game added") 
                : qsTr("%1 games added").arg(newGamesCount);
            root.showToast(qsTr("Games Synced"), msg, "#4CAF50");
        }

        function onPsnGamesCleared(gamesCount) {
            var msg = gamesCount === 0 
                ? qsTr("No saved games to clear")
                : gamesCount === 1 
                    ? qsTr("1 game cleared") 
                    : qsTr("%1 games cleared").arg(gamesCount);
            root.showToast(qsTr("Games Cleared"), msg, "#2196F3");
        }

        function onRegistDialogRequested(host, ps5, duid) {
            // Check if user is logged into PSN
            const isPsnLoggedIn = Chiaki.settings.psnAuthToken && Chiaki.settings.psnAuthToken !== "";
            
            if(!isPsnLoggedIn) {
                // Not logged in to PSN - ask if they want to login for automatic setup or manually register
                root.showConfirmDialog(
                    qsTr("Console Setup"), 
                    qsTr("Would you like to login for automatic console setup?\n\nChoose 'Yes' to login for automatic registration.\nChoose 'No' to manually enter console information."),
                    () => root.showPSNTokenDialog("", false),  // Yes - show PSN login
                    () => showRegistDialog(host, ps5)          // No - show manual registration
                )
            }
            else if(!duid) {
                // Logged in to PSN but console was discovered locally (no duid)
                // Can only do manual registration in this case
                showRegistDialog(host, ps5);
            }
            else {
                // Logged in to PSN and console has duid - can do automatic registration
                if(ps5)
                    root.showConfirmDialog(qsTr("Registration Type"), qsTr("Would you like to use automatic registration?"), () => root.autoRegister(true, host, ps5), () => root.autoRegister(false, host, ps5))
                else
                    root.showConfirmDialog(qsTr("Registration Type"), qsTr("Would you like to use automatic registration (must be main PS4 console registered to your account)?"), () => root.autoRegister(true, host, ps5), () => root.autoRegister(false, host, ps5))
            }
        }

        function onWakeupStartInitiated() {
            stack.replace(stack.get(0), autoConnectViewComponent, {}, StackView.Immediate);
        }

    }

    Component {
        id: mainViewComponent
        MainView { }
    }

    Component {
        id: streamViewComponent
        StreamView { }
    }

    Component {
        id: autoConnectViewComponent
        AutoConnectView { }
    }

    Component {
        id: psnViewComponent
        PsnView {}
    }

    Component {
        id: gamesViewComponent
        GamesView {}
    }

    Component {
        id: manualHostDialogComponent
        ManualHostDialog { }
    }

    Component {
        id: accountViewComponent
        AccountView {}
    }

    Component {
        id: settingsDialogComponent
        SettingsDialog { }
    }

    Component {
        id: displaySettingsDialogComponent
        DisplaySettingsDialog { }
    }

    Component {
        id: placeboSettingsDialogComponent
        PlaceboSettingsDialog { }
    }

    Component {
        id: placeboColorMappingDialogComponent
        PlaceboColorMappingDialog { }
    }

    Component {
        id: profileDialogComponent
        ProfileDialog { }
    }

    Component {
        id: consolePinDialogComponent
        ConsolePinDialog { }
    }

    Component {
        id: steamShortcutDialogComponent
        SteamShortcutDialog { }
    }

    Component {
        id: psnTokenDialogComponent
        PSNTokenDialog { }
    }



    Component {
        id: registDialogComponent
        RegistDialog { }
    }

    Component {
        id: controllerMappingDialogComponent
        ControllerMappingDialog { }
    }

    Component {
        id: consoleSetupWalkthroughComponent
        ConsoleSetupWalkthrough { }
    }

    Component {
        id: donationPromptDialogComponent
        DonationPromptDialog { }
    }

    Component {
        id: onboardingViewComponent
        OnboardingView {
            onSkipRequested: {
                if (stack.depth > 1)
                    stack.pop();
                else
                    stack.replace(stack.get(0), mainViewComponent, {}, StackView.Immediate);
            }
            onLoginCompleted: function(id) {
                if (stack.depth > 1)
                    stack.pop();
                else
                    stack.replace(stack.get(0), mainViewComponent, {}, StackView.Immediate);
            }
        }
    }

    Connections {
        target: DonationManager
        enabled: DonationManager.enabled

        function onShowDonationPromptChanged() {
            if (DonationManager.showDonationPrompt)
                openDonationPrompt();
        }
    }

    // =========================================================================
    // Frameless Window Resize Borders (Active globally when not maximized)
    // =========================================================================
    Item {
        id: resizeBorders
        anchors.fill: parent
        z: 99999
        visible: typeof Chiaki !== "undefined" && Chiaki.window &&
                 Chiaki.window.visibility !== Window.Maximized &&
                 Chiaki.window.visibility !== Window.FullScreen

        readonly property int borderThickness: 6
        readonly property int cornerSize: 14

        // Edges
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: resizeBorders.cornerSize
            anchors.rightMargin: resizeBorders.cornerSize
            height: resizeBorders.borderThickness
            cursorShape: Qt.SizeVerCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.TopEdge)
        }
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: resizeBorders.cornerSize
            anchors.rightMargin: resizeBorders.cornerSize
            height: resizeBorders.borderThickness
            cursorShape: Qt.SizeVerCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.BottomEdge)
        }
        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: resizeBorders.cornerSize
            anchors.bottomMargin: resizeBorders.cornerSize
            width: resizeBorders.borderThickness
            cursorShape: Qt.SizeHorCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.LeftEdge)
        }
        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: resizeBorders.cornerSize
            anchors.bottomMargin: resizeBorders.cornerSize
            width: resizeBorders.borderThickness
            cursorShape: Qt.SizeHorCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.RightEdge)
        }

        // Corners
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            width: resizeBorders.cornerSize
            height: resizeBorders.cornerSize
            cursorShape: Qt.SizeFDiagCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.TopEdge | Qt.LeftEdge)
        }
        MouseArea {
            anchors.top: parent.top
            anchors.right: parent.right
            width: resizeBorders.cornerSize
            height: resizeBorders.cornerSize
            cursorShape: Qt.SizeBDiagCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.TopEdge | Qt.RightEdge)
        }
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: resizeBorders.cornerSize
            height: resizeBorders.cornerSize
            cursorShape: Qt.SizeBDiagCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.BottomEdge | Qt.LeftEdge)
        }
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: resizeBorders.cornerSize
            height: resizeBorders.cornerSize
            cursorShape: Qt.SizeFDiagCursor
            acceptedButtons: Qt.LeftButton
            onPressed: if (Chiaki.window && typeof Chiaki.window.startResize === "function") Chiaki.window.startResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }
}
