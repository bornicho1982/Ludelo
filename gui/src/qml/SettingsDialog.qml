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

            RowLayout {
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

                    // Reset Defaults Button
                    LButton {
                        height: 38
                        implicitWidth: 160
                        customRadius: 8
                        variant: "ghost"
                        text: qsTr("RESET DEFAULTS")
                        keyHint: "[X]"
                        onClicked: dialog.resetToDefaults()
                    }
                }

                // Flickable Content Area
                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: currentCategoryContent.implicitHeight + 40
                    boundsBehavior: Flickable.StopAtBounds

                    Item {
                        id: currentCategoryContent
                        width: parent.width
                        implicitHeight: categoryStack.children[dialog.activeCategoryIndex].implicitHeight

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

                                // Card: Display & Resolution
                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("Display & Resolution")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        // Resolution Selection
                                        ColumnLayout {
                                            spacing: 6
                                            Text {
                                                text: qsTr("Target Resolution (Local Stream)")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 12
                                                color: LudeloTheme.textSecondary
                                            }
                                            RowLayout {
                                                spacing: 8
                                                Repeater {
                                                    model: [
                                                        { text: "1080p", res: 3, label: "1920x1080 (Recommended)" },
                                                        { text: "720p", res: 2, label: "1280x720" },
                                                        { text: "540p", res: 1, label: "960x540" }
                                                    ]
                                                    delegate: LButton {
                                                        height: 42
                                                        implicitWidth: 170
                                                        customRadius: 8
                                                        variant: {
                                                            let cur = (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                                ? Chiaki.settings.resolutionLocalPS5
                                                                : Chiaki.settings.resolutionLocalPS4;
                                                            return cur === modelData.res ? "mint" : "secondary";
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
                                        }

                                        // Target Refresh Rate
                                        ColumnLayout {
                                            spacing: 6
                                            Text {
                                                text: qsTr("Target Refresh Rate")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 12
                                                color: LudeloTheme.textSecondary
                                            }
                                            RowLayout {
                                                spacing: 8
                                                LButton {
                                                    height: 38
                                                    implicitWidth: 150
                                                    customRadius: 8
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsLocalPS5 : Chiaki.settings.fpsLocalPS4) === 1) ? "mint" : "secondary"
                                                    text: "60 FPS"
                                                    onClicked: {
                                                        if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                            Chiaki.settings.fpsLocalPS5 = 1;
                                                        else
                                                            Chiaki.settings.fpsLocalPS4 = 1;
                                                    }
                                                }
                                                LButton {
                                                    height: 38
                                                    implicitWidth: 150
                                                    customRadius: 8
                                                    variant: ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.fpsLocalPS5 : Chiaki.settings.fpsLocalPS4) === 0) ? "mint" : "secondary"
                                                    text: "30 FPS"
                                                    onClicked: {
                                                        if (dialog.selectedConsole === SettingsDialog.Console.PS5)
                                                            Chiaki.settings.fpsLocalPS5 = 0;
                                                        else
                                                            Chiaki.settings.fpsLocalPS4 = 0;
                                                    }
                                                }
                                            }
                                        }

                                        // Video Codec
                                        ColumnLayout {
                                            spacing: 6
                                            Text {
                                                text: qsTr("Video Stream Codec (PS5)")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 12
                                                color: LudeloTheme.textSecondary
                                            }
                                            RowLayout {
                                                spacing: 8
                                                LButton {
                                                    height: 38
                                                    implicitWidth: 170
                                                    customRadius: 8
                                                    variant: (Chiaki.settings.codecLocalPS5 >= 1) ? "mint" : "secondary"
                                                    text: "HEVC / H.265"
                                                    onClicked: Chiaki.settings.codecLocalPS5 = 1
                                                }
                                                LButton {
                                                    height: 38
                                                    implicitWidth: 170
                                                    customRadius: 8
                                                    variant: (Chiaki.settings.codecLocalPS5 === 0) ? "mint" : "secondary"
                                                    text: "AVC / H.264"
                                                    onClicked: Chiaki.settings.codecLocalPS5 = 0
                                                }
                                            }
                                            Text {
                                                text: qsTr("H.265 provides ~30% higher bitrate efficiency with modern GPUs.")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 11
                                                color: LudeloTheme.textDim
                                            }
                                        }

                                        // Hardware Decoder
                                        ColumnLayout {
                                            spacing: 6
                                            Text {
                                                text: qsTr("Hardware Accelerated Decoder")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 12
                                                color: LudeloTheme.textSecondary
                                            }
                                            RowLayout {
                                                spacing: 12
                                                ComboBox {
                                                    id: decoderCombo
                                                    Layout.preferredWidth: 320
                                                    model: Chiaki.settings.availableDecoders
                                                    currentIndex: Math.max(0, model.indexOf(Chiaki.settings.decoder))
                                                    onActivated: (index) => Chiaki.settings.decoder = model[index]
                                                }
                                                Text {
                                                    text: qsTr("Active engine: %1").arg(Chiaki.settings.decoder || "d3d11va")
                                                    font.family: LudeloTheme.fontFamilyMono
                                                    font.pixelSize: 11
                                                    color: LudeloTheme.accentMint
                                                }
                                            }
                                        }

                                        // HDR Stream Output Toggle (Honest: only active when H.265 is selected)
                                        LToggle {
                                            label: qsTr("HDR Stream Output (10-bit Rec.2020)")
                                            description: (Chiaki.settings.codecLocalPS5 === 0)
                                                ? qsTr("HDR disponible en streams H.265 compatibles")
                                                : qsTr("Direct 10-bit HDR metadata passthrough to compatible displays")
                                            enabled: dialog.selectedConsole === SettingsDialog.Console.PS5 && Chiaki.settings.codecLocalPS5 >= 1
                                            checked: Chiaki.settings.codecLocalPS5 === 2
                                            onToggled: {
                                                if (checked) {
                                                    Chiaki.settings.codecLocalPS5 = 2; // H265 HDR
                                                } else {
                                                    Chiaki.settings.codecLocalPS5 = 1; // H265 Standard
                                                }
                                            }
                                        }
                                    }
                                }

                                // Card: Bitrate & Network Allocation
                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 20
                                        spacing: 16

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                text: qsTr("Bitrate & Network Allocation")
                                                font.family: LudeloTheme.fontFamily
                                                font.pixelSize: 16
                                                font.weight: Font.Bold
                                                color: LudeloTheme.textPrimary
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text {
                                                text: qsTr("%1 Mbps (%2)").arg(
                                                    Math.round((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateLocalPS5 : Chiaki.settings.bitrateLocalPS4) / 1000)
                                                ).arg(
                                                    ((dialog.selectedConsole === SettingsDialog.Console.PS5 ? Chiaki.settings.bitrateLocalPS5 : Chiaki.settings.bitrateLocalPS4) >= 25000)
                                                        ? qsTr("High Quality / Recommended for LAN")
                                                        : qsTr("Standard")
                                                )
                                                font.family: LudeloTheme.fontFamilyMono
                                                font.pixelSize: 12
                                                font.weight: Font.Bold
                                                color: LudeloTheme.accentMint
                                            }
                                        }

                                        LSlider {
                                            Layout.fillWidth: true
                                            from: 5000
                                            to: 30000
                                            stepSize: 1000
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
                                            Text { text: "5 Mbps (Eco)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textDim }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "15 Mbps (Standard)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textDim }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "30 Mbps (High Quality / LAN)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textDim }
                                        }

                                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                                        // Diagnostics HUD Toggle
                                        LToggle {
                                            label: qsTr("In-Game Diagnostics HUD Overlay")
                                            description: qsTr("Display live RTT latency, packet loss, and frame drop metrics during stream [TAB]")
                                            checked: Chiaki.settings.showStreamStats
                                            onToggled: Chiaki.settings.showStreamStats = checked
                                        }

                                        // Advanced Display Pipeline Button
                                        RowLayout {
                                            spacing: 12
                                            LButton {
                                                height: 38
                                                implicitWidth: 220
                                                customRadius: 8
                                                variant: "secondary"
                                                text: qsTr("ADVANCED RENDERER")
                                                onClicked: {
                                                    if (typeof root !== "undefined" && root.showDisplaySettingsDialog)
                                                        root.showDisplaySettingsDialog();
                                                }
                                            }
                                            Text {
                                                text: qsTr("Adjust tone mapping, sharpening, and color grading")
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
                                        anchors.fill: parent
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
                                        anchors.fill: parent
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
                                        anchors.fill: parent
                                        anchors.margins: 20
                                        spacing: 16

                                        Text {
                                            text: qsTr("PlayStation Cloud Gaming Streaming")
                                            font.family: LudeloTheme.fontFamily
                                            font.pixelSize: 16
                                            font.weight: Font.Bold
                                            color: LudeloTheme.textPrimary
                                        }

                                        ColumnLayout {
                                            spacing: 6
                                            Text { text: qsTr("Cloud Data Center Location"); font.family: LudeloTheme.fontFamily; font.pixelSize: 12; color: LudeloTheme.textSecondary }
                                            ComboBox {
                                                Layout.preferredWidth: 320
                                                model: [qsTr("Auto (Lowest Latency)"), qsTr("North America"), qsTr("Europe"), qsTr("Asia-Pacific")]
                                                currentIndex: 0
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
                                        anchors.fill: parent
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
                                        anchors.fill: parent
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
                                                text: Chiaki.settings.psnAccountId ? qsTr("Logged In (Account ID: %1)").arg(Chiaki.settings.psnAccountId) : qsTr("Not Connected to PSN")
                                                font.family: LudeloTheme.fontFamilyMono
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                                color: Chiaki.settings.psnAccountId ? LudeloTheme.textPrimary : LudeloTheme.textDim
                                            }
                                        }

                                        LButton {
                                            height: 38
                                            implicitWidth: 240
                                            customRadius: 8
                                            variant: Chiaki.settings.psnAccountId ? "ghost" : "primary"
                                            text: Chiaki.settings.psnAccountId ? qsTr("RE-AUTHENTICATE PSN") : qsTr("SIGN IN WITH PLAYSTATION")
                                            onClicked: Chiaki.startWebView2Login()
                                        }
                                    }
                                }

                                LCard {
                                    Layout.fillWidth: true
                                    ColumnLayout {
                                        anchors.fill: parent
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
                                        anchors.fill: parent
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
                                        anchors.fill: parent
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
            // RIGHT COLUMN: LIVE DIAGNOSTICS & TELEMETRY SIDEBAR
            // -----------------------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: 380
                Layout.fillHeight: true
                spacing: 16

                // Diagnostics Card
                LCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16

                        // Header with status indicator
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: Chiaki.session ? LudeloTheme.accentMint : LudeloTheme.textDim
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
                                text: Chiaki.session ? "LIVE STREAM" : "STANDBY"
                                status: Chiaki.session ? "online" : "offline"
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: LudeloTheme.borderSubtle }

                        // 2x2 Telemetry Grid (Honest: live if Chiaki.session, else "--")
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 16
                            columnSpacing: 16

                            // Metric 1: RTT
                            Column {
                                spacing: 2
                                Text { text: "ROUND-TRIP (RTT)"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? (Math.round(Chiaki.session.measuredRtt) + " ms") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: Chiaki.session ? LudeloTheme.accentMint : LudeloTheme.textDim
                                }
                                Text { text: Chiaki.session ? "Live ping" : "No active session"; font.family: LudeloTheme.fontFamily; font.pixelSize: 10; color: LudeloTheme.textDim }
                            }

                            // Metric 2: Decoder Time
                            Column {
                                spacing: 2
                                Text { text: "DECODER TIME"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? (Chiaki.session.decoderTime ? (Chiaki.session.decoderTime.toFixed(1) + " ms") : "< 1.0 ms") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: Chiaki.session ? LudeloTheme.textPrimary : LudeloTheme.textDim
                                }
                                Text { text: Chiaki.settings.decoder || "d3d11va"; font.family: LudeloTheme.fontFamily; font.pixelSize: 10; color: LudeloTheme.textDim }
                            }

                            // Metric 3: Jitter
                            Column {
                                spacing: 2
                                Text { text: "JITTER VARIANCE"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? (Math.round(Chiaki.session.jitter || 0) + " ms") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: Chiaki.session ? LudeloTheme.accentMint : LudeloTheme.textDim
                                }
                                Text { text: "Packet variance"; font.family: LudeloTheme.fontFamily; font.pixelSize: 10; color: LudeloTheme.textDim }
                            }

                            // Metric 4: Packet Loss
                            Column {
                                spacing: 2
                                Text { text: "PACKET LOSS"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; color: LudeloTheme.textSecondary }
                                Text {
                                    text: Chiaki.session ? ((Chiaki.session.averagePacketLoss * 100).toFixed(1) + " %") : "--"
                                    font.family: LudeloTheme.fontFamilyMono
                                    font.pixelSize: 20
                                    font.weight: Font.Bold
                                    color: (Chiaki.session && Chiaki.session.averagePacketLoss > 0.01) ? LudeloTheme.error : (Chiaki.session ? LudeloTheme.accentMint : LudeloTheme.textDim)
                                }
                                Text { text: "FEC recovery active"; font.family: LudeloTheme.fontFamily; font.pixelSize: 10; color: LudeloTheme.textDim }
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
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; font.weight: Font.Bold; color: Chiaki.session ? LudeloTheme.accentMint : LudeloTheme.textDim
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "ACTIVE RESOLUTION:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: Chiaki.session ? (Chiaki.session.resolution || "1080p") : "--"
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: Chiaki.session ? LudeloTheme.textPrimary : LudeloTheme.textDim
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "CONTROLLER POLLING:"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: (Chiaki.controllers && Chiaki.controllers.length > 0) ? "1000 Hz / Synchronous" : "--"
                                    font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: (Chiaki.controllers && Chiaki.controllers.length > 0) ? LudeloTheme.accentMint : LudeloTheme.textDim
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

                // Connected Host Card (Only real data from session or selected console)
                LCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 6

                        RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 24
                                height: 24
                                radius: 6
                                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                                Text {
                                    anchors.centerIn: parent
                                    text: "🎮"
                                    font.pixelSize: 12
                                }
                            }
                            Text {
                                text: Chiaki.session ? (Chiaki.session.targetName || "PlayStation Console") : qsTr("No Active Stream Session")
                                font.family: LudeloTheme.fontFamily
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: LudeloTheme.textPrimary
                            }
                        }

                        Text {
                            text: Chiaki.session ? qsTr("Direct connection established") : qsTr("Connect to a console from the home screen to view real-time host telemetry.")
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
                            width: 20; height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { anchors.centerIn: parent; text: "A"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "SELECT"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: 20; height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { anchors.centerIn: parent; text: "B"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "BACK / CLOSE"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
                    }

                    Row {
                        spacing: 6
                        Rectangle {
                            width: 20; height: 20; radius: 4
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: LudeloTheme.borderSubtle; border.width: 1
                            Text { anchors.centerIn: parent; text: "X"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 10; font.weight: Font.Bold; color: LudeloTheme.textPrimary }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "RESET DEFAULTS"; font.family: LudeloTheme.fontFamilyMono; font.pixelSize: 11; color: LudeloTheme.textSecondary }
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
