import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects

import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Pane {
    id: consolePane
    padding: 0
    focus: true
    activeFocusOnTab: true

    // Filter state: "all", "ps5", "ps4"
    property string activeFilter: "all"

    // Discovered host counts
    readonly property int allCount: Chiaki.hosts ? Chiaki.hosts.length : 0
    readonly property int ps5Count: {
        if (!Chiaki.hosts) return 0;
        var count = 0;
        for (var i = 0; i < Chiaki.hosts.length; i++) {
            if (Chiaki.hosts[i].ps5) count++;
        }
        return count;
    }
    readonly property int ps4Count: {
        if (!Chiaki.hosts) return 0;
        var count = 0;
        for (var i = 0; i < Chiaki.hosts.length; i++) {
            if (!Chiaki.hosts[i].ps5) count++;
        }
        return count;
    }

    // Filtered model with virtual cards appended
    property var displayModel: {
        var raw = Chiaki.hosts || [];
        var res = [];
        for (var i = 0; i < raw.length; i++) {
            var h = Object.assign({}, raw[i]);
            h.originalIndex = i;
            h.itemType = "console";
            if (activeFilter === "all") {
                res.push(h);
            } else if (activeFilter === "ps5" && h.ps5) {
                res.push(h);
            } else if (activeFilter === "ps4" && !h.ps5) {
                res.push(h);
            }
        }
        // Always include "+ Add New Console / Manual IP"
        res.push({
            itemType: "add_manual",
            name: qsTr("Add New Console / Manual IP")
        });
        // Always include "Hardware Acceleration HUD"
        res.push({
            itemType: "telemetry",
            name: qsTr("Hardware Acceleration HUD")
        });
        return res;
    }

    // Deep Dark Gamer Backdrop
    Rectangle {
        anchors.fill: parent
        color: LudeloTheme.bgBase
        z: -2

        // Top-left subtle indigo atmospheric aura
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            width: 700
            height: 500
            radius: 350
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.08)
            z: 0
        }

        // Top-right subtle mint aura
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 600
            height: 450
            radius: 300
            color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.03)
            z: 0
        }
    }

    StackView.onActivated: {
        Qt.callLater(() => {
            if (StackView.status !== StackView.Active)
                return;
            if (StackView.view && StackView.view.depth > 1)
                return;

            if (mainTabBar.currentIndex === 1) {
                if (cloudPlayLoader.active) {
                    var cloudPlayView = cloudPlayLoader.item;
                    if (cloudPlayView && cloudPlayView.catalogButtonItem) {
                        cloudPlayView.catalogButtonItem.forceActiveFocus(Qt.TabFocusReason);
                        return;
                    }
                }
            } else {
                if (hostsView.count > 0) {
                    hostsView.currentIndex = 0;
                    hostsView.selectedIndex = 0;
                    hostsView.forceActiveFocus(Qt.TabFocusReason);
                }
            }
        });

        if (Chiaki.autoConnect || Chiaki.window.directStream)
            return;

        if (!root.steamShortcutChecked && typeof Chiaki.ensureLudeloSteamShortcut === "function") {
            root.steamShortcutChecked = true;
            Chiaki.ensureLudeloSteamShortcut((created) => {
                if (created)
                    gamingModeAddedDialog.open();
            });
        }
    }

    Keys.onMenuPressed: root.showSettingsDialog()
    Keys.onReturnPressed: {
        if (hostsView.activeFocus && hostsView.currentItem) {
            hostsView.currentItem.connectToHost();
        } else {
            event.accepted = false;
        }
    }
    Keys.onEscapePressed: root.showConfirmDialog(qsTr("Quit"), qsTr("Are you sure you want to quit?"), () => Qt.quit(), null, true)
    Keys.onPressed: (event) => {
        if (event.modifiers)
            return;
        switch (event.key) {
        case Qt.Key_PageUp:
            if (mainTabBar.currentIndex > 0) {
                mainTabBar.currentIndex = 0;
                event.accepted = true;
            } else if (hostsView.activeFocus && hostsView.currentItem) {
                hostsView.currentItem.setConsolePin();
                event.accepted = true;
            }
            break;
        case Qt.Key_PageDown:
            if (mainTabBar.currentIndex < 1) {
                mainTabBar.currentIndex = 1;
                event.accepted = true;
            } else if (Chiaki.settings.psnAuthToken) {
                Chiaki.refreshPsnToken();
                event.accepted = true;
            } else {
                root.showPSNTokenDialog("", false);
                event.accepted = true;
            }
            break;
        case Qt.Key_F1:
            if (typeof Chiaki.createSteamShortcut === "function") root.showSteamShortcutDialog(false);
            event.accepted = true;
            break;
        case Qt.Key_F2:
            root.showManualHostDialog();
            event.accepted = true;
            break;
        case Qt.Key_F5:
            Chiaki.discoveryEnabled = true;
            event.accepted = true;
            break;
        }
    }

    // Top Bar (Ludelo Premium Gamer Header)
    Rectangle {
        id: headerBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 64
        color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.95)
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        z: 10

        // Bottom Glow Accent line
        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: 1
            color: LudeloTheme.borderSubtle
        }

        // Background drag and double-click maximize area
        MouseArea {
            anchors.fill: parent
            z: 0
            acceptedButtons: Qt.LeftButton
            onPressed: {
                console.log("[window] MainView header bgDragArea onPressed triggered");
                if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                    Chiaki.window.startDrag();
                }
            }
            onDoubleClicked: {
                console.log("[window] MainView header bgDragArea onDoubleClicked triggered");
                if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                    Chiaki.window.toggleMaximize();
                }
            }
        }

        RowLayout {
            z: 1
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            spacing: 20

            // Brand Section (Left - Draggable)
            Item {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: brandRow.implicitWidth
                implicitHeight: 36

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onPressed: {
                        console.log("[window] MainView header brand onPressed triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                            Chiaki.window.startDrag();
                        }
                    }
                    onDoubleClicked: {
                        console.log("[window] MainView header brand onDoubleClicked triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                            Chiaki.window.toggleMaximize();
                        }
                    }
                }

                Row {
                    id: brandRow
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    Rectangle {
                        width: 36
                        height: 36
                        radius: 8
                        color: LudeloTheme.bgElevated
                        border.color: LudeloTheme.borderHover
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: "qrc:/icons/logo_square_1024.png"
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 1

                        Text {
                            text: "LUDELO"
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                            font.letterSpacing: 1.5
                        }

                        Text {
                            text: "REMOTE PLAY CLIENT v2.4"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 9
                            color: LudeloTheme.textDim
                            font.letterSpacing: 0.8
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onPressed: {
                        console.log("[window] MainView header spacer 1 onPressed triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                            Chiaki.window.startDrag();
                        }
                    }
                    onDoubleClicked: {
                        console.log("[window] MainView header spacer 1 onDoubleClicked triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                            Chiaki.window.toggleMaximize();
                        }
                    }
                }
            }

            // Center Navigation Tabs
            Row {
                id: mainTabBar
                property int currentIndex: 0
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                Component.onCompleted: {
                    var savedTab = Chiaki.settings.lastSelectedMainTab;
                    if (savedTab >= 0 && savedTab <= 1) {
                        currentIndex = savedTab;
                    }
                }
                onCurrentIndexChanged: {
                    Chiaki.settings.lastSelectedMainTab = currentIndex;
                }

                LNavTab {
                    text: qsTr("Consoles")
                    active: mainTabBar.currentIndex === 0
                    onClicked: mainTabBar.currentIndex = 0
                }

                LNavTab {
                    text: qsTr("Cloud Play")
                    active: mainTabBar.currentIndex === 1
                    onClicked: mainTabBar.currentIndex = 1
                }

                LNavTab {
                    text: qsTr("Settings")
                    active: false
                    onClicked: root.showSettingsDialog()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onPressed: {
                        console.log("[window] MainView header spacer 2 onPressed triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                            Chiaki.window.startDrag();
                        }
                    }
                    onDoubleClicked: {
                        console.log("[window] MainView header spacer 2 onDoubleClicked triggered");
                        if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                            Chiaki.window.toggleMaximize();
                        }
                    }
                }
            }

            // Right Actions & Telemetry Badges
            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                // Stream Ready / Decoder Pill
                LPill {
                    text: qsTr("STREAM READY • %1").arg(Chiaki.settings.decoder ? Chiaki.settings.decoder.toUpperCase() : "D3D11VA")
                    dotColor: LudeloTheme.accentPrimary
                    glowColor: LudeloTheme.accentGlow
                    showDot: true
                }

                // PSN Login / Account Status Pill
                MouseArea {
                    width: psnPill.implicitWidth
                    height: psnPill.implicitHeight
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    LPill {
                        id: psnPill
                        anchors.fill: parent
                        text: Chiaki.settings.psnAuthToken
                            ? (Chiaki.settings.psnAccountId ? LudeloTheme.formatObfuscatedAccountId(Chiaki.settings.psnAccountId) : qsTr("PSN CONNECTED"))
                            : qsTr("SIGN IN PSN")
                        dotColor: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.warn
                        glowColor: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMintGlow : Qt.rgba(1, 0.7, 0, 0.4)
                        pulseDot: Chiaki.settings.psnAuthToken !== ""
                        showDot: true
                    }

                    onClicked: {
                        if (typeof root.showAccountView === "function") {
                            root.showAccountView();
                        } else if (Chiaki.settings.psnAuthToken) {
                            Chiaki.refreshPsnToken();
                        } else {
                            root.showPSNTokenDialog("", false);
                        }
                    }
                }

                // Settings Button
                Rectangle {
                    width: 36
                    height: 36
                    radius: 8
                    color: settingsMouse.containsMouse ? LudeloTheme.bgElevated : "transparent"
                    border.color: settingsMouse.containsMouse ? LudeloTheme.borderHover : "transparent"
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        source: "qrc:/icons/settings-20px.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    MouseArea {
                        id: settingsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.showSettingsDialog()
                    }
                }

                // Minimize Button
                Rectangle {
                    width: 36
                    height: 36
                    radius: 8
                    color: minMouse.containsMouse ? LudeloTheme.bgElevated : "transparent"
                    border.color: minMouse.containsMouse ? LudeloTheme.borderHover : "transparent"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "—"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        color: minMouse.containsMouse ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        id: minMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.showMinimized();
                        }
                    }
                }

                // Maximize / Restore Button
                Rectangle {
                    width: 36
                    height: 36
                    radius: 8
                    color: maxMouse.containsMouse ? LudeloTheme.bgElevated : "transparent"
                    border.color: maxMouse.containsMouse ? LudeloTheme.borderHover : "transparent"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "□"
                        font.pixelSize: 14
                        color: maxMouse.containsMouse ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        id: maxMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof Chiaki !== "undefined" && Chiaki.window) {
                                if (typeof Chiaki.window.toggleMaximize === "function")
                                    Chiaki.window.toggleMaximize();
                                else
                                    Chiaki.window.showMaximized();
                            }
                        }
                    }
                }

                // Exit Button
                Rectangle {
                    width: 36
                    height: 36
                    radius: 8
                    color: closeMouse.containsMouse ? LudeloTheme.error : "transparent"
                    border.color: closeMouse.containsMouse ? LudeloTheme.error : "transparent"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 14
                        color: closeMouse.containsMouse ? "#FFFFFF" : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof Chiaki !== "undefined" && Chiaki.window) Chiaki.window.close();
                            else Qt.quit();
                        }
                    }
                }
            }
        }
    }

    // Main Tab Stack
    StackLayout {
        id: mainTabView
        anchors {
            top: headerBar.bottom
            left: parent.left
            right: parent.right
            bottom: buttonHintsFooter.top
        }
        currentIndex: mainTabBar.currentIndex

        // TAB 0: Remote Play / Consoles Dashboard
        Item {
            // Sub-header Area (Network Topology & Filters)
            Item {
                id: subHeaderArea
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: 28
                    topMargin: 20
                    bottomMargin: 10
                }
                height: 72

                RowLayout {
                    anchors.fill: parent
                    spacing: 20

                    // Left Side: Title & Discovery Info
                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4

                        Row {
                            spacing: 8
                            Text {
                                text: Chiaki.discoveryEnabled
                                    ? "NETWORK TOPOLOGY DETECTED • DIRECT P2P READY"
                                    : "LOCAL DISCOVERY INACTIVE"
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: Chiaki.discoveryEnabled ? LudeloTheme.accentMint : LudeloTheme.warn
                                font.letterSpacing: 1
                            }
                        }

                        Text {
                            text: qsTr("Discovered Hardware")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                        }

                        Text {
                            text: qsTr("Select a console device to initiate a direct stream session")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 13
                            color: LudeloTheme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true } // Spacer

                    // Right Side: Scan Subnet & Real Filter Chips
                    Column {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        spacing: 8

                        // Scan Subnet Button
                        LButton {
                            anchors.right: parent.right
                            height: 36
                            variant: "secondary"
                            customRadius: 8
                            text: qsTr("Scan Subnet")
                            keyHint: "[F5]"
                            iconSource: "qrc:/icons/discover-24px.svg"
                            onClicked: Chiaki.discoveryEnabled = true
                        }

                        // Filter Chips: All, PS5, PS4
                        Row {
                            anchors.right: parent.right
                            spacing: 8

                            // All Filter Chip
                            Rectangle {
                                height: 26
                                width: allChipText.implicitWidth + 20
                                radius: 6
                                color: consolePane.activeFilter === "all" ? LudeloTheme.accentElevated || Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                                border.color: consolePane.activeFilter === "all" ? LudeloTheme.accentPrimary : LudeloTheme.borderSubtle
                                border.width: 1

                                Text {
                                    id: allChipText
                                    anchors.centerIn: parent
                                    text: qsTr("All (%1)").arg(consolePane.allCount)
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 11
                                    font.weight: consolePane.activeFilter === "all" ? Font.Bold : Font.Normal
                                    color: consolePane.activeFilter === "all" ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: consolePane.activeFilter = "all"
                                }
                            }

                            // PS5 Filter Chip
                            Rectangle {
                                height: 26
                                width: ps5ChipText.implicitWidth + 20
                                radius: 6
                                color: consolePane.activeFilter === "ps5" ? Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                                border.color: consolePane.activeFilter === "ps5" ? LudeloTheme.accentPrimary : LudeloTheme.borderSubtle
                                border.width: 1

                                Text {
                                    id: ps5ChipText
                                    anchors.centerIn: parent
                                    text: qsTr("PS5 (%1)").arg(consolePane.ps5Count)
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 11
                                    font.weight: consolePane.activeFilter === "ps5" ? Font.Bold : Font.Normal
                                    color: consolePane.activeFilter === "ps5" ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: consolePane.activeFilter = "ps5"
                                }
                            }

                            // PS4 Filter Chip
                            Rectangle {
                                height: 26
                                width: ps4ChipText.implicitWidth + 20
                                radius: 6
                                color: consolePane.activeFilter === "ps4" ? Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                                border.color: consolePane.activeFilter === "ps4" ? LudeloTheme.accentPrimary : LudeloTheme.borderSubtle
                                border.width: 1

                                Text {
                                    id: ps4ChipText
                                    anchors.centerIn: parent
                                    text: qsTr("PS4 (%1)").arg(consolePane.ps4Count)
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 11
                                    font.weight: consolePane.activeFilter === "ps4" ? Font.Bold : Font.Normal
                                    color: consolePane.activeFilter === "ps4" ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: consolePane.activeFilter = "ps4"
                                }
                            }
                        }
                    }
                }
            }

            // Scrollable Grid of Consoles & Virtual Cards
            ScrollView {
                id: scrollView
                anchors {
                    top: subHeaderArea.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: 28
                    topMargin: 12
                }
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: ScrollBar {
                    width: 8
                    policy: ScrollBar.AsNeeded
                    background: Rectangle {
                        color: Qt.rgba(255, 255, 255, 0.04)
                        radius: 4
                    }
                    contentItem: Rectangle {
                        radius: 4
                        color: LudeloTheme.accentPrimary
                        opacity: 0.7
                    }
                }

                GridView {
                    id: hostsView
                    keyNavigationWraps: true
                    cellWidth: {
                        if (width >= 1200) return width / 3;
                        if (width >= 780) return width / 2;
                        return width;
                    }
                    cellHeight: 340
                    model: consolePane.displayModel

                    property int selectedIndex: -1

                    onActiveFocusChanged: {
                        if (activeFocus && count > 0 && currentIndex < 0) {
                            currentIndex = 0;
                            selectedIndex = 0;
                        }
                    }

                    Keys.onLeftPressed: {
                        if (selectedIndex > 0) {
                            selectedIndex--;
                            currentIndex = selectedIndex;
                        }
                    }
                    Keys.onRightPressed: {
                        if (selectedIndex < count - 1) {
                            selectedIndex++;
                            currentIndex = selectedIndex;
                        }
                    }
                    Keys.onUpPressed: {
                        var itemsPerRow = Math.max(1, Math.floor(width / cellWidth));
                        var newIndex = Math.max(0, selectedIndex - itemsPerRow);
                        if (newIndex === selectedIndex) {
                            if (mainTabBar) mainTabBar.forceActiveFocus();
                            selectedIndex = -1;
                        } else {
                            selectedIndex = newIndex;
                            currentIndex = selectedIndex;
                        }
                    }
                    Keys.onDownPressed: {
                        var itemsPerRow = Math.max(1, Math.floor(width / cellWidth));
                        var newIndex = Math.min(count - 1, selectedIndex + itemsPerRow);
                        selectedIndex = newIndex;
                        currentIndex = selectedIndex;
                    }

                    Keys.onReturnPressed: {
                        if (currentItem && currentItem.connectToHost) {
                            currentItem.connectToHost();
                        }
                    }

                    // Console action shortcuts
                    Keys.onPressed: (event) => {
                        if (event.modifiers)
                            return;
                        switch (event.key) {
                        case Qt.Key_Backslash:
                        case Qt.Key_No:
                            if (currentItem && currentItem.triggerFirstAction && currentItem.triggerFirstAction())
                                event.accepted = true;
                            break;
                        case Qt.Key_C:
                        case Qt.Key_Yes:
                            if (currentItem && currentItem.hasGames && currentItem.viewGames) {
                                currentItem.viewGames();
                                event.accepted = true;
                            } else if (currentItem && currentItem.triggerSecondAction && currentItem.triggerSecondAction()) {
                                event.accepted = true;
                            }
                            break;
                        case Qt.Key_Backspace:
                            if (currentItem && currentItem.canHide) {
                                currentItem.deleteHost();
                                event.accepted = true;
                            }
                            break;
                        }
                    }

                    delegate: Loader {
                        id: delegateLoader
                        width: hostsView.cellWidth
                        height: hostsView.cellHeight

                        property var hostData: modelData
                        property int itemIndex: index
                        property bool isSelected: hostsView.selectedIndex === index

                        sourceComponent: {
                            if (modelData.itemType === "add_manual") return addManualCardComponent;
                            if (modelData.itemType === "telemetry") return telemetryCardComponent;
                            return consoleCardComponent;
                        }

                        // Forward properties & actions to child item
                        property bool canHide: (item && item.canHide) ? true : false
                        property bool canWake: (item && item.canWake) ? true : false
                        property bool canPin: (item && item.canPin) ? true : false
                        property bool hasGames: (item && item.hasGames) ? true : false

                        function connectToHost() { if (item && item.connectToHost) item.connectToHost(); }
                        function wakeUpHost() { if (item && item.wakeUpHost) item.wakeUpHost(); }
                        function deleteHost() { if (item && item.deleteHost) item.deleteHost(); }
                        function setConsolePin() { if (item && item.setConsolePin) item.setConsolePin(); }
                        function viewGames() { if (item && item.viewGames) item.viewGames(); }
                        function triggerFirstAction() { if (item && item.triggerFirstAction) return item.triggerFirstAction(); return false; }
                        function triggerSecondAction() { if (item && item.triggerSecondAction) return item.triggerSecondAction(); return false; }
                    }
                }
            }

            // Empty State (Shown when 0 consoles found and no search results)
            Rectangle {
                id: noConsolesDialog
                anchors.centerIn: parent
                width: 520
                height: 380
                radius: LudeloTheme.radiusCard
                color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.95)
                border.color: LudeloTheme.borderHover
                border.width: 1
                visible: consolePane.allCount === 0 && mainTabBar.currentIndex === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    width: 440
                    spacing: 16

                    // Emblem
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 54
                        height: 54
                        radius: 27
                        color: LudeloTheme.bgElevated
                        border.color: LudeloTheme.accentPrimary
                        border.width: 1

                        Image {
                            anchors.centerIn: parent
                            width: 28
                            height: 28
                            source: "qrc:/icons/discover-off-24px.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("No Consoles Discovered")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 22
                        font.weight: Font.Bold
                        color: LudeloTheme.textPrimary
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        text: qsTr("Make sure Remote Play is enabled on your PS5 / PS4 and both devices are on the same network.")
                        font.pixelSize: 13
                        color: LudeloTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    // Instruction snippet
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        radius: 8
                        color: Qt.rgba(0, 0, 0, 0.3)
                        border.color: LudeloTheme.borderSubtle

                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "PS5: Settings → System → Remote Play"
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "PS4: Settings → Remote Play Connection Settings"
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 8
                        spacing: 12

                        LButton {
                            Layout.preferredWidth: 200
                            height: 44
                            variant: "primary"
                            text: qsTr("Pair with PIN")
                            keyHint: "[F2]"
                            onClicked: root.showManualHostDialog()
                        }

                        LButton {
                            Layout.preferredWidth: 200
                            height: 44
                            variant: "secondary"
                            text: qsTr("Rescan Network")
                            keyHint: "[F5]"
                            onClicked: Chiaki.discoveryEnabled = true
                        }
                    }
                }
            }
        }

        // TAB 1: Cloud Play Tab
        Loader {
            id: cloudPlayLoader
            source: "CloudPlayView.qml"
            active: mainTabBar.currentIndex === 1
            onItemChanged: {
                if (item) {
                    item.mainTabBar = mainTabBar;
                    item.showConfirmDialogFunc = root.showConfirmDialog;
                }
            }
            onLoaded: {
                if (item) {
                    item.mainTabBar = mainTabBar;
                    item.showConfirmDialogFunc = root.showConfirmDialog;
                    if (mainTabBar.currentIndex === 1) {
                        Qt.callLater(() => {
                            item.loadUnifiedCatalog();
                        });
                    }
                }
            }
        }
    }

    // Component: Console Card (Real Data Only - No Fake Stitch Fields)
    Component {
        id: consoleCardComponent

        Item {
            id: cardDelegateRoot
            width: hostsView.cellWidth
            height: hostsView.cellHeight

            // Source hostData from the Loader parent (P0.1 fix: modelData is not in lexical scope for root-level Components)
            property var hostData: parent ? parent.hostData : ({})
            property int hostIndex: hostData.originalIndex !== undefined ? hostData.originalIndex : (parent ? parent.itemIndex : 0)

            property bool canHide: hostData.manual || (hostData.discovered && !hostData.registered)
            property bool canWake: hostData.registered && !hostData.duid && !hostData.discovered
            property bool canPin: hostData.registered
            property bool hasGames: {
                if (!hostData.duid) return false;
                var gamesJson = Chiaki.getPsnInstalledGames();
                if (!gamesJson || gamesJson === "{}") return false;
                try {
                    var devices = JSON.parse(gamesJson);
                    var device = devices[hostData.duid];
                    return device && device.games && device.games.length > 0;
                } catch (e) {
                    return false;
                }
            }

            function connectToHost() {
                if (hostData.discovered)
                    Chiaki.connectToHost(hostIndex, hostData.name);
                else
                    Chiaki.connectToHost(hostIndex);
            }

            function wakeUpHost() {
                if (!hostData.discovered && !hostData.duid)
                    Chiaki.wakeUpHost(hostIndex);
            }

            function deleteHost() {
                if (hostData.manual)
                    root.showConfirmDialog(qsTr("Delete Console"), qsTr("Are you sure you want to delete this console?"), () => Chiaki.deleteHost(hostIndex));
                else if (hostData.discovered && !hostData.registered)
                    root.showConfirmDialog(qsTr("Hide Console"), qsTr("Are you sure you want to hide this console?") + "\n\n" + qsTr("Note: You can unhide from the Consoles section of the Settings under Hidden Consoles"), () => Chiaki.hideHost(hostData.mac, hostData.name));
            }

            function setConsolePin() {
                root.showConsolePinDialog(hostIndex);
            }

            function viewGames() {
                if (hostData.duid) {
                    root.showGamesView(hostData.duid, hostData.name, hostIndex);
                }
            }

            function triggerFirstAction() {
                if (canHide) { deleteHost(); return true; }
                if (canWake) { wakeUpHost(); return true; }
                if (canPin) { setConsolePin(); return true; }
                return false;
            }

            function triggerSecondAction() {
                var seen = 0;
                if (canHide) { seen++; if (seen === 2) { deleteHost(); return true; } }
                if (canWake) { seen++; if (seen === 2) { wakeUpHost(); return true; } }
                if (canPin)  { seen++; if (seen === 2) { setConsolePin(); return true; } }
                return false;
            }

            LCard {
                anchors.fill: parent
                anchors.margins: 10
                customRadius: 16
                hoverLift: true
                cardColor: LudeloTheme.bgCard
                glowColor: hostData.state === "ready" ? LudeloTheme.accentGlow : (hostData.state === "standby" ? Qt.rgba(1, 0.7, 0, 0.3) : LudeloTheme.accentGlow)
                borderColor: hostsView.selectedIndex === (parent ? parent.itemIndex : -1) ? LudeloTheme.borderFocus : LudeloTheme.borderSubtle
                onClicked: {
                    var idx = cardDelegateRoot.parent ? cardDelegateRoot.parent.itemIndex : 0;
                    hostsView.currentIndex = idx;
                    hostsView.selectedIndex = idx;
                    hostsView.forceActiveFocus();
                    cardDelegateRoot.connectToHost();
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12

                    // Card Header: Console Silhouette + Status Pill
                    RowLayout {
                        Layout.fillWidth: true

                        // Console Icon
                        Rectangle {
                            width: 44
                            height: 44
                            radius: 10
                            color: LudeloTheme.bgElevated
                            border.color: LudeloTheme.borderSubtle
                            border.width: 1

                            Image {
                                anchors.centerIn: parent
                                width: 28
                                height: 28
                                source: "image://svg/console-ps" + (hostData.ps5 ? "5" : "4") + (hostData.state === "standby" ? "#light_standby" : "#light_on")
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        Item { Layout.fillWidth: true } // Spacer

                        // Status Pill (Online, Standby, Offline)
                        LPill {
                            text: {
                                if (hostData.state === "ready") return qsTr("ONLINE");
                                if (hostData.state === "standby") return qsTr("STANDBY (REST MODE)");
                                return qsTr("OFFLINE");
                            }
                            dotColor: {
                                if (hostData.state === "ready") return LudeloTheme.accentMint;
                                if (hostData.state === "standby") return LudeloTheme.warn;
                                return LudeloTheme.textDim;
                            }
                            glowColor: {
                                if (hostData.state === "ready") return LudeloTheme.accentMintGlow;
                                if (hostData.state === "standby") return Qt.rgba(1, 0.7, 0, 0.4);
                                return "transparent";
                            }
                            pulseDot: hostData.state === "ready"
                            showDot: true
                        }
                    }

                    // Console Name & Network Line
                    Column {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            width: parent.width
                            text: hostData.name || (hostData.ps5 ? "PlayStation 5" : "PlayStation 4")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: (hostData.address ? (Chiaki.settings.streamerMode ? "IP: hidden" : hostData.address) : "Remote P2P") + (hostData.discovered ? " • Local DDP Subnet" : " • Manual / Remote")
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            color: LudeloTheme.textSecondary
                            elide: Text.ElideRight
                        }
                    }

                    // Real Specs Grid (Only real, verifiable telemetry)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 74
                        radius: 8
                        color: Qt.rgba(0, 0, 0, 0.3)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 16

                            Text {
                                text: qsTr("Power State:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 11
                                color: LudeloTheme.textDim
                            }
                            Text {
                                text: hostData.state === "ready" ? qsTr("Ready (Online)") : (hostData.state === "standby" ? qsTr("Rest Mode (Sleep)") : (hostData.state ? hostData.state : qsTr("Standby / Sleep")))
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                color: hostData.state === "ready" ? LudeloTheme.accentMint : (hostData.state === "standby" ? LudeloTheme.warn : LudeloTheme.textSecondary)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("Registration:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 11
                                color: LudeloTheme.textDim
                            }
                            Text {
                                text: hostData.registered ? qsTr("Paired & Registered") : qsTr("PIN Pairing Required")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                color: hostData.registered ? LudeloTheme.textPrimary : LudeloTheme.warn
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("Active Title:")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 11
                                color: LudeloTheme.textDim
                            }
                            Text {
                                text: hostData.app ? hostData.app : qsTr("Home Screen / Idle")
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 11
                                color: hostData.app ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Main Action Button (Connect or Wake)
                    LButton {
                        Layout.fillWidth: true
                        height: 44
                        variant: hostData.state === "ready" ? "primary" : (hostData.state === "standby" ? "secondary" : (hostData.registered ? "primary" : "mint"))
                        text: {
                            if (hostData.state === "ready") return qsTr("CONNECT DIRECT");
                            if (hostData.state === "standby") return qsTr("WAKE CONSOLE");
                            if (!hostData.registered) return qsTr("PAIR CONSOLE");
                            return qsTr("CONNECT");
                        }
                        keyHint: hostData.state === "standby" ? (LudeloTheme.hintWake + " WAKE") : (LudeloTheme.hintSelect + " CONNECT")
                        onClicked: {
                            if (hostData.state === "standby")
                                cardDelegateRoot.wakeUpHost();
                            else
                                cardDelegateRoot.connectToHost();
                        }
                    }
                }
            }
        }
    }

    // Component: Add New Console / Manual IP Card
    Component {
        id: addManualCardComponent

        Item {
            width: hostsView.cellWidth
            height: hostsView.cellHeight

            LCard {
                anchors.fill: parent
                anchors.margins: 10
                customRadius: 16
                hoverLift: true
                cardColor: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.45)
                borderColor: isFocused || isHovered ? LudeloTheme.accentPrimary : LudeloTheme.borderSubtle
                glowColor: LudeloTheme.accentGlow
                onClicked: root.showManualHostDialog()

                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    spacing: 14

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 52
                        height: 52
                        radius: 26
                        color: LudeloTheme.bgElevated
                        border.color: LudeloTheme.borderHover
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            font.pixelSize: 26
                            font.weight: Font.Light
                            color: LudeloTheme.textPrimary
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Add New Console / Manual IP")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: LudeloTheme.textPrimary
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        text: qsTr("Pair via 8-Digit Registration PIN or enter a custom IPv4 address with subnet broadcast")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 12
                        color: LudeloTheme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    LButton {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 220
                        height: 42
                        variant: "secondary"
                        text: qsTr("MANUAL PIN PAIRING")
                        keyHint: "[F2]"
                        onClicked: root.showManualHostDialog()
                    }
                }
            }
        }
    }

    // Component: Hardware Acceleration HUD (Telemetry Card)
    Component {
        id: telemetryCardComponent

        Item {
            width: hostsView.cellWidth
            height: hostsView.cellHeight

            LCard {
                anchors.fill: parent
                anchors.margins: 10
                customRadius: 16
                hoverLift: true
                cardColor: LudeloTheme.bgCard
                borderColor: isFocused || isHovered ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                glowColor: LudeloTheme.accentMintGlow

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12

                    // Card Header
                    RowLayout {
                        Layout.fillWidth: true

                        Column {
                            Text {
                                text: "NETWORK INTERFACE"
                                font.family: LudeloTheme.fontFamilyMono
                                font.pixelSize: 10
                                color: LudeloTheme.textDim
                                font.letterSpacing: 1
                            }
                            Text {
                                text: qsTr("Hardware Acceleration HUD")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                        }

                        Item { Layout.fillWidth: true } // Spacer

                        LPill {
                            text: "LOW LATENCY"
                            dotColor: LudeloTheme.accentMint
                            glowColor: LudeloTheme.accentMintGlow
                            showDot: true
                        }
                    }

                    // Stat Metrics Box
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: 8
                            color: Qt.rgba(0, 0, 0, 0.3)
                            border.color: LudeloTheme.borderSubtle

                            Column {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "HW DECODER"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 9
                                    color: LudeloTheme.textDim
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: Chiaki.settings.decoder ? Chiaki.settings.decoder.toUpperCase() : "D3D11VA"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentMint
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: 8
                            color: Qt.rgba(0, 0, 0, 0.3)
                            border.color: LudeloTheme.borderSubtle

                            Column {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "TARGET BITRATE"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 9
                                    color: LudeloTheme.textDim
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: {
                                        let br = Chiaki.settings.bitrateLocalPS5;
                                        if (!br || isNaN(br) || br <= 0) return "15 Mbps";
                                        return Math.round(br / 1000) + " Mbps";
                                    }
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentPrimary
                                }
                            }
                        }
                    }

                    // Real Details
                    Column {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            width: parent.width
                            Text { text: qsTr("Controller:"); font.pixelSize: 11; color: LudeloTheme.textDim }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: Chiaki.controllers.length > 0 ? (Chiaki.controllers[0].dualSense ? "Wireless Gamepad" : "Gamepad Connected") : "Keyboard / No Gamepad"
                                font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textPrimary
                            }
                        }

                        RowLayout {
                            width: parent.width
                            Text { text: qsTr("Resolution / FPS:"); font.pixelSize: 11; color: LudeloTheme.textDim }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: {
                                    let res = Chiaki.settings.resolutionLocalPS5;
                                    let resStr = "1080p";
                                    if (res === 0) resStr = "360p";
                                    else if (res === 1) resStr = "540p";
                                    else if (res === 2) resStr = "720p";
                                    else if (res === 3) resStr = "1080p";

                                    let fps = Chiaki.settings.fpsLocalPS5;
                                    let fpsStr = fps === 0 ? "30 FPS" : "60 FPS";
                                    return resStr + " @ " + fpsStr;
                                }
                                font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textPrimary
                            }
                        }

                        RowLayout {
                            width: parent.width
                            Text { text: qsTr("Audio Buffer:"); font.pixelSize: 11; color: LudeloTheme.textDim }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: Chiaki.settings.audioBufferSize + " frames"
                                font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary
                            }
                        }

                        RowLayout {
                            width: parent.width
                            Text { text: qsTr("HW Decoder:"); font.pixelSize: 11; color: LudeloTheme.textDim }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: {
                                    var dec = Chiaki.settings.decoder;
                                    if (dec && dec.length > 0) return dec.toUpperCase();
                                    return "AUTO (D3D11VA)";
                                }
                                font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary
                            }
                        }
                    }

                    LButton {
                        Layout.fillWidth: true
                        height: 40
                        variant: "ghost"
                        text: qsTr("STREAM PREFERENCES")
                        keyHint: LudeloTheme.hintSettings
                        onClicked: root.showSettingsDialog()
                    }
                }
            }
        }
    }

    // Button Hints Footer (Xbox / Text Nomenclature ONLY - NO Sony Glyphs)
    // P0.4: Hidden when Cloud Play tab is active (it has its own footer)
    Rectangle {
        id: buttonHintsFooter
        visible: mainTabBar.currentIndex !== 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 40
        color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.95)
        border.color: LudeloTheme.borderSubtle
        border.width: 1
        z: 100

        Item {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24

            // Left Side Gamepad / Keyboard Dynamic Hints
            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 18

                // SELECT
                Row {
                    spacing: 6
                    Rectangle {
                        width: Math.max(20, aKeyText.implicitWidth + 8); height: 20; radius: 4; color: Qt.rgba(0, 0, 0, 0.4); border.color: LudeloTheme.borderSubtle
                        Text { id: aKeyText; anchors.centerIn: parent; text: LudeloTheme.hintSelectKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                    }
                    Text { text: qsTr("SELECT"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; font.weight: Font.Medium; color: LudeloTheme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                }

                // WAKE
                Row {
                    spacing: 6
                    Rectangle {
                        width: Math.max(20, yKeyText.implicitWidth + 8); height: 20; radius: 4; color: Qt.rgba(0, 0, 0, 0.4); border.color: LudeloTheme.borderSubtle
                        Text { id: yKeyText; anchors.centerIn: parent; text: LudeloTheme.hintWakeKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.warn }
                    }
                    Text { text: qsTr("WAKE"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; font.weight: Font.Medium; color: LudeloTheme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                }

                // DETAILS / GAMES
                Row {
                    spacing: 6
                    Rectangle {
                        width: Math.max(20, xKeyText.implicitWidth + 8); height: 20; radius: 4; color: Qt.rgba(0, 0, 0, 0.4); border.color: LudeloTheme.borderSubtle
                        Text { id: xKeyText; anchors.centerIn: parent; text: LudeloTheme.hintDetailsKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentPrimary }
                    }
                    Text { text: qsTr("DETAILS"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; font.weight: Font.Medium; color: LudeloTheme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                }

                // BACK / EXIT
                Row {
                    spacing: 6
                    Rectangle {
                        width: Math.max(20, bKeyText.implicitWidth + 8); height: 20; radius: 4; color: Qt.rgba(0, 0, 0, 0.4); border.color: LudeloTheme.borderSubtle
                        Text { id: bKeyText; anchors.centerIn: parent; text: LudeloTheme.hintBackKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.error }
                    }
                    Text { text: qsTr("BACK"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; font.weight: Font.Medium; color: LudeloTheme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                }

                // SETTINGS
                Row {
                    spacing: 6
                    Rectangle {
                        width: Math.max(20, startKeyText.implicitWidth + 8); height: 20; radius: 4; color: Qt.rgba(0, 0, 0, 0.4); border.color: LudeloTheme.borderSubtle
                        Text { id: startKeyText; anchors.centerIn: parent; text: LudeloTheme.hintSettingsKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                    }
                    Text { text: qsTr("SETTINGS"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; font.weight: Font.Medium; color: LudeloTheme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            // Center Bitrate & Engine Telemetry
            Text {
                anchors.centerIn: parent
                text: {
                    var br = Chiaki.settings.bitrateLocalPS5;
                    var brMbps = (br && !isNaN(br) && br > 0) ? Math.round(br / 1000) : 15;
                    var dec = Chiaki.settings.decoder;
                    var decStr = (dec && dec.length > 0) ? dec.toUpperCase() : "D3D11VA";
                    return qsTr("BITRATE: %1 Mbps • DECODER: %2").arg(brMbps).arg(decStr);
                }
                font.family: LudeloTheme.fontFamilyMono
                font.pixelSize: 11
                color: LudeloTheme.textDim
            }

            // Right Side Version Info
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Ludelo Remote Play Client • v%1").arg(Qt.application.version || "2.4")
                font.family: LudeloTheme.fontFamilyMono
                font.pixelSize: 11
                color: LudeloTheme.textDim
            }
        }
    }
}