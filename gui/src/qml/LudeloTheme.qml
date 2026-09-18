pragma Singleton
import QtQuick

QtObject {
    // Backgrounds
    readonly property color bgBase: "#0B0E14"
    readonly property color bgDark: "#0B0E14"
    readonly property color bgPanel: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.85)
    readonly property color bgPanelSolid: "#151923"
    readonly property color bgElevated: "#1C2230"
    readonly property color bgHover: Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.95)
    readonly property color bgCard: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.85)
    readonly property color bgCardHover: Qt.rgba(0x1C/255, 0x22/255, 0x30/255, 0.95)
    readonly property color bgDialog: Qt.rgba(0x15/255, 0x19/255, 0x23/255, 0.96)
    readonly property color bgInput: Qt.rgba(0x0B/255, 0x0E/255, 0x14/255, 0.85)
    readonly property color bgGlassOverlay: Qt.rgba(1.0, 1.0, 1.0, 0.04)

    // Accents
    readonly property color accentPrimary: "#6C5CE7"
    readonly property color accent: "#6C5CE7"
    readonly property color accentHover: "#7C6CF2"
    readonly property color accentElevated: "#7C6CF2"
    readonly property color accentGlow: Qt.rgba(0x6C/255, 0x5C/255, 0xE7/255, 0.40)
    readonly property color accentMint: "#00F5D4"
    readonly property color accentMintGlow: Qt.rgba(0x00/255, 0xF5/255, 0xD4/255, 0.35)
    readonly property color accentAmber: "#FFB703"
    readonly property color accentAmberGlow: Qt.rgba(0xFF/255, 0xB7/255, 0x03/255, 0.35)

    // Status Colors
    readonly property color warn: "#FFB703"
    readonly property color colorWarning: "#FFB703"
    readonly property color error: "#EF476F"
    readonly property color colorDanger: "#EF476F"
    readonly property color colorDangerGlow: Qt.rgba(0xEF/255, 0x47/255, 0x6F/255, 0.35)
    readonly property color info: "#6C5CE7"

    // Typography Colors
    readonly property color textPrimary: "#F2F4F8"
    readonly property color textSecondary: "#9AA4B2"
    readonly property color textDim: "#5A6472"
    readonly property color textMuted: "#5A6472"

    // Borders
    readonly property color borderSubtle: Qt.rgba(1.0, 1.0, 1.0, 0.08)
    readonly property color borderMedium: Qt.rgba(1.0, 1.0, 1.0, 0.16)
    readonly property color borderHover: Qt.rgba(0x7C/255, 0x6C/255, 0xF2/255, 0.50)
    readonly property color borderFocus: "#6C5CE7"
    readonly property color borderSuccess: "#00F5D4"

    // Radii
    readonly property int radiusCard: 16
    readonly property int radiusDialog: 20
    readonly property int radiusButton: 10
    readonly property int radiusPill: 9999
    readonly property int radiusInput: 10

    // Elevation & Glow
    readonly property int glowBlur: 24
    readonly property real glowAlpha: 0.35

    // Animation Timings (ms)
    readonly property int animFast: 150
    readonly property int animNormal: 180
    readonly property int animSlow: 250
    readonly property int easingCurve: Easing.OutQuad

    // Typography
    readonly property string fontFamily: "Inter"
    readonly property string fontFamilyMono: "JetBrains Mono"
}
