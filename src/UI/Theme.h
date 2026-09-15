// Archivo: src/UI/Theme.h
// Ludelo Design System V1 2026 — Central Design Tokens
#pragma once

#include <imgui.h>

namespace ludelo::ui::theme {

namespace colors {
    // Core Backgrounds
    constexpr ImVec4 kBase             = {0.055f, 0.067f, 0.090f, 1.00f};  // #0E1117 Deep Dark Void Base
    constexpr ImVec4 kBackground       = kBase;
    constexpr ImVec4 kPanel            = {0.090f, 0.106f, 0.141f, 0.92f};  // #171B24 Glass Panel Surface
    constexpr ImVec4 kSurface          = kPanel;
    constexpr ImVec4 kSurfaceHover     = {0.125f, 0.145f, 0.190f, 0.96f};  // Lighter Glass Hover
    constexpr ImVec4 kSurfaceActive    = {0.150f, 0.175f, 0.230f, 1.00f};  // Panel Active
    constexpr ImVec4 kGlassCard        = {0.090f, 0.106f, 0.141f, 0.85f};
    constexpr ImVec4 kGlassBorder      = {0.200f, 0.235f, 0.320f, 0.40f};  // Subtle border

    // Ludelo Brand Accent (Electric Indigo)
    constexpr ImVec4 kPrimary          = {0.424f, 0.361f, 0.906f, 1.00f};  // #6C5CE7 Electric Indigo
    constexpr ImVec4 kPrimaryHover     = {0.490f, 0.435f, 0.941f, 1.00f};  // #7D6FF0 Indigo Hover
    constexpr ImVec4 kPrimaryActive    = {0.350f, 0.290f, 0.800f, 1.00f};  // Pressed
    constexpr ImVec4 kPrimaryGlow      = {0.424f, 0.361f, 0.906f, 0.35f};

    // States
    constexpr ImVec4 kAccent           = {0.000f, 0.961f, 0.831f, 1.00f};  // #00F5D4 Cyber Mint (Awake / Active)
    constexpr ImVec4 kAccentHover      = {0.200f, 0.980f, 0.880f, 1.00f};  
    constexpr ImVec4 kAccentGlow       = {0.000f, 0.961f, 0.831f, 0.35f};  
    constexpr ImVec4 kSuccess          = kAccent;

    constexpr ImVec4 kAlert            = {1.000f, 0.718f, 0.012f, 1.00f};  // #FFB703 Amber (Standby / Aviso)
    constexpr ImVec4 kWarning          = kAlert;
    constexpr ImVec4 kError            = {0.937f, 0.278f, 0.435f, 1.00f};  // #EF476F Coral Red (Offline / Error)

    // Typography & Content
    constexpr ImVec4 kTextPrimary      = {0.973f, 0.976f, 0.980f, 1.00f};  // #F8F9FA Pure Crisp White
    constexpr ImVec4 kTextSecondary    = {0.580f, 0.639f, 0.722f, 1.00f};  // #94A3B8 Slate Grey
    constexpr ImVec4 kTextMuted        = {0.392f, 0.455f, 0.545f, 1.00f};  // #64748B Deep Slate

    constexpr ImVec4 kCloudPurple      = {0.424f, 0.361f, 0.906f, 1.00f};  // #6C5CE7 Electric Indigo
    constexpr ImVec4 kCloudPurpleGlow  = {0.424f, 0.361f, 0.906f, 0.35f};

    // IM_COL32 helpers
    inline ImU32 col32_base()          { return IM_COL32(14, 17, 23, 255); }
    inline ImU32 col32_panel()         { return IM_COL32(23, 27, 36, 235); }
    inline ImU32 col32_panel_hover()   { return IM_COL32(32, 37, 49, 245); }
    inline ImU32 col32_primary()       { return IM_COL32(108, 92, 231, 255); }
    inline ImU32 col32_accent()        { return IM_COL32(0, 245, 212, 255); }
    inline ImU32 col32_alert()         { return IM_COL32(255, 183, 3, 255); }
    inline ImU32 col32_error()         { return IM_COL32(239, 71, 111, 255); }
    inline ImU32 col32_border()        { return IM_COL32(51, 60, 82, 120); }
} // namespace colors

namespace metrics {
    constexpr float kRadiusComponent   = 12.0f; // Buttons, input fields, badges
    constexpr float kRadiusCard        = 16.0f; // Dashboard cards, side panels
    constexpr float kRadiusModal       = 20.0f; // Dialogs, test panels, modals
} // namespace metrics

// Helper to draw clean elevation border
inline void draw_elevation(ImDrawList* draw, ImVec2 min, ImVec2 max, int level, float radius = metrics::kRadiusCard) {
    if (!draw || level <= 0) return;
    if (level == 1) {
        draw->AddRect(min, max, IM_COL32(108, 92, 231, 50), radius, 0, 1.0f);
    } else if (level == 2) {
        draw->AddRect(ImVec2(min.x - 1, min.y - 1), ImVec2(max.x + 1, max.y + 1),
            IM_COL32(108, 92, 231, 80), radius + 1.0f, 0, 2.0f);
        draw->AddRect(min, max, IM_COL32(108, 92, 231, 160), radius, 0, 1.2f);
    } else if (level >= 3) {
        draw->AddRect(ImVec2(min.x - 2, min.y - 2), ImVec2(max.x + 2, max.y + 2),
            IM_COL32(0, 245, 212, 100), radius + 2.0f, 0, 3.0f);
        draw->AddRect(min, max, IM_COL32(0, 245, 212, 255), radius, 0, 1.8f);
    }
}

} // namespace ludelo::ui::theme
