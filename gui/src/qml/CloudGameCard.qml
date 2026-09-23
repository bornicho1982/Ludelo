import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import org.streetpea.chiaking
import Ludelo 1.0

Rectangle {
    id: card

    property var gameData
    property bool isHovered: cardHoverHandler.hovered
    property bool isCurrentItem: GridView.isCurrentItem || false
    property bool hasFocus: isCurrentItem && GridView.view.activeFocus
    property bool isPsnow: isPsnowGame()
    property string cachedImageUrl: ""
    property var qrCodeDialog: null // Kept for API compatibility

    // Modern PS Plus / Store catalog distinction
    readonly property bool needsAddToLibrary: gameData && gameData.category === "purchaseable"
    property bool isFavorite: false

    // Steam library shortcut toggle
    readonly property bool showCloudSteamShortcut: Chiaki.cloudSteamShortcutEnabled && !needsAddToLibrary

    signal streamGame(string productId, string platform, string serviceType)
    signal createShortcut(string productId, string entitlementId, string platform, string serviceType, string gameName)
    signal toggleFavorite(string productId)

    function isPsnowGame() {
        return !!(gameData && gameData.serviceType === "psnow");
    }

    function getGameName() {
        if (!gameData) return qsTr("Unknown Game");
        if (gameData.name) return gameData.name;
        if (gameData.game_meta && gameData.game_meta.name) return gameData.game_meta.name;
        return qsTr("Unknown Game");
    }

    function getProductId() {
        if (!gameData) return "";
        if (gameData.product_id) return gameData.product_id;
        if (gameData.productId) return gameData.productId;
        if (gameData.id) return gameData.id;
        return "";
    }

    function getProductIdForApi() {
        if (!gameData) return "";
        return gameData.productId || gameData.product_id || gameData.id || "";
    }

    function getStreamingIdentifier() {
        if (!gameData) return "";
        return gameData.streamIdentifier || getProductId();
    }

    function getPlatform() {
        return (gameData && gameData.platform) ? gameData.platform : "ps4";
    }

    function getServiceType() {
        return (gameData && gameData.serviceType) ? gameData.serviceType : "pscloud";
    }

    function getStreamServiceType() {
        if (gameData && gameData.streamServiceType) return gameData.streamServiceType;
        return getServiceType();
    }

    function getImageUrl() {
        if (!gameData) return "";
        if (gameData.extracted_images) {
            if (gameData.extracted_images.cover) return gameData.extracted_images.cover;
            if (gameData.extracted_images.landscape) return gameData.extracted_images.landscape;
        }
        if (!isPsnow) {
            if (gameData.imageUrl) return gameData.imageUrl;
            if (gameData.images && Array.isArray(gameData.images) && gameData.images.length > 0) {
                for (let i = 0; i < gameData.images.length; i++) {
                    let img = gameData.images[i];
                    if (img && img.url && img.type === 10) return img.url;
                }
                for (let i = 0; i < gameData.images.length; i++) {
                    let img = gameData.images[i];
                    if (img && img.url && (img.type === 12 || img.type === 13)) return img.url;
                }
                for (let i = 0; i < gameData.images.length; i++) {
                    let img = gameData.images[i];
                    if (img && img.url) return img.url;
                }
            }
        } else {
            if (gameData.imageUrl) return gameData.imageUrl;
            if (gameData.images && Array.isArray(gameData.images)) {
                for (let i = 0; i < gameData.images.length; i++) {
                    let img = gameData.images[i];
                    if (img && img.url && img.type === 10) return img.url;
                }
                for (let i = 0; i < gameData.images.length; i++) {
                    let img = gameData.images[i];
                    if (img && img.url && (img.type === 12 || img.type === 13)) return img.url;
                }
            }
        }
        return "";
    }

    Component.onCompleted: {
        let initialUrl = getImageUrl();
        if (initialUrl) {
            cachedImageUrl = initialUrl;
        }
    }

    onGameDataChanged: {
        let u = getImageUrl();
        cachedImageUrl = u ? u : "";
    }

    // Card styling
    color: (isHovered || isCurrentItem) ? LudeloTheme.bgCardHover : LudeloTheme.bgCard
    radius: LudeloTheme.radiusCard
    border.width: (isHovered || isCurrentItem) ? 2 : 1
    border.color: {
        if (isCurrentItem) return LudeloTheme.accentMint;
        if (isHovered) return LudeloTheme.accent;
        return LudeloTheme.borderSubtle;
    }

    Behavior on color {
        enabled: !(typeof Chiaki !== "undefined" && Chiaki.window && Chiaki.window.isResizing)
        ColorAnimation { duration: LudeloTheme.animFast }
    }
    Behavior on border.color {
        enabled: !(typeof Chiaki !== "undefined" && Chiaki.window && Chiaki.window.isResizing)
        ColorAnimation { duration: LudeloTheme.animFast }
    }

    HoverHandler {
        id: cardHoverHandler
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: false
        onClicked: {
            if (parent.GridView && parent.GridView.view) {
                parent.GridView.view.currentIndex = index;
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Game Cover Art Container
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            color: LudeloTheme.bgElevated
            radius: LudeloTheme.radiusCard - 2
            clip: true

            Image {
                id: gameImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                smooth: true
                sourceSize.width: 250
                sourceSize.height: 350
                source: cachedImageUrl || ""

                BusyIndicator {
                    anchors.centerIn: parent
                    running: gameImage.status === Image.Loading
                    visible: running
                    width: 32
                    height: 32
                }

                // Typographic fallback initials
                Rectangle {
                    anchors.fill: parent
                    color: Qt.rgba(0.08, 0.10, 0.16, 0.9)
                    visible: gameImage.status !== Image.Ready && gameImage.status !== Image.Loading

                    Label {
                        anchors.centerIn: parent
                        text: getGameName().substring(0, 2).toUpperCase()
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 42
                        font.weight: Font.Black
                        color: LudeloTheme.accentMint
                        opacity: 0.35
                    }
                }
            }

            // Dark gradient overlay on bottom of image for readability
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 50
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Qt.rgba(0.04, 0.05, 0.08, 0.85) }
                }
            }

            // Favorite Button (Top Left)
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 6
                width: 28
                height: 28
                radius: 14
                color: favMouseArea.containsMouse ? Qt.rgba(0, 0, 0, 0.75) : Qt.rgba(0, 0, 0, 0.45)
                border.color: card.isFavorite ? LudeloTheme.colorWarning : LudeloTheme.borderSubtle
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: card.isFavorite ? "★" : "☆"
                    font.pixelSize: 16
                    color: card.isFavorite ? LudeloTheme.colorWarning : LudeloTheme.textMuted
                }

                MouseArea {
                    id: favMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        let productId = getProductId();
                        if (productId) {
                            toggleFavorite(productId);
                        }
                    }
                }
            }

            // Category Badge (Top Right)
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 6
                height: 22
                width: categoryText.implicitWidth + 14
                radius: 4
                color: {
                    if (!gameData) return Qt.rgba(0.96, 0.62, 0.04, 0.25);
                    if (gameData.category === "owned") return Qt.rgba(0.0, 0.96, 0.83, 0.22);
                    if (gameData.category === "streamable") return Qt.rgba(0.42, 0.36, 0.90, 0.25);
                    return Qt.rgba(0.96, 0.62, 0.04, 0.25);
                }
                border.color: {
                    if (!gameData) return LudeloTheme.colorWarning;
                    if (gameData.category === "owned") return LudeloTheme.accentMint;
                    if (gameData.category === "streamable") return LudeloTheme.accent;
                    return LudeloTheme.colorWarning;
                }
                border.width: 1

                Label {
                    id: categoryText
                    anchors.centerIn: parent
                    text: {
                        if (!gameData || !gameData.category) return "CLOUD";
                        if (gameData.category === "owned") return "OWNED";
                        if (gameData.category === "streamable") return "STREAMABLE";
                        return "REQUIRES PS PLUS";
                    }
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: {
                        if (!gameData) return LudeloTheme.colorWarning;
                        if (gameData.category === "owned") return LudeloTheme.accentMint;
                        if (gameData.category === "streamable") return LudeloTheme.accentMint;
                        return LudeloTheme.colorWarning;
                    }
                }
            }

            // Platform tag (Bottom Right of Image)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 6
                height: 20
                width: platformText.implicitWidth + 10
                radius: 4
                color: Qt.rgba(0, 0, 0, 0.75)
                border.color: LudeloTheme.borderSubtle
                border.width: 1
                visible: getPlatform() !== ""

                Label {
                    id: platformText
                    anchors.centerIn: parent
                    text: getPlatform().toUpperCase()
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }
            }
        }

        // Title and Info
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                id: titleLabel
                Layout.fillWidth: true
                text: getGameName()
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: (isHovered || isCurrentItem) ? LudeloTheme.textPrimary : LudeloTheme.textSecondary
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Label {
                Layout.fillWidth: true
                text: {
                    if (needsAddToLibrary) return "External PS Store link";
                    if (gameData && gameData.serviceType === "psnow") return "PlayStation Now Stream";
                    return "Direct Cloud Streaming";
                }
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 10
                color: LudeloTheme.textMuted
                elide: Text.ElideRight
            }
        }

        // Action Buttons Row
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            spacing: 6

            // Steam Shortcut Button (Optional, when enabled)
            Rectangle {
                Layout.preferredWidth: 32
                Layout.fillHeight: true
                visible: showCloudSteamShortcut
                radius: 6
                color: steamBtnArea.containsMouse ? LudeloTheme.bgHover : Qt.rgba(1, 1, 1, 0.05)
                border.color: steamBtnArea.containsMouse ? LudeloTheme.borderMedium : LudeloTheme.borderSubtle
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: "[X]"
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: LudeloTheme.textSecondary
                }

                MouseArea {
                    id: steamBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        let productIdForApi = getProductIdForApi();
                        let entitlementId = getStreamingIdentifier();
                        let platform = getPlatform();
                        let serviceType = getServiceType();
                        let gameName = getGameName();
                        if (productIdForApi !== "") {
                            createShortcut(productIdForApi, entitlementId, platform, serviceType, gameName);
                        }
                    }
                }
            }

            // Primary Action Button: PLAY or REQUIRES PS PLUS
            Rectangle {
                id: playBtnRect
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                color: {
                    if (needsAddToLibrary) {
                        return playBtnMouseArea.containsMouse ? Qt.rgba(0.96, 0.62, 0.04, 0.3) : Qt.rgba(0.96, 0.62, 0.04, 0.15);
                    }
                    if (isHovered || isCurrentItem) {
                        return playBtnMouseArea.containsMouse ? LudeloTheme.accentHover : LudeloTheme.accent;
                    }
                    return Qt.rgba(0.42, 0.36, 0.90, 0.2);
                }
                border.color: {
                    if (needsAddToLibrary) return LudeloTheme.colorWarning;
                    if (isHovered || isCurrentItem) return LudeloTheme.accentMint;
                    return LudeloTheme.borderSubtle;
                }
                border.width: 1

                Row {
                    anchors.centerIn: parent
                    spacing: 6

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "[A]"
                        font.family: LudeloTheme.fontFamilyMono
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: needsAddToLibrary ? LudeloTheme.colorWarning : LudeloTheme.accentMint
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: needsAddToLibrary ? qsTr("PS PLUS") : qsTr("PLAY")
                        font.family: LudeloTheme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: LudeloTheme.textPrimary
                    }
                }

                MouseArea {
                    id: playBtnMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: executeAction()
                }
            }
        }
    }

    function executeAction() {
        if (needsAddToLibrary) {
            // Non-owned / store game: open PlayStation official store externally (never charge inside app)
            let conceptUrl = gameData.conceptUrl || gameData.concept_url;
            if (conceptUrl && conceptUrl.indexOf("http") === 0) {
                Qt.openUrlExternally(conceptUrl);
            } else {
                Qt.openUrlExternally("https://www.playstation.com/ps-plus");
            }
        } else {
            // Streamable / Owned game: initiate cloud session
            let streamingId = getStreamingIdentifier();
            let platform = getPlatform();
            if (streamingId !== "") {
                streamGame(streamingId, platform, getStreamServiceType());
            }
        }
    }

    // Keyboard / Controller input handlers
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Space || event.key === Qt.Key_Enter) {
            executeAction();
            event.accepted = true;
        } else if (event.key === Qt.Key_X && showCloudSteamShortcut) {
            let productIdForApi = getProductIdForApi();
            let entitlementId = getStreamingIdentifier();
            let platform = getPlatform();
            let serviceType = getServiceType();
            let gameName = getGameName();
            if (productIdForApi !== "") {
                createShortcut(productIdForApi, entitlementId, platform, serviceType, gameName);
                event.accepted = true;
            }
        }
    }
}
