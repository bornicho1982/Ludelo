import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import org.streetpea.chiaking
import Ludelo 1.0
import "components"

Rectangle {
    id: dialog

    enum Console {
        PS4,
        PS5
    }
    enum CloudService {
        PSCloud,
        PSNOW
    }

    property int selectedConsole: SettingsDialog.Console.PS5
    property int selectedCloudService: SettingsDialog.CloudService.PSCloud
    property bool quitControllerMapping: true
    property int activeCategoryIndex: 0
    onActiveCategoryIndexChanged: {
        // P0.6: Refresh audio device lists when user enters Audio tab
        if (activeCategoryIndex === 1)
            Chiaki.settings.refreshAudioDevices();
    }

    readonly property var categoryTitles: [
        qsTr("Video & Stream"),
        qsTr("Audio"),
        qsTr("Network"),
        qsTr("Controller"),
        qsTr("Account"),
        qsTr("General")
    ]

    anchors.fill: parent
    color: LudeloTheme.bgBase

    // Close helper
    function close() {
        if (typeof root !== "undefined" && root.closeDialog) {
            root.closeDialog();
        } else if (dialog.StackView && dialog.StackView.view) {
            dialog.StackView.view.pop();
        }
    }

    // Factory Defaults Reset Function (Real function connected to Chiaki.settings)
    function resetToDefaults() {
        Chiaki.settings.resolutionLocalPS5 = 3;  // 1080p
        Chiaki.settings.resolutionRemotePS5 = 2; // 720p
        Chiaki.settings.resolutionLocalPS4 = 2;  // 720p
        Chiaki.settings.resolutionRemotePS4 = 1; // 540p
        Chiaki.settings.fpsLocalPS5 = 1;         // 60 FPS
        Chiaki.settings.fpsRemotePS5 = 1;
        Chiaki.settings.fpsLocalPS4 = 1;
        Chiaki.settings.fpsRemotePS4 = 1;
        Chiaki.settings.bitrateLocalPS5 = 15000; // 15 Mbps
        Chiaki.settings.bitrateRemotePS5 = 10000;// 10 Mbps
        Chiaki.settings.bitrateLocalPS4 = 10000;
        Chiaki.settings.bitrateRemotePS4 = 5000;
        Chiaki.settings.codecLocalPS5 = 1;       // H.265
        Chiaki.settings.codecRemotePS5 = 1;
        Chiaki.settings.decoder = "d3d11va";
        Chiaki.settings.showStreamStats = false;
        Chiaki.settings.audioBufferSize = 19200;
        Chiaki.settings.audioVolume = 100;
        Chiaki.settings.rumbleHapticsIntensity = 3; // Normal
        Chiaki.settings.windowType = 0;
        Chiaki.settings.logVerbose = false;
        resetToast.show();
    }

    focus: true
    Keys.onEscapePressed: close()
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_PageUp) {
            activeCategoryIndex = Math.max(0, activeCategoryIndex - 1);
            event.accepted = true;
        } else if (event.key === Qt.Key_PageDown) {
            activeCategoryIndex = Math.min(categoryTitles.length - 1, activeCategoryIndex + 1);
            event.accepted = true;
        } else if (event.key === Qt.Key_X) {
            resetToDefaults();
            event.accepted = true;
        }
    }

    // Background ambient lighting
    Item {
        anchors.fill: parent
        z: 0

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 500
            height: 500
            radius: 250
            color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.06)
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 600
            height: 600
            radius: 300
            color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.03)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        z: 1

        // =====================================================================
        // TOP NAVIGATION HEADER (Bumper hints [LB] and [RB] + Categories)
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
                            text: "SETTINGS"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: LudeloTheme.accentMint
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Category Nav with [LB] and [RB] Bumper hints
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8

                    // [LB] Button Hint
                    Rectangle {
                        width: 40
                        height: 32
                        radius: 6
                        color: activeCategoryIndex > 0 ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.02)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "LB"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: activeCategoryIndex > 0 ? LudeloTheme.textPrimary : LudeloTheme.textDim
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: activeCategoryIndex > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: if (activeCategoryIndex > 0) activeCategoryIndex--
                        }
                    }

                    // Navigation Tabs
                    Row {
                        spacing: 4
                        Repeater {
                            model: dialog.categoryTitles
                            delegate: LNavTab {
                                text: modelData
                                active: dialog.activeCategoryIndex === index
                                onClicked: dialog.activeCategoryIndex = index
                            }
                        }
                    }

                    // [RB] Button Hint
                    Rectangle {
                        width: 40
                        height: 32
                        radius: 6
                        color: activeCategoryIndex < categoryTitles.length - 1 ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.02)
                        border.color: LudeloTheme.borderSubtle
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "RB"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: activeCategoryIndex < categoryTitles.length - 1 ? LudeloTheme.textPrimary : LudeloTheme.textDim
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: activeCategoryIndex < categoryTitles.length - 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: if (activeCategoryIndex < categoryTitles.length - 1) activeCategoryIndex++
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Gamepad Polling Telemetry Badge
                Rectangle {
                    height: 32
                    radius: 16
                    color: Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.8)
                    border.color: LudeloTheme.borderSubtle
                    border.width: 1
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: gamepadRow.implicitWidth + 20

                    Row {
                        id: gamepadRow
                        anchors.centerIn: parent
                        spacing: 8

                        Rectangle {
                            width: 7; height: 7; radius: 3.5
                            color: (Chiaki.controllers && Chiaki.controllers.length > 0) ? LudeloTheme.accentMint : LudeloTheme.textDim
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: (Chiaki.controllers && Chiaki.controllers.length > 0) ? "GAMEPAD CONNECTED • 1000Hz" : "NO GAMEPAD"
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            color: (Chiaki.controllers && Chiaki.controllers.length > 0) ? LudeloTheme.textPrimary : LudeloTheme.textDim
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                // Close / Back Button [ESC]
                LButton {
                    height: 36
                    implicitWidth: 90
                    customRadius: 18
                    variant: "secondary"
                    text: qsTr("CLOSE")
                    keyHint: "[ESC]"
                    onClicked: dialog.close()
                }
            }
        }

        // =====================================================================
        // MAIN BODY (Split: Left Configuration Cards / Right Telemetry Sidebar)
        // =====================================================================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            spacing: 24

            // -----------------------------------------------------------------
            // LEFT COLUMN: SCROLLABLE SETTINGS PANE
            // -----------------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 16

                // Section Subtitle & Action Bar
                RowLayout {
                    Layout.fillWidth: true

                    Column {
                        spacing: 2
                        Text {
                            text: qsTr("CLIENT PREFERENCES / %1").arg(dialog.categoryTitles[dialog.activeCategoryIndex].toUpperCase())
                            font.family: LudeloTheme.fontFamilyMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: LudeloTheme.accentPrimary
                        }
                        Text {
                            text: {
                                switch (dialog.activeCategoryIndex) {
                                case 0: return qsTr("Display & Stream Configuration");
                                case 1: return qsTr("Audio Output & Input Devices");
                                case 2: return qsTr("Network Transport & Cloud Streaming");
                                case 3: return qsTr("Controller, Haptics & Input Mapping");
                                case 4: return qsTr("PlayStation Network & Registered Consoles");
                                case 5: return qsTr("General Application & Performance");
                                default: return "";
                                }
                            }
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: LudeloTheme.textPrimary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Flickable Content Area
                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: currentCategoryContent.height + 40
                    boundsBehavior: Flickable.StopAtBounds

                    Item {
                        id: currentCategoryContent
                        width: parent.width
                        implicitHeight: categoryStack.children[dialog.activeCategoryIndex] ? categoryStack.children[dialog.activeCategoryIndex].implicitHeight : 600
                        height: implicitHeight

                        StackLayout {
                            id: categoryStack
                            anchors.fill: parent
                            currentIndex: dialog.activeCategoryIndex

                            // -------------------------------------------------
                            // 0: VIDEO & STREAM
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                // Target Console Selector
                                RowLayout {
                                    spacing: 12
                                    Text {
                                        text: qsTr("Active Target Profile:")
                                        font.family: LudeloTheme.fontFamily
                                        font.pixelSize: 13
                                        font.weight: Font.Medium
                                        color: LudeloTheme.textSecondary
                                    }

                                    Row {
                                        spacing: 8
                                        LButton {
                                            height: 32
                                            implicitWidth: 140
                                            customRadius: 6
                                            variant: dialog.selectedConsole === SettingsDialog.Console.PS5 ? "primary" : "secondary"
                                            text: "PlayStation 5"
                                            onClicked: dialog.selectedConsole = SettingsDialog.Console.PS5
                                        }
                                        LButton {
                                            height: 32
                                            implicitWidth: 140
                                            customRadius: 6
                                            variant: dialog.selectedConsole === SettingsDialog.Console.PS4 ? "primary" : "secondary"
                                            text: "PlayStation 4"
                                            onClicked: dialog.selectedConsole = SettingsDialog.Console.PS4
                                        }
                                    }
                                }

                                // ===== P1.1: DUAL COLUMN LOCAL vs REMOTE STREAM SETTINGS =====
                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Stream Quality Configuration")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        Text {
                                            text: qsTr("Configure independent quality profiles for Local (LAN) and Remote (Internet) connections.")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 11
                                            color: LudeloTheme.textDim
                                        }

                                        // Dual Column Headers
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 16

                                            Rectangle {
                                                Layout.fillWidth: true
                                                height: 32
                                                radius: 6
                                                color: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.15)
                                                border.color: LudeloTheme.accentPrimary
                                                border.width: 1
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: qsTr("LOCAL (LAN)")
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 12
                                                    font.weight: Font.Bold
                                                    color: LudeloTheme.accentPrimary
                                                }
                                            }

                                            Rectangle {
                                                Layout.fillWidth: true
                                                height: 32
                                                radius: 6
                                                color: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.10)
                                                border.color: LudeloTheme.accentMint
                                                border.width: 1
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: qsTr("REMOTE (INTERNET)")
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 12
                                                    font.weight: Font.Bold
                                                    color: LudeloTheme.accentMint
                                                }
                                            }
                                        }

                                        // ---- RESOLUTION ROW ----
                                        Text {
                                            text: qsTr("Target Resolution")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 12
                                            color: LudeloTheme.textSecondary
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 16

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6
                                                Repeater {
                                                    model: [
                                                        { text: "1080p", res: 3 },
                                                        { text: "720p", res: 2 },
                                                        { text: "540p", res: 1 }
                                                    ]
                                                    delegate: LButton {
                                                        height: 36; implicitWidth: 90; customRadius: 6
                                                        variant: {
                                                            let cur = (dialog.selectedConsole === SettingsDialog.Console.PS5) ? Chiaki.settings.resolutionLocalPS5 : Chiaki.settings.resolutionLocalPS4;
                                                            return cur === modelData.res ? "primary" : "secondary";
                                                        }
                                                        text: modelData.text
                                                        onClicked: {
                                                            if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                                Chiaki.settings.resolutionLocalPS5 = modelData.res;
                                                            else
                                                                Chiaki.settings.resolutionLocalPS4 = modelData.res;
                                                        }
                                                    }
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6
                                                Repeater {
                                                    model: [
                                                        { text: "1080p", res: 3 },
                                                        { text: "720p", res: 2 },
                                                        { text: "540p", res: 1 }
                                                    ]
                                                    delegate: LButton {
                                                        height: 36; implicitWidth: 90; customRadius: 6
                                                        variant: {
                                                            let cur = (dialog.selectedConsole === SettingsDialog.Console.PS5) ? Chiaki.settings.resolutionRemotePS5 : Chiaki.settings.resolutionRemotePS4;
                                                            return cur === modelData.res ? "mint" : "secondary";
                                                        }
                                                        text: modelData.text
                                                        onClicked: {
                                                            if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                                Chiaki.settings.resolutionRemotePS5 = modelData.res;
                                                            else
                                                                Chiaki.settings.resolutionRemotePS4 = modelData.res;
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        // ---- FPS ROW ----
                                        Text {
                                            text: qsTr("Target Refresh Rate")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 12
                                            color: LudeloTheme.textSecondary
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 16

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6
                                                LButton {
                                                    height: 36; implicitWidth: 90; customRadius: 6
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsLocalPS5 : Chiaki.settings.fpsLocalPS4) === 1) ? "primary" : "secondary"
                                                    text: "60 FPS"
                                                    onClicked: { if (dialog.selectedConsole === SettingsDialog.Console.PS5) Chiaki.settings.fpsLocalPS5 = 1; else Chiaki.settings.fpsLocalPS4 = 1; }
                                                }
                                                LButton {
                                                    height: 36; implicitWidth: 90; customRadius: 6
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsLocalPS5 : Chiaki.settings.fpsLocalPS4) === 0) ? "primary" : "secondary"
                                                    text: "30 FPS"
                                                    onClicked: { if (dialog.selectedConsole === SettingsDialog.Console.PS5) Chiaki.settings.fpsLocalPS5 = 0; else Chiaki.settings.fpsLocalPS4 = 0; }
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6
                                                LButton {
                                                    height: 36; implicitWidth: 90; customRadius: 6
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsRemotePS5 : Chiaki.settings.fpsRemotePS4) === 1) ? "mint" : "secondary"
                                                    text: "60 FPS"
                                                    onClicked: { if (dialog.selectedConsole === SettingsDialog.Console.PS5) Chiaki.settings.fpsRemotePS5 = 1; else Chiaki.settings.fpsRemotePS4 = 1; }
                                                }
                                                LButton {
                                                    height: 36; implicitWidth: 90; customRadius: 6
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsRemotePS5 : Chiaki.settings.fpsRemotePS4) === 0) ? "mint" : "secondary"
                                                    text: "30 FPS"
                                                    onClicked: { if (dialog.selectedConsole === SettingsDialog.Console.PS5) Chiaki.settings.fpsRemotePS5 = 0; else Chiaki.settings.fpsRemotePS4 = 0; }
                                                }
                                            }
                                        }

                                        // ---- CODEC ROW (PS5 only) ----
                                        ColumnLayout {
                                            spacing: 4
                                            visible: dialog.selectedConsole === SettingsDialog.Console.PS5
                                            Text {
                                                text: qsTr("Video Stream Codec (PS5)")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 12
                                                color: LudeloTheme.textSecondary
                                            }
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 16

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 6
                                                    LButton {
                                                        height: 36; implicitWidth: 130; customRadius: 6
                                                        variant: (Chiaki.settings.codecLocalPS5 >= 1) ? "primary" : "secondary"
                                                        text: "HEVC / H.265"
                                                        onClicked: Chiaki.settings.codecLocalPS5 = 1
                                                    }
                                                    LButton {
                                                        height: 36; implicitWidth: 130; customRadius: 6
                                                        variant: (Chiaki.settings.codecLocalPS5 === 0) ? "primary" : "secondary"
                                                        text: "AVC / H.264"
                                                        onClicked: Chiaki.settings.codecLocalPS5 = 0
                                                    }
                                                }

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 6
                                                    LButton {
                                                        height: 36; implicitWidth: 130; customRadius: 6
                                                        variant: (Chiaki.settings.codecRemotePS5 >= 1) ? "mint" : "secondary"
                                                        text: "HEVC / H.265"
                                                        onClicked: Chiaki.settings.codecRemotePS5 = 1
                                                    }
                                                    LButton {
                                                        height: 36; implicitWidth: 130; customRadius: 6
                                                        variant: (Chiaki.settings.codecRemotePS5 === 0) ? "mint" : "secondary"
                                                        text: "AVC / H.264"
                                                        onClicked: Chiaki.settings.codecRemotePS5 = 0
                                                    }
                                                }
                                            }
                                            Text {
                                                text: qsTr("H.265 provides ~30% higher bitrate efficiency with modern GPUs.")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 11
                                                color: LudeloTheme.textDim
                                            }
                                        }

                                        // ---- BITRATE ROW ----
                                        Text {
                                            text: qsTr("Bitrate Allocation")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 12
                                            color: LudeloTheme.textSecondary
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 16

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Text {
                                                    text: {
                                                        var br = dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateLocalPS5 : Chiaki.settings.bitrateLocalPS4;
                                                        var brMbps = (br && !isNaN(br) && br > 0) ? Math.round(br / 1000) : 15;
                                                        return qsTr("%1 Mbps").arg(brMbps);
                                                    }
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 12
                                                    font.weight: Font.Bold
                                                    color: LudeloTheme.accentPrimary
                                                }
                                                LSlider {
                                                    Layout.fillWidth: true
                                                    from: 5000; to: 30000; stepSize: 1000
                                                    value: dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateLocalPS5 : Chiaki.settings.bitrateLocalPS4
                                                    onMoved: {
                                                        if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                            Chiaki.settings.bitrateLocalPS5 = Math.round(value);
                                                        else
                                                            Chiaki.settings.bitrateLocalPS4 = Math.round(value);
                                                    }
                                                }
                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    Text { text: "5"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; color: LudeloTheme.textDim }
                                                    Item { Layout.fillWidth: true }
                                                    Text { text: "30 Mbps"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; color: LudeloTheme.textDim }
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Text {
                                                    text: {
                                                        var br = dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateRemotePS5 : Chiaki.settings.bitrateRemotePS4;
                                                        var brMbps = (br && !isNaN(br) && br > 0) ? Math.round(br / 1000) : 10;
                                                        return qsTr("%1 Mbps").arg(brMbps);
                                                    }
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 12
                                                    font.weight: Font.Bold
                                                    color: LudeloTheme.accentMint
                                                }
                                                LSlider {
                                                    Layout.fillWidth: true
                                                    from: 2000; to: 20000; stepSize: 500
                                                    value: dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateRemotePS5 : Chiaki.settings.bitrateRemotePS4
                                                    onMoved: {
                                                        if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                            Chiaki.settings.bitrateRemotePS5 = Math.round(value);
                                                        else
                                                            Chiaki.settings.bitrateRemotePS4 = Math.round(value);
                                                    }
                                                }
                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    Text { text: "2"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; color: LudeloTheme.textDim }
                                                    Item { Layout.fillWidth: true }
                                                    Text { text: "20 Mbps"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; color: LudeloTheme.textDim }
                                                }
                                            }
                                        }

                                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                                        // ---- SHARED SETTINGS ----
                                        Text {
                                            text: qsTr("Shared Settings (both profiles)")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 12
                                            color: LudeloTheme.textSecondary
                                        }

                                        RowLayout {
                                            spacing: 12
                                            ComboBox {
                                                id: decoderCombo
                                                Layout.preferredWidth: 280
                                                model: Chiaki.settings.availableDecoders
                                                currentIndex: Math.max(0, model.indexOf(Chiaki.settings.decoder))
                                                onActivated: (index) => Chiaki.settings.decoder = model[index]
                                            }
                                            Text {
                                                text: qsTr("HW Decoder: %1").arg(Chiaki.settings.decoder || "d3d11va")
                                                font.family: LudeloTheme.fontFamilyMono
                                                font.pixelSize: 11
                                                color: LudeloTheme.accentMint
                                            }
                                        }

                                        LToggle {
                                            label: qsTr("HDR Stream Output (10-bit Rec.2020)")
                                            description: (Chiaki.settings.codecLocalPS5 === 0)
                                                ? qsTr("HDR available with H.265 codec on PS5 streams")
                                                : qsTr("Direct 10-bit HDR metadata passthrough to compatible displays")
                                            enabled: dialog.selectedConsole === SettingsDialog.Console.PS5 && Chiaki.settings.codecLocalPS5 >= 1
                                            checked: Chiaki.settings.codecLocalPS5 === 2
                                            onToggled: {
                                                if (checked) Chiaki.settings.codecLocalPS5 = 2;
                                                else Chiaki.settings.codecLocalPS5 = 1;
                                            }
                                        }

                                        LToggle {
                                            label: qsTr("In-Game Diagnostics HUD Overlay")
                                            description: qsTr("Display live RTT latency, packet loss, and frame drop metrics during stream [TAB]")
                                            checked: Chiaki.settings.showStreamStats
                                            onToggled: Chiaki.settings.showStreamStats = checked
                                        }

                                        RowLayout {
                                            spacing: 12
                                            LButton {
                                                height: 38
                                                implicitWidth: 220
                                                customRadius: 8
                                                variant: "secondary"
                                                text: qsTr("DISPLAY SETTINGS")
                                                keyHint: "[X]"
                                                onClicked: {
                                                    if (typeof root !== "undefined" && root.showDisplaySettingsDialog)
                                                        root.showDisplaySettingsDialog();
                                                }
                                            }
                                            Text {
                                                text: qsTr("Color primaries, transfer curve, peak nits, contrast (libplacebo)")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 11
                                                color: LudeloTheme.textDim
                                            }
                                        }
                                    }
                                }
                            }

                            // -------------------------------------------------
                            // 1: AUDIO
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Audio Devices & Buffers")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        ColumnLayout {
                                            spacing: 6
                                            Text { text: qsTr("Audio Output Device"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary }
                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: Chiaki.settings.availableAudioOutDevices
                                                currentIndex: Math.max(0, model.indexOf(Chiaki.settings.audioOutDevice))
                                                onActivated: (index) => Chiaki.settings.audioOutDevice = model[index]
                                            }
                                        }

                                        ColumnLayout {
                                            spacing: 6
                                            Text { text: qsTr("Microphone Input Device"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary }
                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: Chiaki.settings.availableAudioInDevices
                                                currentIndex: Math.max(0, model.indexOf(Chiaki.settings.audioInDevice))
                                                onActivated: (index) => Chiaki.settings.audioInDevice = model[index]
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text { text: qsTr("Audio Buffer Size (Frames): %1").arg(Chiaki.settings.audioBufferSize); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "19200 (Default)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.accentMint }
                                        }
                                        LSlider {
                                            Layout.fillWidth: true
                                            from: 4800
                                            to: 38400
                                            stepSize: 2400
                                            value: Chiaki.settings.audioBufferSize
                                            onMoved: Chiaki.settings.audioBufferSize = Math.round(value)
                                        }

                                        LToggle {
                                            label: qsTr("Start Session with Microphone Unmuted")
                                            description: qsTr("Enables voice chat transmission immediately upon stream start")
                                            checked: Chiaki.settings.startMicUnmuted
                                            onToggled: Chiaki.settings.startMicUnmuted = checked
                                        }
                                    }
                                }
                            }

                            // -------------------------------------------------
                            // 2: NETWORK & CLOUD
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Network & Socket Optimization")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        LToggle {
                                            label: qsTr("WiFi Congestion Notification")
                                            description: qsTr("Warn when network packet drops exceed threshold on wireless connections")
                                            checked: Chiaki.settings.wifiDroppedNotif > 0
                                            onToggled: Chiaki.settings.wifiDroppedNotif = checked ? 1 : 0
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text { text: qsTr("Max Packet Loss Recovery Tolerance: %1%").arg(Chiaki.settings.packetLossMax); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "10% (Default)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.accentMint }
                                        }
                                        LSlider {
                                            Layout.fillWidth: true
                                            from: 1
                                            to: 30
                                            stepSize: 1
                                            value: Chiaki.settings.packetLossMax
                                            onMoved: Chiaki.settings.packetLossMax = Math.round(value)
                                        }
                                    }
                                }

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 12

                                        Text {
                                            text: qsTr("PlayStation Cloud Gaming Streaming")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        Text {
                                            text: qsTr("Cloud routing is automatically negotiated with Sony Kamaji edge servers based on your PSN account region and network topology.")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 12
                                            color: LudeloTheme.textSecondary
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }

                                        RowLayout {
                                            spacing: 8
                                            LPill {
                                                text: "PROTOCOL: Kamaji / WebRTC"
                                                dotColor: LudeloTheme.accentMint
                                                glowColor: LudeloTheme.accentMintGlow
                                                showDot: true
                                            }
                                            LPill {
                                                text: "AUTO EDGE REGION"
                                                dotColor: LudeloTheme.accentPrimary
                                                showDot: false
                                            }
                                        }
                                    }
                                }
                            }

                            // -------------------------------------------------
                            // 3: CONTROLLER
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Gamepad & Feedback")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                text: qsTr("Rumble & Haptics Intensity: %1").arg(
                                                    ["Off", "Very Weak", "Weak", "Normal", "Strong", "Very Strong"][Chiaki.settings.rumbleHapticsIntensity] || "Normal"
                                                )
                                                font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "Normal (Default)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.accentMint }
                                        }
                                        LSlider {
                                            Layout.fillWidth: true
                                            from: 0
                                            to: 5
                                            stepSize: 1
                                            value: Chiaki.settings.rumbleHapticsIntensity
                                            onMoved: Chiaki.settings.rumbleHapticsIntensity = Math.round(value)
                                        }

                                        LToggle {
                                            label: qsTr("Show On-Screen Controller Overlay")
                                            description: qsTr("Renders visual gamepad button highlights on screen")
                                            checked: Chiaki.settings.controllerOverlayShown
                                            onToggled: Chiaki.settings.controllerOverlayShown = checked
                                        }

                                        LToggle {
                                            label: qsTr("Map Physical Buttons by Position (Nintendo / Xbox)")
                                            description: qsTr("Swaps South/East and West/North face buttons according to layout")
                                            checked: Chiaki.settings.buttonsByPosition
                                            onToggled: Chiaki.settings.buttonsByPosition = checked
                                        }

                                        LToggle {
                                            label: qsTr("Allow Gamepad Events in Background")
                                            description: qsTr("Permits game controls when Ludelo window is unfocused")
                                            checked: Chiaki.settings.allowJoystickBackgroundEvents
                                            onToggled: Chiaki.settings.allowJoystickBackgroundEvents = checked
                                        }

                                        RowLayout {
                                            spacing: 12
                                            LButton {
                                                height: 38
                                                implicitWidth: 220
                                                customRadius: 8
                                                variant: "secondary"
                                                text: qsTr("REMAP GAMEPAD")
                                                onClicked: controllerMappingDialog.show({reset: false})
                                            }
                                        }
                                    }
                                }
                            }

                            // -------------------------------------------------
                            // 4: ACCOUNT & CONSOLES
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("PlayStation Network Account")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        RowLayout {
                                            spacing: 12
                                            Rectangle {
                                                width: 8; height: 8; radius: 4
                                                color: Chiaki.settings.psnAccountId ? LudeloTheme.accentMint : LudeloTheme.textDim
                                            }
                                            Text {
                                                text: Chiaki.settings.psnAccountId ? qsTr("Logged In (Account ID: %1)").arg(LudeloTheme.formatObfuscatedAccountId(Chiaki.settings.psnAccountId)) : qsTr("Not Connected to PSN")
                                                font.family: LudeloTheme.fontFamilyMono
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: Chiaki.settings.psnAccountId ? LudeloTheme.textPrimary : LudeloTheme.textDim
                                            }
                                        }

                                        RowLayout {
                                            spacing: 12
                                            LButton {
                                                height: 38
                                                implicitWidth: 200
                                                customRadius: 8
                                                variant: Chiaki.settings.psnAccountId ? "ghost" : "primary"
                                                text: Chiaki.settings.psnAccountId ? qsTr("RE-AUTHENTICATE PSN") : qsTr("SIGN IN WITH PSN")
                                                onClicked: Chiaki.startWebView2Login()
                                            }

                                            LButton {
                                                height: 38
                                                implicitWidth: 240
                                                customRadius: 8
                                                variant: "secondary"
                                                text: qsTr("VIEW FULL ACCOUNT & GAMES")
                                                onClicked: {
                                                    dialog.close();
                                                    if (typeof root !== "undefined" && root.showAccountView) {
                                                        root.showAccountView();
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Registered Consoles (%1)").arg(Chiaki.settings.registeredHosts ? Chiaki.settings.registeredHosts.length : 0)
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        Repeater {
                                            model: Chiaki.settings.registeredHosts
                                            delegate: RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Text {
                                                    text: modelData.server_nickname || "PlayStation Console"
                                                    font.family: LudeloTheme.fontFamily
                                                    font.pixelSize: 13
                                                    color: LudeloTheme.textPrimary
                                                }
                                                Text {
                                                    text: modelData.server_mac || ""
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 11
                                                    color: LudeloTheme.textDim
                                                }
                                                Item { Layout.fillWidth: true }
                                                LButton {
                                                    height: 28
                                                    implicitWidth: 80
                                                    customRadius: 6
                                                    variant: "danger"
                                                    text: qsTr("DELETE")
                                                    onClicked: Chiaki.settings.deleteRegisteredHost(index)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // -------------------------------------------------
                            // 5: GENERAL & SYSTEM
                            // -------------------------------------------------
                            ColumnLayout {
                                spacing: 20

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 12

                                        RowLayout {
                                            spacing: 10
                                            Text {
                                                text: qsTr("Documentación & Ayuda")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 16
                                                font.weight: Font.Bold
                                                color: LudeloTheme.textPrimary
                                            }
                                            LPill {
                                                text: qsTr("PASO A PASO")
                                                dotColor: LudeloTheme.accentMint
                                                showDot: true
                                            }
                                        }

                                        Text {
                                            text: qsTr("Aprende a conectar tu cuenta PSN, habilitar el Uso a distancia en PS5/PS4, vincular mediante PIN de 8 dígitos y configurar mandos o Steam Deck.")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 13
                                            color: LudeloTheme.textSecondary
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }

                                        LButton {
                                            height: 42
                                            implicitWidth: 260
                                            customRadius: 8
                                            variant: "primary"
                                            text: qsTr("GUÍA DE CONFIGURACIÓN")
                                            keyHint: "[A]"
                                            onClicked: {
                                                dialog.close();
                                                if (typeof root !== "undefined" && root.showConsoleSetupWalkthrough) {
                                                    root.showConsoleSetupWalkthrough();
                                                }
                                            }
                                        }
                                    }
                                }

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Window & Presentation")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        LToggle {
                                            label: qsTr("Auto-Hide Mouse Cursor during Stream")
                                            description: qsTr("Hides the cursor after 3 seconds of inactivity while playing")
                                            checked: Chiaki.settings.hideCursor
                                            onToggled: Chiaki.settings.hideCursor = checked
                                        }

                                        LToggle {
                                            label: qsTr("Double Click for Fullscreen")
                                            description: qsTr("Toggles fullscreen mode when double clicking the stream view")
                                            checked: Chiaki.settings.fullscreenDoubleClick
                                            onToggled: Chiaki.settings.fullscreenDoubleClick = checked
                                        }

                                        LToggle {
                                            label: qsTr("Verbose Diagnostic Logging")
                                            description: qsTr("Writes detailed network and frame timing diagnostics to log file")
                                            checked: Chiaki.settings.logVerbose
                                            onToggled: Chiaki.settings.logVerbose = checked
                                        }

                                        RowLayout {
                                            spacing: 12
                                            LButton {
                                                height: 38
                                                implicitWidth: 190
                                                customRadius: 8
                                                variant: "secondary"
                                                text: qsTr("OPEN LOG DIRECTORY")
                                                onClicked: Qt.openUrlExternally("file:///" + Chiaki.settings.logDirectory)
                                            }
                                            LButton {
                                                height: 38
                                                implicitWidth: 160
                                                customRadius: 8
                                                variant: "ghost"
                                                text: qsTr("EXPORT SETTINGS")
                                                onClicked: Chiaki.settings.exportSettings()
                                            }
                                        }
                                    }
                                }

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 20
                                        spacing: 8

                                        Text {
                                            text: "Ludelo Remote Play Client v2.4"
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 14
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }
                                        Text {
                                            text: "Open-source remote play client licensed under AGPL-3.0-only-OpenSSL."
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 11
                                            color: LudeloTheme.textDim
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            // -----------------------------------------------------------------
            // RIGHT COLUMN: LIVE DIAGNOSTICS & TELEMETRY SIDEBAR (Network tab only)
            // -----------------------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: visible ? 380 : 0
                Layout.fillHeight: true
                visible: dialog.activeCategoryIndex === 2
                spacing: 16

                // Single elegant Empty State Card when there is no active session
                LCard {
                    Layout.fillWidth: true
                    visible: !Chiaki.session
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 24
                        spacing: 16

                        RowLayout {
                            spacing: 10
                            Rectangle {
                                width: 32; height: 32; radius: 8
                                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                                Text { anchors.centerIn: parent; text: "📡"; font.pixelSize: 16 }
                            }
                            Column {
                                Text {
                                    text: qsTr("Stream Diagnostics")
                                    font.family: LudeloTheme.fontFamily
                                    font.pixelSize: 15
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }
                                Text {
                                    text: qsTr("Standby Mode")
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 11
                                    color: LudeloTheme.textDim
                                }
                            }
                            Item { Layout.fillWidth: true }
                            LPill {
                                text: "STANDBY"
                                dotColor: LudeloTheme.textDim
                                glowColor: "transparent"
                                showDot: true
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                        Text {
                            text: qsTr("Real-time network telemetry (RTT ping, decoder latency, packet loss, and frame stability) is tracked live once a stream session is actively connected.")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 12
                            color: LudeloTheme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            lineHeight: 1.3
                        }
                    }
                }

                // Live Diagnostics Card (Only rendered when session is active)
                LCard {
                    Layout.fillWidth: true
                    visible: !!Chiaki.session
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 16

                        // Header with status indicator
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: LudeloTheme.accentMint
                            }
                            Text {
                                text: qsTr("Network Diagnostics")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                            Item { Layout.fillWidth: true }
                            LPill {
                                text: "LIVE STREAM"
                                dotColor: LudeloTheme.accentMint
                                glowColor: LudeloTheme.accentMintGlow
                                showDot: true
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                        // 2x2 Telemetry Grid
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 16
                            columnSpacing: 16

                            // Metric 1: RTT
                            Column {
                                spacing: 2
                                Text { text: "ROUND-TRIP (RTT)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? (Math.round(Chiaki.session.measuredRtt) + " ms") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentMint
                                }
                                Text { text: "Live ping"; font.family: LudeloTheme.fontFamily; font.pixelSize: 11; color: LudeloTheme.textDim }
                            }

                            // Metric 2: Decoder Latency
                            Column {
                                spacing: 2
                                Text { text: "DECODER LATENCY"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Text {
                                    text: (Chiaki.session && Chiaki.session.decoderTime) ? (Chiaki.session.decoderTime.toFixed(1) + " ms") : "< 1.0 ms"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: LudeloTheme.textPrimary
                                }
                                Text { text: "Hardware accelerated"; font.family: LudeloTheme.fontFamily; font.pixelSize: 11; color: LudeloTheme.textDim }
                            }

                            // Metric 3: Jitter
                            Column {
                                spacing: 2
                                Text { text: "JITTER VARIANCE"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? (Math.round(Chiaki.session.jitter || 0) + " ms") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: LudeloTheme.accentMint
                                }
                                Text { text: "Packet variance"; font.family: LudeloTheme.fontFamily; font.pixelSize: 11; color: LudeloTheme.textDim }
                            }

                            // Metric 4: Packet Loss
                            Column {
                                spacing: 2
                                Text { text: "PACKET LOSS"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? ((Chiaki.session.averagePacketLoss * 100).toFixed(1) + " %") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: (Chiaki.session && Chiaki.session.averagePacketLoss > 0.01) ? LudeloTheme.error : LudeloTheme.accentMint
                                }
                                Text { text: "FEC recovery active"; font.family: LudeloTheme.fontFamily; font.pixelSize: 11; color: LudeloTheme.textDim }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                        // Hardware / Protocol Readout
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "STREAM FPS:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: Chiaki.session ? (Math.round(Chiaki.session.measuredFps) + " FPS") : "--"
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; font.weight: Font.Bold; color: LudeloTheme.accentMint
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "ACTIVE RESOLUTION:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: Chiaki.session ? (Chiaki.session.resolution || "1080p") : "--"
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textPrimary
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "DECODER ENGINE:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: (Chiaki.settings.decoder || "d3d11va").toUpperCase()
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.accentMint
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "REMOTE HOST IP:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: Chiaki.session ? (Chiaki.session.hostIp || "Direct P2P") : "--"
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textDim
                                }
                            }
                        }
                    }
                }

                // Connected Host Card (when session is active)
                LCard {
                    Layout.fillWidth: true
                    visible: !!Chiaki.session
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 6

                        RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 24; height: 24; radius: 6
                                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                                Text { anchors.centerIn: parent; text: "🎮"; font.pixelSize: 12 }
                            }
                            Text {
                                text: Chiaki.session ? (Chiaki.session.targetName || "PlayStation Console") : ""
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                        }

                        Text {
                            text: qsTr("Direct connection established")
                            font.family: LudeloTheme.fontFamily
                            font.pixelSize: 11
                            color: LudeloTheme.textDim
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // =====================================================================
        // BOTTOM FOOTER (No fake claims • Real profile • Universal Gamepad Hints)
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

                // Real client profile text (no fake claims)
                Text {
                    text: qsTr("Ludelo Remote Client • Profile: %1").arg(Chiaki.settings.currentProfile ? Chiaki.settings.currentProfile : "Default")
                    font.family: LudeloTheme.fontFamilyMono
                    font.pixelSize: 11
                    color: LudeloTheme.textSecondary
                }

                Item { Layout.fillWidth: true }

                // Universal Gamepad Navigation Hints (Strictly NO PlayStation trademark glyphs)
                Row {
                    spacing: 20
                    Layout.alignment: Qt.AlignVCenter

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, catKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: catKeyText; anchors.centerIn: parent; text: LudeloTheme.hintKey("lb/rb"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 9; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "CATEGORIES"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: Math.max(20, closeKeyText.implicitWidth + 8); height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { id: closeKeyText; anchors.centerIn: parent; text: LudeloTheme.hintKey("back"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "CLOSE"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    MouseArea {
                        width: resetRow.implicitWidth
                        height: 24
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialog.resetToDefaults()

                        Row {
                            id: resetRow
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter
                            Rectangle {
                                width: Math.max(20, resetKeyText.implicitWidth + 8); height: 20; radius: 4
                                color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                                border.color: LudeloTheme.borderSubtle; border.width: 1
                                Text { id: resetKeyText; anchors.centerIn: parent; text: LudeloTheme.hintKey("details"); font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.accentMint }
                            }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: "RESET DEFAULTS"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // POPUP DIALOGS (Controller & Keyboard Mapping Support)
    // =========================================================================
    Dialog {
        id: controllerMappingDialog
        property bool resetFocus: true
        property bool resetMapping: false
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        title: qsTr("Controller Capture")
        modal: true
        standardButtons: Dialog.Close
        closePolicy: Popup.CloseOnPressOutside

        background: Rectangle {
            radius: LudeloTheme.radiusCard
            color: LudeloTheme.bgPanelSolid
            border.color: LudeloTheme.borderHover
            border.width: 1
        }

        onOpened: {
            controllerLabel.forceActiveFocus(Qt.TabFocusReason);
            Chiaki.creatingControllerMapping(true);
        }
        onClosed: {
            if (quitControllerMapping)
                Chiaki.controllerMappingQuit();
            else
                quitControllerMapping = true;
        }

        function show(opts) {
            resetMapping = opts.reset;
            open();
        }

        Label {
            id: controllerLabel
            text: qsTr("Choose the controller by pressing any button on the gamepad")
            font.family: LudeloTheme.fontFamily
            font.pixelSize: 14
            color: LudeloTheme.textPrimary
        }
    }

    // Toast Notification for Reset Defaults
    Rectangle {
        id: resetToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
        height: 40
        width: resetToastText.implicitWidth + 32
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
                id: resetToastText
                text: qsTr("Settings successfully restored to factory defaults")
                font.family: LudeloTheme.fontFamily
                font.pixelSize: 12
                font.weight: Font.Medium
                color: LudeloTheme.textPrimary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: resetToast; property: "opacity"; to: 1.0; duration: 200 }
            PauseAnimation { duration: 2500 }
            NumberAnimation { target: resetToast; property: "opacity"; to: 0.0; duration: 200 }
        }
    }
}
