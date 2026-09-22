import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Rectangle {
    id: accountRoot

    anchors.fill: parent
    color: LudeloTheme.bgBase
    focus: true

    property var allGames: []
    property var filteredGames: []
    property string activeGameFilter: "all" // "all", "ps5", "ps4"
    property bool showNpssoInput: false

    function close() {
        if (typeof root !== "undefined" && root.closeDialog) {
            root.closeDialog();
        } else if (accountRoot.StackView && accountRoot.StackView.view) {
            accountRoot.StackView.view.pop();
        }
    }

    function signOut() {
        Chiaki.settings.psnAuthToken = "";
        Chiaki.settings.psnRefreshToken = "";
        Chiaki.settings.psnAccountId = "";
        Chiaki.settings.psnNpssoToken = "";
        allGames = [];
        filteredGames = [];
        signOutToast.show();
    }

    function loadGames() {
        let gamesJson = Chiaki.getPsnInstalledGames();
        if (!gamesJson || gamesJson === "{}" || gamesJson === "[]") {
            allGames = [];
            filterGames();
            return;
        }
        try {
            let parsed = JSON.parse(gamesJson);
            let list = [];
            if (Array.isArray(parsed)) {
                list = parsed;
            } else if (typeof parsed === "object") {
                for (let devId in parsed) {
                    let dev = parsed[devId];
                    if (dev.games && Array.isArray(dev.games)) {
                        list = list.concat(dev.games);
                    }
                }
            }
            allGames = list;
        } catch (e) {
            console.error("AccountView: error parsing games JSON", e);
            allGames = [];
        }
        filterGames();
    }

    function filterGames() {
        if (activeGameFilter === "all") {
            filteredGames = allGames;
        } else if (activeGameFilter === "ps5") {
            filteredGames = allGames.filter(g => (g.category && g.category.indexOf("ps5") !== -1) || (g.platform && g.platform.indexOf("ps5") !== -1));
        } else if (activeGameFilter === "ps4") {
            filteredGames = allGames.filter(g => (g.category && g.category.indexOf("ps4") !== -1) || (g.platform && g.platform.indexOf("ps4") !== -1));
        }
    }

    Component.onCompleted: loadGames()

    Connections {
        target: Chiaki
        function onPsnGamesSynced(count) {
            accountRoot.loadGames();
        }
        function onPsnTokenChanged() {
            accountRoot.loadGames();
        }
    }

    Keys.onEscapePressed: close()
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Y) {
            Chiaki.startWebView2Login();
            event.accepted = true;
        } else if (event.key === Qt.Key_X) {
            signOut();
            event.accepted = true;
        }
    }

    // Atmospheric background auras
    Item {
        anchors.fill: parent
        z: 0

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 550
            height: 550
            radius: 275
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.08)
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 600
            height: 600
            radius: 300
            color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.04)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        z: 1

        // =====================================================================
        // TOP NAVIGATION BAR
        // =====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.90)
            border.color: LudeloTheme.borderSubtle
            border.width: 1

            // Background drag and double-click maximize area
            MouseArea {
                anchors.fill: parent
                z: 0
                acceptedButtons: Qt.LeftButton
                onPressed: {
                    if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.startDrag === "function") {
                        Chiaki.window.startDrag();
                    }
                }
                onDoubleClicked: {
                    if (typeof Chiaki !== "undefined" && Chiaki.window && typeof Chiaki.window.toggleMaximize === "function") {
                        Chiaki.window.toggleMaximize();
                    }
                }
            }

            RowLayout {
                z: 1
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                // Brandmark
                Row {
                    spacing: 10
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        width: 34
                        height: 34
                        radius: 8
                        color: LudeloTheme.accentPrimary
                        anchors.verticalCenter: parent.verticalCenter

                        Image {
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            source: "qrc:icons/logo_square_1024.png"
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: "LUDELO"
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.letterSpacing: 1.2
                            color: LudeloTheme.textPrimary
                        }
                        Text {
                            text: "PSN ACCOUNT"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: LudeloTheme.accentMint
                        }
                    }
                }

                // Back Button [ESC]
                LButton {
                    height: 36
                    implicitWidth: 170
                    customRadius: 18
                    variant: "secondary"
                    text: qsTr("BACK TO HOME")
                    keyHint: "[ESC]"
                    onClicked: accountRoot.close()
                }

                Item { Layout.fillWidth: true }

                // Telemetry Badges
                Row {
                    spacing: 12
                    Layout.alignment: Qt.AlignVCenter

                    LPill {
                        text: (Chiaki.controllers && Chiaki.controllers.length > 0) ? "WIRELESS GAMEPAD • 1000Hz" : "NO GAMEPAD"
                        dotColor: (Chiaki.controllers && Chiaki.controllers.length > 0) ? LudeloTheme.accentMint : LudeloTheme.textDim
                        glowColor: LudeloTheme.accentMintGlow
                        showDot: true
                    }

                    LPill {
                        text: "DIRECT P2P LAN"
                        dotColor: LudeloTheme.accentPrimary
                        glowColor: LudeloTheme.accentGlow
                        showDot: true
                    }
                }
            }
        }

        // =====================================================================
        // SCROLLABLE CONTENT BODY
        // =====================================================================
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: mainContentColumn.implicitHeight + 48
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: mainContentColumn
                width: Math.min(parent.width - 48, 1400)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 24
                Layout.topMargin: 24

                // -------------------------------------------------------------
                // 1. HERO CARD: USER PROFILE & IDENTITY
                // -------------------------------------------------------------
                LCard {
                    Layout.fillWidth: true
                    customRadius: LudeloTheme.radiusCard

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 24

                        // Avatar with Mint Ring
                        Item {
                            width: 88
                            height: 88

                            Rectangle {
                                anchors.fill: parent
                                radius: 44
                                color: Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.9)
                                border.color: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                                border.width: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: Chiaki.settings.psnAccountId ? Chiaki.settings.psnAccountId.charAt(0).toUpperCase() : "🎮"
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 32
                                    font.weight: Font.Bold
                                    color: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.textDim
                                }
                            }

                            // Active Online Pulse Dot
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                                anchors.margins: 2
                                width: 18
                                height: 18
                                radius: 9
                                color: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.textDim
                                border.color: LudeloTheme.bgBase
                                border.width: 2
                            }
                        }

                        // Identity & Details
                        ColumnLayout {
                            spacing: 6
                            Layout.fillWidth: true

                            RowLayout {
                                spacing: 10
                                Text {
                                    text: Chiaki.settings.psnAccountId ? Chiaki.settings.psnAccountId : qsTr("Guest Player (Not Connected)")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 22
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }

                                LPill {
                                    text: Chiaki.settings.psnAuthToken ? qsTr("VERIFIED PSN") : qsTr("OFFLINE")
                                    dotColor: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.warn
                                    showDot: true
                                }
                            }

                            // Obfuscated Account ID & Region
                            RowLayout {
                                spacing: 12
                                Text {
                                    text: qsTr("Account ID: %1").arg(LudeloTheme.formatObfuscatedAccountId(Chiaki.settings.psnAccountId))
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    color: LudeloTheme.textSecondary
                                }

                                Rectangle { width: 1; height: 12; color: LudeloTheme.borderSubtle }

                                Text {
                                    visible: Chiaki.settings.psnAuthToken ? true : false
                                    text: qsTr("Store Region: %1").arg(Qt.locale().name.split("_")[1] || "—")
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    color: LudeloTheme.textSecondary
                                }
                            }

                            // Honest Security Badge
                            RowLayout {
                                spacing: 8
                                Rectangle {
                                    height: 24
                                    radius: 6
                                    color: Chiaki.settings.psnAuthToken
                                        ? Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.12)
                                        : Qt.rgba(1.0, 1.0, 1.0, 0.06)
                                    border.color: Chiaki.settings.psnAuthToken
                                        ? Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.35)
                                        : LudeloTheme.borderSubtle
                                    border.width: 1
                                    implicitWidth: securityText.implicitWidth + 16

                                    Text {
                                        id: securityText
                                        anchors.centerIn: parent
                                        text: Chiaki.settings.psnAuthToken
                                            ? qsTr("CONNECTED TO PSN • ENCRYPTED LOCAL STORAGE (DPAPI)")
                                            : qsTr("LOCAL STORAGE ONLY • ENCRYPTED (DPAPI)")
                                        font.family: LudeloTheme.fontFamilyMono
                                        font.pixelSize: 10
                                        font.weight: Font.DemiBold
                                        color: Chiaki.settings.psnAuthToken ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                                    }
                                }

                                Text {
                                    text: qsTr("Zero plaintext credentials stored")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 11
                                    color: LudeloTheme.textDim
                                }
                            }
                        }

                        // Action Buttons Cluster
                        ColumnLayout {
                            spacing: 10
                            Layout.alignment: Qt.AlignVCenter

                            LButton {
                                height: 40
                                implicitWidth: 190
                                customRadius: 8
                                variant: Chiaki.settings.psnAuthToken ? "primary" : "mint"
                                text: Chiaki.settings.psnAuthToken ? qsTr("RE-AUTHENTICATE") : qsTr("SIGN IN WITH PSN")
                                keyHint: "[Y]"
                                onClicked: Chiaki.startWebView2Login()
                            }

                            RowLayout {
                                spacing: 8
                                LButton {
                                    height: 36
                                    implicitWidth: 120
                                    customRadius: 8
                                    variant: "secondary"
                                    text: qsTr("SYNC GAMES")
                                    onClicked: {
                                        Chiaki.refreshPsnToken();
                                        accountRoot.loadGames();
                                    }
                                }
                                LButton {
                                    height: 36
                                    implicitWidth: 100
                                    customRadius: 8
                                    variant: "danger"
                                    text: qsTr("SIGN OUT")
                                    keyHint: "[X]"
                                    enabled: Chiaki.settings.psnAuthToken !== ""
                                    onClicked: accountRoot.signOut()
                                }
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // 2. QUICK STATS STRIP (Only real data)
                // -------------------------------------------------------------
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    // Metric 1: Installed Titles
                    LCard {
                        Layout.fillWidth: true
                        height: 72
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12
                            Text { text: "🎮"; font.pixelSize: 20 }
                            Column {
                                spacing: 2
                                Text { text: qsTr("TOTAL SYNCHRONIZED"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text { text: qsTr("%1 Titles Installed").arg(accountRoot.allGames.length); font.family: LudeloTheme.fontFamily; font.pixelSize: 15; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                            }
                        }
                    }

                    // Metric 2: Primary Linked Console
                    LCard {
                        Layout.fillWidth: true
                        height: 72
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12
                            Text { text: "🖥️"; font.pixelSize: 20 }
                            Column {
                                spacing: 2
                                Text { text: qsTr("PRIMARY CONSOLE"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: {
                                        if (Chiaki.hosts && Chiaki.hosts.length > 0)
                                            return Chiaki.hosts[0].name || "PlayStation Console";
                                        if (Chiaki.settings.registeredHosts && Chiaki.settings.registeredHosts.length > 0)
                                            return Chiaki.settings.registeredHosts[0].server_nickname || "PlayStation Console";
                                        return qsTr("No Console Linked");
                                    }
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 15
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }
                            }
                        }
                    }

                    // Metric 3: Active Gamepad Profile
                    LCard {
                        Layout.fillWidth: true
                        height: 72
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12
                            Text { text: "⚙️"; font.pixelSize: 20 }
                            Column {
                                spacing: 2
                                Text { text: qsTr("CONTROLLER PROFILE"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.settings.currentProfile ? Chiaki.settings.currentProfile : qsTr("Default (Active)")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 15
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentMint
                                }
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // 3. INSTALLED CONSOLE GAMES LIBRARY GRID
                // -------------------------------------------------------------
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    // Section Header & Filters
                    RowLayout {
                        Layout.fillWidth: true

                        Row {
                            spacing: 8
                            Text {
                                text: qsTr("INSTALLED CONSOLE GAMES")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Rectangle {
                                height: 22
                                radius: 11
                                color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.15)
                                border.color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.35)
                                border.width: 1
                                implicitWidth: countLabel.implicitWidth + 14
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    id: countLabel
                                    anchors.centerIn: parent
                                    text: qsTr("%1 Ready").arg(accountRoot.filteredGames.length)
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentMint
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Filter Pills
                        Row {
                            spacing: 6
                            LButton {
                                height: 32
                                implicitWidth: 70
                                customRadius: 6
                                variant: accountRoot.activeGameFilter === "all" ? "primary" : "secondary"
                                text: qsTr("All (%1)").arg(accountRoot.allGames.length)
                                onClicked: { accountRoot.activeGameFilter = "all"; accountRoot.filterGames(); }
                            }
                            LButton {
                                height: 32
                                implicitWidth: 110
                                customRadius: 6
                                variant: accountRoot.activeGameFilter === "ps5" ? "primary" : "secondary"
                                text: "PlayStation 5"
                                onClicked: { accountRoot.activeGameFilter = "ps5"; accountRoot.filterGames(); }
                            }
                            LButton {
                                height: 32
                                implicitWidth: 110
                                customRadius: 6
                                variant: accountRoot.activeGameFilter === "ps4" ? "primary" : "secondary"
                                text: "PlayStation 4"
                                onClicked: { accountRoot.activeGameFilter = "ps4"; accountRoot.filterGames(); }
                            }
                        }
                    }

                    // Games Grid
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 16
                        columnSpacing: 16
                        visible: accountRoot.filteredGames.length > 0

                        Repeater {
                            model: accountRoot.filteredGames
                            delegate: LCard {
                                Layout.fillWidth: true
                                height: 130
                                customRadius: 12

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 16

                                    // Game Cover Art
                                    Rectangle {
                                        width: 160
                                        height: 106
                                        radius: 8
                                        color: LudeloTheme.bgElevated
                                        clip: true

                                        Image {
                                            anchors.fill: parent
                                            source: ChiakiGames.getGameImage(modelData.titleId, "landscape")
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                        }

                                        // Platform Badge Overlay
                                        Rectangle {
                                            anchors.top: parent.top
                                            anchors.left: parent.left
                                            anchors.margins: 6
                                            height: 18
                                            radius: 4
                                            color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.85)
                                            border.color: LudeloTheme.borderSubtle
                                            border.width: 1
                                            implicitWidth: platformBadge.implicitWidth + 8

                                            Text {
                                                id: platformBadge
                                                anchors.centerIn: parent
                                                text: (modelData.category && modelData.category.indexOf("ps5") !== -1) ? "PS5" : "PS4"
                                                font.family: LudeloTheme.fontFamilyMono
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                color: LudeloTheme.accentMint
                                            }
                                        }
                                    }

                                    // Title & Honest Real Metadata
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Text {
                                            text: modelData.name || modelData.titleId || "PlayStation Game"
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 15
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Text {
                                            text: modelData.titleId ? qsTr("Title ID: %1").arg(modelData.titleId) : ""
                                            font.family: LudeloTheme.fontFamilyMono
                                            font.pixelSize: 11
                                            color: LudeloTheme.textDim
                                        }

                                        Item { Layout.fillHeight: true }

                                        // Connect Button (Honest: connects to host session)
                                        LButton {
                                            height: 32
                                            implicitWidth: 140
                                            customRadius: 6
                                            variant: "primary"
                                            text: qsTr("CONECTAR")
                                            keyHint: "[A]"
                                            onClicked: {
                                                if (Chiaki.hosts && Chiaki.hosts.length > 0)
                                                    Chiaki.connectToHost(0, "", modelData.name, modelData.titleId);
                                                else
                                                    accountRoot.close();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty State Card
                    LCard {
                        Layout.fillWidth: true
                        height: 140
                        visible: accountRoot.filteredGames.length === 0

                        Column {
                            anchors.centerIn: parent
                            spacing: 8
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("No installed games found for this filter")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.Medium
                                color: LudeloTheme.textPrimary
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("Turn on your linked PlayStation console and sync your library to populate installed games.")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textDim
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // 4. ADVANCED FALLBACK: MANUAL NPSSO TOKEN ENTRY (Collapsible)
                // -------------------------------------------------------------
                LCard {
                    Layout.fillWidth: true
                    customRadius: LudeloTheme.radiusCard

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "🔑 " + qsTr("ADVANCED AUTHENTICATION & MANUAL NPSSO TOKEN")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 14
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                            Item { Layout.fillWidth: true }
                            LButton {
                                height: 28
                                implicitWidth: 90
                                customRadius: 6
                                variant: "ghost"
                                text: accountRoot.showNpssoInput ? qsTr("COLLAPSE") : qsTr("EXPAND")
                                onClicked: accountRoot.showNpssoInput = !accountRoot.showNpssoInput
                            }
                        }

                        Text {
                            text: qsTr("If browser OAuth authentication fails due to 2FA delays or Captcha blocks, paste your 64-character NPSSO token directly. Tokens are encrypted at rest using Windows DPAPI.")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 12
                            color: LudeloTheme.textDim
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // Collapsible Token Input Row
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            visible: accountRoot.showNpssoInput

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                TextField {
                                    id: npssoField
                                    Layout.fillWidth: true
                                    height: 42
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 12
                                    color: LudeloTheme.textPrimary
                                    echoMode: showPlainTokenCheck.checked ? TextInput.Normal : TextInput.Password
                                    placeholderText: qsTr("Paste 64-character NPSSO token here...")
                                    text: Chiaki.settings.psnNpssoToken || ""

                                    background: Rectangle {
                                        radius: 8
                                        color: LudeloTheme.bgInput
                                        border.color: npssoField.activeFocus ? LudeloTheme.borderFocus : LudeloTheme.borderSubtle
                                        border.width: 1
                                    }
                                }

                                CheckBox {
                                    id: showPlainTokenCheck
                                    text: qsTr("Show")
                                }

                                LButton {
                                    height: 42
                                    implicitWidth: 180
                                    customRadius: 8
                                    variant: "mint"
                                    text: qsTr("VERIFY & SAVE")
                                    enabled: npssoField.text.trim().length === 64
                                    onClicked: {
                                        Chiaki.initPsnAuthV3(npssoField.text.trim(), function(success) {
                                            if (success) {
                                                tokenToast.show();
                                                accountRoot.loadGames();
                                            }
                                        });
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // BOTTOM FOOTER
        // =====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.95)
            border.color: LudeloTheme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                Text {
                    text: qsTr("Ludelo Remote Client • Account Management Profile")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 11
                    color: LudeloTheme.textSecondary
                }

                Item { Layout.fillWidth: true }

                Row {
                    spacing: 20
                    Layout.alignment: Qt.AlignVCenter

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, aKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: aKeyText; anchors.centerIn: parent; text: LudeloTheme.hintSelectKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "SELECT GAME"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, bKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: bKeyText; anchors.centerIn: parent; text: LudeloTheme.hintBackKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "BACK"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, yKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: yKeyText; anchors.centerIn: parent; text: LudeloTheme.hintWakeKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "RE-AUTHENTICATE"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, xKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: xKeyText; anchors.centerIn: parent; text: LudeloTheme.hintDetailsKey; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "SIGN OUT"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }
                }
            }
        }
    }

    // Toast Notification for Token Saved
    Rectangle {
        id: tokenToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
        height: 40
        width: tokenToastText.implicitWidth + 32
        radius: 20
        color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.95)
        border.color: LudeloTheme.accentMint
        border.width: 1
        opacity: 0.0
        z: 999

        function show() {
            toastAnim.restart();
        }

        Row {
            anchors.centerIn: parent
            spacing: 8
            Rectangle {
                width: 6; height: 6; radius: 3
                color: LudeloTheme.accentMint
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                id: tokenToastText
                text: qsTr("NPSSO token verified and stored in encrypted storage")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 12
                font.weight: Font.Medium
                color: LudeloTheme.textPrimary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: tokenToast; property: "opacity"; to: 1.0; duration: 200 }
            PauseAnimation { duration: 2500 }
            NumberAnimation { target: tokenToast; property: "opacity"; to: 0.0; duration: 200 }
        }
    }

    // Toast Notification for Sign Out
    Rectangle {
        id: signOutToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
        height: 40
        width: signOutToastText.implicitWidth + 32
        radius: 20
        color: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.95)
        border.color: LudeloTheme.error
        border.width: 1
        opacity: 0.0
        z: 999

        function show() {
            signOutAnim.restart();
        }

        Row {
            anchors.centerIn: parent
            spacing: 8
            Rectangle {
                width: 6; height: 6; radius: 3
                color: LudeloTheme.error
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                id: signOutToastText
                text: qsTr("Signed out from PlayStation Network")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 12
                font.weight: Font.Medium
                color: LudeloTheme.textPrimary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        SequentialAnimation {
            id: signOutAnim
            NumberAnimation { target: signOutToast; property: "opacity"; to: 1.0; duration: 200 }
            PauseAnimation { duration: 2500 }
            NumberAnimation { target: signOutToast; property: "opacity"; to: 0.0; duration: 200 }
        }
    }
}
