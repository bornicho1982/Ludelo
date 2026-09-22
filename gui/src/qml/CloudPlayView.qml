import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Pane {
    id: root
    padding: 0

    property var mainTabBar: null
    property var settingsButton: null
    property var showConfirmDialogFunc: null

    property var allGames: []
    property var filteredGames: []
    property var currentPageGames: []
    property int renderedCount: 48
    property bool isLoading: false
    property string searchQuery: ""
    property string authErrorMessage: ""
    property string fallbackRegion: ""
    property bool catalogNativeMode: true
    property var activeTagFilters: [] // empty = show all; values: owned, streamable, purchaseable
    property bool showFavoritesOnly: false
    property int sortState: 0 // 0=Playable First, 1=A-Z, 2=Z-A
    property var favoriteProductIds: []
    property var qrCodeDialogRef: null

    readonly property var tagFilterCategories: ["owned", "streamable", "purchaseable"]
    readonly property var tagFilterLabels: [qsTr("Owned"), qsTr("Streamable"), qsTr("Store")]

    // Selected game for Hero Spotlight
    readonly property var selectedGame: {
        if (filteredGames && filteredGames.length > 0) {
            let idx = gamesGrid.currentIndex;
            if (idx >= 0 && idx < filteredGames.length) {
                return filteredGames[idx];
            }
            for (let i = 0; i < filteredGames.length; ++i) {
                if (isPlayableNow(filteredGames[i])) {
                    return filteredGames[i];
                }
            }
            return filteredGames[0];
        }
        return null;
    }

    // Background
    Rectangle {
        anchors.fill: parent
        color: LudeloTheme.bgDark
        z: -2

        // Ambient radial glow in top right
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 500
            height: 500
            radius: 250
            color: LudeloTheme.accentGlow
            opacity: 0.12
        }
    }

    Component.onCompleted: {
        fallbackRegion = Chiaki.settings.cloudResolvedStoreCountry || "";
        catalogNativeMode = Chiaki.settings.cloudCatalogNativeMode;
        sortState = Chiaki.settings.cloudSortState || 0;
        let savedTagFilters = Chiaki.settings.cloudTagFilters;
        if (savedTagFilters) {
            try {
                let parsed = JSON.parse(savedTagFilters);
                if (Array.isArray(parsed))
                    activeTagFilters = parsed;
            } catch (e) {
                console.warn("Failed to parse cloud tag filters:", e);
            }
        }
        let savedFavorites = Chiaki.settings.cloudFavorites;
        if (savedFavorites) {
            try {
                favoriteProductIds = JSON.parse(savedFavorites);
            } catch (e) {
                console.error("Failed to parse saved favorites:", e);
                favoriteProductIds = [];
            }
        }
        Qt.callLater(() => loadUnifiedCatalog());
        initialFocusTimer.restart();
    }

    onVisibleChanged: {
        if (visible) {
            if (allGames.length === 0)
                loadUnifiedCatalog();
            initialFocusTimer.restart();
        }
    }

    StackView.onActivated: {
        Qt.callLater(() => loadUnifiedCatalog());
        initialFocusTimer.restart();
    }

    Connections {
        target: Chiaki.cloudCatalog
        function onCacheInvalidated() {
            loadUnifiedCatalog();
        }
    }

    Timer {
        id: initialFocusTimer
        interval: 150
        repeat: false
        onTriggered: {
            if (gamesGrid.count > 0) {
                gamesGrid.currentIndex = 0;
                gamesGrid.forceActiveFocus();
            }
        }
    }

    Keys.onEscapePressed: {
        if (showConfirmDialogFunc) {
            showConfirmDialogFunc(qsTr("Quit"), qsTr("Are you sure you want to quit?"), () => Qt.quit(), null, true);
        }
    }

    function isTagFilterActive(tag) {
        return !activeTagFilters || activeTagFilters.length === 0
               || activeTagFilters.indexOf(tag) !== -1;
    }

    function setTagFilters(tags) {
        activeTagFilters = tags;
        Chiaki.settings.cloudTagFilters = JSON.stringify(tags);
        applySearchFilter();
    }

    function toggleTagFilter(tag) {
        let current = (!activeTagFilters || activeTagFilters.length === 0)
                      ? tagFilterCategories.slice()
                      : activeTagFilters.slice();
        let idx = current.indexOf(tag);
        if (idx !== -1)
            current.splice(idx, 1);
        else
            current.push(tag);
        if (current.length === 0 || current.length === tagFilterCategories.length)
            setTagFilters([]);
        else
            setTagFilters(current);
    }

    function isPlayableNow(game) {
        return game && game.category !== "purchaseable";
    }

    function sortGames(games) {
        if (!games || games.length === 0) return [];
        let items = new Array(games.length);
        for (let i = 0; i < games.length; i++) {
            let g = games[i];
            items[i] = {
                game: g,
                nameLower: gameName(g).toLowerCase(),
                playable: isPlayableNow(g) ? 1 : 0
            };
        }
        if (sortState === 1) {
            items.sort((a, b) => (a.nameLower < b.nameLower ? -1 : (a.nameLower > b.nameLower ? 1 : 0)));
        } else if (sortState === 2) {
            items.sort((a, b) => (a.nameLower > b.nameLower ? -1 : (a.nameLower < b.nameLower ? 1 : 0)));
        } else {
            items.sort((a, b) => {
                if (a.playable !== b.playable) return b.playable - a.playable;
                return a.nameLower < b.nameLower ? -1 : (a.nameLower > b.nameLower ? 1 : 0);
            });
        }
        let result = new Array(items.length);
        for (let i = 0; i < items.length; i++) {
            result[i] = items[i].game;
        }
        return result;
    }

    function gameName(game) {
        if (!game) return "";
        if (game.name) return game.name;
        if (game.game_meta && game.game_meta.name) return game.game_meta.name;
        return "";
    }

    function loadUnifiedCatalog() {
        let npssoToken = Chiaki.settings.psnNpssoToken;
        if (!npssoToken || npssoToken.trim().length === 0) {
            authErrorMessage = qsTr("NPSSO token is required for cloud games. Please sign in via Settings or Account View.");
        } else {
            authErrorMessage = "";
        }

        allGames = [];
        filteredGames = [];
        currentPageGames = [];
        isLoading = true;

        Chiaki.cloudCatalog.fetchUnifiedCatalog(function(success, message, jsonData) {
            isLoading = false;
            if (!success || !jsonData) {
                allGames = [];
                filteredGames = [];
                currentPageGames = [];
                return;
            }
            try {
                let data = JSON.parse(jsonData);
                if (data.games && Array.isArray(data.games)) {
                    allGames = data.games;
                    fallbackRegion = data.fallbackRegion || "";
                    catalogNativeMode = data.nativeMode !== false;
                    Chiaki.settings.cloudResolvedStoreCountry = fallbackRegion;
                    Chiaki.settings.cloudCatalogNativeMode = catalogNativeMode;
                    if (data.warning)
                        authErrorMessage = data.warning;
                    else if (npssoToken && npssoToken.trim().length > 0)
                        authErrorMessage = "";

                    applySearchFilter();
                    Qt.callLater(() => {
                        if (gamesGrid.count > 0 && !searchField.activeFocus) {
                            gamesGrid.currentIndex = 0;
                            gamesGrid.forceActiveFocus();
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to parse unified catalog:", e);
            }
        });
    }

    function applySearchFilter() {
        let gamesToFilter = allGames.slice();

        if (activeTagFilters && activeTagFilters.length > 0) {
            gamesToFilter = gamesToFilter.filter(function(game) {
                return game.category && activeTagFilters.indexOf(game.category) !== -1;
            });
        }

        if (showFavoritesOnly) {
            gamesToFilter = gamesToFilter.filter(function(game) {
                let productId = game.productId || game.product_id || game.id;
                return favoriteProductIds.indexOf(productId) !== -1;
            });
        }

        if (searchQuery && searchQuery.trim() !== "") {
            let query = searchQuery.toLowerCase().trim();
            gamesToFilter = gamesToFilter.filter(function(game) {
                let name = gameName(game).toLowerCase();
                let pid = (game.productId || game.product_id || "").toLowerCase();
                return name.includes(query) || pid.includes(query);
            });
        }

        filteredGames = sortGames(gamesToFilter);
        renderedCount = Math.min(48, filteredGames.length);
        currentPageGames = filteredGames.slice(0, renderedCount);
    }

    function loadMoreGames() {
        if (renderedCount < filteredGames.length) {
            renderedCount = Math.min(renderedCount + 48, filteredGames.length);
            currentPageGames = filteredGames.slice(0, renderedCount);
        }
    }

    function toggleFavorite(productId) {
        if (!productId) return;
        let index = favoriteProductIds.indexOf(productId);
        let newFavorites = favoriteProductIds.slice();
        if (index !== -1) {
            newFavorites.splice(index, 1);
        } else {
            newFavorites.push(productId);
        }
        favoriteProductIds = newFavorites;
        Chiaki.settings.cloudFavorites = JSON.stringify(favoriteProductIds);
        applySearchFilter();
    }

    onSearchQueryChanged: {
        applySearchFilter();
    }

    // MAIN CONTENT LAYOUT
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. TOP SUBHEADER & TOOLBAR
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Qt.rgba(0.04, 0.05, 0.08, 0.95)
            border.color: LudeloTheme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 12

                // Breadcrumb / Title
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        width: 4
                        height: 18
                        radius: 2
                        color: LudeloTheme.accentMint
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        text: qsTr("CLOUD PLAY")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.Black
                        color: LudeloTheme.textPrimary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Status Pills (Region, Count, Cache)
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignVCenter

                    LPill {
                        text: qsTr("REGION: %1").arg(fallbackRegion ? fallbackRegion.toUpperCase() : "ES-ES")
                        showDot: true
                        dotColor: LudeloTheme.accentMint
                        glowColor: LudeloTheme.accentMintGlow
                    }

                    LPill {
                        text: qsTr("Catálogo: %1 • Mostrando: %2").arg(allGames.length).arg(filteredGames.length)
                        showDot: false
                    }

                    LPill {
                        text: qsTr("CACHE: ACTIVE")
                        showDot: true
                        dotColor: LudeloTheme.accent
                        glowColor: LudeloTheme.accentGlow
                    }
                }

                Item { Layout.fillWidth: true }

                // Search Field
                Rectangle {
                    id: searchBox
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 36
                    radius: LudeloTheme.radiusPill
                    color: searchField.activeFocus ? LudeloTheme.bgElevated : Qt.rgba(1, 1, 1, 0.05)
                    border.color: searchField.activeFocus ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        spacing: 6

                        Text {
                            text: "🔍"
                            font.pixelSize: 12
                            color: LudeloTheme.textMuted
                        }

                        TextInput {
                            id: searchField
                            Layout.fillWidth: true
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 12
                            color: LudeloTheme.textPrimary
                            clip: true
                            onTextChanged: searchQuery = text

                            Text {
                                text: qsTr("Search catalog [Y]...")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 12
                                color: LudeloTheme.textMuted
                                visible: !searchField.text && !searchField.activeFocus
                            }
                        }

                        Button {
                            visible: searchField.text.length > 0
                            text: "✕"
                            flat: true
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            onClicked: {
                                searchField.text = "";
                                searchQuery = "";
                            }
                        }
                    }
                }

                // Refresh Button
                LButton {
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 100
                    variant: "secondary"
                    keyHint: "[F5]"
                    text: qsTr("REFRESH")
                    enabled: !isLoading
                    onClicked: Chiaki.cloudCatalog.invalidateCache()
                }
            }
        }

        // 2. FILTERS & SORT ROW
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: LudeloTheme.bgBase
            border.color: LudeloTheme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 8

                // Filter: ALL
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: allFilterText.implicitWidth + 20
                    radius: LudeloTheme.radiusPill
                    color: (activeTagFilters.length === 0 && !showFavoritesOnly) ? LudeloTheme.accent : Qt.rgba(1, 1, 1, 0.05)
                    border.color: (activeTagFilters.length === 0 && !showFavoritesOnly) ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    Label {
                        id: allFilterText
                        anchors.centerIn: parent
                        text: qsTr("ALL GAMES (%1)").arg(allGames.length)
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: (activeTagFilters.length === 0 && !showFavoritesOnly) ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            showFavoritesOnly = false;
                            setTagFilters([]);
                        }
                    }
                }

                // Filter: STREAMABLE
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: streamableFilterText.implicitWidth + 20
                    radius: LudeloTheme.radiusPill
                    readonly property bool active: isTagFilterActive("streamable") && activeTagFilters.length > 0 && !showFavoritesOnly
                    color: active ? LudeloTheme.accent : Qt.rgba(1, 1, 1, 0.05)
                    border.color: active ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    Label {
                        id: streamableFilterText
                        anchors.centerIn: parent
                        text: qsTr("STREAMABLE")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: parent.active ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            showFavoritesOnly = false;
                            setTagFilters(["streamable"]);
                        }
                    }
                }

                // Filter: OWNED
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: ownedFilterText.implicitWidth + 20
                    radius: LudeloTheme.radiusPill
                    readonly property bool active: isTagFilterActive("owned") && activeTagFilters.length > 0 && !showFavoritesOnly
                    color: active ? LudeloTheme.accent : Qt.rgba(1, 1, 1, 0.05)
                    border.color: active ? LudeloTheme.accentMint : LudeloTheme.borderSubtle
                    border.width: 1

                    Label {
                        id: ownedFilterText
                        anchors.centerIn: parent
                        text: qsTr("OWNED")
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: parent.active ? LudeloTheme.accentMint : LudeloTheme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            showFavoritesOnly = false;
                            setTagFilters(["owned"]);
                        }
                    }
                }

                // Filter: FAVORITES
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: favFilterText.implicitWidth + 24
                    radius: LudeloTheme.radiusPill
                    color: showFavoritesOnly ? Qt.rgba(0.96, 0.62, 0.04, 0.25) : Qt.rgba(1, 1, 1, 0.05)
                    border.color: showFavoritesOnly ? LudeloTheme.colorWarning : LudeloTheme.borderSubtle
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "★"
                            font.pixelSize: 12
                            color: showFavoritesOnly ? LudeloTheme.colorWarning : LudeloTheme.textMuted
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Label {
                            id: favFilterText
                            text: qsTr("FAVORITES (%1)").arg(favoriteProductIds.length)
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: showFavoritesOnly ? LudeloTheme.colorWarning : LudeloTheme.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            showFavoritesOnly = !showFavoritesOnly;
                            applySearchFilter();
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Sort toggle
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: sortLabel.implicitWidth + 20
                    radius: LudeloTheme.radiusPill
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: LudeloTheme.borderSubtle
                    border.width: 1

                    Label {
                        id: sortLabel
                        anchors.centerIn: parent
                        text: {
                            if (sortState === 1) return qsTr("SORT: A → Z");
                            if (sortState === 2) return qsTr("SORT: Z → A");
                            return qsTr("SORT: PLAYABLE FIRST");
                        }
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: LudeloTheme.accentMint
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            sortState = (sortState + 1) % 3;
                            Chiaki.settings.cloudSortState = sortState;
                            applySearchFilter();
                        }
                    }
                }
            }
        }

        // 3. HONEST WARNING BANNERS
        // Auth / NPSSO Warning
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: authErrorMessage.length > 0 ? 52 : 0
            visible: authErrorMessage.length > 0
            color: Qt.rgba(0.94, 0.28, 0.44, 0.15)
            border.color: LudeloTheme.colorDanger
            border.width: 1
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 16

                Text {
                    text: "⚠"
                    font.pixelSize: 16
                    color: LudeloTheme.colorDanger
                }

                Label {
                    text: authErrorMessage
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: LudeloTheme.textPrimary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                LButton {
                    Layout.preferredHeight: 34
                    Layout.preferredWidth: 175
                    implicitWidth: 175
                    customRadius: 6
                    variant: "mint"
                    text: qsTr("RE-AUTHENTICATE")
                    onClicked: {
                        if (Chiaki.settings.psnRefreshToken) {
                            Chiaki.refreshPsnToken();
                        } else if (typeof root !== "undefined" && typeof root.showPSNTokenDialog === "function") {
                            root.showPSNTokenDialog("", false);
                        } else if (settingsButton) {
                            settingsButton.clicked();
                        }
                    }
                }
            }
        }

        // Region Fallback Notice
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: (!catalogNativeMode && authErrorMessage.length === 0 && !isLoading) ? 36 : 0
            visible: !catalogNativeMode && authErrorMessage.length === 0 && !isLoading
            color: Qt.rgba(0.96, 0.62, 0.04, 0.15)
            border.color: LudeloTheme.colorWarning
            border.width: 1
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 10

                Text {
                    text: "ℹ"
                    font.pixelSize: 14
                    color: LudeloTheme.colorWarning
                }

                Label {
                    text: qsTr("PlayStation cloud is not offered natively in your account region. Showing fallback catalog (%1).").arg(fallbackRegion)
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: LudeloTheme.textSecondary
                    Layout.fillWidth: true
                }
            }
        }

        // 4. HERO SPOTLIGHT CARD (Featured / Selected Game)
        Rectangle {
            id: heroSpotlight
            Layout.fillWidth: true
            Layout.preferredHeight: selectedGame ? 170 : 0
            visible: selectedGame !== null
            color: LudeloTheme.bgElevated
            border.color: LudeloTheme.borderSubtle
            border.width: 1
            clip: true

            // Background Cover Art blurred/dimmed
            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                smooth: true
                opacity: 0.25
                source: {
                    if (!selectedGame) return "";
                    if (selectedGame.extracted_images && selectedGame.extracted_images.landscape)
                        return selectedGame.extracted_images.landscape;
                    if (selectedGame.imageUrl)
                        return selectedGame.imageUrl;
                    return "";
                }
            }

            // Radial obsidian gradient over the image
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.rgba(0.04, 0.05, 0.08, 0.98) }
                    GradientStop { position: 0.6; color: Qt.rgba(0.04, 0.05, 0.08, 0.85) }
                    GradientStop { position: 1.0; color: Qt.rgba(0.04, 0.05, 0.08, 0.70) }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.topMargin: 16
                anchors.bottomMargin: 16
                spacing: 24

                // Hero Info Column
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    // Badges row
                    Row {
                        spacing: 8

                        LPill {
                            text: qsTr("FEATURED CLOUD SPOTLIGHT")
                            showDot: true
                            dotColor: LudeloTheme.accentMint
                            glowColor: LudeloTheme.accentMintGlow
                        }

                        LPill {
                            text: qsTr("REGION: %1").arg(fallbackRegion ? fallbackRegion.toUpperCase() : "ES-ES")
                            showDot: false
                        }

                        LPill {
                            text: selectedGame ? (selectedGame.platform || "PS5").toUpperCase() : "PS5"
                            showDot: false
                        }
                    }

                    // Game Title
                    Label {
                        Layout.fillWidth: true
                        text: gameName(selectedGame)
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 22
                        font.weight: Font.Black
                        color: LudeloTheme.textPrimary
                        elide: Text.ElideRight
                    }

                    // Metadata status line
                    Label {
                        Layout.fillWidth: true
                        text: {
                            if (!selectedGame) return "";
                            if (isPlayableNow(selectedGame)) {
                                return qsTr("DIRECT PS CLOUD STREAMING • INSTANT ACCESS • HARDWARE ACCELERATED");
                            }
                            return qsTr("REQUIRES PLAYSTATION PLUS PREMIUM SUBSCRIPTION (VISIT STORE)");
                        }
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        color: isPlayableNow(selectedGame) ? LudeloTheme.accentMint : LudeloTheme.colorWarning
                    }

                    // Actions Row
                    RowLayout {
                        spacing: 12
                        Layout.topMargin: 4

                        LButton {
                            Layout.preferredHeight: 38
                            Layout.preferredWidth: 220
                            variant: isPlayableNow(selectedGame) ? "mint" : "secondary"
                            keyHint: "[A]"
                            text: isPlayableNow(selectedGame) ? qsTr("LAUNCH CLOUD STREAM") : qsTr("REQUIRES PS PLUS")
                            onClicked: {
                                if (!selectedGame) return;
                                if (isPlayableNow(selectedGame)) {
                                    let streamingId = selectedGame.streamIdentifier || selectedGame.productId || selectedGame.product_id || selectedGame.id;
                                    let platform = selectedGame.platform || "ps5";
                                    let serviceType = selectedGame.streamServiceType || selectedGame.serviceType || "pscloud";
                                    launchCloudStreamSession(streamingId, platform, serviceType);
                                } else {
                                    let url = selectedGame.conceptUrl || selectedGame.concept_url || "https://www.playstation.com/ps-plus";
                                    Qt.openUrlExternally(url);
                                }
                            }
                        }

                        LButton {
                            Layout.preferredHeight: 38
                            Layout.preferredWidth: 150
                            variant: "ghost"
                            keyHint: "[X]"
                            text: {
                                if (!selectedGame) return qsTr("FAVORITE");
                                let pid = selectedGame.productId || selectedGame.product_id || selectedGame.id;
                                return (pid && favoriteProductIds && favoriteProductIds.indexOf(pid) !== -1) ? qsTr("UNFAVORITE") : qsTr("FAVORITE");
                            }
                            onClicked: {
                                if (selectedGame) {
                                    let pid = selectedGame.productId || selectedGame.product_id || selectedGame.id;
                                    toggleFavorite(pid);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 5. GRID VIEW OF GAMES
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // Loading Indicator
            ColumnLayout {
                anchors.centerIn: parent
                visible: isLoading
                spacing: 12

                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    running: isLoading
                }

                Label {
                    text: qsTr("Synchronizing PlayStation Cloud Catalog...")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: LudeloTheme.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Empty State
            ColumnLayout {
                anchors.centerIn: parent
                visible: !isLoading && filteredGames.length === 0
                spacing: 10

                Text {
                    text: "🎮"
                    font.pixelSize: 42
                    Layout.alignment: Qt.AlignHCenter
                    opacity: 0.5
                }

                Label {
                    text: qsTr("No games found matching your filters")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: LudeloTheme.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: qsTr("Try adjusting your search query or tag filters.")
                    font.family: LudeloTheme.fontFamily
                    font.pixelSize: 12
                    color: LudeloTheme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Direct Recycled Grid (no redundant ScrollView)
            GridView {
                id: gamesGrid
                anchors.fill: parent
                anchors.margins: 16
                cellWidth: 215
                cellHeight: 295
                focus: true
                clip: true
                model: currentPageGames
                highlightFollowsCurrentItem: true
                keyNavigationEnabled: true
                keyNavigationWraps: false
                visible: !isLoading && filteredGames.length > 0

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                onContentYChanged: {
                    if (contentHeight > 0 && contentY + height >= contentHeight - 400) {
                        root.loadMoreGames();
                    }
                }

                delegate: CloudGameCard {
                    required property int index
                    required property var modelData

                    width: gamesGrid.cellWidth - 14
                    height: gamesGrid.cellHeight - 14
                    gameData: modelData
                    focus: false

                    Binding on isFavorite {
                        value: {
                            if (!modelData) return false;
                            let pid = modelData.productId || modelData.product_id || modelData.id;
                            return root.favoriteProductIds.indexOf(pid) !== -1;
                        }
                    }

                    onToggleFavorite: (productId) => {
                        root.toggleFavorite(productId);
                    }

                    onStreamGame: (streamingId, platform, serviceType) => {
                        launchCloudStreamSession(streamingId, platform, serviceType);
                    }

                    onCreateShortcut: (productId, entitlementId, platform, serviceType, gameName) => {
                        cloudShortcutDialog.showCloudDialog(gameName, entitlementId, serviceType, "cloudGameLibrary", productId);
                    }
                }

                // Keyboard / Gamepad Navigation within grid
                Keys.onPressed: (event) => {
                    let cols = Math.floor(gamesGrid.width / gamesGrid.cellWidth);
                    if (cols < 1) cols = 1;

                    if (event.key === Qt.Key_Left) {
                        if (currentIndex % cols !== 0) {
                            currentIndex = Math.max(0, currentIndex - 1);
                        }
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Right) {
                        if (currentIndex + 1 < model.length && (currentIndex % cols) !== cols - 1) {
                            currentIndex = Math.min(model.length - 1, currentIndex + 1);
                        }
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Up) {
                        if (currentIndex - cols >= 0) {
                            currentIndex -= cols;
                            positionViewAtIndex(currentIndex, GridView.Contain);
                            event.accepted = true;
                        }
                    } else if (event.key === Qt.Key_Down) {
                        if (currentIndex + cols < model.length) {
                            currentIndex += cols;
                            positionViewAtIndex(currentIndex, GridView.Contain);
                            event.accepted = true;
                        }
                    }
                }
            }
        }

        // 6. FOOTER HUD
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: Qt.rgba(0.04, 0.05, 0.08, 0.98)
            border.color: LudeloTheme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24

                // Gamepad hints
                Row {
                    spacing: 16
                    Layout.alignment: Qt.AlignVCenter

                    Row {
                        spacing: 6
                        Label { text: LudeloTheme.hintSelect; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                        Label { text: qsTr("PLAY / STORE"); font.family: LudeloTheme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Label { text: LudeloTheme.hintDetails; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                        Label { text: qsTr("FAVORITE"); font.family: LudeloTheme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Label { text: LudeloTheme.isGamepad ? "[Y]" : "[F5]"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                        Label { text: qsTr("SEARCH"); font.family: LudeloTheme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Label { text: LudeloTheme.hintSettings; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                        Label { text: qsTr("SORT"); font.family: LudeloTheme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; color: LudeloTheme.textSecondary }
                    }
                }

                Item { Layout.fillWidth: true }

                // Honest technical footer
                Label {
                    text: qsTr("Ludelo Cloud Client • Sony PSN Protocol • Hardware Accelerated")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    color: LudeloTheme.textMuted
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }

    // Launch cloud streaming session helper
    function launchCloudStreamSession(streamingId, platform, serviceType) {
        console.log("Starting cloud streaming session for:", streamingId, platform, serviceType);

        let mainComp = root;
        while (mainComp && !mainComp.showStreamView) {
            mainComp = mainComp.parent;
        }
        if (mainComp && mainComp.showStreamView) {
            mainComp.showStreamView();
        }

        Chiaki.cloudStreaming.startCompleteCloudSession(
            serviceType,
            streamingId,
            function(success, message, serverIp) {
                console.log("Cloud streaming result:", success ? "SUCCESS" : "FAILED", message);
                if (!success) {
                    let isOAuth = message && (message.includes("OAuth") || message.includes("authorization"));
                    let duration = isOAuth ? 10000 : 3000;
                    Chiaki.error(qsTr("Cloud Streaming Failed"), message, duration);
                }
            }
        );
    }

    // Shortcut creation dialog
    GameShortcutDialog {
        id: cloudShortcutDialog
        anchors.centerIn: parent
    }
}
